cmake_minimum_required(VERSION 3.22)
project(YARIFILTER VERSION 0.4.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(YARIFILTER
    COMPANY_NAME "DJ Audio Lab"
    BUNDLE_ID com.djaudiolab.yarifilter
    PLUGIN_MANUFACTURER_CODE DjAl
    PLUGIN_CODE YrF3
    FORMATS VST3 Standalone
    PRODUCT_NAME "YARIFILTER"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE)

target_sources(YARIFILTER PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/DSP/FilterEngine.cpp
    Source/DSP/EffectChain.h
    Source/DSP/ModulationProcessor.h
    Source/DSP/KillBandProcessor.h)

target_include_directories(YARIFILTER PRIVATE Source)
target_compile_definitions(YARIFILTER PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(YARIFILTER PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags
           juce::juce_recommended_lto_flags
           juce::juce_recommended_warning_flags)
