#ifndef POOLRAD_RECOVERED_OVERLAY_U7F_RECORD_WORD_ADD_003A_H
#define POOLRAD_RECOVERED_OVERLAY_U7F_RECORD_WORD_ADD_003A_H
#include "poolrad/recovered/overlay_u6c_record_table_sum_3f31.h"
#define PR_U7F_003A_RAM_SIZE PR_U6C_3F31_RAM_SIZE
typedef pr_u6c_3f31_cpu pr_u7f_003a_cpu;
typedef enum { PR_U7F_003A_OK = 0, PR_U7F_003A_ARGUMENT } pr_u7f_003a_status;
/* Complete +003A..0053, after CALL: increment word SS:SP+4, far record
 * SS:SP+6. Add to record word +0102, returning AX and exact ADD FLAGS;
 * reload far pointer before store, POP BP and RETF6 through live RAM.
 * Offsets/SP wrap16, contiguous operands/A20-off RAM wrap20. Immutable
 * code is required. All data/frame aliases and changed outer returns
 * remain visible. No loader or natural-game invocation is claimed. */
pr_u7f_003a_status pr_u7f_record_word_add_003a(
    uint8_t *ram, size_t ram_size, pr_u7f_003a_cpu *cpu);
#endif
