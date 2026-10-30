cmake_minimum_required(VERSION 3.16)
get_filename_component(repo "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(root "${repo}/tmp/obs-dependency-test")
file(REMOVE_RECURSE "${root}")
file(MAKE_DIRECTORY "${root}/source" "${root}/empty")
# Exercise the production discovery module with an empty sysroot, independent
# of whether the runner already has libobs installed. No OpenCV/toolchain needed.
file(WRITE "${root}/source/CMakeLists.txt" "cmake_minimum_required(VERSION 3.16)\nproject(obs_contract NONE)\nset(APPLE FALSE)\nset(WIN32 FALSE)\ninclude(\"${repo}/cmake/OBSDependencies.cmake\")\n")
foreach(mode IN ITEMS plugin standalone)
    set(standalone OFF)
    if(mode STREQUAL "standalone")
        set(standalone ON)
    endif()
    execute_process(COMMAND "${CMAKE_COMMAND}" -S "${root}/source" -B "${root}/${mode}"
        "-DCMAKE_FIND_ROOT_PATH=${root}/empty"
        -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY
        "-DBUILD_STANDALONE=${standalone}"
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(mode STREQUAL "plugin")
        if(result EQUAL 0 OR NOT "${output}${error}" MATCHES "OBS SDK required for plugin builds")
            message(FATAL_ERROR "Missing SDK did not fail closed: ${output}${error}")
        endif()
    elseif(NOT result EQUAL 0)
        message(FATAL_ERROR "Explicit standalone failed: ${output}${error}")
    endif()
endforeach()
# Exercise the pinned OBS 30.0 source correction used by the Windows SDK
# bootstrap, using the exact source anchor and upstream fix without downloads.
include("${repo}/cmake/PatchOBSWindowsSDK.cmake")
set(cache_root "${root}/obs-sdk-cache")
set(cache_source "${cache_root}/obs-studio")
set(cache_build "${cache_root}/build")
set(cache_stamp "${cache_root}/fingerprint")
set(cache_patched "${cache_source}/libobs/cmake/os-windows.cmake")
# Helper for the case matrix: rebuild a fresh cached tree with a patched file
# and a two-line stamp (fingerprint + patched sha256), mirroring what
# FetchWindowsOBS.cmake writes after a successful patch (#380 F-1).
function(write_cache fingerprint patched_content)
    file(REMOVE_RECURSE "${cache_root}")
    file(MAKE_DIRECTORY "${cache_source}/libobs/cmake" "${cache_build}")
    file(WRITE "${cache_source}/CMakeLists.txt" "source")
    file(WRITE "${cache_patched}" "${patched_content}")
    file(WRITE "${cache_build}/state.txt" "build")
    if(NOT "${fingerprint}" STREQUAL "")
        file(SHA256 "${cache_patched}" patched_sha)
        file(WRITE "${cache_stamp}" "${fingerprint}\n${patched_sha}\n")
    endif()
endfunction()
function(expect_cache_removed label)
    if(NOT needs_extract OR EXISTS "${cache_source}/CMakeLists.txt" OR EXISTS "${cache_build}")
        message(FATAL_ERROR "OBS SDK cache was not invalidated: ${label}")
    endif()
endfunction()

# Stale fingerprint -> source, build and stamp all removed.
write_cache("old-fingerprint" "patched")
invalidate_obs_windows_sdk_cache(
    "${cache_source}" "${cache_build}" "${cache_stamp}" "current-fingerprint"
    "${cache_patched}" needs_extract)
expect_cache_removed("stale fingerprint")

# Same fingerprint + intact patched file -> everything kept.
write_cache("current-fingerprint" "patched")
invalidate_obs_windows_sdk_cache(
    "${cache_source}" "${cache_build}" "${cache_stamp}" "current-fingerprint"
    "${cache_patched}" needs_extract)
if(needs_extract OR NOT EXISTS "${cache_source}/CMakeLists.txt" OR NOT EXISTS "${cache_build}/state.txt")
    message(FATAL_ERROR "A current OBS SDK patch fingerprint did not preserve its cache")
endif()

# Same fingerprint + corrupted/hand-edited patched file -> fail closed.
write_cache("current-fingerprint" "patched")
file(WRITE "${cache_patched}" "tampered patched file")
invalidate_obs_windows_sdk_cache(
    "${cache_source}" "${cache_build}" "${cache_stamp}" "current-fingerprint"
    "${cache_patched}" needs_extract)
expect_cache_removed("patched file hash mismatch")

# Missing stamp -> fail closed.
write_cache("" "patched")
invalidate_obs_windows_sdk_cache(
    "${cache_source}" "${cache_build}" "${cache_stamp}" "current-fingerprint"
    "${cache_patched}" needs_extract)
expect_cache_removed("missing stamp")

# Missing source root marker (CMakeLists.txt) -> fail closed.
write_cache("current-fingerprint" "patched")
file(REMOVE "${cache_source}/CMakeLists.txt")
invalidate_obs_windows_sdk_cache(
    "${cache_source}" "${cache_build}" "${cache_stamp}" "current-fingerprint"
    "${cache_patched}" needs_extract)
expect_cache_removed("missing source CMakeLists.txt")

# Legacy single-line stamp (pre-F-1 format) -> self-heals by re-extracting.
write_cache("current-fingerprint" "patched")
file(WRITE "${cache_stamp}" "current-fingerprint\n")
invalidate_obs_windows_sdk_cache(
    "${cache_source}" "${cache_build}" "${cache_stamp}" "current-fingerprint"
    "${cache_patched}" needs_extract)
expect_cache_removed("legacy single-line stamp")
set(obs_source "${root}/obs-source")
file(MAKE_DIRECTORY "${obs_source}/libobs/cmake")
file(WRITE "${obs_source}/libobs/cmake/os-windows.cmake"
    "configure_file(cmake/windows/obs-module.rc.in libobs.rc)\n")
patch_obs_windows_sdk("${obs_source}")
patch_obs_windows_sdk("${obs_source}")
file(READ "${obs_source}/libobs/cmake/os-windows.cmake" obs_windows_cmake)
set(expected_obs_windows_cmake [=[if(NOT TARGET OBS::w32-pthreads)
  add_subdirectory("${CMAKE_SOURCE_DIR}/deps/w32-pthreads" "${CMAKE_BINARY_DIR}/deps/w32-pthreads")
endif()

configure_file(cmake/windows/obs-module.rc.in libobs.rc)
]=])
if(NOT "${obs_windows_cmake}" STREQUAL "${expected_obs_windows_cmake}")
    message(FATAL_ERROR "OBS Windows SDK correction was not inserted exactly once or changed from the upstream OBS 30.1 hunk:\n${obs_windows_cmake}")
endif()
file(REMOVE_RECURSE "${root}")
message(STATUS "OBS dependency regression passed: plugin fails without SDK; explicit standalone succeeds")
