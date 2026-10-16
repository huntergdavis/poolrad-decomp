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
add_library(poolrad_recovered STATIC src/recovered/overlay_u3c2_add_ax_dx_05eb_fixture.c
                                    src/recovered/overlay_u3c2_cbw_05ea_fixture.c
                                    src/recovered/overlay_u3c2_mov_sp_0903_fixture.c
                                    src/recovered/overlay_u3c2_stack_load_05e7_fixture.c
                                    src/recovered/overlay_u3c2_stack_load_0900_fixture.c
                                    src/recovered/overlay_u3c2_cbw_05e4_fixture.c
                                    src/recovered/overlay_u3c2_zero_store_05b1_fixture.c
                                    src/recovered/overlay_u3c2_mov_dx_05e5_fixture.c
                                    src/recovered/overlay_u3c2_jump_only_05b5_fixture.c
                                    src/recovered/overlay_u3c2_jle_decision_05af_fixture.c
                                    src/recovered/ecl_helpers.c
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
                                    src/recovered/resident_464_bitmask_frame.c
                                    src/recovered/resident_464_masked_byte_0c3d.c
                                    src/recovered/resident_464_loop_c6e.c
                                    src/recovered/resident_464_reverse_1484.c
                                    src/recovered/resident_464_row56_transfer.c
                                    src/recovered/resident_464_dirty_transfer.c
                                    src/recovered/resident_464_2632_run014.c
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
                                    src/recovered/resident_unequal_display_1816.c
                                    src/recovered/overlay_u3bd_wallset.c
                                    src/recovered/font_8x8d_201.c
                                    src/recovered/slot_a_printer_page.c
                                    src/recovered/slot_a_u3b0_prompt_row.c
                                    src/recovered/slot_a_second_cycle_zero_row.c
                                    src/recovered/resident_0709_rect_fill_00b7.c
                                    src/recovered/slot_a_glyph_plane_byte.c
                                    src/recovered/overlay_u3bd_cell.c
                                    src/recovered/overlay_u3bd_nibble.c
                                    src/recovered/overlay_u3bd_first_wall.c
                                    src/recovered/overlay_u3bd_view9_wall.c
                                    src/recovered/overlay_u3bd_loop_reset_07c7.c
                                    src/recovered/overlay_u3bd_first_call_07e9.c
                                    src/recovered/overlay_u3bd_leaf_call_07f9.c
                                    src/recovered/overlay_u3bd_jne_080a.c
                                    src/recovered/overlay_u3bd_dual_wall.c
                                    src/recovered/overlay_u3bd_view1_scan.c
                                    src/recovered/overlay_u3bd_view3_wall.c
                                    src/recovered/overlay_u3bd_view4_wall.c
                                    src/recovered/overlay_u40c_tile_plane.c
                                    src/recovered/overlay_u40c_group3_tile_plane.c
                                    src/recovered/resident_464_group3_dirty_metadata.c
                                    src/recovered/overlay_u3bd_field.c
                                    src/recovered/overlay_u25_move.c
                                    src/recovered/overlay_u25_party_index_0756.c
                                    src/recovered/overlay_u25_load_word_0e0c.c
                                    src/recovered/overlay_u25_store_word_0c77.c
                                    src/recovered/overlay_u6a_sort_0824.c
                                    src/recovered/overlay_u6a_record_remaining_025c.c
                                    src/recovered/overlay_u6a_record_clear_high_0159.c
                                    src/recovered/overlay_u2f_record_link_clear_122b.c
                                    src/recovered/overlay_u6c_record_table_sum_3f31.c
                                    src/recovered/overlay_u6a_party_before_14a9.c
                                    src/recovered/overlay_u6a_party_after_1601.c
                                    src/recovered/overlay_u77_weighted_28be.c
                                    src/recovered/overlay_u77_split_17ed.c
                                    src/recovered/overlay_u1c_sprite_off_2aa2.c
                                    src/recovered/overlay_u1c_clear_box_2b14.c
                                    src/recovered/overlay_u1c_clock_2ac1.c
                                    src/recovered/overlay_u1c_return_0a9f.c
                                    src/recovered/overlay_u1c_compare_and_0aed.c
                                    src/recovered/overlay_u1c_gosub_0111.c
                                    src/recovered/overlay_u1c_goto_00f4.c
                                    src/recovered/overlay_u1c_input_number_0951.c
                                    src/recovered/overlay_u1c_input_string_0994.c
                                    src/recovered/overlay_u1c_vertical_menu_prefix_0f08.c
                                    src/recovered/overlay_u3b0_entry6_1140.c
                                    src/recovered/overlay_u3b0_0874_di_load.c
                                    src/recovered/overlay_u3b0_0877_cmp_al.c
                                    src/recovered/overlay_u3b0_087b_jae.c
                                    src/recovered/overlay_u3b0_087d_les_di.c
                                    src/recovered/overlay_u3b0_0880_es_al_read.c
                                    src/recovered/overlay_u3b0_0883_xor_ah.c
                                    src/recovered/overlay_u3b0_0885_mov_dx_ax.c
                                    src/recovered/overlay_u3b0_0887_ss_di_read.c
                                    src/recovered/overlay_u3b0_088f_add_ax_dx.c
                                    src/recovered/overlay_u3b0_0891_push_ax.c
                                    src/recovered/overlay_u3b0_0895_push_ax.c
                                    src/recovered/overlay_u3b0_088a_ss_al_read.c
                                    src/recovered/overlay_u3b0_088e_cbw.c
                                    src/recovered/overlay_u3b0_08ba_les_di.c
                                    src/recovered/overlay_u3b0_08bd_es_ax_read.c
                                    src/recovered/overlay_u3b0_08c1_es_dx_read.c
                                    src/recovered/overlay_u3b0_08c5_ss_ax_store.c
                                    src/recovered/overlay_u3b0_08c8_ss_dx_store.c
                                    src/recovered/overlay_u3b0_08cb_ss_inc.c
                                    src/recovered/overlay_u3b0_08ce_jmp.c
                                    src/recovered/overlay_u3b0_0892_ss_al_load.c
                                    src/recovered/overlay_u3b0_0896_mov_al_zero.c
                                    src/recovered/overlay_u3b0_0898_push_ax.c
                                    src/recovered/overlay_u3b0_0899_les_di.c
                                    src/recovered/overlay_u38_sidecar_flag_0918.c
                                    src/recovered/overlay_u38_target_select_101d.c
                                    src/recovered/overlay_u38_roll_dispatch_1699.c
                                    src/recovered/overlay_u38_target_reset_2b7d.c
                                    src/recovered/overlay_u38_link_message_2bc8.c
                                    src/recovered/overlay_u38_target_gate_2ae6.c
                                    src/recovered/overlay_u38_link_roll_2b20.c
                                    src/recovered/overlay_u1c_vertical_menu_one_node_0f73.c
                                    src/recovered/overlay_u1c_vertical_menu_null_head_0f73.c
                                    src/recovered/overlay_u1c_vertical_menu_two_node_0f73.c
                                    src/recovered/overlay_u7f_div5.c
                                    src/recovered/overlay_u7f_div5_dwords.c
                                    src/recovered/overlay_u7f_party_count_05e0.c
                                    src/recovered/overlay_u7f_state_flags_0f7e.c
                                    src/recovered/overlay_u77_active_select_27df.c
                                    src/recovered/overlay_u33_byte_flags_08a4.c
                                    src/recovered/overlay_u86_procvars_3413.c
                                    src/recovered/overlay_u86_record_predicate_323d.c
                                    src/recovered/overlay_u38_effects_install_31a2.c
                                    src/recovered/overlay_u77_stack_189d.c
                                    src/recovered/overlay_u68_up_preflight.c
                                    src/recovered/overlay_u77_spell_299c.c
                                    src/recovered/overlay_u68_coordinate_writer.c
                                    src/recovered/overlay_u68_entry1.c
                                    src/recovered/overlay_u68_party_byte_0236.c
                                    src/recovered/overlay_u68_field_clear_0000.c
                                    src/recovered/overlay_u68_roll_gate_0294.c
                                    src/recovered/overlay_u68_tile_step_0818.c
                                    src/recovered/overlay_u1c_loop_3387.c
                                    src/recovered/overlay_u1c_key_branch_33bd.c
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
                                    src/recovered/overlay_u5e_half_0e52.c
                                    src/recovered/overlay_u1c_arith_01a3.c
                                    src/recovered/overlay_u1c_gettable_0e5e.c
                                    src/recovered/overlay_u1c_party_surprise_1629.c
                                    src/recovered/overlay_u1c_surprise_16c9.c
                                    src/recovered/overlay_u1c_setup_monster_03b4.c
                                    src/recovered/overlay_u1c_approach_0817.c
                                    src/recovered/overlay_u1c_delay_271f.c
                                    src/recovered/resident_0709_delay_wrapper_0dbe.c
                                    src/recovered/resident_a40_delay_029c.c
                                    src/recovered/overlay_u1c_horizontal_menu_10d1.c
                                    src/recovered/overlay_u1c_clearmonsters_125d.c
                                    src/recovered/overlay_u25_entry5_0411.c
                                    src/recovered/overlay_u1c_load_monster_prefix_0450.c
                                    src/recovered/overlay_u1c_load_monster_04af.c
                                    src/recovered/overlay_u3ce_entry4_01c8.c
                                    src/recovered/overlay_u1c_load_monster_tail_0537.c
                                    src/recovered/resident_video_init_4426.c
                                    src/recovered/resident_palette_2a0d.c
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
                                    src/recovered/overlay_u7d_carry_02f2.c
                                    src/recovered/overlay_u7d_zero.c
                                    src/recovered/overlay_u9e_entry2.c
                                    src/recovered/overlay_u3c6_slot.c
                                    src/recovered/overlay_u3c6_mov_sp_bp_0062.c
                                    src/recovered/overlay_u9e_0160.c
                                    src/recovered/overlay_u9e_entry8_roll.c
                                    src/recovered/overlay_u9e_entry9_roll_latch_0e2e.c
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
                                    src/recovered/slot_a_u6f_19ea_caller.c
                                    src/recovered/slot_a_u6f_1937_precall.c
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
                                    src/recovered/resident_loader_seek_028f.c
                                    src/recovered/resident_loader_read_gate_0291.c
                                    src/recovered/resident_loader_dos_read_029f.c
                                    src/recovered/resident_loader_read_branch_02a1.c
                                    src/recovered/resident_loader_composed_028f.c
                                    src/recovered/resident_loader_0355.c
                                    src/recovered/resident_8ae_dispatch.c
                                    src/recovered/resident_8ae_table_17ae.c
                                    src/recovered/resident_8ae_table_1742.c
                                    src/recovered/resident_time_caller_prefix_08ae_001f.c
                                    src/recovered/resident_time_caller_return_08ae_002a.c
                                    src/recovered/resident_time_caller_08ae_0031.c
                                    src/recovered/resident_time_caller_composed_08ae_001f.c
                                    src/recovered/resident_time_caller_08ae_0033.c
                                    src/recovered/resident_time_caller_08ae_0036.c)
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
    add_executable(poolrad_overlay_u3c2_add_ax_dx_05eb_fixture_tests
                   tests/recovered/test_overlay_u3c2_add_ax_dx_05eb_fixture.c)
    target_link_libraries(poolrad_overlay_u3c2_add_ax_dx_05eb_fixture_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3c2_add_ax_dx_05eb_fixture_tests
             COMMAND poolrad_overlay_u3c2_add_ax_dx_05eb_fixture_tests)

    add_executable(poolrad_overlay_u3c2_cbw_05ea_fixture_tests
                   tests/recovered/test_overlay_u3c2_cbw_05ea_fixture.c)
    target_link_libraries(poolrad_overlay_u3c2_cbw_05ea_fixture_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3c2_cbw_05ea_fixture_tests
             COMMAND poolrad_overlay_u3c2_cbw_05ea_fixture_tests)

    add_executable(poolrad_overlay_u3c2_mov_sp_0903_fixture_tests
                   tests/recovered/test_overlay_u3c2_mov_sp_0903_fixture.c)
    target_link_libraries(poolrad_overlay_u3c2_mov_sp_0903_fixture_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3c2_mov_sp_0903_fixture_tests
             COMMAND poolrad_overlay_u3c2_mov_sp_0903_fixture_tests)

    add_test(NAME poolrad_ctest_source_paths
             COMMAND "${CMAKE_COMMAND}" -P
                     "${CMAKE_CURRENT_SOURCE_DIR}/tests/qa/test_ctest_source_paths.cmake")
    add_test(NAME poolrad_ctest_depends
             COMMAND "${CMAKE_COMMAND}" -P
                     "${CMAKE_CURRENT_SOURCE_DIR}/tests/qa/test_ctest_depends.cmake")
    add_test(NAME poolrad_ctest_depends_graph
             COMMAND "${CMAKE_COMMAND}" -P
                     "${CMAKE_CURRENT_SOURCE_DIR}/tests/qa/test_ctest_depends_graph.cmake")
    add_executable(poolrad_overlay_u3c2_stack_load_05e7_fixture_tests
                   tests/recovered/test_overlay_u3c2_stack_load_05e7_fixture.c)
    target_link_libraries(poolrad_overlay_u3c2_stack_load_05e7_fixture_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3c2_stack_load_05e7_fixture_tests
             COMMAND poolrad_overlay_u3c2_stack_load_05e7_fixture_tests)

    add_executable(poolrad_overlay_u3c2_stack_load_0900_fixture_tests
                   tests/recovered/test_overlay_u3c2_stack_load_0900_fixture.c)
    target_link_libraries(poolrad_overlay_u3c2_stack_load_0900_fixture_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3c2_stack_load_0900_fixture_tests
             COMMAND poolrad_overlay_u3c2_stack_load_0900_fixture_tests)

    add_executable(poolrad_overlay_u3c2_cbw_05e4_fixture_tests
                   tests/recovered/test_overlay_u3c2_cbw_05e4_fixture.c)
    target_link_libraries(poolrad_overlay_u3c2_cbw_05e4_fixture_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3c2_cbw_05e4_fixture_tests
             COMMAND poolrad_overlay_u3c2_cbw_05e4_fixture_tests)

    add_executable(poolrad_overlay_u3c2_zero_store_05b1_fixture_tests
                   tests/recovered/test_overlay_u3c2_zero_store_05b1_fixture.c)
    target_link_libraries(poolrad_overlay_u3c2_zero_store_05b1_fixture_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3c2_zero_store_05b1_fixture_tests
             COMMAND poolrad_overlay_u3c2_zero_store_05b1_fixture_tests)

    add_executable(poolrad_overlay_u3c2_mov_dx_05e5_fixture_tests
                   tests/recovered/test_overlay_u3c2_mov_dx_05e5_fixture.c)
    target_link_libraries(poolrad_overlay_u3c2_mov_dx_05e5_fixture_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3c2_mov_dx_05e5_fixture_tests
             COMMAND poolrad_overlay_u3c2_mov_dx_05e5_fixture_tests)

    add_executable(poolrad_overlay_u3c2_jump_only_05b5_fixture_tests
                   tests/recovered/test_overlay_u3c2_jump_only_05b5_fixture.c)
    target_link_libraries(poolrad_overlay_u3c2_jump_only_05b5_fixture_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3c2_jump_only_05b5_fixture_tests
             COMMAND poolrad_overlay_u3c2_jump_only_05b5_fixture_tests)

    add_executable(poolrad_overlay_u3c2_jle_decision_05af_fixture_tests
                   tests/recovered/test_overlay_u3c2_jle_decision_05af_fixture.c)
    target_link_libraries(poolrad_overlay_u3c2_jle_decision_05af_fixture_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3c2_jle_decision_05af_fixture_tests
             COMMAND poolrad_overlay_u3c2_jle_decision_05af_fixture_tests)

    add_executable(poolrad_format_tests tests/formats/test_formats.c)
    target_link_libraries(poolrad_format_tests PRIVATE poolrad_formats)
    add_test(NAME poolrad_format_tests COMMAND poolrad_format_tests)
    add_executable(poolrad_ecl_tests tests/formats/test_ecl.c)
    target_link_libraries(poolrad_ecl_tests PRIVATE poolrad_formats)
    add_test(NAME poolrad_ecl_tests COMMAND poolrad_ecl_tests)
    add_executable(poolrad_ega_tests tests/formats/test_ega.c)
    target_link_libraries(poolrad_ega_tests PRIVATE poolrad_formats)
    add_test(NAME poolrad_ega_tests COMMAND poolrad_ega_tests)
    add_executable(poolrad_ega_planar_byte_probe
                   tools/replay/ega_planar_byte_probe.c)
    target_link_libraries(poolrad_ega_planar_byte_probe PRIVATE poolrad_formats)
    add_executable(poolrad_ega_planar_row_probe
                   tools/replay/ega_planar_row_probe.c)
    target_link_libraries(poolrad_ega_planar_row_probe PRIVATE poolrad_formats)
    add_executable(poolrad_slot_a_ega_row56_transfer_probe
                   tools/replay/slot_a_ega_row56_transfer_probe.c)
    target_link_libraries(poolrad_slot_a_ega_row56_transfer_probe PRIVATE
                          poolrad_recovered)
    add_executable(poolrad_slot_a_ega_all_dirty_probe
                   tools/replay/slot_a_ega_all_dirty_probe.c)
    target_link_libraries(poolrad_slot_a_ega_all_dirty_probe PRIVATE
                          poolrad_recovered)
    add_executable(poolrad_slot_a_ega_dirty_indexed_probe
                   tools/replay/slot_a_ega_dirty_indexed_probe.c)
    target_link_libraries(poolrad_slot_a_ega_dirty_indexed_probe PRIVATE
                          poolrad_recovered poolrad_formats)
    add_executable(poolrad_resident_464_dirty_indexed_tests
                   tests/recovered/test_resident_464_dirty_indexed.c)
    target_link_libraries(poolrad_resident_464_dirty_indexed_tests PRIVATE
                          poolrad_recovered poolrad_formats)
    add_test(NAME poolrad_resident_464_dirty_indexed_tests
             COMMAND poolrad_resident_464_dirty_indexed_tests)
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
    add_executable(poolrad_resident_unequal_display_1816_tests
                   tests/recovered/test_resident_unequal_display_1816.c)
    target_link_libraries(poolrad_resident_unequal_display_1816_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_resident_unequal_display_1816_tests
             COMMAND poolrad_resident_unequal_display_1816_tests)
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
    add_executable(poolrad_overlay_u3bd_first_wall_tests
                   tests/recovered/test_overlay_u3bd_first_wall.c)
    target_link_libraries(poolrad_overlay_u3bd_first_wall_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3bd_first_wall_tests
             COMMAND poolrad_overlay_u3bd_first_wall_tests)
    add_executable(poolrad_overlay_u3bd_view9_wall_tests
                   tests/recovered/test_overlay_u3bd_view9_wall.c)
    target_link_libraries(poolrad_overlay_u3bd_view9_wall_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3bd_view9_wall_tests
             COMMAND poolrad_overlay_u3bd_view9_wall_tests)
    add_executable(poolrad_overlay_u3bd_loop_reset_07c7_tests
                   tests/recovered/test_overlay_u3bd_loop_reset_07c7.c)
    target_link_libraries(poolrad_overlay_u3bd_loop_reset_07c7_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3bd_loop_reset_07c7_tests
             COMMAND poolrad_overlay_u3bd_loop_reset_07c7_tests)
    add_executable(poolrad_overlay_u3bd_first_call_07e9_tests
                   tests/recovered/test_overlay_u3bd_first_call_07e9.c)
    target_link_libraries(poolrad_overlay_u3bd_first_call_07e9_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3bd_first_call_07e9_tests
             COMMAND poolrad_overlay_u3bd_first_call_07e9_tests)
    add_executable(poolrad_overlay_u3bd_leaf_call_07f9_tests
                   tests/recovered/test_overlay_u3bd_leaf_call_07f9.c)
    target_link_libraries(poolrad_overlay_u3bd_leaf_call_07f9_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3bd_leaf_call_07f9_tests
             COMMAND poolrad_overlay_u3bd_leaf_call_07f9_tests)
    add_executable(poolrad_overlay_u3bd_jne_080a_tests
                   tests/recovered/test_overlay_u3bd_jne_080a.c)
    target_link_libraries(poolrad_overlay_u3bd_jne_080a_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3bd_jne_080a_tests
             COMMAND poolrad_overlay_u3bd_jne_080a_tests)
    add_executable(poolrad_overlay_u3bd_dual_wall_tests
                   tests/recovered/test_overlay_u3bd_dual_wall.c)
    target_link_libraries(poolrad_overlay_u3bd_dual_wall_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3bd_dual_wall_tests
             COMMAND poolrad_overlay_u3bd_dual_wall_tests)
    add_executable(poolrad_overlay_u3bd_view1_scan_tests
                   tests/recovered/test_overlay_u3bd_view1_scan.c)
    target_link_libraries(poolrad_overlay_u3bd_view1_scan_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3bd_view1_scan_tests
             COMMAND poolrad_overlay_u3bd_view1_scan_tests)
    add_executable(poolrad_overlay_u3bd_view3_wall_tests
                   tests/recovered/test_overlay_u3bd_view3_wall.c)
    target_link_libraries(poolrad_overlay_u3bd_view3_wall_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3bd_view3_wall_tests
             COMMAND poolrad_overlay_u3bd_view3_wall_tests)
    add_executable(poolrad_overlay_u3bd_view4_wall_tests
                   tests/recovered/test_overlay_u3bd_view4_wall.c)
    target_link_libraries(poolrad_overlay_u3bd_view4_wall_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3bd_view4_wall_tests
             COMMAND poolrad_overlay_u3bd_view4_wall_tests)
    add_executable(poolrad_overlay_u40c_tile_plane_tests
                   tests/recovered/test_overlay_u40c_tile_plane.c)
    target_link_libraries(poolrad_overlay_u40c_tile_plane_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u40c_tile_plane_tests
             COMMAND poolrad_overlay_u40c_tile_plane_tests)
    add_executable(poolrad_slot_a_view4_tile04_tests
                   tests/replay/test_slot_a_view4_tile04.c)
    target_link_libraries(poolrad_slot_a_view4_tile04_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_slot_a_view4_tile04_tests
             COMMAND poolrad_slot_a_view4_tile04_tests)
    add_executable(poolrad_slot_a_view4_tile05_tests
                   tests/replay/test_slot_a_view4_tile05.c)
    target_link_libraries(poolrad_slot_a_view4_tile05_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_slot_a_view4_tile05_tests
             COMMAND poolrad_slot_a_view4_tile05_tests)
    add_executable(poolrad_slot_a_view4_tile06_tests
                   tests/replay/test_slot_a_view4_tile06.c)
    target_link_libraries(poolrad_slot_a_view4_tile06_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_slot_a_view4_tile06_tests
             COMMAND poolrad_slot_a_view4_tile06_tests)
    add_executable(poolrad_slot_a_view4_tile07_tests
                   tests/replay/test_slot_a_view4_tile07.c)
    target_link_libraries(poolrad_slot_a_view4_tile07_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_slot_a_view4_tile07_tests
             COMMAND poolrad_slot_a_view4_tile07_tests)
    add_executable(poolrad_slot_a_view4_first8_compose_tests
                   tests/replay/test_slot_a_view4_first8_compose.c)
    target_link_libraries(poolrad_slot_a_view4_first8_compose_tests
                          PRIVATE poolrad_recovered)
    add_test(NAME poolrad_slot_a_view4_first8_compose_tests
             COMMAND poolrad_slot_a_view4_first8_compose_tests)
    add_executable(poolrad_slot_a_view4_last6_compose_tests
                   tests/replay/test_slot_a_view4_last6_compose.c)
    target_link_libraries(poolrad_slot_a_view4_last6_compose_tests
                          PRIVATE poolrad_recovered)
    add_test(NAME poolrad_slot_a_view4_last6_compose_tests
             COMMAND poolrad_slot_a_view4_last6_compose_tests)
    add_executable(poolrad_slot_a_view4_post16_first_writer_tests
                   tests/replay/test_slot_a_view4_post16_first_writer.c)
    target_link_libraries(poolrad_slot_a_view4_post16_first_writer_tests
                          PRIVATE poolrad_recovered)
    add_test(NAME poolrad_slot_a_view4_post16_first_writer_tests
             COMMAND poolrad_slot_a_view4_post16_first_writer_tests)
    add_executable(poolrad_overlay_u40c_group3_tile_plane_tests
                   tests/recovered/test_overlay_u40c_group3_tile_plane.c)
    target_link_libraries(poolrad_overlay_u40c_group3_tile_plane_tests
                          PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u40c_group3_tile_plane_tests
             COMMAND poolrad_overlay_u40c_group3_tile_plane_tests)
    add_executable(poolrad_resident_464_group3_dirty_metadata_tests
                   tests/recovered/test_resident_464_group3_dirty_metadata.c)
    target_link_libraries(poolrad_resident_464_group3_dirty_metadata_tests
                          PRIVATE poolrad_recovered)
    add_test(NAME poolrad_resident_464_group3_dirty_metadata_tests
             COMMAND poolrad_resident_464_group3_dirty_metadata_tests)
    add_executable(poolrad_slot_a_u40c_group3_dirty_probe
                   tools/replay/slot_a_u40c_group3_dirty_probe.c)
    target_link_libraries(poolrad_slot_a_u40c_group3_dirty_probe
                          PRIVATE poolrad_recovered)
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
    add_executable(poolrad_overlay_u25_party_index_0756_tests
                   tests/recovered/test_overlay_u25_party_index_0756.c)
    target_link_libraries(poolrad_overlay_u25_party_index_0756_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u25_party_index_0756_tests
             COMMAND poolrad_overlay_u25_party_index_0756_tests)
    add_executable(poolrad_overlay_u25_load_word_0e0c_tests
                   tests/recovered/test_overlay_u25_load_word_0e0c.c)
    target_link_libraries(poolrad_overlay_u25_load_word_0e0c_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u25_load_word_0e0c_tests
             COMMAND poolrad_overlay_u25_load_word_0e0c_tests)
    add_executable(poolrad_overlay_u25_store_word_0c77_tests
                   tests/recovered/test_overlay_u25_store_word_0c77.c)
    target_link_libraries(poolrad_overlay_u25_store_word_0c77_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u25_store_word_0c77_tests
             COMMAND poolrad_overlay_u25_store_word_0c77_tests)
    add_executable(poolrad_overlay_u6a_sort_0824_tests
                   tests/recovered/test_overlay_u6a_sort_0824.c)
    target_link_libraries(poolrad_overlay_u6a_sort_0824_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u6a_sort_0824_tests
             COMMAND poolrad_overlay_u6a_sort_0824_tests)
    add_executable(poolrad_overlay_u6a_record_remaining_025c_tests
                   tests/recovered/test_overlay_u6a_record_remaining_025c.c)
    target_link_libraries(poolrad_overlay_u6a_record_remaining_025c_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u6a_record_remaining_025c_tests
             COMMAND poolrad_overlay_u6a_record_remaining_025c_tests)
    add_executable(poolrad_overlay_u6a_record_clear_high_0159_tests
                   tests/recovered/test_overlay_u6a_record_clear_high_0159.c)
    target_link_libraries(poolrad_overlay_u6a_record_clear_high_0159_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u6a_record_clear_high_0159_tests
             COMMAND poolrad_overlay_u6a_record_clear_high_0159_tests)
    add_executable(poolrad_overlay_u2f_record_link_clear_122b_tests
                   tests/recovered/test_overlay_u2f_record_link_clear_122b.c)
    target_link_libraries(poolrad_overlay_u2f_record_link_clear_122b_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u2f_record_link_clear_122b_tests
             COMMAND poolrad_overlay_u2f_record_link_clear_122b_tests)
    add_executable(poolrad_overlay_u6c_record_table_sum_3f31_tests
                   tests/recovered/test_overlay_u6c_record_table_sum_3f31.c)
    target_link_libraries(poolrad_overlay_u6c_record_table_sum_3f31_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u6c_record_table_sum_3f31_tests
             COMMAND poolrad_overlay_u6c_record_table_sum_3f31_tests)
    add_executable(poolrad_overlay_u6a_party_before_14a9_tests
                   tests/recovered/test_overlay_u6a_party_before_14a9.c)
    target_link_libraries(poolrad_overlay_u6a_party_before_14a9_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u6a_party_before_14a9_tests
             COMMAND poolrad_overlay_u6a_party_before_14a9_tests)
    add_executable(poolrad_overlay_u6a_party_after_1601_tests
                   tests/recovered/test_overlay_u6a_party_after_1601.c)
    target_link_libraries(poolrad_overlay_u6a_party_after_1601_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u6a_party_after_1601_tests
             COMMAND poolrad_overlay_u6a_party_after_1601_tests)
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
    add_executable(poolrad_overlay_u7f_party_count_05e0_tests
                   tests/recovered/test_overlay_u7f_party_count_05e0.c)
    target_link_libraries(poolrad_overlay_u7f_party_count_05e0_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u7f_party_count_05e0_tests
             COMMAND poolrad_overlay_u7f_party_count_05e0_tests)
    add_executable(poolrad_overlay_u7f_state_flags_0f7e_tests
                   tests/recovered/test_overlay_u7f_state_flags_0f7e.c)
    target_link_libraries(poolrad_overlay_u7f_state_flags_0f7e_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u7f_state_flags_0f7e_tests
             COMMAND poolrad_overlay_u7f_state_flags_0f7e_tests)
    add_executable(poolrad_overlay_u33_byte_flags_08a4_tests
                   tests/recovered/test_overlay_u33_byte_flags_08a4.c)
    target_link_libraries(poolrad_overlay_u33_byte_flags_08a4_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u33_byte_flags_08a4_tests
             COMMAND poolrad_overlay_u33_byte_flags_08a4_tests)
    add_executable(poolrad_overlay_u86_record_predicate_323d_tests
                   tests/recovered/test_overlay_u86_record_predicate_323d.c)
    target_link_libraries(poolrad_overlay_u86_record_predicate_323d_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u86_record_predicate_323d_tests
             COMMAND poolrad_overlay_u86_record_predicate_323d_tests)
    add_executable(poolrad_overlay_u38_effects_install_31a2_tests
                   tests/recovered/test_overlay_u38_effects_install_31a2.c)
    target_link_libraries(poolrad_overlay_u38_effects_install_31a2_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u38_effects_install_31a2_tests
             COMMAND poolrad_overlay_u38_effects_install_31a2_tests)
    add_executable(poolrad_overlay_u86_procvars_3413_tests
                   tests/recovered/test_overlay_u86_procvars_3413.c)
    target_link_libraries(poolrad_overlay_u86_procvars_3413_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u86_procvars_3413_tests
             COMMAND poolrad_overlay_u86_procvars_3413_tests)
    add_executable(poolrad_overlay_u77_active_select_27df_tests
                   tests/recovered/test_overlay_u77_active_select_27df.c)
    target_link_libraries(poolrad_overlay_u77_active_select_27df_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u77_active_select_27df_tests
             COMMAND poolrad_overlay_u77_active_select_27df_tests)
    add_executable(poolrad_overlay_u77_weighted_28be_tests
                   tests/recovered/test_overlay_u77_weighted_28be.c)
    target_link_libraries(poolrad_overlay_u77_weighted_28be_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u77_weighted_28be_tests
             COMMAND poolrad_overlay_u77_weighted_28be_tests)
    add_executable(poolrad_overlay_u77_stack_189d_tests
                   tests/recovered/test_overlay_u77_stack_189d.c)
    target_link_libraries(poolrad_overlay_u77_stack_189d_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u77_stack_189d_tests
             COMMAND poolrad_overlay_u77_stack_189d_tests)
    add_executable(poolrad_overlay_u77_split_17ed_tests
                   tests/recovered/test_overlay_u77_split_17ed.c)
    target_link_libraries(poolrad_overlay_u77_split_17ed_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u77_split_17ed_tests
             COMMAND poolrad_overlay_u77_split_17ed_tests)
    add_executable(poolrad_overlay_u1c_sprite_off_2aa2_tests
                   tests/recovered/test_overlay_u1c_sprite_off_2aa2.c)
    target_link_libraries(poolrad_overlay_u1c_sprite_off_2aa2_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u1c_sprite_off_2aa2_tests
             COMMAND poolrad_overlay_u1c_sprite_off_2aa2_tests)
    add_executable(poolrad_overlay_u1c_clear_box_2b14_tests
                   tests/recovered/test_overlay_u1c_clear_box_2b14.c)
    target_link_libraries(poolrad_overlay_u1c_clear_box_2b14_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u1c_clear_box_2b14_tests
             COMMAND poolrad_overlay_u1c_clear_box_2b14_tests)
    add_executable(poolrad_overlay_u1c_clock_2ac1_tests
                   tests/recovered/test_overlay_u1c_clock_2ac1.c)
    target_link_libraries(poolrad_overlay_u1c_clock_2ac1_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u1c_clock_2ac1_tests
             COMMAND poolrad_overlay_u1c_clock_2ac1_tests)
    add_executable(poolrad_overlay_u1c_return_0a9f_tests
                   tests/recovered/test_overlay_u1c_return_0a9f.c)
    target_link_libraries(poolrad_overlay_u1c_return_0a9f_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u1c_return_0a9f_tests
             COMMAND poolrad_overlay_u1c_return_0a9f_tests)
    add_executable(poolrad_overlay_u1c_compare_and_0aed_tests
                   tests/recovered/test_overlay_u1c_compare_and_0aed.c)
    target_link_libraries(poolrad_overlay_u1c_compare_and_0aed_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u1c_compare_and_0aed_tests
             COMMAND poolrad_overlay_u1c_compare_and_0aed_tests)
    add_executable(poolrad_overlay_u1c_gosub_0111_tests
                   tests/recovered/test_overlay_u1c_gosub_0111.c)
    target_link_libraries(poolrad_overlay_u1c_gosub_0111_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u1c_gosub_0111_tests
             COMMAND poolrad_overlay_u1c_gosub_0111_tests)
    add_executable(poolrad_overlay_u1c_goto_00f4_tests
                   tests/recovered/test_overlay_u1c_goto_00f4.c)
    target_link_libraries(poolrad_overlay_u1c_goto_00f4_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u1c_goto_00f4_tests
             COMMAND poolrad_overlay_u1c_goto_00f4_tests)
    add_executable(poolrad_overlay_u1c_input_number_0951_tests
                   tests/recovered/test_overlay_u1c_input_number_0951.c)
    target_link_libraries(poolrad_overlay_u1c_input_number_0951_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u1c_input_number_0951_tests
             COMMAND poolrad_overlay_u1c_input_number_0951_tests)
    add_executable(poolrad_overlay_u1c_input_string_0994_tests
                   tests/recovered/test_overlay_u1c_input_string_0994.c)
    target_link_libraries(poolrad_overlay_u1c_input_string_0994_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u1c_input_string_0994_tests
             COMMAND poolrad_overlay_u1c_input_string_0994_tests)
    add_executable(poolrad_overlay_u1c_vertical_menu_prefix_0f08_tests
                   tests/recovered/test_overlay_u1c_vertical_menu_prefix_0f08.c)
    target_link_libraries(poolrad_overlay_u1c_vertical_menu_prefix_0f08_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u1c_vertical_menu_prefix_0f08_tests
             COMMAND poolrad_overlay_u1c_vertical_menu_prefix_0f08_tests)
    add_executable(poolrad_overlay_u3b0_entry6_1140_tests
                   tests/recovered/test_overlay_u3b0_entry6_1140.c)
    target_link_libraries(poolrad_overlay_u3b0_entry6_1140_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3b0_entry6_1140_tests
             COMMAND poolrad_overlay_u3b0_entry6_1140_tests)
    add_executable(poolrad_overlay_u3b0_087b_jae_tests
                   tests/recovered/test_overlay_u3b0_087b_jae.c)
    target_link_libraries(poolrad_overlay_u3b0_087b_jae_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3b0_087b_jae_tests
             COMMAND poolrad_overlay_u3b0_087b_jae_tests)
    add_executable(poolrad_overlay_u3b0_0880_es_al_read_tests
                   tests/recovered/test_overlay_u3b0_0880_es_al_read.c)
    target_link_libraries(poolrad_overlay_u3b0_0880_es_al_read_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3b0_0880_es_al_read_tests
             COMMAND poolrad_overlay_u3b0_0880_es_al_read_tests)
    add_executable(poolrad_overlay_u3b0_0883_xor_ah_tests
                   tests/recovered/test_overlay_u3b0_0883_xor_ah.c)
    target_link_libraries(poolrad_overlay_u3b0_0883_xor_ah_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3b0_0883_xor_ah_tests
             COMMAND poolrad_overlay_u3b0_0883_xor_ah_tests)
    add_executable(poolrad_overlay_u3b0_0885_mov_dx_ax_tests
                   tests/recovered/test_overlay_u3b0_0885_mov_dx_ax.c)
    target_link_libraries(poolrad_overlay_u3b0_0885_mov_dx_ax_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3b0_0885_mov_dx_ax_tests
             COMMAND poolrad_overlay_u3b0_0885_mov_dx_ax_tests)
    add_executable(poolrad_overlay_u3b0_0887_ss_di_read_tests
                   tests/recovered/test_overlay_u3b0_0887_ss_di_read.c)
    target_link_libraries(poolrad_overlay_u3b0_0887_ss_di_read_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3b0_0887_ss_di_read_tests
             COMMAND poolrad_overlay_u3b0_0887_ss_di_read_tests)
    add_executable(poolrad_overlay_u3b0_088a_ss_al_read_tests
                   tests/recovered/test_overlay_u3b0_088a_ss_al_read.c)
    target_link_libraries(poolrad_overlay_u3b0_088a_ss_al_read_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3b0_088a_ss_al_read_tests
             COMMAND poolrad_overlay_u3b0_088a_ss_al_read_tests)
    add_executable(poolrad_overlay_u3b0_088e_cbw_tests
                   tests/recovered/test_overlay_u3b0_088e_cbw.c)
    target_link_libraries(poolrad_overlay_u3b0_088e_cbw_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3b0_088e_cbw_tests
             COMMAND poolrad_overlay_u3b0_088e_cbw_tests)
    add_executable(poolrad_overlay_u3b0_08ba_les_di_tests
                   tests/recovered/test_overlay_u3b0_08ba_les_di.c)
    target_link_libraries(poolrad_overlay_u3b0_08ba_les_di_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3b0_08ba_les_di_tests
             COMMAND poolrad_overlay_u3b0_08ba_les_di_tests)
    add_executable(poolrad_overlay_u3b0_08bd_es_ax_read_tests
                   tests/recovered/test_overlay_u3b0_08bd_es_ax_read.c)
    target_link_libraries(poolrad_overlay_u3b0_08bd_es_ax_read_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3b0_08bd_es_ax_read_tests
             COMMAND poolrad_overlay_u3b0_08bd_es_ax_read_tests)
    add_executable(poolrad_overlay_u3b0_08c1_es_dx_read_tests
                   tests/recovered/test_overlay_u3b0_08c1_es_dx_read.c)
    target_link_libraries(poolrad_overlay_u3b0_08c1_es_dx_read_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3b0_08c1_es_dx_read_tests
             COMMAND poolrad_overlay_u3b0_08c1_es_dx_read_tests)
    add_executable(poolrad_overlay_u3b0_08c5_ss_ax_store_tests
                   tests/recovered/test_overlay_u3b0_08c5_ss_ax_store.c)
    target_link_libraries(poolrad_overlay_u3b0_08c5_ss_ax_store_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3b0_08c5_ss_ax_store_tests
             COMMAND poolrad_overlay_u3b0_08c5_ss_ax_store_tests)
    add_executable(poolrad_overlay_u3b0_08c8_ss_dx_store_tests
                   tests/recovered/test_overlay_u3b0_08c8_ss_dx_store.c)
    target_link_libraries(poolrad_overlay_u3b0_08c8_ss_dx_store_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3b0_08c8_ss_dx_store_tests
             COMMAND poolrad_overlay_u3b0_08c8_ss_dx_store_tests)
    add_executable(poolrad_overlay_u3b0_088f_add_ax_dx_tests
                   tests/recovered/test_overlay_u3b0_088f_add_ax_dx.c)
    target_link_libraries(poolrad_overlay_u3b0_088f_add_ax_dx_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3b0_088f_add_ax_dx_tests
             COMMAND poolrad_overlay_u3b0_088f_add_ax_dx_tests)
    add_executable(poolrad_overlay_u3b0_0891_push_ax_tests
                   tests/recovered/test_overlay_u3b0_0891_push_ax.c)
    target_link_libraries(poolrad_overlay_u3b0_0891_push_ax_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3b0_0891_push_ax_tests
             COMMAND poolrad_overlay_u3b0_0891_push_ax_tests)
    add_executable(poolrad_overlay_u3b0_0895_push_ax_tests
                   tests/recovered/test_overlay_u3b0_0895_push_ax.c)
    target_link_libraries(poolrad_overlay_u3b0_0895_push_ax_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3b0_0895_push_ax_tests
             COMMAND poolrad_overlay_u3b0_0895_push_ax_tests)
    add_executable(poolrad_overlay_u3b0_08cb_ss_inc_tests
                   tests/recovered/test_overlay_u3b0_08cb_ss_inc.c)
    target_link_libraries(poolrad_overlay_u3b0_08cb_ss_inc_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3b0_08cb_ss_inc_tests
             COMMAND poolrad_overlay_u3b0_08cb_ss_inc_tests)
    add_executable(poolrad_overlay_u3b0_08ce_jmp_tests
                   tests/recovered/test_overlay_u3b0_08ce_jmp.c)
    target_link_libraries(poolrad_overlay_u3b0_08ce_jmp_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3b0_08ce_jmp_tests
             COMMAND poolrad_overlay_u3b0_08ce_jmp_tests)
    add_executable(poolrad_overlay_u3b0_0892_ss_al_load_tests
                   tests/recovered/test_overlay_u3b0_0892_ss_al_load.c)
    target_link_libraries(poolrad_overlay_u3b0_0892_ss_al_load_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3b0_0892_ss_al_load_tests
             COMMAND poolrad_overlay_u3b0_0892_ss_al_load_tests)
    add_executable(poolrad_overlay_u3b0_0896_mov_al_zero_tests
                   tests/recovered/test_overlay_u3b0_0896_mov_al_zero.c)
    target_link_libraries(poolrad_overlay_u3b0_0896_mov_al_zero_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3b0_0896_mov_al_zero_tests
             COMMAND poolrad_overlay_u3b0_0896_mov_al_zero_tests)
    add_executable(poolrad_overlay_u3b0_0898_push_ax_tests
                   tests/recovered/test_overlay_u3b0_0898_push_ax.c)
    target_link_libraries(poolrad_overlay_u3b0_0898_push_ax_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3b0_0898_push_ax_tests
             COMMAND poolrad_overlay_u3b0_0898_push_ax_tests)
    add_executable(poolrad_overlay_u3b0_0899_les_di_tests
                   tests/recovered/test_overlay_u3b0_0899_les_di.c)
    target_link_libraries(poolrad_overlay_u3b0_0899_les_di_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3b0_0899_les_di_tests
             COMMAND poolrad_overlay_u3b0_0899_les_di_tests)
    add_executable(poolrad_overlay_u3b0_087d_les_di_tests
                   tests/recovered/test_overlay_u3b0_087d_les_di.c)
    target_link_libraries(poolrad_overlay_u3b0_087d_les_di_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3b0_087d_les_di_tests
             COMMAND poolrad_overlay_u3b0_087d_les_di_tests)
    add_executable(poolrad_overlay_u3b0_0877_cmp_al_tests
                   tests/recovered/test_overlay_u3b0_0877_cmp_al.c)
    target_link_libraries(poolrad_overlay_u3b0_0877_cmp_al_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3b0_0877_cmp_al_tests
             COMMAND poolrad_overlay_u3b0_0877_cmp_al_tests)
    add_executable(poolrad_overlay_u3b0_0874_di_load_tests
                   tests/recovered/test_overlay_u3b0_0874_di_load.c)
    target_link_libraries(poolrad_overlay_u3b0_0874_di_load_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3b0_0874_di_load_tests
             COMMAND poolrad_overlay_u3b0_0874_di_load_tests)
    add_executable(poolrad_overlay_u38_sidecar_flag_0918_tests
                   tests/recovered/test_overlay_u38_sidecar_flag_0918.c)
    target_link_libraries(poolrad_overlay_u38_sidecar_flag_0918_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u38_sidecar_flag_0918_tests
             COMMAND poolrad_overlay_u38_sidecar_flag_0918_tests)
    add_executable(poolrad_overlay_u38_target_select_101d_tests
                   tests/recovered/test_overlay_u38_target_select_101d.c)
    target_link_libraries(poolrad_overlay_u38_target_select_101d_tests
                          PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u38_target_select_101d_tests
             COMMAND poolrad_overlay_u38_target_select_101d_tests)
    add_executable(poolrad_overlay_u38_roll_dispatch_1699_tests
                   tests/recovered/test_overlay_u38_roll_dispatch_1699.c)
    target_link_libraries(poolrad_overlay_u38_roll_dispatch_1699_tests
                          PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u38_roll_dispatch_1699_tests
             COMMAND poolrad_overlay_u38_roll_dispatch_1699_tests)
    add_executable(poolrad_overlay_u38_target_reset_2b7d_tests
                   tests/recovered/test_overlay_u38_target_reset_2b7d.c)
    target_link_libraries(poolrad_overlay_u38_target_reset_2b7d_tests
                          PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u38_target_reset_2b7d_tests
             COMMAND poolrad_overlay_u38_target_reset_2b7d_tests)
    add_executable(poolrad_overlay_u38_link_message_2bc8_tests
                   tests/recovered/test_overlay_u38_link_message_2bc8.c)
    target_link_libraries(poolrad_overlay_u38_link_message_2bc8_tests
                          PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u38_link_message_2bc8_tests
             COMMAND poolrad_overlay_u38_link_message_2bc8_tests)
    add_executable(poolrad_overlay_u38_target_gate_2ae6_tests
                   tests/recovered/test_overlay_u38_target_gate_2ae6.c)
    target_link_libraries(poolrad_overlay_u38_target_gate_2ae6_tests
                          PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u38_target_gate_2ae6_tests
             COMMAND poolrad_overlay_u38_target_gate_2ae6_tests)
    add_executable(poolrad_overlay_u38_link_roll_2b20_tests
                   tests/recovered/test_overlay_u38_link_roll_2b20.c)
    target_link_libraries(poolrad_overlay_u38_link_roll_2b20_tests
                          PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u38_link_roll_2b20_tests
             COMMAND poolrad_overlay_u38_link_roll_2b20_tests)
    add_executable(poolrad_overlay_u1c_vertical_menu_one_node_0f73_tests
                   tests/recovered/test_overlay_u1c_vertical_menu_one_node_0f73.c)
    target_link_libraries(poolrad_overlay_u1c_vertical_menu_one_node_0f73_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u1c_vertical_menu_one_node_0f73_tests
             COMMAND poolrad_overlay_u1c_vertical_menu_one_node_0f73_tests)
    add_executable(poolrad_overlay_u1c_vertical_menu_null_head_0f73_tests
                   tests/recovered/test_overlay_u1c_vertical_menu_null_head_0f73.c)
    target_link_libraries(poolrad_overlay_u1c_vertical_menu_null_head_0f73_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u1c_vertical_menu_null_head_0f73_tests
             COMMAND poolrad_overlay_u1c_vertical_menu_null_head_0f73_tests)
    add_executable(poolrad_overlay_u1c_vertical_menu_two_node_0f73_tests
                   tests/recovered/test_overlay_u1c_vertical_menu_two_node_0f73.c)
    target_link_libraries(poolrad_overlay_u1c_vertical_menu_two_node_0f73_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u1c_vertical_menu_two_node_0f73_tests
             COMMAND poolrad_overlay_u1c_vertical_menu_two_node_0f73_tests)
    add_executable(poolrad_overlay_u68_up_preflight_tests
                   tests/recovered/test_overlay_u68_up_preflight.c)
    target_link_libraries(poolrad_overlay_u68_up_preflight_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u68_up_preflight_tests
             COMMAND poolrad_overlay_u68_up_preflight_tests)
    add_executable(poolrad_overlay_u77_spell_299c_tests
                   tests/recovered/test_overlay_u77_spell_299c.c)
    target_link_libraries(poolrad_overlay_u77_spell_299c_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u77_spell_299c_tests
             COMMAND poolrad_overlay_u77_spell_299c_tests)
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
    add_executable(poolrad_overlay_u1c_key_branch_33bd_tests
                   tests/recovered/test_overlay_u1c_key_branch_33bd.c)
    target_link_libraries(poolrad_overlay_u1c_key_branch_33bd_tests
                          PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u1c_key_branch_33bd_tests
             COMMAND poolrad_overlay_u1c_key_branch_33bd_tests)
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
    add_executable(poolrad_overlay_u5e_half_0e52_tests
                   tests/recovered/test_overlay_u5e_half_0e52.c)
    target_link_libraries(poolrad_overlay_u5e_half_0e52_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u5e_half_0e52_tests
             COMMAND poolrad_overlay_u5e_half_0e52_tests)
    add_executable(poolrad_overlay_u3c6_mov_sp_bp_0062_tests
                   tests/recovered/test_u3c6_mov_sp_bp_0062.c)
    target_link_libraries(poolrad_overlay_u3c6_mov_sp_bp_0062_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3c6_mov_sp_bp_0062_tests
             COMMAND poolrad_overlay_u3c6_mov_sp_bp_0062_tests)
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
    add_executable(poolrad_resident_0709_delay_wrapper_0dbe_tests
                   tests/recovered/test_resident_0709_delay_wrapper_0dbe.c)
    target_link_libraries(poolrad_resident_0709_delay_wrapper_0dbe_tests
                          PRIVATE poolrad_recovered)
    add_test(NAME poolrad_resident_0709_delay_wrapper_0dbe_tests
             COMMAND poolrad_resident_0709_delay_wrapper_0dbe_tests)
    add_executable(poolrad_resident_a40_delay_029c_tests
                   tests/recovered/test_resident_a40_delay_029c.c)
    target_link_libraries(poolrad_resident_a40_delay_029c_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_resident_a40_delay_029c_tests
             COMMAND poolrad_resident_a40_delay_029c_tests)
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
    add_executable(poolrad_resident_video_init_4426_tests
                   tests/recovered/test_resident_video_init_4426.c)
    target_link_libraries(poolrad_resident_video_init_4426_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_resident_video_init_4426_tests
             COMMAND poolrad_resident_video_init_4426_tests)
    add_executable(poolrad_resident_palette_2a0d_tests
                   tests/recovered/test_resident_palette_2a0d.c)
    target_link_libraries(poolrad_resident_palette_2a0d_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_resident_palette_2a0d_tests
             COMMAND poolrad_resident_palette_2a0d_tests)
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
    add_executable(poolrad_overlay_u7d_carry_02f2_tests
                   tests/recovered/test_overlay_u7d_carry_02f2.c)
    target_link_libraries(poolrad_overlay_u7d_carry_02f2_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u7d_carry_02f2_tests
             COMMAND poolrad_overlay_u7d_carry_02f2_tests)
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
    add_executable(poolrad_overlay_u9e_entry9_roll_latch_0e2e_tests
                   tests/recovered/test_overlay_u9e_entry9_roll_latch_0e2e.c)
    target_link_libraries(poolrad_overlay_u9e_entry9_roll_latch_0e2e_tests
                          PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u9e_entry9_roll_latch_0e2e_tests
             COMMAND poolrad_overlay_u9e_entry9_roll_latch_0e2e_tests)
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
    add_executable(poolrad_slot_a_static_view_tests
                   tests/replay/test_slot_a_static_view.c
                   src/replay/slot_a_static_view.c
                   src/replay/slot_a_native_save.c)
    target_link_libraries(poolrad_slot_a_static_view_tests PRIVATE
                          poolrad_formats)
    add_test(NAME poolrad_slot_a_static_view_tests
             COMMAND poolrad_slot_a_static_view_tests)
    add_executable(poolrad_slot_a_static_view_probe
                   tools/replay/slot_a_static_view_probe.c
                   src/replay/slot_a_static_view.c
                   src/replay/slot_a_native_save.c)
    target_link_libraries(poolrad_slot_a_static_view_probe PRIVATE
                          poolrad_formats)
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
    add_executable(poolrad_slot_a_u6f_19ea_caller_tests
                   tests/recovered/test_slot_a_u6f_19ea_caller.c)
    target_link_libraries(poolrad_slot_a_u6f_19ea_caller_tests
                          PRIVATE poolrad_recovered)
    add_test(NAME poolrad_slot_a_u6f_19ea_caller_tests
             COMMAND poolrad_slot_a_u6f_19ea_caller_tests)
    add_executable(poolrad_slot_a_u6f_1937_precall_tests
                   tests/recovered/test_slot_a_u6f_1937_precall.c)
    target_link_libraries(poolrad_slot_a_u6f_1937_precall_tests
                          PRIVATE poolrad_recovered)
    add_test(NAME poolrad_slot_a_u6f_1937_precall_tests
             COMMAND poolrad_slot_a_u6f_1937_precall_tests)
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
    add_executable(poolrad_resident_time_caller_prefix_08ae_001f_tests
                   tests/recovered/test_resident_time_caller_prefix_08ae_001f.c)
    target_link_libraries(poolrad_resident_time_caller_prefix_08ae_001f_tests
                          PRIVATE poolrad_recovered)
    add_test(NAME poolrad_resident_time_caller_prefix_08ae_001f_tests
             COMMAND poolrad_resident_time_caller_prefix_08ae_001f_tests)
    add_executable(poolrad_resident_time_caller_return_08ae_002a_tests
                   tests/recovered/test_resident_time_caller_return_08ae_002a.c)
    target_link_libraries(poolrad_resident_time_caller_return_08ae_002a_tests
                          PRIVATE poolrad_recovered)
    add_test(NAME poolrad_resident_time_caller_return_08ae_002a_tests
             COMMAND poolrad_resident_time_caller_return_08ae_002a_tests)
    add_executable(poolrad_resident_time_caller_08ae_0031_tests
                   tests/recovered/test_resident_time_caller_08ae_0031.c)
    target_link_libraries(poolrad_resident_time_caller_08ae_0031_tests
                          PRIVATE poolrad_recovered)
    add_test(NAME poolrad_resident_time_caller_08ae_0031_tests
             COMMAND poolrad_resident_time_caller_08ae_0031_tests)
    add_executable(poolrad_resident_time_caller_composed_08ae_001f_tests
                   tests/recovered/test_resident_time_caller_composed_08ae_001f.c)
    target_link_libraries(poolrad_resident_time_caller_composed_08ae_001f_tests
                          PRIVATE poolrad_recovered)
    add_test(NAME poolrad_resident_time_caller_composed_08ae_001f_tests
             COMMAND poolrad_resident_time_caller_composed_08ae_001f_tests)
    add_executable(poolrad_resident_time_caller_08ae_0033_tests
                   tests/recovered/test_resident_time_caller_08ae_0033.c)
    target_link_libraries(poolrad_resident_time_caller_08ae_0033_tests
                          PRIVATE poolrad_recovered)
    add_test(NAME poolrad_resident_time_caller_08ae_0033_tests
             COMMAND poolrad_resident_time_caller_08ae_0033_tests)
    add_executable(poolrad_resident_time_caller_08ae_0036_tests
                   tests/recovered/test_resident_time_caller_08ae_0036.c)
    target_link_libraries(poolrad_resident_time_caller_08ae_0036_tests
                          PRIVATE poolrad_recovered)
    add_test(NAME poolrad_resident_time_caller_08ae_0036_tests
             COMMAND poolrad_resident_time_caller_08ae_0036_tests)
    add_executable(poolrad_resident_464_bitmask_tests
                   tests/recovered/test_resident_464_bitmask.c)
    target_link_libraries(poolrad_resident_464_bitmask_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_resident_464_bitmask_tests
             COMMAND poolrad_resident_464_bitmask_tests)
    add_executable(poolrad_resident_464_bitmask_frame_tests
                   tests/recovered/test_resident_464_bitmask_frame.c)
    target_link_libraries(poolrad_resident_464_bitmask_frame_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_resident_464_bitmask_frame_tests
             COMMAND poolrad_resident_464_bitmask_frame_tests)
    add_executable(poolrad_resident_464_masked_byte_0c3d_tests
                   tests/recovered/test_resident_464_masked_byte_0c3d.c)
    target_link_libraries(poolrad_resident_464_masked_byte_0c3d_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_resident_464_masked_byte_0c3d_tests
             COMMAND poolrad_resident_464_masked_byte_0c3d_tests)
    add_executable(poolrad_resident_464_loop_c6e_tests
                   tests/recovered/test_resident_464_loop_c6e.c)
    target_link_libraries(poolrad_resident_464_loop_c6e_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_resident_464_loop_c6e_tests
             COMMAND poolrad_resident_464_loop_c6e_tests)
    add_executable(poolrad_resident_464_reverse_1484_tests
                   tests/recovered/test_resident_464_reverse_1484.c)
    target_link_libraries(poolrad_resident_464_reverse_1484_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_resident_464_reverse_1484_tests
             COMMAND poolrad_resident_464_reverse_1484_tests)
    add_executable(poolrad_resident_464_row56_transfer_tests
                   tests/recovered/test_resident_464_row56_transfer.c)
    target_link_libraries(poolrad_resident_464_row56_transfer_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_resident_464_row56_transfer_tests
             COMMAND poolrad_resident_464_row56_transfer_tests)
    add_executable(poolrad_resident_464_dirty_transfer_tests
                   tests/recovered/test_resident_464_dirty_transfer.c)
    target_link_libraries(poolrad_resident_464_dirty_transfer_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_resident_464_dirty_transfer_tests
             COMMAND poolrad_resident_464_dirty_transfer_tests)
    add_executable(poolrad_resident_464_2632_run014_tests
                   tests/recovered/test_resident_464_2632_run014.c)
    target_link_libraries(poolrad_resident_464_2632_run014_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_resident_464_2632_run014_tests
             COMMAND poolrad_resident_464_2632_run014_tests)
    add_executable(poolrad_resident_464_2632_first_call_tests
                   tests/recovered/test_resident_464_2632_first_call.c)
    target_link_libraries(poolrad_resident_464_2632_first_call_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_resident_464_2632_first_call_tests
             COMMAND poolrad_resident_464_2632_first_call_tests)
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
    add_executable(poolrad_resident_loader_seek_028f_tests
                   tests/recovered/test_resident_loader_seek_028f.c)
    target_link_libraries(poolrad_resident_loader_seek_028f_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_resident_loader_seek_028f_tests
             COMMAND poolrad_resident_loader_seek_028f_tests)
    add_executable(poolrad_resident_loader_read_gate_0291_tests
                   tests/recovered/test_resident_loader_read_gate_0291.c)
    target_link_libraries(poolrad_resident_loader_read_gate_0291_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_resident_loader_read_gate_0291_tests
             COMMAND poolrad_resident_loader_read_gate_0291_tests)
    add_executable(poolrad_resident_loader_dos_read_029f_tests
                   tests/recovered/test_resident_loader_dos_read_029f.c)
    target_link_libraries(poolrad_resident_loader_dos_read_029f_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_resident_loader_dos_read_029f_tests
             COMMAND poolrad_resident_loader_dos_read_029f_tests)
    add_executable(poolrad_resident_loader_read_branch_02a1_tests
                   tests/recovered/test_resident_loader_read_branch_02a1.c)
    target_link_libraries(poolrad_resident_loader_read_branch_02a1_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_resident_loader_read_branch_02a1_tests
             COMMAND poolrad_resident_loader_read_branch_02a1_tests)
    add_executable(poolrad_resident_loader_composed_028f_tests
                   tests/recovered/test_resident_loader_composed_028f.c)
    target_link_libraries(poolrad_resident_loader_composed_028f_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_resident_loader_composed_028f_tests
             COMMAND poolrad_resident_loader_composed_028f_tests)
    add_executable(poolrad_resident_loader_0355_tests
                   tests/recovered/test_resident_loader_0355.c)
    target_link_libraries(poolrad_resident_loader_0355_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_resident_loader_0355_tests
             COMMAND poolrad_resident_loader_0355_tests)
    add_executable(poolrad_font_8x8d_201_tests
                   tests/recovered/test_font_8x8d_201.c
                   src/replay/slot_a_static_view.c
                   src/replay/slot_a_native_save.c)
    target_link_libraries(poolrad_font_8x8d_201_tests PRIVATE
                          poolrad_recovered poolrad_formats)
    add_test(NAME poolrad_font_8x8d_201_tests
             COMMAND poolrad_font_8x8d_201_tests)
    add_executable(poolrad_slot_a_printer_page_tests
                   tests/recovered/test_slot_a_printer_page.c)
    target_link_libraries(poolrad_slot_a_printer_page_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_slot_a_printer_page_tests
             COMMAND poolrad_slot_a_printer_page_tests)
    add_executable(poolrad_slot_a_u3b0_prompt_row_tests
                   tests/recovered/test_slot_a_u3b0_prompt_row.c)
    target_link_libraries(poolrad_slot_a_u3b0_prompt_row_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_slot_a_u3b0_prompt_row_tests
             COMMAND poolrad_slot_a_u3b0_prompt_row_tests)
    add_executable(poolrad_slot_a_u3b0_prompt_row_probe
                   tools/replay/slot_a_u3b0_prompt_row_probe.c)
    target_link_libraries(poolrad_slot_a_u3b0_prompt_row_probe PRIVATE
                          poolrad_recovered)
    add_executable(poolrad_slot_a_second_cycle_zero_row_tests
                   tests/recovered/test_slot_a_second_cycle_zero_row.c)
    target_link_libraries(poolrad_slot_a_second_cycle_zero_row_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_slot_a_second_cycle_zero_row_tests
             COMMAND poolrad_slot_a_second_cycle_zero_row_tests)
    add_executable(poolrad_resident_0709_rect_fill_00b7_tests
                   tests/recovered/test_resident_0709_rect_fill_00b7.c)
    target_link_libraries(poolrad_resident_0709_rect_fill_00b7_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_resident_0709_rect_fill_00b7_tests
             COMMAND poolrad_resident_0709_rect_fill_00b7_tests)
    add_executable(poolrad_resident_0709_rect_fill_probe
                   tools/replay/resident_0709_rect_fill_probe.c)
    target_link_libraries(poolrad_resident_0709_rect_fill_probe PRIVATE
                          poolrad_recovered)
    add_executable(poolrad_slot_a_third_printer_cells_tests
                   tests/replay/test_slot_a_third_printer_cells.c)
    target_link_libraries(poolrad_slot_a_third_printer_cells_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_slot_a_third_printer_cells_tests
             COMMAND poolrad_slot_a_third_printer_cells_tests)
    add_executable(poolrad_slot_a_row5_col38_printer_tests
                   tests/replay/test_slot_a_row5_col38_printer.c)
    target_link_libraries(poolrad_slot_a_row5_col38_printer_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_slot_a_row5_col38_printer_tests
             COMMAND poolrad_slot_a_row5_col38_printer_tests)
    add_executable(poolrad_slot_a_six_cell_printer_tests
                   tests/replay/test_slot_a_six_cell_printer.c)
    target_link_libraries(poolrad_slot_a_six_cell_printer_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_slot_a_six_cell_printer_tests
             COMMAND poolrad_slot_a_six_cell_printer_tests)
    add_executable(poolrad_slot_a_row5_one_cell_printer_tests
                   tests/replay/test_slot_a_row5_one_cell_printer.c)
    target_link_libraries(poolrad_slot_a_row5_one_cell_printer_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_slot_a_row5_one_cell_printer_tests
             COMMAND poolrad_slot_a_row5_one_cell_printer_tests)
    add_executable(poolrad_slot_a_row6_eight_cell_printer_tests
                   tests/replay/test_slot_a_row6_eight_cell_printer.c)
    target_link_libraries(poolrad_slot_a_row6_eight_cell_printer_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_slot_a_row6_eight_cell_printer_tests
             COMMAND poolrad_slot_a_row6_eight_cell_printer_tests)
    add_executable(poolrad_slot_a_row6_col34_printer_tests
                   tests/replay/test_slot_a_row6_col34_printer.c)
    target_link_libraries(poolrad_slot_a_row6_col34_printer_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_slot_a_row6_col34_printer_tests
             COMMAND poolrad_slot_a_row6_col34_printer_tests)
    add_executable(poolrad_slot_a_row6_col38_printer_tests
                   tests/replay/test_slot_a_row6_col38_printer.c)
    target_link_libraries(poolrad_slot_a_row6_col38_printer_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_slot_a_row6_col38_printer_tests
             COMMAND poolrad_slot_a_row6_col38_printer_tests)
    add_executable(poolrad_slot_a_row7_twelve_cell_printer_tests
                   tests/replay/test_slot_a_row7_twelve_cell_printer.c)
    target_link_libraries(poolrad_slot_a_row7_twelve_cell_printer_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_slot_a_row7_twelve_cell_printer_tests
             COMMAND poolrad_slot_a_row7_twelve_cell_printer_tests)
    add_executable(poolrad_slot_a_printer_page_probe
                   tools/replay/slot_a_printer_page_probe.c)
    target_link_libraries(poolrad_slot_a_printer_page_probe PRIVATE
                          poolrad_recovered)
    add_executable(poolrad_slot_a_row7_col34_printer_tests
                   tests/replay/test_slot_a_row7_col34_printer.c)
    target_link_libraries(poolrad_slot_a_row7_col34_printer_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_slot_a_row7_col34_printer_tests
             COMMAND poolrad_slot_a_row7_col34_printer_tests)
    add_executable(poolrad_slot_a_row2_title_page_tests
                   tests/replay/test_slot_a_row2_title_page.c)
    target_link_libraries(poolrad_slot_a_row2_title_page_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_slot_a_row2_title_page_tests
             COMMAND poolrad_slot_a_row2_title_page_tests)
    add_executable(poolrad_slot_a_row2_title_page_probe
                   tools/replay/slot_a_printer_page_probe.c)
    target_compile_definitions(poolrad_slot_a_row2_title_page_probe PRIVATE
                               PR_PAGE_BASE=0 PR_PAGE_BYTES=8192)
    target_link_libraries(poolrad_slot_a_row2_title_page_probe PRIVATE
                          poolrad_recovered)
    add_executable(poolrad_slot_a_row7_col37_two_cell_printer_tests
                   tests/replay/test_slot_a_row7_col37_two_cell_printer.c)
    target_link_libraries(poolrad_slot_a_row7_col37_two_cell_printer_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_slot_a_row7_col37_two_cell_printer_tests
             COMMAND poolrad_slot_a_row7_col37_two_cell_printer_tests)
    add_executable(poolrad_slot_a_row8_eight_cell_printer_tests
                   tests/replay/test_slot_a_row8_eight_cell_printer.c)
    target_link_libraries(poolrad_slot_a_row8_eight_cell_printer_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_slot_a_row8_eight_cell_printer_tests
             COMMAND poolrad_slot_a_row8_eight_cell_printer_tests)
    add_executable(poolrad_slot_a_row8_col34_printer_tests
                   tests/replay/test_slot_a_row8_col34_printer.c)
    target_link_libraries(poolrad_slot_a_row8_col34_printer_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_slot_a_row8_col34_printer_tests
             COMMAND poolrad_slot_a_row8_col34_printer_tests)
    add_executable(poolrad_slot_a_row8_col38_printer_tests
                   tests/replay/test_slot_a_row8_col38_printer.c)
    target_link_libraries(poolrad_slot_a_row8_col38_printer_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_slot_a_row8_col38_printer_tests
             COMMAND poolrad_slot_a_row8_col38_printer_tests)
    add_executable(poolrad_slot_a_row2_second_title_page_tests
                   tests/replay/test_slot_a_row2_second_title_page.c)
    target_link_libraries(poolrad_slot_a_row2_second_title_page_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_slot_a_row2_second_title_page_tests
             COMMAND poolrad_slot_a_row2_second_title_page_tests)
    add_executable(poolrad_slot_a_glyph_plane_byte_tests
                   tests/recovered/test_slot_a_glyph_plane_byte.c)
    target_link_libraries(poolrad_slot_a_glyph_plane_byte_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_slot_a_glyph_plane_byte_tests
             COMMAND poolrad_slot_a_glyph_plane_byte_tests)
    add_executable(poolrad_slot_a_glyph_plane_probe
                   tools/replay/slot_a_glyph_plane_probe.c)
    target_link_libraries(poolrad_slot_a_glyph_plane_probe PRIVATE
                          poolrad_recovered)
    add_executable(poolrad_slot_a_caller_to_entry1_tests
                   tests/replay/test_slot_a_caller_to_entry1.c
                   src/replay/slot_a_caller_to_entry1.c)
    target_include_directories(poolrad_slot_a_caller_to_entry1_tests PRIVATE
                               ${CMAKE_CURRENT_SOURCE_DIR}/include)
    add_test(NAME poolrad_slot_a_caller_to_entry1_tests
             COMMAND poolrad_slot_a_caller_to_entry1_tests)
    add_executable(poolrad_slot_a_row9_seven_cell_printer_tests
                   tests/replay/test_slot_a_row9_seven_cell_printer.c)
    target_link_libraries(poolrad_slot_a_row9_seven_cell_printer_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_slot_a_row9_seven_cell_printer_tests
             COMMAND poolrad_slot_a_row9_seven_cell_printer_tests)
    add_executable(poolrad_slot_a_row9_col34_printer_tests
                   tests/replay/test_slot_a_row9_col34_printer.c)
    target_link_libraries(poolrad_slot_a_row9_col34_printer_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_slot_a_row9_col34_printer_tests
             COMMAND poolrad_slot_a_row9_col34_printer_tests)
    add_executable(poolrad_slot_a_row9_col38_printer_tests
                   tests/replay/test_slot_a_row9_col38_printer.c)
    target_link_libraries(poolrad_slot_a_row9_col38_printer_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_slot_a_row9_col38_printer_tests
             COMMAND poolrad_slot_a_row9_col38_printer_tests)
    add_executable(poolrad_slot_a_row9_three_call_composition_tests
                   tests/replay/test_slot_a_row9_three_call_composition.c)
    target_link_libraries(poolrad_slot_a_row9_three_call_composition_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_slot_a_row9_three_call_composition_tests
             COMMAND poolrad_slot_a_row9_three_call_composition_tests)
    add_executable(poolrad_slot_a_after_row9_full_page_tests
                   tests/replay/test_slot_a_after_row9_full_page.c)
    target_link_libraries(poolrad_slot_a_after_row9_full_page_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_slot_a_after_row9_full_page_tests
             COMMAND poolrad_slot_a_after_row9_full_page_tests)
    add_executable(poolrad_slot_a_full_ega_printer_page_probe
                   tools/replay/slot_a_printer_page_probe.c)
    target_compile_definitions(poolrad_slot_a_full_ega_printer_page_probe
                               PRIVATE PR_PAGE_BASE=0 PR_PAGE_BYTES=8000)
    target_link_libraries(poolrad_slot_a_full_ega_printer_page_probe PRIVATE
                          poolrad_recovered)
    add_executable(poolrad_slot_a_second_cycle_compose_probe
                   tools/replay/slot_a_second_cycle_compose_probe.c)
    target_link_libraries(poolrad_slot_a_second_cycle_compose_probe PRIVATE
                          poolrad_recovered)
    add_executable(poolrad_slot_a_two_cycle_reset_compose_probe
                   tools/replay/slot_a_two_cycle_reset_compose_probe.c)
    target_link_libraries(poolrad_slot_a_two_cycle_reset_compose_probe PRIVATE
                          poolrad_recovered)
    add_executable(poolrad_slot_a_res464_second_flush_tests
                   tests/replay/test_slot_a_res464_second_flush.c)
    target_link_libraries(poolrad_slot_a_res464_second_flush_tests PRIVATE
                          poolrad_recovered)
    add_test(NAME poolrad_slot_a_res464_second_flush_tests
             COMMAND poolrad_slot_a_res464_second_flush_tests)
endif()
