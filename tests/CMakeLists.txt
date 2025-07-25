# Test framework CMakeLists.txt for OBS Stabilizer Plugin
cmake_minimum_required(VERSION 3.16)

# Find required packages
find_package(OpenCV REQUIRED COMPONENTS core imgproc features2d video)

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

# Include test source directory
include_directories(../src)

# Create test executable
add_executable(
    stabilizer_tests
    test_main.cpp
    test_stabilizer_core.cpp
    test_feature_tracking.cpp
    test_transform_smoothing.cpp
    ../src/plugin-main.cpp  # Include source for testing
)

# Link libraries
target_link_libraries(
    stabilizer_tests
    gtest_main
    ${OpenCV_LIBS}
)

# Include directories
target_include_directories(stabilizer_tests PRIVATE ${OpenCV_INCLUDE_DIRS})

# Set C++ standard
set_property(TARGET stabilizer_tests PROPERTY CXX_STANDARD 17)

# Add compile definitions for testing
target_compile_definitions(stabilizer_tests PRIVATE -DENABLE_STABILIZATION -DTESTING_MODE)

# Enable testing
enable_testing()

# Add test
add_test(NAME StabilizerUnitTests COMMAND stabilizer_tests)