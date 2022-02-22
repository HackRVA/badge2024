
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
            ${CMAKE_CURRENT_LIST_DIR}/audio_output_rp2040.c
            ${CMAKE_CURRENT_LIST_DIR}/rtc_rp2040.c
            ${CMAKE_CURRENT_LIST_DIR}/random_rp2040.c
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
            ${CMAKE_CURRENT_LIST_DIR}/audio_output_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/rtc_sim.c
            ${CMAKE_CURRENT_LIST_DIR}/random_sim.c
            )
endif()

target_include_directories(badge2022_c PUBLIC .)