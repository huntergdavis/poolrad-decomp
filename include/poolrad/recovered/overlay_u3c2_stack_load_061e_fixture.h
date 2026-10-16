#ifndef POOLRAD_RECOVERED_OVERLAY_U3C2_STACK_LOAD_061E_FIXTURE_H
#define POOLRAD_RECOVERED_OVERLAY_U3C2_STACK_LOAD_061E_FIXTURE_H
#include "poolrad/recovered/overlay_u6c_record_table_sum_3f31.h"
#define PR_U3C2_061E_ENTRY 0x061Eu
typedef enum { PR_U3C2_061E_OK = 0, PR_U3C2_061E_ARGUMENT } pr_u3c2_stack_load_061e_status;
typedef enum { PR_U3C2_061E_AF_PRESERVE_SUPPLIED = 1 } pr_u3c2_061e_af_policy;
/* MOV AL,SS:[BP+0A] only; stops before 0621. Mandatory host policy
 * preserves supplied AF and all FLAGS; upstream native AF is unknown.
 * Raw 1 MiB RAM and distinct 14-word CPU. Code immutable, no async/debug traps.
 * Invalid pointers/size/IP or unsupported policy reject unchanged.
 * No predecessor consistency, successor effect, result or return is claimed. */
pr_u3c2_stack_load_061e_status pr_u3c2_stack_load_061e_fixture(
    uint8_t *ram, size_t ram_size, pr_u6c_3f31_cpu *cpu, pr_u3c2_061e_af_policy af_policy);
#endif
