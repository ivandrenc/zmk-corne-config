/*
 * Copyright (c) 2024 ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * Cycles Claude View animations on the BLE peripheral (GLOBAL locality).
 * See behavior_anim_pick.c for transport details.
 */

#define DT_DRV_COMPAT zmk_behavior_anim_cycle

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#if defined(CONFIG_BOARD_EYELASH_CORNE_RIGHT)
extern uint8_t zmk_claude_view_get_animation(void);
extern void zmk_claude_view_set_animation(uint8_t idx);
#endif

static int anim_cycle_binding_pressed(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);
    ARG_UNUSED(event);

#if defined(CONFIG_BOARD_EYELASH_CORNE_LEFT)
    return ZMK_BEHAVIOR_OPAQUE;
#elif defined(CONFIG_BOARD_EYELASH_CORNE_RIGHT)
    zmk_claude_view_set_animation(zmk_claude_view_get_animation() + 1);
    return ZMK_BEHAVIOR_OPAQUE;
#else
    return ZMK_BEHAVIOR_OPAQUE;
#endif
}

static int anim_cycle_binding_released(struct zmk_behavior_binding *binding,
                                       struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);
    ARG_UNUSED(event);
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_anim_cycle_driver_api = {
    .binding_pressed = anim_cycle_binding_pressed,
    .binding_released = anim_cycle_binding_released,
    .locality = BEHAVIOR_LOCALITY_GLOBAL,
};

BEHAVIOR_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL,
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_anim_cycle_driver_api);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
