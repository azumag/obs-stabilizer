# Static OpenCV Configuration for OBS Stabilizer
# This file configures OpenCV for static linking

# Check if we want to use static OpenCV
if(NOT OPENCV_DEPLOYMENT_STRATEGY STREQUAL "Static" AND 
   NOT OPENCV_DEPLOYMENT_STRATEGY STREQUAL "Hybrid")
    return()
endif()

message(STATUS "Configuring static OpenCV linking")

# Find static OpenCV libraries
find_path(OPENCV_STATIC_ROOT
    NAMES lib/cmake/opencv4/OpenCVConfig.cmake
    PATHS 
        ${CMAKE_SOURCE_DIR}/opencv-static
        ${CMAKE_BINARY_DIR}/opencv-static
        /opt/opencv-static
        /usr/local/opencv-static
    DOC "Path to static OpenCV installation"
)

if(NOT OPENCV_STATIC_ROOT)
    message(STATUS "Static OpenCV not found, building from source")
    include(ExternalProject)
    
    set(OPENCV_VERSION "4.12.0")
    set(OPENCV_SOURCE_URL "https://github.com/opencv/opencv/archive/${OPENCV_VERSION}.tar.gz")
    
    ExternalProject_Add(opencv-static
        URL ${OPENCV_SOURCE_URL}
        DOWNLOAD_DIR ${CMAKE_BINARY_DIR}/downloads
        SOURCE_DIR ${CMAKE_BINARY_DIR}/opencv-src
        BINARY_DIR ${CMAKE_BINARY_DIR}/opencv-build
        INSTALL_DIR ${CMAKE_BINARY_DIR}/opencv-static
        
        CMAKE_ARGS
            -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/opencv-static
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DBUILD_SHARED_LIBS=OFF
            -DBUILD_STATIC_LIBS=ON
            -DCMAKE_POSITION_INDEPENDENT_CODE=ON
            
            # Disable unnecessary modules to reduce size
            -DBUILD_opencv_dnn=OFF
            -DBUILD_opencv_gapi=OFF
            -DBUILD_opencv_python2=OFF
            -DBUILD_opencv_python3=OFF
            -DBUILD_opencv_java=OFF
            
            # Disable tests and examples
            -DBUILD_EXAMPLES=OFF
            -DBUILD_TESTS=OFF
            -DBUILD_PERF_TESTS=OFF
            -DBUILD_DOCS=OFF
            
            # Disable heavy dependencies
            -DWITH_CUDA=OFF
            -DWITH_OPENCL=OFF
            -DWITH_TBB=OFF
            -DWITH_IPP=OFF
            -DWITH_EIGEN=OFF
            -DWITH_LAPACK=OFF
            
            # Enable required components
            -DBUILD_opencv_core=ON
            -DBUILD_opencv_imgproc=ON
            -DBUILD_opencv_video=ON
            -DBUILD_opencv_features2d=ON
            -DBUILD_opencv_calib3d=ON
            -DBUILD_opencv_flann=ON
            
            # Platform-specific optimizations
            -DWITH_PTHREADS_PF=ON
            -DWITH_V4L=ON
            -DWITH_FFMPEG=ON
            
        BUILD_COMMAND ${CMAKE_COMMAND} --build . --config ${CMAKE_BUILD_TYPE} --parallel
        
        INSTALL_COMMAND ${CMAKE_COMMAND} --install . --config ${CMAKE_BUILD_TYPE}
        
        # Build after external dependencies are resolved
        DEPENDS ""
    )
    
    set(OPENCV_STATIC_ROOT ${CMAKE_BINARY_DIR}/opencv-static)
    
    # Add dependency to main target
    add_dependencies(obs-stabilizer-opencv opencv-static)
endif()

# Find static OpenCV
set(OpenCV_DIR "${OPENCV_STATIC_ROOT}/lib/cmake/opencv4")
find_package(OpenCV REQUIRED COMPONENTS core imgproc video features2d calib3d flann)

# Filter static libraries
set(OPENCV_STATIC_LIBS "")
foreach(lib ${OpenCV_LIBS})
    if(${lib} MATCHES "\\.a$")
        list(APPEND OPENCV_STATIC_LIBS ${lib})
    endif()
endforeach()

message(STATUS "Found static OpenCV libraries: ${OPENCV_STATIC_LIBS}")

# Configure static linking
target_link_libraries(obs-stabilizer-opencv 
    ${OPENCV_STATIC_LIBS}
)

# Platform-specific static linking configuration
if(APPLE)
    # On macOS, ensure we link required system frameworks
    find_library(FOUNDATION_FRAMEWORK Foundation)
    find_library(COREVIDEO_FRAMEWORK CoreVideo)
    find_library(COREMEDIA_FRAMEWORK CoreMedia)
    find_library(QUARTZCORE_FRAMEWORK QuartzCore)
    find_library(AVFOUNDATION_FRAMEWORK AVFoundation)
    
    target_link_libraries(obs-stabilizer-opencv
        ${FOUNDATION_FRAMEWORK}
        ${COREVIDEO_FRAMEWORK}
        ${COREMEDIA_FRAMEWORK}
        ${QUARTZCORE_FRAMEWORK}
        ${AVFOUNDATION_FRAMEWORK}
    )
    
    # Remove RPATH for static linking
    set_target_properties(obs-stabilizer-opencv PROPERTIES
        INSTALL_RPATH ""
        BUILD_WITH_INSTALL_RPATH FALSE
    )
    
elseif(WIN32)
    # On Windows, add required Windows libraries
    target_link_libraries(obs-stabilizer-opencv
        kernel32
        user32
        gdi32
        winspool
        comdlg32
        advapi32
        shell32
        ole32
        oleaut32
        uuid
        odbc32
        odbccp32
    )
    
    # Set runtime library for static linking
    set_property(TARGET obs-stabilizer-opencv PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
    
elseif(UNIX AND NOT APPLE)
    # On Linux, add required system libraries
    target_link_libraries(obs-stabilizer-opencv
        pthread
        dl
        rt
        m
    )
endif()

# Add preprocessor definitions for static linking
target_compile_definitions(obs-stabilizer-opencv PRIVATE
    OPENCV_STATIC=1
    OPENCV_HEADER_ONLY=0
)

# Message for successful configuration
message(STATUS "Static OpenCV configuration completed successfully")
message(STATUS "Plugin will be approximately 15-20MB larger due to static linking")