cmake_minimum_required(VERSION 3.16...3.26)

# Set project name for minimal test plugin
project(obs-stabilizer-minimal VERSION 0.1.0)
add_library(${CMAKE_PROJECT_NAME} MODULE)

# === Verification ===
message(STATUS "Project name: ${CMAKE_PROJECT_NAME}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Configure target properties
set_target_properties(
  ${CMAKE_PROJECT_NAME}
  PROPERTIES AUTOMOC ON
             AUTOUIC ON
             AUTORCC ON)

# Find OBS headers and libraries using direct path method
find_path(OBS_INCLUDE_DIR obs-module.h 
    PATHS 
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include/obs
        /usr/include/obs
        /usr/local/include/obs
        /Applications/OBS.app/Contents/Frameworks/libobs.framework/Versions/A/Headers
    NO_DEFAULT_PATH
)

# Find OBS library - direct path for macOS framework
if(APPLE AND EXISTS "/Applications/OBS.app/Contents/Frameworks/libobs.framework/Versions/A/libobs")
    set(OBS_LIBRARY "/Applications/OBS.app/Contents/Frameworks/libobs.framework/Versions/A/libobs")
    message(STATUS "Found OBS library at: ${OBS_LIBRARY}")
else()
    # Fallback for other platforms
    find_library(OBS_LIBRARY NAMES libobs obs PATHS /usr/lib /usr/local/lib NO_DEFAULT_PATH)
endif()

# Set target sources - MINIMAL VERSION (no OpenCV, no stabilizer)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE ../../src/minimal_plugin_main.cpp)

# Add stub functions if OBS not found
if(NOT OBS_INCLUDE_DIR)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE ../../src/obs_stubs.c)
endif()

# Include directories
if(OBS_INCLUDE_DIR)
    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${OBS_INCLUDE_DIR})
else()
    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ../../include)
endif()

# Link libraries - MINIMAL DEPENDENCIES ONLY
if(OBS_LIBRARY)
    target_link_libraries(${CMAKE_PROJECT_NAME} ${OBS_LIBRARY})
endif()

# Set plugin properties for macOS
if(APPLE)
  set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
    BUNDLE TRUE
    BUNDLE_EXTENSION "plugin"
    OUTPUT_NAME ${CMAKE_PROJECT_NAME}
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info-minimal.plist.in"
  )
endif()

# Installation
if(APPLE)
  install(TARGETS ${CMAKE_PROJECT_NAME}
    BUNDLE DESTINATION "$ENV{HOME}/Library/Application Support/obs-studio/plugins/"
    LIBRARY DESTINATION "$ENV{HOME}/Library/Application Support/obs-studio/plugins/"
    COMPONENT Runtime)
endif()