cmake_minimum_required(VERSION 3.16...3.30)

# OBS Stabilizer Plugin - Simplified Architecture
project(obs-stabilizer VERSION 0.1.0)

# Language standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

# Compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O2 -g -DNDEBUG")
endif()

# Find OpenCV - Re-enabled for full plugin with proper library path handling
find_package(OpenCV REQUIRED COMPONENTS core imgproc video features2d)

# Skip Qt6 for now - create Qt-independent minimal plugin
# This eliminates Qt version conflicts while maintaining plugin functionality
# find_package(Qt6 6.8 REQUIRED COMPONENTS Core Widgets)

# Build options
option(BUILD_STANDALONE "Build standalone executable for testing" OFF)

# Find OBS headers and libraries - revert to working direct path method
if(NOT BUILD_STANDALONE)
    find_path(OBS_INCLUDE_DIR obs-module.h 
        PATHS 
            ${CMAKE_CURRENT_SOURCE_DIR}/../../include/obs
            /usr/include/obs
            /usr/local/include/obs
            /Applications/OBS.app/Contents/Frameworks/libobs.framework/Versions/A/Headers
        NO_DEFAULT_PATH
    )
endif()

# Find OBS library - direct path for macOS framework
if(NOT BUILD_STANDALONE)
    if(APPLE AND EXISTS "/Applications/OBS.app/Contents/Frameworks/libobs.framework/Versions/A/libobs")
        set(OBS_LIBRARY "/Applications/OBS.app/Contents/Frameworks/libobs.framework/Versions/A/libobs")
        message(STATUS "Found OBS library at: ${OBS_LIBRARY}")
    else()
        # Fallback for other platforms
        find_library(OBS_LIBRARY NAMES libobs obs PATHS /usr/lib /usr/local/lib NO_DEFAULT_PATH)
    endif()
endif()

# Skip OBS frontend API - not needed for basic video filters
# Most OBS plugins work without frontend API dependency

# Create the plugin library or standalone executable
if(BUILD_STANDALONE)
    add_executable(${CMAKE_PROJECT_NAME}-standalone src/standalone_test.cpp)
    message(STATUS "Building standalone executable for CI/CD testing")
else()
    add_library(${CMAKE_PROJECT_NAME} MODULE)
endif()

# Set target sources - use minimal plugin main with OpenCV support
if(NOT BUILD_STANDALONE)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE 
        ../../src/minimal_plugin_main.cpp
        ../../src/plugin-support.c
    )
endif()

# Add stub functions for CI/CD builds without OBS library
if(NOT OBS_INCLUDE_DIR AND NOT BUILD_STANDALONE)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE ../../src/obs_stubs.c)
endif()

# Link OpenCV libraries - Re-enabled with absolute path handling
if(BUILD_STANDALONE)
    target_link_libraries(${CMAKE_PROJECT_NAME}-standalone 
        ${OpenCV_LIBS}
    )
else()
    target_link_libraries(${CMAKE_PROJECT_NAME} 
        ${OpenCV_LIBS}
    )
    
    # Link Qt libraries - TEMPORARILY DISABLED FOR Qt-independent plugin
    # target_link_libraries(${CMAKE_PROJECT_NAME} Qt6::Core Qt6::Widgets)
    
    # Link libraries - only if OBS_LIBRARY is found
    if(OBS_LIBRARY)
        target_link_libraries(${CMAKE_PROJECT_NAME} ${OBS_LIBRARY})
    endif()
    
    # Set proper rpath to find OpenCV libraries
    # This ensures OBS can find OpenCV libraries at runtime
    if(APPLE AND OpenCV_LIBS)
        set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
            INSTALL_RPATH "/opt/homebrew/lib;${CMAKE_INSTALL_PREFIX}/lib"
            BUILD_WITH_INSTALL_RPATH TRUE
        )
        message(STATUS "Set rpath to include OpenCV library directory: /opt/homebrew/lib")
    endif()
endif()

# Skip OBS frontend API linking - not required for video filters

# Include directories
if(BUILD_STANDALONE)
    target_include_directories(${CMAKE_PROJECT_NAME}-standalone PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/../../src
    )
else()
    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/../../src
    )
    
    # Add OBS include directory if found  
    if(OBS_INCLUDE_DIR)
        target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${OBS_INCLUDE_DIR})
        target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE HAVE_OBS_HEADERS=1)
        message(STATUS "Building with OBS headers from: ${OBS_INCLUDE_DIR}")
    endif()
endif()

# Plugin properties
if(NOT BUILD_STANDALONE)
    set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
        PREFIX ""
        SUFFIX ""
        OUTPUT_NAME "test-stabilizer"
    )
endif()

# Platform-specific settings
if(APPLE AND NOT BUILD_STANDALONE)
    set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
        BUNDLE TRUE
        BUNDLE_EXTENSION "plugin"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/../../Info.plist.in"
        INSTALL_RPATH "@loader_path/../Frameworks;@executable_path/../Frameworks;/Applications/OBS.app/Contents/Frameworks"
        BUILD_WITH_INSTALL_RPATH TRUE
    )
    
    # macOS plugin fixes no longer needed with proper rpath configuration
endif()

message(STATUS "Building OBS Stabilizer Plugin v${CMAKE_PROJECT_VERSION} (Simplified Architecture)")