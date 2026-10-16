#ifndef POOLRAD_OVERLAY_U86_ENTRY1_POSTDC0_LEA_DI_019C_H
#define POOLRAD_OVERLAY_U86_ENTRY1_POSTDC0_LEA_DI_019C_H

#include "resident_1816_caller_routes.h"

/* Execute only U86 entry-1 +019C LEA DI,[BP-32h] on supplied state.
 * Require the supplied loaded CS and exact 8D 7E CE bytes. Stop before
 * +019F PUSH SS; zero means success, one means no mutation. */
int pr_u86_entry1_019c_lea_di(pr_1816_caller_cpu *cpu, const uint8_t *ram,
                              size_t ram_size, uint16_t supplied_u86_cs);

#endif
