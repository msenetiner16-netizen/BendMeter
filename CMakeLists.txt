cmake_minimum_required(VERSION 3.22)
project(BendMeter VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "" FORCE)

# ── JUCE ──────────────────────────────────────────────────────────────────────
if(DEFINED JUCE_PATH)
    add_subdirectory(${JUCE_PATH} JUCE)
else()
    add_subdirectory($ENV{HOME}/JUCE JUCE)
endif()

# ── Plugin target ─────────────────────────────────────────────────────────────
juce_add_plugin(BendMeter
    COMPANY_NAME                "bend music"
    BUNDLE_ID                   "com.bendmusic.BendMeter"
    PLUGIN_MANUFACTURER_CODE    Bndm
    PLUGIN_CODE                 BnMt
    FORMATS                     VST3 AU
    PRODUCT_NAME                "BendMeter"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES             "Fx" "Analyzer"
    AU_MAIN_TYPE                kAudioUnitType_Effect
)

juce_add_binary_data(BendMeterAssets SOURCES
    assets/logo.svg
    assets/screw.png
    assets/wood.jpeg
    assets/paint.jpg
)

juce_generate_juce_header(BendMeter)

target_sources(BendMeter PRIVATE
    src/PluginProcessor.cpp
    src/PluginEditor.cpp
)

target_include_directories(BendMeter PRIVATE src)

target_compile_definitions(BendMeter PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(BendMeter PRIVATE
    BendMeterAssets
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
