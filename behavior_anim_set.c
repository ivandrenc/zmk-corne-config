/*
 * Copyright (c) 2024 ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * Fixed-index animation selectors (wave, gym, …) for split keyboards.
 */

#define DT_DRV_COMPAT zmk_behavior_anim_set

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>

#include "claude_view_display.h"

__weak void zmk_claude_view_set_animation(uint8_t idx) {
    ARG_UNUSED(idx);
}

struct behavior_anim_set_config {
    uint8_t animation_index;
};

static int anim_set_binding_released(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);
    ARG_UNUSED(event);
    return ZMK_BEHAVIOR_OPAQUE;
}

#define ANIM_SET_INST(n)                                                                           \
    static int anim_set_binding_pressed_##n(struct zmk_behavior_binding *binding,                \
                                            struct zmk_behavior_binding_event event) {           \
        ARG_UNUSED(binding);                                                                     \
        ARG_UNUSED(event);                                                                       \
        zmk_claude_view_set_animation(config_##n.animation_index);                                 \
        return ZMK_BEHAVIOR_OPAQUE;                                                              \
    }                                                                                            \
                                                                                                 \
    static const struct behavior_anim_set_config config_##n = {                                  \
        .animation_index = DT_INST_PROP(n, animation_index),                                     \
    };                                                                                           \
                                                                                                 \
    static const struct behavior_driver_api api_##n = {                                          \
        .binding_pressed = anim_set_binding_pressed_##n,                                         \
        .binding_released = anim_set_binding_released,                                           \
        .locality = BEHAVIOR_LOCALITY_GLOBAL,                                                    \
    };                                                                                           \
                                                                                                 \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, &config_##n, NULL, POST_KERNEL,                       \
                           CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &api_##n);

DT_INST_FOREACH_STATUS_OKAY(ANIM_SET_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
