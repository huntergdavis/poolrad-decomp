#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_PROGRAM_2E10_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_PROGRAM_2E10_H

/* Whole-routine translation of unit 0x1c +0x2E10, ECL opcode 0x38 (PROGRAM): 109 bytes, 40 instructions, ending in RETF (no
 * argument bytes). File 0x3B56 (unit base 0x0D46 + 0x2E10). SHA-256 of the 109 bytes:
 *   839f9df6a713088c9889d3d9969424d858f757a7ab51e1d7e05f9a8f0bab100c
 * Evidence, decode and the exact claim: docs/re/U1C_PROGRAM_2E10.md.
 *
 * The routine works on DS-relative bytes and words and makes far or CS-pushed near calls; it writes no other memory of its own
 * (its frame is four bytes, used only for locals). The DS locations it touches are 0x4430 (flag byte), 0x4436..0x4439 (read),
 * 0x5D92..0x5D95 (written), and 0x49ED..0x49EE (saved and restored around one call).
 *
 * Far callees are hooks. Their identity in the live game is not established here:
 *   0x2B:0x2A  load operands (unit 0x25 entry 2, repository model pr_rec_ecl_load_operands), one word;
 *   0x2B:0x25  slot value (unit 0x25 entry 1, repository model pr_rec_ecl_slot_value), one word; the returned AL selects a branch;
 *   0x78:0x25  no arguments; no repository model (unresolved);
 *   0x82:0x25  no arguments; no repository model (unresolved);
 *   0xBA:0x27D9 no arguments (RES_00BA_27D9, the resident callback named in ecl_helpers.h);
 *   push cs; call near 0x2DD3   and   push cs; call near 0x0001: CS-pushed near calls within the unit. The callee returns with
 *                                 RETF, which pops the pushed CS; both are hooks with no arguments.
 *
 * ABI facts the translation fixes from the bytes (addresses in the evidence doc):
 *   - The first word is AL=1 with AH from AX at the push. AX is loaded from [0x4436] only on the flag path (the byte [0x4430] is
 *     nonzero at entry), so on that path AH is the high byte of [0x4436]; otherwise AH is entry_ah.
 *   - The second word is AL=1 with AH the high byte of the first callee's returned AX.
 *   - The returned AL of the second callee selects: 0 calls 0x78:0x25 then 0xBA:0x27D9; 8 calls 0x82:0x25; 9 saves [0x49ED],
 *     calls the near +2DD3, restores [0x49ED], and calls the near +0001; any other value makes no further call.
 *   - BP and SP are restored by the routine's own instructions (mov sp,bp; pop bp; retf). DS is never loaded into a segment
 *     register by the routine; ES and SS are not touched.
 *
 * Classification: whole-routine C translation of the routine's effects with explicit callee hooks. No game-function credit,
 * identity or parity claim is made here. */

#include <stddef.h>
#include <stdint.h>

#define PR_U1C_PROGRAM_DS_SIZE 0x5D96u   /* the highest DS byte the routine touches is 0x5D95 */

typedef struct {
    /* lcall 0x2B:0x2A with one word pushed; returns the callee's AX. */
    uint16_t (*call_2b_2a)(void *ctx, uint16_t arg);
    /* lcall 0x2B:0x25 with one word pushed; returns the callee's AX (only AL is used by the routine). */
    uint16_t (*call_2b_25)(void *ctx, uint16_t arg);
    /* lcall 0x78:0x25 with no arguments. */
    void (*call_78_25)(void *ctx);
    /* lcall 0x82:0x25 with no arguments. */
    void (*call_82_25)(void *ctx);
    /* lcall 0xBA:0x27D9 with no arguments. */
    void (*call_ba_27d9)(void *ctx);
    /* push cs; call near +0x2DD3: the callee returns with RETF, which pops the pushed CS. */
    void (*call_near_2dd3)(void *ctx);
    /* push cs; call near +0x0001: the same convention. */
    void (*call_near_0001)(void *ctx);
} pr_u1c_program_hooks;

typedef struct {
    uint8_t *ds;         /* DS image, at least PR_U1C_PROGRAM_DS_SIZE bytes; the routine reads and writes it in place */
    size_t ds_size;
    pr_u1c_program_hooks hooks;
    void *ctx;           /* passed to every hook; the hooks may write the DS image through it */
} pr_u1c_program_env;

typedef enum {
    PR_U1C_PROGRAM_OK = 0,
    PR_U1C_PROGRAM_ARGUMENT  /* null env, ds or hook, or ds_size too small: rejected before any effect */
} pr_u1c_program_status;

/* Translates the routine. entry_ah is the high byte of AX on entry; AL on entry is not read. */
pr_u1c_program_status pr_u1c_program_2e10(const pr_u1c_program_env *env, uint8_t entry_ah);

#endif
