/*
 * Copyright (c) 2024 ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * Claude View - custom peripheral status screen for ZMK nice!view.
 * Shows a 4-frame mascot animation that cycles continuously using
 * LVGL's lv_animimg widget (no ZMK event headers required).
 */

#include <zephyr/kernel.h>
#include <lvgl.h>

/* ── Frame declarations ─────────────────────────────────────── */

LV_IMG_DECLARE(claude_frame_1);
LV_IMG_DECLARE(claude_frame_2);
LV_IMG_DECLARE(claude_frame_3);
LV_IMG_DECLARE(claude_frame_4);

/* 6-entry sequence produces a smooth back-and-forth wave: 1→2→3→4→3→2 */
static const lv_img_dsc_t *claude_frames[] = {
    &claude_frame_1,
    &claude_frame_2,
    &claude_frame_3,
    &claude_frame_4,
    &claude_frame_3,
    &claude_frame_2,
};
#define FRAME_COUNT  6
#define MS_PER_FRAME 200   /* 200 ms per frame → 1.2 s full cycle */

/* ── Screen init (called once by ZMK display subsystem) ─────── */

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);

    lv_obj_t *art = lv_animimg_create(screen);
    lv_animimg_set_src(art, (const void **)claude_frames, FRAME_COUNT);
    lv_animimg_set_duration(art, MS_PER_FRAME * FRAME_COUNT);
    lv_animimg_set_repeat_count(art, LV_ANIM_REPEAT_INFINITE);
    lv_animimg_start(art);
    lv_obj_align(art, LV_ALIGN_CENTER, 0, 0);

    return screen;
}
