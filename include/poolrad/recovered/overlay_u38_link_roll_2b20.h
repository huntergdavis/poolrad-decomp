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


/* Complete U38 [2B20,2B63), RETF0Ah; raw U9E latch/dice/SYSTEM chain.
 * A nonnull link with byte+2E==55h rolls 1d6, latches the count at DS:681C,
 * then stores zero-extended AL+1 to DS:6818. Other paths preserve the amount.
 * Same raw profile as pr_u9e_roll_latch_0e2e_cpu, but IP2B20 and
 * SP0040..FFFC. Ten unused argument bytes. The loader-fixed FAR CALL
 * must name an EA 0E2E stub at :004D; guard it before pushing the far frame.
 * No callback or state rollback. latch_status records the nested raw latch
 * result; dice_status records its dependency stop. Both start at -1.
 * max_system_calls is passed unchanged to the dice dependency.
 * Existing callback API remains independent. See the linked evidence. */
pr_roll_chain_status pr_u38_link_roll_2b20_cpu(uint8_t *ram,size_t size,
    pr_roll_chain_cpu *cpu,unsigned max_system_calls,
    pr_roll_chain_diagnostic *diagnostic);

#endif
