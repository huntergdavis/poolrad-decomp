#ifndef POOLRAD_RECOVERED_OVERLAY_U6C_RECORD_ROLL_SUM_4235_H
#define POOLRAD_RECOVERED_OVERLAY_U6C_RECORD_ROLL_SUM_4235_H

#include "poolrad/recovered/overlay_u6c_record_table_sum_3f31.h"

#define PR_U6C_4235_RAM_SIZE PR_U6C_3F31_RAM_SIZE
typedef pr_u6c_3f31_cpu pr_u6c_4235_cpu;
typedef struct {
    uint16_t dice_cs;   /* Loaded U9E entry 8 body; transparent stub at +0048. */
    uint16_t system_cs; /* Loaded SYSTEM +11C0, +120F and constant +1245. */
} pr_u6c_4235_layout;
typedef enum {
    PR_U6C_4235_OK = 0,
    PR_U6C_4235_ARGUMENT,
    PR_U6C_4235_TRANSFER
} pr_u6c_4235_status;

/* Complete U6C +4235..42EC and reached U9E/SYSTEM bodies. Enter after CALL:
 * mask word at SS:SP+4, far record at SS:SP+6; RETF 6 returns byte sum in AL.
 * No result callbacks: DS:4428/442A seed writes and nested call frames use
 * RAM, including aliases. Offsets wrap at 16 bits, physical bytes at 20 bits.
 * Code/constant and loaded entry layout must stay immutable and disjoint
 * from writes. Loader dispatch is outside this contract. TRANSFER stops at
 * the first changed internal return CS:IP, after that return's stack pop.
 * Undefined arithmetic FLAGS after SYSTEM DIV (or AF after OR) may differ
 * on such a transfer; all FLAGS are defined by the final CMP on normal exit.
 * No artificial iteration limit is imposed on alias-mutated loop state.
 */
pr_u6c_4235_status pr_u6c_record_roll_sum_4235(
    uint8_t *ram, size_t ram_size, pr_u6c_4235_cpu *cpu,
    const pr_u6c_4235_layout *layout);

#endif
