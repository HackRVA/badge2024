#!/bin/sh

cmake -S . -B build_sdl_sim/ -DTARGET=SDL_SIMULATOR -DCMAKE_BUILD_TYPE=DEBUG -G "Unix Makefiles"

