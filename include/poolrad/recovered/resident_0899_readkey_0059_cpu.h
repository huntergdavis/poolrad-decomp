#ifndef POOLRAD_RECOVERED_RESIDENT_0899_READKEY_0059_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_0899_READKEY_0059_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/crt_keyboard_cpu.h"
#include "poolrad/recovered/resident_0899_quit_0000_cpu.h"

#define PR_R0899_KEY_RAM_SIZE (1u << 20)

/* Raw-CPU, full-state model of START.EXE resident 0899:[0059,0104) (171 bytes, 61 instructions), the game's ReadKey with
 * hotkeys (docs/re/RES_0899_READKEY_0059_CPU.md). Far procedure, no argument, RETF; AL returns the key. Frame: [BP-2] the
 * key, [BP-1] the result, [BP-102h] a 256-byte string.
 *   0060 key := ReadKey (Crt 0A40:030F)
 *   0068 key = 13h: if [5372] <> 2 then [5373] := [5372]; 08AE:00DC([2612]); [5372] := 2      (sound off)
 *                   else if [5373] IN CS:0033 ({0,1}, SYSTEM 0DC0) then [5372] := [5373]; 08AE:017B   (sound on)
 *   00A6 ParamStr(1) (SYSTEM 16BC) into [BP-102h]; compare with the CS literal 'STING' at CS:0053 (SYSTEM 0C10)
 *   00C1 only when equal: key = 04 toggles [49F7]; key = 03 does PUSH CS; CALL 0000 (QUIT: EXIT, never returns)
 *   00DE if key <> 0: while KeyPressed (Crt 02FD) do key := ReadKey          (drain, keeping the last key)
 *   00F7 result := key; AL := result; MOV SP,BP; POP BP; RETF
 * The eight far-call segment words (CS:0063, 0082, 0097, 00A4, 00B3, 00BD, 00E7, 00F0) are read live (loader relocations).
 *
 * Composed, each through its merged model and real interface: Crt ReadKey/KeyPressed (INT 16h the declared service),
 * the 08AE driver entries 00DC and 017B (resident_08ae_driver_cpu.h), SYSTEM set IN, string compare and ParamStr (pure
 * memory), and RES_0899_0000 QUIT. Exactly 1 MiB of A20-off RAM; IP = 0059h; entry SP in [0112h,FFFEh] so this body's own
 * pushes (deepest entry SP - 112h, at the set-IN and ParamStr calls) do not wrap. */
typedef struct {
    pr_r0899_quit_env quit;              /* the driver environment (also used by 00DC/017B here) and QUIT's INT 10h handler */
    pr_crt_int16_service keyboard;       /* BIOS INT 16h, AH 00h/01h */
    void *keyboard_context;
    unsigned drain_limit;                /* iterations of the 00E4 drain loop before the LIMIT stop */
} pr_r0899_key_env;

typedef enum {
    PR_R0899_KEY_RETURN = 0,   /* RETF executed; AL = the key */
    PR_R0899_KEY_ARGUMENT,     /* null pointers, size, IP not 0059h, SP outside [0112h,FFFEh]; nothing changed */
    PR_R0899_KEY_EXIT,         /* key 03 with ParamStr(1) = 'STING': QUIT reached Halt; the process exits */
    PR_R0899_KEY_BREAK,        /* a Crt ReadKey call took its Ctrl-Break path (EXIT class, CRT_READKEY_030F_CPU.md) */
    PR_R0899_KEY_CRT,          /* ReadKey/KeyPressed stopped otherwise (SERVICE, ENV, ARGUMENT; diagnostic) */
    PR_R0899_KEY_DRIVER,       /* the composed 08AE driver stopped (diagnostic) */
    PR_R0899_KEY_LEAF,         /* SYSTEM set IN, string compare or ParamStr refused (diagnostic) */
    PR_R0899_KEY_QUIT,         /* QUIT stopped without reaching Halt (diagnostic carries its status and diagnostic) */
    PR_R0899_KEY_LIMIT,        /* the drain loop ran drain_limit times; stopped at 00E4 before the next KeyPressed */
    PR_R0899_KEY_TRANSFER      /* a callee returned somewhere other than the instruction after its call */
} pr_r0899_key_status;

typedef struct {
    uint16_t callee;                     /* call site offset of the stopping callee, 0 when none */
    int status;                          /* that callee's own status */
    pr_r08ae_driver_diagnostic driver;
    pr_r0899_quit_diagnostic quit;
    unsigned readkeys, keypressed;       /* Crt calls made */
} pr_r0899_key_diagnostic;

pr_r0899_key_status pr_r0899_readkey_0059_cpu(uint8_t *ram, size_t ram_size, pr_r0899_cpu *cpu,
                                              const pr_r0899_key_env *env, pr_r0899_key_diagnostic *diagnostic);
#endif
