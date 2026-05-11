/*
 * Copyright (c) 2024 ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * Claude View - custom peripheral status screen for ZMK nice!view.
 * Shows a 4-frame Claude mascot animation on the right (peripheral) half.
 * Animation cycles continuously at ~2.5fps (400ms per frame).
 */

#include <zephyr/kernel.h>
#include <lvgl.h>
#include <zmk/display.h>
#include <zmk/battery.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/split_peripheral_status_changed.h>
#include <zmk/split/bluetooth/peripheral.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/* ── Frame declarations ─────────────────────────────────────── */

LV_IMG_DECLARE(claude_frame_1);
LV_IMG_DECLARE(claude_frame_2);
LV_IMG_DECLARE(claude_frame_3);
LV_IMG_DECLARE(claude_frame_4);

static const lv_img_dsc_t *claude_anim_imgs[] = {
    &claude_frame_1,
    &claude_frame_2,
    &claude_frame_3,
    &claude_frame_4,
};

#define CLAUDE_FRAME_COUNT   4
#define ANIM_DURATION_MS     1600  /* total cycle: 400ms × 4 frames */

/* ── Battery widget ─────────────────────────────────────────── */

static lv_obj_t *bat_label;

struct battery_state {
    uint8_t level;
};

static void battery_update_cb(struct battery_state state) {
    if (bat_label == NULL) {
        return;
    }
    char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", state.level);
    lv_label_set_text(bat_label, buf);
}

static struct battery_state battery_get_state(const zmk_event_t *eh) {
    return (struct battery_state){
        .level = zmk_battery_state_of_charge(),
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_battery, struct battery_state,
                             battery_update_cb, battery_get_state)
ZMK_SUBSCRIPTION(widget_battery, zmk_battery_state_changed);

/* ── Connection widget ──────────────────────────────────────── */

static lv_obj_t *conn_label;

struct conn_state {
    bool connected;
};

static void conn_update_cb(struct conn_state state) {
    if (conn_label == NULL) {
        return;
    }
    lv_label_set_text(conn_label, state.connected ? "OK" : "--");
}

static struct conn_state conn_get_state(const zmk_event_t *eh) {
    return (struct conn_state){
        .connected = zmk_split_bt_peripheral_is_connected(),
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_conn, struct conn_state,
                             conn_update_cb, conn_get_state)
ZMK_SUBSCRIPTION(widget_conn, zmk_split_peripheral_status_changed);

/* ── Screen init ────────────────────────────────────────────── */

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);

    /* Animated mascot — 140×68, centered on the 160×68 display */
    lv_obj_t *art = lv_animimg_create(screen);
    lv_obj_remove_style_all(art);
    lv_animimg_set_src(art, (const void **)claude_anim_imgs, CLAUDE_FRAME_COUNT);
    lv_animimg_set_duration(art, ANIM_DURATION_MS);
    lv_animimg_set_repeat_count(art, LV_ANIM_REPEAT_INFINITE);
    lv_animimg_start(art);
    lv_obj_align(art, LV_ALIGN_CENTER, 0, 0);

    /* Battery % — small label, top-right corner */
    bat_label = lv_label_create(screen);
    lv_obj_set_style_text_font(bat_label, &lv_font_unscii_8, 0);
    lv_label_set_text(bat_label, "?%");
    lv_obj_align(bat_label, LV_ALIGN_TOP_RIGHT, -1, 2);

    /* BLE connection status — bottom-right corner */
    conn_label = lv_label_create(screen);
    lv_obj_set_style_text_font(conn_label, &lv_font_unscii_8, 0);
    lv_label_set_text(conn_label, "--");
    lv_obj_align(conn_label, LV_ALIGN_BOTTOM_RIGHT, -1, -2);

    widget_battery_init();
    widget_conn_init();

    return screen;
}
