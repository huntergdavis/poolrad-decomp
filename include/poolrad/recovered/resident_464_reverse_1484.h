#ifndef POOLRAD_RECOVERED_RESIDENT_464_REVERSE_1484_H
#define POOLRAD_RECOVERED_RESIDENT_464_REVERSE_1484_H

#include "poolrad/recovered/slot_a_directory_fullstate.h"

/* Complete call-free RES_0464:1484 value contract. The high argument byte
 * is ignored. Source-library ownership remains unresolved. */
uint8_t pr_r464_reverse_1484(uint16_t argument);

/* Run the complete leaf from IP 1484 through RETF 2. The existing full-state
 * container supplies real-mode CPU registers and linear 20-bit RAM. CS is
 * caller supplied: the independent audit must verify the loaded code there.
 * Requires SP in [4, FFFA], so word accesses do not cross the segment end.
 * Returns 1 on success, 0 without mutation for an invalid entry or inaccessible
 * frame. All CPU registers, FLAGS, and final saved/local stack bytes are modeled.
 * No code bytes are embedded here. See docs/re/RESIDENT_464_REVERSE_1484.md. */
int pr_r464_reverse_1484_frame(pr_slot_a_directory_state *state);

#endif
