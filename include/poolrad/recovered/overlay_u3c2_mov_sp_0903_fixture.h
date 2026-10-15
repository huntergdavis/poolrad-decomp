#ifndef POOLRAD_RECOVERED_OVERLAY_U3C2_MOV_SP_0903_FIXTURE_H
#define POOLRAD_RECOVERED_OVERLAY_U3C2_MOV_SP_0903_FIXTURE_H
#include "poolrad/recovered/overlay_u6c_record_table_sum_3f31.h"
#define PR_U3C2_0903_ENTRY 0x0903u
#define PR_U3C2_0903_STOP 0x0905u
typedef enum { PR_U3C2_0903_OK = 0, PR_U3C2_0903_ARGUMENT } pr_u3c2_mov_sp_0903_status;
typedef enum { PR_U3C2_0903_AF_PRESERVE_SUPPLIED = 1 } pr_u3c2_0903_af_policy;
/* MOV SP,BP only; host explicitly preserves supplied AF, whose native value is unknown.
 * Stop before 0905; full 1 MiB RAM and distinct 14-word CPU.
 * Code immutable; no async/debug exceptions. Invalid inputs reject without mutation.
 * No predecessor reach, successor effect or return is claimed. */
pr_u3c2_mov_sp_0903_status pr_u3c2_mov_sp_0903_fixture(
    uint8_t *ram, size_t ram_size, pr_u6c_3f31_cpu *cpu, pr_u3c2_0903_af_policy af_policy);
#endif
