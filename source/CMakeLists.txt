
add_executable(badge2022_c main.c)

add_subdirectory(apps)
add_subdirectory(cli)
add_subdirectory(core)
add_subdirectory(display)
add_subdirectory(hal)

if (${TARGET} STREQUAL "PICO")
    # pull in Pico subcomponents
    target_link_libraries(badge2022_c
            pico_stdlib
            pico_multicore
            hardware_adc
            hardware_dma
            hardware_flash
            hardware_pio
            hardware_pwm
            hardware_timer
            hardware_resets
            hardware_rtc
            hardware_irq
            hardware_sleep
            hardware_spi
            nec_transmit_library
            nec_receive_library)

    # enable usb output, disable uart output
    pico_enable_stdio_usb(badge2022_c 1)
    pico_enable_stdio_uart(badge2022_c 0)

    # create map/bin/hex file etc.
    pico_add_extra_outputs(badge2022_c)
endif()

