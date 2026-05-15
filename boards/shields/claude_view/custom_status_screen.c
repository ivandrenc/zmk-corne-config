/*
 * Copyright (c) 2024 ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * Claude View — custom peripheral status screen for ZMK nice!view.
 *
 * Animation switching: Fn layer &anim_wave / &anim_gym / &anim_boo / &anim_cyc.
 * Central forwards via BEHAVIOR_LOCALITY_GLOBAL (split BLE run-behavior).
 * Updates are applied on the LVGL display work queue.
 */

#include <zephyr/kernel.h>
#include <lvgl.h>

#include <zmk/display.h>

#include "assets/claude_art.h"
#include "assets/claude_view_display.h"

static lv_obj_t *anim_widget = NULL;
static uint8_t   active_anim = UINT8_MAX;

static void apply_animation(uint8_t idx) {
    if (anim_widget == NULL || animation_count == 0) {
        return;
    }
    if (idx >= animation_count) {
        idx = 0;
    }
    if (idx == active_anim) {
        return;
    }
    const struct claude_animation *a = &animations[idx];
    lv_animimg_set_src(anim_widget, (const void **)a->frames, a->count);
    lv_animimg_set_duration(anim_widget, (uint32_t)a->ms_per_frame * a->count);
    lv_animimg_set_repeat_count(anim_widget, LV_ANIM_REPEAT_INFINITE);
    lv_animimg_start(anim_widget);
    active_anim = idx;
}

static void anim_apply_work_handler(struct k_work *work) {
    ARG_UNUSED(work);
    apply_animation(zmk_claude_view_get_animation());
}

static K_WORK_DEFINE(anim_apply_work, anim_apply_work_handler);

void zmk_claude_view_animation_dirty(void) {
    if (zmk_display_is_initialized()) {
        k_work_submit_to_queue(zmk_display_work_q(), &anim_apply_work);
    }
}

static void anim_refresh_cb(lv_timer_t *timer) {
    ARG_UNUSED(timer);
    uint8_t requested = zmk_claude_view_get_animation();
    if (requested != active_anim) {
        apply_animation(requested);
    }
}

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);

    anim_widget = lv_animimg_create(screen);
    lv_obj_align(anim_widget, LV_ALIGN_CENTER, 0, 0);

    apply_animation(zmk_claude_view_get_animation());

    lv_timer_create(anim_refresh_cb, 50, NULL);

    return screen;
}
