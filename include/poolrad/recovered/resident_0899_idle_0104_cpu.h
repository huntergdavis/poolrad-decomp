#ifndef POOLRAD_RECOVERED_RESIDENT_0899_IDLE_0104_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_0899_IDLE_0104_CPU_H

#include <stddef.h>
#include <stdint.h>

#define PR_R0899_IDLE_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_r0899_idle_cpu;

typedef enum {
    PR_R0899_IDLE_OK = 0,
    PR_R0899_IDLE_ARGUMENT  /* null RAM/CPU, RAM that is not exactly 1 MiB, or IP not 0104h; nothing changed */
} pr_r0899_idle_status;

/* Raw-CPU, full-state model of START.EXE resident 0899:0104..0121 (30
 * bytes), a pure callee of the RES 00BA:16EC closure's 0709:0768. From its
 * entry only seven instructions are reachable: PUSH BP; MOV BP,SP; SUB
 * SP,2 (its flags are the routine's); JMP 011E; MOV SP,BP; POP BP; RETF.
 * The KeyPressed/ReadKey loop at 010C..011D is never reached from 0104.
 * The POP and the RETF frame are read physically contiguous; offsets wrap at
 * 16 bits, physical addresses at 1 MiB. The code must sit at its original
 * offsets (IP = 0104h). */
pr_r0899_idle_status pr_r0899_idle_0104_cpu(uint8_t *ram, size_t ram_size, pr_r0899_idle_cpu *cpu);

#endif
