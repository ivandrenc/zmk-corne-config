/*
 * Copyright (c) 2024 ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * Select Claude View animation by index on the BLE peripheral (EVENT_SOURCE locality).
 */

#include <zephyr/devicetree.h>

#if DT_NODE_EXISTS(DT_NODELABEL(anim_pick))

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if defined(CONFIG_BOARD_EYELASH_CORNE_RIGHT)
#include "claude_view_display.h"
#endif

static int anim_pick_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    ARG_UNUSED(event);

#if defined(CONFIG_BOARD_EYELASH_CORNE_RIGHT)
    zmk_claude_view_set_animation((uint8_t)binding->param1);
#endif
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
    .locality = BEHAVIOR_LOCALITY_EVENT_SOURCE,
};

BEHAVIOR_DT_DEFINE(DT_NODELABEL(anim_pick), NULL, NULL, NULL, NULL, POST_KERNEL,
                    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_anim_pick_driver_api);

#endif /* DT_NODE_EXISTS(DT_NODELABEL(anim_pick)) */
