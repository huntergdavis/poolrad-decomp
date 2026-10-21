#ifndef POOLRAD_RECOVERED_OVERLAY_U7F_NUMBER_0294_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U7F_NUMBER_0294_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/memory_bus.h"
#include "poolrad/recovered/resident_0709_printer_0626_cpu.h"
#include "poolrad/recovered/resident_0709_space_05eb_cpu.h"
#include "poolrad/recovered/resident_0899_readkey_0059_cpu.h"

#define PR_U7F_NUMBER_RAM_SIZE (1u << 20)

/* Raw-CPU, full-state model of GAME.OVR unit 7Fh +0294..+0440 (429 bytes, 188 instructions, RETF 8), the game's numeric
 * entry line editor, OVL_007F_0294 (docs/re/U7F_NUMBER_0294_CPU.md). Entered by a far call (stub 008F:003E) or PUSH CS;
 * near CALL from +0CEB. Arguments: [BP+6] the colour byte, [BP+8] the far prompt string, [BP+0Ch] max (a word).
 *   prompt := prompt[1..40] (StrStore); clear row 24 cols 0..39 (0709:05EB); print the prompt at row 24 (0709:0626)
 *   column := length(prompt); maxtext := Str(max:5) (0AF8:1366); text := ''
 *   repeat key := 0899:0059
 *     '0'..'9': text := text + key (string load, char-to-string, concat, StrStore[6]); v := Val(text);
 *               if v > max then text := maxtext, column := promptlen + length(maxtext) else column := column + 1;
 *               print text at row 24, column promptlen, colour 0Fh
 *     08 (BS) with text <> '': text := Copy(text, 1, length-1); clear the cell at column; column := column - 1
 *   until key = 0Dh or 1Bh
 *   clear row 24; result := 0 for Esc, else Val(text)                                          (AX; RETF 8)
 * The 17 far-call segment words are the unit's fixups, read live from CS. Every callee is composed through its merged model:
 * StrStore, Str, string load/concat/char-to-string/Copy and Val (SYSTEM), the 0709 printer and space routines (EGA through
 * the bus), and RES_0899_0059 (which composes Crt, the 08AE driver, ParamStr and QUIT).
 *
 * Contract: exactly 1 MiB of A20-off RAM through the bus with an EGA attached (the printer/space contracts); IP = 0294h;
 * entry SP in [025Ah,FFF4h] so this body's own frame and pushes (deepest entry SP - 25Ah) do not wrap. The key loop
 * (+03FF JMP 02FB) is bounded by key_limit (LIMIT, stopped at +02FB before the next ReadKey call). */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u7f_cpu;

typedef struct {
    pr_r0899_key_env key;            /* RES_0899_0059's environment (keyboard, driver, QUIT) */
    size_t space_loopbacks;          /* 0709:05EB budget */
    size_t printer_loopbacks;        /* 0709:0626 outer budget */
    size_t glyph_loopbacks;          /* 0709:0626 glyph budget */
    unsigned key_limit;              /* iterations of the key loop */
} pr_u7f_number_env;

typedef enum {
    PR_U7F_NUMBER_RETURN = 0,   /* RETF 8 executed; AX = the value (0 on Esc) */
    PR_U7F_NUMBER_ARGUMENT,     /* null pointers, size, no EGA, IP not 0294h, SP outside [025Ah,FFF4h]; nothing changed */
    PR_U7F_NUMBER_EXIT,         /* RES_0899_0059 ended the process (STING + key 03 -> QUIT -> Halt) */
    PR_U7F_NUMBER_BREAK,        /* RES_0899_0059's ReadKey took the Ctrl-Break path (EXIT class) */
    PR_U7F_NUMBER_CALLEE,       /* a composed callee stopped otherwise (diagnostic: call site and its status) */
    PR_U7F_NUMBER_LIMIT,        /* key_limit reached */
    PR_U7F_NUMBER_TRANSFER      /* a callee returned somewhere other than the instruction after its call */
} pr_u7f_number_status;

typedef struct {
    uint16_t site;                   /* call site of the stopping callee, 0 when none */
    int status;                      /* that callee's own status */
    pr_r0899_key_diagnostic key;
    pr_r0709_printer_cpu_diagnostic printer;
    pr_r0709_space_cpu_diagnostic space;
    unsigned keys;                   /* RES_0899_0059 calls made */
} pr_u7f_number_diagnostic;

pr_u7f_number_status pr_u7f_number_0294_cpu(pr_bus *bus, size_t ram_size, pr_u7f_cpu *cpu, const pr_u7f_number_env *env,
                                            pr_u7f_number_diagnostic *diagnostic);
#endif
