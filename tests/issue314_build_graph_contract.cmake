cmake_minimum_required(VERSION 3.16)

set(source_dir "${CMAKE_CURRENT_LIST_DIR}/..")
set(build_root "${CMAKE_CURRENT_BINARY_DIR}/issue314_build_graph")
file(REMOVE_RECURSE "${build_root}")

function(issue314_configure name)
    set(build_dir "${build_root}/${name}")
    set(query_dir "${build_dir}/.cmake/api/v1/query")
    file(MAKE_DIRECTORY "${query_dir}")
    file(WRITE "${query_dir}/codemodel-v2" "")

    execute_process(
        COMMAND
            "${CMAKE_COMMAND}"
            -S "${source_dir}"
            -B "${build_dir}"
            -DBUILD_TESTING=${ARGN}
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE error
    )
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Configure failed for ${name}:\n${output}\n${error}")
    endif()

    set("${name}_build_dir" "${build_dir}" PARENT_SCOPE)
    set("${name}_output" "${output}\n${error}" PARENT_SCOPE)
endfunction()

function(issue314_assert_target build_dir target should_exist)
    file(GLOB target_files "${build_dir}/.cmake/api/v1/reply/target-*.json")
    if(should_exist AND NOT target_files)
        message(FATAL_ERROR "CMake File API target metadata was not generated for ${build_dir}")
    endif()

    set(found FALSE)
    foreach(target_file IN LISTS target_files)
        file(READ "${target_file}" target_json)
        if(target_json MATCHES "\"name\"[ \t\r\n]*:[ \t\r\n]*\"${target}\"")
            set(found TRUE)
        endif()
    endforeach()

    if(should_exist AND NOT found)
        message(FATAL_ERROR "Expected target '${target}' in ${build_dir}, but it was absent")
    endif()
    if(NOT should_exist AND found)
        message(FATAL_ERROR "Target '${target}' was unexpectedly present in ${build_dir}")
    endif()
endfunction()

function(issue314_assert_target_property build_dir target property pattern)
    file(GLOB target_files "${build_dir}/.cmake/api/v1/reply/target-*.json")
    set(target_file "")
    foreach(candidate IN LISTS target_files)
        file(READ "${candidate}" target_json)
        if(target_json MATCHES "\"name\"[ \t\r\n]*:[ \t\r\n]*\"${target}\"")
            set(target_file "${candidate}")
        endif()
    endforeach()

    if("${target_file}" STREQUAL "")
        message(FATAL_ERROR "Expected target '${target}' in ${build_dir}, but it was absent")
    endif()

    file(READ "${target_file}" target_json)
    if(NOT target_json MATCHES "\"${property}\"[ \t\r\n]*:[^]]*\"fragment\"[ \t\r\n]*:[ \t\r\n]*\"${pattern}\"")
        message(FATAL_ERROR "Expected ${property} fragment '${pattern}' on target '${target}' in ${target_file}")
    endif()
endfunction()

issue314_configure(no_tests OFF -DPMM_BUILD_EXAMPLES=OFF)
if(EXISTS "${no_tests_build_dir}/_deps/catch2-src")
    message(FATAL_ERROR "Catch2 was fetched even though BUILD_TESTING=OFF")
endif()
issue314_assert_target("${no_tests_build_dir}" basic_usage FALSE)

issue314_configure(compact_tests ON -DPMM_BUILD_EXAMPLES=OFF)
issue314_assert_target("${compact_tests_build_dir}" test_allocate TRUE)
issue314_assert_target("${compact_tests_build_dir}" basic_usage FALSE)
if(MSVC)
    issue314_assert_target_property("${compact_tests_build_dir}" Catch2 compileCommandFragments "/MP")
    issue314_assert_target_property("${compact_tests_build_dir}" Catch2WithMain compileCommandFragments "/MP")
endif()

issue314_configure(default_graph ON -DPMM_BUILD_EXAMPLES=ON)
issue314_assert_target("${default_graph_build_dir}" test_allocate TRUE)
issue314_assert_target("${default_graph_build_dir}" basic_usage TRUE)
