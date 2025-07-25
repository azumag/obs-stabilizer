# Standalone performance test CMakeLists.txt
# Use: cmake -S . -B build-perftest -f CMakeLists-perftest.txt

cmake_minimum_required(VERSION 3.16)
project(obs-stabilizer-perftest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenCV
find_package(OpenCV REQUIRED COMPONENTS core imgproc features2d video)

# Create performance test executable
add_executable(perftest performance-test.cpp)

# Create memory test executable
add_executable(memtest memory-test.cpp)

# Link OpenCV libraries
target_link_libraries(perftest ${OpenCV_LIBS})
target_link_libraries(memtest ${OpenCV_LIBS})

# Include OpenCV headers
target_include_directories(perftest PRIVATE ${OpenCV_INCLUDE_DIRS})
target_include_directories(memtest PRIVATE ${OpenCV_INCLUDE_DIRS})

# Compiler-specific optimizations
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(perftest PRIVATE -O3 -march=native)
    target_compile_options(memtest PRIVATE -O3 -march=native)
endif()

# Print OpenCV info
message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")
message(STATUS "OpenCV include dirs: ${OpenCV_INCLUDE_DIRS}")