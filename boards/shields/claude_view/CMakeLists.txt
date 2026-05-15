if(CONFIG_ZMK_DISPLAY)
    zephyr_library_sources(custom_status_screen.c)
    zephyr_library_sources(assets/claude_art.c)
    # behaviors/anim_select.c compiles only on the right (peripheral) half.
    # The behavior DTS nodes exist in claude_view.overlay and the YAML bindings
    # are auto-discovered from the shield's dts/bindings/ by ZMK's build system.
    zephyr_library_sources(behaviors/anim_select.c)
endif()
