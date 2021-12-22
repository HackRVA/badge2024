
add_executable(badge2022_c main.c)

add_subdirectory(cli)
add_subdirectory(hal)

# pull in Pico subcomponents
target_link_libraries(badge2022_c
        pico_stdlib
        pico_multicore
        hardware_flash
        hardware_spi)

# enable usb output, disable uart output
pico_enable_stdio_usb(badge2022_c 1)
pico_enable_stdio_uart(badge2022_c 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(badge2022_c)
