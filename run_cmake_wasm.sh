#!/bin/sh

echo "$PATH" | grep emscripten
if [ "$?" = "1" ]
then
	echo "You need to source emsdk_env.sh first." 1>&2
	exit 1
fi

emcmake cmake -S . -B build_wasm_sim/ -DTARGET=WASM -DCMAKE_BUILD_TYPE=DEBUG -G "Unix Makefiles"

echo
echo
echo "Now cd build_wasm_sim and type 'emmake make'"
echo "then cd to source, and run 'python -m SimpleHTTPServer'"
echo "then point your browser at 'localhost:8000'"
echo
echo "Note: this doesn't actually seem to work yet."
echo
echo

