#ifndef POOLRAD_RECOVERED_OVERLAY_U3B0_SPACE_SCAN_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U3B0_SPACE_SCAN_CPU_H

#include <stddef.h>
#include <stdint.h>

#define PR_U3B0S_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u3b0s_cpu;

typedef enum {
    PR_U3B0S_OK = 0,
    PR_U3B0S_ARGUMENT,  /* null RAM/CPU or RAM that is not exactly 1 MiB; nothing changed */
    PR_U3B0S_CODE,      /* the reached LCALL is not xxxx:0B25 (9A 25 0B); stopped before it, IP at the site */
    PR_U3B0S_TRANSFER   /* the string store's RETF 0Ah popped a CS:IP other than the call's next instruction;
                           cannot occur with the modeled store, which writes only the local above its frame */
} pr_u3b0s_status;

/* Raw-CPU, full-state models of two complete GAME.OVR unit 0x3B0 routines,
 * each entered after the far CALL with SS:[SP] return IP, [SP+2] return
 * CS, [SP+4] a word that is never read and [SP+6]/[SP+8] a far pointer S to
 * a Pascal string; RETF 6. Each opens PUSH BP; MOV BP,SP; SUB SP,2Ch and
 * copies S into the local string[40] at SS:BP-2Ah through the Turbo Pascal
 * SYSTEM string store (LES DI,S; PUSH ES; PUSH DI; LEA DI,[BP-2Ah]; PUSH SS;
 * PUSH DI; MOV AX,28h; PUSH AX; LCALL xxxx:0B25), a real far call read from
 * RAM and run by the raw-CPU pr_system_strstore_0b25, which must return.
 *   +08D7 (76 bytes, 31 instructions): i := 1 in [BP-2Bh] and n := the copy's
 *     length in [BP-2Ch]; while i < n and local[i] is a space (20h), i := i+1.
 *   +0923 (70 bytes, 29 instructions): i := n in [BP-2Bh]; while i > 1 and
 *     local[i] is a space, i := i-1.
 * [BP-1] := i; AL := [BP-1]; MOV SP,BP; POP BP; RETF 6. The final flags are
 * the loop's last CMP; INC/DEC/XOR writes before it are folded. SUB SP,2Ch's
 * flags are computed because a CODE stop leaves them; on every other path
 * the store's XOR CH,CH overwrites them. The far call's segment word is
 * taken from RAM (the loader-relocated SYSTEM segment). Every
 * operand is reread from RAM in source order; offsets wrap at 16 bits,
 * physical addresses at 1 MiB; LES pairs and the RETF frame are read
 * physically contiguous. *callee_status (may be NULL) receives the string
 * store's status, or -1 when it did not run. */
pr_u3b0s_status pr_u3b0_lead_spaces_08d7_cpu(uint8_t *ram, size_t ram_size, pr_u3b0s_cpu *cpu, int *callee_status);
pr_u3b0s_status pr_u3b0_trail_spaces_0923_cpu(uint8_t *ram, size_t ram_size, pr_u3b0s_cpu *cpu, int *callee_status);

#endif
