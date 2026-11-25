#ifndef POOLRAD_RECOVERED_OVERLAY_U3B4_VIEW_REFRESH_016D_H
#define POOLRAD_RECOVERED_OVERLAY_U3B4_VIEW_REFRESH_016D_H

/* Whole-routine translation of unit 0x3B4 +0x016D (public entry 1, the view refresh): 307 bytes, 114 instructions, ending in a plain
 * RETF (no argument bytes). File 0x2DCF4 (unit code at 0x2DB87 + 0x016D). SHA-256 of the 307 bytes:
 *   c6bb0b220344d76f8924b0f96bcea506bb55dbdad753e2f945fa25bf782255e8
 * Evidence, decode and the exact claim: docs/re/U3B4_VIEW_REFRESH_016D.md.
 *
 * The routine reads a far record pointer at DS:0x49D2 (offset word, then segment word) and reads six words from it at +0x186, +0x188,
 * +0x1CC, +0x1F6, +0x1FA and +0x1FC. It reads and writes DS bytes at 0x49FA, 0x6AAC, 0x6AAD..0x6AAF, 0x6AB1, 0x6A60..0x6A63, 0x6F68,
 * 0x6F69 and 0x84E5, and reads the table at DS:0x3C76. Its callees are hooks:
 *   lcall 0x3D5:0x34 (unit 0x3BD entry 4), two words;
 *   lcall 0x3D5:0x25 (unit 0x3BD entry 1), four words;
 *   lcall 0x3D5:0x39 (unit 0x3BD entry 5), three words;
 *   push cs; call near 0x00C7, 0x00FB and 0x012F (unit 0x3B4 nested case maps; each takes one argument word and returns AL with AH
 *     unchanged);
 *   lcall 0x3CD:0x25 (unit 0x3B6 entry 0), two words;
 *   push cs; call near 0x0000 (unit 0x3B4 entry 0), no arguments.
 *
 * ABI facts the translation fixes from the bytes (addresses in the evidence doc):
 *   - AX is modelled as a full 16-bit register from entry. Each push pushes the whole AX, so each argument word is (AH from the last
 *     write to AX) : (AL the constant or byte the MOV AL loaded). MOV AL changes only AL. The routine writes AX in full at CBW (sign-
 *     extension of AL into AX), at MOV AX, WORD and at ADD AX/DEC AX, and each callee return sets AX.
 *   - Entry AH is read by the first argument words; entry AL is not read.
 *   - The far-call returns set AX to the callee's full AX; the near case maps return AL and keep AH.
 *   - The record far pointer is re-read at each LES, so a callee that changes DS:0x49D2 changes what later reads see.
 *   - The flag at [BP-3] is 1 when the record word at +0x1CC is nonzero, otherwise DS:0x49FA. Flag 1 runs the call sequence; flag 2,
 *     3 or 4 runs the coordinate clamp and calls 0x3CD:0x25 and near 0x0000; any other value makes no call.
 *   - BP and SP are restored by the routine's own instructions (mov sp,bp; pop bp; retf). DS and SS are not written; ES is set by each
 *     LES and is not restored.
 *   - The routine's exit AX, DX and flags depend on the path (for example, the last callee's return). They are not modelled.
 *
 * Classification: whole-routine C translation of the routine's effects with explicit callee hooks. No game-function credit, identity
 * or parity claim is made here. */

#include <stddef.h>
#include <stdint.h>

#define PR_U3B4_016D_DS_SIZE 0x10000u   /* DS-relative offsets wrap at 16 bits; the image is the whole segment */

typedef struct {
    /* lcall 0x3D5:0x34 with two words pushed (w1 first); returns the callee's full AX. */
    uint16_t (*call_3d5_34)(void *ctx, uint16_t w1, uint16_t w2);
    /* lcall 0x3D5:0x25 with four words pushed (w1 first); returns the callee's full AX. */
    uint16_t (*call_3d5_25)(void *ctx, uint16_t w1, uint16_t w2, uint16_t w3, uint16_t w4);
    /* lcall 0x3D5:0x39 with three words pushed (w1 first); returns the callee's full AX. */
    uint16_t (*call_3d5_39)(void *ctx, uint16_t w1, uint16_t w2, uint16_t w3);
    /* push cs; call near 0x00C7 with one word pushed (the argument; only its low byte is used). Returns AL; the caller keeps AH. */
    uint8_t (*near_00c7)(void *ctx, uint16_t arg);
    /* push cs; call near 0x00FB, the same convention. */
    uint8_t (*near_00fb)(void *ctx, uint16_t arg);
    /* push cs; call near 0x012F, the same convention. */
    uint8_t (*near_012f)(void *ctx, uint16_t arg);
    /* lcall 0x3CD:0x25 with two words pushed (w1 first); its return AX is not used by the routine. */
    void (*call_3cd_25)(void *ctx, uint16_t w1, uint16_t w2);
    /* push cs; call near 0x0000 with no arguments; its return is not used by the routine. */
    void (*near_0000)(void *ctx);
    /* Read the word at ES:off, where ES:off is the far record pointer DS:0x49D2 (offset) and DS:0x49D4 (segment), plus the field
     * offset (16-bit wrap). Returns 0 on success. */
    int (*far_read16)(void *ctx, uint16_t seg, uint16_t off, uint16_t *out);
} pr_u3b4_016d_hooks;

typedef struct {
    uint8_t *ds;         /* DS image, PR_U3B4_016D_DS_SIZE bytes; the routine reads and writes it in place */
    size_t ds_size;
    pr_u3b4_016d_hooks hooks;
    void *ctx;           /* passed to every hook */
} pr_u3b4_016d_env;

typedef enum {
    PR_U3B4_016D_OK = 0,
    PR_U3B4_016D_ARGUMENT,   /* null env, ds or hook, or ds_size below 0x10000: rejected before any effect */
    PR_U3B4_016D_FAR_READ    /* far_read16 failed: the routine stops at that read; earlier effects stand */
} pr_u3b4_016d_status;

/* Translates the routine. entry_ah is the high byte of AX on entry; AL on entry is not read. */
pr_u3b4_016d_status pr_u3b4_view_refresh_016d(const pr_u3b4_016d_env *env, uint8_t entry_ah);

#endif
