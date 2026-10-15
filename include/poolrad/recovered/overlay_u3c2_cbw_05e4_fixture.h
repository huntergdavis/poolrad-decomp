#ifndef POOLRAD_RECOVERED_OVERLAY_U3C2_CBW_05E4_FIXTURE_H
#define POOLRAD_RECOVERED_OVERLAY_U3C2_CBW_05E4_FIXTURE_H
#include "poolrad/recovered/overlay_u6c_record_table_sum_3f31.h"
#define PR_U3C2_05E4_ENTRY 0x05E4u
#define PR_U3C2_05E4_STOP 0x05E5u
typedef enum { PR_U3C2_05E4_OK = 0, PR_U3C2_05E4_ARGUMENT } pr_u3c2_cbw_05e4_status;
typedef enum { PR_U3C2_05E4_AF_PRESERVE_SUPPLIED = 1 } pr_u3c2_05e4_af_policy;
/* CBW (opcode 98 in 16-bit mode) only; host explicitly preserves supplied AF, whose native value is unknown.
 * Stop before 05E5; full 1 MiB RAM and distinct 14-word CPU.
 * Code immutable; no async/debug exceptions. Invalid inputs reject without mutation.
 * No predecessor reach, successor effect or return is claimed. */
pr_u3c2_cbw_05e4_status pr_u3c2_cbw_05e4_fixture(
    uint8_t *ram, size_t ram_size, pr_u6c_3f31_cpu *cpu, pr_u3c2_05e4_af_policy af_policy);
#endif
