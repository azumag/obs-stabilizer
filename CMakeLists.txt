cmake_minimum_required(VERSION 3.16)
project(obs-stabilizer-opencv)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES "arm64")

# Find OpenCV
find_package(OpenCV REQUIRED COMPONENTS core imgproc video features2d)
message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")
message(STATUS "OpenCV include dirs: ${OpenCV_INCLUDE_DIRS}")

# OBS paths - use local headers
set(OBS_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include/obs")
set(OBS_FRONTEND_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include/obs-frontend-api")
set(OBS_LIB_DIR "/Applications/OBS.app/Contents/Frameworks")

# Check if OBS headers exist
if(NOT EXISTS "${OBS_INCLUDE_DIR}/obs-module.h")
    message(WARNING "OBS headers not found at ${OBS_INCLUDE_DIR}")
    message(STATUS "Building standalone test version")
    add_definitions(-DSTANDALONE_TEST)
else()
    message(STATUS "Found OBS headers at ${OBS_INCLUDE_DIR}")
    add_definitions(-DHAVE_OBS_HEADERS=1)
endif()

# Include directories
include_directories(
    ${OBS_INCLUDE_DIR}
    ${OBS_FRONTEND_INCLUDE_DIR}
    ${OpenCV_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Source files
set(SOURCES
    src/stabilizer_opencv.cpp
    src/plugin-support.c
    src/obs_stubs.c
)

# Create plugin library
add_library(obs-stabilizer-opencv MODULE ${SOURCES})

# Link libraries
target_link_libraries(obs-stabilizer-opencv
    ${OpenCV_LIBS}
)

# macOS specific settings
if(APPLE)
    # Find OBS library
    if(EXISTS "/Applications/OBS.app/Contents/Frameworks/libobs.framework/Versions/A/libobs")
        set(OBS_LIBRARY "/Applications/OBS.app/Contents/Frameworks/libobs.framework/Versions/A/libobs")
        message(STATUS "Found OBS library at: ${OBS_LIBRARY}")
        target_link_libraries(obs-stabilizer-opencv ${OBS_LIBRARY})
        # Allow undefined symbols to be resolved at runtime by OBS
        set_target_properties(obs-stabilizer-opencv PROPERTIES
            LINK_FLAGS "-undefined dynamic_lookup"
        )
    else()
        message(WARNING "OBS library not found, building without OBS linking")
    endif()
    
    # Set RPATH for finding OpenCV libraries at runtime
    set_target_properties(obs-stabilizer-opencv PROPERTIES
        INSTALL_RPATH "@loader_path/../Frameworks;@loader_path/../Resources/lib;/opt/homebrew/lib;/usr/local/lib"
        BUILD_WITH_INSTALL_RPATH TRUE
        MACOSX_RPATH TRUE
    )
endif()

# Output settings
set_target_properties(obs-stabilizer-opencv PROPERTIES
    PREFIX ""
    OUTPUT_NAME "obs-stabilizer-opencv"
    SUFFIX ".so"
)

# Install target
install(TARGETS obs-stabilizer-opencv
    LIBRARY DESTINATION obs-plugins
    RUNTIME DESTINATION obs-plugins
)

# Performance test executables (from src/CMakeLists.txt)
add_executable(perftest src/performance-test.cpp)
add_executable(memtest src/memory-test.cpp)
target_link_libraries(perftest ${OpenCV_LIBS})
target_link_libraries(memtest ${OpenCV_LIBS})
target_include_directories(perftest PRIVATE ${OpenCV_INCLUDE_DIRS})
target_include_directories(memtest PRIVATE ${OpenCV_INCLUDE_DIRS})

# Compiler-specific optimizations for performance tests
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(perftest PRIVATE -O3)
    target_compile_options(memtest PRIVATE -O3)
endif()

# Test configuration (from src/tests/CMakeLists.txt)
# Find Google Test
find_package(GTest REQUIRED)

# Check if we have test files
set(TEST_SOURCES "")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/tests/test_main.cpp")
    list(APPEND TEST_SOURCES src/tests/test_main.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/tests/test_exception_safety_isolated.cpp")
    list(APPEND TEST_SOURCES src/tests/test_exception_safety_isolated.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/stabilizer.cpp")
    list(APPEND TEST_SOURCES src/stabilizer.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/obs_stubs.c")
    list(APPEND TEST_SOURCES src/obs_stubs.c)
endif()

# Create test executable if we have test sources
if(TEST_SOURCES)
    add_executable(stabilizer_tests ${TEST_SOURCES})
    
    # Link against Google Test and OpenCV
    target_link_libraries(stabilizer_tests 
        GTest::gtest 
        GTest::gtest_main
        ${OpenCV_LIBS}
    )
    
    # Include directories
    target_include_directories(stabilizer_tests PRIVATE 
        ${OpenCV_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/include/obs
    )
    
    # Enable testing
    enable_testing()
    add_test(NAME stabilizer_unit_tests COMMAND stabilizer_tests)
    
    message(STATUS "Test suite enabled with sources: ${TEST_SOURCES}")
else()
    message(STATUS "No test sources found, skipping test suite")
endif()

# Print configuration summary
message(STATUS "========================================")
message(STATUS "Configuration Summary:")
message(STATUS "  OpenCV Version: ${OpenCV_VERSION}")
message(STATUS "  OBS Headers: ${OBS_INCLUDE_DIR}")
message(STATUS "  OBS Library: ${OBS_LIBRARY}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "========================================")