cmake_minimum_required(VERSION 3.28...3.30)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version})

option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" ON)
option(ENABLE_QT "Use Qt functionality" ON)

include(compilerconfig)
include(defaults)
include(helpers)

add_library(${CMAKE_PROJECT_NAME} MODULE)

find_package(libobs REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs)

if(ENABLE_FRONTEND_API)
  find_package(obs-frontend-api REQUIRED)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::obs-frontend-api)
endif()

if(ENABLE_QT)
  find_package(Qt6 COMPONENTS Widgets Core)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets)
  target_compile_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header -Wno-comma>
  )
  set_target_properties(
    ${CMAKE_PROJECT_NAME}
    PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON
  )
endif()

# OpenCV dependency for video stabilization with comprehensive version checking
find_package(OpenCV 4.5 QUIET COMPONENTS core imgproc video features2d)
if(NOT OpenCV_FOUND)
    # Try alternative package names
    find_package(OpenCV QUIET COMPONENTS core imgproc video features2d)
    if(NOT OpenCV_FOUND)
        message(WARNING "OpenCV not found. Building without stabilization features.")
        message(STATUS "Install OpenCV 4.5+ with: brew install opencv (macOS) or apt-get install libopencv-dev (Ubuntu)")
        set(ENABLE_STABILIZATION OFF)
    endif()
endif()

if(OpenCV_FOUND)
    # Comprehensive version compatibility validation
    message(STATUS "Found OpenCV ${OpenCV_VERSION}")
    
    # Version compatibility matrix
    if(OpenCV_VERSION VERSION_LESS "4.5.0")
        message(FATAL_ERROR "OpenCV ${OpenCV_VERSION} is too old. Minimum required: 4.5.0")
    elseif(OpenCV_VERSION VERSION_GREATER_EQUAL "4.5.0" AND OpenCV_VERSION VERSION_LESS "5.0.0")
        message(STATUS "✅ OpenCV ${OpenCV_VERSION} - Fully tested and supported")
        set(OPENCV_COMPATIBILITY_LEVEL "FULLY_SUPPORTED")
    elseif(OpenCV_VERSION VERSION_GREATER_EQUAL "5.0.0" AND OpenCV_VERSION VERSION_LESS "6.0.0")
        message(WARNING "⚠️  OpenCV ${OpenCV_VERSION} - Limited testing. May have API changes")
        message(STATUS "Consider using OpenCV 4.8.x for maximum stability")
        set(OPENCV_COMPATIBILITY_LEVEL "EXPERIMENTAL")
    else()
        message(WARNING "❌ OpenCV ${OpenCV_VERSION} - Untested version. Use at your own risk")
        set(OPENCV_COMPATIBILITY_LEVEL "UNSUPPORTED")
        message(STATUS "Recommended versions: 4.5.x - 4.8.x")
    endif()
    
    # Validate required components
    set(REQUIRED_OPENCV_COMPONENTS core imgproc video features2d)
    foreach(component ${REQUIRED_OPENCV_COMPONENTS})
        if(NOT ${component} IN_LIST OpenCV_LIBS)
            message(FATAL_ERROR "Required OpenCV component '${component}' not found")
        endif()
    endforeach()
    
    # API compatibility compile-time checks
    set(ENABLE_STABILIZATION ON)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${OpenCV_LIBS})
    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${OpenCV_INCLUDE_DIRS})
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE 
        ENABLE_STABILIZATION
        OPENCV_VERSION_MAJOR=${OpenCV_VERSION_MAJOR}
        OPENCV_VERSION_MINOR=${OpenCV_VERSION_MINOR}
        OPENCV_COMPATIBILITY_LEVEL="${OPENCV_COMPATIBILITY_LEVEL}"
    )
    
    # Log detailed version information for debugging
    message(STATUS "OpenCV Configuration:")
    message(STATUS "  - Version: ${OpenCV_VERSION}")
    message(STATUS "  - Components: ${OpenCV_LIBS}")
    message(STATUS "  - Include dirs: ${OpenCV_INCLUDE_DIRS}")
    message(STATUS "  - Compatibility: ${OPENCV_COMPATIBILITY_LEVEL}")
endif()

# Configure plugin support file
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plugin-support.c.in
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plugin-support.c
    @ONLY
)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE 
  src/plugin-main.cpp
  src/plugin-support.c
  src/core/stabilizer_core.cpp
  src/obs/obs_integration.cpp
)

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})

# Ensure C++17 standard for future C++ code
set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY CXX_STANDARD 17)
set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)
