
target_sources(badge2022_c PUBLIC
        delay_rp2040.c flash_storage_rp2040.c init_rp2040.c usb_rp2040.c)

target_include_directories(badge2022_c PUBLIC .)