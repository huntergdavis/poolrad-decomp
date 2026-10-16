#ifndef POOLRAD_RECOVERED_OVERLAY_U3C2_JE_DECISION_05FE_FIXTURE_H
#define POOLRAD_RECOVERED_OVERLAY_U3C2_JE_DECISION_05FE_FIXTURE_H
#include "poolrad/recovered/overlay_u6c_record_table_sum_3f31.h"
#define PR_U3C2_05FE_ENTRY 0x05FEu
typedef enum { PR_U3C2_05FE_OK = 0, PR_U3C2_05FE_ARGUMENT } pr_u3c2_je_decision_05fe_status;
/* JE from supplied flags; stop before 0610 or 0600.
 * Raw 1MiB RAM and distinct 14-word CPU; code immutable, no async/debug traps.
 * Null/short RAM/wrong entry reject without mutation.
 * All supplied FLAGS, including AF, are retained; no native AF value inferred.
 * No predecessor reach, successor effect or return is claimed. */
pr_u3c2_je_decision_05fe_status pr_u3c2_je_decision_05fe_fixture(
    uint8_t *ram, size_t ram_size, pr_u6c_3f31_cpu *cpu);
#endif
