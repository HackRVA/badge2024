cmake_minimum_required(VERSION 3.12)

# For simplicity of setup, just set defaults here. Can be overridden in IDE project settings or CLI
set(PICO_SDK_FETCH_FROM_GIT ON CACHE BOOL "Download Pico SDK from Git. Default on.")
set(PICO_EXTRAS_FETCH_FROM_GIT ON CACHE BOOL "Download Pico SDK Extras from Git. Default on.")
set(TARGET "PICO" CACHE STRING "Target hardware. For now, only Pico, in the future, badge/simulator")
set(PRODUCT "badge2024_c")
set(SIMULATOR_AUDIO "no") # change to "no" to avoid compiling audio code in simulator
set(BADGE_HAS_ROTARY_SWITCHES "no")

if (${TARGET} STREQUAL "PICO")
    # Pull in SDK (must be before project).
    # Also does toolchain setup - configures us to use arm-none-eabi-gcc
    include(pico_sdk_import.cmake)
    include(pico_extras_import.cmake)
endif()

project(${PRODUCT} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
enable_testing()

if (${TARGET} STREQUAL "WASM")
	set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif()

# Initialize the SDK
if (${TARGET} STREQUAL "PICO")
    add_compile_definitions(TARGET_PICO)
    pico_sdk_init()
elseif(${TARGET} STREQUAL "SIMULATOR" OR ${TARGET} STREQUAL "SDL_SIMULATOR" OR ${TARGET} STREQUAL "WASM")
    add_compile_definitions(TARGET_SIMULATOR)
    if (${SIMULATOR_AUDIO} STREQUAL "yes")
        add_compile_definitions(SIMULATOR_AUDIO)
    endif()
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
    # Note: cmake doesn't know about portaudio, so we cram -lportaudio -lasound -lm -lpthread in here
    add_compile_options(-fsanitize=address -fsanitize=undefined -Wstrict-prototypes)
    if (${SIMULATOR_AUDIO} STREQUAL "yes")
	    add_link_options(-fsanitize=address -fsanitize=undefined -lportaudio -lasound -lm -lpthread -rdynamic)
    else()
	    add_link_options(-fsanitize=address -fsanitize=undefined)
    endif()
endif()

if (${TARGET} STREQUAL "WASM")
	add_compile_options(-sUSE_SDL=2 -sUSE_LIBPNG)
endif()

if (${BADGE_HAS_ROTARY_SWITCHES} STREQUAL "yes")
	add_compile_options(-DBADGE_HAS_ROTARY_SWITCHES=1)
endif()

add_subdirectory(source)
