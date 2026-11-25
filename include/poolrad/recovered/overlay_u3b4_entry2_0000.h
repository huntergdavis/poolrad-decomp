#ifndef POOLRAD_RECOVERED_OVERLAY_U3B4_ENTRY2_0000_H
#define POOLRAD_RECOVERED_OVERLAY_U3B4_ENTRY2_0000_H

/* Whole-routine translation of unit 0x3B4 +0x0000 (the unit's entry 0 in the ledger's slot 2): 199 bytes, 72 instructions, ending in a
 * plain RETF. File 0x2DB87 (unit code). SHA-256 of the 199 bytes:
 *   4a0b67222fd84d903b9a817b68b7cd6398cbc864b7737e198427034682788880
 * Evidence, decode and the exact claim: docs/re/U3B4_ENTRY2_0000.md.
 *
 * The routine computes two coordinate-like bytes from a table byte and the far record at DS:0x49D2, calls unit 0x3CE entry slot 5 (the
 * 0x3E9:0x39 stub) with five words, updates the byte DS:0x6DDC modulo 6, and calls the resident 0x464:0x1B52 with no arguments. It is
 * reached by a near call (push cs; call near 0x0000) from the view refresh at unit +0x016D. Its callees are hooks:
 *   lcall 0x3E9:0x39, five words pushed in order (w1 first, w5 at the top of the stack); the callee returns with RETF 0xA;
 *   lcall 0x464:0x1B52, no arguments (resident, unresolved in this repository); its return is not used.
 *
 * ABI facts the translation fixes from the bytes (addresses in the evidence doc):
 *   - AX is a full register. The routine's first instruction that matters sets AH to 0 (xor ah,ah) and the entry AX is never read.
 *   - Each argument word is (AH of the last full AX write) : (AL that the last MOV AL loaded). After the record add the high byte is
 *     fixed for all the pushed words, so the first three words share it.
 *   - The record words read are +0x186 (added to the table byte), +0x188 (the second byte); the far pointer at DS:0x49D2 is re-read at
 *     each LES.
 *   - The two remaining words are signed-division results: w4 is the remainder of DS:0x6DDC by 2 (CWD then IDIV CX=2), w5 is
 *     (DS:0x6DDC >> 1) + 9. DS:0x6DDC is then set to (DS:0x6DDC + 1) mod 6 (CWD, IDIV CX=6, the remainder). The value is read after
 *     the first callee returns, so a callee that changes DS:0x6DDC changes the stored value.
 *   - BP and SP are restored by the routine's own instructions (mov sp,bp; pop bp; retf). DS is not reloaded; ES is set by each LES and
 *     is not restored.
 *   - The routine's exit AX and DX depend on the last division and the last callee path. They are not modelled.
 *
 * Classification: whole-routine C translation of the routine's effects with explicit callee hooks. No game-function credit, identity
 * or parity claim is made here. */

#include <stddef.h>
#include <stdint.h>

#define PR_U3B4_ENTRY2_DS_SIZE 0x10000u   /* DS-relative offsets wrap at 16 bits; the table index can reach DS:0x3D75 */

typedef struct {
    /* lcall 0x3E9:0x39 with five words pushed (w1 first); the callee returns with RETF 0xA. Its return is not used. */
    void (*call_3e9_39)(void *ctx, uint16_t w1, uint16_t w2, uint16_t w3, uint16_t w4, uint16_t w5);
    /* lcall 0x464:0x1B52 with no arguments; the callee's return is not used. */
    void (*call_464_1b52)(void *ctx);
    /* Read the word at ES:off, where ES:off is the far record pointer DS:0x49D2 (offset) and DS:0x49D4 (segment), plus the field
     * offset (16-bit wrap). Returns 0 on success. */
    int (*far_read16)(void *ctx, uint16_t seg, uint16_t off, uint16_t *out);
} pr_u3b4_entry2_hooks;

typedef struct {
    uint8_t *ds;          /* DS image, PR_U3B4_ENTRY2_DS_SIZE bytes; the routine reads and writes it in place */
    size_t ds_size;
    pr_u3b4_entry2_hooks hooks;
    void *ctx;            /* passed to every hook */
} pr_u3b4_entry2_env;

typedef enum {
    PR_U3B4_ENTRY2_OK = 0,
    PR_U3B4_ENTRY2_ARGUMENT,  /* null env, ds or hook, or ds_size below 0x10000: rejected before any effect */
    PR_U3B4_ENTRY2_FAR_READ   /* far_read16 failed: the routine stops at that read; earlier effects stand */
} pr_u3b4_entry2_status;

/* Translates the routine. The entry AX is not read: the routine overwrites AH before its first use. */
pr_u3b4_entry2_status pr_u3b4_entry2_0000(const pr_u3b4_entry2_env *env);

#endif
