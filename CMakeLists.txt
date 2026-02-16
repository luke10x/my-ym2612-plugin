cmake_minimum_required(VERSION 3.22)

# ─── macOS deployment / SDK ──────────────────────────────────────────────────
if(APPLE AND NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0" CACHE STRING
        "Minimum macOS version – 14.0 avoids JUCE 8 / Xcode 16 SDK conflicts" FORCE)
endif()
if(APPLE)
    add_compile_options(-Wno-error=unavailable -Wno-deprecated-declarations)
endif()

project(SquareWaveSynth VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# ─── JUCE ────────────────────────────────────────────────────────────────────
if(DEFINED JUCE_SOURCE_DIR AND EXISTS "${JUCE_SOURCE_DIR}")
    message(STATUS "Using JUCE from: ${JUCE_SOURCE_DIR}")
    add_subdirectory(${JUCE_SOURCE_DIR} ${CMAKE_BINARY_DIR}/JUCE)
else()
    message(STATUS "Fetching JUCE via FetchContent")
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.12
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

# ─── ymfm ────────────────────────────────────────────────────────────────────
# ymfm has no CMakeLists, so we fetch the source and build only what we need.
FetchContent_Declare(
    ymfm
    GIT_REPOSITORY https://github.com/aaronsgiles/ymfm.git
    GIT_TAG        main
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(ymfm)

# Build the ymfm sources we need as a static library.
# YM2612 = OPN2 family, lives in ymfm_opn.cpp.
# ymfm_opn.cpp depends on ymfm_adpcm and ymfm_ssg internally.
add_library(ymfm_lib STATIC
    ${ymfm_SOURCE_DIR}/src/ymfm_misc.cpp
    ${ymfm_SOURCE_DIR}/src/ymfm_ssg.cpp
    ${ymfm_SOURCE_DIR}/src/ymfm_adpcm.cpp
    ${ymfm_SOURCE_DIR}/src/ymfm_opn.cpp
)
target_include_directories(ymfm_lib PUBLIC ${ymfm_SOURCE_DIR}/src)
# ymfm is C++14-compatible; use 17 to match our plugin
set_target_properties(ymfm_lib PROPERTIES CXX_STANDARD 17)
# Suppress warnings from ymfm internals – not our code
if(APPLE OR UNIX)
    target_compile_options(ymfm_lib PRIVATE -Wno-all -Wno-extra)
elseif(MSVC)
    target_compile_options(ymfm_lib PRIVATE /W0)
endif()

# ─── Plugin target ───────────────────────────────────────────────────────────
set(PLUGIN_FORMATS VST3)
if(APPLE)
    list(APPEND PLUGIN_FORMATS AU)
endif()

juce_add_plugin(SquareWaveSynth
    COMPANY_NAME            "YourCompany"
    BUNDLE_ID               "com.yourcompany.squarewavesynth"
    PLUGIN_MANUFACTURER_CODE "YrCo"
    PLUGIN_CODE              "Ym26"

    FORMATS                 ${PLUGIN_FORMATS}

    IS_SYNTH                TRUE
    NEEDS_MIDI_INPUT        TRUE
    NEEDS_MIDI_OUTPUT       FALSE
    IS_MIDI_EFFECT          FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE

    PRODUCT_NAME            "YM2612 Synth"
    VERSION                 "1.0.0"

    VST3_CATEGORIES         "Instrument" "Synths"
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
)

# ─── Compile definitions ──────────────────────────────────────────────────────
target_compile_definitions(SquareWaveSynth
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

# ─── Link libraries ───────────────────────────────────────────────────────────
target_link_libraries(SquareWaveSynth
    PRIVATE
        ymfm_lib                        # <── ymfm FM emulation
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
)

# ─── IDE source grouping ─────────────────────────────────────────────────────
source_group("Source" FILES
    Source/PluginProcessor.cpp  Source/PluginProcessor.h
    Source/PluginEditor.cpp     Source/PluginEditor.h
    Source/Ym2612Voice.h
    Source/SynthSound.h
)
