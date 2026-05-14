if(CONFIG_ZMK_DISPLAY)
    zephyr_library_sources(custom_status_screen.c)
    zephyr_library_sources(assets/claude_art.c)
    # Expose ZMK app headers (zmk/event_manager.h, zmk/events/*.h, etc.)
    # to shield source files; the variable is set by Zephyr's module system.
    zephyr_library_include_directories(${ZEPHYR_ZMK_MODULE_DIR}/app/include)
endif()
