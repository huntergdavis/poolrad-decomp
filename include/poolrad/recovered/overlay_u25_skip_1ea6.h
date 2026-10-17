#ifndef POOLRAD_RECOVERED_OVERLAY_U25_SKIP_1EA6_H
#define POOLRAD_RECOVERED_OVERLAY_U25_SKIP_1EA6_H

#include "poolrad/recovered/overlay_u25_loader_0000.h"

#define PR_U25_1EA6_RAM_SIZE PR_U25_0000_RAM_SIZE
typedef pr_u25_0000_cpu pr_u25_1ea6_cpu;
typedef enum {
    PR_U25_1EA6_OK = 0,
    PR_U25_1EA6_ARGUMENT,
    PR_U25_1EA6_OPERAND_LIMIT,
    PR_U25_1EA6_STRING_LIMIT,
    PR_U25_1EA6_TRANSFER,
    PR_U25_1EA6_CALLEE
} pr_u25_1ea6_status;

/* Complete U25 entry 22, [1EA6,1FB1): fetch/latch an opcode and skip its
 * statement using the original +0000 operand loader and string callees.
 * Enter IP=1EA6 after far CALL, with return IP/CS at SS:SP. No arguments;
 * RETF. All 256 opcode values retain the original count/fallback behavior.
 *
 * RAM must be exactly 1 MiB, synchronous, distinct from CPU. Loaded code
 * and SYSTEM binding remain immutable; data writes may not overlap code.
 * system_cs is the loaded SYSTEM segment. Effective offsets wrap16, word
 * and LES bytes are physically contiguous with A20 off. Initial SP must
 * be in [0232,FFFC], reserving the complete nested stack without wrap.
 *
 * Both budgets must be positive. OPERAND_LIMIT and STRING_LIMIT expose
 * the reached loader/string state, using the +0000 contract's budgets.
 * TRANSFER exposes an unexpected nested return; CALLEE exposes a rejected
 * nested entry. No hook, successful return or termination is synthesized.
 * Undefined flags follow the composed loader's convention. Invalid initial
 * arguments reject atomically. Native route parity is a separate gate.
 */
pr_u25_1ea6_status pr_u25_skip_statement_1ea6(
    uint8_t *ram, size_t ram_size, pr_u25_1ea6_cpu *cpu,
    uint16_t system_cs, size_t max_operands, size_t max_string_iterations);

#endif
