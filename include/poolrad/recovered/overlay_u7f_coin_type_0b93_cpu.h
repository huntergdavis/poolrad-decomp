#ifndef POOLRAD_RECOVERED_OVERLAY_U7F_COIN_TYPE_0B93_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U7F_COIN_TYPE_0B93_CPU_H

#include <stddef.h>
#include <stdint.h>

#define PR_U7F_0B93_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u7f_0b93_cpu;

typedef enum {
    PR_U7F_0B93_OK = 0,
    PR_U7F_0B93_ARGUMENT,  /* null RAM/CPU or RAM that is not exactly 1 MiB; nothing changed */
    PR_U7F_0B93_CODE,      /* a reached LCALL is not xxxx:0B25 or xxxx:182A as expected; stopped before it, IP at the site */
    PR_U7F_0B93_TRANSFER,  /* a callee's RETF popped a CS:IP other than the call's next instruction */
    PR_U7F_0B93_LIMIT      /* host cap on the space-skip loop-backs; IP at the loop head (entry IP + 20h) */
} pr_u7f_0b93_status;

/* Raw-CPU, full-state model of the complete GAME.OVR unit 0x7F routine
 * +0B93..+0CB9 (295 bytes of code; the bytes +0CBA..+0CEA after its RETF 8
 * are string data), entered after a far call with SS:[SP] return IP, [SP+2]
 * return CS, [SP+4]/[SP+6] a far pointer D to an output string and
 * [SP+8]/[SP+0Ah] a far pointer S to an input string; RETF 8. With BP =
 * SP-2 after PUSH BP; SUB SP,102h:
 *   copy S into the local string[255] L at SS:BP-101h (string store, cap
 *   FFh); i := 1 in [BP-102h]; while L[i] is a space, i := i+1 (byte, no
 *   bound); then by c := L[i]:
 *     'G': if UpCase(L[i+1]) = 'E', D := CS:0B5A and r := 5, else
 *          D := CS:0B60 and r := 3;
 *     'P': D := CS:0B66, r := 4;   'E': D := CS:0B70, r := 2;
 *     'S': D := CS:0B7A, r := 1;   'C': D := CS:0B82, r := 0;
 *     'J': D := CS:0B8A, r := 6;   otherwise r is the stale byte [BP-1];
 *   each D := literal is the string store with cap FFh from the code-segment
 *   Pascal string at that immediate offset; [BP-1] := r; AL := [BP-1].
 * Every call (the stores and UpCase) is a real far call read from RAM and
 * run by pr_system_strstore_0b25 or pr_system_upcase_182a, which must
 * return to the next instruction. The final flags are the last store's
 * XOR CH,CH flags, or on the unmatched path CMP AL,4Ah's, with DF clear
 * from the first store; SUB SP,102h's flags are computed for a CODE stop
 * at the first site, XOR AH,AH and INC AX's for one at the UpCase site, and
 * the loop's INC flags for a LIMIT stop. An
 * all-space L loops forever in DOS: max_loopbacks
 * caps the taken JMP +0BB3, and the next one stops there with LIMIT and
 * every earlier effect kept. Offsets wrap at 16 bits, physical addresses at
 * 1 MiB; LES pairs, POP and the RETF frames are read physically contiguous.
 * *callee_status (may be NULL) receives the last callee's status, or -1
 * when none ran. */
pr_u7f_0b93_status pr_u7f_coin_type_0b93_cpu(uint8_t *ram, size_t ram_size, pr_u7f_0b93_cpu *cpu, unsigned max_loopbacks,
                                             int *callee_status);

#endif
