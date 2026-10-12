#include "poolrad/recovered/resident_464_bitmask.h"

uint8_t pr_r464_bitmask(uint16_t first_word, uint16_t second_word,
                         uint8_t selector) {
    uint8_t packed[4];
    uint8_t result = 0;
    uint8_t bit = 0x80;
    uint8_t high_selector = (uint8_t)((uint16_t)selector << 4);
    unsigned i;

    packed[0] = (uint8_t)first_word;
    packed[1] = (uint8_t)(first_word >> 8);
    packed[2] = (uint8_t)second_word;
    packed[3] = (uint8_t)(second_word >> 8);

    /* +0x0024..+0x006A: exactly four source bytes, high nibble then low
     * nibble. Each successful comparison adds the current distinct bit. */
    for (i = 0; i < 4; ++i) {
        if ((uint8_t)(packed[i] & 0xF0u) == high_selector)
            result = (uint8_t)(result + bit);
        bit = (uint8_t)(bit >> 1);
        if ((uint8_t)(packed[i] & 0x0Fu) == selector)
            result = (uint8_t)(result + bit);
        bit = (uint8_t)(bit >> 1);
    }
    return result;
}
