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
set(obs_source "${root}/obs-source")
file(MAKE_DIRECTORY "${obs_source}/libobs/cmake")
file(WRITE "${obs_source}/libobs/cmake/os-windows.cmake"
    "configure_file(cmake/windows/obs-module.rc.in libobs.rc)\n")
patch_obs_windows_sdk("${obs_source}")
patch_obs_windows_sdk("${obs_source}")
file(READ "${obs_source}/libobs/cmake/os-windows.cmake" obs_windows_cmake)
string(REGEX MATCHALL "OBS::w32-pthreads" target_matches "${obs_windows_cmake}")
list(LENGTH target_matches target_count)
if(NOT target_count EQUAL 1 OR NOT obs_windows_cmake MATCHES
    "add_subdirectory.*deps/w32-pthreads")
    message(FATAL_ERROR "OBS Windows SDK patch did not add exactly one w32-pthreads target")
endif()
file(REMOVE_RECURSE "${root}")
message(STATUS "OBS dependency regression passed: plugin fails without SDK; explicit standalone succeeds")
