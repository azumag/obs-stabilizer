# Carry the upstream OBS 30.1 fix for building the 32-bit SDK slice while
# preserving the OBS 30.0 API level required by this plugin.
function(patch_obs_windows_sdk obs_source_dir)
    set(libobs_windows_cmake "${obs_source_dir}/libobs/cmake/os-windows.cmake")
    if(NOT EXISTS "${libobs_windows_cmake}")
        message(FATAL_ERROR "Official OBS Windows libobs CMake file not found: ${libobs_windows_cmake}")
    endif()

    file(READ "${libobs_windows_cmake}" contents)
    if(contents MATCHES "if\\(NOT TARGET OBS::w32-pthreads\\)")
        return()
    endif()

    set(anchor "configure_file(cmake/windows/obs-module.rc.in libobs.rc)")
    string(FIND "${contents}" "${anchor}" anchor_index)
    if(anchor_index EQUAL -1)
        message(FATAL_ERROR "Unexpected official OBS 30.0 Windows libobs CMake layout")
    endif()

    set(target_fix [=[if(NOT TARGET OBS::w32-pthreads)
  add_subdirectory("${CMAKE_SOURCE_DIR}/deps/w32-pthreads" "${CMAKE_BINARY_DIR}/deps/w32-pthreads")
endif()

]=])
    string(REPLACE "${anchor}" "${target_fix}${anchor}" patched_contents "${contents}")
    file(WRITE "${libobs_windows_cmake}" "${patched_contents}")
    message(STATUS "Applied upstream OBS 30.1 Windows w32-pthreads target fix to the local OBS 30.0 SDK source")
endfunction()
