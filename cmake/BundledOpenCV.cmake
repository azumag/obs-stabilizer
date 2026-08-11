cmake_minimum_required(VERSION 3.16)

if(NOT APPLE)
    message(FATAL_ERROR "BundledOpenCV.cmake can only package a macOS plugin")
endif()

foreach(required_variable BUNDLE_PATH BINARY_PATH)
    if(NOT DEFINED ${required_variable} OR "${${required_variable}}" STREQUAL "")
        message(FATAL_ERROR "${required_variable} is required")
    endif()
endforeach()

get_filename_component(BUNDLE_PATH "${BUNDLE_PATH}" ABSOLUTE)
get_filename_component(BINARY_PATH "${BINARY_PATH}" ABSOLUTE)
if(NOT IS_DIRECTORY "${BUNDLE_PATH}")
    message(FATAL_ERROR "Plugin bundle not found: ${BUNDLE_PATH}")
endif()
if(NOT EXISTS "${BINARY_PATH}")
    message(FATAL_ERROR "Plugin binary not found: ${BINARY_PATH}")
endif()

if(NOT DEFINED SEARCH_DIRS)
    set(SEARCH_DIRS "")
endif()

include(BundleUtilities)

set(FRAMEWORKS_DIR "${BUNDLE_PATH}/Contents/Frameworks")
file(MAKE_DIRECTORY "${FRAMEWORKS_DIR}")
get_filename_component(BINARY_DIR "${BINARY_PATH}" DIRECTORY)

function(is_runtime_dependency dependency result_var)
    if(dependency MATCHES "^/System/" OR
       dependency MATCHES "^/usr/lib/" OR
       dependency MATCHES "^@rpath/libobs\\.framework/" OR
       dependency MATCHES "^/Applications/OBS\\.app/Contents/Frameworks/")
        set(${result_var} FALSE PARENT_SCOPE)
    else()
        set(${result_var} TRUE PARENT_SCOPE)
    endif()
endfunction()

set(PENDING_ITEMS "${BINARY_PATH}")
set(PROCESSED_ITEMS "")
set(BUNDLED_ITEMS "")

# Discover the dependency closure one level at a time. This keeps OBS and
# system frameworks external while following Homebrew OpenCV dependencies.
while(PENDING_ITEMS)
    list(POP_FRONT PENDING_ITEMS current_item)
    if(current_item IN_LIST PROCESSED_ITEMS)
        continue()
    endif()
    list(APPEND PROCESSED_ITEMS "${current_item}")

    set(current_rpaths "")
    get_item_rpaths("${current_item}" current_rpaths)
    set(current_dependencies "")
    get_prerequisites(
        "${current_item}"
        current_dependencies
        0
        0
        "${BINARY_DIR}"
        "${SEARCH_DIRS}"
        "${current_rpaths}"
    )

    foreach(dependency IN LISTS current_dependencies)
        is_runtime_dependency("${dependency}" should_bundle)
        if(NOT should_bundle)
            continue()
        endif()

        gp_resolve_item(
            "${current_item}"
            "${dependency}"
            "${BINARY_DIR}"
            "${SEARCH_DIRS}"
            resolved_dependency
            "${current_rpaths}"
        )
        if(NOT IS_ABSOLUTE "${resolved_dependency}" OR NOT EXISTS "${resolved_dependency}")
            message(FATAL_ERROR "Unable to resolve dependency ${dependency} from ${current_item}")
        endif()

        get_filename_component(resolved_realpath "${resolved_dependency}" REALPATH)
        string(FIND "${resolved_realpath}" "${BUNDLE_PATH}/" bundle_prefix)
        if(bundle_prefix EQUAL 0)
            if(NOT "${resolved_dependency}" IN_LIST BUNDLED_ITEMS)
                list(APPEND BUNDLED_ITEMS "${resolved_dependency}")
            endif()
            list(APPEND PENDING_ITEMS "${resolved_dependency}")
            continue()
        endif()

        string(MD5 dependency_key "${resolved_realpath}")
        get_filename_component(dependency_name "${dependency}" NAME)
        if(NOT DEFINED BUNDLED_NAME_${dependency_key})
            string(MD5 dependency_name_key "${dependency_name}")
            if(DEFINED BUNDLED_SOURCE_${dependency_name_key} AND
               NOT "${BUNDLED_SOURCE_${dependency_name_key}}" STREQUAL "${resolved_realpath}")
                message(FATAL_ERROR "Two dependencies use the same bundle name: ${dependency_name}")
            endif()
            set(BUNDLED_SOURCE_${dependency_name_key} "${resolved_realpath}")
            set(BUNDLED_NAME_${dependency_key} "${dependency_name}")
            set(destination "${FRAMEWORKS_DIR}/${dependency_name}")
            execute_process(
                COMMAND "${CMAKE_COMMAND}" -E copy "${resolved_realpath}" "${destination}"
                RESULT_VARIABLE copy_result
            )
            if(NOT copy_result EQUAL 0)
                message(FATAL_ERROR "Unable to copy dependency ${resolved_realpath}")
            endif()
            list(APPEND BUNDLED_ITEMS "${destination}")
            list(APPEND PENDING_ITEMS "${destination}")
        endif()
    endforeach()
endwhile()

find_program(CMAKE_INSTALL_NAME_TOOL install_name_tool)
if(NOT CMAKE_INSTALL_NAME_TOOL)
    message(FATAL_ERROR "install_name_tool was not found")
endif()
set(ITEMS_TO_FIX "${BINARY_PATH}" ${BUNDLED_ITEMS})

foreach(current_item IN LISTS ITEMS_TO_FIX)
    set(current_rpaths "")
    get_item_rpaths("${current_item}" current_rpaths)
    set(current_dependencies "")
    get_prerequisites(
        "${current_item}"
        current_dependencies
        0
        0
        "${BINARY_DIR}"
        "${SEARCH_DIRS};${FRAMEWORKS_DIR}"
        "${current_rpaths}"
    )

    set(changes "")
    foreach(dependency IN LISTS current_dependencies)
        is_runtime_dependency("${dependency}" should_bundle)
        if(NOT should_bundle)
            continue()
        endif()

        gp_resolve_item(
            "${current_item}"
            "${dependency}"
            "${BINARY_DIR}"
            "${SEARCH_DIRS};${FRAMEWORKS_DIR}"
            resolved_dependency
            "${current_rpaths}"
        )
        if(NOT IS_ABSOLUTE "${resolved_dependency}" OR NOT EXISTS "${resolved_dependency}")
            message(FATAL_ERROR "Unable to rewrite dependency ${dependency} in ${current_item}")
        endif()

        get_filename_component(resolved_realpath "${resolved_dependency}" REALPATH)
        string(MD5 dependency_key "${resolved_realpath}")
        string(FIND "${resolved_dependency}" "${FRAMEWORKS_DIR}/" frameworks_prefix)
        if(DEFINED BUNDLED_NAME_${dependency_key})
            set(dependency_name "${BUNDLED_NAME_${dependency_key}}")
        elseif(frameworks_prefix EQUAL 0)
            get_filename_component(dependency_name "${resolved_dependency}" NAME)
        else()
            continue()
        endif()

        if("${current_item}" STREQUAL "${BINARY_PATH}")
            # OBS loads plugins with dlopen(). @loader_path is therefore rooted
            # at the plugin binary, while @executable_path would incorrectly
            # resolve relative to the OBS application executable.
            set(replacement "@loader_path/../Frameworks/${dependency_name}")
        else()
            set(replacement "@loader_path/${dependency_name}")
        endif()
        list(APPEND changes -change "${dependency}" "${replacement}")
    endforeach()

    if(NOT "${current_item}" STREQUAL "${BINARY_PATH}")
        get_filename_component(current_name "${current_item}" NAME)
        list(APPEND changes -id "@rpath/${current_name}")
    else()
        foreach(current_rpath IN LISTS current_rpaths)
            if(IS_ABSOLUTE "${current_rpath}" OR
               "${current_rpath}" STREQUAL "@executable_path/../Frameworks")
                list(APPEND changes -delete_rpath "${current_rpath}")
            endif()
        endforeach()
        if(NOT "@loader_path/../Frameworks" IN_LIST current_rpaths)
            list(APPEND changes -add_rpath "@loader_path/../Frameworks")
        endif()
    endif()

    if(changes)
        execute_process(COMMAND chmod u+w "${current_item}" RESULT_VARIABLE chmod_result)
        if(NOT chmod_result EQUAL 0)
            message(FATAL_ERROR "Unable to make ${current_item} writable")
        endif()
        execute_process(
            COMMAND "${CMAKE_INSTALL_NAME_TOOL}" ${changes} "${current_item}"
            RESULT_VARIABLE install_name_result
        )
        if(NOT install_name_result EQUAL 0)
            message(FATAL_ERROR "Unable to rewrite install names in ${current_item}")
        endif()
    endif()
endforeach()

list(LENGTH BUNDLED_ITEMS bundled_count)
message(STATUS "Bundled ${bundled_count} non-system libraries into ${FRAMEWORKS_DIR}")
