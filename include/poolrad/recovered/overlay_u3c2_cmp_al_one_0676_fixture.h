#ifndef POOLRAD_RECOVERED_OVERLAY_U3C2_CMP_AL_ONE_0676_FIXTURE_H
#define POOLRAD_RECOVERED_OVERLAY_U3C2_CMP_AL_ONE_0676_FIXTURE_H
#include "poolrad/recovered/overlay_u6c_record_table_sum_3f31.h"
#define PR_U3C2_0676_ENTRY 0x0676u
#define PR_U3C2_0676_STOP 0x0678u
typedef enum { PR_U3C2_0676_OK = 0, PR_U3C2_0676_ARGUMENT } pr_u3c2_cmp_al_one_0676_status;
typedef enum { PR_U3C2_0676_AF_CMP_DEFINED = 1 } pr_u3c2_0676_af_policy;
/* CMP AL,1 only; stop before 0678 conditional.
 * Mandatory policy computes CF/PF/AF/ZF/SF/OF; other FLAGS retained.
 * Raw >=1 MiB RAM and distinct 14-word CPU; RAM is retained; no memory operand is read.
 * Code immutable, no async/debug traps. Invalid inputs reject unchanged.
 * No predecessor consistency, branch effect, frame join or return is claimed. */
pr_u3c2_cmp_al_one_0676_status pr_u3c2_cmp_al_one_0676_fixture(
    uint8_t *ram, size_t ram_size, pr_u6c_3f31_cpu *cpu, pr_u3c2_0676_af_policy af_policy);
#endif
