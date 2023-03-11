#!/bin/sh

echo
echo "The GTK based simulator is deprecated.  You probably want to run run_cmake_sdl_sim.sh instead"
echo

cmake -S . -B build_sim/ -DTARGET=SIMULATOR -DCMAKE_BUILD_TYPE=DEBUG -G "Unix Makefiles"

echo
echo "The GTK based simulator is deprecated.  You probably want to run run_cmake_sdl_sim.sh instead"
echo



