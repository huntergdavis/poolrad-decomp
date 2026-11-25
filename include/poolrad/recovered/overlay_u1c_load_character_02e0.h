#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_LOAD_CHARACTER_02E0_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_LOAD_CHARACTER_02E0_H

/* Whole-routine translation of unit 0x1c +0x02E0, ECL opcode 0x0A (LOAD CHARACTER): 212 bytes, 70 instructions, ending in RETF
 * (no argument bytes). File 0x1026 (unit base 0x0D46 + 0x02E0). SHA-256 of the 212 bytes:
 *   07903667cd47edfcd46ca97d08b10a6bb494b07008fd02290b22b642d7e42333
 * Evidence, decode and the exact claim: docs/re/U1C_LOAD_CHARACTER_02E0.md.
 *
 * The routine works on DS-relative bytes and words, walks a far pointer chain through ES (+0x104 offset word, +0x106 segment word per
 * node), and makes four far calls. The four callees are the file operands 0x2B:0x2A, 0x2B:0x25, 0x78:0x2A and 0xBA:0x767; they are
 * supplied as hooks. Their identity in the live game is not established here.
 *
 * ABI facts the translation fixes (read from the file bytes, see the evidence doc):
 *   - Each argument word is AL=constant with AH taken from the register AX at that point. The routine never sets AH itself. For the
 *     first call AH is the AH the caller passed in (entry_ah). For the second call AH is the high byte of the first callee's returned
 *     AX. The third and fourth calls run only on the flag path, and there AH is the high byte of word [0x4436], read at the flag
 *     test (AX is loaded from it immediately before the compare).
 *   - The flag path is: hi_flag set, [0x84D2] nonzero and [0x84D3] nonzero. Any other case skips the third and fourth calls and the
 *     two clears (the file's je 0x3B0 jumps go to the frame teardown).
 *   - The routine does not clean the stack after its calls; its last instruction pair (mov sp,bp / pop bp) restores SP. The
 *     translation does not model callee stack cleanup.
 *   - The AL value on entry is not read.
 *
 * Classification: whole-routine C translation of the routine's effects, with explicit callee hooks. No game-function credit or
 * parity claim is made here. */

#include <stddef.h>
#include <stdint.h>

#define PR_U1C_LC02E0_DS_SIZE 0x84D4u   /* the highest DS byte the routine touches is 0x84D3 */

typedef struct {
    /* lcall 0x2B:0x2A with one word pushed; returns the callee's AX. */
    uint16_t (*call_2b_2a)(void *ctx, uint16_t arg);
    /* lcall 0x2B:0x25 with one word pushed; returns the callee's AX (only AL is used by the routine). */
    uint16_t (*call_2b_25)(void *ctx, uint16_t arg);
    /* lcall 0x78:0x2A; `first` is pushed before `second`. Return values are not used. */
    void (*call_78_2a)(void *ctx, uint16_t first, uint16_t second);
    /* lcall 0xBA:0x767; `off` is the word pushed last (the top of stack at the call), `seg` was pushed first. */
    void (*call_ba_767)(void *ctx, uint16_t off, uint16_t seg);
    /* Read one little-endian word from far address seg:off (off is a 16-bit effective address). Returns 0 on success. */
    int (*far_read16)(void *ctx, uint16_t seg, uint16_t off, uint16_t *out);
} pr_u1c_lc02e0_hooks;

typedef struct {
    uint8_t *ds;                    /* DS image, at least PR_U1C_LC02E0_DS_SIZE bytes; the routine reads and writes it in place */
    size_t ds_size;
    pr_u1c_lc02e0_hooks hooks;
    void *ctx;                      /* passed to every hook */
} pr_u1c_lc02e0_env;

typedef enum {
    PR_U1C_LC02E0_OK = 0,
    PR_U1C_LC02E0_ARGUMENT,         /* null env, ds, or hook, or ds_size too small: rejected before any effect */
    PR_U1C_LC02E0_FAR_READ          /* far_read16 failed: the routine stops at that read; effects before it stand */
} pr_u1c_lc02e0_status;

/* Translates the routine. entry_ah is the AH value the caller passes in AX (the routine reads no other entry register). */
pr_u1c_lc02e0_status pr_u1c_load_character_02e0(const pr_u1c_lc02e0_env *env, uint8_t entry_ah);

#endif
