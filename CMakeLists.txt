cmake_minimum_required(VERSION 3.22)

project(SHPPluginManager
    VERSION 0.1.0
    DESCRIPTION "SHP Plugin Manager — installs and updates SHP VST3 plugins"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

set(SHP_JUCE_TAG "8.0.12" CACHE STRING "JUCE git tag to fetch")

set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(JUCE_BUILD_EXTRAS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG ${SHP_JUCE_TAG}
    GIT_SHALLOW TRUE)

FetchContent_MakeAvailable(juce)

juce_add_gui_app(SHPPluginManager
    PRODUCT_NAME "SHP Plugin Manager"
    COMPANY_NAME "Sombre Harfang Productions"
    BUNDLE_ID "com.sombreharfangproductions.shppluginmanager"
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/assets/shp_logo_v3.png")

juce_generate_juce_header(SHPPluginManager)

juce_add_binary_data(ManagerAssets
    HEADER_NAME "BinaryData.h"
    NAMESPACE "BinaryData"
    SOURCES
        assets/shp_logo_v3.png)

target_sources(SHPPluginManager
    PRIVATE
        Source/Main.cpp
        Source/MainWindow.h
        Source/MainWindow.cpp
        Source/MainComponent.h
        Source/MainComponent.cpp
        Source/PluginCardComponent.h
        Source/PluginCardComponent.cpp
        Source/PluginInfo.h
        Source/RegistryClient.h
        Source/RegistryClient.cpp
        Source/InstallManager.h
        Source/InstallManager.cpp
        Source/VersionTracker.h
        Source/VersionTracker.cpp
        Source/Settings.h
        Source/Settings.cpp
        Source/SettingsPage.h
        Source/SettingsPage.cpp
        Source/ElevationHelper.h
        Source/ElevationHelper.cpp
        Source/UpdateChecker.h
        Source/UpdateChecker.cpp
        Source/ShpLookAndFeel.h
        Source/ShpLookAndFeel.cpp
        Source/Theme.h)

target_compile_definitions(SHPPluginManager
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=1
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:SHPPluginManager,JUCE_PROJECT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:SHPPluginManager,JUCE_VERSION>")

target_link_libraries(SHPPluginManager
    PRIVATE
        ManagerAssets
        juce::juce_gui_extra
        juce::juce_gui_basics
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

target_include_directories(SHPPluginManager PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/juce_binarydata_ManagerAssets/JuceLibraryCode")
