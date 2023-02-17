cmake_minimum_required(VERSION 3.12)

# For simplicity of setup, just set defaults here. Can be overridden in IDE project settings or CLI
set(PICO_SDK_FETCH_FROM_GIT ON CACHE BOOL "Download Pico SDK from Git. Default on.")
set(PICO_EXTRAS_FETCH_FROM_GIT ON CACHE BOOL "Download Pico SDK Extras from Git. Default on.")
set(TARGET "PICO" CACHE STRING "Target hardware. For now, only Pico, in the future, badge/simulator")

if (${TARGET} STREQUAL "PICO")
    # Pull in SDK (must be before project).
    # Also does toolchain setup - configures us to use arm-none-eabi-gcc
    include(pico_sdk_import.cmake)
    include(pico_extras_import.cmake)
endif()

project(badge2022_c C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
enable_testing()

# Initialize the SDK
if (${TARGET} STREQUAL "PICO")
    add_compile_definitions(TARGET_PICO)
    pico_sdk_init()
elseif(${TARGET} STREQUAL "SIMULATOR" OR ${TARGET} STREQUAL "SDL_SIMULATOR")
    add_compile_definitions(TARGET_SIMULATOR)
endif()

add_compile_options(-Wall -Wextra)

if (${TARGET} STREQUAL "PICO")
    add_compile_options(
        -Wno-format          # pico-examples: int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # pico-examples: we have some for the docs that aren't called
        -Wno-maybe-uninitialized # sam: see if this causes extra warnings outside of pico-examples
    )
endif()

if (${TARGET} STREQUAL "SIMULATOR" OR ${TARGET} STREQUAL "SDL_SIMULATOR")
    # Suppress GTK deprecated declarations
    add_compile_options(-Wno-deprecated-declarations -fsanitize=address)
    add_link_options(-fsanitize=address)
endif()

add_subdirectory(source)
