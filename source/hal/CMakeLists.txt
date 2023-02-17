
if (${TARGET} STREQUAL "PICO")
    target_sources(badge2022_c PUBLIC
            ${CMAKE_CURRENT_LIST_DIR}/delay_rp2040.c
            ${CMAKE_CURRENT_LIST_DIR}/flash_storage_rp2040.c
            ${CMAKE_CURRENT_LIST_DIR}/init_rp2040.c
            ${CMAKE_CURRENT_LIST_DIR}/usb_rp2040.c
            ${CMAKE_CURRENT_LIST_DIR}/display_s6b33_rp2040.c
            ${CMAKE_CURRENT_LIST_DIR}/led_pwm_rp2040.c
            ${CMAKE_CURRENT_LIST_DIR}/button_rp2040.c
            ${CMAKE_CURRENT_LIST_DIR}/ir_rp2040.c
            ${CMAKE_CURRENT_LIST_DIR}/audio_rp2040.c
            ${CMAKE_CURRENT_LIST_DIR}/rtc_rp2040.c
            ${CMAKE_CURRENT_LIST_DIR}/random_rp2040.c
            ${CMAKE_CURRENT_LIST_DIR}/uid_rp2040.c
            )

    add_subdirectory(ir_library)
endif()

if (${TARGET} STREQUAL "SIMULATOR")
    target_sources(badge2022_c PUBLIC
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
    target_include_directories(badge2022_c PRIVATE ${GTK2_INCLUDE_DIRS})
    target_link_directories(badge2022_c PRIVATE ${GTK2_LIBRARY_DIRS})
    target_link_libraries(badge2022_c ${GTK2_LIBRARIES})
    # Add other flags to the compiler
    target_compile_definitions(badge2022_c PRIVATE ${GTK2_CFLAGS_OTHER})

    find_package(Threads REQUIRED)
    target_link_libraries(badge2022_c Threads::Threads)
endif()

if (${TARGET} STREQUAL "SDL_SIMULATOR")
    target_sources(badge2022_c PUBLIC
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
            ${CMAKE_CURRENT_LIST_DIR}/uid_sim.c
            )

    cmake_policy(SET CMP0079 NEW)
    find_package(SDL2 REQUIRED)
    target_include_directories(badge2022_c PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_directories(badge2022_c PRIVATE ${SDL2_LIBRARY_DIRS})
    target_link_libraries(badge2022_c ${SDL2_LIBRARIES})
    # Add other flags to the compiler
    target_compile_definitions(badge2022_c PRIVATE ${SDL2_CFLAGS_OTHER})
    
    find_package(Threads REQUIRED)
    target_link_libraries(badge2022_c Threads::Threads)
endif()

target_include_directories(badge2022_c PUBLIC .)
