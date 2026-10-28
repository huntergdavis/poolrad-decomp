#ifndef POOLRAD_RECOVERED_OVERLAY_U6C_2033_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U6C_2033_CPU_H

#include <stddef.h>
#include <stdint.h>

#include "poolrad/recovered/memory_bus.h"

#define PR_U6C_2033_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u6c_2033_cpu;

typedef enum {
    PR_U6C_2033_RETURNED = 0,   /* the body returned through RETF 4 */
    PR_U6C_2033_ARGUMENT,       /* bad pointers, RAM size or entry IP */
    PR_U6C_2033_CODE,           /* a checked opcode or call operand differs */
    PR_U6C_2033_BOUNDARY,       /* stopped at the declared 0085:002F call site */
    PR_U6C_2033_CALLEE,         /* composed 00BA:0973 model failed */
    PR_U6C_2033_TRANSFER        /* composed callee returned somewhere unexpected */
} pr_u6c_2033_status;

typedef struct {
    size_t printer_loopbacks;
    size_t printer_glyph_loopbacks;
} pr_u6c_2033_env;

typedef struct {
    uint16_t site;
    int callee;                 /* -1 none, 0 declared boundary 0085:002F, 1 composed 00BA:0973 */
    int callee_status;
} pr_u6c_2033_diagnostic;

/* Raw-CPU model of GAME.OVR unit 0x06C [2033,206E), 59 bytes / 23 instructions,
 * RETF 4 at 206B. Loaded unit segment 0078h, the unit maps at offset 0, so the
 * body runs at 0078:2033. The far call at 2033-59 is Pascal: [bp+6] is a word
 * offset in SS and [bp+8] the second byte parameter; the frame holds two argument
 * words and RETF 4 consumes them.
 *
 * Reads the byte at SS:([bp+6] - 00Dh). Below 6 it loads that byte and [bp+8] as
 * words and reaches the far call at 204Ch (fixup 0085:002F, unit 0x77 entry 3);
 * that callee is OUT OF SCOPE and declared a boundary: the model pushes exactly
 * what the original pushed and returns PR_U6C_2033_BOUNDARY with CS:IP at the
 * call site, flags from the CMP at 2039. Six or above pushes the far pointer
 * DS:[5D94]:[5D92], x 4, y 012h and [bp+8], then composes the accepted resident
 * 00BA:0973 model (formatter 1037 and printer 0626 internally); AL, the final
 * flags and the return frame are that model's, then the body pops BP and RETFs 4.
 * The body writes no game memory and performs no I/O itself.
 * CONTRACT: LOAD-ZERO ONLY. The checked operands are the load-zero fixups
 * (0085:002F and 00BA:0973); a relocated load is not modelled. The node offset
 * must not alias the caller's stack frame, and the composed 0973 contract still
 * applies (node/stack not aliasing A0000..AFFFF, its entry-SP window).
 * OWNERSHIP: unresolved; this model assigns no authorship. */
pr_u6c_2033_status pr_ovl_006c_2033_cpu(pr_bus *bus, size_t ram_size, pr_u6c_2033_cpu *cpu,
                                        const pr_u6c_2033_env *env, pr_u6c_2033_diagnostic *diagnostic);

#endif
