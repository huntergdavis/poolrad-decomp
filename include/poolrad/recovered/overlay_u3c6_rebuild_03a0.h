#ifndef POOLRAD_RECOVERED_OVERLAY_U3C6_REBUILD_03A0_H
#define POOLRAD_RECOVERED_OVERLAY_U3C6_REBUILD_03A0_H
#include <stddef.h>
#include <stdint.h>
#define PR_U3C6_03A0_RAM_SIZE (1u<<20)
typedef struct { uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags; } pr_u3c6_03a0_cpu;
typedef enum {
    PR_U3C6_03A0_OK=0,PR_U3C6_03A0_ARGUMENT,PR_U3C6_03A0_SLOT_LIMIT,
    PR_U3C6_03A0_CELL_LIMIT,PR_U3C6_03A0_TRANSFER,PR_U3C6_03A0_CALLEE
} pr_u3c6_03a0_status;
/* Whole entry5 [03A0,04BE), no arguments, RETF. Exact 1MiB A20-off RAM,
 * separate CPU object, immutable code/bindings; all writes must avoid code.
 * Initial IP03A0, SP[001C,FFFC]; nonwrapping far-return frames. system_cs
 * binds FillChar +16A8. Same-CS table-pair +0000 uses WRAPPED_SS policy.
 * Clears DS:[60DB,65BD), then uses live slot fields and shape-table bytes
 * to write occupancy and relative-coordinate arrays. No coordinate clipping.
 * All data/stack aliases remain live. Words are physically contiguous;
 * effective offsets and physical addresses wrap at 16 and 20 bits.
 * Positive shared budgets count visits to slot +03CF and cell +03ED.
 * Stops retain exact partial CPU/RAM; unexpected direct returns produce
 * TRANSFER and dependency rejection produces CALLEE. A modified final
 * caller return is preserved as a completed return. See evidence document
 * docs/re/U3C6_REBUILD_03A0.md for scope and limitations. */
pr_u3c6_03a0_status pr_u3c6_rebuild_03a0(uint8_t *ram,size_t size,
    pr_u3c6_03a0_cpu *cpu,uint16_t system_cs,size_t max_slots,size_t max_cells);
#endif
