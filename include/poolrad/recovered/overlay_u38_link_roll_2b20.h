#ifndef POOLRAD_RECOVERED_OVERLAY_U38_LINK_ROLL_2B20_H
#define POOLRAD_RECOVERED_OVERLAY_U38_LINK_ROLL_2B20_H

#include "poolrad/recovered/overlay_u9e_entry9_roll_latch_0e2e.h"

typedef pr_u9e_0e2e_cpu pr_u38_2b20_cpu;

/* Called at the loaded far stub :004D after the actual U38 call frame
 * is written. The callback executes the whole U9E entry-9 callee,
 * including its RETF 4, and returns at caller CS:2B57. */
typedef int (*pr_u38_2b20_roll_latch)(void *context, uint8_t *ram,
                                      size_t ram_size, pr_u38_2b20_cpu *cpu);

typedef enum {
    PR_U38_2B20_OK = 0,
    PR_U38_2B20_ARGUMENT,
    PR_U38_2B20_CALL_ABORT
} pr_u38_2b20_status;

pr_u38_2b20_status pr_u38_link_roll_2b20(
    uint8_t *ram, size_t ram_size, pr_u38_2b20_cpu *cpu,
    pr_u38_2b20_roll_latch roll_latch, void *context);

#endif
