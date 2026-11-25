#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_WHO_2B33_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_WHO_2B33_H

/* Whole-routine translation of unit 0x1c +0x2B33, ECL opcode 0x39 (WHO): 75 bytes, 36 instructions, ending in RETF (no argument
 * bytes). File 0x3879 (unit base 0x0D46 + 0x2B33). SHA-256 of the 75 bytes:
 *   b373a049e2df103b77382331370aefb2a5378b0f9d66b947333c16298e646083
 * The span follows the CLEAR BOX routine (unit +0x2B14..+0x2B32), whose last byte is its RETF. Evidence, decode and the exact
 * claim: docs/re/U1C_WHO_2B33.md.
 *
 * The routine writes no memory of its own. It reserves a 256-byte frame (sub sp,0x100), makes four far calls and returns. Its
 * observable behaviour is therefore the four calls: their argument words, their order, and the value it leaves for the next
 * call's argument high bytes. Every far callee is a hook:
 *   - 0x2B:0x2A, one word (ECL load-operands entry in this repository);
 *   - 0x709:0x0DD3, four words (the resident display routine CLEAR BOX also calls; retf 8);
 *   - 0xAF8:0x0B25, five words: DS segment, 0x70C8, SS segment, the frame address BP-0x100, and 0x00FF (the StrStore entry);
 *   - 0xBA:0x2C48, three words (resident, unresolved in this repository; kept as a hook).
 *
 * ABI facts the translation fixes from the bytes (see the evidence doc for the instruction addresses):
 *   - Each argument word is AL=constant with AH from the register AX at that point. AX is never loaded from memory between the
 *     first call and its argument words, so:
 *       (a) the first call's word carries the caller's entry AH (entry_ah);
 *       (b) the four words of the display call carry the high byte of the first callee's returned AX;
 *       (c) the StrStore's last word is 0x00FF (AX := 0x00FF), so its AH is 0;
 *       (d) the final word carries the high byte of the StrStore's returned AX.
 *   - The frame address is BP-0x100 with BP = entry SP - 2 (push bp; mov bp,sp), so the StrStore destination offset is
 *     entry_sp - 0x102.
 *   - Stack cleanup of the callees is not modelled; the routine restores SP and BP with mov sp,bp; pop bp.
 *
 * Classification: whole-routine C translation of the routine's calls, with explicit callee hooks. No game-function credit, identity
 * or parity claim is made here. */

#include <stddef.h>
#include <stdint.h>

typedef struct {
    /* lcall 0x2B:0x2A with one word pushed; returns the callee's AX. */
    uint16_t (*call_2b_2a)(void *ctx, uint16_t arg);
    /* lcall 0x709:0x0DD3 with four words pushed; p0 was pushed first (deepest), p3 last (top of stack). */
    void (*call_709_0dd3)(void *ctx, uint16_t p0, uint16_t p1, uint16_t p2, uint16_t p3);
    /* lcall 0xAF8:0x0B25 with five words pushed in this order: DS segment, source offset 0x70C8, SS segment, destination offset
     * (frame address), and the word 0x00FF. Returns the callee's AX. */
    uint16_t (*call_af8_0b25)(void *ctx, uint16_t ds_seg, uint16_t src_off, uint16_t ss_seg, uint16_t dst_off, uint16_t max);
    /* lcall 0xBA:0x2C48 with three words pushed in this order: a word (AH:00), the DS segment, and the offset 0x5D92. */
    void (*call_ba_2c48)(void *ctx, uint16_t word, uint16_t ptr_seg, uint16_t ptr_off);
} pr_u1c_who_hooks;

typedef struct {
    uint16_t ds_seg;     /* the DS value at entry (pushed as a far-pointer segment word) */
    uint16_t ss_seg;     /* the SS value at entry (pushed as the destination segment) */
    uint16_t entry_sp;   /* SP at entry, before push bp; BP = entry_sp - 2, the frame is [BP-0x100, BP) */
    pr_u1c_who_hooks hooks;
    void *ctx;           /* passed to every hook */
} pr_u1c_who_env;

typedef enum {
    PR_U1C_WHO_OK = 0,
    PR_U1C_WHO_ARGUMENT  /* null env or hook, or entry_sp < 0x0102 (the frame address would wrap): rejected before any call */
} pr_u1c_who_status;

/* Translates the routine. entry_ah is the high byte of AX on entry (the routine reads no other entry register). */
pr_u1c_who_status pr_u1c_who_2b33(const pr_u1c_who_env *env, uint8_t entry_ah);

#endif
