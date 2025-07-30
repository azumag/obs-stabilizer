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

# Find OpenCV
find_package(OpenCV REQUIRED)

# Find OBS headers and libraries
find_path(OBS_INCLUDE_DIR obs-module.h 
    PATHS 
        ${CMAKE_CURRENT_SOURCE_DIR}/include/obs
        /usr/include/obs
        /usr/local/include/obs
        /Applications/OBS.app/Contents/Frameworks/libobs.framework/Versions/A/Headers
    NO_DEFAULT_PATH
)

# Find OBS library - try multiple approaches for macOS
if(APPLE AND EXISTS "/Applications/OBS.app/Contents/Frameworks/libobs.framework/Versions/A/libobs")
    # Direct path to macOS framework library
    set(OBS_LIBRARY "/Applications/OBS.app/Contents/Frameworks/libobs.framework/Versions/A/libobs")
    message(STATUS "Found OBS library at: ${OBS_LIBRARY}")
else()
    # Fallback to standard find_library for other platforms
    find_library(OBS_LIBRARY 
        NAMES libobs obs
        PATHS
            /usr/lib
            /usr/local/lib
        NO_DEFAULT_PATH
    )
endif()

# Create the plugin library
add_library(${CMAKE_PROJECT_NAME} SHARED)

# Add simplified source files
target_sources(${CMAKE_PROJECT_NAME} PRIVATE 
    src/plugin_main.cpp
    src/obs_plugin.cpp
    src/stabilizer.cpp
    src/plugin-support.c
)

# Add stub functions for CI/CD builds without OBS library
if(NOT OBS_INCLUDE_DIR)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE src/obs_stubs.c)
endif()

# Link libraries (OBS library must come before OpenCV)
target_link_libraries(${CMAKE_PROJECT_NAME} 
    ${OpenCV_LIBS}
)

# Link OBS library if found
if(OBS_LIBRARY)
    target_link_libraries(${CMAKE_PROJECT_NAME} ${OBS_LIBRARY})
    message(STATUS "Linking with OBS library: ${OBS_LIBRARY}")
else()
    message(WARNING "OBS library not found - plugin may not load properly")
endif()

# Include directories
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${OpenCV_INCLUDE_DIRS}
)

# Add OBS include directory if found
if(OBS_INCLUDE_DIR)
    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${OBS_INCLUDE_DIR})
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE HAVE_OBS_HEADERS=1)
    message(STATUS "Building with OBS headers from: ${OBS_INCLUDE_DIR}")
else()
    message(WARNING "OBS headers not found - plugin may not load properly")
endif()

# Plugin properties
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
    PREFIX ""
    SUFFIX ""
    OUTPUT_NAME "obs-stabilizer"
)

# Platform-specific settings
if(APPLE)
    set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
        BUNDLE TRUE
        BUNDLE_EXTENSION "plugin"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Info.plist.in"
    )
    
    # Apply macOS plugin fixes
    include(cmake/macOS-plugin-fix.cmake)
    fix_macos_plugin_binary(${CMAKE_PROJECT_NAME})
endif()

message(STATUS "Building OBS Stabilizer Plugin v${CMAKE_PROJECT_VERSION} (Simplified Architecture)")