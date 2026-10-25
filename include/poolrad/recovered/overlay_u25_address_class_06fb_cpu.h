#ifndef POOLRAD_RECOVERED_OVERLAY_U25_ADDRESS_CLASS_06FB_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U25_ADDRESS_CLASS_06FB_CPU_H
#include <stddef.h>
#include <stdint.h>

#define PR_U25_06FB_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u25_06fb_cpu;

typedef enum {
    PR_U25_06FB_RETURNED = 0,
    PR_U25_06FB_ARGUMENT,   /* null RAM/CPU, RAM not exactly 1 MiB, IP not 06FBh or entry SP outside [0004h,FFFAh]; nothing changed */
    PR_U25_06FB_TRANSFER    /* RETF 2 popped a return address other than the entry frame's; the CPU is left at the popped CS:IP */
} pr_u25_06fb_status;

/* GAME.OVR unit 0x25 [06FB,0756): the ECL address classifier, 91 bytes / 28 instructions, a leaf (no public entry: callers PUSH CS; CALL).
 * Enter at IP 06FB with SS:[SP] return IP, [SP+2] return CS and the address word at [SP+4]. PUSH BP; MOV BP,SP; SUB SP,2; [BP-1] := 4; then
 * for the ranges 4900-4CFF, 6B00-6EFF, 9700-98FF and 9900-B6FF in order: CMP WORD [BP+6],lo; JB next; CMP WORD [BP+6],hi; JA next;
 * [BP-1] := 0, 1, 2 or 3. AL := [BP-1]; MOV SP,BP; POP BP; RETF 2. FULL pinned flags: SUB SP,2's and every CMP's; the last CMP executed
 * reaches the RETF. The address is reread for every CMP; all accesses are flat. Exactly 1 MiB of A20-off RAM; entry SP in [0004h,FFFAh].
 * The shared entry for the full-state body inside the credited overlay_u25_string_cores.c (its +12FB calls it), which also defines this
 * function. Three other static full-state
 * copies remain, in overlay_u25_store_string_0f53.c, overlay_u25_store_word_0c77.c and overlay_u25_load_word_0e0c.c (PM i22463: not now).
 * pr_rec_ecl_address_class is its semantic view. */
pr_u25_06fb_status pr_u25_address_class_06fb_cpu(uint8_t *ram, size_t ram_size, pr_u25_06fb_cpu *cpu);
#endif
