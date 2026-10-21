#ifndef POOLRAD_RECOVERED_OVERLAY_U6F_ENTRY6_0EC4_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U6F_ENTRY6_0EC4_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/memory_bus.h"
#include "poolrad/recovered/resident_0709_msgbox_0bf5_cpu.h"
#include "poolrad/recovered/resident_0802_0716_composed_cpu.h"
#include "poolrad/recovered/resident_0899_idle_0104_cpu.h"
#include "poolrad/recovered/resident_0899_quit_0000_cpu.h"

/* Raw-CPU, full-state model of unit 0x6F [0EC4,1262) (926 bytes, 341 instructions, RETF 6): the monster-record loader
 * (docs/re/OVL_006F_0EC4_CPU.md). Entered by a far call: [SP] IP, [SP+2] CS, [SP+4]/[SP+6] the far record R (offset, segment),
 * [SP+0Ah] the block id (a word slot, read as a byte). DS:[5376] is the digit byte of the three block names.
 *
 *   0EC4 PUSH BP; MOV BP,SP; SUB SP,11Ah          locals: name [BP-114h], Str digit [BP-12h], ext [BP-119h], size [BP-2],
 *                                                  block [BP-6], R copy [BP-18h], roll [BP-14h], index [BP-13h], cursors [BP-8..-0Ch]
 *   0ECB..0F2C block 1 name := c0E98 + Str(DS:[5376]) + c0E9C, ext := c0EA0; 0802:0716 loads it             (size [BP-2])
 *   0F2C size 0: the message c0EA5 through 0709:0BF5, then 0899:0000 (QUIT; never returns)
 *   0F4E Move(11Dh, block, R); R+C8/CA, +7F/81, +104/106, +108/10A := 0; class byte R+C0 := 1 for R+2E in {1,3,5},
 *        2 for {2,4,6,7}; five rolls through the stub 00B0:0048 (count 1, sides 6) store ((r+8)<<4)+r at R+C0+i, i = 1..3,
 *        R+C4 := C4h, then i = 5..6; FreeMem(block, size); DS:[557C] := 1
 *   1080 block 2 (name c0E98 + digit + c0EBC): size 0 skips; else 9-byte units chained from R+7F/81 (GetMem 9, Move 9, link +5/7)
 *   1168 block 3 (c0EC0): size 0 skips; else 3Fh-byte units chained from R+C8/CA (link +2A/2C)
 *   1252 DS:[557C] := 0; 0899:0104 (the idle prologue); MOV SP,BP; POP BP; RETF 6
 *
 * Composition, every callee through its merged model: Str 1366, StrLoad 0B0B, StrConcat 0B98, Move 1685, GetMem 023F and
 * FreeMem 0254 (SYSTEM, segment word read live from the call), the block loader 0802:0716 (pr_r0802_0716_composed, with its
 * whole cluster and its QUIT), the message 0709:0BF5, QUIT 0899:0000 and the idle 0899:0104. The roll goes through the stub
 * 00B0:0048: its segment word is a relocation read live, and the stub entry must be a far JMP (EAh) to offset 0DE3 whose live
 * segment is the loaded unit 0x9E; the INT 3Fh overlay manager is NOT executed, the stub is the declared trampoline to the
 * strict 0DE3 model. Each callee must return to the instruction after its call (else TRANSFER); a call whose opcode or offset
 * is not the original is CODE, with IP at the call and before its return frame is pushed. Every immediate and every byte of
 * the span is original (hash-pinned); the five name constants are read by StrLoad from the code segment.
 *
 * End states: RETURNED; EXIT (Halt reached: QUIT after the message, QUIT or ReadKey inside the loader), RUNERROR (RunError at
 * 0AF8:00E2: IOCheck, GetMem, FreeMem), DIVIDE, BREAK, STALLED (050F's fixed point, the original never returns), and the
 * declared non-architectural stops LIMIT (a walk, drain or arrival cap), SERVICE (a declined DOS or keyboard service),
 * CALLEE (a model stopped otherwise) and TRANSFER, which the audit measures as zero. Exactly 1 MiB of A20-off RAM; IP 0EC4h;
 * entry SP in [09F4h,FFF4h]: the binding callee is 0802:05CD, entered from 0716 at its first open, which needs SP >= 066Ah (0716's own bound is 0300h); 066Ah plus 0716's frame and pushes (258h) plus the 132h this body has pushed when it calls 0716 (BP, the 11Ah locals, the two string pointers left by the loads, the id and two output pointers, the return frame) is 09F4h, and a scan at every SP from 0432h to 1200h over ten scenarios found nested refusals up to exactly 09F3h and none from 09F4h; the three argument words stay below the top).
 * Every access is physical flat RAM: the record, the blocks, the heap and the string windows exclude A0000..AFFFF, the one
 * aperture being the EGA that 0BF5 reaches through the bus. Flags: every flag writer is executed; the shift and the adds
 * whose flags are overwritten before any call are folded. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u6f_0ec4_cpu;

typedef struct {
    pr_r0802_0716c_env load;     /* the block loader's whole environment (file service, keyboard, driver, QUIT, caps) */
    pr_r0709_0bf5_env msg;       /* the message's environment; msg.key.quit also serves this body's QUIT */
    unsigned heap_steps;         /* the GetMem / FreeMem free-list walk cap */
    unsigned roll_calls;         /* the 0DE3 model's cap on SYSTEM call slices */
} pr_u6f_0ec4_env;

typedef enum {
    PR_U6F_0EC4_RETURNED = 0,    /* RETF 6 executed */
    PR_U6F_0EC4_ARGUMENT,        /* null pointers, RAM size, IP not 0EC4h, entry SP out of range; nothing changed */
    PR_U6F_0EC4_CODE,            /* a call's opcode, offset or the roll stub is not the original; IP at the call */
    PR_U6F_0EC4_EXIT,            /* Halt reached */
    PR_U6F_0EC4_RUNERROR,        /* RunError entered at 0AF8:00E2 */
    PR_U6F_0EC4_DIVIDE,          /* BlockRead's DIV by recsize 0 */
    PR_U6F_0EC4_BREAK,           /* ReadKey's Ctrl-Break path */
    PR_U6F_0EC4_STALLED,         /* 050F's proven fixed point: the original never returns (a HANG) */
    PR_U6F_0EC4_LIMIT,           /* a declared cap */
    PR_U6F_0EC4_SERVICE,         /* a declared service declined */
    PR_U6F_0EC4_CALLEE,          /* a model stopped otherwise */
    PR_U6F_0EC4_TRANSFER         /* a callee returned somewhere other than the instruction after its call */
} pr_u6f_0ec4_status;

typedef enum {
    PR_U6F_0EC4_NONE = -1,
    PR_U6F_0EC4_STR = 0, PR_U6F_0EC4_STRLOAD, PR_U6F_0EC4_STRCAT, PR_U6F_0EC4_MOVE, PR_U6F_0EC4_GETMEM, PR_U6F_0EC4_FREEMEM,
    PR_U6F_0EC4_LOAD, PR_U6F_0EC4_MSG, PR_U6F_0EC4_QUIT, PR_U6F_0EC4_IDLE, PR_U6F_0EC4_ROLL
} pr_u6f_0ec4_callee;

typedef struct {
    uint16_t site;               /* IP of the call this stop concerns (0 when none) */
    int callee;                  /* pr_u6f_0ec4_callee of that call (-1 when none) */
    int callee_status;           /* that model's own status (-1 when none) */
    unsigned calls;              /* far calls run to their return */
    unsigned rolls;              /* roll calls run to their return */
    pr_r0802_0716c_diagnostic load;
    pr_r0709_0bf5_diagnostic msg;
    pr_r0899_quit_diagnostic quit;
} pr_u6f_0ec4_diagnostic;

pr_u6f_0ec4_status pr_u6f_entry6_0ec4_cpu(pr_bus *bus, size_t ram_size, pr_u6f_0ec4_cpu *cpu, const pr_u6f_0ec4_env *env,
                                          pr_u6f_0ec4_diagnostic *diagnostic);
#endif
