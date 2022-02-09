
target_sources(badge2022_c PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/delay_rp2040.c
        ${CMAKE_CURRENT_LIST_DIR}/flash_storage_rp2040.c
        ${CMAKE_CURRENT_LIST_DIR}/init_rp2040.c
        ${CMAKE_CURRENT_LIST_DIR}/usb_rp2040.c
        ${CMAKE_CURRENT_LIST_DIR}/display_s6b33_rp2040.c
        ${CMAKE_CURRENT_LIST_DIR}/led_pwm_rp2040.c
        ${CMAKE_CURRENT_LIST_DIR}/button_rp2040.c)

target_include_directories(badge2022_c PUBLIC .)