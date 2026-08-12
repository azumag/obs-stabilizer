cmake_minimum_required(VERSION 3.16)
project(obs-stabilizer-opencv VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Allow user to specify architecture via -DCMAKE_OSX_ARCHITECTURES
# Default to arm64 only for Apple Silicon, user can override with -DCMAKE_OSX_ARCHITECTURES="x86_64"
if(APPLE AND NOT CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Build architectures" FORCE)
    message(STATUS "Setting default CMAKE_OSX_ARCHITECTURES to arm64 (Apple Silicon)")
endif()

# --- OpenCV Static Linking Option ---
option(OPENCV_STATIC_LINKING "Link OpenCV statically (for deployment)" OFF)

# --- OpenCV Dependencies ---
find_package(OpenCV REQUIRED COMPONENTS core imgproc video calib3d features2d flann)
include_directories(SYSTEM ${OpenCV_INCLUDE_DIRS})

# --- nlohmann/json Dependency (for PresetManager) ---
find_path(NLOHMANN_JSON_INCLUDE_DIR nlohmann/json.hpp
    PATHS
        /opt/homebrew/include  # macOS Homebrew (Apple Silicon)
        /usr/local/include      # macOS Homebrew (Intel)
        /usr/include           # Linux system
        C:/vcpkg/installed/x64-windows/include  # Windows vcpkg
        $ENV{VCPKG_ROOT}/installed/x64-windows/include  # Windows vcpkg (environment variable)
        ${CMAKE_SOURCE_DIR}/include
)

if(NLOHMANN_JSON_INCLUDE_DIR)
    include_directories(SYSTEM ${NLOHMANN_JSON_INCLUDE_DIR})
    message(STATUS "Found nlohmann/json: ${NLOHMANN_JSON_INCLUDE_DIR}")
else()
    message(WARNING "nlohmann/json not found, PresetManager will not work in standalone mode")
endif()

# --- OBS Dependencies ---
# Allow user to specify custom OBS paths
set(OBS_INCLUDE_PATH "" CACHE PATH "Path to OBS include directory")
set(OBS_LIBRARY_PATH "" CACHE PATH "Path to OBS library directory")
set(OBS_SOURCE_DIR "" CACHE PATH "Path to an OBS Studio source tree")
set(HAVE_OBS_HEADERS FALSE)

# On macOS, always compile the distributable plugin against the official OBS
# headers. CI runners do not have OBS.app installed, so falling back to the
# standalone stubs here would produce a bundle with no usable OBS module.
if(APPLE)
    if(NOT OBS_SOURCE_DIR)
        include(FetchContent)
        set(OBS_SOURCE_URL https://github.com/obsproject/obs-studio/archive/refs/tags/30.0.0.tar.gz)
        set(OBS_SOURCE_HASH SHA256=c23dd463862b1a8f40365d84fd52105d7eafc3614fb3d470b695ba28a6e4da06)

        # Build against the documented minimum so the module ABI remains
        # loadable by OBS 30.x as well as newer OBS releases.
        if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.18)
            FetchContent_Declare(
                obs_source_headers
                URL ${OBS_SOURCE_URL}
                URL_HASH ${OBS_SOURCE_HASH}
                DOWNLOAD_EXTRACT_TIMESTAMP TRUE
                SOURCE_SUBDIR _headers_only
            )
            FetchContent_MakeAvailable(obs_source_headers)
        else()
            FetchContent_Declare(
                obs_source_headers
                URL ${OBS_SOURCE_URL}
                URL_HASH ${OBS_SOURCE_HASH}
                DOWNLOAD_EXTRACT_TIMESTAMP TRUE
            )
            FetchContent_GetProperties(obs_source_headers)
            if(NOT obs_source_headers_POPULATED)
                FetchContent_Populate(obs_source_headers)
            endif()
        endif()
        set(OBS_SOURCE_DIR "${obs_source_headers_SOURCE_DIR}")
    endif()

    if(NOT EXISTS "${OBS_SOURCE_DIR}/libobs/obs-module.h")
        message(FATAL_ERROR "OBS source headers not found under ${OBS_SOURCE_DIR}/libobs")
    endif()

    set(OBS_INCLUDE_DIR "${OBS_SOURCE_DIR}/libobs")
    set(OBS_FRONTEND_INCLUDE_DIR "")
    set(OBS_RELEASE_CANDIDATE 0)
    set(OBS_BETA 0)
    set(OBS_GENERATED_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/obs-generated")
    file(MAKE_DIRECTORY "${OBS_GENERATED_INCLUDE_DIR}")
    configure_file(
        "${OBS_INCLUDE_DIR}/obsconfig.h.in"
        "${OBS_GENERATED_INCLUDE_DIR}/obsconfig.h"
        @ONLY
    )
    set(HAVE_OBS_HEADERS TRUE)
    message(STATUS "OBS headers: ${OBS_INCLUDE_DIR}")
    set(OBS_LIBRARY "")
    message(STATUS "OBS symbols will resolve when the plugin is loaded")
else()
    # Windows/Linux OBS header search
    find_path(OBS_INCLUDE_DIR obs.h
        PATHS
            ${OBS_INCLUDE_PATH}
            /usr/include           # Linux system
            ${CMAKE_SOURCE_DIR}/include
        PATH_SUFFIXES obs)

    find_library(OBS_LIBRARY obs
        PATHS
            ${OBS_LIBRARY_PATH}
            /usr/lib/obs           # Linux OBS packages
            /usr/local/lib/obs)

    if(OBS_INCLUDE_DIR AND OBS_LIBRARY)
        set(HAVE_OBS_HEADERS TRUE)
        message(STATUS "Found OBS headers at ${OBS_INCLUDE_DIR}")
        message(STATUS "Found OBS library at ${OBS_LIBRARY}")
    else()
        message(WARNING "OBS not found. Building in standalone mode.")
        add_definitions(-DSTANDALONE_TEST)
        add_definitions(-DBUILD_STANDALONE)
        set(OBS_INCLUDE_DIR "")
        set(OBS_FRONTEND_INCLUDE_DIR "")
    endif()
endif()


# --- Include Directories ---
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OBS_INCLUDE_DIR}
    ${OBS_FRONTEND_INCLUDE_DIR}
    ${OBS_GENERATED_INCLUDE_DIR}
)

# --- Source Files ---
set(SOURCES
    src/stabilizer_opencv.cpp
    src/core/stabilizer_core.cpp
    src/core/stabilizer_wrapper.cpp
    src/core/preset_manager.cpp
    src/core/kalman_transform_filter.cpp
    src/ui/stabilizer_properties.cpp
)

# Frame utils is only needed when OBS headers are available
if(HAVE_OBS_HEADERS)
    list(APPEND SOURCES src/core/frame_utils.cpp)
endif()

# --- Main Plugin Target ---
add_library(obs-stabilizer-opencv MODULE ${SOURCES})

if(HAVE_OBS_HEADERS)
    target_compile_definitions(
        obs-stabilizer-opencv
        PRIVATE HAVE_OBS_HEADERS=1 OBS_STABILIZER_USE_REAL_OBS=1
    )
endif()

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

    # Get OpenCV library directory for rpath
    # Use OpenCVConfig.cmake variables directly
    set(OPENCV_LIB_REALDIR "${OpenCV_DIR}/../lib")
    get_filename_component(OPENCV_LIB_REALDIR "${OPENCV_LIB_REALDIR}" REALPATH)

    # Additional fallback search
    if(NOT EXISTS "${OPENCV_LIB_REALDIR}")
        if(EXISTS "/opt/homebrew/opt/opencv/lib")
            set(OPENCV_LIB_REALDIR "/opt/homebrew/opt/opencv/lib")
        elseif(EXISTS "/usr/local/opt/opencv/lib")
            set(OPENCV_LIB_REALDIR "/usr/local/opt/opencv/lib")
        endif()
    endif()

    message(STATUS "OpenCV library directory for rpath: ${OPENCV_LIB_REALDIR}")

    # Set rpath to include OpenCV library directory and OBS framework
    # This ensures OBS can find the libraries at runtime
    set(RPATH_DIRS "")

    # Add OpenCV rpath
    if(EXISTS "${OPENCV_LIB_REALDIR}")
        set(RPATH_DIRS "${OPENCV_LIB_REALDIR}")
    endif()

    # Keep bundled dependencies next to the plugin binary while retaining a
    # development fallback for locally installed OpenCV.
    list(PREPEND RPATH_DIRS "@loader_path/../Frameworks")

    # Set link flags and rpath
    set_target_properties(obs-stabilizer-opencv PROPERTIES
        LINK_FLAGS "-undefined dynamic_lookup"
        INSTALL_RPATH "${RPATH_DIRS}"
        BUILD_WITH_INSTALL_RPATH TRUE
        MACOSX_RPATH TRUE
    )

    # For static linking (if enabled in future)
    if(OPENCV_STATIC_LINKING)
        set_target_properties(obs-stabilizer-opencv PROPERTIES
            INSTALL_RPATH ""
            BUILD_WITH_INSTALL_RPATH FALSE
            MACOSX_RPATH FALSE
        )
    endif()
endif()

# Set platform-specific suffix for the library
if(WIN32)
    set_target_properties(obs-stabilizer-opencv PROPERTIES
        PREFIX ""
        OUTPUT_NAME "obs-stabilizer-opencv"
        SUFFIX ".dll"
    )
elseif(APPLE)
    # OBS discovers macOS plugins as CFBundle directories. The target name stays
    # unchanged for CMake, while the produced artifact is obs-stabilizer.plugin.
    set(OBS_STABILIZER_BUNDLE_NAME "OBS Stabilizer")
    set(OBS_STABILIZER_BUNDLE_EXECUTABLE "obs-stabilizer")
    set(OBS_STABILIZER_BUNDLE_IDENTIFIER "com.azumag.obs-stabilizer")
    set(OBS_STABILIZER_INFO_PLIST "${CMAKE_CURRENT_BINARY_DIR}/obs-stabilizer-Info.plist")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Info.plist.in"
        "${OBS_STABILIZER_INFO_PLIST}"
        @ONLY
    )

    set_target_properties(obs-stabilizer-opencv PROPERTIES
        PREFIX ""
        OUTPUT_NAME "${OBS_STABILIZER_BUNDLE_EXECUTABLE}"
        SUFFIX ""
        BUNDLE TRUE
        BUNDLE_EXTENSION plugin
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Info.plist.in"
        MACOSX_BUNDLE_GUI_IDENTIFIER "${OBS_STABILIZER_BUNDLE_IDENTIFIER}"
        MACOSX_BUNDLE_BUNDLE_NAME "${OBS_STABILIZER_BUNDLE_NAME}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        CXX_VISIBILITY_PRESET hidden
    )

    # Some non-Xcode generators omit bundle metadata for MODULE targets. Make
    # the installable OBS bundle complete regardless of the chosen generator.
    add_custom_command(TARGET obs-stabilizer-opencv POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_BUNDLE_CONTENT_DIR:obs-stabilizer-opencv>/Resources"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OBS_STABILIZER_INFO_PLIST}"
            "$<TARGET_BUNDLE_CONTENT_DIR:obs-stabilizer-opencv>/Info.plist"
        COMMENT "Completing the macOS OBS plugin bundle"
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
if(APPLE)
    install(TARGETS obs-stabilizer-opencv
        BUNDLE DESTINATION .
        LIBRARY DESTINATION .
    )
else()
    install(TARGETS obs-stabilizer-opencv
        LIBRARY DESTINATION obs-plugins
        RUNTIME DESTINATION obs-plugins
    )
endif()

 # --- Tests ---
  enable_testing()

  # Test Coverage Option
  option(ENABLE_TEST_COVERAGE "Enable test coverage with gcov/lcov" OFF)

  # Google Test
  find_package(GTest REQUIRED)

        # Test Sources
        set(TEST_SOURCES
        tests/test_basic.cpp
        tests/test_stabilizer_core.cpp
        tests/test_data_generator.cpp
        tests/test_edge_cases.cpp
        tests/test_integration.cpp
        tests/test_memory_leaks.cpp
        tests/test_visual_quality.cpp
        tests/test_performance_thresholds.cpp
        tests/test_multi_source.cpp
        tests/test_preset_manager.cpp
        tests/test_thread_safety.cpp
        tests/test_frame_utils.cpp
        tests/test_obs_integration.cpp
    )

    if(TEST_SOURCES)
        message(STATUS "Found test sources, enabling test suite")

            # Add test source files and core source files for testing
            set(TEST_CORE_SOURCES
                ${CMAKE_CURRENT_SOURCE_DIR}/src/core/stabilizer_core.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/src/core/stabilizer_wrapper.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/src/core/preset_manager.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/src/core/frame_utils.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/src/core/kalman_transform_filter.cpp
            )

        add_executable(stabilizer_tests ${TEST_SOURCES} ${TEST_CORE_SOURCES})

       # Enable coverage flags if requested
       if(ENABLE_TEST_COVERAGE)
           message(STATUS "Enabling test coverage with gcov/lcov")
           target_compile_options(stabilizer_tests PRIVATE --coverage)
           target_link_options(stabilizer_tests PRIVATE --coverage)
       endif()

      target_include_directories(stabilizer_tests PRIVATE
          ${CMAKE_CURRENT_SOURCE_DIR}
          ${CMAKE_CURRENT_SOURCE_DIR}/src
          ${CMAKE_CURRENT_SOURCE_DIR}/include
          ${OBS_INCLUDE_DIR}
      )

      # Unit tests use the isolated OBS stubs even when a real OBS SDK is
      # available for the plugin target.
      target_compile_definitions(
          stabilizer_tests
          PRIVATE BUILD_STANDALONE STANDALONE_TEST HAVE_OBS_HEADERS=1
      )
      target_link_libraries(stabilizer_tests GTest::gtest_main ${OpenCV_LIBS})
     
     add_test(NAME stabilizer_unit_tests COMMAND stabilizer_tests)
     if(UNIX)
         add_test(
             NAME macos_packaging_scripts
             COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_macos_packaging_scripts.sh
         )
         set_tests_properties(
             macos_packaging_scripts
             PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
         )
     endif()
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
              ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_data_generator.cpp
              ${CMAKE_CURRENT_SOURCE_DIR}/tools/performance_benchmark.cpp
          )

        add_executable(performance_benchmark ${PERF_SOURCES})

          # Singlerun executable - lightweight validation tool
          set(SINGLERUN_SOURCES
              tools/singlerun.cpp
              src/core/benchmark.cpp
              src/core/stabilizer_core.cpp
              tests/test_data_generator.cpp
          )

        add_executable(singlerun ${SINGLERUN_SOURCES})

        target_include_directories(singlerun PRIVATE
           ${CMAKE_CURRENT_SOURCE_DIR}
           ${CMAKE_CURRENT_SOURCE_DIR}/src
           ${CMAKE_CURRENT_SOURCE_DIR}/src/core
       )

       target_compile_definitions(singlerun PRIVATE BUILD_STANDALONE STANDALONE_TEST)

       target_link_libraries(singlerun ${OpenCV_LIBS})

       target_include_directories(performance_benchmark PRIVATE
          ${CMAKE_CURRENT_SOURCE_DIR}
          ${CMAKE_CURRENT_SOURCE_DIR}/src
          ${CMAKE_CURRENT_SOURCE_DIR}/src/core
      )

       target_compile_definitions(performance_benchmark PRIVATE BUILD_STANDALONE STANDALONE_TEST)

       target_link_libraries(performance_benchmark ${OpenCV_LIBS})
      
      message(STATUS "Performance testing infrastructure enabled")
 endif()
