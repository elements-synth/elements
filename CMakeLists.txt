cmake_minimum_required(VERSION 3.22)
project(Elements VERSION 0.9.2 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch JUCE
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.6
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# Plugin target
juce_add_plugin(Elements
    COMPANY_NAME            "ElementsSynth"
    PLUGIN_MANUFACTURER_CODE Elem
    PLUGIN_CODE             Elmt
    FORMATS                 VST3 Standalone
    PRODUCT_NAME            "Elements"
    IS_SYNTH                TRUE
    NEEDS_MIDI_INPUT        TRUE
    NEEDS_MIDI_OUTPUT       FALSE
    IS_MIDI_EFFECT          FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    VST3_CATEGORIES         Instrument Synth
    COPY_PLUGIN_AFTER_BUILD FALSE
)

# Generate JuceHeader.h (needed by source files that #include <JuceHeader.h>)
juce_generate_juce_header(Elements)

# Source files
target_sources(Elements
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/SynthEngine.cpp
        Source/Physics.cpp
)

# Binary resources (fonts, logo, knob frames)
file(GLOB KNOB_FRAMES "custom_knob_render/knob/knob_*.png")
juce_add_binary_data(ElementsBinaryData
    SOURCES
        Source/JetBrainsMono-Regular.ttf
        Source/JetBrainsMono-Bold.ttf
        Source/elements-logo.png
        studio_kontrast_03_2k.hdr
        ${KNOB_FRAMES}
)

# Include paths
target_include_directories(Elements
    PRIVATE
        Source
)

# Compile definitions
target_compile_definitions(Elements
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

# Link JUCE modules + binary data
target_link_libraries(Elements
    PRIVATE
        ElementsBinaryData
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_opengl
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)
