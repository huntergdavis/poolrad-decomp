#ifndef POOLRAD_RECOVERED_U1C_SAVE_TABLE_0EBC_H
#define POOLRAD_RECOVERED_U1C_SAVE_TABLE_0EBC_H

#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/candidate_ecl_cpu.h"

/* Candidate-scout draft, no credit claimed.
 * GAME.OVR unit 1Ch +[0EBC,0F08): ECL opcode 0x35 SAVE TABLE. Far Pascal, no arguments, RETF. Loads 3 operands; dest = operand_word(slot 2) + slot_value(slot 3), value = slot_value(slot 1); then a word store.
 * Raw-CPU C: one case per original instruction, operands reread from flat 1 MiB RAM.
 * Unit-0x25 runtime calls are shared candidate doubles, not the real helpers. */
typedef enum {
    PR_U1C_SAVE_TABLE_0EBC_OK = 0,
    PR_U1C_SAVE_TABLE_0EBC_ARGUMENT,
    PR_U1C_SAVE_TABLE_0EBC_TRANSFER
} pr_u1c_save_table_0ebc_status;

pr_u1c_save_table_0ebc_status pr_u1c_save_table_0ebc_cpu(uint8_t *b, size_t ram_size,
                               pr_candidate_cpu *c, uint16_t caller_cs,
                               void *context);

#endif
