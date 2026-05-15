/*
 * Copyright (c) 2024 ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * Animation index store for the claude_view display.
 *
 * Provides zmk_claude_view_get_animation() / zmk_claude_view_set_animation()
 * for use by custom_status_screen.c.
 *
 * BLE split + ZMK v0.3: the peripheral firmware does not include keymap or
 * layer_state_changed (central-only in app/CMakeLists.txt). Keys pressed on the
 * left half cannot update animation selection on the right half via behaviors.
 *
 * Pick default wave vs gym: INITIAL_ANIM below (then rebuild / flash right half).
 */

#include <zephyr/sys/atomic.h>
#include <zephyr/types.h>

#include "../assets/claude_art.h"

#define INITIAL_ANIM 0   /* 0 = wave,  1 = gym */

static atomic_t anim_idx = ATOMIC_INIT(INITIAL_ANIM);

uint8_t zmk_claude_view_get_animation(void) {
    return (uint8_t)atomic_get(&anim_idx);
}

void zmk_claude_view_set_animation(uint8_t idx) {
    atomic_set(&anim_idx, idx % animation_count);
}
