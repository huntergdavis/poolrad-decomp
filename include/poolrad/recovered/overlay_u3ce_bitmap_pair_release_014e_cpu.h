#ifndef POOLRAD_RECOVERED_OVERLAY_U3CE_BITMAP_PAIR_RELEASE_014E_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U3CE_BITMAP_PAIR_RELEASE_014E_CPU_H
#include "poolrad/recovered/resident_0464_bitmap_release_2595_cpu.h"
typedef pr_r0464_2595_cpu pr_u3ce_014e_cpu;
typedef enum { PR_U3CE_014E_RETURNED=0, PR_U3CE_014E_ARGUMENT,
    PR_U3CE_014E_CALLEE, PR_U3CE_014E_TRANSFER } pr_u3ce_014e_status;
typedef struct {
    int resident_status; /* -1 until first invocation */
    pr_r0464_2595_diagnostic resident; /* latest invocation */
    unsigned calls;
    uint16_t call_site;
} pr_u3ce_014e_diagnostic;
/* GAME.OVR U3CE [014E,01B0): far Pascal byte index, RETF2.
 * CPU14 and exactly 1MiB A20-off ordinary RAM, entry IP014E, SP[0080,FFFA].
 * Immutable code; valid nonwrapping reached runtime frames; no interrupts
 * or faults. Both caller bindings are checked atomically before mutation.
 * The resident's SYSTEM bindings must satisfy its published contract.
 * CPU/diagnostic are distinct host objects outside RAM. All RAM aliases
 * remain live, including the argument reload after the first release.
 * At most two resident calls; max_heap_steps resets per FreeMem invocation.
 * No callbacks or fabricated allocator results. All nonargument stops retain
 * exact partial CPU/RAM; unexpected callee return => TRANSFER. ARGUMENT
 * changes no object. Null diagnostic allowed. Accepted DOSBox-X normal/386
 * flag convention; no output masks.
 */
pr_u3ce_014e_status pr_u3ce_bitmap_pair_release_014e_cpu(uint8_t *ram,
    size_t ram_size,pr_u3ce_014e_cpu *cpu,uint16_t resident_cs,
    uint16_t system_cs,unsigned max_heap_steps,pr_u3ce_014e_diagnostic *diagnostic);
#endif
