#ifndef POOLRAD_RECOVERED_OVERLAY_U25_COMPARE_WORDS_18EB_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U25_COMPARE_WORDS_18EB_CPU_H
#include <stddef.h>
#include <stdint.h>

#define PR_U25_18EB_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u25_18eb_cpu;

typedef enum {
    PR_U25_18EB_RETURNED = 0,
    PR_U25_18EB_ARGUMENT,   /* null RAM/CPU, RAM not exactly 1 MiB, IP not 18EBh or entry SP outside [000Eh,FFF8h]; nothing changed */
    PR_U25_18EB_CODE,       /* the LCALL at +18FA is not 9A A8 16 in RAM; the CPU and RAM are left at that instruction */
    PR_U25_18EB_TRANSFER    /* FillChar or RETF 4 returned somewhere other than the pushed address; the CPU is left at the popped CS:IP */
} pr_u25_18eb_status;

/* GAME.OVR unit 0x25 [18EB,1951): the ECL word compare (public entry 16, run-time stub 2B:70), 102 bytes / 36 instructions, raw CPU. Enter
 * after the far call at IP 18EB with SS:[SP] return IP, [SP+2] return CS, [SP+4] b and [SP+6] a. PUSH BP; MOV BP,SP; then FillChar(DS:6F78,
 * 6, 0) through the LCALL 0AF8:16A8 at +18FA, composed with the raw SYSTEM FillChar (pr_system_fillchar_16a8, segment word read from the
 * loaded code at +18FD); then six times MOV AX,[BP+8]; CMP AX,[BP+6]; Jcc; MOV BYTE [6F78+k],1 for equal, not equal, below, above, below or
 * equal and above or equal (unsigned). POP BP; RETF 4. Every operand is reread from RAM in instruction order, so a flag byte written on the
 * frame is seen by the later compares. The FULL pinned flags of each CMP are computed; the last reaches the RETF, with DF clear from
 * FillChar. Exactly 1 MiB of A20-off RAM; entry SP in [000Eh,FFF8h] (no stack wrap); code is immutable. The pr_rec_ecl_compare_words
 * contract in ecl_helpers.h is the semantic view of the same body; this is its one raw translation. */
pr_u25_18eb_status pr_u25_compare_words_18eb_cpu(uint8_t *ram, size_t ram_size, pr_u25_18eb_cpu *cpu);
#endif
