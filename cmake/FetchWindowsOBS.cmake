# Build the official minimum-supported libobs SDK, following the OBS plugin
# template's source-build approach. This does not install or run OBS Studio.
cmake_minimum_required(VERSION 3.22)
include("${CMAKE_CURRENT_LIST_DIR}/PatchOBSWindowsSDK.cmake")
if(NOT WIN32)
    message(FATAL_ERROR "This SDK bootstrap requires Windows/MSVC")
endif()
get_filename_component(root "${CMAKE_CURRENT_LIST_DIR}/../tmp/obs-sdk" ABSOLUTE)
file(MAKE_DIRECTORY "${root}")
set(archive "${root}/30.0.0.tar.gz")
set(archive_hash "c23dd463862b1a8f40365d84fd52105d7eafc3614fb3d470b695ba28a6e4da06")
file(DOWNLOAD
    https://github.com/obsproject/obs-studio/archive/refs/tags/30.0.0.tar.gz
    "${archive}"
    EXPECTED_HASH "SHA256=${archive_hash}"
    TLS_VERIFY ON
)
set(source "${root}/obs-studio-30.0.0")
set(build "${root}/build")
set(patch_stamp "${root}/obs-studio-30.0.0-patch-fingerprint")
file(SHA256 "${CMAKE_CURRENT_LIST_DIR}/PatchOBSWindowsSDK.cmake" patch_sha256)
string(SHA256 source_fingerprint "${archive_hash};${patch_sha256}")
# Verification runs against the *patched* file before any re-patching, so a
# tampered or partially patched tree is caught by the recorded hash instead of
# being silently repaired by the idempotent patch step (#380 F-1).
invalidate_obs_windows_sdk_cache(
    "${source}" "${build}" "${patch_stamp}" "${source_fingerprint}"
    "${source}/libobs/cmake/os-windows.cmake" needs_extract)
if(needs_extract)
    file(ARCHIVE_EXTRACT INPUT "${archive}" DESTINATION "${root}")
endif()
patch_obs_windows_sdk("${source}")
file(SHA256 "${source}/libobs/cmake/os-windows.cmake" patched_cmake_sha256)
file(WRITE "${patch_stamp}" "${source_fingerprint}\n${patched_cmake_sha256}\n")
message(STATUS
    "OBS Windows SDK hashes: archive=${archive_hash}, patch=${patch_sha256}, patched os-windows.cmake=${patched_cmake_sha256}")
execute_process(
    COMMAND "${CMAKE_COMMAND}" -S "${source}" -B "${build}" -A x64
        -DOBS_CMAKE_VERSION=3.0.0 -DOBS_VERSION_OVERRIDE=30.0.0
        -DENABLE_UI=OFF -DENABLE_PLUGINS=OFF -DENABLE_BROWSER=OFF -DENABLE_SCRIPTING=OFF
    COMMAND_ERROR_IS_FATAL ANY
)
execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${build}" --target libobs
        --config RelWithDebInfo --parallel 4
    COMMAND_ERROR_IS_FATAL ANY
)
if(NOT EXISTS "${build}/libobs/RelWithDebInfo/obs.lib")
    message(FATAL_ERROR "Official libobs build did not produce the expected obs.lib")
endif()
