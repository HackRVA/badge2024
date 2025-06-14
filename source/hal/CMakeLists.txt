
if (${TARGET} STREQUAL "PICO")
    target_sources(${PRODUCT} PUBLIC
            ${CMAKE_CURRENT_LIST_DIR}/delay_rp2040.c
            ${CMAKE_CURRENT_LIST_DIR}/flash_storage_rp2040.c
            ${CMAKE_CURRENT_LIST_DIR}/init_rp2040.c
            ${CMAKE_CURRENT_LIST_DIR}/usb_rp2040.c
            ${CMAKE_CURRENT_LIST_DIR}/led_pwm_rp2040.c
            ${CMAKE_CURRENT_LIST_DIR}/button_rp2040.c
            ${CMAKE_CURRENT_LIST_DIR}/ir_rp2040.c
            ${CMAKE_CURRENT_LIST_DIR}/audio_common.c
            ${CMAKE_CURRENT_LIST_DIR}/audio_rp2040.c
            ${CMAKE_CURRENT_LIST_DIR}/rtc_rp2040.c
            ${CMAKE_CURRENT_LIST_DIR}/random_rp2040.c
            ${CMAKE_CURRENT_LIST_DIR}/uid_rp2040.c
            ${CMAKE_CURRENT_LIST_DIR}/display_st7735s_rp2040.c
            ${CMAKE_CURRENT_LIST_DIR}/driver-ST7735S/source/st7735s.c
            ${CMAKE_CURRENT_LIST_DIR}/analog_rp2040.c
            ${CMAKE_CURRENT_LIST_DIR}/color_sensor_cls16d2444_rp2040.c
            ${CMAKE_CURRENT_LIST_DIR}/mic_pdm_rp2040.c
            )

    target_include_directories(${PRODUCT} PUBLIC ./driver-ST7735S/source)

    add_subdirectory(ir_library)
    add_subdirectory(mic_library)
endif()

if (${TARGET} STREQUAL "SIMULATOR")
    target_sources(${PRODUCT} PUBLIC
            ${CMAKE_CURRENT_LIST_DIR}/delay_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/flash_storage_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/init_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/usb_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/display_s6b33_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/led_pwm_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/button_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/ir_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/audio_common.c
            ${CMAKE_CURRENT_LIST_DIR}/audio_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/rtc_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/random_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/uid_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/analog_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/color_sensor_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/mic_pdm_sim.c
            )

    cmake_policy(SET CMP0079 NEW)
    find_package(GTK2 2.6 REQUIRED gtk)
    target_include_directories(${PRODUCT} PRIVATE ${GTK2_INCLUDE_DIRS})
    target_link_directories(${PRODUCT} PRIVATE ${GTK2_LIBRARY_DIRS})
    target_link_libraries(${PRODUCT} ${GTK2_LIBRARIES})
    # Add other flags to the compiler
    target_compile_definitions(${PRODUCT} PRIVATE ${GTK2_CFLAGS_OTHER})

    find_package(Threads REQUIRED)
    target_link_libraries(${PRODUCT} Threads::Threads)
endif()

if (${TARGET} STREQUAL "SDL_SIMULATOR" OR ${TARGET} STREQUAL "WASM")
    target_sources(${PRODUCT} PUBLIC
            ${CMAKE_CURRENT_LIST_DIR}/button_coords.c
            ${CMAKE_CURRENT_LIST_DIR}/delay_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/flash_storage_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/init_sdl_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/usb_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/display_s6b33_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/led_pwm_sdl_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/button_sdl_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/ir_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/audio_common.c
            ${CMAKE_CURRENT_LIST_DIR}/audio_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/rtc_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/random_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/sim_lcd_params.c
            ${CMAKE_CURRENT_LIST_DIR}/uid_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/png_utils.c
            ${CMAKE_CURRENT_LIST_DIR}/vec3.c
            ${CMAKE_CURRENT_LIST_DIR}/quat.c
            ${CMAKE_CURRENT_LIST_DIR}/analog_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/color_sensor_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/mic_pdm_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/sim_slider_input.c
            )

    cmake_policy(SET CMP0079 NEW)
    if (${TARGET} STREQUAL "SDL_SIMULATOR")
      find_package(SDL2 REQUIRED)
    endif()
    target_include_directories(${PRODUCT} PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_directories(${PRODUCT} PRIVATE ${SDL2_LIBRARY_DIRS})
    target_link_libraries(${PRODUCT} ${SDL2_LIBRARIES})
    # Add other flags to the compiler
    target_compile_definitions(${PRODUCT} PRIVATE ${SDL2_CFLAGS_OTHER})

    find_package(PNG REQUIRED)
    target_include_directories(${PRODUCT} PRIVATE ${PNG_INCLUDE_DIRS})
    target_link_libraries(${PRODUCT} ${PNG_LIBRARIES})
    target_compile_definitions(${PRODUCT} PRIVATE ${PNG_CFLAGS_OTHER})

    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    target_link_libraries(${PRODUCT} Threads::Threads)
    target_link_libraries(${PRODUCT} m)

endif()

target_include_directories(${PRODUCT} PUBLIC .)
