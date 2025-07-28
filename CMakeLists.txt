cmake_minimum_required(VERSION 3.16...3.30)

# OBS Stabilizer Plugin - Simple build configuration
project(obs-stabilizer VERSION 0.1.0)

# C++ standard
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

# Create plugin library
add_library(${CMAKE_PROJECT_NAME} SHARED)

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

# Core source files
target_sources(${CMAKE_PROJECT_NAME} PRIVATE 
    src/plugin-main.cpp
    src/core/stabilizer_core.cpp
    src/core/stabilizer_core_debug.cpp
    src/core/error_handler.cpp
    src/core/parameter_validator.cpp
    src/core/transform_matrix.cpp
    src/obs/obs_integration.cpp
)

# Include directories
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/obs
)

# Plugin properties
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES 
    OUTPUT_NAME obs-stabilizer
    VERSION ${CMAKE_PROJECT_VERSION}
    SOVERSION ${CMAKE_PROJECT_VERSION_MAJOR}
)

message(STATUS "Building OBS Stabilizer Plugin v${CMAKE_PROJECT_VERSION}")