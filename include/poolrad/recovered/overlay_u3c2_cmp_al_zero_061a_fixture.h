#ifndef POOLRAD_RECOVERED_OVERLAY_U3C2_CMP_AL_ZERO_061A_FIXTURE_H
#define POOLRAD_RECOVERED_OVERLAY_U3C2_CMP_AL_ZERO_061A_FIXTURE_H
#include "poolrad/recovered/overlay_u6c_record_table_sum_3f31.h"
#define PR_U3C2_061A_ENTRY 0x061Au
#define PR_U3C2_061A_STOP 0x061Cu
typedef enum { PR_U3C2_061A_OK = 0, PR_U3C2_061A_ARGUMENT } pr_u3c2_cmp_al_zero_061a_status;
typedef enum { PR_U3C2_061A_AF_CMP_DEFINED = 1 } pr_u3c2_061a_af_policy;
/* CMP AL,0 only; stop before 061C conditional.
 * Mandatory policy sets CF/AF/OF=0, computes PF/ZF/SF; other FLAGS retained.
 * Raw >=1 MiB RAM and distinct 14-word CPU; RAM is retained; no memory operand is read.
 * Code immutable, no async/debug traps. Invalid inputs reject unchanged.
 * No predecessor consistency, branch effect, frame join or return is claimed. */
pr_u3c2_cmp_al_zero_061a_status pr_u3c2_cmp_al_zero_061a_fixture(
    uint8_t *ram, size_t ram_size, pr_u6c_3f31_cpu *cpu, pr_u3c2_061a_af_policy af_policy);
#endif
