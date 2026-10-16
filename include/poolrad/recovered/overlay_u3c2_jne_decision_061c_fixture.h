#ifndef POOLRAD_RECOVERED_OVERLAY_U3C2_JNE_DECISION_061C_FIXTURE_H
#define POOLRAD_RECOVERED_OVERLAY_U3C2_JNE_DECISION_061C_FIXTURE_H
#include "poolrad/recovered/overlay_u6c_record_table_sum_3f31.h"
#define PR_U3C2_061C_ENTRY 0x061cu
typedef enum { PR_U3C2_061C_OK = 0, PR_U3C2_061C_ARGUMENT } pr_u3c2_jne_decision_061c_status;
/* JNE from supplied flags; stop before 0676 or 061E.
 * Raw 1MiB RAM and distinct 14-word CPU; code immutable, no async/debug traps.
 * Null/short RAM/wrong entry reject without mutation.
 * All supplied FLAGS, including AF, are retained; no native AF value inferred.
 * No predecessor reach, successor effect or return is claimed. */
pr_u3c2_jne_decision_061c_status pr_u3c2_jne_decision_061c_fixture(
    uint8_t *ram, size_t ram_size, pr_u6c_3f31_cpu *cpu);
#endif
