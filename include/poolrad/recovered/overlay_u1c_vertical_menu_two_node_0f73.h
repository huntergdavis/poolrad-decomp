#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_VERTICAL_MENU_TWO_NODE_0F73_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_VERTICAL_MENU_TWO_NODE_0F73_H

#include "poolrad/recovered/overlay_u1c_vertical_menu_null_head_0f73.h"

/* Exactly two non-null nodes followed by a zero link, then menu/RETF. */
pr_u1c_0f73_status pr_u1c_vertical_menu_two_node_0f73(
    uint8_t *ram, size_t ram_size, pr_u1c_0f08_cpu *cpu,
    pr_u1c_0f73_external external, void *context);

#endif
