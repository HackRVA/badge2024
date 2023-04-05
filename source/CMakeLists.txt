
add_executable(${PRODUCT} main.c)

add_subdirectory(apps)
add_subdirectory(cli)
add_subdirectory(core)
add_subdirectory(display)
add_subdirectory(hal)

if (${TARGET} STREQUAL "PICO")
    # pull in Pico subcomponents
    target_link_libraries(${PRODUCT}
            pico_stdlib
            pico_multicore
            hardware_adc
            hardware_dma
            hardware_exception
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
    pico_enable_stdio_usb(${PRODUCT} 1)
    pico_enable_stdio_uart(${PRODUCT} 0)

    # create map/bin/hex file etc.
    pico_add_extra_outputs(${PRODUCT})
endif()

