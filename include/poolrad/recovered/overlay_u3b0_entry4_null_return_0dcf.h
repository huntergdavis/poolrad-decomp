#ifndef POOLRAD_RECOVERED_OVERLAY_U3B0_ENTRY4_NULL_RETURN_0DCF_H
#define POOLRAD_RECOVERED_OVERLAY_U3B0_ENTRY4_NULL_RETURN_0DCF_H

#include "poolrad/recovered/overlay_u3b0_entry4_node_gate_0dc7.h"

typedef enum {
    PR_U3B0_0DCF_OK = 0,
    PR_U3B0_0DCF_ARGUMENT
} pr_u3b0_0dcf_status;

/* Execute the null-head +0DCF branch through the shared +10A1 RETF 0x28. */
pr_u3b0_0dcf_status pr_u3b0_entry4_null_return_0dcf(
    uint8_t *ram, size_t ram_size, pr_u3b0_0dc7_cpu *cpu);

#endif
