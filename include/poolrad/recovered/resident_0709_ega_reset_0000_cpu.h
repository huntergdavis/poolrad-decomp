#ifndef POOLRAD_RECOVERED_RESIDENT_0709_EGA_RESET_0000_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_0709_EGA_RESET_0000_CPU_H

#include <stddef.h>
#include <stdint.h>

#include "poolrad/recovered/port_env.h"

#define PR_R0709_EGA_RESET_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_r0709_ega_reset_cpu;

typedef enum {
    PR_R0709_EGA_RESET_OK = 0,
    PR_R0709_EGA_RESET_ARGUMENT,  /* null RAM/CPU/env/out, RAM that is not exactly 1 MiB, or IP not 0000h; nothing changed */
    PR_R0709_EGA_RESET_PORT       /* env->out refused an OUT; stopped at it with AL and DX loaded */
} pr_r0709_ega_reset_status;

/* Raw-CPU, full-state model of START.EXE resident 0709:0000..0070 (113
 * bytes, 58 instructions, call-free), the first port-bound routine of the
 * RES 00BA:16EC closure. Entered after a far call; RETF. PUSH BP; MOV BP,SP;
 * then eighteen byte OUTs, each MOV AL,imm8; MOV DX,imm16; OUT DX,AL, which
 * reset the EGA graphics controller (index/data at 3CEh/3CFh: registers 0,
 * 1, 2, 3, 4 and 5 := 0, 7 := 0Fh, 8 := FFh) and the sequencer map mask (3C4h
 * index 2, 3C5h := 0Fh); POP BP; RETF. No instruction changes FLAGS. After
 * a return AL = 0Fh (AH kept) and DX = 03C5h. Each OUT goes to env->out at
 * its own CS:IP in order; the POP and the RETF frame are read physically
 * contiguous; offsets wrap at 16 bits, physical addresses at 1 MiB. The
 * code must sit at its original offsets (IP = 0000h). */
pr_r0709_ega_reset_status pr_r0709_ega_reset_0000_cpu(uint8_t *ram, size_t ram_size, pr_r0709_ega_reset_cpu *cpu,
                                                      const pr_port_env *env);

#endif
