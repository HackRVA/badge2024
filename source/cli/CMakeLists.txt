
target_sources(${PRODUCT} PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/cli.c
        ${CMAKE_CURRENT_LIST_DIR}/cli_button.c
        ${CMAKE_CURRENT_LIST_DIR}/cli_flash.c
        ${CMAKE_CURRENT_LIST_DIR}/cli_led.c
        ${CMAKE_CURRENT_LIST_DIR}/cli_ir.c)

target_include_directories(${PRODUCT} PUBLIC .)
