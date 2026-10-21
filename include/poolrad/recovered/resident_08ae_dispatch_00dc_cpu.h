#ifndef POOLRAD_RECOVERED_RESIDENT_08AE_DISPATCH_00DC_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_08AE_DISPATCH_00DC_CPU_H
#include <stddef.h>
#include <stdint.h>
#define PR_R08AE_00DC_RAM_SIZE (1u << 20)
typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_r08ae_00dc_cpu;
typedef enum {
    PR_R08AE_00DC_RETURN = 0,   /* RETF 2 executed: IP/CS popped, SP advanced past the argument word */
    PR_R08AE_00DC_ARGUMENT,     /* host or profile violation; nothing changed */
    PR_R08AE_00DC_CALL_137C,    /* near CALL executed (CS and the return IP pushed); IP = the callee */
    PR_R08AE_00DC_CALL_17AE,
    PR_R08AE_00DC_CALL_1742,
    PR_R08AE_00DC_CALL_181A,
    PR_R08AE_00DC_CALL_006C,
    PR_R08AE_00DC_CALL_13B1
} pr_r08ae_00dc_status;
/* Original 08AE [00DC,017B), 159 bytes, RETF 2: a far procedure with one WORD argument A at [BP+6]. It is the composition POINT of the
 * 08AE driver set: its callees are 17AE (x4 sites), 1742 (x4), 181A (x2), 006C, 137C and 13B1, each reached by PUSH CS; near CALL
 * (the far-return convention). The callees are STOPS, not structure: the model returns the CALL_* status in the state at the callee's
 * entry (return IP and CS pushed, then the pushed argument word if any below them, IP = the callee) and does not model the callee. The
 * thirteen call sites are told apart by the return IP at SS:[SP]: 00F4 (137C), 0103/012E/0156/016C (17AE), 010A/0135/0147/0173
 * (1742), 0111/0120 (181A), 0165 (006C), 0177 (13B1). To continue after a callee returns, set the post-return CPU state and call again
 * with IP at the return point: the entries are 00DC (the function) and the thirteen return points 00F4 0103 010A 0111 0120 012E 0135
 * 0147 0156 0165 016C 0173 0177; any other IP is ARGUMENT. A resumed state is arbitrary inside the profile (the callee may have changed
 * any register, flag or memory). The body reads DS:5372 (byte) and DS:5374 (word) with the live caller DS and the argument through
 * SS:[BP+6] with the live BP; it has no relocated immediate, interrupt, port access or indirect transfer. Exactly 1 MiB of A20-off RAM,
 * CPU and RAM distinct host objects; SP in [0008,FFFA] at every entry (the pushes and the RETF 2 frame neither wrap nor reach below 0);
 * code immutable; no interrupts or faults. Offsets wrap at 16 bits, physical addresses at 1 MiB, words are physically contiguous.
 * Operands are reread from RAM in source order. Flags: CMP and DEC are the real flags (DEC keeps CF), XOR AX,AX gives CF=OF=AF=SF=0,
 * ZF=PF=1, JBE after CMP word,0 is ZF; no flag is masked. */
pr_r08ae_00dc_status pr_r08ae_dispatch_00dc_cpu(uint8_t *ram, size_t ram_size, pr_r08ae_00dc_cpu *cpu);
#endif
