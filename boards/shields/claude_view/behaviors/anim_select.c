/*
 * Copyright (c) 2024 ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * Animation index store for the claude_view display.
 */

#include <zephyr/sys/atomic.h>
#include <zephyr/types.h>

#include "../assets/claude_art.h"
#include "../assets/claude_view_display.h"

#define INITIAL_ANIM 0   /* 0 = look, 1 = gym, 2 = confetti (boo disabled in manifest) */

static atomic_t anim_idx = ATOMIC_INIT(INITIAL_ANIM);

uint8_t zmk_claude_view_get_animation(void) {
    return (uint8_t)atomic_get(&anim_idx);
}

void zmk_claude_view_set_animation(uint8_t idx) {
    if (animation_count == 0) {
        return;
    }
    atomic_set(&anim_idx, idx % animation_count);
    zmk_claude_view_animation_dirty();
}
