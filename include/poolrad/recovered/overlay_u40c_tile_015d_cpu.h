#ifndef POOLRAD_RECOVERED_OVERLAY_U40C_TILE_015D_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U40C_TILE_015D_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/memory_bus.h"
#include "poolrad/recovered/resident_0464_blit_1951_cpu.h"
#include "poolrad/recovered/resident_0464_compose_08d4_cpu.h"
#include "poolrad/recovered/resident_0464_video_mode_cpu.h"
#include "poolrad/recovered/resident_0899_readkey_0059_cpu.h"
#include "poolrad/recovered/system_text_write_cpu.h"

/* Raw-CPU, full-state model of GAME.OVR unit 40Ch +[015D,02AD) (336 bytes, 127 instructions, RETF 0Ah): place tile
 * number [BP+0Ah] (1..11Eh) from its tile-set band. The number selects band 0..4 (1..2Dh, 2Eh..73h, 74h..B9h, BAh..FFh,
 * 100h..11Eh) into [BP-1]. 0 or 11Fh..7FFFh takes the error branch: restore the video mode (0464:00FF),
 * Write(Output, the CS:013A message), Writeln(Output), ReadKey (0899:0059) into [BP-2], then QUIT (0899:0000), which
 * never returns. 8000h..FFFFh matches no test and leaves [BP-1] as it was on the stack. Then, when the band's far
 * pointer DS:5E3A[band] is non-null, the number is rebased by the word DS:2722[band] and
 *   [BP+8] byte non-zero: 0464:08D4 (tile compositor) into the object DS:4A20 with item [BP+0Ah], flag [BP+6] and the
 *                         sign-extended bytes [BP+0Ch] and [BP+0Eh];
 *   [BP+8] byte zero:     SYSTEM Move(tile set + 17h + item * row size, DS:5E4E object + 17h, row size (+11h)), then
 *                         0464:1951 (blit) of the DS:5E4E object at the sign-extended bytes [BP+0Ch], [BP+0Eh].
 * Entered after a far call: [SP] IP, [SP+2] CS, then the five argument words; RETF 0Ah. CS is the unit's load segment;
 * the eight far-call segment words (CS:01C8, 01DA, 01DF, 01E4, 01EC, 024C, 028E, 02A5) are loader fixups, read live.
 * Each callee runs through its merged model on the same RAM. Operands are reread in source order; offsets wrap at 16
 * bits and physical addresses at 1 MiB. See docs/re/OVL_040C_015D_CPU.md for the contract and evidence. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_ovl040c_015d_regs;

typedef struct {
    pr_r0464_video_env video;   /* 00FF: the INT 10h handler behind Dos.Intr */
    pr_system_text_env text;    /* Write/Writeln: Output's InOutFunc and FlushFunc */
    pr_r0899_key_env key;       /* ReadKey: INT 16h, the 08AE driver and QUIT; QUIT itself takes key.quit */
    size_t max_loopbacks;       /* passed to 08D4 and 1951 */
} pr_ovl040c_015d_env;

typedef enum {
    PR_OVL040C_015D_OK = 0,     /* RETF 0Ah executed */
    PR_OVL040C_015D_ARGUMENT,   /* null pointers, RAM not exactly 1 MiB, IP not 015Dh; nothing changed */
    PR_OVL040C_015D_CALLEE,     /* a callee stopped (diagnostic: site and its status); state as it left it */
    PR_OVL040C_015D_TRANSFER,   /* a callee returned somewhere other than the instruction after its call */
    PR_OVL040C_015D_CONTRACT,   /* EGA attached and a flat-RAM callee (error branch) or Move meeting A0000..AFFFF; stopped at its entry */
    PR_OVL040C_015D_EXIT        /* QUIT reached Halt, or ReadKey exited (its key 03 QUIT or the Crt Ctrl-Break path) */
} pr_ovl040c_015d_status;

typedef struct {
    uint16_t site;              /* the far-call site of the stopping (or exiting) callee, 0 when none */
    int status;                 /* that callee's own status, -1 when none */
    int service;                /* 00FF: Dos.Intr status; 0919/084A: the driver result; otherwise -1 */
    unsigned calls;             /* far calls entered */
    pr_r0899_key_diagnostic key;
    pr_r0899_quit_diagnostic quit;
    pr_r0464_compose_cpu_diagnostic compose;
    pr_r0464_blit_cpu_diagnostic blit;
} pr_ovl040c_015d_diagnostic;

pr_ovl040c_015d_status pr_ovl040c_015d_cpu(pr_bus *bus, size_t ram_size, pr_ovl040c_015d_regs *cpu,
                                           const pr_ovl040c_015d_env *env, pr_ovl040c_015d_diagnostic *diagnostic);
#endif
