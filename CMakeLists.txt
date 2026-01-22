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
find_path(OBS_INCLUDE_DIR obs.h
    PATHS /usr/local/include /usr/include ${CMAKE_SOURCE_DIR}/include
    PATH_SUFFIXES obs)
find_path(OBS_FRONTEND_INCLUDE_DIR obs-frontend-api.h
    PATHS /usr/local/include /usr/include ${CMAKE_SOURCE_DIR}/include)
find_library(OBS_LIBRARY obs PATHS /Applications/OBS.app/Contents/Frameworks)

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
    src/core/apple_accelerate.cpp
    src/core/neon_feature_detection.cpp
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
    target_include_directories(obs-stabilizer-opencv PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>
    )
endif()

if(APPLE)
    # Apple-specific optimizations
    # Note: Accelerate framework excluded due to CarbonCore naming conflicts with OpenCV
    # Focusing on NEON feature detection which is platform-specific and doesn't require Accelerate
    target_include_directories(obs-stabilizer-opencv PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>
    )
endif()

if(APPLE)
    set_target_properties(obs-stabilizer-opencv PROPERTIES
        LINK_FLAGS "-undefined dynamic_lookup"
        INSTALL_RPATH "@loader_path/../Frameworks"
        BUILD_WITH_INSTALL_RPATH TRUE
        MACOSX_RPATH TRUE
    )
endif()

set_target_properties(obs-stabilizer-opencv PROPERTIES
    PREFIX ""
    OUTPUT_NAME "obs-stabilizer-opencv"
    SUFFIX ".so"
)

# --- Installation ---
install(TARGETS obs-stabilizer-opencv
    LIBRARY DESTINATION obs-plugins
    RUNTIME DESTINATION obs-plugins
)

 # --- Tests ---
enable_testing()

# Google Test
find_package(GTest REQUIRED)

# Remove compiler ID files from test sources to avoid duplicate symbols
set(TEST_SOURCES ${TEST_SOURCES})
list(FILTER TEST_SOURCES EXCLUDE REGEX "CompilerId")

 # Test Sources
set(TEST_SOURCES
    tests/test_basic.cpp
    tests/test_stabilizer_core.cpp
    tests/test_adaptive_stabilizer.cpp
    tests/test_motion_classifier.cpp
    tests/test_data_generator.cpp
)

 if(TEST_SOURCES)
     message(STATUS "Found test sources, enabling test suite")
     
     # Add test source files and core source files for testing
     set(TEST_CORE_SOURCES
         src/core/stabilizer_core.cpp
         src/core/apple_accelerate.cpp
         src/core/neon_feature_detection.cpp
         src/core/motion_classifier.cpp
         src/core/adaptive_stabilizer.cpp
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
         target_link_libraries(stabilizer_tests GTest::gtest_main ${OpenCV_LIBS})
     endif()
     
     add_test(NAME stabilizer_unit_tests COMMAND stabilizer_tests)
     message(STATUS "Test suite enabled.")
 else()
     message(STATUS "No test sources found, skipping test suite.")
 endif()
