#ifndef POOLRAD_RECOVERED_OVERLAY_U3C6_0CB9_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U3C6_0CB9_CPU_H

#include <stddef.h>
#include <stdint.h>

/* GAME.OVR unit 0x3C6 + 0x0CB9 (public entry 16, stub 0x3E0:0x70), the whole
 * function [0CB9,0E11), 344 bytes, executed against live CPU and 1 MiB A20-off
 * RAM. Evidence: docs/re/U3C6_0CB9_SCAN.md.
 *
 * Enter at loaded CS:0CB9 with SS:SP on the far return IP and CS and 14
 * argument bytes above them: [bp+6] far byte pointer P6, [bp+0xA] far byte
 * pointer P10, [bp+0xE] byte k, [bp+0x10] far pointer Q. The body stores
 * *P10 = 0 and *P6 = 0x17, finds Q's slot s through entry 15, then for
 * i = 0..3 asks entry 1 for an offset pair and entry 6 for the two cell bytes
 * at that position, and updates *P10 / *P6 from them. MOV SP,BP; POP BP;
 * RETF 0xE.
 *
 * The three near callees (`push cs; call near`) are folded exactly from their
 * original bytes and run on the same live state, with their real pushes,
 * frames, flags and RETFs: entry 15 (+0C5F, retf 4), entry 1 (+0000,
 * retf 0xC) and entry 6 (+04BE, retf 0xC). No other call is reachable, so the
 * model has no hook. The loaded bytes of [0000,0068), [04BE,052B) and
 * [0C5F,0E11) must be original; every write, including pushes, is checked
 * against them and the model stops before an aliasing byte. Every pointer and
 * table read is live (P6/P10 are re-read through LES at each original site).
 *
 * Profile: LES takes the segment word at linear EA+2, minus 0x10000 when the
 * offset is above 0xFFFC (pinned DOSBox-X normal core); word accesses are
 * physically consecutive (high byte at linear+1, A20 off). Undefined flags:
 * AF = 0 after XOR/OR/SHL; OF after SHL by 3 = MSB(result) XOR CF. */

#define PR_U3C6_0CB9_CPU_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_u3c6_0cb9_cpu;

typedef enum {
    PR_U3C6_0CB9_CPU_RETURNED = 0,     /* retf 0xE executed; CS:IP is what the frame held */
    PR_U3C6_0CB9_CPU_ARGUMENT,         /* bad pointers/size, or IP not 0x0CB9; nothing changed */
    PR_U3C6_0CB9_CPU_UNSCREENED_ALIAS, /* a write would alias a modelled code byte; IP names the
                                        * instruction, which has not executed */
    PR_U3C6_0CB9_CPU_RETURN_CHANGED    /* a folded callee's RETF popped a CS:IP other than the
                                        * one pushed; CS:IP hold the popped values */
} pr_u3c6_0cb9_cpu_status;

pr_u3c6_0cb9_cpu_status pr_u3c6_0cb9_cpu_run(uint8_t *ram, size_t ram_size, pr_u3c6_0cb9_cpu *cpu);

#endif
