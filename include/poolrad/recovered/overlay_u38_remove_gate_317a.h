#ifndef POOLRAD_RECOVERED_OVERLAY_U38_REMOVE_GATE_317A_H
#define POOLRAD_RECOVERED_OVERLAY_U38_REMOVE_GATE_317A_H

#include "poolrad/recovered/overlay_u38_target_select_101d.h"

typedef pr_u38_101d_cpu pr_u38_317a_cpu;

/* Enter loaded U9E :002A with the original far-call frame. A successful
 * callback executes its RETF 10 and returns at caller CS:3197. */
typedef int (*pr_u38_317a_remove)(void *context, uint8_t *ram,
                                   size_t ram_size, pr_u38_317a_cpu *cpu);

typedef enum {
    PR_U38_317A_OK = 0,
    PR_U38_317A_ARGUMENT,
    PR_U38_317A_CALL_ABORT
} pr_u38_317a_status;

pr_u38_317a_status pr_u38_remove_gate_317a(
    uint8_t *ram, size_t ram_size, pr_u38_317a_cpu *cpu,
    pr_u38_317a_remove remove_item, void *context);

#endif
