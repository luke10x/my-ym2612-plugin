#!/bin/bash 
g++ \
    -I../bowling/build/macos/sdl2/include/ \
    -I../bowling/3rdparty/SDL/include \
    -I./build/_deps/ymfm-src/src/ \
    ../bowling/build/macos/usr/lib/libSDL2.a \
    build/_deps/ymfm-src/src/ymfm_misc.cpp \
    build/_deps/ymfm-src/src/ymfm_adpcm.cpp \
    build/_deps/ymfm-src/src/ymfm_ssg.cpp \
    build/_deps/ymfm-src/src/ymfm_opn.cpp \
    patchtest.cpp \
    -std=c++17 \
    -framework Cocoa -framework IOKit -framework CoreVideo -framework CoreAudio -framework AudioToolbox \
    -framework ForceFeedback -framework Carbon -framework Metal -framework GameController -framework CoreHaptics \
    -lobjc -o patchtest && ./patchtest 