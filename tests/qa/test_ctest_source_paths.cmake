cmake_minimum_required(VERSION 3.16)
get_filename_component(source_root "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(checker "${source_root}/tools/qa/check_ctest_source_paths.cmake")

# Real public tree, with no configure/build or executable tests required.
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${source_root}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "public-tree source check failed:\n${output}${error}")
endif()
message(STATUS "${output}")

# A target with two sources must fail when its second source is absent.
# An unregistered helper is intentionally outside the checker scope.
string(RANDOM LENGTH 16 ALPHABET 0123456789abcdef fixture_id)
set(fixture "${CMAKE_CURRENT_BINARY_DIR}/ctest-source-paths-${fixture_id}")
if(EXISTS "${fixture}")
    message(FATAL_ERROR "refusing to overwrite an existing fixture directory")
endif()
file(MAKE_DIRECTORY "${fixture}")
file(WRITE "${fixture}/exists.c" "int main(void) { return 0; }\n")
file(WRITE "${fixture}/CMakeLists.txt" [=[
# add_executable(comment_only missing_comment.c)
add_executable(unregistered missing_unregistered.c)
add_test(NAME fixture_test COMMAND fixture_target)
add_executable(fixture_target "exists.c" "missing.c")
]=])
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(result EQUAL 0 OR NOT error MATCHES "fixture_test: missing registered test source:.*missing.c")
    message(FATAL_ERROR "missing-source fixture was not rejected correctly:\n${output}${error}")
endif()
# Restore the same fixture to prove the negative did not fail for unrelated syntax.
file(WRITE "${fixture}/missing.c" "/* synthetic second source */\n")
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "restored fixture failed:\n${output}${error}")
endif()
file(REMOVE_RECURSE "${fixture}")
message(STATUS "Missing registered source rejected; restored fixture accepted")
