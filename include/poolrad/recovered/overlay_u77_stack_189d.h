#ifndef POOLRAD_RECOVERED_OVERLAY_U77_STACK_189D_H
#define POOLRAD_RECOVERED_OVERLAY_U77_STACK_189D_H

#include <stddef.h>
#include <stdint.h>

#define PR_U77_189D_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u77_189d_cpu;

/* Called at the source's BA:1509 far call after its four argument words
 * have been pushed. The adapter owns all resident sidecar-removal effects.
 * Return 1 to continue. The model performs RETF 8 stack cleanup. */
typedef int (*pr_u77_189d_remove)(void *context, uint8_t *ram,
                                   size_t ram_size, pr_u77_189d_cpu *cpu,
                                   uint16_t node_offset,
                                   uint16_t node_segment,
                                   uint16_t character_offset,
                                   uint16_t character_segment);

typedef enum {
    PR_U77_189D_OK = 0,
    PR_U77_189D_ARGUMENT,
    PR_U77_189D_LIMIT,
    PR_U77_189D_REMOVE_ABORT
} pr_u77_189d_status;

/* Complete in-body state through RETF 4, with BA:1509 supplied by callback.
 * `max_nodes` bounds a corrupt cyclic chain on the host; DOS has no bound. */
pr_u77_189d_status pr_u77_stack_189d(
    uint8_t *ram, size_t ram_size, pr_u77_189d_cpu *cpu,
    pr_u77_189d_remove remove_node, void *context, size_t max_nodes);

#endif
