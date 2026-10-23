#ifndef POOLRAD_RECOVERED_OVERLAY_LOADED_CONTRACTS_H
#define POOLRAD_RECOVERED_OVERLAY_LOADED_CONTRACTS_H
#include "overlay_loaded_stubs.h"

/* Shared dependency preflights keep outer refusal atomic as well. */
static inline int pr_u3c2_0909_stubs_loaded(const uint8_t *ram, uint16_t cs, uint16_t slots)
{
    return pr_overlay_stub_matches(ram, cs, 0x092Au, 0x0025u, 0x0000u, slots) &&
           pr_overlay_stub_matches(ram, cs, 0x09D8u, 0x0025u, 0x0000u, slots);
}
static inline int pr_u5e_263d_stubs_loaded(const uint8_t *ram, uint16_t cs,
                                         uint16_t slots, uint16_t predicate)
{
    return pr_overlay_stub_matches(ram, cs, 0x2649u, 0x005Cu, 0x0BE7u, slots) &&
           pr_overlay_stub_matches(ram, cs, 0x2657u, 0x0061u, 0x0C0Fu, slots) &&
           pr_overlay_stub_matches(ram, cs, 0x2665u, 0x005Cu, 0x0BE7u, slots) &&
           pr_overlay_stub_matches(ram, cs, 0x2673u, 0x0061u, 0x0C0Fu, slots) &&
           pr_overlay_stub_matches(ram, cs, 0x2693u, 0x0034u, 0x057Bu, predicate);
}
#endif
