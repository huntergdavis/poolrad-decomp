#ifndef POOLRAD_RECOVERED_RESIDENT_0899_QUIT_0000_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_0899_QUIT_0000_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/dos_intr_000b_cpu.h"
#include "poolrad/recovered/resident_08ae_driver_cpu.h"

#define PR_R0899_QUIT_RAM_SIZE (1u << 20)

/* Raw-CPU, full-state model of START.EXE resident 0899:[0000,0033) (51 bytes, 19 instructions), the game's QUIT
 * routine (docs/re/RES_0899_QUIT_0000_CPU.md). Entered by a far call (overlay callers) or PUSH CS; near CALL (0899:0059
 * key 03); it takes no argument.
 *   0000 PUSH BP; MOV BP,SP; [5372] := [5373] (restore the saved sound state)
 *   0009 PUSH [2612]; LCALL 08AE:00DC        composed: the driver dispatch with its member models
 *   0012 [4A03] := 0; [4A02] := [4A1C]       Regs.AX at DS:4A02 = AH 0 (set mode), AL the saved video mode
 *   001D PUSH 10h; PUSH DS; PUSH 4A02h; LCALL 0A32:000B   composed: Dos.Intr(10h, Regs) (#1259)
 *   002A XOR AX,AX; LCALL 0AF8:00E9          SYSTEM Halt(0): the declared EXIT boundary (#1260)
 *   0031 POP BP; RETF                        DEAD: Halt never returns
 * Every path ends at EXIT: the model stops with the Halt call executed (return 0899:0031 pushed, CS:IP = the live far
 * target, AX = 0 the exit code) and never returns. The three far-call segment words are read live from CS:0010, CS:0028
 * and CS:002F (loader relocations).
 *
 * Declared environment: the driver environment (DOS vector service, ports, the GetTime INT 21h/2Ch service, 006C's step
 * limit; resident_08ae_driver_cpu.h) and the INT 10h handler that Intr transfers to (dos_intr_000b_cpu.h). Exactly
 * 1 MiB of A20-off RAM; IP = 0000h; entry SP in [000Ch,FFFEh] so this body's own pushes (deepest entry SP - 0Ch) do not
 * wrap; deeper frames are guarded by the composed models, whose refusal is a named stop. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_r0899_cpu;

typedef struct {
    pr_r08ae_driver_env driver;
    pr_dos_intr_service video;   /* the INT 10h handler Intr transfers to */
    void *video_context;
} pr_r0899_quit_env;

typedef enum {
    PR_R0899_QUIT_EXIT = 0,   /* LCALL 0AF8:00E9 (Halt) executed: the process exits; never a return */
    PR_R0899_QUIT_ARGUMENT,   /* null pointers, size, IP not 0000h, SP outside [000Ch,FFFEh]; nothing changed */
    PR_R0899_QUIT_DRIVER,     /* the composed 08AE driver stopped (diagnostic: member and status) */
    PR_R0899_QUIT_INTR,       /* Dos.Intr did not return (diagnostic: its status; SERVICE = the INT 10h handler) */
    PR_R0899_QUIT_TRANSFER    /* a callee returned somewhere other than the instruction after its call */
} pr_r0899_quit_status;

typedef struct {
    pr_r08ae_driver_diagnostic driver;
    int intr_status;          /* pr_dos_intr_status, -1 when not called */
} pr_r0899_quit_diagnostic;

pr_r0899_quit_status pr_r0899_quit_0000_cpu(uint8_t *ram, size_t ram_size, pr_r0899_cpu *cpu,
                                            const pr_r0899_quit_env *env, pr_r0899_quit_diagnostic *diagnostic);
#endif
