if(CONFIG_ZMK_DISPLAY)
    zephyr_library_sources(custom_status_screen.c)
    zephyr_library_sources(assets/claude_art.c)
    zephyr_library_sources(behaviors/anim_select.c)

    # Expose the shield's dts/bindings/ directory to the Zephyr DTS toolchain
    list(APPEND DTS_ROOT ${CMAKE_CURRENT_LIST_DIR})
    set(DTS_ROOT ${DTS_ROOT} CACHE INTERNAL "")
endif()
