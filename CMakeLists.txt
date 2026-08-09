cmake_minimum_required(VERSION 3.22)
project(JARIFILTER VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(JARIFILTER
    COMPANY_NAME "DJ Audio Lab"
    BUNDLE_ID com.djaudiolab.jarifilter
    PLUGIN_MANUFACTURER_CODE DjAl
    PLUGIN_CODE JrFl
    FORMATS VST3 Standalone
    PRODUCT_NAME "JARIFILTER"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE)

target_sources(JARIFILTER PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/DSP/FilterEngine.cpp
    Source/DSP/EffectChain.h
    Source/DSP/KillBandProcessor.h)

target_include_directories(JARIFILTER PRIVATE Source)
target_compile_definitions(JARIFILTER PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(JARIFILTER PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags
           juce::juce_recommended_lto_flags
           juce::juce_recommended_warning_flags)
