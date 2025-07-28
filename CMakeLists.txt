cmake_minimum_required(VERSION 3.16...3.30)

# OBS Stabilizer Plugin - Simple build configuration
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

# Build type options
option(BUILD_STANDALONE "Build as standalone library instead of OBS plugin" OFF)

# Create library type based on BUILD_STANDALONE option or OBS availability
if(BUILD_STANDALONE)
    add_library(${CMAKE_PROJECT_NAME} SHARED)
    message(STATUS "Building as standalone library (BUILD_STANDALONE=ON)")
elseif(OBS_INCLUDE_DIR)
    add_library(${CMAKE_PROJECT_NAME} SHARED)
    message(STATUS "Building as OBS plugin (shared library)")
else()
    add_executable(${CMAKE_PROJECT_NAME})
    message(STATUS "Building as standalone executable for testing")
endif()

# Find minimal OBS headers for plugin development
find_path(OBS_INCLUDE_DIR obs-module.h 
    PATHS 
        ${CMAKE_CURRENT_SOURCE_DIR}/include/obs
        /usr/include/obs
        /usr/local/include/obs
        /opt/homebrew/include/obs
        $ENV{OBS_STUDIO_PATH}/include
    DOC "OBS Studio header directory"
)

# Find OBS libraries when available (more comprehensive linking)
find_library(OBS_LIBRARY obs
    PATHS
        /usr/lib
        /usr/local/lib
        /opt/homebrew/lib
        $ENV{OBS_STUDIO_PATH}/lib
    DOC "OBS Studio library"
)

if(BUILD_STANDALONE)
    message(STATUS "Building standalone - skipping OBS integration")
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE BUILD_STANDALONE_TEST)
elseif(OBS_INCLUDE_DIR)
    message(STATUS "Found OBS headers at: ${OBS_INCLUDE_DIR}")
    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${OBS_INCLUDE_DIR})
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE HAVE_OBS_HEADERS)
    
    # Link OBS libraries when available
    if(OBS_LIBRARY)
        message(STATUS "Found OBS library at: ${OBS_LIBRARY}")
        target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${OBS_LIBRARY})
    else()
        message(WARNING "OBS headers found but libraries not found - plugin may have linking issues")
    endif()
else()
    message(WARNING "OBS headers not found. Building as standalone library for testing.")
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE BUILD_STANDALONE_TEST)
endif()

# Find OpenCV
find_package(OpenCV QUIET COMPONENTS core imgproc features2d video calib3d)
if(OpenCV_FOUND)
    message(STATUS "Found OpenCV ${OpenCV_VERSION}")
    set(ENABLE_STABILIZATION ON)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${OpenCV_LIBS})
    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${OpenCV_INCLUDE_DIRS})
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE ENABLE_STABILIZATION)
    message(STATUS "OpenCV enabled with libraries: ${OpenCV_LIBS}")
else()
    message(WARNING "OpenCV not found. Building without stabilization features.")
    set(ENABLE_STABILIZATION OFF)
endif()

# Use existing plugin-support.c directly (no template processing needed)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/plugin-support.c)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE src/plugin-support.c)
    message(STATUS "Using existing plugin-support.c")
else()
    message(WARNING "plugin-support.c not found - plugin may not register properly")
endif()

# Add OBS stub functions when needed
if(BUILD_STANDALONE OR (NOT OBS_INCLUDE_DIR) OR (OBS_INCLUDE_DIR AND NOT OBS_LIBRARY))
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE src/obs_stubs.c)
    if(BUILD_STANDALONE)
        message(STATUS "Using OBS stub functions for standalone build")
    elseif(OBS_INCLUDE_DIR AND NOT OBS_LIBRARY)
        message(STATUS "Using OBS stub functions - headers found but no libraries")
    else()
        message(STATUS "Using OBS stub functions - no OBS installation found")
    endif()
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE USE_OBS_STUBS)
else()
    message(STATUS "Full OBS installation found - using system OBS functions")
endif()

# Core source files (always included)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE 
    src/plugin-main.cpp
    src/core/stabilizer_core.cpp
    src/core/stabilizer_core_debug.cpp
    src/core/error_handler.cpp
    src/core/parameter_validator.cpp
    src/core/transform_matrix.cpp
)

# OBS-specific source files (only when not standalone and OBS headers available)
if(NOT BUILD_STANDALONE AND OBS_INCLUDE_DIR)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE 
        src/obs/obs_integration.cpp
    )
endif()

# Include directories
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/obs
)

# Plugin properties
if(NOT BUILD_STANDALONE AND OBS_INCLUDE_DIR AND APPLE)
    # Build as macOS plugin bundle
    set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES 
        OUTPUT_NAME obs-stabilizer
        VERSION ${CMAKE_PROJECT_VERSION}
        SOVERSION ${CMAKE_PROJECT_VERSION_MAJOR}
        BUNDLE TRUE
        BUNDLE_EXTENSION "plugin"
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
    )
else()
    # Build as regular library/executable for testing
    set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES 
        OUTPUT_NAME obs-stabilizer
        VERSION ${CMAKE_PROJECT_VERSION}
    )
endif()

# Install locale files for plugin bundle
if(NOT BUILD_STANDALONE AND OBS_INCLUDE_DIR AND APPLE)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/data/locale/en-US.ini
        DESTINATION ${CMAKE_PROJECT_NAME}.plugin/Contents/Resources/locale
    )
endif()

# Include macOS plugin fix module
if(APPLE AND NOT BUILD_STANDALONE AND OBS_INCLUDE_DIR)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/macOS-plugin-fix.cmake)
    message(STATUS "Included macOS plugin loading fix")
endif()

message(STATUS "Building OBS Stabilizer Plugin v${CMAKE_PROJECT_VERSION}")