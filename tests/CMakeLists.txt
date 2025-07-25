# Test framework CMakeLists.txt for OBS Stabilizer Plugin
cmake_minimum_required(VERSION 3.16)

# Set project name for test suite
project(stabilizer_tests)

# Find required packages
find_package(OpenCV 4.5 QUIET COMPONENTS core imgproc features2d video)
if(NOT OpenCV_FOUND)
    # Try alternative package names and paths
    find_package(OpenCV QUIET COMPONENTS core imgproc features2d video)
    if(NOT OpenCV_FOUND)
        message(WARNING "OpenCV not found. Building basic tests without OpenCV functionality.")
        set(SKIP_OPENCV_TESTS ON)
    endif()
endif()

if(OpenCV_FOUND)
    message(STATUS "Found OpenCV ${OpenCV_VERSION} - Full test suite enabled")
    set(SKIP_OPENCV_TESTS OFF)
endif()

# Try to find Google Test
find_package(GTest QUIET)

if(NOT GTEST_FOUND)
    # Download and build Google Test
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50f33f9142fd5d5e60e4c4a2e65020.zip
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

# Include test source directory and modular source directories
include_directories(../src)
include_directories(../src/core)
include_directories(../src/obs)

# Create test executable with modular architecture sources
if(SKIP_OPENCV_TESTS)
    # Basic test suite without OpenCV functionality
    add_executable(
        stabilizer_tests
        test_main.cpp
    )
    
    # Link libraries (no OpenCV)
    target_link_libraries(
        stabilizer_tests
        gtest_main
    )
    
    # Add compile definitions for testing
    target_compile_definitions(stabilizer_tests PRIVATE 
        TESTING_MODE
        SKIP_OPENCV_TESTS
    )
    
    message(STATUS "Building basic test suite without OpenCV")
else()
    # Full test suite with OpenCV functionality
    add_executable(
        stabilizer_tests
        test_main.cpp
        test_stabilizer_core_new.cpp
        # Include only the core stabilizer implementation for testing
        ../src/core/stabilizer_core.cpp
    )
    
    # Link libraries
    target_link_libraries(
        stabilizer_tests
        gtest_main
        ${OpenCV_LIBS}
    )
    
    # Include directories
    target_include_directories(stabilizer_tests PRIVATE 
        ${OpenCV_INCLUDE_DIRS}
        ../src
        ../src/core
        ../src/obs
    )
    
    # Add compile definitions for testing
    target_compile_definitions(stabilizer_tests PRIVATE 
        ENABLE_STABILIZATION 
        TESTING_MODE
        OPENCV_VERSION_MAJOR=${OpenCV_VERSION_MAJOR}
        OPENCV_VERSION_MINOR=${OpenCV_VERSION_MINOR}
    )
    
    message(STATUS "Building full test suite with OpenCV ${OpenCV_VERSION}")
endif()

# Common settings
# Include directories (basic)
target_include_directories(stabilizer_tests PRIVATE 
    ../src
    ../src/core
    ../src/obs
)

# Enable testing
enable_testing()

# Add test
add_test(NAME StabilizerUnitTests COMMAND stabilizer_tests)