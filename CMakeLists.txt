# ZMK user module — linked into every firmware image built from this repo.

zephyr_library_named(zmk_corne_anim_split)

zephyr_include_directories(${APPLICATION_SOURCE_DIR}/include)

zephyr_library_sources(behavior_anim_set.c behavior_anim_cycle.c)

# claude_view_display.h (peripheral animation API)
zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/boards/shields/claude_view/assets)
