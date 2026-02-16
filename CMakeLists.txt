cmake_minimum_required(VERSION 3.16)
project(obs-stabilizer-opencv)

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

# For macOS, prefer OBS app bundle
if(APPLE)
    # Check if OBS.app exists
    if(EXISTS "/Applications/OBS.app")
        # Use minimal stub header for compilation (doesn't require full OBS headers)
        set(OBS_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")
        set(OBS_FRONTEND_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")

        # OBS framework exists for runtime linking
        set(OBS_LIBRARY "/Applications/OBS.app/Contents/Frameworks/libobs.framework")

        # Set HAVE_OBS_HEADERS as a CMake variable and preprocessor definition
        # RATIONALE: This variable is used in CMakeLists.txt conditional checks
        # to determine which source files to include (e.g., frame_utils.cpp).
        set(HAVE_OBS_HEADERS TRUE CACHE BOOL "OBS headers are available" FORCE)
        add_definitions(-DHAVE_OBS_HEADERS=1)
        message(STATUS "Using OBS.app at /Applications/OBS.app")
        message(STATUS "OBS headers: ${OBS_INCLUDE_DIR}")
        message(STATUS "OBS library: ${OBS_LIBRARY}")
    else()
        # OBS.app not found, try other locations
        find_path(OBS_INCLUDE_DIR obs.h
            PATHS
                ${OBS_INCLUDE_PATH}
                /opt/homebrew/include  # macOS Homebrew (Apple Silicon)
                /usr/local/include      # macOS Homebrew (Intel)
                ${CMAKE_SOURCE_DIR}/include
            PATH_SUFFIXES obs)

        if(OBS_INCLUDE_DIR)
            find_library(OBS_LIBRARY obs
                PATHS
                    ${OBS_LIBRARY_PATH}
                    /opt/homebrew/lib/obs  # macOS Homebrew (Apple Silicon)
                    /usr/local/lib/obs)     # macOS Homebrew (Intel)

            if(OBS_LIBRARY)
                add_definitions(-DHAVE_OBS_HEADERS=1)
                message(STATUS "Found OBS headers at ${OBS_INCLUDE_DIR}")
            else()
                message(WARNING "OBS library not found. Building in standalone mode.")
                add_definitions(-DSTANDALONE_TEST)
                add_definitions(-DBUILD_STANDALONE)
                set(OBS_INCLUDE_DIR "")
            endif()
        else()
            message(WARNING "OBS headers not found. Building in standalone mode.")
            add_definitions(-DSTANDALONE_TEST)
            add_definitions(-DBUILD_STANDALONE)
            set(OBS_INCLUDE_DIR "")
            set(OBS_FRONTEND_INCLUDE_DIR "")
        endif()
    endif()
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
        add_definitions(-DHAVE_OBS_HEADERS=1)
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
)

# --- Source Files ---
set(SOURCES
    src/stabilizer_opencv.cpp
    src/core/stabilizer_core.cpp
    src/core/stabilizer_wrapper.cpp
    src/core/preset_manager.cpp
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

    # Add OBS.framework rpath if using OBS.app
    if(OBS_LIBRARY AND EXISTS "/Applications/OBS.app/Contents/Frameworks")
        set(RPATH_DIRS "${RPATH_DIRS};/Applications/OBS.app/Contents/Frameworks")
        message(STATUS "Adding OBS.framework to rpath")
    endif()

    # Set link flags and rpath
    set_target_properties(obs-stabilizer-opencv PROPERTIES
        LINK_FLAGS "-undefined dynamic_lookup"
        INSTALL_RPATH "${RPATH_DIRS}"
        BUILD_WITH_INSTALL_RPATH TRUE
        MACOSX_RPATH TRUE
    )

    # Convert OpenCV library paths from absolute to @rpath references
    # This ensures the plugin can find OpenCV libraries at runtime when loaded by OBS
    # RATIONALE: OpenCV libraries are embedded with absolute paths during linking,
    # which prevents OBS from finding them at runtime. Converting to @rpath
    # allows the dynamic loader to use the RPATH_DIRS we configured above.
    #
    # REFACTORED: Use glob pattern to find all opencv libraries instead of hardcoding version numbers.
    # This makes the build system future-proof and compatible with different OpenCV versions.
    # Previously hardcoded version 4.12, now works with any OpenCV 4.x version.
    if(EXISTS "${OPENCV_LIB_REALDIR}")
        # Use glob to find all opencv libraries dynamically
        # RATIONALE: Eliminates version hardcoding, making build system maintainable across OpenCV versions
        file(GLOB OPENCV_LIBS_TO_FIX
            "${OPENCV_LIB_REALDIR}/libopencv_*.dylib"
        )

        # Verify we found some libraries
        list(LENGTH OPENCV_LIBS_TO_FIX NUM_LIBS)
        if(NUM_LIBS GREATER 0)
            message(STATUS "Found ${NUM_LIBS} OpenCV libraries to convert to @rpath")

            # Use install_name_tool to convert each library path
            foreach(opencv_lib ${OPENCV_LIBS_TO_FIX})
                get_filename_component(opencv_lib_name "${opencv_lib}" NAME)
                add_custom_command(TARGET obs-stabilizer-opencv POST_BUILD
                    COMMAND ${CMAKE_INSTALL_NAME_TOOL}
                        -change "${opencv_lib}"
                                "@rpath/${opencv_lib_name}"
                        $<TARGET_FILE:obs-stabilizer-opencv>
                    COMMENT "Converting ${opencv_lib_name} to use @rpath"
                )
            endforeach()
            message(STATUS "Configured post-build step to convert OpenCV libs to @rpath")
        else()
            message(WARNING "No OpenCV libraries found in ${OPENCV_LIB_REALDIR}")
        endif()
    endif()

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
    # macOS typically uses .so or .dylib for OBS plugins
    # Using .so to match OBS plugin conventions
    set_target_properties(obs-stabilizer-opencv PROPERTIES
        PREFIX ""
        OUTPUT_NAME "obs-stabilizer-opencv"
        SUFFIX ".so"
        CXX_VISIBILITY_PRESET hidden
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
    )

    if(TEST_SOURCES)
        message(STATUS "Found test sources, enabling test suite")

            # Add test source files and core source files for testing
            set(TEST_CORE_SOURCES
                ${CMAKE_CURRENT_SOURCE_DIR}/src/core/stabilizer_core.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/src/core/preset_manager.cpp
            )

        # Add frame_utils.cpp if OBS headers are available
        # RATIONALE: frame_utils.cpp contains implementations for
        # FRAME_UTILS::Conversion::obs_to_cv() and FRAME_UTILS::Validation::validate_cv_mat()
        # which are used by tests when HAVE_OBS_HEADERS is defined.
        # In standalone mode, these are provided as inline implementations in frame_utils.hpp.
        if(HAVE_OBS_HEADERS)
            list(APPEND TEST_CORE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/core/frame_utils.cpp)
        endif()

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

      if(HAVE_OBS_HEADERS)
          # Use real OBS headers but test with standalone PresetManager
          # RATIONALE: Tests need to use the standalone PresetManager implementation
          # because OBS stub functions (obs_data_create, etc.) return nullptr.
          # The STANDALONE_TEST macro ensures preset_manager.cpp uses the
          # nlohmann/json-based standalone implementation instead of OBS APIs.
          target_compile_definitions(stabilizer_tests PRIVATE STANDALONE_TEST)
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

