cmake_minimum_required(VERSION 3.22)

# ─── macOS deployment / SDK ──────────────────────────────────────────────────
# JUCE 8.0.x uses CGWindowListCreateImage which was removed in the macOS 15 SDK
# (Xcode 16).  Targeting macOS 14 keeps the symbol available.
# Override at configure time: cmake … -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0
if(APPLE AND NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0" CACHE STRING
        "Minimum macOS version – 14.0 avoids JUCE 8 / Xcode 16 SDK conflicts" FORCE)
endif()

# The Xcode 16 SDK still ships the old headers but marks them unavailable.
# Tell the compiler to treat them as merely deprecated (warnings, not errors)
# so juceaide can finish building.  This flag is harmless on older SDKs.
if(APPLE)
    add_compile_options(-Wno-error=unavailable)
    # Also silence the flood of -Wdeprecated-declarations from JUCE internals;
    # these are JUCE's own macOS 15 compat issues, not our code.
    add_compile_options(-Wno-deprecated-declarations)
endif()

project(SquareWaveSynth VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ─── JUCE ────────────────────────────────────────────────────────────────────
# Option A: JUCE as a subdirectory (git submodule) – preferred for CI/CD
# Option B: Fetched automatically via FetchContent (requires network)
# Toggle with -DJUCE_SOURCE_DIR=/path/to/JUCE or leave blank for FetchContent.

if(DEFINED JUCE_SOURCE_DIR AND EXISTS "${JUCE_SOURCE_DIR}")
    message(STATUS "Using JUCE from: ${JUCE_SOURCE_DIR}")
    add_subdirectory(${JUCE_SOURCE_DIR} ${CMAKE_BINARY_DIR}/JUCE)
else()
    message(STATUS "JUCE_SOURCE_DIR not set – fetching via FetchContent")
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.12   # JUCE 8 – latest stable
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

# ─── Plugin formats ──────────────────────────────────────────────────────────
# VST3 is cross-platform; AU is macOS-only (JUCE handles the guard internally).
set(PLUGIN_FORMATS VST3)
if(APPLE)
    list(APPEND PLUGIN_FORMATS AU)
endif()

# ─── Plugin target ───────────────────────────────────────────────────────────
juce_add_plugin(SquareWaveSynth
    COMPANY_NAME            "YourCompany"
    BUNDLE_ID               "com.yourcompany.squarewavesynth"
    PLUGIN_MANUFACTURER_CODE "YrCo"
    PLUGIN_CODE              "SqWv"

    FORMATS                 ${PLUGIN_FORMATS}

    IS_SYNTH                TRUE
    NEEDS_MIDI_INPUT        TRUE
    NEEDS_MIDI_OUTPUT       FALSE
    IS_MIDI_EFFECT          FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE

    PRODUCT_NAME            "SquareWaveSynth"
    VERSION                 "1.0.0"

    # VST3 category
    VST3_CATEGORIES         "Instrument" "Synths"
    # AU type: 'aumu' = Music Device (instrument)
    AU_MAIN_TYPE            "kAudioUnitType_MusicDevice"
)

# ─── Sources ─────────────────────────────────────────────────────────────────
target_sources(SquareWaveSynth
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/Ym2612Voice.h
        Source/SynthSound.h
        Source/MidiKeyboardComponent.h
        # MAME YM2612 emulator core (C, GPL v2+) – compiled as C not C++
        Source/mame/mame_ym2612fm.c
        Source/mame/mame_ym2612fm.h
        Source/mame/mamedef.h
)

# The MAME YM2612 source is plain C99; tell CMake to compile it as C
set_source_files_properties(Source/mame/mame_ym2612fm.c PROPERTIES LANGUAGE C)

# ─── JUCE modules ────────────────────────────────────────────────────────────
target_compile_definitions(SquareWaveSynth
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(SquareWaveSynth
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils          # MidiKeyboardComponent lives here
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        # juce_recommended_warning_flags promotes -Wdeprecated to errors,
        # which breaks JUCE 8 on the macOS 15 SDK (Xcode 16).
        # Re-enable once JUCE fixes CGWindowListCreateImage / CVDisplayLink.
        # juce::juce_recommended_warning_flags
        # juce::juce_recommended_lto_flags  # optional, slows clean builds
)

# ─── IDE source grouping ─────────────────────────────────────────────────────
source_group("Source" FILES
    Source/PluginProcessor.cpp  Source/PluginProcessor.h
    Source/PluginEditor.cpp     Source/PluginEditor.h
    Source/Ym2612Voice.h
    Source/SynthSound.h
)
source_group("Source/mame" FILES
    Source/mame/mame_ym2612fm.c
    Source/mame/mame_ym2612fm.h
    Source/mame/mamedef.h
)

# ─── Install / copy to plugin folders (optional) ─────────────────────────────
# Uncomment to auto-copy after build on macOS:
# add_custom_command(TARGET SquareWaveSynth POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy_directory
#         "$<TARGET_FILE_DIR:SquareWaveSynth_VST3>"
#         "$ENV{HOME}/Library/Audio/Plug-Ins/VST3"
# )
