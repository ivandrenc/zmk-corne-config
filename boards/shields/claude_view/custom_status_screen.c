/*
 * Copyright (c) 2024 ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * Claude View - custom peripheral status screen for ZMK nice!view.
 * Shows a 4-frame Claude mascot animation on the right (peripheral) half.
 * Animation cycles continuously at ~2.5fps (400ms per frame).
 *
 * Intentionally uses only <zephyr/kernel.h> and <lvgl.h> so that this
 * file compiles cleanly from a user-config module without needing
 * zmk/display.h (which is only in ZMK's own app/include tree).
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <lvgl.h>

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

/* ── Screen init ────────────────────────────────────────────── */

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);

    /* Animated mascot — 140×68px, centered on the 160×68 display */
    lv_obj_t *art = lv_animimg_create(screen);
    lv_obj_remove_style_all(art);
    lv_animimg_set_src(art, (const void **)claude_anim_imgs, 4);
    lv_animimg_set_duration(art, 1600); /* 400ms × 4 frames */
    lv_animimg_set_repeat_count(art, LV_ANIM_REPEAT_INFINITE);
    lv_animimg_start(art);
    lv_obj_align(art, LV_ALIGN_CENTER, 0, 0);

    return screen;
}
