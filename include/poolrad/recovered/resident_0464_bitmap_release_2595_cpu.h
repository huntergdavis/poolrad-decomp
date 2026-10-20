#ifndef POOLRAD_RECOVERED_RESIDENT_0464_BITMAP_RELEASE_2595_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_0464_BITMAP_RELEASE_2595_CPU_H
#include "poolrad/recovered/system_freemem_0254_cpu.h"
typedef pr_system_freemem_cpu pr_r0464_2595_cpu;
typedef enum {
    PR_R0464_2595_RETURNED=0, PR_R0464_2595_ARGUMENT,
    PR_R0464_2595_CALLEE, PR_R0464_2595_TRANSFER
} pr_r0464_2595_status;
typedef struct {
    int freemem_status; /* -1 until called; otherwise exact SYSTEM status */
    uint16_t call_site;
    unsigned calls;
} pr_r0464_2595_diagnostic;
/* Original [2595,2632): far Pascal pointer-to-pointer argument, RETF4.
 * Full caller composed with raw SYSTEM FreeMem, no host release callback.
 * Exact1MiB A20-off ordinary RAM; IP2595, SP[0040,FFF8]. Immutable code and
 * all three far-call bindings must target system_cs:0254; checked atomically.
 * Host CPU/diagnostic objects are outside RAM and mutually distinct. Live
 * memory aliases are preserved; all reached runtime frames must remain
 * nonwrapping and within the callee's documented entry range. Interrupts,
 * faults and code writes excluded. max_heap_steps resets per FreeMem call.
 * No caller loops. All non-argument stops preserve exact partial CPU/RAM;
 * successful unexpected callee returns produce TRANSFER. ARGUMENT changes
 * neither RAM, CPU nor diagnostic. A null diagnostic is supported.
 * FLAGS follow DOSBox-X normal/386 (logical AF0, SHR AF1); no output masks.
 */
pr_r0464_2595_status pr_r0464_bitmap_release_2595_cpu(uint8_t *ram,
    size_t ram_size,pr_r0464_2595_cpu *cpu,uint16_t system_cs,
    unsigned max_heap_steps,pr_r0464_2595_diagnostic *diagnostic);
#endif
