cmake_minimum_required(VERSION 3.16)
get_filename_component(source_root "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(checker "${source_root}/tools/qa/check_ctest_depends.cmake")
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${source_root}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "public-tree DEPENDS check failed:\n${output}${error}")
endif()
message(STATUS "${output}")

string(RANDOM LENGTH 16 ALPHABET 0123456789abcdef fixture_id)
set(fixture "${CMAKE_CURRENT_BINARY_DIR}/ctest-depends-${fixture_id}")
if(EXISTS "${fixture}")
    message(FATAL_ERROR "refusing to overwrite an existing fixture directory")
endif()
file(MAKE_DIRECTORY "${fixture}")
file(WRITE "${fixture}/exists.c" "int main(void) { return 0; }\n")
set(base [=[
add_executable(fixture_target exists.c)
add_test(NAME first COMMAND fixture_target)
add_test(NAME second COMMAND fixture_target)
add_test(NAME consumer COMMAND fixture_target)
]=])
file(WRITE "${fixture}/CMakeLists.txt" "${base}" [=[
set_tests_properties(consumer PROPERTIES DEPENDS "first;second" LABELS "some label")
set_property(TEST consumer APPEND PROPERTY DEPENDS first second)
set_tests_properties(consumer PROPERTIES DEPENDS "")
set_property(TEST consumer PROPERTY DEPENDS)
set_tests_properties(consumer PROPERTIES DEPENDS ";first;;second;")
set_property(TEST consumer APPEND PROPERTY DEPENDS ";first;;second;")
set_tests_properties(consumer PROPERTIES DEPENDS ";;")
set_property(TEST consumer PROPERTY DEPENDS ";;")
]=])
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0 OR NOT output MATCHES "Checked 8 literal CTest DEPENDS setters")
    message(FATAL_ERROR "valid-reference fixture failed:\n${output}${error}")
endif()

foreach(setter IN ITEMS
        "set_tests_properties(consumer PROPERTIES DEPENDS \"first;missing\")"
        "set_property(TEST consumer APPEND PROPERTY DEPENDS first missing)"
        "set_tests_properties(consumer PROPERTIES DEPENDS \";first;;missing;\")"
        "set_property(TEST consumer APPEND PROPERTY DEPENDS \";first;;missing;\")")
    file(WRITE "${fixture}/CMakeLists.txt" "${base}\n${setter}\n")
    execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                            -P "${checker}"
                    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(result EQUAL 0 OR NOT error MATCHES "DEPENDS reference: unregistered CTest name: missing")
        message(FATAL_ERROR "dangling-reference fixture was not rejected correctly:\n${output}${error}")
    endif()
endforeach()
# Reject expressions visibly rather than evaluating or guessing their value.
file(WRITE "${fixture}/CMakeLists.txt" "${base}" [=[
set_tests_properties(consumer PROPERTIES DEPENDS "${dynamic_name}")
]=])
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(result EQUAL 0 OR NOT error MATCHES "DEPENDS reference: nonliteral test names are unsupported")
    message(FATAL_ERROR "dynamic-reference fixture was not rejected correctly:\n${output}${error}")
endif()
message(STATUS "Literal DEPENDS references accepted; dangling and dynamic references rejected")

# Escaped # must survive comment removal in names and either property setter.
# Exact counts catch the former silent omission of these declarations.
file(WRITE "${fixture}/CMakeLists.txt" [=[
add_executable(fixture_target exists.c)
add_test(NAME first\#part COMMAND fixture_target)
add_test(NAME second\#part COMMAND fixture_target)
set_tests_properties(first\#part PROPERTIES DEPENDS second\#part)
set_property(TEST first\#part PROPERTY DEPENDS second\#part)
]=])
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0 OR NOT output MATCHES "Checked 2 CTest registrations and 2 source paths" OR
   NOT output MATCHES "Checked 2 literal CTest DEPENDS setters")
    message(FATAL_ERROR "escaped-hash DEPENDS fixture failed:\n${output}${error}")
endif()
foreach(setter IN ITEMS
        "set_tests_properties(first\\#part PROPERTIES DEPENDS missing\\#part)"
        "set_property(TEST first\\#part PROPERTY DEPENDS missing\\#part)")
    file(WRITE "${fixture}/CMakeLists.txt"
         "add_executable(fixture_target exists.c)\nadd_test(NAME first\\#part COMMAND fixture_target)\n${setter}\n")
    execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                            -P "${checker}"
                    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(result EQUAL 0 OR NOT error MATCHES "DEPENDS reference: unregistered CTest name: missing#part")
        message(FATAL_ERROR "dangling escaped-hash dependency was not rejected:\n${output}${error}")
    endif()
endforeach()
message(STATUS "Escaped # DEPENDS names preserved; dangling # references rejected")

set(identity_base [=[
add_executable(fixture_target exists.c)
add_test(NAME "consumer;part" COMMAND fixture_target)
add_test(NAME "dep;part" COMMAND fixture_target)
add_test(NAME dep COMMAND fixture_target)
add_test(NAME part COMMAND fixture_target)
add_test(NAME normal COMMAND fixture_target)
add_test(NAME missing COMMAND fixture_target)
]=])
file(WRITE "${fixture}/CMakeLists.txt" "${identity_base}" [=[
set_tests_properties("consumer;part" PROPERTIES DEPENDS "dep\;part")
set_property(TEST "consumer;part" PROPERTY DEPENDS "dep\;part")
set_property(TEST "consumer;part" APPEND PROPERTY DEPENDS "dep\;part;normal")
set_tests_properties("consumer;part" PROPERTIES DEPENDS "dep;part")
set_property(TEST "consumer;part" PROPERTY DEPENDS dep;part)
set_tests_properties("consumer;part" PROPERTIES DEPENDS ";dep\;part;;normal;")
set_property(TEST "consumer;part" APPEND PROPERTY DEPENDS ";dep\;part;;normal;")
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS alpha;beta)
]=])
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0 OR NOT output MATCHES "Checked 6 CTest registrations and 6 source paths" OR
   NOT output MATCHES "Checked 7 literal CTest DEPENDS setters")
    message(FATAL_ERROR "semicolon DEPENDS identity/count fixture failed:\n${output}${error}")
endif()
# The component names missing and part both exist, while missing;part does
# not. A tokenizer that splits the escaped reference would falsely pass.
foreach(setter IN ITEMS
        "set_tests_properties(\"consumer;part\" PROPERTIES DEPENDS \"missing\\;part\")"
        "set_property(TEST \"consumer;part\" PROPERTY DEPENDS \"missing\\;part\")"
        "set_tests_properties(\"consumer;part\" PROPERTIES DEPENDS \";missing\\;part;;\")"
        "set_property(TEST \"consumer;part\" PROPERTY DEPENDS \";missing\\;part;;\")")
    file(WRITE "${fixture}/CMakeLists.txt" "${identity_base}${setter}\n")
    execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                            -P "${checker}"
                    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(result EQUAL 0 OR NOT error MATCHES "DEPENDS reference: unregistered CTest name: missing;part")
        message(FATAL_ERROR "dangling literal-semicolon reference lost identity:\n${output}${error}")
    endif()
endforeach()
foreach(setter IN ITEMS
        "set_tests_properties(\"missing;part\" PROPERTIES DEPENDS normal)"
        "set_property(TEST \"missing;part\" PROPERTY DEPENDS normal)")
    file(WRITE "${fixture}/CMakeLists.txt" "${identity_base}${setter}\n")
    execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                            -P "${checker}"
                    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(result EQUAL 0 OR NOT error MATCHES "DEPENDS owner: unregistered CTest name: missing;part")
        message(FATAL_ERROR "dangling literal-semicolon owner lost identity:\n${output}${error}")
    endif()
endforeach()
file(WRITE "${fixture}/CMakeLists.txt" "${identity_base}" [=[
set_tests_properties("consumer\;part" PROPERTIES DEPENDS normal)
]=])
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(result EQUAL 0 OR NOT error MATCHES "quoted escaped-semicolon DEPENDS owners are unsupported")
    message(FATAL_ERROR "backslash-bearing owner was not rejected explicitly:\n${output}${error}")
endif()
file(WRITE "${fixture}/CMakeLists.txt" "${identity_base}"
     "set_tests_properties(consumer;part PROPERTIES DEPENDS normal)\n")
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(result EQUAL 0 OR NOT error MATCHES "DEPENDS owner: unregistered CTest name: consumer")
    message(FATAL_ERROR "unquoted owner was not expanded into component names:\n${output}${error}")
endif()
foreach(owner IN ITEMS OFF FALSE 0)
    set(false_base "add_executable(fixture_target exists.c)\nadd_test(NAME ${owner} COMMAND fixture_target)\nadd_test(NAME dep COMMAND fixture_target)\n")
    file(WRITE "${fixture}/CMakeLists.txt" "${false_base}"
         "set_property(TEST ${owner} PROPERTY DEPENDS dep)\nset_property(TEST ${owner} APPEND PROPERTY DEPENDS dep)\nset_tests_properties(${owner} PROPERTIES DEPENDS dep)\n")
    execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                            -P "${checker}"
                    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(NOT result EQUAL 0 OR NOT output MATCHES "Checked 2 CTest registrations and 2 source paths" OR
       NOT output MATCHES "Checked 3 literal CTest DEPENDS setters")
        message(FATAL_ERROR "false-valued owner '${owner}' lost membership/count:\n${output}${error}")
    endif()
    file(WRITE "${fixture}/CMakeLists.txt" "${false_base}"
         "set_property(TEST ${owner} PROPERTY DEPENDS missing)\n")
    execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                            -P "${checker}"
                    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(result EQUAL 0 OR NOT error MATCHES "DEPENDS reference: unregistered CTest name: missing")
        message(FATAL_ERROR "false-valued owner masked dangling reference:\n${output}${error}")
    endif()
endforeach()
file(WRITE "${fixture}/CMakeLists.txt" "${false_base}"
     "set_property(TEST NOTFOUND PROPERTY DEPENDS dep)\n")
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(result EQUAL 0 OR NOT error MATCHES "DEPENDS owner: unregistered CTest name: NOTFOUND")
    message(FATAL_ERROR "unregistered false-valued owner not rejected:\n${output}${error}")
endif()
file(WRITE "${fixture}/CMakeLists.txt" "${false_base}"
     "set_property(TEST APPEND PROPERTY DEPENDS dep)\n")
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(result EQUAL 0 OR NOT error MATCHES "DEPENDS setter has no test owner")
    message(FATAL_ERROR "empty owner list was not rejected:\n${output}${error}")
endif()
set(boundary_base [=[
add_executable(target exists.c)
add_test(NAME first COMMAND target)
add_test(NAME second COMMAND target)
not_set_property(TEST phantom PROPERTY DEPENDS missing)
myset_property(TEST first PROPERTY DEPENDS second)
not_set_tests_properties(phantom PROPERTIES DEPENDS missing)
myset_tests_properties(second PROPERTIES DEPENDS first)
]=])
file(WRITE "${fixture}/CMakeLists.txt" "${boundary_base}")
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0 OR NOT output MATCHES "Checked 0 literal CTest DEPENDS setters")
    message(FATAL_ERROR "custom command suffix counted as a setter:\n${output}${error}")
endif()
# Earlier custom calls contain identical setter suffixes; consume the full
# matched boundary so they cannot shift or duplicate the genuine assignment.
file(APPEND "${fixture}/CMakeLists.txt"
     "set_property(TEST first PROPERTY DEPENDS second)\nset_tests_properties(second PROPERTIES DEPENDS first)\n")
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0 OR NOT output MATCHES "Checked 2 literal CTest DEPENDS setters")
    message(FATAL_ERROR "real setters after identical custom suffixes lost count:\n${output}${error}")
endif()
foreach(setter IN ITEMS
        "set_property(TEST first PROPERTY DEPENDS missing)"
        "set_tests_properties(missing PROPERTIES DEPENDS first)")
    file(WRITE "${fixture}/CMakeLists.txt" "${boundary_base}${setter}\n")
    execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                            -P "${checker}"
                    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(result EQUAL 0 OR NOT error MATCHES "unregistered CTest name: missing")
        message(FATAL_ERROR "real dangling setter masked by custom suffix:\n${output}${error}")
    endif()
endforeach()
set(quoted_data_base [=[
add_executable(target exists.c)
add_test(NAME first COMMAND target)
add_test(NAME second COMMAND target)
set(blob "set_property(TEST first PROPERTY DEPENDS second)")
set(other "set_tests_properties(second PROPERTIES DEPENDS first)")
set(escaped "quoted \"set_property(TEST missing PROPERTY DEPENDS missing)\"")
]=])
file(WRITE "${fixture}/CMakeLists.txt" "${quoted_data_base}")
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0 OR NOT output MATCHES "Checked 0 literal CTest DEPENDS setters")
    message(FATAL_ERROR "quoted setter data invented dependencies:\n${output}${error}")
endif()
file(APPEND "${fixture}/CMakeLists.txt"
     "set_property(TEST first PROPERTY DEPENDS second)\nset_tests_properties(second PROPERTIES DEPENDS first)\n")
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0 OR NOT output MATCHES "Checked 2 literal CTest DEPENDS setters")
    message(FATAL_ERROR "real setters following quoted data lost count:\n${output}${error}")
endif()
foreach(setter IN ITEMS
        "set_property(TEST first PROPERTY DEPENDS missing)"
        "set_tests_properties(missing PROPERTIES DEPENDS first)")
    file(WRITE "${fixture}/CMakeLists.txt" "${quoted_data_base}${setter}\n")
    execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                            -P "${checker}"
                    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(result EQUAL 0 OR NOT error MATCHES "unregistered CTest name: missing")
        message(FATAL_ERROR "quoted data masked a real dangling setter:\n${output}${error}")
    endif()
endforeach()
set(bracket_base "add_executable(target exists.c)\nadd_test(NAME first COMMAND target)\nadd_test(NAME second COMMAND target)\n")
foreach(equals IN ITEMS "" "=" "==")
    set(bracket_data "set(blob [${equals}[ ) set_property(TEST first PROPERTY DEPENDS first) ]${equals}])\nset(other [${equals}[ ) set_tests_properties(missing PROPERTIES DEPENDS phantom) ]${equals}])\n")
    file(WRITE "${fixture}/CMakeLists.txt" "${bracket_base}${bracket_data}")
    execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                            -P "${checker}"
                    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(NOT result EQUAL 0 OR NOT output MATCHES "Checked 0 literal CTest DEPENDS setters")
        message(FATAL_ERROR "bracket setter data invented dependencies for '${equals}':\n${output}${error}")
    endif()
    file(APPEND "${fixture}/CMakeLists.txt"
         "set_property(TEST first PROPERTY DEPENDS second)\n")
    execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                            -P "${checker}"
                    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(NOT result EQUAL 0 OR NOT output MATCHES "Checked 1 literal CTest DEPENDS setters")
        message(FATAL_ERROR "real setter following bracket data lost count:\n${output}${error}")
    endif()
endforeach()
file(WRITE "${fixture}/CMakeLists.txt" "${bracket_base}"
     "set(blob [=[ ) set_property(TEST first PROPERTY DEPENDS first) ]=])\n"
     "set_property(TEST first PROPERTY DEPENDS missing)\n")
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(result EQUAL 0 OR NOT error MATCHES "DEPENDS reference: unregistered CTest name: missing")
    message(FATAL_ERROR "bracket data masked a real dangling reference:\n${output}${error}")
endif()
file(WRITE "${fixture}/CMakeLists.txt" "${bracket_base}"
     "set(blob [=[ ) set_property(TEST first PROPERTY DEPENDS first)\n")
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(result EQUAL 0 OR NOT error MATCHES "unterminated CMake bracket (argument|data)")
    message(FATAL_ERROR "unterminated bracket data not rejected:\n${output}${error}")
endif()
set(pair_base [=[
add_executable(target exists.c)
add_test(NAME first COMMAND target)
add_test(NAME second COMMAND target)
]=])
foreach(setter IN ITEMS
        "set_tests_properties(first PROPERTIES)"
        "set_tests_properties(first second PROPERTIES)"
        "set_tests_properties(first PROPERTIES DEPENDS)"
        "set_tests_properties(first PROPERTIES TIMEOUT 1 DEPENDS)")
    file(WRITE "${fixture}/CMakeLists.txt" "${pair_base}${setter}\n")
    execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                            -P "${checker}"
                    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(result EQUAL 0 OR NOT error MATCHES "set_tests_properties requires property/value pairs")
        message(FATAL_ERROR "missing or incomplete property pair not rejected:\n${output}${error}")
    endif()
endforeach()
foreach(properties IN ITEMS "DEPENDS \"\"" "TIMEOUT 1 DEPENDS \"\"" "DEPENDS second TIMEOUT 1")
    file(WRITE "${fixture}/CMakeLists.txt" "${pair_base}"
         "set_tests_properties(first PROPERTIES ${properties})\n")
    execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                            -P "${checker}"
                    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(NOT result EQUAL 0 OR NOT output MATCHES "Checked 1 literal CTest DEPENDS setters")
        message(FATAL_ERROR "valid complete property pairs or empty value rejected:\n${output}${error}")
    endif()
endforeach()
file(WRITE "${fixture}/CMakeLists.txt" "${pair_base}"
     "set_tests_properties(first PROPERTIES TIMEOUT 1)\n")
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                        -P "${checker}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0 OR NOT output MATCHES "Checked 0 literal CTest DEPENDS setters")
    message(FATAL_ERROR "valid non-DEPENDS property rejected or counted:\n${output}${error}")
endif()
set(retained_base [=[
add_executable(target exists.c)
add_test(NAME "first;part" COMMAND target)
add_test(NAME first COMMAND target)
add_test(NAME part COMMAND target)
add_test(NAME missing COMMAND target)
add_test(NAME consumer COMMAND target)
]=])
function(check_retained_reference name setter diagnostic)
    file(WRITE "${fixture}/CMakeLists.txt" "${retained_base}${setter}\n")
    execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                            -P "${checker}"
                    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(diagnostic STREQUAL "")
        if(NOT result EQUAL 0 OR NOT output MATCHES "Checked 5 CTest registrations and 5 source paths" OR
           NOT output MATCHES "Checked 1 literal CTest DEPENDS setters")
            message(FATAL_ERROR "${name} lost retained reference identity/count:\n${output}${error}")
        endif()
    elseif(result EQUAL 0 OR NOT error MATCHES "${diagnostic}")
        message(FATAL_ERROR "${name} did not reject the full dangling name:\n${output}${error}")
    endif()
endfunction()
check_retained_reference(retained_set_property [=[
set_property(TEST consumer PROPERTY DEPENDS first\\\;part)
]=] "")
check_retained_reference(retained_set_tests_properties [=[
set_tests_properties(consumer PROPERTIES DEPENDS first\\\;part)
]=] "")
check_retained_reference(retained_dangling_property [=[
set_property(TEST consumer PROPERTY DEPENDS missing\\\;part)
]=] "DEPENDS reference: unregistered CTest name: missing;part")
check_retained_reference(retained_dangling_properties [=[
set_tests_properties(consumer PROPERTIES DEPENDS missing\\\;part)
]=] "DEPENDS reference: unregistered CTest name: missing;part")
function(check_quoted_escape name setters diagnostic)
    file(WRITE "${fixture}/CMakeLists.txt" "${escape_base}${setters}\n")
    execute_process(COMMAND "${CMAKE_COMMAND}" "-DPOOLRAD_SOURCE_ROOT=${fixture}"
                            -P "${checker}"
                    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(diagnostic STREQUAL "")
        if(NOT result EQUAL 0 OR NOT output MATCHES "Checked 1 literal CTest DEPENDS setters")
            message(FATAL_ERROR "${name} valid escape rejected:\n${output}${error}")
        endif()
    elseif(result EQUAL 0 OR NOT error MATCHES "${diagnostic}")
        message(FATAL_ERROR "${name} lost escape diagnostic:\n${output}${error}")
    endif()
endfunction()
foreach(setter IN ITEMS set_property set_tests_properties)
    foreach(escape IN ITEMS q Q 0 9 t n r "@")
        if(escape MATCHES "^[tnr@]$")
            set(identity "fi\\${escape}rst")
            set(diagnostic "")
        else()
            # The slash-dropped alias exists, so rejection must be lexical.
            set(identity "fi${escape}rst")
            set(diagnostic "invalid alphanumeric escape in literal CTest property setter")
        endif()
        set(escape_base "add_executable(target exists.c)\nadd_test(NAME \"${identity}\" COMMAND target)\nadd_test(NAME consumer COMMAND target)\n")
        if(setter STREQUAL "set_property")
            set(declaration "set_property(TEST consumer PROPERTY DEPENDS \"fi\\${escape}rst\")")
        else()
            set(declaration "set_tests_properties(consumer PROPERTIES DEPENDS \"fi\\${escape}rst\")")
        endif()
        check_quoted_escape("${setter}_${escape}" "${declaration}" "${diagnostic}")
    endforeach()
endforeach()
set(escape_base "add_executable(target exists.c)\nadd_test(NAME fiqrst COMMAND target)\nadd_test(NAME consumer COMMAND target)\n")
check_quoted_escape(escaped_backslash [=[
set_property(TEST consumer PROPERTY DEPENDS "fi\\qrst")
]=] "DEPENDS reference: unregistered CTest name:")
check_quoted_escape(bracket_literal_data [==[
set_tests_properties(consumer PROPERTIES LABELS [=[fi\qrst]=] DEPENDS fiqrst)
]==] "")
foreach(setter IN ITEMS set_property set_tests_properties)
    foreach(escape IN ITEMS q Q 0 9 t n r "@")
        if(escape MATCHES "^[tnr@]$")
            set(identity "fi\\${escape}rst")
            set(diagnostic "")
        else()
            # Keep the slash-dropped alias registered to expose false passes.
            set(identity "fi${escape}rst")
            set(diagnostic "invalid unquoted alphanumeric escape in literal CTest property setter")
        endif()
        set(escape_base "add_executable(target exists.c)\nadd_test(NAME \"${identity}\" COMMAND target)\nadd_test(NAME consumer COMMAND target)\n")
        if(setter STREQUAL "set_property")
            set(declaration "set_property(TEST consumer PROPERTY DEPENDS fi\\${escape}rst)")
        else()
            set(declaration "set_tests_properties(consumer PROPERTIES DEPENDS fi\\${escape}rst)")
        endif()
        check_quoted_escape("unquoted_${setter}_${escape}" "${declaration}" "${diagnostic}")
    endforeach()
endforeach()
set(escape_base "add_executable(target exists.c)\nadd_test(NAME fiqrst COMMAND target)\nadd_test(NAME consumer COMMAND target)\n")
check_quoted_escape(unquoted_backslash_pair [=[
set_property(TEST consumer PROPERTY DEPENDS fi\\qrst)
]=] "DEPENDS reference: unregistered CTest name:")
check_quoted_escape(unquoted_punctuation_dangling [=[
set_tests_properties(consumer PROPERTIES DEPENDS missing\@rst)
]=] "DEPENDS reference: unregistered CTest name: missing@rst")
set(legacy_base [==[
add_executable(target exists.c)
add_test(NAME "fi\"q\"rst" COMMAND target)
add_test(NAME fiqrst COMMAND target)
add_test(NAME "fi\"q r\"st" COMMAND target)
add_test(NAME "fiq rst" COMMAND target)
add_test(NAME "fi\"q\"\"r\"st" COMMAND target)
add_test(NAME "fi@\"q\"rst" COMMAND target)
add_test(NAME fi@qrst COMMAND target)
add_test(NAME first COMMAND target)
add_test(NAME "\"fi\"qrst" COMMAND target)
add_test(NAME consumer COMMAND target)
]==])
set(legacy_full_owner [=["fi\"q\"rst"]=])
set(legacy_reference [=[fi"q"rst]=])
function(make_legacy_setter setter owner reference output)
    if(setter STREQUAL "set_property")
        set(${output} "set_property(TEST ${owner} PROPERTY DEPENDS ${reference})" PARENT_SCOPE)
    else()
        set(${output} "set_tests_properties(${owner} PROPERTIES DEPENDS ${reference})" PARENT_SCOPE)
    endif()
endfunction()
set(escape_base "${legacy_base}")
foreach(setter IN ITEMS set_property set_tests_properties)
    make_legacy_setter("${setter}" consumer "${legacy_reference}" declaration)
    check_quoted_escape("${setter}_legacy_reference" "${declaration}" "")
    make_legacy_setter("${setter}" "${legacy_reference}" consumer declaration)
    check_quoted_escape("${setter}_legacy_owner" "${declaration}" "")
endforeach()
make_legacy_setter(set_property consumer "${legacy_full_owner}" declaration)
check_quoted_escape(genuine_quoted_reference "${declaration}" "")
make_legacy_setter(set_property consumer [=[fi\"q\"rst]=] declaration)
check_quoted_escape(escaped_quote_reference "${declaration}" "")
make_legacy_setter(set_property consumer [=["first;fiqrst"]=] declaration)
check_quoted_escape(genuine_quoted_list "${declaration}" "")
foreach(reference IN ITEMS [=[fi"q;part"rst]=] [=[fi"q\;part"rst]=])
    make_legacy_setter(set_property consumer "${reference}" declaration)
    check_quoted_escape(legacy_semicolon_boundary "${declaration}"
        "unsupported semicolon in legacy unquoted DEPENDS argument")
endforeach()
# The quote-dropped alias exists, but the actual full name does not.
set(escape_base [=[
add_executable(target exists.c)
add_test(NAME fiqrst COMMAND target)
add_test(NAME consumer COMMAND target)
]=])
foreach(setter IN ITEMS set_property set_tests_properties)
    make_legacy_setter("${setter}" consumer "${legacy_reference}" declaration)
    check_quoted_escape("${setter}_legacy_dangling_reference" "${declaration}"
        [=[DEPENDS reference: unregistered CTest name: fi"q"rst]=])
    make_legacy_setter("${setter}" "${legacy_reference}" consumer declaration)
    check_quoted_escape("${setter}_legacy_missing_owner" "${declaration}"
        [=[DEPENDS owner: unregistered CTest name: fi"q"rst]=])
endforeach()
file(REMOVE_RECURSE "${fixture}")
message(STATUS "Semicolon DEPENDS identities preserved; full-name dangling/unsupported diagnostics checked")
