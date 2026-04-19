cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED SOURCE_DIR)
    message(FATAL_ERROR "SOURCE_DIR is required")
endif()

if(NOT DEFINED BINARY_DIR)
    message(FATAL_ERROR "BINARY_DIR is required")
endif()

file(REMOVE_RECURSE "${BINARY_DIR}")
file(MAKE_DIRECTORY "${BINARY_DIR}")

execute_process(
    COMMAND "${CMAKE_COMMAND}" -S "${SOURCE_DIR}" -B "${BINARY_DIR}" -DBUILD_TESTING=OFF
    RESULT_VARIABLE configure_result
    OUTPUT_VARIABLE configure_output
    ERROR_VARIABLE configure_error
)

if(NOT configure_result EQUAL 0)
    message(FATAL_ERROR "Configure failed:\n${configure_output}\n${configure_error}")
endif()

file(
    GLOB_RECURSE generated_project_files
    "${BINARY_DIR}/*.vcxproj"
    "${BINARY_DIR}/build.ninja"
    "${BINARY_DIR}/Makefile"
)

foreach(project_file IN LISTS generated_project_files)
    get_filename_component(project_name "${project_file}" NAME_WE)
    if(project_name MATCHES "^test_[A-Za-z0-9_]+$")
        list(APPEND generated_test_projects "${project_file}")
    endif()
endforeach()

if(generated_test_projects)
    list(JOIN generated_test_projects "\n" generated_test_projects_text)
    message(FATAL_ERROR "BUILD_TESTING=OFF generated test targets:\n${generated_test_projects_text}")
endif()
