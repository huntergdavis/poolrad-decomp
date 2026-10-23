#ifndef POOLRAD_RECOVERED_OVERLAY_U31_STEP_GATE_0675_H
#define POOLRAD_RECOVERED_OVERLAY_U31_STEP_GATE_0675_H

#include <stddef.h>
#include <stdint.h>

#define PR_U31_0675_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u31_0675_cpu;

typedef enum {
    PR_U31_0675_OK = 0,
    PR_U31_0675_ARGUMENT,  /* null RAM/CPU or RAM that is not exactly 1 MiB; nothing changed */
    PR_U31_0675_CODE,      /* the reached call site, or the 03E0:0070 stub it names, is not in its original loaded
                            * form; stopped before that call with every earlier effect kept */
    PR_U31_0675_CALLEE,    /* a composed model stopped; CPU and RAM hold its state, `callee` names it */
    PR_U31_0675_TRANSFER   /* a composed model returned to a CS:IP other than its call's next instruction */
} pr_u31_0675_status;

typedef struct {
    uint16_t site;   /* unit offset of the call that stopped or ran last (06DA, 0758, 076F, 0786, 079D); 0 if none */
    int status;      /* that model's status (pr_u3c6_0cb9_cpu_status or pr_res_2199_status); -1 if none ran */
} pr_u31_0675_callee;

/* Complete GAME.OVR unit 0x31:+0675..+07D5 (353 bytes, 137 instructions, ten
 * conditional branches), entered after the far CALL (in the game PUSH CS;
 * CALL from +07F2 at +094B) with SS:[SP] return IP, [SP+2] return CS and the
 * Pascal arguments [BP+6]/[BP+8] result byte pointer R, [BP+0Ah] byte b,
 * [BP+0Ch] byte c and [BP+0Eh]/[BP+10h] object pointer X; RETF 0Ch.
 *
 * Ordered effects: *R := 0 and the result [BP-1] := 0. With Y = X->+108
 * (far pointer), t = DS:[02AE + 5*Y[15h] + c] is stored in [BP-6] and
 * r = (b + t) mod 8 (CWD; IDIV by CX = 8; XCHG) in [BP-7]. The far call
 * through stub 03E0:0070 runs U3C6 +0CB9 (P6 = SS:BP-9, P10 = SS:BP-8,
 * k = r, Q = X). If [BP-9] is 0, *R := 1 and the result stays 0; if [BP-8]
 * is nonzero the result stays 0. Otherwise w = DS:[2744 + 4*[BP-9]]; unless
 * w < Y[6] the result stays 0. The threshold [BP-0Ah] := w * (2, or 3 when r
 * is odd) (low byte of MUL). When [BP-9] is 1Eh, four far calls to
 * resident RES_00BA_2199 look up keys 20h, 1Eh, 6Fh and 7Dh in X's sidecar
 * list with cursor cell SS:BP-5, stopping at the first hit; with no hit the
 * threshold := Y[6] + 1. Finally the result := 1 unless Y[6] < threshold,
 * AL := the result, MOV SP,BP; POP BP; RETF 0Ch.
 *
 * Composition: each far call is read from RAM at its site (9A, offset,
 * segment). The +0CB9 call requires offset 0070h and a stub whose bytes are
 * EA B9 0C plus a segment, then runs pr_u3c6_0cb9_cpu_run there with the
 * CONTIGUOUS_PHYSICAL RETF policy; the lookups require offset 2199h and run
 * pr_res_00ba_2199_cpu at the call's segment with lookup_max_loopbacks. Each
 * call pushes CS and the next IP relative to the entry IP, and each model
 * must return to it. Every operand is reread from RAM in source order;
 * offsets wrap at 16 bits, physical addresses at 1 MiB; words, LES pairs
 * and the RETF frame are read physically contiguous.
 *
 * Flags: the final flags are the last CMP's (+06DF, +06EF, +070F or +07C4).
 * IDIV (pinned DIVW/IDIVW values) and OR AL,AL after each lookup are written,
 * because their flags reach a far call: the callee's entry and this model's
 * CODE or CALLEE stop there. The flag writes of SUB SP, XOR AH,AH, SHL, ADD,
 * SHR, MUL and INC are folded: on every path a later writer overwrites them
 * before any branch, call, stop or return (docs/re/FLAG_FOLDED_U31_0255_0675.md,
 * tools/overlay/flag_fold_liveness.py). SHR's carry selects the factor directly.
 * A CODE stop leaves CS:IP at the call it refused. */
pr_u31_0675_status pr_u31_step_gate_0675(uint8_t *ram, size_t ram_size, pr_u31_0675_cpu *cpu,
                                         unsigned lookup_max_loopbacks, pr_u31_0675_callee *callee);

#endif
