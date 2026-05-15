/*
 * Copyright (c) 2024 ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * Select Claude View animation by index on the display half (GLOBAL locality,
 * same split forwarding path as RGB underglow).
 */

#include <zephyr/devicetree.h>

#if DT_NODE_EXISTS(DT_NODELABEL(anim_pick))

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zephyr/logging/log.h>

#include "claude_view_display.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/* Strong definition in claude_view/behaviors/anim_select.c (right half only). */
__weak void zmk_claude_view_set_animation(uint8_t idx) {
    ARG_UNUSED(idx);
}

static int anim_pick_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    ARG_UNUSED(event);

    zmk_claude_view_set_animation((uint8_t)binding->param1);
    return ZMK_BEHAVIOR_OPAQUE;
}

static int anim_pick_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);
    ARG_UNUSED(event);
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_anim_pick_driver_api = {
    .binding_pressed = anim_pick_binding_pressed,
    .binding_released = anim_pick_binding_released,
    .locality = BEHAVIOR_LOCALITY_GLOBAL,
};

BEHAVIOR_DT_DEFINE(DT_NODELABEL(anim_pick), NULL, NULL, NULL, NULL, POST_KERNEL,
                    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_anim_pick_driver_api);

#endif /* DT_NODE_EXISTS(DT_NODELABEL(anim_pick)) */
