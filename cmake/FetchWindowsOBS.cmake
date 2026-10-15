# Build the official minimum-supported libobs SDK, following the OBS plugin
# template's source-build approach. This does not install or run OBS Studio.
cmake_minimum_required(VERSION 3.22)
if(NOT WIN32)
    message(FATAL_ERROR "This SDK bootstrap requires Windows/MSVC")
endif()
get_filename_component(root "${CMAKE_CURRENT_LIST_DIR}/../tmp/obs-sdk" ABSOLUTE)
file(MAKE_DIRECTORY "${root}")
set(archive "${root}/30.0.0.tar.gz")
file(DOWNLOAD
    https://github.com/obsproject/obs-studio/archive/refs/tags/30.0.0.tar.gz
    "${archive}"
    EXPECTED_HASH SHA256=c23dd463862b1a8f40365d84fd52105d7eafc3614fb3d470b695ba28a6e4da06
    TLS_VERIFY ON
)
if(NOT EXISTS "${root}/obs-studio-30.0.0/CMakeLists.txt")
    file(ARCHIVE_EXTRACT INPUT "${archive}" DESTINATION "${root}")
endif()
set(source "${root}/obs-studio-30.0.0")
execute_process(
    COMMAND "${CMAKE_COMMAND}" -S "${source}" -B "${root}/build" -A x64
        -DOBS_CMAKE_VERSION=3.0.0 -DOBS_VERSION_OVERRIDE=30.0.0
        -DENABLE_UI=OFF -DENABLE_PLUGINS=OFF -DENABLE_BROWSER=OFF -DENABLE_SCRIPTING=OFF
    COMMAND_ERROR_IS_FATAL ANY
)
execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${root}/build" --target libobs
        --config RelWithDebInfo --parallel 4
    COMMAND_ERROR_IS_FATAL ANY
)
if(NOT EXISTS "${root}/build/libobs/RelWithDebInfo/obs.lib")
    message(FATAL_ERROR "Official libobs build did not produce the expected obs.lib")
endif()
