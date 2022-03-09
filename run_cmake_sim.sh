#!/bin/sh

cmake -S . -B build_sim/ -DTARGET=SIMULATOR -G "Unix Makefiles"

