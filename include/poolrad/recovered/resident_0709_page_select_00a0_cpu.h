#ifndef POOLRAD_RECOVERED_RESIDENT_0709_PAGE_SELECT_00A0_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_0709_PAGE_SELECT_00A0_CPU_H

#include <stddef.h>
#include <stdint.h>

#define PR_R0709_PAGE_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_r0709_page_cpu;

typedef enum {
    PR_R0709_PAGE_OK = 0,
    PR_R0709_PAGE_ARGUMENT  /* null RAM/CPU, RAM that is not exactly 1 MiB, or IP not 00A0h; nothing changed */
} pr_r0709_page_status;

/* Raw-CPU, full-state model of START.EXE resident 0709:00A0..00B6 (23
 * bytes, 10 instructions, call-free and port-free), the EGA page select of
 * the RES 00BA:16EC closure, called by the rect fill 0709:00B7. Entered after
 * a far call with the page word at [SP+4]; RETF 2. PUSH BP; MOV BP,SP;
 * MOV AL,[BP+6]; XOR AH,AH; MOV CX,9; SHL AX,CL; ADD AX,0A000h; MOV
 * DS:[4A18h],AX; POP BP; RETF 2. So DS:[4A18h] := A000h + 200h * page (16-bit,
 * so page 80h wraps to A000h), AX = that word, CX = 9, and the flags are ADD's
 * (SHL's never survive). The word store precedes the POP and the RETF, which
 * read RAM physically contiguous afterwards, so a store that aliases the
 * frame changes them as on the CPU; offsets wrap at 16 bits, physical
 * addresses at 1 MiB. The code must sit at its original offsets (IP = 00A0h). */
pr_r0709_page_status pr_r0709_page_select_00a0_cpu(uint8_t *ram, size_t ram_size, pr_r0709_page_cpu *cpu);

#endif
