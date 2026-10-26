#ifndef POOLRAD_RECOVERED_CANDIDATE_ECL_CPU_H
#define POOLRAD_RECOVERED_CANDIDATE_ECL_CPU_H

#include <stddef.h>
#include <stdint.h>

/* Candidate-scout batch 3: shared CPU layout and helper doubles for the two ECL
 * handler bodies (OVL_001C_0292 SAVE, OVL_001C_0EBC SAVE TABLE).
 *
 * The helper doubles stand in for the unit-0x25 runtime routines at the far-call
 * boundary. Both the original-bytes side (through a Unicorn code hook) and the
 * compiled-C side call THESE SAME functions, so the differential verifies the
 * handler bodies' control flow and argument marshalling up to the boundary. It does
 * NOT verify the real runtime helpers. No credit is claimed.
 */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_candidate_cpu;

typedef struct {
    uint16_t operand_base;   /* DS offset of the three-byte operand block */
} pr_candidate_ctx;

/* Far-call doubles. Each reads its arguments from the guest stack exactly where the
 * original body left them, applies a deterministic effect, and emulates the callee's
 * RETF plus Pascal argument cleanup. */
int pr_candidate_load_operands(uint8_t *b, pr_candidate_cpu *c, pr_candidate_ctx *ctx);
int pr_candidate_operand_word(uint8_t *b, pr_candidate_cpu *c, pr_candidate_ctx *ctx);
int pr_candidate_slot_value(uint8_t *b, pr_candidate_cpu *c, pr_candidate_ctx *ctx);
int pr_candidate_store_word(uint8_t *b, pr_candidate_cpu *c, pr_candidate_ctx *ctx);
int pr_candidate_store_string(uint8_t *b, pr_candidate_cpu *c, pr_candidate_ctx *ctx);

#endif
