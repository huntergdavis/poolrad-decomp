#ifndef POOLRAD_RECOVERED_U1C_SAVE_0292_H
#define POOLRAD_RECOVERED_U1C_SAVE_0292_H

#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/candidate_ecl_cpu.h"

/* Candidate-scout draft, no credit claimed.
 * GAME.OVR unit 1Ch +[0292,02E0): ECL opcode 0x09 SAVE. Far Pascal, no arguments, RETF. Loads 2 operands; slot 1 is the value, slot 2 the destination; tag[1] >= 80h is a string SAVE, else a word SAVE.
 * Raw-CPU C: one case per original instruction, operands reread from flat 1 MiB RAM.
 * Unit-0x25 runtime calls are shared candidate doubles, not the real helpers. */
typedef enum {
    PR_U1C_SAVE_0292_OK = 0,
    PR_U1C_SAVE_0292_ARGUMENT,
    PR_U1C_SAVE_0292_TRANSFER
} pr_u1c_save_0292_status;

pr_u1c_save_0292_status pr_u1c_save_0292_cpu(uint8_t *b, size_t ram_size,
                               pr_candidate_cpu *c, uint16_t caller_cs,
                               void *context);

#endif
