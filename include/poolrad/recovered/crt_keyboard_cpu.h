#ifndef POOLRAD_RECOVERED_CRT_KEYBOARD_CPU_H
#define POOLRAD_RECOVERED_CRT_KEYBOARD_CPU_H
#include <stddef.h>
#include <stdint.h>

#define PR_CRT_KEYBOARD_RAM_SIZE (1u << 20)

/* Raw-CPU, full-state models of the Turbo Pascal Crt keyboard routines in START.EXE segment 0A40.
 * They stay DECLARED_BOUNDARY in accounting with no credit: the C exists only so that a strict caller
 * (RES_0899_0059) can compose the real bytes (docs/re/CRT_KEYPRESSED_02FD_CPU.md).
 *
 * BIOS INT 16h is the single DECLARED environment input. The service receives the CPU at the INT (IP of
 * the INT instruction, AH the function) and returns the complete CPU after the BIOS returns, at the next
 * instruction. Only AH 00h (read key) and 01h (key pending, ZF) are listed; any other AH at an INT is an
 * ENV refusal, never a default return. The model refuses (SERVICE) a declined call or a result whose CS,
 * IP, SS, SP or BP is not that continuation. The service must not write RAM: the BIOS's own interrupt
 * frame and stack use below SP are outside these models (stated limit). Stack words are linear-contiguous
 * with 1 MiB wrap; the entry SP guard keeps the return frame from crossing SS:FFFF. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_crt_cpu;

typedef enum {
    PR_CRT_RETURNED = 0,  /* RETF executed */
    PR_CRT_ARGUMENT,      /* null RAM/CPU/service, RAM not exactly 1 MiB, wrong entry IP, or SP above FFFCh; nothing changed */
    PR_CRT_SERVICE,       /* the INT 16h service declined or returned a non-continuation; stopped at the INT */
    PR_CRT_ENV            /* an INT 16h function other than 00h/01h would be issued; stopped at the INT */
} pr_crt_status;

/* Returns 0 with *after filled, or nonzero to decline. */
typedef int (*pr_crt_int16_service)(void *context, const pr_crt_cpu *before, pr_crt_cpu *after);

/* 0A40:[02FD,030F) KeyPressed, 18 bytes, 8 instructions, RETF: AL := 1 if the buffered extended-key byte
 * DS:[8501] is non-zero; otherwise INT 16h AH=01h and AL := ZF ? 0 : 1 (AH and the flags as the BIOS left
 * them). */
pr_crt_status pr_crt_keypressed_02fd(uint8_t *ram, size_t ram_size, pr_crt_cpu *cpu,
                                     pr_crt_int16_service service, void *context);
#endif
