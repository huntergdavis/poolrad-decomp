#ifndef POOLRAD_REPLAY_SLOT_A_AND_COMPARE_HELPER_H
#define POOLRAD_REPLAY_SLOT_A_AND_COMPARE_HELPER_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
#include "poolrad/recovered/overlay_u25_compare_words_18eb_cpu.h"
/* Caller authenticates call 5, stub 5, body 102 and fill 20 bytes, keeps
 * guest writes away from code, and supplies disjoint host objects. */
typedef struct { const uint8_t *call,*stub,*body,*fill; } pr_and_compare_refs;
int pr_slot_a_and_compare_helper(uint8_t *ram,size_t size,const pr_and_compare_refs *refs,uint8_t synthetic,pr_u1c_3171_cpu *cpu,pr_u25_18eb_status *status);
#endif
