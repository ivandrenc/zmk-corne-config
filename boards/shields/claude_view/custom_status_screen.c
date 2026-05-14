/*
 * Copyright (c) 2024 ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * Claude View - custom peripheral status screen for ZMK nice!view.
 * Shows a 4-frame mascot animation on the right (peripheral) half.
 * Each keypress on this half advances to the next frame in order.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <lvgl.h>

#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/* ── Frame declarations ─────────────────────────────────────── */

LV_IMG_DECLARE(claude_frame_1);
LV_IMG_DECLARE(claude_frame_2);
LV_IMG_DECLARE(claude_frame_3);
LV_IMG_DECLARE(claude_frame_4);

static const lv_img_dsc_t *claude_frames[] = {
    &claude_frame_1,
    &claude_frame_2,
    &claude_frame_3,
    &claude_frame_4,
};
#define FRAME_COUNT 4

/* ── Keypress counter ───────────────────────────────────────── */
/*
 * press_count is incremented in the ZMK event thread whenever a key on
 * this peripheral half goes DOWN.  The LVGL timer reads it and advances
 * the displayed frame accordingly.  Atomic ops keep the two threads safe.
 */
static atomic_t press_count = ATOMIC_INIT(0);

static int position_state_cb(const zmk_event_t *eh) {
    const struct zmk_position_state_changed *ev = as_zmk_position_state_changed(eh);
    if (ev != NULL && ev->state) {   /* key pressed, not released */
        atomic_inc(&press_count);
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(mascot_keypress, position_state_cb);
ZMK_SUBSCRIPTION(mascot_keypress, zmk_position_state_changed);

/* ── LVGL frame-advance timer (runs on the display work queue) ── */

static lv_obj_t *art_img       = NULL;
static uint8_t   cur_frame     = 0;
static uint32_t  last_count    = 0;

static void frame_tick(lv_timer_t *timer) {
    if (art_img == NULL) {
        return;
    }
    uint32_t current = (uint32_t)atomic_get(&press_count);
    if (current == last_count) {
        return;
    }
    /* Advance by however many presses happened since last tick */
    uint32_t delta = current - last_count;
    last_count     = current;
    cur_frame      = (cur_frame + delta) % FRAME_COUNT;
    lv_img_set_src(art_img, claude_frames[cur_frame]);
}

/* ── Screen init (called once by ZMK display subsystem) ─────── */

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);

    art_img = lv_img_create(screen);
    lv_obj_remove_style_all(art_img);
    lv_img_set_src(art_img, claude_frames[0]);
    lv_obj_align(art_img, LV_ALIGN_CENTER, 0, 0);

    /* Poll for keypresses every 50 ms; fast enough to feel immediate */
    lv_timer_create(frame_tick, 50, NULL);

    return screen;
}
