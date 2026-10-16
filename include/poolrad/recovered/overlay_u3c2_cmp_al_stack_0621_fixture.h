#ifndef POOLRAD_RECOVERED_OVERLAY_U3C2_CMP_AL_STACK_0621_FIXTURE_H
#define POOLRAD_RECOVERED_OVERLAY_U3C2_CMP_AL_STACK_0621_FIXTURE_H
#include "poolrad/recovered/overlay_u6c_record_table_sum_3f31.h"
#define PR_U3C2_0621_ENTRY 0x0621u
#define PR_U3C2_0621_STOP 0x0624u
typedef enum { PR_U3C2_0621_OK = 0, PR_U3C2_0621_ARGUMENT } pr_u3c2_cmp_al_stack_0621_status;
typedef enum { PR_U3C2_0621_AF_CMP_DEFINED = 1 } pr_u3c2_0621_af_policy;
/* CMP AL,SS:[BP-02] only; stop before 0624 conditional.
 * Mandatory policy computes defined CF/PF/AF/ZF/SF/OF; other FLAGS retained.
 * Raw >=1 MiB RAM and distinct 14-word CPU; 16-bit offset wrap and A20 mask.
 * Code immutable, no async/debug traps. Invalid inputs reject unchanged.
 * No predecessor consistency, branch effect, frame join or return is claimed. */
pr_u3c2_cmp_al_stack_0621_status pr_u3c2_cmp_al_stack_0621_fixture(
    uint8_t *ram, size_t ram_size, pr_u6c_3f31_cpu *cpu, pr_u3c2_0621_af_policy af_policy);
#endif
