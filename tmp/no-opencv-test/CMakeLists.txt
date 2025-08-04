cmake_minimum_required(VERSION 3.16)
project(test-opencv-detection)

set(BUILD_STANDALONE ON)

# Test the OpenCV detection logic
if(BUILD_STANDALONE)
    find_package(OpenCV QUIET COMPONENTS core imgproc video features2d)
    if(OpenCV_FOUND)
        message(STATUS "OpenCV found for standalone build - full functionality enabled")
        message(STATUS "OpenCV version: ${OpenCV_VERSION}")
        message(STATUS "OpenCV include dirs: ${OpenCV_INCLUDE_DIRS}")
    else()
        message(STATUS "OpenCV not found for standalone build - basic validation only")
    endif()
else()
    find_package(OpenCV REQUIRED COMPONENTS core imgproc video features2d)
endif()

# Test executable
add_executable(test-app ../../src/standalone_test.cpp)

# Test linking
if(BUILD_STANDALONE)
    if(OpenCV_FOUND)
        target_link_libraries(test-app ${OpenCV_LIBS})
        message(STATUS "Standalone build linked with OpenCV for full testing")
    else()
        message(STATUS "Standalone build without OpenCV - basic validation only")
    endif()
endif()