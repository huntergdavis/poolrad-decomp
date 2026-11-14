#ifndef POOLRAD_REPLAY_SLOT_A_COMPARE_TAG_TESTS_H
#define POOLRAD_REPLAY_SLOT_A_COMPARE_TAG_TESTS_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
/* Conditional COMPARE 0135..0142 tag tests. Caller authenticates code,
 * supplies disjoint host objects and full RAM. Invalid input refuses
 * atomically; success stops before string setup or numeric setup. */
int pr_slot_a_compare_tag_tests(const uint8_t *ram,size_t size,const uint8_t *code,
 size_t code_size,pr_u1c_3171_cpu *cpu);
#endif
