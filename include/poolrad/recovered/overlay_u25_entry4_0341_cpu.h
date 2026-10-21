#ifndef POOLRAD_RECOVERED_OVERLAY_U25_ENTRY4_0341_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U25_ENTRY4_0341_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/memory_bus.h"
#include "poolrad/recovered/overlay_u3b0_row_clear_06fb_cpu.h"
#include "poolrad/recovered/resident_0709_printer_0626_cpu.h"
#include "poolrad/recovered/resident_0802_0716_composed_cpu.h"

/* Raw-CPU, full-state model of unit 0x25 [0341,0411) (208 bytes, 89 instructions, RETF 2), the loader of a 1E00h-byte screen block
 * (docs/re/OVL_0025_0341_CPU.md). Entered by a far call: [SP] IP, [SP+2] CS, [SP+4] the block id (a word slot, read as a byte). RETF 2.
 *
 *   0341 PUSH BP; MOV BP,SP; SUB SP,112h
 *   0348 FillChar(DS:[49DE]^, 1E00h, 0)                                        LCALL 0AF8:16A8     (the far pointer at DS:49DE is a global buffer)
 *   035A DS:[557C] := 0
 *   035F row clear  LCALL 03C5:002F    unit 0x3B0 stub entry 3 = +06FB          <- the retry loop head
 *   0364 the message c0322 into [BP-22h] (StrLoad), printed by 0709:0626 (x 0, y 18h, colour 0Fh)
 *   0381 name := c0338 + Str(DS:[5376], width 0, max length 1), extension := c033C; 0802:0716(id, &size [BP-8], &block [BP-0Ch])
 *   03D8 CMP word [BP-8],2; JB 035F            size below 2: retry from the row clear
 *   03DE Move(size-2, DS:[49DE]^, block+2); FreeMem(block, size); row clear again
 *   040B MOV SP,BP; POP BP; RETF 2
 *
 * Composition: every callee through its merged model (SYSTEM FillChar, StrLoad, Str, StrConcat, Move and FreeMem; the block loader pr_r0802_0716_composed;
 * the printer 0709:0626 and the row clear +06FB, both drawing through the bus). The two row-clear calls go through the stub 03C5:002F exactly as 0411/0EC4 go
 * through theirs: the segment words at CS:0362 and CS:0409 are relocations read live; the stub entry there must be a far JMP (EAh) to offset 06FBh whose live segment is the
 * loaded unit 0x3B0; the INT 3Fh overlay-manager dispatch is NOT executed. A call whose opcode or offset is not the original, or a stub that is not that JMP,
 * is CODE with IP at the call and before its return frame. Each callee must return to the instruction after its call (else TRANSFER).
 *
 * The retry loop is unbounded in the original: a block of size 0 or 1 (an absent id in a present file) loops forever with no key wait. `retries` is a fresh host
 * budget per entry counting taken JB 035F loop-backs; when a loop-back would run past it the model stops LIMIT at IP 03DCh with the JB not executed and CPU, RAM
 * and EGA as they stand. Whether that is a hang is a property of the original, shown in the audit by a fixed point (the same CPU and RAM at consecutive arrivals).
 *
 * End states: RETURNED; every 0716 end state propagates (EXIT, RUNERROR, DIVIDE, BREAK, STALLED, LIMIT, SERVICE, CALLEE); GetMem is not called here, FreeMem's RUNERROR
 * and the printer's and row clear's stops map as CALLEE/LIMIT/TRANSFER. ARGUMENT before any change: IP 0341h, entry SP in [09ECh,FFFAh] (0716's entry bound 08C2h
 * plus the 12Ah this body has pushed when it calls it; the argument word stays below the top). Exactly 1 MiB of A20-off RAM, an attached EGA. The body reads one
 * pointer it did not create, the far pointer at DS:[49DE] (flat, in the data segment); it never dereferences it itself, it hands it to FillChar and Move, whose
 * contracts are flat RAM: the buffer excludes A0000..AFFFF. The block pointer is created by 0716. */
typedef pr_r0709_printer_cpu pr_u25_0341_cpu;

typedef struct {
    pr_r0802_0716c_env load;     /* the block loader's whole environment */
    size_t space_loopbacks;      /* the row clear's glyph budget (05EB's max_loopbacks) */
    size_t printer_loopbacks;    /* the printer's outer budget */
    size_t glyph_loopbacks;      /* the printer's fresh glyph budget per call */
    unsigned heap_steps;         /* the FreeMem free-list walk cap */
    unsigned retries;            /* taken JB 035F loop-backs allowed, fresh on each entry */
} pr_u25_0341_env;

typedef enum {
    PR_U25_0341_RETURNED = 0, PR_U25_0341_ARGUMENT, PR_U25_0341_CODE, PR_U25_0341_EXIT, PR_U25_0341_RUNERROR,
    PR_U25_0341_DIVIDE, PR_U25_0341_BREAK, PR_U25_0341_STALLED, PR_U25_0341_LIMIT, PR_U25_0341_SERVICE,
    PR_U25_0341_CALLEE, PR_U25_0341_TRANSFER
} pr_u25_0341_status;

typedef enum {
    PR_U25_0341_NONE = -1, PR_U25_0341_FILL = 0, PR_U25_0341_ROWCLEAR, PR_U25_0341_STRLOAD, PR_U25_0341_PRINT, PR_U25_0341_STR,
    PR_U25_0341_STRCAT, PR_U25_0341_LOAD, PR_U25_0341_MOVE, PR_U25_0341_FREEMEM
} pr_u25_0341_callee;

typedef struct {
    uint16_t site;                       /* IP of the call this stop concerns (0 when none; 03DCh for the retry LIMIT) */
    int callee;                          /* pr_u25_0341_callee (-1 when none) */
    int callee_status;                   /* that model's own status (-1 when none) */
    unsigned calls;                      /* far calls run to their return */
    unsigned retries_taken;              /* JB 035F loop-backs taken */
    pr_r0802_0716c_diagnostic load;
    pr_u3b0_06fb_diagnostic rowclear;
    pr_r0709_printer_cpu_diagnostic printer;
} pr_u25_0341_diagnostic;

pr_u25_0341_status pr_u25_entry4_0341_cpu(pr_bus *bus, size_t ram_size, pr_u25_0341_cpu *cpu, const pr_u25_0341_env *env,
                                          pr_u25_0341_diagnostic *diagnostic);
#endif
