#!/bin/sh

mkdir build-wasm
emcc -o build-wasm/index.html \
	src/main.c src/modules/* \
	-Os -Wall raylib/src/libraylib.a \
	-I. -Iraylib/src/ -L. -Lraylib/src/ -s USE_GLFW=3 \
	-DPLATFORM_WEB \
	-sEXPORTED_RUNTIME_METHODS=ccall \
	-sEXPORTED_FUNCTIONS=_load_settings,_pause,_main \
	--shell-file=wasm-frontend/shell.html \
	-lidbfs.js \
	--preload-file ./res
