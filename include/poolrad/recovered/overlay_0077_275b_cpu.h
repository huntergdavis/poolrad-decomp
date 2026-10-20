#ifndef POOLRAD_RECOVERED_OVERLAY_0077_275B_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_0077_275B_CPU_H
#include "poolrad/recovered/resident_00ba_character_0bb8_cpu.h"

/* U77 [275B,27DD): 132 bytes, 45 starts, RETF 8. Same CPU/status vocabulary as
 * the resident models it composes. */
typedef pr_res00ba_cpu pr_ovl0077_275b_cpu;

typedef enum {
    PR_OVL0077_275B_RETURNED = 0,
    PR_OVL0077_275B_ARGUMENT,
    PR_OVL0077_275B_CALLEE,
    PR_OVL0077_275B_LIMIT,
    PR_OVL0077_275B_TRANSFER
} pr_ovl0077_275b_status;

typedef struct {
    int first_status;   /* -1 until 00BA:0BB8 runs */
    int second_status;  /* -1 until 00BA:13AB runs */
    unsigned calls;
} pr_ovl0077_275b_diagnostic;

/* Capacity/encumbrance check. Frame words: [bp+0x0A] far pointer to the target
 * object, [bp+0x0C] its segment word, [bp+0x06] far pointer to the record.
 * Reads record +0xC7 (type byte vs 15), +0x37 (quantity word), +0x39 (count
 * byte); multiplies quantity by count when count != 0; adds the composed
 * 00BA:0BB8 result plus 1500 and compares the sign-extended 32-bit total with
 * target +0x102 plus the product. Returns the flag in AL and at [bp-1].
 *
 * Both callees are credited raw residents composed directly: 0BB8 through
 * pr_res00ba_0bb8_cpu and 13AB through pr_res00ba_helpers_cpu with entry
 * 13AB. `budget` is reset before each resident call. RAM is exactly 1 MiB of
 * A20-off ordinary memory; CPU and diagnostic live outside it. Code immutable,
 * SP in [0x0080,0xFFF8], frames non-wrapping; async interrupts and device
 * state are outside this model. A callee that does not return to its pushed
 * frame is TRANSFER, not success. */
pr_ovl0077_275b_status pr_ovl0077_275b_cpu(uint8_t *ram, size_t ram_size,
                                          pr_ovl0077_275b_cpu *cpu,
                                          size_t *budget,
                                          pr_ovl0077_275b_diagnostic *diag);

#endif
