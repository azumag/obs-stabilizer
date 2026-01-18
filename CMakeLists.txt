cmake_minimum_required(VERSION 3.16)
project(obs-stabilizer-opencv)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES "arm64")

# --- OpenCV Dependencies ---
find_package(OpenCV REQUIRED)
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
    src/obs/obs_integration.cpp
    src/plugin-support.c
    src/obs_stubs.c
)

# --- Main Plugin Target ---
add_library(obs-stabilizer-opencv MODULE ${SOURCES})

target_link_libraries(obs-stabilizer-opencv ${OpenCV_LIBS})
if(OBS_LIBRARY)
    target_link_libraries(obs-stabilizer-opencv ${OBS_LIBRARY})
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

# Test Sources
file(GLOB_RECURSE TEST_SOURCES "src/tests/*.cpp" "src/tests/*.c")

if(TEST_SOURCES)
    add_executable(stabilizer_tests ${TEST_SOURCES})
    target_link_libraries(stabilizer_tests GTest::gtest GTest::gtest_main ${OpenCV_LIBS})
    target_include_directories(stabilizer_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${OBS_INCLUDE_DIR}
    )
    add_test(NAME stabilizer_unit_tests COMMAND stabilizer_tests)
    message(STATUS "Test suite enabled.")
else()
    message(STATUS "No test sources found, skipping test suite.")
endif()
