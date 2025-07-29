# macOS Plugin Loading Fix for OBS Stabilizer
# This module fixes the install name and dependencies for proper plugin loading

if(NOT APPLE)
    return()
endif()

# Function to fix macOS plugin binary post-build
function(fix_macos_plugin_binary TARGET)
    # Get output name for the target
    get_target_property(OUTPUT_NAME ${TARGET} OUTPUT_NAME)
    if(NOT OUTPUT_NAME)
        set(OUTPUT_NAME ${TARGET})
    endif()
    
    # Set proper install name without self-reference (removes circular dependency)
    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND ${CMAKE_INSTALL_NAME_TOOL} -id "${OUTPUT_NAME}" 
            "$<TARGET_FILE:${TARGET}>"
        COMMENT "Setting correct install name for macOS plugin"
    )
    
    # Add rpath for OBS framework (plugin needs to find libobs)
    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND ${CMAKE_INSTALL_NAME_TOOL} -add_rpath "@loader_path/../../../../Frameworks" "$<TARGET_FILE:${TARGET}>" || true
        COMMENT "Adding rpath for OBS framework"
    )
    
    # Fix OpenCV dependencies if they exist
    if(OpenCV_FOUND)
        # Add OpenCV rpath entries for different package managers (ignore errors as paths may already exist)
        set(OPENCV_RPATHS 
            "/opt/homebrew/opt/opencv/lib"
            "/usr/local/lib" 
            "/opt/local/lib"
        )
        foreach(RPATH_DIR ${OPENCV_RPATHS})
            add_custom_command(TARGET ${TARGET} POST_BUILD
                COMMAND ${CMAKE_INSTALL_NAME_TOOL} -add_rpath "${RPATH_DIR}" "$<TARGET_FILE:${TARGET}>" || true
                COMMENT "Adding OpenCV rpath: ${RPATH_DIR}"
            )
        endforeach()
    endif()
    
    # Sign the plugin binary (ad-hoc signing)
    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND codesign --force --sign - "$<TARGET_FILE:${TARGET}>"
        COMMENT "Code signing macOS plugin binary"
    )
endfunction()

# Apply the fix to the current target if it's a macOS plugin
if(TARGET ${CMAKE_PROJECT_NAME} AND APPLE AND OBS_INCLUDE_DIR)
    fix_macos_plugin_binary(${CMAKE_PROJECT_NAME})
    message(STATUS "macOS plugin loading fix applied to ${CMAKE_PROJECT_NAME}")
endif()
