#!/bin/sh

cmake -S . -B build_sim/ -DTARGET=SIMULATOR -DCMAKE_BUILD_TYPE=DEBUG -G "Unix Makefiles"

