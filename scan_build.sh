#!/bin/sh

# This script runs clang scan-build, the clang static analyzer
# to help find bugs in the code.
#
# This only works with the simulator because I don't know how to
# get clang to work with the raspberry pi pico sdk.
#
# See https://clang-analyzer.llvm.org/scan-build.html for more info
# about clang scan-build.

scan-build cmake -S . -B zscan_build/ -DTARGET=SDL_SIMULATOR -DCMAKE_BUILD_TYPE=DEBUG -G "Unix Makefiles" || exit 1
cd zscan_build || exit 1
make clean || exit 1;
scan-build make || exit 1
exit 0


