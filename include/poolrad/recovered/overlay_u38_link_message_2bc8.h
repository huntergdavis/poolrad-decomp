#ifndef POOLRAD_RECOVERED_OVERLAY_U38_LINK_MESSAGE_2BC8_H
#define POOLRAD_RECOVERED_OVERLAY_U38_LINK_MESSAGE_2BC8_H

#include "poolrad/recovered/overlay_u38_target_select_101d.h"

typedef enum {
    PR_U38_2BC8_OK = 0,
    PR_U38_2BC8_ARGUMENT,
    PR_U38_2BC8_CALL_ABORT
} pr_u38_2bc8_status;

/* Callback enters the distinct +0FAC callee before its instructions. On
 * success, this model applies the callee's RETF 6 to the callback state. */
typedef int (*pr_u38_2bc8_dispatch)(void *context, uint8_t *ram,
                                     size_t ram_size, pr_u38_101d_cpu *cpu);

/* Reachable instructions of public entry 104, +2BC8..+2C11. The public
 * span continues with 11 bytes of non-code data up to entry 105. */
pr_u38_2bc8_status pr_u38_link_message_2bc8(
    uint8_t *ram, size_t ram_size, pr_u38_101d_cpu *cpu,
    pr_u38_2bc8_dispatch dispatch, void *context);

#endif
