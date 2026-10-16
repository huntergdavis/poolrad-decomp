#ifndef POOLRAD_OVERLAY_U86_ENTRY1_POSTDC0_JZ_01ED_H
#define POOLRAD_OVERLAY_U86_ENTRY1_POSTDC0_JZ_01ED_H

#include "resident_1816_caller_routes.h"

/* Execute only U86 entry-1 +01ED JZ after the supplied 0AF8:0DC0 return.
 * Require the supplied loaded U86 CS and exact 74 AD opcode bytes. Stop
 * before +019C or +01EF; zero means success, one means no mutation. */
int pr_u86_entry1_01ed_jz(pr_1816_caller_cpu *cpu, const uint8_t *ram,
                          size_t ram_size, uint16_t supplied_u86_cs);

#endif
