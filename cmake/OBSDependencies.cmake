# --- OBS Dependencies ---
option(BUILD_STANDALONE "Build test/development code without the OBS module ABI" OFF)

# Allow user to specify custom OBS paths
set(OBS_INCLUDE_PATH "" CACHE PATH "Path to OBS include directory")
set(OBS_LIBRARY_PATH "" CACHE PATH "Path to OBS library directory")
set(OBS_SOURCE_DIR "" CACHE PATH "Path to an OBS Studio source tree")
set(HAVE_OBS_HEADERS FALSE)

# On macOS, always compile the distributable plugin against the official OBS
# headers. CI runners do not have OBS.app installed, so falling back to the
# standalone stubs here would produce a bundle with no usable OBS module.
if(BUILD_STANDALONE)
    add_compile_definitions(STANDALONE_TEST BUILD_STANDALONE)
    set(OBS_INCLUDE_DIR "")
    set(OBS_LIBRARY "")
    message(STATUS "Explicit standalone build (not an installable OBS plugin)")
elseif(APPLE OR (WIN32 AND OBS_SOURCE_DIR))
    if(NOT OBS_SOURCE_DIR)
        include(FetchContent)
        set(OBS_SOURCE_URL https://github.com/obsproject/obs-studio/archive/refs/tags/30.0.0.tar.gz)
        set(OBS_SOURCE_HASH SHA256=c23dd463862b1a8f40365d84fd52105d7eafc3614fb3d470b695ba28a6e4da06)

        # Build against the documented minimum so the module ABI remains
        # loadable by OBS 30.x as well as newer OBS releases.
        if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.18)
            FetchContent_Declare(
                obs_source_headers
                URL ${OBS_SOURCE_URL}
                URL_HASH ${OBS_SOURCE_HASH}
                DOWNLOAD_EXTRACT_TIMESTAMP TRUE
                SOURCE_SUBDIR _headers_only
            )
            FetchContent_MakeAvailable(obs_source_headers)
        else()
            FetchContent_Declare(
                obs_source_headers
                URL ${OBS_SOURCE_URL}
                URL_HASH ${OBS_SOURCE_HASH}
                DOWNLOAD_EXTRACT_TIMESTAMP TRUE
            )
            FetchContent_GetProperties(obs_source_headers)
            if(NOT obs_source_headers_POPULATED)
                FetchContent_Populate(obs_source_headers)
            endif()
        endif()
        set(OBS_SOURCE_DIR "${obs_source_headers_SOURCE_DIR}")
    endif()

    if(NOT EXISTS "${OBS_SOURCE_DIR}/libobs/obs-module.h")
        message(FATAL_ERROR "OBS source headers not found under ${OBS_SOURCE_DIR}/libobs")
    endif()

    set(OBS_INCLUDE_DIR "${OBS_SOURCE_DIR}/libobs")
    set(OBS_FRONTEND_INCLUDE_DIR "")
    set(OBS_RELEASE_CANDIDATE 0)
    set(OBS_BETA 0)
    set(OBS_GENERATED_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/obs-generated")
    file(MAKE_DIRECTORY "${OBS_GENERATED_INCLUDE_DIR}")
    configure_file(
        "${OBS_INCLUDE_DIR}/obsconfig.h.in"
        "${OBS_GENERATED_INCLUDE_DIR}/obsconfig.h"
        @ONLY
    )
    set(HAVE_OBS_HEADERS TRUE)
    message(STATUS "OBS headers: ${OBS_INCLUDE_DIR}")
    if(APPLE)
        set(OBS_LIBRARY "")
        message(STATUS "OBS symbols will resolve when the plugin is loaded")
    else()
        # Windows needs the import library built from this official source tree.
        find_library(OBS_LIBRARY obs PATHS "${OBS_LIBRARY_PATH}" NO_DEFAULT_PATH)
        if(NOT OBS_LIBRARY)
            message(FATAL_ERROR "OBS import library required at OBS_LIBRARY_PATH=${OBS_LIBRARY_PATH}")
        endif()
        set(OBS_FRONTEND_INCLUDE_DIR "${OBS_SOURCE_DIR}/deps/w32-pthreads")
        message(STATUS "OBS import library: ${OBS_LIBRARY}")
    endif()
else()
    # Windows/Linux OBS header search
    find_path(OBS_INCLUDE_DIR obs.h
        PATHS
            ${OBS_INCLUDE_PATH}
            /usr/include           # Linux system
            ${CMAKE_SOURCE_DIR}/include
        PATH_SUFFIXES obs
        NO_DEFAULT_PATH)

    find_library(OBS_LIBRARY obs
        PATHS
            ${OBS_LIBRARY_PATH}
            /usr/lib/obs           # Linux OBS packages
            /usr/local/lib/obs
            /usr/lib /usr/local/lib
            /usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}
        NO_DEFAULT_PATH)

    if(OBS_INCLUDE_DIR AND OBS_LIBRARY)
        set(HAVE_OBS_HEADERS TRUE)
        message(STATUS "Found OBS headers at ${OBS_INCLUDE_DIR}")
        message(STATUS "Found OBS library at ${OBS_LIBRARY}")
    else()
        message(FATAL_ERROR
            "OBS SDK required for plugin builds (headers and libobs/import library). "
            "Set OBS_INCLUDE_PATH and OBS_LIBRARY_PATH, or explicitly use "
            "-DBUILD_STANDALONE=ON for non-distributable test/development builds.")
    endif()
endif()
