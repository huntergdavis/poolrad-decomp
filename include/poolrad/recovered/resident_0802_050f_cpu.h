#ifndef POOLRAD_RECOVERED_RESIDENT_0802_050F_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_0802_050F_CPU_H
#include <stddef.h>
#include <stdint.h>

#define PR_R0802_050F_RAM_SIZE (1u << 20)

/* Raw-CPU, full-state model of START.EXE resident 0802:[050F,05CB) (188 bytes, 83 instructions, RETF 10h), the t30 cluster's
 * block decompressor (docs/re/RES_0802_050F_CPU.md). Entered by PUSH CS; CALL (0716 at +0935) or a far call: [SP] IP, [SP+2]
 * CS, [SP+4] the far address of the output-size word (never read), [SP+8] the far address of the packed-size word, [SP+0Ch]
 * the far address of the output far-pointer variable, [SP+10h] the far address of the packed far-pointer variable.
 *
 *   050F PUSH BP; MOV BP,SP; SUB SP,6; in := 0 [BP-2]; out := 0 [BP-4]
 *   051F loop: c := packed^[in] (the pointer variable reloaded, its offset + in, 16-bit) -> [BP-5]
 *   0533 c in 00h..7Eh (signed): Move(packed^ + in + 1, output^ + out, c + 1); in += c + 2; out += c + 1
 *   0574 c in 81h..FFh (signed): n := -c; FillChar(output^ + out, n, packed^[in + 1]); in += 2; out += n
 *        c = 7Fh or 80h: neither path; in and out unchanged
 *   05B7 in <u packed-size word (reloaded): JMP 051F; else MOV SP,BP; POP BP; RETF 10h
 *
 * A control 7Fh or 80h with in below the packed size repeats the same iteration forever: the original never returns. The model
 * stops at the loop head (IP 051Fh) with STALLED once an iteration has run that took neither copy path, left memory unchanged
 * (its only write, [BP-5], stored the value already there) and returned to the loop head with the same 14 CPU words as the
 * arrival before it. The next iteration is then the same deterministic function of the same state, so the state is a fixed
 * point. Other inputs can cycle without a fixed point (the 16-bit in wrapping below a packed size near FFFFh); the declared
 * budget max_arrivals bounds them: the model stops BUDGET at the loop head before arrival max_arrivals + 1 runs. Neither stop
 * claims a DOS outcome.
 *
 * Composition: Move 0AF8:1685 and FillChar 0AF8:16A8 (system_string_move_leaves_cpu, system_runtime_leaves_cpu), each far call's segment
 * word read live from the code, each callee required to return after its call (else CALLEE). No DOS service and no I/O.
 * Exactly 1 MiB of A20-off RAM with the relocated code at CS; IP 050Fh; entry SP in [0016h,FFECh] (the deepest frame, Move's,
 * reaches entry SP - 16h; RETF 10h needs entry SP + 14h within the segment). */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_r0802_050f_cpu;

typedef enum {
    PR_R0802_050F_RETURNED = 0,  /* RETF 10h executed */
    PR_R0802_050F_ARGUMENT,      /* null pointers, RAM size, IP not 050Fh, entry SP out of range; nothing changed */
    PR_R0802_050F_STALLED,       /* a 7Fh/80h control below the packed size: a fixed point at 051Fh; the original never returns */
    PR_R0802_050F_BUDGET,        /* the declared max_arrivals reached at 051Fh (no claim about the original's outcome) */
    PR_R0802_050F_CALLEE         /* a composed model stopped otherwise or returned elsewhere (see the diagnostic) */
} pr_r0802_050f_status;

typedef struct {
    uint16_t site;               /* the LCALL of a CALLEE stop, else 0 */
    int callee_status;           /* that callee's status, else -1 */
    uint32_t arrivals;           /* arrivals at the loop head 051Fh, the first from the prologue included */
} pr_r0802_050f_diagnostic;

pr_r0802_050f_status pr_r0802_050f(uint8_t *ram, size_t ram_size, pr_r0802_050f_cpu *cpu, uint32_t max_arrivals,
                                   pr_r0802_050f_diagnostic *diagnostic);
#endif
