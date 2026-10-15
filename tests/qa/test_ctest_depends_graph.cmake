cmake_minimum_required(VERSION 3.16)
get_filename_component(source_root "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(checker "${source_root}/tools/qa/check_ctest_depends_graph.cmake")
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${source_root}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "public-tree graph check failed:\n${output}${error}")
endif()

string(RANDOM LENGTH 16 ALPHABET 0123456789abcdef fixture_id)
set(fixture "${CMAKE_CURRENT_BINARY_DIR}/ctest-graph-${fixture_id}")
if(EXISTS "${fixture}")
    message(FATAL_ERROR "refusing to overwrite an existing fixture directory")
endif()
file(MAKE_DIRECTORY "${fixture}")
file(WRITE "${fixture}/exists.c" "int main(void) { return 0; }\n")
set(base [=[
add_executable(target exists.c)
add_test(NAME first COMMAND target)
add_test(NAME second COMMAND target)
add_test(NAME third COMMAND target)
]=])
set(expected_count 3)
function(check_case name setters diagnostic)
    file(WRITE "${fixture}/CMakeLists.txt" "${base}\n${setters}\n")
    execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                            -P "${checker}"
                    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(diagnostic STREQUAL "")
        if(NOT result EQUAL 0 OR NOT output MATCHES "Checked ${expected_count} CTest registrations and ${expected_count} source paths")
            message(FATAL_ERROR "${name} rejected:\n${output}${error}")
        endif()
    elseif(result EQUAL 0 OR NOT error MATCHES "${diagnostic}")
        message(FATAL_ERROR "${name} not rejected by expected diagnostic:\n${output}${error}")
    endif()
    string(ASCII 30 identity_marker)
    if(error MATCHES "${identity_marker}")
        message(FATAL_ERROR "${name} leaked an internal identity marker in its diagnostic")
    endif()
endfunction()

check_case(duplicate_edges [=[
set_tests_properties(second PROPERTIES DEPENDS "first;first")
set_property(TEST third APPEND PROPERTY DEPENDS second first)
]=] "")
check_case(overwritten_cycle [=[
set_tests_properties(first PROPERTIES DEPENDS second)
set_property(TEST second PROPERTY DEPENDS first)
set_property(TEST first PROPERTY DEPENDS)
]=] "")
check_case(replaced_cycle [=[
set_tests_properties(first PROPERTIES DEPENDS second)
set_property(TEST second APPEND PROPERTY DEPENDS first)
set_tests_properties(first PROPERTIES DEPENDS third)
]=] "")
check_case(self_reference [=[
set_tests_properties(first PROPERTIES DEPENDS first)
]=] "self CTest DEPENDS reference: first")
check_case(two_node_cycle [=[
set_tests_properties(first PROPERTIES DEPENDS second)
set_property(TEST second APPEND PROPERTY DEPENDS first)
]=] "CTest DEPENDS cycle leaves blocked tests:")
check_case(three_node_cycle [=[
set_tests_properties(first PROPERTIES DEPENDS second)
set_tests_properties(second PROPERTIES DEPENDS third)
set_property(TEST third PROPERTY DEPENDS first)
]=] "CTest DEPENDS cycle leaves blocked tests:")
set(base [=[
add_executable(target exists.c)
add_test(NAME "first;part" COMMAND target)
add_test(NAME "second;part" COMMAND target)
add_test(NAME "third;part" COMMAND target)
add_test(NAME first COMMAND target)
add_test(NAME part COMMAND target)
]=])
set(expected_count 5)
check_case(semicolon_repeated_empty [=[
set_tests_properties("first;part" PROPERTIES DEPENDS ";second\;part;;second\;part;")
set_property(TEST "third;part" APPEND PROPERTY DEPENDS "first\;part")
]=] "")
check_case(semicolon_cleared_cycle [=[
set_tests_properties("first;part" PROPERTIES DEPENDS "second\;part")
set_property(TEST "second;part" PROPERTY DEPENDS "first\;part")
set_property(TEST "first;part" PROPERTY DEPENDS ";;")
]=] "")
check_case(semicolon_replaced_cycle [=[
set_tests_properties("first;part" PROPERTIES DEPENDS "second\;part")
set_property(TEST "second;part" APPEND PROPERTY DEPENDS "first\;part")
set_tests_properties("first;part" PROPERTIES DEPENDS "third\;part")
]=] "")
check_case(semicolon_distinct_component_names [=[
set_tests_properties("first;part" PROPERTIES DEPENDS first)
set_property(TEST first PROPERTY DEPENDS part)
]=] "")
check_case(semicolon_self_reference [=[
set_tests_properties("first;part" PROPERTIES DEPENDS "first\;part")
]=] "self CTest DEPENDS reference: first;part")
check_case(semicolon_two_node_cycle [=[
set_tests_properties("first;part" PROPERTIES DEPENDS "second\;part")
set_property(TEST "second;part" APPEND PROPERTY DEPENDS "first\;part")
]=] "CTest DEPENDS cycle leaves blocked tests:")
check_case(semicolon_three_node_cycle [=[
set_tests_properties("first;part" PROPERTIES DEPENDS "second\;part")
set_tests_properties("second;part" PROPERTIES DEPENDS "third\;part")
set_property(TEST "third;part" PROPERTY DEPENDS "first\;part")
]=] "CTest DEPENDS cycle leaves blocked tests:")
check_case(semicolon_component_cycle [=[
set_tests_properties("first;part" PROPERTIES DEPENDS first)
set_property(TEST first PROPERTY DEPENDS "first\;part")
]=] "CTest DEPENDS cycle leaves blocked tests:")
check_case(unsupported_quoted_escape_name [=[
add_test(NAME "unsupported\;part" COMMAND target)
]=] "quoted escaped-semicolon CTest names are unsupported")
check_case(unsupported_quoted_escape_owner [=[
set_property(TEST "first\;part" PROPERTY DEPENDS first)
]=] "quoted escaped-semicolon DEPENDS owners are unsupported")
set(base [=[
add_executable(target exists.c)
add_test(NAME "owner)part" COMMAND target)
add_test(NAME "dep)part" COMMAND target)
add_test(NAME "third)part" COMMAND target)
]=])
set(expected_count 3)
check_case(quoted_parenthesis_setters [=[
set_tests_properties("owner)part" PROPERTIES DEPENDS "dep)part")
set_property(TEST "third)part" APPEND PROPERTY DEPENDS "owner)part")
]=] "")
check_case(quoted_parenthesis_duplicate [=[
add_test(NAME "owner)part" COMMAND target)
]=] "duplicate top-level CTest NAME registration: owner\\)part")
check_case(quoted_parenthesis_dangling_reference [=[
set_tests_properties("owner)part" PROPERTIES DEPENDS "missing)part")
]=] "DEPENDS reference: unregistered CTest name: missing\\)part")
check_case(quoted_parenthesis_dangling_owner [=[
set_property(TEST "missing)part" PROPERTY DEPENDS "dep)part")
]=] "DEPENDS owner: unregistered CTest name: missing\\)part")
check_case(quoted_parenthesis_self [=[
set_property(TEST "owner)part" PROPERTY DEPENDS "owner)part")
]=] "self CTest DEPENDS reference: owner\\)part")
check_case(quoted_parenthesis_cycle [=[
set_tests_properties("owner)part" PROPERTIES DEPENDS "dep)part")
set_property(TEST "dep)part" PROPERTY DEPENDS "owner)part")
]=] "CTest DEPENDS cycle leaves blocked tests:")
check_case(quoted_parenthesis_dynamic_unsupported [=[
add_test(NAME "${dynamic})part" COMMAND target)
]=] "unsupported literal CTest NAME expression")
set(base [=[
add_executable(target exists.c)
add_test(NAME "a\tb" COMMAND target)
add_test(NAME atb COMMAND target)
add_test(NAME missingtpart COMMAND target)
]=])
set(expected_count 3)
string(ASCII 9 literal_tab)
check_case(quoted_tab_distinct_identities [=[
set_tests_properties(atb PROPERTIES DEPENDS "a\tb")
set_property(TEST "a\tb" PROPERTY DEPENDS "")
set_property(TEST missingtpart APPEND PROPERTY DEPENDS atb)
]=] "")
check_case(quoted_tab_duplicate
           "add_test(NAME \"a${literal_tab}b\" COMMAND target)"
           "duplicate top-level CTest NAME registration: a${literal_tab}b")
check_case(quoted_tab_dangling_reference [=[
set_tests_properties(atb PROPERTIES DEPENDS "missing\tpart")
]=] "DEPENDS reference: unregistered CTest name: missing${literal_tab}part")
check_case(quoted_tab_dangling_owner [=[
set_property(TEST "missing\tpart" PROPERTY DEPENDS atb)
]=] "DEPENDS owner: unregistered CTest name: missing${literal_tab}part")
check_case(quoted_tab_self [=[
set_property(TEST "a\tb" PROPERTY DEPENDS "a\tb")
]=] "self CTest DEPENDS reference: a${literal_tab}b")
check_case(quoted_tab_cycle [=[
set_tests_properties(atb PROPERTIES DEPENDS "a\tb")
set_property(TEST "a\tb" PROPERTY DEPENDS atb)
]=] "CTest DEPENDS cycle leaves blocked tests:")
check_case(quoted_tab_literal_backslash_unsupported [=[
add_test(NAME "a\\tb" COMMAND target)
]=] "unsupported literal CTest NAME expression")
check_case(quoted_tab_dynamic_unsupported [=[
add_test(NAME "${dynamic}\tb" COMMAND target)
]=] "unsupported literal CTest NAME expression")
set(base [=[
add_executable(target exists.c)
add_test(NAME owner\)part COMMAND target)
add_test(NAME dep\)part COMMAND target)
add_test(NAME third\)part COMMAND target)
]=])
set(expected_count 3)
check_case(escaped_parenthesis_setters [=[
set_tests_properties(owner\)part PROPERTIES DEPENDS dep\)part)
set_property(TEST third\)part APPEND PROPERTY DEPENDS owner\)part)
]=] "")
check_case(escaped_parenthesis_duplicate [=[
add_test(NAME "owner)part" COMMAND target)
]=] "duplicate top-level CTest NAME registration: owner\\)part")
check_case(escaped_parenthesis_dangling_reference [=[
set_tests_properties(owner\)part PROPERTIES DEPENDS missing\)part)
]=] "DEPENDS reference: unregistered CTest name: missing\\)part")
check_case(escaped_parenthesis_dangling_owner [=[
set_property(TEST missing\)part PROPERTY DEPENDS dep\)part)
]=] "DEPENDS owner: unregistered CTest name: missing\\)part")
check_case(escaped_parenthesis_self [=[
set_property(TEST owner\)part PROPERTY DEPENDS owner\)part)
]=] "self CTest DEPENDS reference: owner\\)part")
check_case(escaped_parenthesis_cycle [=[
set_tests_properties(owner\)part PROPERTIES DEPENDS dep\)part)
set_property(TEST dep\)part PROPERTY DEPENDS owner\)part)
]=] "CTest DEPENDS cycle leaves blocked tests:")
check_case(escaped_parenthesis_backslash_unsupported [=[
add_test(NAME raw\\\)part COMMAND target)
]=] "unsupported literal CTest NAME expression")
set(base [=[
add_executable(target exists.c)
add_test(NAME "a\nb" COMMAND target)
add_test(NAME anb COMMAND target)
add_test(NAME missingnpart COMMAND target)
]=])
set(expected_count 3)
string(ASCII 10 literal_newline)
check_case(quoted_newline_distinct_identities [=[
set_tests_properties(anb PROPERTIES DEPENDS "a\nb")
set_property(TEST "a\nb" PROPERTY DEPENDS "")
set_property(TEST missingnpart APPEND PROPERTY DEPENDS anb)
]=] "")
# CMake diagnostics indent each line of a newline-bearing name.
check_case(quoted_newline_duplicate
           "add_test(NAME \"a${literal_newline}b\" COMMAND target)"
           "duplicate top-level CTest NAME registration: a[\r\n \t]+b")
check_case(quoted_newline_dangling_reference [=[
set_tests_properties(anb PROPERTIES DEPENDS "missing\npart")
]=] "DEPENDS reference: unregistered CTest name: missing[\r\n \t]+part")
check_case(quoted_newline_dangling_owner [=[
set_property(TEST "missing\npart" PROPERTY DEPENDS anb)
]=] "DEPENDS owner: unregistered CTest name: missing[\r\n \t]+part")
check_case(quoted_newline_self [=[
set_property(TEST "a\nb" PROPERTY DEPENDS "a\nb")
]=] "self CTest DEPENDS reference: a[\r\n \t]+b")
check_case(quoted_newline_cycle [=[
set_tests_properties(anb PROPERTIES DEPENDS "a\nb")
set_property(TEST "a\nb" PROPERTY DEPENDS anb)
]=] "CTest DEPENDS cycle leaves blocked tests:")
check_case(quoted_newline_literal_backslash_unsupported [=[
add_test(NAME "a\\nb" COMMAND target)
]=] "unsupported literal CTest NAME expression")
check_case(quoted_newline_dynamic_unsupported [=[
add_test(NAME "${dynamic}\nb" COMMAND target)
]=] "unsupported literal CTest NAME expression")
set(base [=[
add_executable(target exists.c)
add_test(NAME "a\rb" COMMAND target)
add_test(NAME arb COMMAND target)
add_test(NAME missingrpart COMMAND target)
]=])
set(expected_count 3)
check_case(quoted_carriage_return_distinct_identities [=[
set_tests_properties(arb PROPERTIES DEPENDS "a\rb")
set_property(TEST "a\rb" PROPERTY DEPENDS "")
set_property(TEST missingrpart APPEND PROPERTY DEPENDS arb)
]=] "")
check_case(quoted_carriage_return_duplicate [=[
add_test(NAME "a\rb" COMMAND target)
]=] "duplicate top-level CTest NAME registration: a[\r\n \t]+b")
check_case(quoted_carriage_return_dangling_reference [=[
set_tests_properties(arb PROPERTIES DEPENDS "missing\rpart")
]=] "DEPENDS reference: unregistered CTest name: missing[\r\n \t]+part")
check_case(quoted_carriage_return_dangling_owner [=[
set_property(TEST "missing\rpart" PROPERTY DEPENDS arb)
]=] "DEPENDS owner: unregistered CTest name: missing[\r\n \t]+part")
check_case(quoted_carriage_return_self [=[
set_property(TEST "a\rb" PROPERTY DEPENDS "a\rb")
]=] "self CTest DEPENDS reference: a[\r\n \t]+b")
check_case(quoted_carriage_return_cycle [=[
set_tests_properties(arb PROPERTIES DEPENDS "a\rb")
set_property(TEST "a\rb" PROPERTY DEPENDS arb)
]=] "CTest DEPENDS cycle leaves blocked tests:")
check_case(quoted_carriage_return_literal_backslash_unsupported [=[
add_test(NAME "a\\rb" COMMAND target)
]=] "unsupported literal CTest NAME expression")
check_case(quoted_carriage_return_dynamic_unsupported [=[
add_test(NAME "${dynamic}\rb" COMMAND target)
]=] "unsupported literal CTest NAME expression")
set(base [=[
add_executable(target exists.c)
add_test(NAME 'ab' COMMAND target)
add_test(NAME ab COMMAND target)
add_test(NAME a'b COMMAND target)
add_test(NAME missing COMMAND target)
]=])
set(expected_count 4)
check_case(apostrophe_distinct_identities [=[
set_tests_properties(ab PROPERTIES DEPENDS 'ab')
set_property(TEST 'ab' PROPERTY DEPENDS "")
set_property(TEST a'b APPEND PROPERTY DEPENDS ab)
]=] "")
check_case(apostrophe_duplicate [=[
add_test(NAME "'ab'" COMMAND target)
]=] "duplicate top-level CTest NAME registration: 'ab'")
check_case(apostrophe_dangling_reference [=[
set_tests_properties(ab PROPERTIES DEPENDS 'missing')
]=] "DEPENDS reference: unregistered CTest name: 'missing'")
check_case(apostrophe_dangling_owner [=[
set_property(TEST 'missing' PROPERTY DEPENDS ab)
]=] "DEPENDS owner: unregistered CTest name: 'missing'")
check_case(apostrophe_self [=[
set_property(TEST 'ab' PROPERTY DEPENDS 'ab')
]=] "self CTest DEPENDS reference: 'ab'")
check_case(apostrophe_cycle [=[
set_tests_properties(ab PROPERTIES DEPENDS 'ab')
set_property(TEST 'ab' PROPERTY DEPENDS ab)
]=] "CTest DEPENDS cycle leaves blocked tests:")
check_case(apostrophe_literal_backslash_unsupported [=[
add_test(NAME "a\\'b" COMMAND target)
]=] "unsupported literal CTest NAME expression")
check_case(apostrophe_dynamic_unsupported [=[
add_test(NAME "${dynamic}'b" COMMAND target)
]=] "unsupported literal CTest NAME expression")
file(WRITE "${fixture}/script'part.cmake" "")
set(base [=[
add_test(NAME script_source COMMAND "${CMAKE_COMMAND}" -P script'part.cmake)
]=])
set(expected_count 1)
check_case(apostrophe_script_source "" "")
set(base [=[
add_test(NAME script_source COMMAND "${CMAKE_COMMAND}" -P missing'part.cmake)
]=])
check_case(apostrophe_missing_script_source ""
           "missing registered test source:.*missing'part.cmake")
foreach(control IN ITEMS t n r)
    set(escaped_name "a\\${control}b")
    set(plain_name "a${control}b")
    set(escaped_missing "missing\\${control}part")
    set(base "add_executable(target exists.c)\nadd_test(NAME ${escaped_name} COMMAND target)\nadd_test(NAME ${plain_name} COMMAND target)\nadd_test(NAME missing${control}part COMMAND target)\n")
    set(expected_count 3)
    check_case("unquoted_${control}_distinct"
               "set_tests_properties(${plain_name} PROPERTIES DEPENDS ${escaped_name})\nset_property(TEST ${escaped_name} PROPERTY DEPENDS \"\")"
               "")
    check_case("unquoted_${control}_duplicate"
               "add_test(NAME \"${escaped_name}\" COMMAND target)"
               "duplicate top-level CTest NAME registration: a[\r\n \t]+b")
    check_case("unquoted_${control}_dangling_reference"
               "set_tests_properties(${plain_name} PROPERTIES DEPENDS ${escaped_missing})"
               "DEPENDS reference: unregistered CTest name: missing[\r\n \t]+part")
    check_case("unquoted_${control}_dangling_owner"
               "set_property(TEST ${escaped_missing} PROPERTY DEPENDS ${plain_name})"
               "DEPENDS owner: unregistered CTest name: missing[\r\n \t]+part")
    check_case("unquoted_${control}_self"
               "set_property(TEST ${escaped_name} PROPERTY DEPENDS ${escaped_name})"
               "self CTest DEPENDS reference: a[\r\n \t]+b")
    check_case("unquoted_${control}_cycle"
               "set_property(TEST ${escaped_name} PROPERTY DEPENDS ${plain_name})\nset_tests_properties(${plain_name} PROPERTIES DEPENDS ${escaped_name})"
               "CTest DEPENDS cycle leaves blocked tests:")
    check_case("unquoted_${control}_backslash_unsupported"
               "add_test(NAME a\\\\${control}b COMMAND target)"
               "unsupported literal CTest NAME expression")
endforeach()
set(base [=[
ADD_EXECUTABLE(target exists.c)
ADD_TEST(NAME First COMMAND target)
aDd_TeSt(NAME first COMMAND target)
AdD_TeSt(NAME second COMMAND target)
]=])
set(expected_count 3)
check_case(command_case_setters [=[
SeT_TeStS_PrOpErTiEs(first PROPERTIES DEPENDS First)
SET_PROPERTY(TEST second APPEND PROPERTY DEPENDS first)
]=] "")
check_case(command_case_duplicate [=[
add_test(NAME First COMMAND target)
]=] "duplicate top-level CTest NAME registration: First")
check_case(command_case_dangling_reference [=[
SET_TESTS_PROPERTIES(first PROPERTIES DEPENDS missing)
]=] "DEPENDS reference: unregistered CTest name: missing")
check_case(command_case_dangling_owner [=[
SeT_PrOpErTy(TEST missing PROPERTY DEPENDS First)
]=] "DEPENDS owner: unregistered CTest name: missing")
check_case(command_case_self [=[
SET_PROPERTY(TEST First PROPERTY DEPENDS First)
]=] "self CTest DEPENDS reference: First")
check_case(command_case_cycle [=[
SeT_TeStS_PrOpErTiEs(first PROPERTIES DEPENDS second)
SET_PROPERTY(TEST second PROPERTY DEPENDS first)
]=] "CTest DEPENDS cycle leaves blocked tests:")
check_case(command_case_dynamic_unsupported [=[
ADD_TEST(NAME "${dynamic}" COMMAND target)
]=] "unsupported literal CTest NAME expression")
set(base [=[
ADD_EXECUTABLE(target exists.c)
ADD_TEST(NAME "ADD_TEST(foo" COMMAND target)
ADD_TEST(NAME "add_test(foo" COMMAND target)
]=])
set(expected_count 2)
check_case(command_case_quoted_identity_preserved "" "")
foreach(owner IN ITEMS OFF FALSE 0)
    set(base "add_executable(target exists.c)\nadd_test(NAME ${owner} COMMAND target)\nadd_test(NAME dep COMMAND target)\n")
    set(expected_count 2)
    check_case("false_owner_${owner}_acyclic"
               "set_property(TEST ${owner} PROPERTY DEPENDS dep)" "")
    check_case("false_owner_${owner}_self"
               "set_property(TEST ${owner} PROPERTY DEPENDS ${owner})"
               "self CTest DEPENDS reference: ${owner}")
    check_case("false_owner_${owner}_cycle"
               "set_property(TEST ${owner} PROPERTY DEPENDS dep)\nset_tests_properties(dep PROPERTIES DEPENDS ${owner})"
               "CTest DEPENDS cycle leaves blocked tests:")
    check_case("false_owner_${owner}_cleared_cycle"
               "set_property(TEST ${owner} PROPERTY DEPENDS dep)\nset_tests_properties(dep PROPERTIES DEPENDS ${owner})\nset_property(TEST ${owner} PROPERTY DEPENDS)"
               "")
endforeach()
set(base [=[
add_executable(target exists.c)
add_test(NAME first COMMAND target)
add_test(NAME second COMMAND target)
]=])
set(expected_count 2)
check_case(custom_setter_suffixes [=[
not_set_property(TEST first PROPERTY DEPENDS first)
myset_property(TEST first PROPERTY DEPENDS second)
not_set_tests_properties(second PROPERTIES DEPENDS first)
myset_tests_properties(second PROPERTIES DEPENDS second)
]=] "")
check_case(real_setter_after_custom_suffix [=[
myset_property(TEST first PROPERTY DEPENDS second)
set_property(TEST first PROPERTY DEPENDS second)
not_set_tests_properties(second PROPERTIES DEPENDS first)
]=] "")
check_case(real_self_after_custom_suffix [=[
not_set_property(TEST first PROPERTY DEPENDS first)
set_property(TEST first PROPERTY DEPENDS first)
]=] "self CTest DEPENDS reference: first")
check_case(real_cycle_after_custom_suffix [=[
myset_property(TEST first PROPERTY DEPENDS second)
not_set_tests_properties(second PROPERTIES DEPENDS first)
set_property(TEST first PROPERTY DEPENDS second)
set_tests_properties(second PROPERTIES DEPENDS first)
]=] "CTest DEPENDS cycle leaves blocked tests:")
check_case(quoted_setter_data [=[
set(blob "set_property(TEST first PROPERTY DEPENDS first)")
set(other "set_tests_properties(second PROPERTIES DEPENDS first)")
set(escaped "quoted \"set_property(TEST missing PROPERTY DEPENDS missing)\"")
]=] "")
check_case(real_setter_after_quoted_data [=[
set(blob "set_property(TEST first PROPERTY DEPENDS second)")
set_property(TEST first PROPERTY DEPENDS second)
set(other "set_tests_properties(second PROPERTIES DEPENDS first)")
]=] "")
check_case(real_self_after_quoted_data [=[
set(blob "set_property(TEST first PROPERTY DEPENDS first)")
set_property(TEST first PROPERTY DEPENDS first)
]=] "self CTest DEPENDS reference: first")
check_case(real_cycle_after_quoted_data [=[
set(blob "set_property(TEST first PROPERTY DEPENDS second)")
set(other "set_tests_properties(second PROPERTIES DEPENDS first)")
set_property(TEST first PROPERTY DEPENDS second)
set_tests_properties(second PROPERTIES DEPENDS first)
]=] "CTest DEPENDS cycle leaves blocked tests:")
set(base [=[
add_executable(target exists.c)
add_test(NAME "set_property(TEST first PROPERTY DEPENDS first)" COMMAND target)
add_test(NAME second COMMAND target)
]=])
check_case(quoted_setter_text_identity [=[
set_property(TEST second PROPERTY DEPENDS "set_property(TEST first PROPERTY DEPENDS first)")
]=] "")
check_case(quoted_setter_text_self [=[
set_property(TEST "set_property(TEST first PROPERTY DEPENDS first)" PROPERTY DEPENDS
             "set_property(TEST first PROPERTY DEPENDS first)")
]=] "self CTest DEPENDS reference: set_property")
set(base [=[
add_executable(target exists.c)
add_test(NAME first COMMAND target)
add_test(NAME second COMMAND target)
]=])
foreach(equals IN ITEMS "" "=" "==")
    check_case("bracket_setter_data_${equals}"
               "set(blob [${equals}[ ) set_property(TEST first PROPERTY DEPENDS first) ]${equals}])\nset(other [${equals}[ ) set_tests_properties(second PROPERTIES DEPENDS first) ]${equals}])" "")
endforeach()
# The combined source lexer explicitly rejects literal brackets in a bracket
# argument before DEPENDS collection; retain that conservative boundary.
check_case(bracket_shorter_close [===[
set(blob [==[ ) ]=] set_property(TEST first PROPERTY DEPENDS first) ]==])
]===] "unsupported bracket argument")
check_case(real_self_after_bracket_data [==[
set(blob [=[ ) set_property(TEST first PROPERTY DEPENDS first) ]=])
set_property(TEST first PROPERTY DEPENDS first)
]==] "self CTest DEPENDS reference: first")
check_case(real_cycle_after_bracket_data [==[
set(blob [=[ ) set_property(TEST first PROPERTY DEPENDS second) ]=])
set(other [=[ ) set_tests_properties(second PROPERTIES DEPENDS first) ]=])
set_property(TEST first PROPERTY DEPENDS second)
set_tests_properties(second PROPERTIES DEPENDS first)
]==] "CTest DEPENDS cycle leaves blocked tests:")
check_case(real_clear_after_bracket_data [==[
set(blob [=[ ) set_property(TEST first PROPERTY DEPENDS first) ]=])
set_property(TEST first PROPERTY DEPENDS second)
set_tests_properties(second PROPERTIES DEPENDS first)
set_property(TEST first PROPERTY DEPENDS)
]==] "")
# Harden the reviewed combined lexer; actual CTest already agrees with these
# bracket owner/reference identities, so this adds regression coverage.
set(base [==[
add_executable(target exists.c)
add_test(NAME [=[first;part]=] COMMAND target)
add_test(NAME [=[second;part]=] COMMAND target)
add_test(NAME consumer COMMAND target)
add_test(NAME first COMMAND target)
add_test(NAME part COMMAND target)
add_test(NAME missing COMMAND target)
]==])
set(expected_count 6)
foreach(equals IN ITEMS "" "=" "==")
    check_case("bracket_owner_reference_${equals}"
               "set_tests_properties([${equals}[consumer]${equals}] PROPERTIES DEPENDS [${equals}[first\\;part]${equals}])" "")
    check_case("bracket_append_repeated_empty_${equals}"
               "set_property(TEST [${equals}[consumer]${equals}] APPEND PROPERTY DEPENDS [${equals}[;first\\;part;;first\\;part;]${equals}])" "")
    # Owner semicolons retain identity; ordinary property semicolons split
    # references to the two component names rather than introducing a self edge.
    check_case("bracket_owner_component_references_${equals}"
               "set_property(TEST [${equals}[first;part]${equals}] PROPERTY DEPENDS [${equals}[first;part]${equals}])" "")
    check_case("bracket_owner_self_${equals}"
               "set_property(TEST [${equals}[first;part]${equals}] PROPERTY DEPENDS [${equals}[first\\;part]${equals}])"
               "self CTest DEPENDS reference: first;part")
endforeach()
check_case(bracket_two_node_cycle [==[
set_tests_properties([=[first;part]=] PROPERTIES DEPENDS [=[second\;part]=])
set_property(TEST [=[second;part]=] PROPERTY DEPENDS [=[first\;part]=])
]==] "CTest DEPENDS cycle leaves blocked tests:")
check_case(bracket_replaced_cycle [==[
set_tests_properties([=[first;part]=] PROPERTIES DEPENDS [=[second\;part]=])
set_property(TEST [=[second;part]=] PROPERTY DEPENDS [=[first\;part]=])
set_tests_properties([=[first;part]=] PROPERTIES DEPENDS [=[first;part]=])
]==] "")
check_case(bracket_cleared_cycle [==[
set_tests_properties([=[first;part]=] PROPERTIES DEPENDS [=[second\;part]=])
set_property(TEST [=[second;part]=] PROPERTY DEPENDS [=[first\;part]=])
set_property(TEST [=[first;part]=] PROPERTY DEPENDS [=[]=])
]==] "")
check_case(bracket_dangling_full_reference [==[
set_property(TEST consumer PROPERTY DEPENDS [=[missing\;part]=])
]==] "DEPENDS reference: unregistered CTest name: missing;part")
check_case(bracket_dangling_full_owner [==[
set_tests_properties([=[missing;part]=] PROPERTIES DEPENDS first)
]==] "DEPENDS owner: unregistered CTest name: missing;part")
check_case(bracket_initial_newline [==[
set_property(TEST [=[
consumer]=] PROPERTY DEPENDS [=[
first\;part]=])
]==] "")
check_case(bracket_unsupported_variable_text [==[
set_property(TEST consumer PROPERTY DEPENDS [=[${dynamic}]=])
]==] "unsupported bracket argument")
check_case(bracket_unterminated_owner [==[
set_property(TEST [=[consumer PROPERTY DEPENDS first)
]==] "unterminated CMake bracket argument")
# Actual CTest agrees: APPEND-empty retains a cycle, replacement-empty clears
# it. Registered false-valued owners must retain their whole name identity.
set(base [==[
add_executable(target exists.c)
add_test(NAME [=[OFF]=] COMMAND target)
add_test(NAME [=[NOTFOUND]=] COMMAND target)
]==])
set(expected_count 2)
foreach(owner IN ITEMS OFF NOTFOUND)
    if(owner STREQUAL "OFF")
        set(other NOTFOUND)
    else()
        set(other OFF)
    endif()
    foreach(equals IN ITEMS "" "=" "==")
        set(owner_argument "[${equals}[${owner}]${equals}]")
        set(other_argument "[${equals}[${other}]${equals}]")
        set(empty_argument "[${equals}[]${equals}]")
        set(cycle "set_property(TEST ${owner_argument} PROPERTY DEPENDS ${other_argument})\nset_tests_properties(${other_argument} PROPERTIES DEPENDS ${owner_argument})\n")
        check_case("false_bracket_${owner}_${equals}_acyclic"
                   "set_property(TEST ${owner_argument} PROPERTY DEPENDS ${other_argument})" "")
        check_case("false_bracket_${owner}_${equals}_self"
                   "set_property(TEST ${owner_argument} PROPERTY DEPENDS ${owner_argument})"
                   "self CTest DEPENDS reference: ${owner}")
        check_case("false_bracket_${owner}_${equals}_cycle" "${cycle}"
                   "CTest DEPENDS cycle leaves blocked tests:")
        check_case("false_bracket_${owner}_${equals}_append_empty"
                   "${cycle}set_property(TEST ${owner_argument} APPEND PROPERTY DEPENDS ${empty_argument})"
                   "CTest DEPENDS cycle leaves blocked tests:")
        check_case("false_bracket_${owner}_${equals}_replace_empty"
                   "${cycle}set_property(TEST ${owner_argument} PROPERTY DEPENDS ${empty_argument})" "")
        check_case("false_bracket_${owner}_${equals}_properties_empty"
                   "${cycle}set_tests_properties(${owner_argument} PROPERTIES DEPENDS ${empty_argument})" "")
    endforeach()
endforeach()
check_case(false_bracket_unregistered_owner [==[
set_property(TEST [=[NO]=] PROPERTY DEPENDS OFF)
]==] "DEPENDS owner: unregistered CTest name: NO")
check_case(false_bracket_unregistered_reference [==[
set_property(TEST [=[NOTFOUND]=] PROPERTY DEPENDS [=[NO]=])
]==] "DEPENDS reference: unregistered CTest name: NO")
file(REMOVE_RECURSE "${fixture}")
message(STATUS "Repeated/replaced/cleared edges accepted; self and cyclic graphs rejected")
