#ifndef POOLRAD_RECOVERED_OVERLAY_U3C2_JLE_DECISION_05AF_FIXTURE_H
#define POOLRAD_RECOVERED_OVERLAY_U3C2_JLE_DECISION_05AF_FIXTURE_H
#include "poolrad/recovered/overlay_u6c_record_table_sum_3f31.h"
#define PR_U3C2_05AF_ENTRY 0x05AFu
typedef enum { PR_U3C2_05AF_OK = 0, PR_U3C2_05AF_ARGUMENT } pr_u3c2_jle_decision_05af_status;
/* JLE from supplied flags; stop before 05B8 or 05B1.
 * Raw 1MiB RAM and distinct 14-word CPU; code immutable, no async/debug traps.
 * Null/short RAM/wrong entry reject without mutation.
 * All supplied FLAGS, including AF, are retained; no native AF value inferred.
 * No predecessor reach, successor effect or return is claimed. */
pr_u3c2_jle_decision_05af_status pr_u3c2_jle_decision_05af_fixture(
    uint8_t *ram, size_t ram_size, pr_u6c_3f31_cpu *cpu);
#endif
