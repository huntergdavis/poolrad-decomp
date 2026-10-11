#include "poolrad/recovered/resident_dax_selector_push.h"

uint8_t pr_rec_res_0802_094a(pr_rec_dax_selector_state *state,
                              uint16_t pushed_word) {
    uint8_t al;
    /* 0x0896D..0x08970: save the old selector byte. */
    state->b5377 = state->b5376;
    /* 0x08973..0x0897A: read only [BP+6]'s low byte, replace the current
     * selector, and return that same byte in AL. */
    al = (uint8_t)pushed_word;
    state->b5376 = al;
    return al;
}

void pr_rec_res_0802_094a_hook(void *context, uint16_t pushed_word) {
    (void)pr_rec_res_0802_094a(context, pushed_word);
}
