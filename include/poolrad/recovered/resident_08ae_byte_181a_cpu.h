#ifndef POOLRAD_RECOVERED_RESIDENT_08AE_BYTE_181A_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_08AE_BYTE_181A_CPU_H
#include <stddef.h>
#include <stdint.h>
#define PR_R08AE_181A_RAM_SIZE (1u << 20)
typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_r08ae_181a_cpu;
typedef enum { PR_R08AE_181A_OK, PR_R08AE_181A_ARGUMENT } pr_r08ae_181a_status;
/* Original [181A,1830), RETF 2. Exactly 1 MiB A20-off ordinary RAM;
 * IP=181A, SP in [6,FFFA]. Code immutable; reads/writes outside MMIO;
 * CPU and RAM distinct. MOV AX's relocated operand at CS:1820 is live.
 * All stack/data aliases remain live. No interrupts/faults or arbitrary-IP
 * resume. Flags untouched. ARGUMENT preserves CPU and RAM. */
pr_r08ae_181a_status pr_r08ae_byte_181a_cpu(
    uint8_t *ram, size_t ram_size, pr_r08ae_181a_cpu *cpu);
#endif
