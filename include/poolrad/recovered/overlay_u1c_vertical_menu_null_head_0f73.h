#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_VERTICAL_MENU_NULL_HEAD_0F73_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_VERTICAL_MENU_NULL_HEAD_0F73_H

#include "poolrad/recovered/overlay_u1c_vertical_menu_one_node_0f73.h"

/* Exact shared tail after the linked-list loop; entry must be +0FFB. */
pr_u1c_0f73_status pr_u1c_vertical_menu_after_list_0ffb(
    uint8_t *ram, size_t ram_size, pr_u1c_0f08_cpu *cpu,
    pr_u1c_0f73_external external, void *context);

/* Starts at +0F73 and follows only the zero-head branch through RETF. */
pr_u1c_0f73_status pr_u1c_vertical_menu_null_head_0f73(
    uint8_t *ram, size_t ram_size, pr_u1c_0f08_cpu *cpu,
    pr_u1c_0f73_external external, void *context);

#endif
