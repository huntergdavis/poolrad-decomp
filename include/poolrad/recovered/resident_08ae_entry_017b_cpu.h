#ifndef POOLRAD_RECOVERED_RESIDENT_08AE_ENTRY_017B_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_08AE_ENTRY_017B_CPU_H
#include <stddef.h>
#include <stdint.h>
#define PR_R08AE_017B_RAM_SIZE (1u << 20)
typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_r08ae_017b_cpu;
typedef enum {
    PR_R08AE_017B_RETURN = 0,   /* RETF executed: IP/CS popped, SP advanced by 6 from the POP BP */
    PR_R08AE_017B_ARGUMENT,     /* host or profile violation; nothing changed */
    PR_R08AE_017B_CALL_137C,    /* near CALL 137C executed (CS and the return IP pushed); IP = 137C */
    PR_R08AE_017B_CALL_1742,    /* near CALL 1742 executed; IP = 1742 */
    PR_R08AE_017B_CALL_17AE,    /* near CALL 17AE executed; IP = 17AE */
    PR_R08AE_017B_CALL_13B1     /* near CALL 13B1 executed; IP = 13B1 */
} pr_r08ae_017b_status;
/* Original 08AE [017B,01A5), 42 bytes / 21 instructions, RETF (no immediate). Reads DS:5372 into AL, stores it at DS:5373,
 * compares DS:5372 with 2 and, when equal, jumps to the epilogue; otherwise calls, in order, 137C, 1742 (argument 0), 17AE
 * (argument 0) and 13B1 with the far-return convention (PUSH CS; near CALL), then POP BP; RETF. DS is the live caller value.
 * The body has no relocated immediate, no interrupt, no port access and no indirect transfer. Its four callees are STOPS: the
 * model returns the CALL_* status in the state at the callee's entry (return IP and CS pushed, IP = the callee) and does not
 * model the callee. To continue after a callee returns, set the CPU to the post-return state and call again with IP at the
 * return point: the entries are 017B (the function) and the return points 0191, 0198, 019F and 01A3; any other IP is ARGUMENT.
 * A resumed state is arbitrary (the callee may have changed any register, flag or memory) inside the profile. Exactly 1 MiB
 * of A20-off RAM, CPU and RAM distinct host objects; SP in [0006,FFFA] at every entry (the pushes and the RETF frame neither
 * wrap nor reach below 0); code immutable; no interrupts or faults. Offsets wrap at 16 bits, physical addresses at 1 MiB, words are
 * physically contiguous. Operands are reread from RAM in source order, so the stack frame may alias DS:5373. Flags: XOR AX,AX
 * gives CF=OF=AF=SF=0 and ZF=PF=1; CMP is the byte SUB flags; the epilogue changes no flag. */
pr_r08ae_017b_status pr_r08ae_entry_017b_cpu(uint8_t *ram, size_t ram_size, pr_r08ae_017b_cpu *cpu);
#endif
