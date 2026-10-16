#ifndef POOLRAD_RECOVERED_OVERLAY_U3C2_JNE_DECISION_05F6_FIXTURE_H
#define POOLRAD_RECOVERED_OVERLAY_U3C2_JNE_DECISION_05F6_FIXTURE_H
#include "poolrad/recovered/overlay_u6c_record_table_sum_3f31.h"
#define PR_U3C2_05F6_ENTRY 0x05F6u
typedef enum { PR_U3C2_05F6_OK = 0, PR_U3C2_05F6_ARGUMENT } pr_u3c2_jne_decision_05f6_status;
/* JNE from supplied flags; stop before 0600 or 05F8.
 * Raw 1MiB RAM and distinct 14-word CPU; code immutable, no async/debug traps.
 * Null/short RAM/wrong entry reject without mutation.
 * All supplied FLAGS, including AF, are retained; no native AF value inferred.
 * No predecessor reach, successor effect or return is claimed. */
pr_u3c2_jne_decision_05f6_status pr_u3c2_jne_decision_05f6_fixture(
    uint8_t *ram, size_t ram_size, pr_u6c_3f31_cpu *cpu);
#endif
