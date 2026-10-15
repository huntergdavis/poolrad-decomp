#ifndef POOLRAD_RECOVERED_OVERLAY_U3C2_JUMP_ONLY_05B5_FIXTURE_H
#define POOLRAD_RECOVERED_OVERLAY_U3C2_JUMP_ONLY_05B5_FIXTURE_H
#include "poolrad/recovered/overlay_u6c_record_table_sum_3f31.h"
#define PR_U3C2_05B5_ENTRY 0x05B5u
#define PR_U3C2_05B5_STOP 0x0900u
typedef enum { PR_U3C2_05B5_OK = 0, PR_U3C2_05B5_ARGUMENT } pr_u3c2_jump_only_05b5_status;
typedef enum { PR_U3C2_05B5_AF_PRESERVE_SUPPLIED = 1 } pr_u3c2_05b5_af_policy;
/* JMP +0900 only; host explicitly preserves supplied AF, whose native value is unknown.
 * Stop before 0900; full 1 MiB RAM and distinct 14-word CPU.
 * Code immutable; no async/debug exceptions. Invalid inputs reject without mutation.
 * No predecessor reach, successor effect or return is claimed. */
pr_u3c2_jump_only_05b5_status pr_u3c2_jump_only_05b5_fixture(
    uint8_t *ram, size_t ram_size, pr_u6c_3f31_cpu *cpu, pr_u3c2_05b5_af_policy af_policy);
#endif
