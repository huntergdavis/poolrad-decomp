#ifndef POOLRAD_RESIDENT_1816_PRINTER_GATE_H
#define POOLRAD_RESIDENT_1816_PRINTER_GATE_H

#include <stddef.h>
#include <stdint.h>

#define PR_1816_GATE_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp;
    uint16_t cs, ip, ds, es, ss, flags;
} pr_1816_gate_cpu;

/* Execute RES_00BA:1816 through the first far-call entry at 0709:0626.
 * RAM is a flat 20-bit real-mode address space. The printer body is outside
 * this boundary. Return 0 on success, 1 for invalid arguments. */
int pr_1816_first_printer_gate(uint8_t *ram, size_t ram_size,
                               pr_1816_gate_cpu *cpu);

#endif
