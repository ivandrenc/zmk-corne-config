if(CONFIG_ZMK_DISPLAY)
    # Expose zmk/app/include so shield files can use <zmk/...> and <drivers/...> headers.
    # APPLICATION_SOURCE_DIR is zmk/app in a ZMK build.
    zephyr_include_directories(${APPLICATION_SOURCE_DIR}/include)

    zephyr_library_sources(custom_status_screen.c)
    zephyr_library_sources(assets/claude_art.c)
    zephyr_library_sources(behaviors/anim_select.c)
endif()
