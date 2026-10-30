# Carry the upstream OBS 30.1 fix for building the 32-bit SDK slice while
# preserving the OBS 30.0 API level required by this plugin.
# Reuse requires the whole stamp contract to hold: the same
# archive+patch fingerprint AND the patched file still byte-for-byte equal to
# what the recorded patch produced. A matching fingerprint alone must never
# trust the tree: a partially patched or hand-edited patched file is exactly
# the silent damage this cache is meant to catch (#380 F-1). The stamp keeps
# two lines: `<fingerprint>\n<patched file sha256>`; an older single-line stamp
# is treated as invalid so the cache self-heals on the next run.
function(invalidate_obs_windows_sdk_cache obs_source_dir obs_build_dir stamp_path fingerprint patched_file out_needs_extract)
    set(needs_extract FALSE)
    if(NOT EXISTS "${obs_source_dir}/CMakeLists.txt" OR NOT EXISTS "${stamp_path}")
        set(needs_extract TRUE)
    else()
        file(STRINGS "${stamp_path}" stamp_lines LIMIT_COUNT 2)
        list(LENGTH stamp_lines stamp_line_count)
        if(stamp_line_count LESS 2)
            set(needs_extract TRUE)
        else()
            list(GET stamp_lines 0 cached_fingerprint)
            list(GET stamp_lines 1 cached_patched_sha256)
            string(STRIP "${cached_fingerprint}" cached_fingerprint)
            string(STRIP "${cached_patched_sha256}" cached_patched_sha256)
            if(NOT "${cached_fingerprint}" STREQUAL "${fingerprint}")
                set(needs_extract TRUE)
            elseif(NOT EXISTS "${patched_file}")
                set(needs_extract TRUE)
            else()
                file(SHA256 "${patched_file}" patched_sha256)
                if(NOT "${patched_sha256}" STREQUAL "${cached_patched_sha256}")
                    set(needs_extract TRUE)
                endif()
            endif()
        endif()
    endif()

    if(needs_extract)
        file(REMOVE_RECURSE "${obs_source_dir}" "${obs_build_dir}" "${stamp_path}")
    endif()
    set(${out_needs_extract} "${needs_extract}" PARENT_SCOPE)
endfunction()

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
