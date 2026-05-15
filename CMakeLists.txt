# ZMK user module — linked into every firmware image built from this repo.
# See behavior_anim_pick.c for how companion-display animation switching uses the
# stock split BLE "run behavior" characteristic (no fork of ZMK required).

zephyr_library_named(zmk_corne_anim_split)

# ZMK behavior headers (drivers/behavior.h, zmk/behavior.h) live in zmk/app/include
zephyr_include_directories(${APPLICATION_SOURCE_DIR}/include)

zephyr_library_sources(behavior_anim_pick.c behavior_anim_cycle.c)
