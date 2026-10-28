#ifndef POOLRAD_RECOVERED_RESIDENT_00BA_0973_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_00BA_0973_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/memory_bus.h"
#include "poolrad/recovered/resident_0709_printer_0626_cpu.h"

#define PR_RES_0973_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_res_0973_cpu;

typedef enum {
    PR_RES_0973_RETURNED = 0,
    PR_RES_0973_ARGUMENT,
    PR_RES_0973_CODE,
    PR_RES_0973_CALLEE,
    PR_RES_0973_TRANSFER,
    PR_RES_0973_LIMIT
} pr_res_0973_status;

typedef struct {
    size_t printer_loopbacks;       /* 0709:0626 outer budget, each entry */
    size_t printer_glyph_loopbacks; /* 0709:0626 +01DF budget, each glyph call */
} pr_res_0973_env;

typedef struct {
    uint16_t site;
    int callee;                     /* -1 none, 0 1037, 1 printer 0626 */
    int callee_status;
    pr_r0709_printer_cpu_diagnostic printer;
} pr_res_0973_diagnostic;

/* Raw-CPU model of START.EXE resident 00BA:[0973,09C9), 86 bytes / 32
 * instructions, RETF 0Ah. Entered after the far call with five argument
 * words: node offset/segment, x, y and the override byte. Reads node+011Bh
 * and node+0032h, selects style 0Eh when the first is below the second and
 * 0Ah otherwise, forces 0Dh when the override byte is nonzero, then formats
 * a fresh node+011Bh byte through the accepted 00BA:1037 model into
 * SS:BP-0102h and prints it through the accepted 0709:0626 model. AL on
 * return is the printer's. Each callee runs as its raw model; the far/near
 * call site opcodes and offsets are checked, and the printer is called on
 * the supplied bus (its EGA effects belong to the caller's world). Flags at
 * the printer are 1037's; the final flags are the printer's.
 * CONTRACT: the node pointer, stack and formatter destination must not alias
 * A0000..AFFFF; the body does not guard those aliases.
 * LOAD-ZERO ONLY: the model requires the resident code at CS=00BA and the
 * printer at 0709:0626; it is not relocated-load parity.
 * ACCEPTANCE WINDOW: entry SP in [0242h,FFF0h]. SP>FFF0h is outside the
 * claimed domain (C 16-bit arithmetic vs the Unicorn contiguous return-frame
 * read); 1037 uses a 100h frame and 0626 needs entry SP>=0130h. */
pr_res_0973_status pr_res_00ba_0973_cpu(pr_bus *bus, size_t ram_size, pr_res_0973_cpu *cpu,
                                        const pr_res_0973_env *env, pr_res_0973_diagnostic *diagnostic);
#endif
