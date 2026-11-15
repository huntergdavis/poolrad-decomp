#ifndef POOLRAD_REPLAY_SLOT_A_AND_WORD_HELPER_H
#define POOLRAD_REPLAY_SLOT_A_AND_WORD_HELPER_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
#include "poolrad/recovered/overlay_u25_loader_0000.h"
/* Synthetic-result-guarded 0E1B far call through :004D to U25:06AF. Caller authenticates
 * all references and loaded binding, keeps host objects disjoint and guest
 * writes away from code. Invalid binding refuses atomically. Once reached,
 * helper status and actual CPU/RAM (including altered returns) are exposed. */
int pr_slot_a_and_word_helper(uint8_t *ram,size_t size,const uint8_t *call,
    size_t call_size,const uint8_t *stub,size_t stub_size,const uint8_t *body,
    size_t body_size,uint8_t synthetic,pr_u1c_3171_cpu *cpu,pr_u25_0000_status *status);
#endif
