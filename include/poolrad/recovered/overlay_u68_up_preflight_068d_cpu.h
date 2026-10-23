#ifndef POOLRAD_RECOVERED_OVERLAY_U68_UP_PREFLIGHT_068D_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U68_UP_PREFLIGHT_068D_CPU_H
#include "poolrad/recovered/memory_bus.h"

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u68_068d_cpu;

typedef enum {
    PR_U68_068D_RETURNED = 0,
    PR_U68_068D_ARGUMENT,   /* null bus/RAM/CPU, RAM not exactly 1 MiB, IP not 068Dh or entry SP outside [0030h,FFFCh]; nothing changed */
    PR_U68_068D_OVERLAY,    /* the LCALL's stub is not a loaded far JMP to unit 0x3BD +0350 (an unloaded stub holds INT 3Fh): the CPU is left AT THE STUB with the
                               far return frame and the three arguments pushed, exactly where the original enters the overlay manager, which this model does not run */
    PR_U68_068D_CALLEE,     /* the composed +0350 model rejected its entry (its status is not RETURNED/OK); the CPU is left at +0350 in the stub's target segment */
    PR_U68_068D_TRANSFER    /* +0350's RETF 6, or this body's RETF, popped a return address other than its own frame's; the CPU is left at the popped CS:IP */
} pr_u68_068d_status;

/* GAME.OVR unit 0x68 [068D,073B): the Up preflight, 174 bytes / 55 instructions, RETF (no arguments). Enter after the far call at IP 068D.
 * Reads the party x/y/facing bytes DS:[6AAD]/[6AAE]/[6AAF] into locals (x, y sign-extended; facing zero-extended), clears the word at
 * ES:[DI+5AAh] (ES:DI = DS:[49D6]), then LCALL 03D5:002F, the overlay stub of unit 0x3BD public entry 3 (+0350), with x, y and facing pushed.
 * THE STUB IS READ, NOT ASSUMED: the LCALL's segment word (relocated, at CS:06C3) and offset (CS:06C1) are read from RAM, and the stub's five
 * bytes must be EAh, offset 0350h, segment S; the composed pr_u3bd_field_0350_fullstate then runs at S:0350. Otherwise OVERLAY.
 * On a non-zero result it adds the facing-indexed steps DS:[2736+f] and DS:[273F+f] (signed bytes) to the x and y locals and clamps each to
 * 0..15: a value above 15 stores 0Fh to the party byte, below 0 stores 00h, and each clamp sets the word at ES:[DI+5AAh] to 1 (DS:[49D6]
 * re-read each time). The candidate is never stored when it stays in range. The two 5AAh-word stores go THROUGH THE BUS (the far pointer comes
 * from memory); the DS globals, tables, frame and stack are flat. Flags at return are those of the last CMP executed, or of OR AL,AL on a zero
 * result. Exactly 1 MiB of A20-off RAM; entry SP in [0030h,FFFCh]. Code is immutable. */
pr_u68_068d_status pr_u68_up_preflight_068d_cpu(pr_bus *bus, size_t ram_size, pr_u68_068d_cpu *cpu);
#endif
