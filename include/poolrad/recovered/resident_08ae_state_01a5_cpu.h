#ifndef POOLRAD_RECOVERED_RESIDENT_08AE_STATE_01A5_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_08AE_STATE_01A5_CPU_H
#include <stddef.h>
#include <stdint.h>
#define PR_R08AE_01A5_RAM_SIZE (1u << 20)
typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_r08ae_01a5_cpu;
typedef enum {
    PR_R08AE_01A5_RETURN = 0,   /* RETF 2 executed: IP/CS popped, SP advanced past the argument word */
    PR_R08AE_01A5_ARGUMENT,     /* host or profile violation; nothing changed */
    PR_R08AE_01A5_CALL_13B1,    /* near CALL 13B1 executed (CS and the return IP 01C2 pushed); IP = 13B1 */
    PR_R08AE_01A5_CALL_137C     /* near CALL 137C executed (CS and the return IP 01C8 pushed); IP = 137C */
} pr_r08ae_01a5_status;
/* Original 08AE [01A5,01D2), 45 bytes / 18 instructions, RETF 2: a far procedure with one WORD argument A at [BP+6]. When DS:5372 == 2
 * it returns at once. Otherwise A == 0 calls 137C, and A != 0 calls 13B1 only when the WORD DS:5374 is zero; after either path (or
 * the skipped call) it stores A at DS:5374 and returns. Both calls are PUSH CS; near CALL with no pushed argument. The callees are STOPS:
 * the model returns the CALL_* status in the state at the callee's entry (return IP and CS pushed, IP = the callee) and does not model the
 * callee. To continue after a callee returns, set the post-return CPU state and call again with IP at the return point; the entries are
 * 01A5 (the function) and 01C2 (after 13B1) and 01C8 (after 137C); any other IP is ARGUMENT. A resumed state is arbitrary inside the
 * profile. DS is the live caller value, the argument is read through the live BP; the body has no relocated immediate, interrupt, port
 * access or indirect transfer. Exactly 1 MiB of A20-off RAM, CPU and RAM distinct host objects; SP in [0006,FFFA] at every entry (the
 * pushes and the RETF 2 frame neither wrap nor reach below 0); code immutable; no interrupts or faults. Offsets wrap at 16 bits, physical
 * addresses at 1 MiB, words are physically contiguous. Operands are reread from RAM in source order, so the stack frame may alias
 * DS:5374. Flags: CMP is the real SUB flags and JBE after CMP word,0 is ZF; the epilogue changes no flag. */
pr_r08ae_01a5_status pr_r08ae_state_01a5_cpu(uint8_t *ram, size_t ram_size, pr_r08ae_01a5_cpu *cpu);
#endif
