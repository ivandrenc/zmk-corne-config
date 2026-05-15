/*
 * Copyright (c) 2024 ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * Animation index store for the claude_view display.
 *
 * Provides zmk_claude_view_get_animation() / zmk_claude_view_set_animation()
 * for use by custom_status_screen.c.
 *
 * Runtime keybinding switching requires cross-split ZMK event forwarding
 * (central → peripheral), which is deferred to a future implementation.
 * For now the index stays at 0 (first animation) after flashing; you can
 * change the default by editing INITIAL_ANIM below and reflashing.
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
