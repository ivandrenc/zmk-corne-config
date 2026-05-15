/*
 * Copyright (c) 2024 ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * ZMK behaviors for switching the claude_view display animation at runtime.
 *
 * Two behaviors are provided:
 *   &anim_select N   — jump directly to animation index N
 *   &anim_next       — cycle forward through all animations (wraps around)
 *
 * The animation index is stored as a Zephyr atomic so reads from the display
 * work queue and writes from the key-event work queue are safe.
 *
 * custom_status_screen.c polls zmk_claude_view_get_animation() via an
 * lv_timer every 50 ms and calls lv_animimg_set_src() when the index changes.
 */

#include <zephyr/device.h>
#include <zephyr/sys/atomic.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>

#include "../assets/claude_art.h"   /* animation_count */

/* ── Shared state ──────────────────────────────────────────────────────── */

static atomic_t anim_idx = ATOMIC_INIT(0);

uint8_t zmk_claude_view_get_animation(void) {
    return (uint8_t)atomic_get(&anim_idx);
}

/* ── &anim_select N ────────────────────────────────────────────────────── */

#define DT_DRV_COMPAT zmk_behavior_anim_select

static int anim_select_pressed(struct zmk_behavior_binding *binding,
                                struct zmk_behavior_binding_event event) {
    atomic_set(&anim_idx, binding->param1 % animation_count);
    return ZMK_BEHAVIOR_OPAQUE;
}

static int anim_select_released(struct zmk_behavior_binding *binding,
                                 struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api anim_select_api = {
    .binding_pressed  = anim_select_pressed,
    .binding_released = anim_select_released,
};

BEHAVIOR_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL,
                         POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                         &anim_select_api);

#undef DT_DRV_COMPAT

/* ── &anim_next ────────────────────────────────────────────────────────── */

#define DT_DRV_COMPAT zmk_behavior_anim_next

static int anim_next_pressed(struct zmk_behavior_binding *binding,
                              struct zmk_behavior_binding_event event) {
    atomic_t old = atomic_get(&anim_idx);
    atomic_set(&anim_idx, ((uint8_t)old + 1) % animation_count);
    return ZMK_BEHAVIOR_OPAQUE;
}

static int anim_next_released(struct zmk_behavior_binding *binding,
                               struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api anim_next_api = {
    .binding_pressed  = anim_next_pressed,
    .binding_released = anim_next_released,
};

BEHAVIOR_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL,
                         POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                         &anim_next_api);

#undef DT_DRV_COMPAT
