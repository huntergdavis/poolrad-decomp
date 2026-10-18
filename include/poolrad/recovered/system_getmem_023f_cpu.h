#ifndef POOLRAD_RECOVERED_SYSTEM_GETMEM_023F_CPU_H
#define POOLRAD_RECOVERED_SYSTEM_GETMEM_023F_CPU_H

#include <stddef.h>
#include <stdint.h>

#define PR_SYSTEM_GETMEM_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_system_getmem_cpu;

typedef enum {
    PR_SYSTEM_GETMEM_RETURNED = 0,  /* RETF 2 executed: DX:AX is the block, or nil for a zero size */
    PR_SYSTEM_GETMEM_ARGUMENT,      /* null RAM/CPU, RAM that is not exactly 1 MiB, or IP not 023Fh; nothing changed */
    PR_SYSTEM_GETMEM_RUNERROR,      /* allocation failed: AX = 203 (CBh) at CS:00E2, RunError's entry, with the caller's
                                       far return frame on top of the stack, as RunError pops it */
    PR_SYSTEM_GETMEM_HEAP_ERROR,    /* the HeapError pointer DS:4414h is not this SYSTEM's default CS:00A9; stopped at the
                                       indirect LCALL (+03B7 or +03C3) with its argument pushed, the call not executed */
    PR_SYSTEM_GETMEM_TRANSFER,      /* a near RET or the default HeapError's RETF 2 popped another return address */
    PR_SYSTEM_GETMEM_LIMIT          /* host cap on the free-list walk's taken JA +0319; IP at +0319 */
} pr_system_getmem_status;

/* Raw-CPU, full-state model of Turbo Pascal 6 GetMem in the START.EXE SYSTEM
 * segment (0AF8 at load segment 0): the entry 023F..0253 (PUSH BP; MOV
 * BP,SP; MOV AX,[BP+6]; CALL 0308; POP BP; JB 024E; RETF 2; 024E MOV
 * AX,0CBh; JMP 00E2), its near core 0308..03D2, the size rounding
 * 0491..04A1 and the default HeapError 00A9 (XOR AX,AX; RETF 2). Entered
 * after a far call: SS:[SP] return IP, [SP+2] return CS, [SP+4] the size.
 * The heap variables are DS words: HeapPtr 4404h/4406h, HeapEnd 4408h/440Ah,
 * FreeList 440Ch/440Eh, HeapError 4414h/4416h and the saved size 8754h.
 *  1. A zero size returns nil (XOR AX,AX; CWD).
 *  2. The size is saved and rounded up to whole 8-byte granules: DX := its
 *     paragraphs ((size + 7) >> 4, with the 17th bit through RCR) and AX :=
 *     (size + 7) & 8.
 *  3. From the link FreeList, each normalized free block (next pointer at
 *     +0, size paragraphs at +6 and granule at +4) is visited until a block
 *     whose segment equals HeapPtr's (the end of the list). The first block
 *     at least as large is taken: on an exact fit its link is unlinked;
 *     otherwise the remainder, normalized, becomes a free block in its place.
 *     DX:AX := the block.
 *  4. At the list's end the block is HeapPtr itself, when HeapPtr + size
 *     (normalized) does not pass HeapEnd: HeapPtr and the last link become
 *     the new top and HeapError is called with 0.
 *  5. Otherwise HeapError is called with the size; its AL = 0 returns with
 *     CF set, and GetMem's JB jumps to RunError with AX = 203.
 * The default HeapError (DS:4414h = 00A9h, DS:4416h = this CS) is folded:
 * it returns AL = 0, so the failure path always ends in RunError, and the
 * AL = 1 (nil) and AL >= 2 (retry) branches need another HeapError, at
 * whose call the model stops. All near calls and returns use the live
 * stack; every operand is reread in source order, so the heap may alias
 * the stack and the variables. A cyclic free list walks forever, so
 * max_steps caps the walk's taken JA +0319 and the next one stops there
 * with LIMIT. Flags: on return they are those of the path's last writer
 * (XOR AX,AX for nil, the equal CMP for an exact fit, AND AL,0Fh for a
 * split, the default HeapError's XOR on the heap top) with CF clear; at
 * RunError those of CMP AL,1 with AL = 0; at a stop those the original
 * leaves there. OR, XOR and AND leave AF clear. Offsets wrap at 16 bits,
 * physical addresses at 1 MiB; LES pairs, POP and every RET/RETF frame are
 * read physically contiguous. The code must sit at its original offsets,
 * hence IP = 023Fh. */
pr_system_getmem_status pr_system_getmem_023f(uint8_t *ram, size_t ram_size, pr_system_getmem_cpu *cpu, unsigned max_steps);

#endif
