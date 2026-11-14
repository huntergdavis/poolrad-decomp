#ifndef POOLRAD_REPLAY_SLOT_A_COMPARE_WORD_EFFECTS_H
#define POOLRAD_REPLAY_SLOT_A_COMPARE_WORD_EFFECTS_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
#include "poolrad/recovered/overlay_u25_compare_words_18eb_cpu.h"
/* Authenticated spans: call 14, stub 5, body 102, FillChar 20 bytes.
 * Guest writes must not overlap code and host objects must be disjoint. */
typedef struct { const uint8_t *call,*stub,*body,*fill; } pr_compare_word_refs;
int pr_slot_a_compare_word_effects(uint8_t *ram,size_t size,const pr_compare_word_refs *refs,
 pr_u1c_3171_cpu *cpu,pr_u25_18eb_status *status);
#endif
