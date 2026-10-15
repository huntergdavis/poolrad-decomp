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
message(STATUS "Missing registered source rejected; restored fixture accepted")

# Different names may legitimately share a command. CTest names are case
# sensitive, so a case-only difference is also a valid distinct registration.
file(APPEND "${fixture}/CMakeLists.txt" [=[
add_test(NAME fixture_other COMMAND fixture_target)
add_test(NAME Fixture_test COMMAND fixture_target)
]=])
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "distinct-name fixture failed:\n${output}${error}")
endif()
# Both commands have existing sources, so this failure must identify the
# repeated NAME, not an unrelated executable or source-path error.
file(APPEND "${fixture}/CMakeLists.txt" [=[
add_executable(other_target "exists.c")
add_test(NAME fixture_test COMMAND other_target)
]=])
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(result EQUAL 0 OR NOT error MATCHES "duplicate top-level CTest NAME registration: fixture_test")
    message(FATAL_ERROR "duplicate-name fixture was not rejected correctly:\n${output}${error}")
endif()
message(STATUS "Distinct CTest names accepted; duplicate NAME rejected")

# The source list may contain only executable options. These tokens must not
# make a registered target look as though it has a source file (CMake rejects
# the EXCLUDE_FROM_ALL-only case with 'No SOURCES given to target').
foreach(option IN ITEMS WIN32 MACOSX_BUNDLE EXCLUDE_FROM_ALL)
    file(WRITE "${fixture}/CMakeLists.txt"
         "add_executable(flag_target ${option})\nadd_test(NAME flag_test COMMAND flag_target)\n")
    execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                            -P "${checker}"
                    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(result EQUAL 0 OR NOT error MATCHES "flag_test: executable has no actual source paths")
        message(FATAL_ERROR "${option}-only fixture was not rejected correctly:\n${output}${error}")
    endif()
    file(WRITE "${fixture}/CMakeLists.txt"
         "add_executable(flag_target ${option} exists.c)\nadd_test(NAME flag_test COMMAND flag_target)\n")
    execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                            -P "${checker}"
                    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "${option} with a real source was rejected:\n${output}${error}")
    endif()
endforeach()
message(STATUS "Option-only targets rejected; options with real sources accepted")

# CMake stops recognizing executable options when the first source begins.
# Each trailing option spelling must therefore name an existing source file.
foreach(option IN ITEMS WIN32 MACOSX_BUNDLE EXCLUDE_FROM_ALL)
    file(WRITE "${fixture}/CMakeLists.txt"
         "add_executable(position_target exists.c ${option})\nadd_test(NAME position_test COMMAND position_target)\n")
    execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                            -P "${checker}"
                    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(result EQUAL 0 OR NOT error MATCHES "position_test: missing registered test source:.*${option}")
        message(FATAL_ERROR "trailing ${option} source was not checked:\n${output}${error}")
    endif()
    file(WRITE "${fixture}/${option}" "/* synthetic trailing option-named source */\n")
    execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                            -P "${checker}"
                    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(NOT result EQUAL 0 OR NOT output MATCHES "Checked 1 CTest registrations and 2 source paths")
        message(FATAL_ERROR "restored trailing ${option} source lost membership:\n${output}${error}")
    endif()
endforeach()
message(STATUS "Trailing executable option spellings checked as source paths")

# A # inside a quoted source filename is data; real full-line and trailing
# comments still must not become phantom executable/test registrations.
file(WRITE "${fixture}/source#part.c" "int main(void) { return 0; }\n")
file(WRITE "${fixture}/CMakeLists.txt" [=[
# add_test(NAME phantom COMMAND unknown_target)
add_executable(hash_target
    "source#part.c" # comment with "quoted#text"
)
add_test(NAME hash_test COMMAND hash_target) # real trailing comment
]=])
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0 OR NOT output MATCHES "Checked 1 CTest registrations and 1 source paths")
    message(FATAL_ERROR "quoted-hash source/comment fixture failed:\n${output}${error}")
endif()
file(WRITE "${fixture}/CMakeLists.txt" [=[
add_executable(hash_target "missing#part.c")
add_test(NAME hash_test COMMAND hash_target)
]=])
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(result EQUAL 0 OR NOT error MATCHES "missing registered test source:.*missing#part.c")
    message(FATAL_ERROR "missing quoted-hash source was not rejected correctly:\n${output}${error}")
endif()
message(STATUS "Quoted # paths and real comments accepted; missing # source rejected")

file(WRITE "${fixture}/CMakeLists.txt" [=[
add_executable(hash_target source\#part.c)
add_test(NAME hash\#test COMMAND hash_target) # actual trailing comment
]=])
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0 OR NOT output MATCHES "Checked 1 CTest registrations and 1 source paths")
    message(FATAL_ERROR "escaped-hash source/name fixture failed:\n${output}${error}")
endif()
file(WRITE "${fixture}/CMakeLists.txt" [=[
add_executable(hash_target missing\#part.c)
add_test(NAME hash\#test COMMAND hash_target)
]=])
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(result EQUAL 0 OR NOT error MATCHES "missing registered test source:.*missing#part.c")
    message(FATAL_ERROR "missing escaped-hash source was not rejected:\n${output}${error}")
endif()
message(STATUS "Escaped # paths/names accepted; missing escaped # source rejected")

file(WRITE "${fixture}/source;part.c" "/* synthetic literal-semicolon source */\n")
file(WRITE "${fixture}/CMakeLists.txt" [=[
add_executable(semicolon_target exists.c "source\;part.c")
add_test(NAME semicolon_test COMMAND semicolon_target)
]=])
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0 OR NOT output MATCHES "Checked 1 CTest registrations and 2 source paths")
    message(FATAL_ERROR "quoted escaped-semicolon source failed:\n${output}${error}")
endif()
file(WRITE "${fixture}/CMakeLists.txt" [=[
add_executable(semicolon_target exists.c "missing\;part.c")
add_test(NAME semicolon_test COMMAND semicolon_target)
]=])
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(result EQUAL 0 OR NOT error MATCHES "missing registered test source:.*missing;part.c")
    message(FATAL_ERROR "missing literal-semicolon source not rejected:\n${output}${error}")
endif()
# An unquoted \; undergoes CMake list expansion at the executable source API.
# It must not accidentally acquire the quoted literal-path behavior.
file(WRITE "${fixture}/CMakeLists.txt" [=[
add_executable(semicolon_target source\;part.c)
add_test(NAME semicolon_test COMMAND semicolon_target)
]=])
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(result EQUAL 0 OR NOT error MATCHES "missing registered test source:.*source")
    message(FATAL_ERROR "unquoted semicolon source was incorrectly accepted:\n${output}${error}")
endif()
# Literal semicolon names are single identities, distinct from their parts
# and from a case-only variant. Unquoted escaped semicolons have the same
# literal-name behavior in actual CMake.
file(WRITE "${fixture}/CMakeLists.txt" [=[
add_executable(semicolon_target exists.c)
add_test(NAME "semicolon;name" COMMAND semicolon_target)
add_test(NAME semicolon COMMAND semicolon_target)
add_test(NAME name COMMAND semicolon_target)
add_test(NAME "Semicolon;name" COMMAND semicolon_target)
add_test(NAME escaped\;name COMMAND semicolon_target)
]=])
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0 OR NOT output MATCHES "Checked 5 CTest registrations and 5 source paths")
    message(FATAL_ERROR "literal-semicolon names lost identity/count:\n${output}${error}")
endif()
file(APPEND "${fixture}/CMakeLists.txt"
     "add_test(NAME \"semicolon;name\" COMMAND semicolon_target)\n")
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(result EQUAL 0 OR NOT error MATCHES "duplicate top-level CTest NAME registration: semicolon;name")
    message(FATAL_ERROR "duplicate semicolon name not rejected:\n${output}${error}")
endif()
file(WRITE "${fixture}/CMakeLists.txt" [=[
add_executable(semicolon_target missing_identity.c)
add_test(NAME "semicolon;name" COMMAND semicolon_target)
]=])
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(result EQUAL 0 OR NOT error MATCHES "semicolon;name: missing registered test source:.*missing_identity.c")
    message(FATAL_ERROR "missing-source name diagnostic lost identity:\n${output}${error}")
endif()
file(WRITE "${fixture}/CMakeLists.txt" [=[
add_executable(semicolon_target exists.c)
add_test(NAME "${dynamic};name" COMMAND semicolon_target)
]=])
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(result EQUAL 0 OR NOT error MATCHES "unsupported literal CTest NAME expression")
    message(FATAL_ERROR "dynamic semicolon name not rejected:\n${output}${error}")
endif()
file(WRITE "${fixture}/CMakeLists.txt" [=[
add_executable(semicolon_target exists.c)
add_test(NAME "semicolon\;name" COMMAND semicolon_target)
]=])
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(result EQUAL 0 OR NOT error MATCHES "quoted escaped-semicolon CTest names are unsupported")
    message(FATAL_ERROR "backslash-bearing semicolon name not rejected:\n${output}${error}")
endif()
message(STATUS "Literal-semicolon name identities preserved; duplicate/missing/unsupported cases rejected")

file(WRITE "${fixture}/CMakeLists.txt" [=[
function(not_add_test)
endfunction()
function(myadd_test)
endfunction()
function(not_add_executable)
endfunction()
add_executable(boundary_target exists.c)
not_add_test(NAME boundary_test COMMAND boundary_target)
myadd_test(NAME boundary_test COMMAND boundary_target)
add_test(NAME boundary_test COMMAND boundary_target)
not_add_test(NAME phantom COMMAND unknown_target)
myadd_test(NAME phantom_other COMMAND unknown_target)
not_add_executable(boundary_target missing_boundary_source.c)
]=])
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0 OR NOT output MATCHES "Checked 1 CTest registrations and 1 source paths")
    message(FATAL_ERROR "custom command suffix was treated as a registration:\n${output}${error}")
endif()
file(APPEND "${fixture}/CMakeLists.txt"
     "add_test(NAME boundary_test COMMAND boundary_target)\n")
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(result EQUAL 0 OR NOT error MATCHES "duplicate top-level CTest NAME registration: boundary_test")
    message(FATAL_ERROR "real command duplicate was not rejected:\n${output}${error}")
endif()
file(WRITE "${fixture}/CMakeLists.txt" [=[
add_executable(boundary_target missing_boundary_source.c)
not_add_executable(boundary_target exists.c)
add_test(NAME boundary_test COMMAND boundary_target)
]=])
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(result EQUAL 0 OR NOT error MATCHES "boundary_test: missing registered test source:.*missing_boundary_source.c")
    message(FATAL_ERROR "custom executable suffix masked a real missing source:\n${output}${error}")
endif()
file(WRITE "${fixture}/CMakeLists.txt" [=[
add_executable(continuation_target "exi\
sts.c")
add_test(NAME "a\
b" COMMAND continuation_target)
add_test(NAME "a
b" COMMAND continuation_target)
]=])
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0 OR NOT output MATCHES "Checked 2 CTest registrations and 2 source paths")
    message(FATAL_ERROR "quoted continuation aliased physical newline or source path:\n${output}${error}")
endif()
file(APPEND "${fixture}/CMakeLists.txt"
     "add_test(NAME ab COMMAND continuation_target)\n")
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(result EQUAL 0 OR NOT error MATCHES "duplicate top-level CTest NAME registration: ab")
    message(FATAL_ERROR "continued name did not alias its actual literal spelling:\n${output}${error}")
endif()
file(WRITE "${fixture}/CMakeLists.txt" [=[
add_executable(continuation_target exists.c)
add_test(NAME "a\\
b" COMMAND continuation_target)
]=])
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(result EQUAL 0 OR NOT error MATCHES "unsupported literal CTest NAME expression")
    message(FATAL_ERROR "escaped backslash before newline lost unsupported identity:\n${output}${error}")
endif()
file(WRITE "${fixture}/CMakeLists.txt" [=[
add_executable(continuation_target "missing\
_continuation.c")
add_test(NAME ab COMMAND continuation_target)
]=])
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(result EQUAL 0 OR NOT error MATCHES "missing registered test source:.*missing_continuation.c")
    message(FATAL_ERROR "continued missing source diagnostic lost literal path:\n${output}${error}")
endif()
file(WRITE "${fixture}/source#bracket).c" "/* synthetic bracket source */\n")
foreach(equals IN ITEMS "" "=" "==")
    file(WRITE "${fixture}/CMakeLists.txt"
         "add_executable(bracket_target [${equals}[\nsource#bracket).c]${equals}])\n"
         "add_test(NAME [${equals}[\nliteral;name]${equals}] COMMAND bracket_target)\n"
         "add_test(NAME literal COMMAND bracket_target)\nadd_test(NAME name COMMAND bracket_target)\n")
    execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                            -P "${checker}"
                    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(NOT result EQUAL 0 OR NOT output MATCHES "Checked 3 CTest registrations and 3 source paths")
        message(FATAL_ERROR "bracket NAME/source identity failed for '${equals}':\n${output}${error}")
    endif()
    file(APPEND "${fixture}/CMakeLists.txt"
         "add_test(NAME \"literal;name\" COMMAND bracket_target)\n")
    execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                            -P "${checker}"
                    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(result EQUAL 0 OR NOT error MATCHES "duplicate top-level CTest NAME registration: literal;name")
        message(FATAL_ERROR "bracket/quoted duplicate NAME lost identity:\n${output}${error}")
    endif()
endforeach()
file(WRITE "${fixture}/CMakeLists.txt" [===[
add_executable(bracket_target [==[exists.c]==])
add_test(NAME [==[literal]middle;name]==] COMMAND bracket_target)
]===])
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(result EQUAL 0 OR NOT error MATCHES "unsupported bracket argument")
    message(FATAL_ERROR "literal bracket-bearing NAME lost unsupported boundary:\n${output}${error}")
endif()
file(WRITE "${fixture}/CMakeLists.txt" [==[
add_executable(bracket_target [=[missing_bracket_identity.c]=])
add_test(NAME [=[literal;name]=] COMMAND bracket_target)
]==])
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(result EQUAL 0 OR NOT error MATCHES "literal;name: missing registered test source:.*missing_bracket_identity.c")
    message(FATAL_ERROR "bracket NAME missing-source diagnostic lost identity:\n${output}${error}")
endif()
file(WRITE "${fixture}/CMakeLists.txt" [==[
add_executable(bracket_target [=[${CMAKE_CURRENT_SOURCE_DIR}/exists.c]=])
add_test(NAME literal COMMAND bracket_target)
]==])
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(result EQUAL 0 OR NOT error MATCHES "unsupported bracket argument")
    message(FATAL_ERROR "bracket variable text was incorrectly expanded:\n${output}${error}")
endif()
file(WRITE "${fixture}/CMakeLists.txt"
     "add_executable(bracket_target exists.c)\nadd_test(NAME [=[literal COMMAND bracket_target)\n")
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(result EQUAL 0 OR NOT error MATCHES "unterminated CMake bracket argument")
    message(FATAL_ERROR "unterminated bracket NAME was not rejected:\n${output}${error}")
endif()
set(quoted_registration_data [==[
set(blob "add_test(NAME phantom COMMAND unknown_target)")
set(duplicate "add_test(NAME first COMMAND target)")
set(executable "add_executable(target missing_quoted_registration.c)")
set(escaped "quoted \"add_test(NAME escaped_phantom COMMAND unknown_target)\"")
set(bracket [=[add_test(NAME bracket_phantom COMMAND unknown_target)]=])
]==])
file(WRITE "${fixture}/CMakeLists.txt" "${quoted_registration_data}" [=[
add_executable(target exists.c)
add_test(NAME first COMMAND target)
]=])
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0 OR NOT output MATCHES "Checked 1 CTest registrations and 1 source paths")
    message(FATAL_ERROR "quoted registration data invented tests/sources:\n${output}${error}")
endif()
file(APPEND "${fixture}/CMakeLists.txt" "add_test(NAME first COMMAND target)\n")
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(result EQUAL 0 OR NOT error MATCHES "duplicate top-level CTest NAME registration: first")
    message(FATAL_ERROR "real duplicate following quoted data not rejected:\n${output}${error}")
endif()
file(WRITE "${fixture}/CMakeLists.txt" [=[
set(blob "add_executable(target exists.c)")
add_executable(target missing_quoted_registration.c)
add_test(NAME first COMMAND target)
]=])
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(result EQUAL 0 OR NOT error MATCHES "first: missing registered test source:.*missing_quoted_registration.c")
    message(FATAL_ERROR "quoted executable data masked real missing source:\n${output}${error}")
endif()
file(WRITE "${fixture}/CMakeLists.txt" "${quoted_registration_data}" [=[
add_executable(target exists.c)
add_test(NAME "add_test(NAME phantom COMMAND unknown_target)" COMMAND target)
]=])
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0 OR NOT output MATCHES "Checked 1 CTest registrations and 1 source paths")
    message(FATAL_ERROR "registration-looking NAME lost literal identity:\n${output}${error}")
endif()
file(WRITE "${fixture}/CMakeLists.txt" "${quoted_registration_data}"
     "add_executable(target exists.c)\nadd_test(NAME unfinished COMMAND target\n")
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(result EQUAL 0 OR NOT error MATCHES "unterminated literal CTest registration command")
    message(FATAL_ERROR "unterminated real registration not rejected:\n${output}${error}")
endif()
foreach(equals IN ITEMS "" "=" "==")
    file(WRITE "${fixture}/CMakeLists.txt"
         "#[${equals}[add_test(NAME phantom COMMAND unknown_target)\n"
         "add_executable(target missing_comment_port.c)\n"
         "# unmatched quoted data: \"not a registration\n]other]\n]${equals}]\n"
         "add_executable(target exists.c #[${equals}[missing_comment_port.c]${equals}])\n"
         "add_test(NAME \"literal;name\" COMMAND target)\n")
    execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                            -P "${checker}"
                    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(NOT result EQUAL 0 OR NOT output MATCHES "Checked 1 CTest registrations and 1 source paths")
        message(FATAL_ERROR "ported bracket comment changed identities for '${equals}':\n${output}${error}")
    endif()
    file(APPEND "${fixture}/CMakeLists.txt"
         "add_test(NAME \"literal;name\" COMMAND target)\n")
    execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                            -P "${checker}"
                    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(result EQUAL 0 OR NOT error MATCHES "duplicate top-level CTest NAME registration: literal;name")
        message(FATAL_ERROR "bracket comment masked real duplicate identity:\n${output}${error}")
    endif()
endforeach()
file(WRITE "${fixture}/CMakeLists.txt" [===[
#[==[ ) ]=] add_test(NAME phantom COMMAND unknown_target)
]==]
add_executable(target missing_comment_port.c)
add_test(NAME real COMMAND target)
]===])
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(result EQUAL 0 OR NOT error MATCHES "real: missing registered test source:.*missing_comment_port.c")
    message(FATAL_ERROR "bracket comment masked real missing source:\n${output}${error}")
endif()
file(WRITE "${fixture}/CMakeLists.txt"
     "add_executable(target exists.c)\nadd_test(NAME real COMMAND target)\n#[=[unterminated\n")
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(result EQUAL 0 OR NOT error MATCHES "unterminated CMake bracket comment")
    message(FATAL_ERROR "unterminated bracket comment not rejected:\n${output}${error}")
endif()
foreach(sources IN ITEMS "exists.c \"\"" "\"\" exists.c" "EXCLUDE_FROM_ALL \"\" exists.c" "\"\" exists.c \"\"")
    file(WRITE "${fixture}/CMakeLists.txt"
         "add_executable(empty_target ${sources})\nadd_test(NAME empty_source_test COMMAND empty_target)\n")
    execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                            -P "${checker}"
                    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(NOT result EQUAL 0 OR NOT output MATCHES "Checked 1 CTest registrations and 1 source paths")
        message(FATAL_ERROR "empty source contributed a path or hid its real source:\n${output}${error}")
    endif()
endforeach()
foreach(sources IN ITEMS "\"\"" "\"\" \"\"" "EXCLUDE_FROM_ALL \"\" \"\"")
    file(WRITE "${fixture}/CMakeLists.txt"
         "add_executable(empty_target ${sources})\nadd_test(NAME empty_source_test COMMAND empty_target)\n")
    execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                            -P "${checker}"
                    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(result EQUAL 0 OR NOT error MATCHES "empty_source_test: executable has no actual source paths")
        message(FATAL_ERROR "empty-only source list was not rejected:\n${output}${error}")
    endif()
endforeach()
file(WRITE "${fixture}/CMakeLists.txt" [=[
add_executable(empty_target "" missing_after_empty.c)
add_test(NAME empty_source_test COMMAND empty_target)
]=])
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(result EQUAL 0 OR NOT error MATCHES "missing registered test source:.*missing_after_empty.c")
    message(FATAL_ERROR "empty source masked a genuine missing source:\n${output}${error}")
endif()
# Earlier fixtures restored this option-named file. Remove only that owned
# fixture file to prove the empty argument preserves positional source rules.
file(REMOVE "${fixture}/EXCLUDE_FROM_ALL")
file(WRITE "${fixture}/CMakeLists.txt" [=[
add_executable(empty_target "" EXCLUDE_FROM_ALL exists.c)
add_test(NAME empty_source_test COMMAND empty_target)
]=])
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(result EQUAL 0 OR NOT error MATCHES "missing registered test source:.*EXCLUDE_FROM_ALL")
    message(FATAL_ERROR "empty source incorrectly extended leading options:\n${output}${error}")
endif()
file(WRITE "${fixture}/EXCLUDE_FROM_ALL" "/* synthetic option-named source */\n")
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0 OR NOT output MATCHES "Checked 1 CTest registrations and 2 source paths")
    message(FATAL_ERROR "restored option-named source lost count after empty entry:\n${output}${error}")
endif()
file(REMOVE_RECURSE "${fixture}")
message(STATUS "Exact command boundaries enforced; real duplicate/missing-source checks preserved")
