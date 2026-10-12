cmake_minimum_required(VERSION 3.16)
project(poolrad_formats C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(poolrad_formats STATIC src/formats/dax.c src/formats/geo.c
                                   src/formats/ecl.c src/formats/ega.c
                                   src/formats/walldef.c
                                   src/formats/dos_character.c
                                   src/formats/dos_savgam.c)
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
                                    src/recovered/overlay_unit38_effects.c
                                    src/recovered/resident_character_target_reset.c
                                    src/recovered/overlay_unit38_effects_next.c
                                    src/recovered/resident_character_target_select.c
                                    src/recovered/overlay_unit38_helper.c
                                    src/recovered/resident_ecl_dax_block.c
                                    src/recovered/resident_dax_rle.c
                                    src/recovered/overlay_unit38_walk.c
                                    src/recovered/resident_dax_file_open.c
                                    src/recovered/resident_dax_nested_open.c
                                    src/recovered/overlay_ecl_store_hook.c
                                    src/recovered/resident_dax_retry_display.c
                                    src/recovered/resident_dax_selector_02eb.c
                                    src/recovered/resident_dax_selector_push.c
                                    src/recovered/overlay_u3bd_pieces.c
                                    src/recovered/resident_load_route_27d9.c
                                    src/recovered/resident_load_pieces_0767.c
                                    src/recovered/overlay_u3bd_wallset.c
                                    src/recovered/overlay_u3bd_cell.c
                                    src/recovered/overlay_u3bd_nibble.c
                                    src/recovered/overlay_u3bd_field.c
                                    src/recovered/overlay_u7f_div5.c
                                    src/recovered/overlay_u7f_div5_dwords.c
                                    src/recovered/resident_route_290b.c
                                    src/recovered/resident_field_08ef.c)
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
add_executable(poolrad_resident_ecl_dax_block_audit
               tests/recovered/audit_resident_ecl_dax_block.c)
target_link_libraries(poolrad_resident_ecl_dax_block_audit PRIVATE
                      poolrad_recovered poolrad_formats)
add_executable(poolrad_resident_dax_rle_audit
               tests/recovered/audit_resident_dax_rle.c)
target_link_libraries(poolrad_resident_dax_rle_audit PRIVATE
                      poolrad_recovered poolrad_formats)

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
    add_executable(poolrad_dos_savgam_tests tests/formats/test_dos_savgam.c)
    target_link_libraries(poolrad_dos_savgam_tests PRIVATE poolrad_formats)
    add_test(NAME poolrad_dos_savgam_tests COMMAND poolrad_dos_savgam_tests)
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
    add_executable(poolrad_resident_character_target_reset_tests
                   tests/recovered/test_resident_character_target_reset.c)
    target_link_libraries(poolrad_resident_character_target_reset_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_resident_character_target_reset_tests
             COMMAND poolrad_resident_character_target_reset_tests)
    add_executable(poolrad_overlay_unit38_effects_next_tests
                   tests/recovered/test_overlay_unit38_effects_next.c)
    target_link_libraries(poolrad_overlay_unit38_effects_next_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_unit38_effects_next_tests
             COMMAND poolrad_overlay_unit38_effects_next_tests)
    add_executable(poolrad_resident_character_target_select_tests
                   tests/recovered/test_resident_character_target_select.c)
    target_link_libraries(poolrad_resident_character_target_select_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_resident_character_target_select_tests
             COMMAND poolrad_resident_character_target_select_tests)
    add_executable(poolrad_overlay_unit38_helper_tests
                   tests/recovered/test_overlay_unit38_helper.c)
    target_link_libraries(poolrad_overlay_unit38_helper_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_unit38_helper_tests
             COMMAND poolrad_overlay_unit38_helper_tests)
    add_executable(poolrad_resident_ecl_dax_block_tests
                   tests/recovered/test_resident_ecl_dax_block.c)
    target_link_libraries(poolrad_resident_ecl_dax_block_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_resident_ecl_dax_block_tests
             COMMAND poolrad_resident_ecl_dax_block_tests)
    add_executable(poolrad_resident_dax_rle_tests
                   tests/recovered/test_resident_dax_rle.c)
    target_link_libraries(poolrad_resident_dax_rle_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_resident_dax_rle_tests
             COMMAND poolrad_resident_dax_rle_tests)
    add_executable(poolrad_overlay_unit38_walk_tests
                   tests/recovered/test_overlay_unit38_walk.c)
    target_link_libraries(poolrad_overlay_unit38_walk_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_unit38_walk_tests
             COMMAND poolrad_overlay_unit38_walk_tests)
    add_executable(poolrad_resident_dax_file_open_tests
                   tests/recovered/test_resident_dax_file_open.c)
    target_link_libraries(poolrad_resident_dax_file_open_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_resident_dax_file_open_tests
             COMMAND poolrad_resident_dax_file_open_tests)
    add_executable(poolrad_resident_dax_nested_open_tests
                   tests/recovered/test_resident_dax_nested_open.c)
    target_link_libraries(poolrad_resident_dax_nested_open_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_resident_dax_nested_open_tests
             COMMAND poolrad_resident_dax_nested_open_tests)
    add_executable(poolrad_overlay_ecl_store_hook_tests
                   tests/recovered/test_overlay_ecl_store_hook.c)
    target_link_libraries(poolrad_overlay_ecl_store_hook_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_ecl_store_hook_tests
             COMMAND poolrad_overlay_ecl_store_hook_tests)
    add_executable(poolrad_resident_dax_retry_display_tests
                   tests/recovered/test_resident_dax_retry_display.c)
    target_link_libraries(poolrad_resident_dax_retry_display_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_resident_dax_retry_display_tests
             COMMAND poolrad_resident_dax_retry_display_tests)
    add_executable(poolrad_resident_dax_selector_02eb_tests
                   tests/recovered/test_resident_dax_selector_02eb.c)
    target_link_libraries(poolrad_resident_dax_selector_02eb_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_resident_dax_selector_02eb_tests
             COMMAND poolrad_resident_dax_selector_02eb_tests)
    add_executable(poolrad_resident_dax_selector_push_tests
                   tests/recovered/test_resident_dax_selector_push.c)
    target_link_libraries(poolrad_resident_dax_selector_push_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_resident_dax_selector_push_tests
             COMMAND poolrad_resident_dax_selector_push_tests)
    add_executable(poolrad_overlay_u3bd_pieces_tests
                   tests/recovered/test_overlay_u3bd_pieces.c)
    target_link_libraries(poolrad_overlay_u3bd_pieces_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3bd_pieces_tests
             COMMAND poolrad_overlay_u3bd_pieces_tests)
    add_executable(poolrad_resident_load_route_27d9_tests
                   tests/recovered/test_resident_load_route_27d9.c)
    target_link_libraries(poolrad_resident_load_route_27d9_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_resident_load_route_27d9_tests
             COMMAND poolrad_resident_load_route_27d9_tests)
    add_executable(poolrad_resident_load_pieces_0767_tests
                   tests/recovered/test_resident_load_pieces_0767.c)
    target_link_libraries(poolrad_resident_load_pieces_0767_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_resident_load_pieces_0767_tests
             COMMAND poolrad_resident_load_pieces_0767_tests)
    add_executable(poolrad_overlay_u3bd_wallset_tests
                   tests/recovered/test_overlay_u3bd_wallset.c)
    target_link_libraries(poolrad_overlay_u3bd_wallset_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3bd_wallset_tests
             COMMAND poolrad_overlay_u3bd_wallset_tests)
    add_executable(poolrad_overlay_u3bd_cell_tests
                   tests/recovered/test_overlay_u3bd_cell.c)
    target_link_libraries(poolrad_overlay_u3bd_cell_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3bd_cell_tests
             COMMAND poolrad_overlay_u3bd_cell_tests)
    add_executable(poolrad_overlay_u3bd_nibble_tests
                   tests/recovered/test_overlay_u3bd_nibble.c)
    target_link_libraries(poolrad_overlay_u3bd_nibble_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3bd_nibble_tests
             COMMAND poolrad_overlay_u3bd_nibble_tests)
    add_executable(poolrad_overlay_u3bd_field_tests
                   tests/recovered/test_overlay_u3bd_field.c)
    target_link_libraries(poolrad_overlay_u3bd_field_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3bd_field_tests
             COMMAND poolrad_overlay_u3bd_field_tests)
    add_executable(poolrad_overlay_u7f_div5_tests
                   tests/recovered/test_overlay_u7f_div5.c)
    target_link_libraries(poolrad_overlay_u7f_div5_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u7f_div5_tests
             COMMAND poolrad_overlay_u7f_div5_tests)
    add_executable(poolrad_overlay_u7f_div5_dwords_tests
                   tests/recovered/test_overlay_u7f_div5_dwords.c)
    target_link_libraries(poolrad_overlay_u7f_div5_dwords_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u7f_div5_dwords_tests
             COMMAND poolrad_overlay_u7f_div5_dwords_tests)
    add_executable(poolrad_resident_route_290b_tests
                   tests/recovered/test_resident_route_290b.c)
    target_link_libraries(poolrad_resident_route_290b_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_resident_route_290b_tests
             COMMAND poolrad_resident_route_290b_tests)
    add_executable(poolrad_resident_field_08ef_tests
                   tests/recovered/test_resident_field_08ef.c)
    target_link_libraries(poolrad_resident_field_08ef_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_resident_field_08ef_tests
             COMMAND poolrad_resident_field_08ef_tests)
endif()
