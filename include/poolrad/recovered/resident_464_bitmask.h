#ifndef POOLRAD_RECOVERED_RESIDENT_464_BITMASK_H
#define POOLRAD_RECOVERED_RESIDENT_464_BITMASK_H

#include <stdint.h>

/* Full call-free RES_0464:0000 helper. The two words are the four packed
 * bytes at DOS frame [BP+6]..[BP+9], in little-endian order. The low byte
 * of [BP+0x0A] is the raw selector; RETF 6 returns the mask in AL.
 * See docs/re/RESIDENT_464_BITMASK.md for address and branch evidence. */
uint8_t pr_r464_bitmask(uint16_t first_word, uint16_t second_word,
                         uint8_t selector);

#endif
