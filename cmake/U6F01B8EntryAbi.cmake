# Isolated bounded ABI composition; no changes to poolrad_recovered.
add_library(poolrad_u6f_01b8_entry_abi STATIC
    src/replay/u6f_01b8_entry_abi.c
    src/replay/u6f_02ae_abi.c
    src/replay/slot_a_string_0356_abi.c
    src/replay/system_cbc_abi.c
    src/replay/system_b98_abi.c
    src/recovered/slot_a_directory_fullstate.c)
target_include_directories(poolrad_u6f_01b8_entry_abi PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
if(BUILD_TESTING)
    add_executable(poolrad_u6f_01b8_entry_abi_tests
        tests/replay/test_u6f_01b8_entry_abi.c)
    target_link_libraries(poolrad_u6f_01b8_entry_abi_tests PRIVATE
        poolrad_u6f_01b8_entry_abi)
    add_test(NAME poolrad_u6f_01b8_entry_abi_tests
        COMMAND poolrad_u6f_01b8_entry_abi_tests)
endif()
