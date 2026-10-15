#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_VERTICAL_MENU_ONE_NODE_0F73_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_VERTICAL_MENU_ONE_NODE_0F73_H

#include "poolrad/recovered/overlay_u1c_vertical_menu_prefix_0f08.h"

typedef enum {
    PR_U1C_0F73_NODE = 0,    /* 3C5:003E, RETF 6 */
    PR_U1C_0F73_DRAW,        /* 709:0768, RETF 16 */
    PR_U1C_0F73_COPY,        /* AF8:0B25, RETF 10 */
    PR_U1C_0F73_MENU,        /* 2B:0066, RETF 40 */
    PR_U1C_0F73_STORE,       /* 2B:0052, RETF 4 */
    PR_U1C_0F73_RELEASE,     /* 3C5:0043, RETF 4 */
    PR_U1C_0F73_CLEANUP      /* 709:0DD3, RETF 8 */
} pr_u1c_0f73_call;

/* Entry CPU/RAM must be at +0F73 after the merged +0F08 prefix. Callback
 * supplies every external call's RAM/CPU effects; IP names the call site. */
typedef int (*pr_u1c_0f73_external)(void *context, uint8_t *ram,
                                    size_t ram_size, pr_u1c_0f08_cpu *cpu,
                                    pr_u1c_0f73_call call);

typedef enum {
    PR_U1C_0F73_OK = 0,
    PR_U1C_0F73_ARGUMENT,
    PR_U1C_0F73_CALL_ABORT,
    PR_U1C_0F73_BRANCH_STOP /* null head or nonnull next link */
} pr_u1c_0f73_status;

pr_u1c_0f73_status pr_u1c_vertical_menu_one_node_0f73(
    uint8_t *ram, size_t ram_size, pr_u1c_0f08_cpu *cpu,
    pr_u1c_0f73_external external, void *context);

#endif
