
if (${TARGET} STREQUAL "PICO")
    target_sources(badge2023_c PUBLIC
            ${CMAKE_CURRENT_LIST_DIR}/delay_rp2040.c
            ${CMAKE_CURRENT_LIST_DIR}/flash_storage_rp2040.c
            ${CMAKE_CURRENT_LIST_DIR}/init_rp2040.c
            ${CMAKE_CURRENT_LIST_DIR}/usb_rp2040.c
            ${CMAKE_CURRENT_LIST_DIR}/led_pwm_rp2040.c
            ${CMAKE_CURRENT_LIST_DIR}/button_rp2040.c
            ${CMAKE_CURRENT_LIST_DIR}/ir_rp2040.c
            ${CMAKE_CURRENT_LIST_DIR}/audio_rp2040.c
            ${CMAKE_CURRENT_LIST_DIR}/rtc_rp2040.c
            ${CMAKE_CURRENT_LIST_DIR}/random_rp2040.c
            ${CMAKE_CURRENT_LIST_DIR}/uid_rp2040.c
            )

    # The 2023 badge uses a new display!
    if (DEFINED COMPAT_BADGE_2022)
        target_sources(badge2022_c PUBLIC
                ${CMAKE_CURRENT_LIST_DIR}/display_s6b33_rp2040.c
                )
    else()
        target_sources(badge2022_c PUBLIC
                ${CMAKE_CURRENT_LIST_DIR}/display_st7735s_rp2040.c
                )
    endif ()

    add_subdirectory(ir_library)
endif()

if (${TARGET} STREQUAL "SIMULATOR")
    target_sources(badge2023_c PUBLIC
            ${CMAKE_CURRENT_LIST_DIR}/delay_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/flash_storage_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/init_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/usb_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/display_s6b33_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/led_pwm_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/button_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/ir_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/audio_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/rtc_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/random_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/uid_sim.c
            )

    cmake_policy(SET CMP0079 NEW)
    find_package(GTK2 2.6 REQUIRED gtk)
    target_include_directories(badge2023_c PRIVATE ${GTK2_INCLUDE_DIRS})
    target_link_directories(badge2023_c PRIVATE ${GTK2_LIBRARY_DIRS})
    target_link_libraries(badge2023_c ${GTK2_LIBRARIES})
    # Add other flags to the compiler
    target_compile_definitions(badge2023_c PRIVATE ${GTK2_CFLAGS_OTHER})

    find_package(Threads REQUIRED)
    target_link_libraries(badge2023_c Threads::Threads)
endif()

if (${TARGET} STREQUAL "SDL_SIMULATOR")
    target_sources(badge2023_c PUBLIC
            ${CMAKE_CURRENT_LIST_DIR}/button_coords.c
            ${CMAKE_CURRENT_LIST_DIR}/delay_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/flash_storage_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/init_sdl_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/usb_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/display_s6b33_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/led_pwm_sdl_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/button_sdl_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/ir_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/audio_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/rtc_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/random_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/sim_lcd_params.c
            ${CMAKE_CURRENT_LIST_DIR}/uid_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/png_utils.c
            )

    cmake_policy(SET CMP0079 NEW)
    find_package(SDL2 REQUIRED)
    target_include_directories(badge2023_c PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_directories(badge2023_c PRIVATE ${SDL2_LIBRARY_DIRS})
    target_link_libraries(badge2023_c ${SDL2_LIBRARIES})
    # Add other flags to the compiler
    target_compile_definitions(badge2023_c PRIVATE ${SDL2_CFLAGS_OTHER})
    
    find_package(PNG REQUIRED)
    target_include_directories(badge2023_c PRIVATE ${PNG_INCLUDE_DIRS})
    target_link_libraries(badge2023_c ${PNG_LIBRARIES})
    target_compile_definitions(badge2023_c PRIVATE ${PNG_CFLAGS_OTHER})

    find_package(Threads REQUIRED)
    target_link_libraries(badge2023_c Threads::Threads)
endif()

target_include_directories(badge2023_c PUBLIC .)
