#ifndef POOLRAD_RECOVERED_OVERLAY_U3BD_ENTRY1_0155_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U3BD_ENTRY1_0155_CPU_H
#include <stddef.h>
#include <stdint.h>

#define PR_U3BD_0155_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u3bd_0155_cpu;

typedef enum {
    PR_U3BD_0155_RETURNED = 0,
    PR_U3BD_0155_ARGUMENT,   /* null RAM/CPU/budget, RAM not exactly 1 MiB, IP not 0155h or entry SP outside [0002h,FFF4h]; nothing changed */
    PR_U3BD_0155_LIMIT,      /* the host budget ran out at the next unexecuted instruction (cpu->ip) */
    PR_U3BD_0155_TRANSFER    /* RETF 8 popped a return address other than the entry frame's (the body's DS stores can reach it when DS:6A60..6A63 overlaps the frame); the CPU is left at the popped CS:IP */
} pr_u3bd_0155_status;

/* GAME.OVR unit 0x3BD [0155,0174): public entry 1, 31 bytes / 12 instructions, a straight-line leaf. Enter after the far call at IP 0155.
 * Frame: SP IP, SP+2 CS, SP+4 / +6 / +8 / +0A four argument word slots (the fourth, third, second and first pushed), each read as its LOW byte.
 * PUSH BP; MOV BP,SP; then DS:[6A60] := [BP+0C], DS:[6A61] := [BP+0A], DS:[6A62] := [BP+8], DS:[6A63] := [BP+6], each through AL (AL ends as the last
 * byte); POP BP; RETF 8. No instruction writes the flags. Every operand is reread from RAM in source order, so a store that lands on a later argument
 * or on the frame is seen. DS globals and the stack are flat RAM (the body dereferences no caller-supplied pointer). Exactly 1 MiB of A20-off RAM;
 * entry SP in [0002h,FFF4h] so no word access straddles the segment end. Each instruction consumes one unit of *budget; LIMIT leaves the next
 * unexecuted instruction in cpu->ip. CPU, budget and RAM are distinct. Code is immutable. */
pr_u3bd_0155_status pr_u3bd_entry1_0155_cpu(uint8_t *ram, size_t ram_size, pr_u3bd_0155_cpu *cpu, size_t *budget);
#endif
