#ifndef POOLRAD_RECOVERED_RESIDENT_DAX_SELECTOR_PUSH_H
#define POOLRAD_RECOVERED_RESIDENT_DAX_SELECTOR_PUSH_H

#include <stdint.h>

/* Two resident data bytes touched by RES_0802:094A. Their numeric addresses
 * are retained because their complete game/library meaning is unresolved. */
typedef struct {
    uint8_t b5376;
    uint8_t b5377;
} pr_rec_dax_selector_state;

/* Exact local behavior at loaded image 0x0896A, RETF 2. The argument is the
 * complete pushed WORD. Only its low byte is read; the previous b5376 is
 * copied to b5377 before b5376 is replaced. AL returns that low byte.
 * state must point to a valid writable state object. */
uint8_t pr_rec_res_0802_094a(pr_rec_dax_selector_state *state,
                              uint16_t pushed_word);

/* Adapter for the merged class-1 hook's void(void *, uint16_t) callout.
 * The hook ignores the resident routine's AL. context must point to a
 * pr_rec_dax_selector_state. */
void pr_rec_res_0802_094a_hook(void *context, uint16_t pushed_word);

#endif
