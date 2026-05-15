/*
 * Copyright (c) 2024 ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * Runtime animation control for the claude_view display (peripheral half).
 */

#pragma once

#include <zephyr/types.h>

uint8_t zmk_claude_view_get_animation(void);
void zmk_claude_view_set_animation(uint8_t idx);
void zmk_claude_view_animation_dirty(void);
