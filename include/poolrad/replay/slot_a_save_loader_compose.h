#ifndef POOLRAD_REPLAY_SLOT_A_SAVE_LOADER_COMPOSE_H
#define POOLRAD_REPLAY_SLOT_A_SAVE_LOADER_COMPOSE_H
#include "poolrad/recovered/overlay_u25_loader_0000.h"
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
/* Conditional :002A stub -> U25 +0000. Caller independently authenticates
 * stub/body references and SYSTEM binding, ensures code immutability and the
 * raw loader's nonalias contract, and supplies disjoint host objects. A loader
 * limit/transfer exposes its reached CPU/RAM, never a fabricated return. */
typedef enum { PR_SLOT_A_LOAD_REFUSED=0, PR_SLOT_A_LOAD_REACHED=1 } pr_slot_a_load_status;
pr_slot_a_load_status pr_slot_a_save_loader_compose(uint8_t *ram,size_t size,
    const uint8_t *stub,size_t stub_size,const uint8_t *body,size_t body_size,
    uint16_t system_cs,size_t max_operands,size_t max_strings,
    pr_u1c_3171_cpu *cpu,pr_u25_0000_status *loader_status);
#endif
