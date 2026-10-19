#ifndef POOLRAD_RECOVERED_OVERLAY_U3B0_ENTRY6_1140_H
#define POOLRAD_RECOVERED_OVERLAY_U3B0_ENTRY6_1140_H

#include <stddef.h>
#include <stdint.h>

#define PR_U3B0_1140_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u3b0_1140_cpu;

/* Called with IP at +114A or +11A8 and 0x002E at SS:SP. The callback
 * supplies all allocator RAM/CPU effects, including returned DX:AX.
 * This wrapper performs the pinned RETF 2 cleanup after success. */
typedef int (*pr_u3b0_1140_alloc)(void *context, uint8_t *ram,
                                  size_t ram_size, pr_u3b0_1140_cpu *cpu);

typedef enum {
    PR_U3B0_1140_OK = 0,
    PR_U3B0_1140_ARGUMENT,
    PR_U3B0_1140_ALLOC_ABORT
} pr_u3b0_1140_status;

pr_u3b0_1140_status pr_u3b0_entry6_1140(
    uint8_t *ram, size_t ram_size, pr_u3b0_1140_cpu *cpu,
    pr_u3b0_1140_alloc alloc, void *context);

typedef enum {
    PR_U3B0_1140_CPU_OK = 0,
    PR_U3B0_1140_CPU_ARGUMENT,
    PR_U3B0_1140_CPU_CODE,
    PR_U3B0_1140_CPU_CALLEE,
    PR_U3B0_1140_CPU_TRANSFER,
    PR_U3B0_1140_CPU_LIMIT
} pr_u3b0_1140_cpu_status;

/* Raw companion to the callback entry above. Enter at CS:1140 after a far
 * call: SS:[SP] return IP, +2 return CS, +4 output offset, +6 output segment,
 * +8 signed count. RAM is exactly 1 MiB, A20 off, ordinary memory; code is
 * immutable except for deliberately rejected call guards. Nonwrapping active
 * stack frames require entry SP in [0040,FFF6].
 *
 * Both LCALLs must have opcode 9A and offset 023F; their live segment is used.
 * The original far frame is pushed and pr_system_getmem_023f executes the
 * allocator. A non-returning allocator retains every CPU/RAM effect and
 * returns CALLEE; *getmem_status is its status (including HeapError, RunError
 * and free-list LIMIT). Unexpected far return CS:IP produces TRANSFER.
 * max_heap_steps is applied separately to each GetMem invocation.
 * max_allocations caps executed GetMem calls. At exhaustion LIMIT stops at
 * the next LCALL, size already pushed, far frame not pushed. Guard checking
 * precedes the allocation budget. *getmem_status starts at -1 and otherwise
 * describes the last invoked allocator, including a previous successful call.
 * A NULL diagnostic is allowed. ARGUMENT changes neither CPU nor RAM.
 * The final RETF 6 uses the live caller return frame, including output aliases.
 */
pr_u3b0_1140_cpu_status pr_u3b0_entry6_1140_cpu(
    uint8_t *ram, size_t ram_size, pr_u3b0_1140_cpu *cpu,
    unsigned max_allocations, unsigned max_heap_steps, int *getmem_status);

#endif
