#ifndef POOLRAD_RECOVERED_OVERLAY_U3C6_SLOT_GETTERS_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U3C6_SLOT_GETTERS_CPU_H
#include <stddef.h>
#include <stdint.h>
#define PR_U3C6_GETTER_RAM_SIZE (1u<<20)
typedef struct { uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags; } pr_u3c6_getter_cpu;
typedef enum { PR_U3C6_GETTER_OK=0,PR_U3C6_GETTER_ARGUMENT,
    PR_U3C6_GETTER_LIMIT,PR_U3C6_GETTER_TRANSFER } pr_u3c6_getter_status;
/* Raw U3C6 entries12/13/14/15, IP0BE7/0C0F/0C37/0C5F, all RETF4.
 * After far CALL, SS:[SP+4]/[SP+6] is a record offset/segment.
 * +0C5F searches DS:65B9+4*i by exact far-pointer identity, i=1..count at
 * DS:5F2A; zero count includes wrap through i=0. AL=matched index or zero.
 * The other entries call +0C5F, then return DS byte5F27/5F28/5F2A+4*index.
 * AH=0 for getters. Every operand, local and return frame is live.
 * Exact1MiB A20-off RAM; SP[0020,FFF8]; CPU must be distinct from RAM.
 * Original code/bindings immutable; data aliases permitted, code aliases
 * excluded. Reached return frames nonwrapping; word operands contiguous.
 * No IRQs/faults. Normal/386 undefined flags: logical AF0, SHL-by-one AF1.
 * max_slot_visits counts visits to +0C71 before INC, zero allowed; LIMIT
 * preserves exact partial state before the exhausted visit.
 * TRANSFER preserves a changed inner +0C5F return. Outer returns use live
 * CS:IP, including aliases. ARGUMENT changes neither CPU nor RAM.
 * Existing scalar APIs and strict credits are unchanged.
 */
pr_u3c6_getter_status pr_u3c6_slot_getter_cpu(uint8_t *ram,size_t size,
    pr_u3c6_getter_cpu *cpu,size_t max_slot_visits);
#endif
