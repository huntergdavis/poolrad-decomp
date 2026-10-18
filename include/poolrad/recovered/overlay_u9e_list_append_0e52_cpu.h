#ifndef POOLRAD_RECOVERED_OVERLAY_U9E_LIST_APPEND_0E52_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U9E_LIST_APPEND_0E52_CPU_H

#include <stddef.h>
#include <stdint.h>

#define PR_U9E_0E52_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u9e_0e52_cpu;

typedef enum {
    PR_U9E_0E52_OK = 0,
    PR_U9E_0E52_ARGUMENT,  /* null RAM/CPU, RAM that is not exactly 1 MiB, or IP not 0E52h; nothing changed */
    PR_U9E_0E52_CODE,      /* the GetMem LCALL at +0E5C is not 9A 3F 02; stopped at the site with the size pushed */
    PR_U9E_0E52_CALLEE,    /* GetMem stopped (RunError 203, a custom HeapError, its walk cap, a return guard);
                              *getmem_status holds its status */
    PR_U9E_0E52_TRANSFER,  /* GetMem's RETF 2 popped a CS:IP other than the call's next instruction +0E61 */
    PR_U9E_0E52_LIMIT      /* the tail walk had followed max_links links and found another; stopped at +0EA8 */
} pr_u9e_0e52_status;

/* Raw-CPU, full-state model of the complete GAME.OVR unit 0x9E entry 10,
 * +0E52..+0F05 (180 bytes, 57 instructions), entered after a far call:
 * SS:[SP] return IP, [SP+2] return CS, then the pushed words d, c, a, b
 * and the far pointer P (offset at [SP+0Ch], segment at [SP+0Eh]);
 * RETF 0Ch. It appends a 9-byte node to the list whose far head is at
 * P+7Fh/P+81h. After PUSH BP; MOV BP,SP; SUB SP,8:
 *  1. node := GetMem(9), kept at [BP-4]/[BP-2].
 *  2. If the head is nil, head := node. Otherwise a cursor at [BP-8]/[BP-6]
 *     starts at the head and follows each node's link (+5/+7) while it is
 *     not nil; the last node's link := node.
 *  3. The node's link := nil, then byte +0 := b, word +1 := a, byte +4 := d
 *     and byte +3 := c (the low bytes of the pushed words; this order).
 *  4. MOV SP,BP; POP BP; RETF 0Ch.
 * GetMem (LCALL xxxx:023F, its segment read from RAM) is the merged
 * pr_system_getmem_023f with max_heap_steps for its free-list walk; it keeps
 * its own contract (its default HeapError is xxxx:00A9; a failed allocation
 * stops at RunError with AX = 203, which this routine reports as CALLEE).
 * The tail walk is the original's unbounded loop; max_links is a host cap.
 * Every operand is reread from RAM in source order (each LES reloads its
 * pointer), so the record, the list, the node and the frame may alias. The
 * entry must be +0E52. At return AX = (a & FF00h) | (c & FFh) as last
 * loaded, DX and ES:DI hold the node as last loaded from [BP-2] and [BP-4],
 * BX/CX/SI are GetMem's, and the arithmetic flags are XOR AX,AX's
 * (CF = OF = SF = AF = 0, ZF = PF = 1)
 * over GetMem's other flags. At a GetMem stop the flags are SUB SP,8's or
 * GetMem's; at LIMIT they are OR AX,ES:[DI+7]'s. Offsets wrap at 16 bits,
 * physical addresses at 1 MiB; word operands, LES pairs, POP and the RETF
 * frame are read physically contiguous. *getmem_status (may be NULL)
 * receives GetMem's status, or -1 when it did not run. */
pr_u9e_0e52_status pr_u9e_list_append_0e52_cpu(uint8_t *ram, size_t ram_size, pr_u9e_0e52_cpu *cpu, unsigned max_heap_steps,
                                               size_t max_links, int *getmem_status);

#endif
