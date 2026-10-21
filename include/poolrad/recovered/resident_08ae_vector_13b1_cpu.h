#ifndef POOLRAD_RECOVERED_RESIDENT_08AE_VECTOR_13B1_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_08AE_VECTOR_13B1_CPU_H
#include <stddef.h>
#include <stdint.h>
#define PR_R08AE_13B1_RAM_SIZE (1u << 20)
typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_r08ae_13b1_cpu;
typedef enum {
    PR_R08AE_13B1_OK = 0, PR_R08AE_13B1_ARGUMENT, PR_R08AE_13B1_ENV_DOS,
    PR_R08AE_13B1_ENV_PORT
} pr_r08ae_13b1_status;
/* Declared environment. None of the hooks is real DOS, the PIT, the PPI or the PSG.
 *
 * dos: called for the one `INT 21h` after the hardware frame (FLAGS, CS, next IP) is
 * pushed. It performs the documented DOS "set interrupt vector" service ONLY (AH=25h
 * stores DX offset and DS segment into table[AL] and sets AL to a scripted value) and
 * reports the flags the handler returns with. It must refuse (nonzero, changing
 * nothing) any other AH, including AH=35h. The model then returns as DOS does,
 * `RETF 2`: pops IP and CS, drops the pushed FLAGS word and loads *flags_out.
 * port_out: called before each OUT (listed ports 40h, 61h, C0h); port_in: called for
 * the one IN (listed port 61h) and supplies the byte read. A nonzero return refuses. */
typedef struct {
    void *context;
    int (*dos)(void *context, uint8_t *ram, pr_r08ae_13b1_cpu *cpu, uint16_t *flags_out);
    int (*port_out)(void *context, uint16_t port, uint8_t value);
    int (*port_in)(void *context, uint16_t port, uint8_t *value);
} pr_r08ae_13b1_env;
/* Original [13B1,13EA), RETF, 57 bytes / 27 instructions, straight-line. Restores the
 * INT 08h vector to the pointer READ FROM CS:1378 (offset) and CS:137A (segment) -- bytes
 * outside the span, written by RES_08AE_137C -- with INT 21h AH=25h; writes the PIT
 * channel-0 count FFFFh (OUT 40h twice); clears the two low bits of port 61h (IN, AND FCh,
 * OUT) and writes 9Fh,BFh,DFh,FFh to port C0h. The segment immediate at CS:13B3 is a loader
 * relocation and is read live.
 * Exactly 1 MiB A20-off ordinary RAM; IP=13B1, SP in [000A,FFFC]; the span is immutable
 * and the stack (every push, including the INT frame) must not overlap CS:13B1..13E9 or the
 * environment's vector-table entry. The stack MAY overlap CS:1378..137B: the body then reads
 * what the pushes left there. The body writes only the stack; the environment writes only
 * its table. No MMIO, interrupts, faults or arbitrary-IP resume. ENV_DOS stops after the
 * frame push with CS:IP at the INT; ENV_PORT stops before the OUT/IN with nothing executed
 * (the IN counts as the access). ARGUMENT preserves CPU/RAM. */
pr_r08ae_13b1_status pr_r08ae_vector_13b1_cpu(
    uint8_t *ram, size_t ram_size, pr_r08ae_13b1_cpu *cpu, const pr_r08ae_13b1_env *env);
#endif
