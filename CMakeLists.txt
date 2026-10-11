cmake_minimum_required(VERSION 3.16)
project(poolrad_formats C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(poolrad_formats STATIC src/formats/dax.c src/formats/geo.c
                                   src/formats/ecl.c src/formats/ega.c
                                   src/formats/walldef.c
                                   src/formats/dos_character.c)
target_include_directories(poolrad_formats PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_library(poolrad_recovered STATIC src/recovered/ecl_helpers.c
                                    src/recovered/resident_sidecar_remove.c
                                    src/recovered/resident_sidecar_append.c
                                    src/recovered/resident_sidecar_lookup.c
                                    src/recovered/resident_party_count.c
                                    src/recovered/resident_character_field_zero.c
                                    src/recovered/resident_character_link_check.c
                                    src/recovered/resident_character_link_flags.c
                                    src/recovered/resident_464_bitmap_release.c
                                    src/recovered/overlay_unit38_effects.c)
target_include_directories(poolrad_recovered PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(poolrad_formats PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(poolrad_recovered PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(poolrad_format_audit tests/formats/audit.c)
target_link_libraries(poolrad_format_audit PRIVATE poolrad_formats)
add_executable(poolrad_ecl_audit tests/formats/ecl_audit.c)
target_link_libraries(poolrad_ecl_audit PRIVATE poolrad_formats)
add_executable(poolrad_ega_audit tests/formats/ega_audit.c)
target_link_libraries(poolrad_ega_audit PRIVATE poolrad_formats)
add_executable(poolrad_walldef_audit tests/formats/walldef_audit.c)
target_link_libraries(poolrad_walldef_audit PRIVATE poolrad_formats)
add_executable(poolrad_dos_character_audit tests/formats/dos_character_audit.c)
target_link_libraries(poolrad_dos_character_audit PRIVATE poolrad_formats)

include(CTest)
if(BUILD_TESTING)
    add_executable(poolrad_format_tests tests/formats/test_formats.c)
    target_link_libraries(poolrad_format_tests PRIVATE poolrad_formats)
    add_test(NAME poolrad_format_tests COMMAND poolrad_format_tests)
    add_executable(poolrad_ecl_tests tests/formats/test_ecl.c)
    target_link_libraries(poolrad_ecl_tests PRIVATE poolrad_formats)
    add_test(NAME poolrad_ecl_tests COMMAND poolrad_ecl_tests)
    add_executable(poolrad_ega_tests tests/formats/test_ega.c)
    target_link_libraries(poolrad_ega_tests PRIVATE poolrad_formats)
    add_test(NAME poolrad_ega_tests COMMAND poolrad_ega_tests)
    add_executable(poolrad_walldef_tests tests/formats/test_walldef.c)
    target_link_libraries(poolrad_walldef_tests PRIVATE poolrad_formats)
    add_test(NAME poolrad_walldef_tests COMMAND poolrad_walldef_tests)
    add_executable(poolrad_dos_character_tests tests/formats/test_dos_character.c)
    target_link_libraries(poolrad_dos_character_tests PRIVATE poolrad_formats)
    add_test(NAME poolrad_dos_character_tests COMMAND poolrad_dos_character_tests)
    add_executable(poolrad_recovered_tests tests/recovered/test_ecl_helpers.c)
    target_link_libraries(poolrad_recovered_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_recovered_tests COMMAND poolrad_recovered_tests)
    add_executable(poolrad_resident_remove_tests tests/recovered/test_resident_sidecar_remove.c)
    target_link_libraries(poolrad_resident_remove_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_resident_remove_tests COMMAND poolrad_resident_remove_tests)
    add_executable(poolrad_resident_append_tests tests/recovered/test_resident_sidecar_append.c)
    target_link_libraries(poolrad_resident_append_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_resident_append_tests COMMAND poolrad_resident_append_tests)
    add_executable(poolrad_resident_lookup_tests tests/recovered/test_resident_sidecar_lookup.c)
    target_link_libraries(poolrad_resident_lookup_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_resident_lookup_tests COMMAND poolrad_resident_lookup_tests)
    add_executable(poolrad_resident_party_count_tests tests/recovered/test_resident_party_count.c)
    target_link_libraries(poolrad_resident_party_count_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_resident_party_count_tests COMMAND poolrad_resident_party_count_tests)
    add_executable(poolrad_resident_character_field_zero_tests
                   tests/recovered/test_resident_character_field_zero.c)
    target_link_libraries(poolrad_resident_character_field_zero_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_resident_character_field_zero_tests
             COMMAND poolrad_resident_character_field_zero_tests)
    add_executable(poolrad_resident_character_link_check_tests
                   tests/recovered/test_resident_character_link_check.c)
    target_link_libraries(poolrad_resident_character_link_check_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_resident_character_link_check_tests
             COMMAND poolrad_resident_character_link_check_tests)
    add_executable(poolrad_resident_character_link_flags_tests
                   tests/recovered/test_resident_character_link_flags.c)
    target_link_libraries(poolrad_resident_character_link_flags_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_resident_character_link_flags_tests
             COMMAND poolrad_resident_character_link_flags_tests)
    add_executable(poolrad_resident_464_bitmap_release_tests
                   tests/recovered/test_resident_464_bitmap_release.c)
    target_link_libraries(poolrad_resident_464_bitmap_release_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_resident_464_bitmap_release_tests
             COMMAND poolrad_resident_464_bitmap_release_tests)
    add_executable(poolrad_overlay_unit38_effects_tests
                   tests/recovered/test_overlay_unit38_effects.c)
    target_link_libraries(poolrad_overlay_unit38_effects_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_unit38_effects_tests
             COMMAND poolrad_overlay_unit38_effects_tests)
endif()
