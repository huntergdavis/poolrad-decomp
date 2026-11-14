#ifndef POOLRAD_REPLAY_SLOT_A_RUNNER_OPCODE_PREFIX_H
#define POOLRAD_REPLAY_SLOT_A_RUNNER_OPCODE_PREFIX_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
/* Composes existing fixed 3582:318D..3199 models. Returns the first refused
 * instruction offset, or 319A on success. Prior reached CPU/RAM effects remain.
 * RAM and CPU must be disjoint; no subsequent ADD or opcode fetch is run. */
uint16_t pr_slot_a_runner_opcode_prefix(uint8_t *ram,size_t size,pr_u1c_3171_cpu *cpu);
#endif
