cmake_minimum_required(VERSION 3.16)
project(obs-stabilizer-opencv)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES "arm64")

# --- OpenCV Static Linking Option ---
option(OPENCV_STATIC_LINKING "Link OpenCV statically (for deployment)" OFF)

# --- OpenCV Dependencies ---
find_package(OpenCV REQUIRED COMPONENTS core imgproc video calib3d)
include_directories(SYSTEM ${OpenCV_INCLUDE_DIRS})

# --- OBS Dependencies ---
# Allow user to specify custom OBS paths
set(OBS_INCLUDE_PATH "" CACHE PATH "Path to OBS include directory")
set(OBS_LIBRARY_PATH "" CACHE PATH "Path to OBS library directory")

# Find OBS headers with multiple search paths
find_path(OBS_INCLUDE_DIR obs.h
    PATHS
        ${OBS_INCLUDE_PATH}
        /opt/homebrew/include  # macOS Homebrew (Apple Silicon)
        /usr/local/include      # macOS Homebrew (Intel)
        /Applications/OBS.app/Contents/Frameworks/Headers
        /usr/include           # Linux system
        ${CMAKE_SOURCE_DIR}/include
    PATH_SUFFIXES obs)

find_path(OBS_FRONTEND_INCLUDE_DIR obs-frontend-api.h
    PATHS
        ${OBS_INCLUDE_PATH}
        /opt/homebrew/include  # macOS Homebrew (Apple Silicon)
        /usr/local/include      # macOS Homebrew (Intel)
        /Applications/OBS.app/Contents/Frameworks/Headers
        /usr/include           # Linux system
        ${CMAKE_SOURCE_DIR}/include)

# Find OBS library with multiple search paths
find_library(OBS_LIBRARY obs
    PATHS
        ${OBS_LIBRARY_PATH}
        /opt/homebrew/lib/obs  # macOS Homebrew (Apple Silicon)
        /usr/local/lib/obs      # macOS Homebrew (Intel)
        /Applications/OBS.app/Contents/Frameworks
        /usr/lib/obs           # Linux OBS packages
        /usr/local/lib/obs)

if(OBS_INCLUDE_DIR AND OBS_FRONTEND_INCLUDE_DIR)
    add_definitions(-DHAVE_OBS_HEADERS=1)
    message(STATUS "Found OBS headers at ${OBS_INCLUDE_DIR}")
    message(STATUS "Found OBS frontend headers at ${OBS_FRONTEND_INCLUDE_DIR}")
else()
    message(WARNING "OBS headers not found. Building in standalone mode.")
    add_definitions(-DSTANDALONE_TEST)
    add_definitions(-DBUILD_STANDALONE)
    set(OBS_INCLUDE_DIR "")
    set(OBS_FRONTEND_INCLUDE_DIR "")
endif()


# --- Include Directories ---
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${OBS_INCLUDE_DIR}
    ${OBS_FRONTEND_INCLUDE_DIR}
)

# --- Source Files ---
set(SOURCES
    src/stabilizer_opencv.cpp
    src/core/stabilizer_core.cpp
    src/core/stabilizer_wrapper.cpp
    src/core/feature_detection.cpp
    src/core/motion_classifier.cpp
    src/core/adaptive_stabilizer.cpp
    src/plugin-support.c
)

# Frame utils is only needed when OBS headers are available
if(OBS_INCLUDE_DIR AND OBS_FRONTEND_INCLUDE_DIR)
    list(APPEND SOURCES src/core/frame_utils.cpp)
endif()

# --- Main Plugin Target ---
add_library(obs-stabilizer-opencv MODULE ${SOURCES})

target_link_libraries(obs-stabilizer-opencv ${OpenCV_LIBS})
if(OBS_LIBRARY)
    target_link_libraries(obs-stabilizer-opencv ${OBS_LIBRARY})
endif()

if(APPLE)
    # Apple-specific configurations
    # Note: Accelerate framework excluded due to CarbonCore naming conflicts with OpenCV
    # Focusing on NEON feature detection which is platform-specific and doesn't require Accelerate
    target_include_directories(obs-stabilizer-opencv PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>
    )

    set_target_properties(obs-stabilizer-opencv PROPERTIES
        LINK_FLAGS "-undefined dynamic_lookup"
        INSTALL_RPATH "@loader_path/../Frameworks"
        BUILD_WITH_INSTALL_RPATH TRUE
        MACOSX_RPATH TRUE
    )
endif()

# Set platform-specific suffix for the library
if(WIN32)
    set_target_properties(obs-stabilizer-opencv PROPERTIES
        PREFIX ""
        OUTPUT_NAME "obs-stabilizer-opencv"
        SUFFIX ".dll"
    )
elseif(APPLE)
    # macOS typically uses .so or .dylib for OBS plugins
    # Using .so to match OBS plugin conventions
    set_target_properties(obs-stabilizer-opencv PROPERTIES
        PREFIX ""
        OUTPUT_NAME "obs-stabilizer-opencv"
        SUFFIX ".so"
    )
else()
    # Linux and other Unix systems
    set_target_properties(obs-stabilizer-opencv PROPERTIES
        PREFIX ""
        OUTPUT_NAME "obs-stabilizer-opencv"
        SUFFIX ".so"
    )
endif()

# --- Installation ---
install(TARGETS obs-stabilizer-opencv
    LIBRARY DESTINATION obs-plugins
    RUNTIME DESTINATION obs-plugins
)

 # --- Tests ---
 enable_testing()

 # Google Test
 find_package(GTest REQUIRED)

    # Test Sources
     set(TEST_SOURCES
        tests/test_basic.cpp
        tests/test_stabilizer_core.cpp
        tests/test_adaptive_stabilizer.cpp
        tests/test_motion_classifier.cpp
        tests/test_data_generator.cpp
        tests/test_feature_detection.cpp
        tests/test_edge_cases.cpp
        tests/test_integration.cpp
        tests/test_memory_leaks.cpp
    )

  if(TEST_SOURCES)
      message(STATUS "Found test sources, enabling test suite")
      
        # Add test source files and core source files for testing
        set(TEST_CORE_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/src/core/stabilizer_core.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/core/feature_detection.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/core/motion_classifier.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/core/adaptive_stabilizer.cpp
        )
      
      add_executable(stabilizer_tests ${TEST_SOURCES} ${TEST_CORE_SOURCES})

      target_include_directories(stabilizer_tests PRIVATE
          ${CMAKE_CURRENT_SOURCE_DIR}
          ${CMAKE_CURRENT_SOURCE_DIR}/src
          ${OBS_INCLUDE_DIR}
      )

      if(HAVE_OBS_HEADERS)
          # Use real OBS headers
          target_link_libraries(stabilizer_tests GTest::gtest_main ${OpenCV_LIBS})
      else()
          # Use stubs for standalone testing
          target_compile_definitions(stabilizer_tests PRIVATE BUILD_STANDALONE STANDALONE_TEST)
          target_link_libraries(stabilizer_tests GTest::gtest_main ${OpenCV_LIBS})
      endif()
     
     add_test(NAME stabilizer_unit_tests COMMAND stabilizer_tests)
     message(STATUS "Test suite enabled.")
 else()
      message(STATUS "No test sources found, skipping test suite.")
 endif()

  # --- Performance Testing Infrastructure ---
  option(ENABLE_PERFORMANCE_TESTS "Build performance testing infrastructure" ON)

  if(ENABLE_PERFORMANCE_TESTS)
      message(STATUS "Building performance testing infrastructure")
      
        # Performance benchmark executable
        set(PERF_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/src/core/benchmark.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/core/stabilizer_core.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/core/feature_detection.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_data_generator.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/performance_benchmark.cpp
        )

        add_executable(performance_benchmark ${PERF_SOURCES})

        # Singlerun executable - lightweight validation tool
        set(SINGLERUN_SOURCES
            tools/singlerun.cpp
            src/core/benchmark.cpp
            src/core/stabilizer_core.cpp
            src/core/feature_detection.cpp
            tests/test_data_generator.cpp
        )

        add_executable(singlerun ${SINGLERUN_SOURCES})

        target_include_directories(singlerun PRIVATE
           ${CMAKE_CURRENT_SOURCE_DIR}
           ${CMAKE_CURRENT_SOURCE_DIR}/src
           ${CMAKE_CURRENT_SOURCE_DIR}/src/core
       )

       if(NOT HAVE_OBS_HEADERS)
           target_compile_definitions(singlerun PRIVATE BUILD_STANDALONE STANDALONE_TEST)
       endif()

       target_link_libraries(singlerun ${OpenCV_LIBS})

       target_include_directories(performance_benchmark PRIVATE
          ${CMAKE_CURRENT_SOURCE_DIR}
          ${CMAKE_CURRENT_SOURCE_DIR}/src
          ${CMAKE_CURRENT_SOURCE_DIR}/src/core
      )

       if(NOT HAVE_OBS_HEADERS)
           target_compile_definitions(performance_benchmark PRIVATE BUILD_STANDALONE STANDALONE_TEST)
       endif()

       target_link_libraries(performance_benchmark ${OpenCV_LIBS})
      
      message(STATUS "Performance testing infrastructure enabled")
 endif()

