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
    
    # Set correct install name for plugin bundle
    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND ${CMAKE_INSTALL_NAME_TOOL} -id "@loader_path/../MacOS/${OUTPUT_NAME}" 
            "$<TARGET_FILE:${TARGET}>"
        COMMENT "Fixing install name for macOS plugin bundle"
    )
    
    # Fix OpenCV dependencies if they exist
    if(OpenCV_FOUND)
        # Add OpenCV rpath entries for different package managers (ignore errors as paths may already exist)
        add_custom_command(TARGET ${TARGET} POST_BUILD
            COMMAND bash -c "${CMAKE_INSTALL_NAME_TOOL} -add_rpath /opt/homebrew/opt/opencv/lib $<TARGET_FILE:${TARGET}> 2>/dev/null || true"
            COMMENT "Adding Homebrew OpenCV rpath for macOS plugin"
        )
        add_custom_command(TARGET ${TARGET} POST_BUILD
            COMMAND bash -c "${CMAKE_INSTALL_NAME_TOOL} -add_rpath /usr/local/lib $<TARGET_FILE:${TARGET}> 2>/dev/null || true"
            COMMENT "Adding /usr/local/lib rpath for macOS plugin"
        )
        add_custom_command(TARGET ${TARGET} POST_BUILD
            COMMAND bash -c "${CMAKE_INSTALL_NAME_TOOL} -add_rpath /opt/local/lib $<TARGET_FILE:${TARGET}> 2>/dev/null || true"
            COMMENT "Adding MacPorts rpath for macOS plugin"
        )
    endif()
    
    # Sign the plugin binary (ad-hoc signing)
    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND codesign --force --sign - "$<TARGET_FILE:${TARGET}>"
        COMMENT "Code signing macOS plugin binary"
    )
    
    # Verify the fixes
    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND otool -L "$<TARGET_FILE:${TARGET}>" | head -5
        COMMENT "Verifying plugin binary dependencies"
    )
endfunction()

# Apply the fix to the current target if it's a macOS plugin
if(TARGET ${CMAKE_PROJECT_NAME} AND APPLE AND OBS_INCLUDE_DIR)
    fix_macos_plugin_binary(${CMAKE_PROJECT_NAME})
    message(STATUS "macOS plugin loading fix applied to ${CMAKE_PROJECT_NAME}")
endif()