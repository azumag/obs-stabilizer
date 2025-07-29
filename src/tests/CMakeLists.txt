# Test Suite CMakeLists.txt for OBS Stabilizer Plugin
cmake_minimum_required(VERSION 3.16)

project(stabilizer_tests)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Google Test
find_package(GTest REQUIRED)

# Find OpenCV
find_package(OpenCV 4.5 QUIET)
if(OpenCV_FOUND)
    message(STATUS "Found OpenCV ${OpenCV_VERSION} - Full test suite enabled")
    add_definitions(-DENABLE_STABILIZATION)
else()
    message(STATUS "OpenCV not found - Testing interface compatibility only")
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../core)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../obs)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../include/obs)

# Add test executable with working test files only
add_executable(stabilizer_tests
    test_main.cpp
    test_exception_safety_isolated.cpp
    
    # Include source files for testing
    ../core/stabilizer_core.cpp
    ../core/stabilizer_core_debug.cpp
    ../core/error_handler.cpp
    ../core/parameter_validator.cpp
    ../core/transform_matrix.cpp
    ../plugin-support.c
    ../obs_stubs.c
)

# Link against Google Test and OpenCV if available
target_link_libraries(stabilizer_tests 
    GTest::gtest 
    GTest::gtest_main
)

if(OpenCV_FOUND)
    target_link_libraries(stabilizer_tests ${OpenCV_LIBS})
    target_include_directories(stabilizer_tests PRIVATE ${OpenCV_INCLUDE_DIRS})
endif()

# Enable testing
enable_testing()
add_test(NAME stabilizer_unit_tests COMMAND stabilizer_tests)