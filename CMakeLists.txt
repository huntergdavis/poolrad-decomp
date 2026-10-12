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
                                    src/recovered/resident_464_block_wrapper.c
                                    src/recovered/resident_464_bitmap_constructor.c
                                    src/recovered/resident_464_bitmask.c
                                    src/recovered/resident_464_nibble_007b.c
                                    src/recovered/resident_464_mode2.c
                                    src/recovered/resident_464_mode1.c
                                    src/recovered/resident_464_mode0_prefix.c
                                    src/recovered/resident_464_mode0_primary.c
                                    src/recovered/resident_464_mode0_second.c
                                    src/recovered/resident_464_mode0_loops.c
                                    src/recovered/resident_464_mode0_single.c
                                    src/recovered/resident_464_mode0_two_y.c
                                    src/recovered/resident_464_mode0_iterator.c
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
                                    src/recovered/overlay_u25_move.c
                                    src/recovered/overlay_u7f_div5.c
                                    src/recovered/overlay_u7f_div5_dwords.c
                                    src/recovered/overlay_u68_up_preflight.c
                                    src/recovered/overlay_u68_coordinate_writer.c
                                    src/recovered/overlay_u68_entry1.c
                                    src/recovered/overlay_u68_party_byte_0236.c
                                    src/recovered/overlay_u68_field_clear_0000.c
                                    src/recovered/overlay_u68_roll_gate_0294.c
                                    src/recovered/overlay_u68_tile_step_0818.c
                                    src/recovered/overlay_u1c_loop_3387.c
                                    src/recovered/overlay_u1c_and_or_0def.c
                                    src/recovered/overlay_u1c_picture_0856.c
                                    src/recovered/overlay_u1c_print_0a09.c
                                    src/recovered/overlay_u1c_on_goto_18f8.c
                                    src/recovered/overlay_u1c_party_strength_12c6.c
                                    src/recovered/overlay_u1c_random_0234.c
                                    src/recovered/overlay_u6f_party_load.c
                                    src/recovered/overlay_u9b_record_incr_01af.c
                                    src/recovered/overlay_u9b_record_table_0255.c
                                    src/recovered/overlay_u9b_record_fields_0329.c
                                    src/recovered/overlay_u9b_entry_zero_0000.c
                                    src/recovered/overlay_u5e_party_value_2879.c
                                    src/recovered/overlay_u1c_arith_01a3.c
                                    src/recovered/overlay_u1c_gettable_0e5e.c
                                    src/recovered/overlay_u1c_party_surprise_1629.c
                                    src/recovered/overlay_u1c_surprise_16c9.c
                                    src/recovered/overlay_u1c_setup_monster_03b4.c
                                    src/recovered/overlay_u1c_approach_0817.c
                                    src/recovered/overlay_u1c_delay_271f.c
                                    src/recovered/overlay_u1c_horizontal_menu_10d1.c
                                    src/recovered/overlay_u1c_clearmonsters_125d.c
                                    src/recovered/overlay_u25_entry5_0411.c
                                    src/recovered/overlay_u1c_load_monster_prefix_0450.c
                                    src/recovered/overlay_u1c_load_monster_04af.c
                                    src/recovered/overlay_u3ce_entry4_01c8.c
                                    src/recovered/overlay_u1c_load_monster_tail_0537.c
                                    src/recovered/overlay_u6f_entry6_0ec4.c
                                    src/recovered/overlay_u6f_reader_adapter.c
                                    src/recovered/overlay_u1c_exit_0001.c
                                    src/recovered/overlay_u68_record_scan.c
                                    src/recovered/overlay_u68_list_search.c
                                    src/recovered/overlay_u68_record_clear.c
                                    src/recovered/overlay_u68_movement_flag.c
                                    src/recovered/overlay_u68_map_update.c
                                    src/recovered/overlay_u68_entry2_up.c
                                    src/recovered/overlay_u7d_entry1.c
                                    src/recovered/overlay_u7d_zero.c
                                    src/recovered/overlay_u9e_entry2.c
                                    src/recovered/overlay_u3c6_slot.c
                                    src/recovered/overlay_u9e_0160.c
                                    src/recovered/overlay_u9e_entry8_roll.c
                                    src/recovered/resident_route_290b.c
                                    src/recovered/resident_field_08ef.c
                                    src/recovered/resident_field_0973.c
                                    src/recovered/resident_string_0356.c
                                    src/recovered/resident_string_0425.c
                                    src/recovered/overlay_u6f_party_string_chain.c
                                    src/recovered/overlay_u6f_party_filename.c
                                    src/recovered/overlay_u6f_postinsert_02ae.c
                                    src/recovered/slot_a_directory_fullstate.c
                                    src/recovered/slot_a_u6f_02da_decision.c
                                    src/recovered/slot_a_u6f_031f_return.c
                                    src/recovered/overlay_u6f_party_insert_1446.c
                                    src/recovered/overlay_u6f_party_insert_1446_dos.c
                                    src/recovered/overlay_u6f_insert_ss_probe.c
                                    src/recovered/overlay_u6f_party_insert_1446_ss.c
                                    src/recovered/resident_handler_prefix_02e6.c
                                    src/recovered/resident_loader_prefix_0338.c
                                    src/recovered/resident_loader_0553.c
                                    src/recovered/resident_loader_0538.c
                                    src/recovered/resident_loader_0366.c
                                    src/recovered/resident_loader_0268.c
                                    src/recovered/resident_loader_0355.c
                                    src/recovered/resident_8ae_dispatch.c
                                    src/recovered/resident_8ae_table_17ae.c
                                    src/recovered/resident_8ae_table_1742.c)
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
    add_executable(poolrad_resident_464_block_wrapper_tests
                   tests/recovered/test_resident_464_block_wrapper.c)
    target_link_libraries(poolrad_resident_464_block_wrapper_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_resident_464_block_wrapper_tests
             COMMAND poolrad_resident_464_block_wrapper_tests)
    add_executable(poolrad_resident_464_bitmap_constructor_tests
                   tests/recovered/test_resident_464_bitmap_constructor.c)
    target_link_libraries(poolrad_resident_464_bitmap_constructor_tests
                          PRIVATE poolrad_recovered)
    add_test(NAME poolrad_resident_464_bitmap_constructor_tests
             COMMAND poolrad_resident_464_bitmap_constructor_tests)
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
    add_executable(poolrad_overlay_u25_move_tests
                   tests/recovered/test_overlay_u25_move.c)
    target_link_libraries(poolrad_overlay_u25_move_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u25_move_tests
             COMMAND poolrad_overlay_u25_move_tests)
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
    add_executable(poolrad_overlay_u68_up_preflight_tests
                   tests/recovered/test_overlay_u68_up_preflight.c)
    target_link_libraries(poolrad_overlay_u68_up_preflight_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u68_up_preflight_tests
             COMMAND poolrad_overlay_u68_up_preflight_tests)
    add_executable(poolrad_overlay_u68_coordinate_writer_tests
                   tests/recovered/test_overlay_u68_coordinate_writer.c)
    target_link_libraries(poolrad_overlay_u68_coordinate_writer_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u68_coordinate_writer_tests
             COMMAND poolrad_overlay_u68_coordinate_writer_tests)
    add_executable(poolrad_overlay_u68_entry1_tests
                   tests/recovered/test_overlay_u68_entry1.c)
    target_link_libraries(poolrad_overlay_u68_entry1_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u68_entry1_tests
             COMMAND poolrad_overlay_u68_entry1_tests)
    add_executable(poolrad_overlay_u68_party_byte_0236_tests
                   tests/recovered/test_overlay_u68_party_byte_0236.c)
    target_link_libraries(poolrad_overlay_u68_party_byte_0236_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u68_party_byte_0236_tests
             COMMAND poolrad_overlay_u68_party_byte_0236_tests)
    add_executable(poolrad_overlay_u68_field_clear_0000_tests
                   tests/recovered/test_overlay_u68_field_clear_0000.c)
    target_link_libraries(poolrad_overlay_u68_field_clear_0000_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u68_field_clear_0000_tests
             COMMAND poolrad_overlay_u68_field_clear_0000_tests)
    add_executable(poolrad_overlay_u68_roll_gate_0294_tests
                   tests/recovered/test_overlay_u68_roll_gate_0294.c)
    target_link_libraries(poolrad_overlay_u68_roll_gate_0294_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u68_roll_gate_0294_tests
             COMMAND poolrad_overlay_u68_roll_gate_0294_tests)
    add_executable(poolrad_overlay_u68_tile_step_0818_tests
                   tests/recovered/test_overlay_u68_tile_step_0818.c)
    target_link_libraries(poolrad_overlay_u68_tile_step_0818_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u68_tile_step_0818_tests
             COMMAND poolrad_overlay_u68_tile_step_0818_tests)
    add_executable(poolrad_overlay_u1c_loop_3387_tests
                   tests/recovered/test_overlay_u1c_loop_3387.c)
    target_link_libraries(poolrad_overlay_u1c_loop_3387_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u1c_loop_3387_tests
             COMMAND poolrad_overlay_u1c_loop_3387_tests)
    add_executable(poolrad_overlay_u1c_and_or_0def_tests
                   tests/recovered/test_overlay_u1c_and_or_0def.c)
    target_link_libraries(poolrad_overlay_u1c_and_or_0def_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u1c_and_or_0def_tests
             COMMAND poolrad_overlay_u1c_and_or_0def_tests)
    add_executable(poolrad_overlay_u1c_picture_0856_tests
                   tests/recovered/test_overlay_u1c_picture_0856.c)
    target_link_libraries(poolrad_overlay_u1c_picture_0856_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u1c_picture_0856_tests
             COMMAND poolrad_overlay_u1c_picture_0856_tests)
    add_executable(poolrad_overlay_u1c_print_0a09_tests
                   tests/recovered/test_overlay_u1c_print_0a09.c)
    target_link_libraries(poolrad_overlay_u1c_print_0a09_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u1c_print_0a09_tests
             COMMAND poolrad_overlay_u1c_print_0a09_tests)
    add_executable(poolrad_overlay_u1c_on_goto_18f8_tests
                   tests/recovered/test_overlay_u1c_on_goto_18f8.c)
    target_link_libraries(poolrad_overlay_u1c_on_goto_18f8_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u1c_on_goto_18f8_tests
             COMMAND poolrad_overlay_u1c_on_goto_18f8_tests)
    add_executable(poolrad_overlay_u1c_party_strength_12c6_tests
                   tests/recovered/test_overlay_u1c_party_strength_12c6.c)
    target_link_libraries(poolrad_overlay_u1c_party_strength_12c6_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u1c_party_strength_12c6_tests
             COMMAND poolrad_overlay_u1c_party_strength_12c6_tests)
    add_executable(poolrad_overlay_u1c_random_0234_tests
                   tests/recovered/test_overlay_u1c_random_0234.c)
    target_link_libraries(poolrad_overlay_u1c_random_0234_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u1c_random_0234_tests
             COMMAND poolrad_overlay_u1c_random_0234_tests)
    add_executable(poolrad_overlay_u6f_party_load_tests
                   tests/recovered/test_overlay_u6f_party_load.c)
    target_link_libraries(poolrad_overlay_u6f_party_load_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u6f_party_load_tests
             COMMAND poolrad_overlay_u6f_party_load_tests)
    add_executable(poolrad_overlay_u9b_record_incr_01af_tests
                   tests/recovered/test_overlay_u9b_record_incr_01af.c)
    target_link_libraries(poolrad_overlay_u9b_record_incr_01af_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u9b_record_incr_01af_tests
             COMMAND poolrad_overlay_u9b_record_incr_01af_tests)
    add_executable(poolrad_overlay_u9b_record_table_0255_tests
                   tests/recovered/test_overlay_u9b_record_table_0255.c)
    target_link_libraries(poolrad_overlay_u9b_record_table_0255_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u9b_record_table_0255_tests
             COMMAND poolrad_overlay_u9b_record_table_0255_tests)
    add_executable(poolrad_overlay_u9b_record_fields_0329_tests
                   tests/recovered/test_overlay_u9b_record_fields_0329.c)
    target_link_libraries(poolrad_overlay_u9b_record_fields_0329_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u9b_record_fields_0329_tests
             COMMAND poolrad_overlay_u9b_record_fields_0329_tests)
    add_executable(poolrad_overlay_u9b_entry_zero_0000_tests
                   tests/recovered/test_overlay_u9b_entry_zero_0000.c)
    target_link_libraries(poolrad_overlay_u9b_entry_zero_0000_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u9b_entry_zero_0000_tests
             COMMAND poolrad_overlay_u9b_entry_zero_0000_tests)
    add_executable(poolrad_overlay_u5e_party_value_2879_tests
                   tests/recovered/test_overlay_u5e_party_value_2879.c)
    target_link_libraries(poolrad_overlay_u5e_party_value_2879_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u5e_party_value_2879_tests
             COMMAND poolrad_overlay_u5e_party_value_2879_tests)
    add_executable(poolrad_overlay_u1c_arith_01a3_tests
                   tests/recovered/test_overlay_u1c_arith_01a3.c)
    target_link_libraries(poolrad_overlay_u1c_arith_01a3_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u1c_arith_01a3_tests
             COMMAND poolrad_overlay_u1c_arith_01a3_tests)
    add_executable(poolrad_overlay_u1c_gettable_0e5e_tests
                   tests/recovered/test_overlay_u1c_gettable_0e5e.c)
    target_link_libraries(poolrad_overlay_u1c_gettable_0e5e_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u1c_gettable_0e5e_tests
             COMMAND poolrad_overlay_u1c_gettable_0e5e_tests)
    add_executable(poolrad_overlay_u1c_party_surprise_1629_tests
                   tests/recovered/test_overlay_u1c_party_surprise_1629.c)
    target_link_libraries(poolrad_overlay_u1c_party_surprise_1629_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u1c_party_surprise_1629_tests
             COMMAND poolrad_overlay_u1c_party_surprise_1629_tests)
    add_executable(poolrad_overlay_u1c_surprise_16c9_tests
                   tests/recovered/test_overlay_u1c_surprise_16c9.c)
    target_link_libraries(poolrad_overlay_u1c_surprise_16c9_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u1c_surprise_16c9_tests
             COMMAND poolrad_overlay_u1c_surprise_16c9_tests)
    add_executable(poolrad_overlay_u1c_setup_monster_03b4_tests
                   tests/recovered/test_overlay_u1c_setup_monster_03b4.c)
    target_link_libraries(poolrad_overlay_u1c_setup_monster_03b4_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u1c_setup_monster_03b4_tests
             COMMAND poolrad_overlay_u1c_setup_monster_03b4_tests)
    add_executable(poolrad_overlay_u1c_approach_0817_tests
                   tests/recovered/test_overlay_u1c_approach_0817.c)
    target_link_libraries(poolrad_overlay_u1c_approach_0817_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u1c_approach_0817_tests
             COMMAND poolrad_overlay_u1c_approach_0817_tests)
    add_executable(poolrad_overlay_u1c_delay_271f_tests
                   tests/recovered/test_overlay_u1c_delay_271f.c)
    target_link_libraries(poolrad_overlay_u1c_delay_271f_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u1c_delay_271f_tests
             COMMAND poolrad_overlay_u1c_delay_271f_tests)
    add_executable(poolrad_overlay_u1c_horizontal_menu_10d1_tests
                   tests/recovered/test_overlay_u1c_horizontal_menu_10d1.c)
    target_link_libraries(poolrad_overlay_u1c_horizontal_menu_10d1_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u1c_horizontal_menu_10d1_tests
             COMMAND poolrad_overlay_u1c_horizontal_menu_10d1_tests)
    add_executable(poolrad_overlay_u1c_clearmonsters_125d_tests
                   tests/recovered/test_overlay_u1c_clearmonsters_125d.c)
    target_link_libraries(poolrad_overlay_u1c_clearmonsters_125d_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u1c_clearmonsters_125d_tests
             COMMAND poolrad_overlay_u1c_clearmonsters_125d_tests)
    add_executable(poolrad_overlay_u25_entry5_0411_tests
                   tests/recovered/test_overlay_u25_entry5_0411.c)
    target_link_libraries(poolrad_overlay_u25_entry5_0411_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u25_entry5_0411_tests
             COMMAND poolrad_overlay_u25_entry5_0411_tests)
    add_executable(poolrad_overlay_u1c_load_monster_prefix_0450_tests
                   tests/recovered/test_overlay_u1c_load_monster_prefix_0450.c)
    target_link_libraries(poolrad_overlay_u1c_load_monster_prefix_0450_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u1c_load_monster_prefix_0450_tests
             COMMAND poolrad_overlay_u1c_load_monster_prefix_0450_tests)
    add_executable(poolrad_overlay_u1c_load_monster_04af_tests
                   tests/recovered/test_overlay_u1c_load_monster_04af.c)
    target_link_libraries(poolrad_overlay_u1c_load_monster_04af_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u1c_load_monster_04af_tests
             COMMAND poolrad_overlay_u1c_load_monster_04af_tests)
    add_executable(poolrad_overlay_u3ce_entry4_01c8_tests
                   tests/recovered/test_overlay_u3ce_entry4_01c8.c)
    target_link_libraries(poolrad_overlay_u3ce_entry4_01c8_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3ce_entry4_01c8_tests
             COMMAND poolrad_overlay_u3ce_entry4_01c8_tests)
    add_executable(poolrad_overlay_u1c_load_monster_tail_0537_tests
                   tests/recovered/test_overlay_u1c_load_monster_tail_0537.c)
    target_link_libraries(poolrad_overlay_u1c_load_monster_tail_0537_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u1c_load_monster_tail_0537_tests
             COMMAND poolrad_overlay_u1c_load_monster_tail_0537_tests)
    add_executable(poolrad_overlay_u6f_entry6_0ec4_tests
                   tests/recovered/test_overlay_u6f_entry6_0ec4.c)
    target_link_libraries(poolrad_overlay_u6f_entry6_0ec4_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u6f_entry6_0ec4_tests
             COMMAND poolrad_overlay_u6f_entry6_0ec4_tests)
    add_executable(poolrad_overlay_u6f_reader_adapter_tests
                   tests/recovered/test_overlay_u6f_reader_adapter.c)
    target_link_libraries(poolrad_overlay_u6f_reader_adapter_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u6f_reader_adapter_tests
             COMMAND poolrad_overlay_u6f_reader_adapter_tests)
    add_executable(poolrad_overlay_u1c_exit_0001_tests
                   tests/recovered/test_overlay_u1c_exit_0001.c)
    target_link_libraries(poolrad_overlay_u1c_exit_0001_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u1c_exit_0001_tests
             COMMAND poolrad_overlay_u1c_exit_0001_tests)
    add_executable(poolrad_overlay_u68_record_scan_tests
                   tests/recovered/test_overlay_u68_record_scan.c)
    target_link_libraries(poolrad_overlay_u68_record_scan_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u68_record_scan_tests
             COMMAND poolrad_overlay_u68_record_scan_tests)
    add_executable(poolrad_overlay_u68_list_search_tests
                   tests/recovered/test_overlay_u68_list_search.c)
    target_link_libraries(poolrad_overlay_u68_list_search_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u68_list_search_tests
             COMMAND poolrad_overlay_u68_list_search_tests)
    add_executable(poolrad_overlay_u68_record_clear_tests
                   tests/recovered/test_overlay_u68_record_clear.c)
    target_link_libraries(poolrad_overlay_u68_record_clear_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u68_record_clear_tests
             COMMAND poolrad_overlay_u68_record_clear_tests)
    add_executable(poolrad_overlay_u68_movement_flag_tests
                   tests/recovered/test_overlay_u68_movement_flag.c)
    target_link_libraries(poolrad_overlay_u68_movement_flag_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u68_movement_flag_tests
             COMMAND poolrad_overlay_u68_movement_flag_tests)
    add_executable(poolrad_overlay_u68_map_update_tests
                   tests/recovered/test_overlay_u68_map_update.c)
    target_link_libraries(poolrad_overlay_u68_map_update_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u68_map_update_tests
             COMMAND poolrad_overlay_u68_map_update_tests)
    add_executable(poolrad_overlay_u68_entry2_up_tests
                   tests/recovered/test_overlay_u68_entry2_up.c)
    target_link_libraries(poolrad_overlay_u68_entry2_up_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u68_entry2_up_tests
             COMMAND poolrad_overlay_u68_entry2_up_tests)
    add_executable(poolrad_overlay_u7d_entry1_tests
                   tests/recovered/test_overlay_u7d_entry1.c)
    target_link_libraries(poolrad_overlay_u7d_entry1_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u7d_entry1_tests
             COMMAND poolrad_overlay_u7d_entry1_tests)
    add_executable(poolrad_overlay_u7d_zero_tests
                   tests/recovered/test_overlay_u7d_zero.c)
    target_link_libraries(poolrad_overlay_u7d_zero_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u7d_zero_tests
             COMMAND poolrad_overlay_u7d_zero_tests)
    add_executable(poolrad_overlay_u9e_entry2_tests
                   tests/recovered/test_overlay_u9e_entry2.c)
    target_link_libraries(poolrad_overlay_u9e_entry2_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u9e_entry2_tests
             COMMAND poolrad_overlay_u9e_entry2_tests)
    add_executable(poolrad_overlay_u9e_0160_tests
                   tests/recovered/test_overlay_u9e_0160.c)
    target_link_libraries(poolrad_overlay_u9e_0160_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u9e_0160_tests
             COMMAND poolrad_overlay_u9e_0160_tests)
    add_executable(poolrad_overlay_u9e_entry8_roll_tests
                   tests/recovered/test_overlay_u9e_entry8_roll.c)
    target_link_libraries(poolrad_overlay_u9e_entry8_roll_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u9e_entry8_roll_tests
             COMMAND poolrad_overlay_u9e_entry8_roll_tests)
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
    add_executable(poolrad_resident_field_0973_tests
                   tests/recovered/test_resident_field_0973.c)
    target_link_libraries(poolrad_resident_field_0973_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_resident_field_0973_tests
             COMMAND poolrad_resident_field_0973_tests)
    add_executable(poolrad_resident_string_0356_tests
                   tests/recovered/test_resident_string_0356.c)
    target_link_libraries(poolrad_resident_string_0356_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_resident_string_0356_tests
             COMMAND poolrad_resident_string_0356_tests)
    add_executable(poolrad_resident_string_0425_tests
                   tests/recovered/test_resident_string_0425.c)
    target_link_libraries(poolrad_resident_string_0425_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_resident_string_0425_tests
             COMMAND poolrad_resident_string_0425_tests)
    add_executable(poolrad_overlay_u6f_party_string_chain_tests
                   tests/recovered/test_overlay_u6f_party_string_chain.c
                   src/replay/slot_a_party_entry.c
                   src/replay/slot_a_native_save.c)
    target_link_libraries(poolrad_overlay_u6f_party_string_chain_tests
                          PRIVATE poolrad_recovered poolrad_formats)
    add_test(NAME poolrad_overlay_u6f_party_string_chain_tests
             COMMAND poolrad_overlay_u6f_party_string_chain_tests)
    add_executable(poolrad_overlay_u6f_party_filename_tests
                   tests/recovered/test_overlay_u6f_party_filename.c
                   src/replay/slot_a_party_entry.c
                   src/replay/slot_a_native_save.c)
    target_link_libraries(poolrad_overlay_u6f_party_filename_tests
                          PRIVATE poolrad_recovered poolrad_formats)
    add_test(NAME poolrad_overlay_u6f_party_filename_tests
             COMMAND poolrad_overlay_u6f_party_filename_tests)
    add_executable(poolrad_overlay_u6f_postinsert_02ae_tests
                   tests/recovered/test_overlay_u6f_postinsert_02ae.c)
    target_link_libraries(poolrad_overlay_u6f_postinsert_02ae_tests
                          PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u6f_postinsert_02ae_tests
             COMMAND poolrad_overlay_u6f_postinsert_02ae_tests)
    add_executable(poolrad_slot_a_directory_fullstate_tests
                   tests/recovered/test_slot_a_directory_fullstate.c)
    target_link_libraries(poolrad_slot_a_directory_fullstate_tests
                          PRIVATE poolrad_recovered)
    add_test(NAME poolrad_slot_a_directory_fullstate_tests
             COMMAND poolrad_slot_a_directory_fullstate_tests)
    add_executable(poolrad_slot_a_u6f_02da_decision_tests
                   tests/recovered/test_slot_a_u6f_02da_decision.c)
    target_link_libraries(poolrad_slot_a_u6f_02da_decision_tests
                          PRIVATE poolrad_recovered)
    add_test(NAME poolrad_slot_a_u6f_02da_decision_tests
             COMMAND poolrad_slot_a_u6f_02da_decision_tests)
    add_executable(poolrad_slot_a_u6f_031f_return_tests
                   tests/recovered/test_slot_a_u6f_031f_return.c)
    target_link_libraries(poolrad_slot_a_u6f_031f_return_tests
                          PRIVATE poolrad_recovered)
    add_test(NAME poolrad_slot_a_u6f_031f_return_tests
             COMMAND poolrad_slot_a_u6f_031f_return_tests)
    add_executable(poolrad_overlay_u6f_party_insert_1446_tests
                   tests/recovered/test_overlay_u6f_party_insert_1446.c)
    target_link_libraries(poolrad_overlay_u6f_party_insert_1446_tests
                          PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u6f_party_insert_1446_tests
             COMMAND poolrad_overlay_u6f_party_insert_1446_tests)
    add_executable(poolrad_overlay_u6f_party_insert_1446_dos_tests
                   tests/recovered/test_overlay_u6f_party_insert_1446_dos.c)
    target_link_libraries(poolrad_overlay_u6f_party_insert_1446_dos_tests
                          PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u6f_party_insert_1446_dos_tests
             COMMAND poolrad_overlay_u6f_party_insert_1446_dos_tests)
    add_executable(poolrad_overlay_u6f_insert_ss_probe_tests
                   tests/recovered/test_overlay_u6f_insert_ss_probe.c)
    target_link_libraries(poolrad_overlay_u6f_insert_ss_probe_tests
                          PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u6f_insert_ss_probe_tests
             COMMAND poolrad_overlay_u6f_insert_ss_probe_tests)
    add_executable(poolrad_overlay_u6f_party_insert_1446_ss_tests
                   tests/recovered/test_overlay_u6f_party_insert_1446_ss.c)
    target_link_libraries(poolrad_overlay_u6f_party_insert_1446_ss_tests
                          PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u6f_party_insert_1446_ss_tests
             COMMAND poolrad_overlay_u6f_party_insert_1446_ss_tests)
    add_executable(poolrad_resident_8ae_dispatch_tests
                   tests/recovered/test_resident_8ae_dispatch.c)
    target_link_libraries(poolrad_resident_8ae_dispatch_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_resident_8ae_dispatch_tests
             COMMAND poolrad_resident_8ae_dispatch_tests)
    add_executable(poolrad_resident_8ae_table_17ae_tests
                   tests/recovered/test_resident_8ae_table_17ae.c)
    target_link_libraries(poolrad_resident_8ae_table_17ae_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_resident_8ae_table_17ae_tests
             COMMAND poolrad_resident_8ae_table_17ae_tests)
    add_executable(poolrad_resident_8ae_table_1742_tests
                   tests/recovered/test_resident_8ae_table_1742.c)
    target_link_libraries(poolrad_resident_8ae_table_1742_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_resident_8ae_table_1742_tests
             COMMAND poolrad_resident_8ae_table_1742_tests)
    add_executable(poolrad_resident_464_bitmask_tests
                   tests/recovered/test_resident_464_bitmask.c)
    target_link_libraries(poolrad_resident_464_bitmask_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_resident_464_bitmask_tests
             COMMAND poolrad_resident_464_bitmask_tests)
    add_executable(poolrad_resident_464_nibble_007b_tests
                   tests/recovered/test_resident_464_nibble_007b.c)
    target_link_libraries(poolrad_resident_464_nibble_007b_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_resident_464_nibble_007b_tests
             COMMAND poolrad_resident_464_nibble_007b_tests)
    add_executable(poolrad_resident_464_mode2_tests
                   tests/recovered/test_resident_464_mode2.c)
    target_link_libraries(poolrad_resident_464_mode2_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_resident_464_mode2_tests
             COMMAND poolrad_resident_464_mode2_tests)
    add_executable(poolrad_resident_464_mode1_tests
                   tests/recovered/test_resident_464_mode1.c)
    target_link_libraries(poolrad_resident_464_mode1_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_resident_464_mode1_tests
             COMMAND poolrad_resident_464_mode1_tests)
    add_executable(poolrad_resident_464_mode0_prefix_tests
                   tests/recovered/test_resident_464_mode0_prefix.c)
    target_link_libraries(poolrad_resident_464_mode0_prefix_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_resident_464_mode0_prefix_tests
             COMMAND poolrad_resident_464_mode0_prefix_tests)
    add_executable(poolrad_resident_464_mode0_primary_tests
                   tests/recovered/test_resident_464_mode0_primary.c)
    target_link_libraries(poolrad_resident_464_mode0_primary_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_resident_464_mode0_primary_tests
             COMMAND poolrad_resident_464_mode0_primary_tests)
    add_executable(poolrad_resident_464_mode0_second_tests
                   tests/recovered/test_resident_464_mode0_second.c)
    target_link_libraries(poolrad_resident_464_mode0_second_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_resident_464_mode0_second_tests
             COMMAND poolrad_resident_464_mode0_second_tests)
    add_executable(poolrad_resident_464_mode0_loops_tests
                   tests/recovered/test_resident_464_mode0_loops.c)
    target_link_libraries(poolrad_resident_464_mode0_loops_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_resident_464_mode0_loops_tests
             COMMAND poolrad_resident_464_mode0_loops_tests)
    add_executable(poolrad_resident_464_mode0_single_tests
                   tests/recovered/test_resident_464_mode0_single.c)
    target_link_libraries(poolrad_resident_464_mode0_single_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_resident_464_mode0_single_tests
             COMMAND poolrad_resident_464_mode0_single_tests)
    add_executable(poolrad_resident_464_mode0_two_y_tests
                   tests/recovered/test_resident_464_mode0_two_y.c)
    target_link_libraries(poolrad_resident_464_mode0_two_y_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_resident_464_mode0_two_y_tests
             COMMAND poolrad_resident_464_mode0_two_y_tests)
    add_executable(poolrad_resident_464_mode0_iterator_tests
                   tests/recovered/test_resident_464_mode0_iterator.c)
    target_link_libraries(poolrad_resident_464_mode0_iterator_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_resident_464_mode0_iterator_tests
             COMMAND poolrad_resident_464_mode0_iterator_tests)
    add_executable(poolrad_resident_handler_prefix_02e6_tests
                   tests/recovered/test_resident_handler_prefix_02e6.c)
    target_link_libraries(poolrad_resident_handler_prefix_02e6_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_resident_handler_prefix_02e6_tests
             COMMAND poolrad_resident_handler_prefix_02e6_tests)
    add_executable(poolrad_resident_loader_prefix_0338_tests
                   tests/recovered/test_resident_loader_prefix_0338.c)
    target_link_libraries(poolrad_resident_loader_prefix_0338_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_resident_loader_prefix_0338_tests
             COMMAND poolrad_resident_loader_prefix_0338_tests)
    add_executable(poolrad_resident_loader_0553_tests
                   tests/recovered/test_resident_loader_0553.c)
    target_link_libraries(poolrad_resident_loader_0553_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_resident_loader_0553_tests
             COMMAND poolrad_resident_loader_0553_tests)
    add_executable(poolrad_resident_loader_0538_tests
                   tests/recovered/test_resident_loader_0538.c)
    target_link_libraries(poolrad_resident_loader_0538_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_resident_loader_0538_tests
             COMMAND poolrad_resident_loader_0538_tests)
    add_executable(poolrad_resident_loader_0366_tests
                   tests/recovered/test_resident_loader_0366.c)
    target_link_libraries(poolrad_resident_loader_0366_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_resident_loader_0366_tests
             COMMAND poolrad_resident_loader_0366_tests)
    add_executable(poolrad_resident_loader_0268_tests
                   tests/recovered/test_resident_loader_0268.c)
    target_link_libraries(poolrad_resident_loader_0268_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_resident_loader_0268_tests
             COMMAND poolrad_resident_loader_0268_tests)
    add_executable(poolrad_resident_loader_0355_tests
                   tests/recovered/test_resident_loader_0355.c)
    target_link_libraries(poolrad_resident_loader_0355_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_resident_loader_0355_tests
             COMMAND poolrad_resident_loader_0355_tests)
endif()
