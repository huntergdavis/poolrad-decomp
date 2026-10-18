#ifndef POOLRAD_RECOVERED_OVERLAY_U6F_PARTY_INSERT_1446_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U6F_PARTY_INSERT_1446_CPU_H

#include <stddef.h>
#include <stdint.h>

#define PR_U6F_INSERT_CPU_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u6f_insert_cpu;

typedef enum {
    PR_U6F_INSERT_CPU_NO_CALLEE = -1,
    PR_U6F_INSERT_CPU_FILLCHAR = 0,
    PR_U6F_INSERT_CPU_SET_IN,
    PR_U6F_INSERT_CPU_U9B
} pr_u6f_insert_cpu_callee;

typedef enum {
    PR_U6F_INSERT_CPU_OK = 0,
    PR_U6F_INSERT_CPU_ARGUMENT,  /* null RAM/CPU or RAM that is not exactly 1 MiB; nothing changed */
    PR_U6F_INSERT_CPU_CODE,      /* a reached LCALL is not in its expected form (stopped at the site, nothing pushed), or
                                    the 00AC:0025 stub it reaches is not the loaded EA jump to U9B +0000, a cold INT 3Fh
                                    stub included (stopped at the stub, after the call's pushes) */
    PR_U6F_INSERT_CPU_CALLEE,    /* the U9B +0000 model stopped; *callee_status holds its status */
    PR_U6F_INSERT_CPU_TRANSFER,  /* a callee's RETF popped a CS:IP other than the call's next instruction */
    PR_U6F_INSERT_CPU_LIMIT      /* host cap on the two list-walk loop-backs; IP at the loop head reached */
} pr_u6f_insert_cpu_status;

/* Raw-CPU, full-state model of the complete GAME.OVR unit 0x6F routine
 * +1446..+157E (313 bytes, 99 instructions), the party insertion, entered
 * after a near CALL preceded by PUSH CS (SS:[SP] return IP, [SP+2] return
 * CS) with [SP+4]/[SP+6] the incoming record far pointer P; RETF 4. It is
 * the CPU14 counterpart of pr_u6f_party_insert_1446_dos and _ss, whose
 * source order it follows through the end of the routine. After PUSH BP;
 * MOV BP,SP; SUB SP,10h:
 *  1. P^[0BFh] := FFh. If the DS head (5D96h/5D98h) is nil, head := P and
 *     active (5D92h/5D94h) := P; otherwise walk to the node whose next link
 *     (+104h/+106h) is nil (loop A) and set that link to P.
 *  2. FillChar clears the eight mark bytes at BP-0Ch (LCALL xxxx:16A8).
 *     For every node from the head (loop B), if its slot byte +0BFh is in
 *     the 32-byte set at CS:1426h (LCALL xxxx:0DC0), mark[slot] := 1 at
 *     BP-0Ch + slot (any slot the set admits, so the store can reach the
 *     frame).
 *  3. active := P; the local rec := active (re-read); rec^[0BFh] := 0, then
 *     while rec^[0BFh] < 8 and its mark is set, rec^[0BFh] += 1 (loop C).
 *  4. INC WORD at window1 (DS:49D6h far pointer) + 67Ch; PUSH P (re-read
 *     from the frame); LCALL 00AC:0025, whose loaded stub EA jumps to U9B
 *     +0000 (the merged raw-CPU pr_u9b_entry_zero_0000); MOV SP,BP; POP BP;
 *     RETF 4.
 * The SYSTEM callees are the merged pr_system_fillchar_16a8 and
 * pr_system_set_in_0dc0. Each LCALL's operands are read from RAM before
 * its pushes, as the CPU fetches them; the stub is read after them, when
 * the CPU reaches it. The set pointer's 1426h immediate stays absolute.
 * Every operand is reread from RAM in source order, so records, DS
 * globals, window1, the set and the frame may alias. A cyclic list loops
 * forever in loops A and B, so max_loopbacks caps their taken JMP +1487 and
 * JMP +14DC together; the next one stops at that loop head with LIMIT and
 * every earlier effect kept. Loop C needs no cap: each pass raises one byte
 * that was below 8, and only the four pointer bytes at BP-10h can move it
 * to another byte, so it takes at most 33 * 8 loop-backs. U9B's own loop keeps
 * its contract (aliasing its index can make it run forever). Flags: the
 * final flags are U9B's; at a stop they are those the original leaves: OR
 * with a zero result at the FillChar site, loop B's OR at the set site,
 * INC WORD's at the U9B site and stub (CF clear: both loop C exits leave
 * it clear), and at LIMIT loop A's OR of a nonzero link, or loop B's XOR
 * AH,AH or set test. OR and XOR leave AF clear, as both oracles do.
 * Offsets wrap at 16 bits, physical addresses at 1 MiB; LES pairs, POP and
 * this routine's RETF frame are read physically contiguous, and each
 * callee keeps its own contract. *callee and *callee_status (either may be
 * NULL) receive the last callee run and its status, or NO_CALLEE and -1. */
pr_u6f_insert_cpu_status pr_u6f_party_insert_1446_cpu(uint8_t *ram, size_t ram_size, pr_u6f_insert_cpu *cpu, unsigned max_loopbacks,
                                                      pr_u6f_insert_cpu_callee *callee, int *callee_status);

#endif
