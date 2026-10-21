#ifndef POOLRAD_RECOVERED_RESIDENT_08AE_VECTOR_137C_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_08AE_VECTOR_137C_CPU_H
#include <stddef.h>
#include <stdint.h>
#define PR_R08AE_137C_RAM_SIZE (1u << 20)
typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_r08ae_137c_cpu;
typedef enum {
    PR_R08AE_137C_OK = 0, PR_R08AE_137C_ARGUMENT, PR_R08AE_137C_ENV_DOS,
    PR_R08AE_137C_ENV_PORT
} pr_r08ae_137c_status;
/* Declared environment. Neither hook is the real DOS or PIT.
 *
 * dos: called for each `INT 21h` after the hardware frame (FLAGS, CS, next IP) is
 * pushed. It performs the documented DOS vector-table service ONLY (AH=35h loads
 * ES:BX, AH=25h stores DX:DS, both indexed by AL) over a table the environment owns,
 * and reports the flags the handler returns with. It must refuse (nonzero, changing
 * nothing) any other AH. The model then returns as DOS does, `RETF 2`: it pops IP
 * and CS, drops the pushed FLAGS word and loads *flags_out.
 * port_out: called before each `OUT`; nonzero refuses that port. */
typedef struct {
    void *context;
    int (*dos)(void *context, uint8_t *ram, pr_r08ae_137c_cpu *cpu, uint16_t *flags_out);
    int (*port_out)(void *context, uint16_t port, uint8_t value);
} pr_r08ae_137c_env;
/* Original [137C,13B1), RETF, 53 bytes / 24 instructions, straight-line. Saves the
 * current INT 08h vector at CS:1378 (offset) and CS:137A (segment) -- FOUR BYTES
 * BEFORE THE ENTRY, outside the span -- installs DS:DX = (relocated seg):13EA with
 * INT 21h AH=25h, then writes OUT 43h<-B6h and OUT 40h<-B1h,13h. The two segment
 * immediates at CS:137F and CS:1394 are loader relocations and are read live.
 * Exactly 1 MiB A20-off ordinary RAM; IP=137C, SP in [000C,FFFC]. The span is
 * immutable and the stack (all pushes, including the two INT frames) must not overlap
 * CS:1378..13B0. Besides the stack the body writes only CS:1378..137B, and the
 * environment writes only its own vector table. No MMIO, interrupts, faults or
 * arbitrary-IP resume. ENV_DOS stops after the frame push with CS:IP at the INT;
 * ENV_PORT stops before the OUT with nothing executed. ARGUMENT preserves CPU/RAM. */
pr_r08ae_137c_status pr_r08ae_vector_137c_cpu(
    uint8_t *ram, size_t ram_size, pr_r08ae_137c_cpu *cpu, const pr_r08ae_137c_env *env);
#endif
