#ifndef POOLRAD_RECOVERED_RESIDENT_08AE_DELAY_006C_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_08AE_DELAY_006C_CPU_H
#include <stddef.h>
#include <stdint.h>
#define PR_R08AE_006C_RAM_SIZE (1u << 20)
typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_r08ae_006c_cpu;
typedef enum {
    PR_R08AE_006C_OK = 0, PR_R08AE_006C_ARGUMENT, PR_R08AE_006C_CODE,
    PR_R08AE_006C_TIME, PR_R08AE_006C_LIMIT
} pr_r08ae_006c_status;
/* The time source is the original RES_08AE_0000, reached by `PUSH CS; CALL 0`
 * at CS:0072/0073 and CS:0089/008A. The hook is called with the two-word
 * return frame already pushed and IP=0000. It must set the registers the
 * callee leaves (DX:AX is the 32-bit time; BX/CX/DI/flags are callee-owned)
 * and may write RAM below SP; it must NOT pop the frame, the model performs
 * the RETF from the live stack words afterwards. A nonzero return is the named
 * TIME stop and leaves CPU/RAM exactly as the hook found them. */
typedef int (*pr_r08ae_006c_time_fn)(void *context, uint8_t *ram,
                                     pr_r08ae_006c_cpu *cpu);
/* Original [006C,00DC), RETF 2, 112 bytes / 46 instructions: wait until the
 * 32-bit time from RES_08AE_0000 reaches (first reading + WORD argument), with
 * a guard that treats a reading more than the argument below the target as a
 * wrapped clock. Exactly 1 MiB A20-off ordinary RAM; IP=006C, SP in [000E,FFFA]
 * (the frame, eight locals, call frames and argument stay inside one SS offset space). Code
 * immutable; no MMIO; CPU/RAM distinct; no interrupts, faults or arbitrary-IP
 * resume. CODE (both CALL sites must hold E8 with displacement to CS:0000)
 * stops before the pushes of that call. LIMIT counts dispatch steps (each
 * original instruction, with `PUSH CS; CALL` counted as one step) and stops before the next. ARGUMENT preserves CPU/RAM. */
pr_r08ae_006c_status pr_r08ae_delay_006c_cpu(
    uint8_t *ram, size_t ram_size, pr_r08ae_006c_cpu *cpu,
    pr_r08ae_006c_time_fn time, void *context, unsigned max_steps);
#endif
