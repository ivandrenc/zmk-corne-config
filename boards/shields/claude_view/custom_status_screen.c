/*
 * Copyright (c) 2024 ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * Claude View - custom peripheral status screen for ZMK nice!view.
 * Shows a 4-frame Claude mascot on the right (peripheral) half:
 *   - Frame 0 shown statically when idle
 *   - Frames 1-3 cycle at 400 ms/frame while typing
 *   - Returns to frame 0 after 3 s of inactivity
 *
 * Uses only zephyr/kernel.h + lvgl.h + zmk event headers (no zmk/display.h).
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <lvgl.h>

#if IS_ENABLED(CONFIG_ZMK_WPM)
#include <zmk/event_manager.h>
#include <zmk/events/wpm_state_changed.h>
#endif

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
#define FRAME_COUNT       4
#define FRAME_INTERVAL_MS 400   /* ms per frame while typing */
#define IDLE_TIMEOUT_MS   3000  /* ms of silence before returning to frame 0 */

/* ── Shared typing flag (set from ZMK thread, read from LVGL thread) ─ */

static ATOMIC_DEFINE(typing_bits, 1);
#define TYPING_BIT 0

static struct k_timer idle_timer;

static void idle_timeout_cb(struct k_timer *tmr) {
    atomic_clear_bit(typing_bits, TYPING_BIT);
}
K_TIMER_DEFINE(idle_timer, idle_timeout_cb, NULL);

/* ── ZMK WPM event listener ─────────────────────────────────── */

#if IS_ENABLED(CONFIG_ZMK_WPM)
static int wpm_event_cb(const zmk_event_t *eh) {
    const struct zmk_wpm_state_changed *ev = as_zmk_wpm_state_changed(eh);
    if (ev != NULL && ev->state > 0) {
        atomic_set_bit(typing_bits, TYPING_BIT);
        k_timer_start(&idle_timer, K_MSEC(IDLE_TIMEOUT_MS), K_NO_WAIT);
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(claude_wpm_listener, wpm_event_cb);
ZMK_SUBSCRIPTION(claude_wpm_listener, zmk_wpm_state_changed);
#endif /* CONFIG_ZMK_WPM */

/* ── LVGL frame-advance timer (runs on the display work queue) ── */

static lv_obj_t *art_img    = NULL;
static uint8_t  cur_frame   = 0;

static void frame_tick(lv_timer_t *timer) {
    if (art_img == NULL) {
        return;
    }
    if (!atomic_test_bit(typing_bits, TYPING_BIT)) {
        /* Not typing: snap back to idle frame */
        if (cur_frame != 0) {
            cur_frame = 0;
            lv_img_set_src(art_img, claude_frames[0]);
        }
        return;
    }
    /* Typing: advance to next frame */
    cur_frame = (cur_frame + 1) % FRAME_COUNT;
    lv_img_set_src(art_img, claude_frames[cur_frame]);
}

/* ── Screen init (called once by ZMK display subsystem) ─────── */

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);

    art_img = lv_img_create(screen);
    lv_obj_remove_style_all(art_img);
    lv_img_set_src(art_img, claude_frames[0]);
    lv_obj_align(art_img, LV_ALIGN_CENTER, 0, 0);

    /* Tick every FRAME_INTERVAL_MS; advances only while typing */
    lv_timer_create(frame_tick, FRAME_INTERVAL_MS, NULL);

    return screen;
}
