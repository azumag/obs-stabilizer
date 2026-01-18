# Bundled OpenCV Configuration for OBS Stabilizer
# This file configures OpenCV for bundled distribution

# Check if we want to use bundled OpenCV
if(NOT OPENCV_DEPLOYMENT_STRATEGY STREQUAL "Bundled")
    return()
endif()

message(STATUS "Configuring bundled OpenCV distribution")

# Find system OpenCV for bundling
find_package(OpenCV REQUIRED COMPONENTS core imgproc video features2d calib3d flann)

# Filter dynamic libraries
set(OPENCV_BUNDLED_LIBS "")
foreach(lib ${OpenCV_LIBS})
    if(${lib} MATCHES "\\.dylib$" OR ${lib} MATCHES "\\.so$" OR ${lib} MATCHES "\\.dll$")
        list(APPEND OPENCV_BUNDLED_LIBS ${lib})
    endif()
endforeach()

message(STATUS "Found OpenCV libraries for bundling: ${OPENCV_BUNDLED_LIBS}")

# Create bundled libraries directory
set(OPENCV_BUNDLED_DIR "${CMAKE_BINARY_DIR}/opencv-libraries")
file(MAKE_DIRECTORY ${OPENCV_BUNDLED_DIR})

# Copy OpenCV libraries to bundled directory
foreach(lib ${OPENCV_BUNDLED_LIBS})
    get_filename_component(lib_name ${lib} NAME)
    configure_file(
        ${lib}
        ${OPENCV_BUNDLED_DIR}/${lib_name}
        COPYONLY
    )
    message(STATUS "Bundled ${lib_name}")
endforeach()

# Install bundled libraries with the plugin
install(DIRECTORY ${OPENCV_BUNDLED_DIR}/
    DESTINATION obs-plugins
    COMPONENT Runtime
    FILES_MATCHING
    PATTERN "*.dylib"
    PATTERN "*.so*"
    PATTERN "*.dll"
)

# Configure runtime loading paths
if(APPLE)
    # On macOS, set @loader_path for relative loading
    set_target_properties(obs-stabilizer-opencv PROPERTIES
        INSTALL_RPATH "@loader_path/opencv-libraries"
        BUILD_WITH_INSTALL_RPATH TRUE
        MACOSX_RPATH TRUE
    )
    
    # Update library IDs for relative paths
    foreach(lib ${OPENCV_BUNDLED_LIBS})
        get_filename_component(lib_name ${lib} NAME)
        add_custom_command(
            TARGET obs-stabilizer-opencv POST_BUILD
            COMMAND install_name_tool -change ${lib} "@loader_path/opencv-libraries/${lib_name}" $<TARGET_FILE:obs-stabilizer-opencv>
            COMMENT "Updating library path for ${lib_name}"
        )
    endforeach()
    
elseif(WIN32)
    # On Windows, no RPATH needed, DLLs are found in the same directory
    # Copy DLLs to the same directory as the plugin
    foreach(lib ${OPENCV_BUNDLED_LIBS})
        get_filename_component(lib_name ${lib} NAME)
        add_custom_command(
            TARGET obs-stabilizer-opencv POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${lib} $<TARGET_FILE_DIR:obs-stabilizer-opencv>/
            COMMENT "Copying ${lib_name} to plugin directory"
        )
    endforeach()
    
elseif(UNIX)
    # On Linux, set $ORIGIN for relative loading
    set_target_properties(obs-stabilizer-opencv PROPERTIES
        INSTALL_RPATH "$ORIGIN/opencv-libraries"
        BUILD_WITH_INSTALL_RPATH TRUE
        LINK_FLAGS "-Wl,--enable-new-dtags"
    )
endif()

# Add bundled loading code
if(APPLE)
    target_compile_definitions(obs-stabilizer-opencv PRIVATE
        OPENCV_BUNDLED=1
        OPENCV_BUNDLED_PATH="@loader_path/opencv-libraries"
    )
elseif(WIN32)
    target_compile_definitions(obs-stabilizer-opencv PRIVATE
        OPENCV_BUNDLED=1
        OPENCV_BUNDLED_PATH="./opencv-libraries"
    )
elseif(UNIX)
    target_compile_definitions(obs-stabilizer-opencv PRIVATE
        OPENCV_BUNDLED=1
        OPENCV_BUNDLED_PATH="$ORIGIN/opencv-libraries"
    )
endif()

# Create verification script for bundled libraries
configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/verify-bundled-libs.sh.in
    ${CMAKE_BINARY_DIR}/verify-bundled-libs.sh
    @ONLY
)

# Add verification as build step
add_custom_target(verify_bundled
    COMMAND ${CMAKE_BINARY_DIR}/verify-bundled-libs.sh
    COMMENT "Verifying bundled OpenCV libraries"
)

add_dependencies(obs-stabilizer-opencv verify_bundled)

# Message for successful configuration
message(STATUS "Bundled OpenCV configuration completed successfully")
message(STATUS "Plugin will bundle ${CMAKE_LIST_LENGTH OPENCV_BUNDLED_LIBS} OpenCV libraries")
message(STATUS "Total bundled size will be approximately 8MB")