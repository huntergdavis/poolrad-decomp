#include "poolrad/recovered/resident_8ae_dispatch.h"

static pr_rec_8ae_status invoke(pr_rec_8ae_state *state,
                                 uint16_t site, pr_rec_8ae_target target,
                                 uint8_t has_word, uint16_t word,
                                 pr_rec_8ae_result *result)
{
    pr_rec_8ae_call call = {site, target, has_word, word};
    if (result != 0) result->stop_offset = site;
    if (state->hooks == 0 || state->hooks->call == 0 ||
        state->hooks->call(state->hooks->context, state, &call) != 0)
        return PR_REC_8AE_BOUNDARY;
    if (result != 0) ++result->completed_calls;
    return PR_REC_8AE_OK;
}

/* Exact +181A..+182F leaf: the callee stores low(argument) to
 * DS=0x8AE:+020D, restores DS, then RETF 2. */
static void store_181a(pr_rec_8ae_state *state, uint16_t word)
{
    state->driver_byte_020d = (uint8_t)word;
}

#define CALL(site, target, has_word, word) do { \
    if (invoke(state, (site), (target), (has_word), (word), result) != \
        PR_REC_8AE_OK) return PR_REC_8AE_BOUNDARY; \
} while (0)

pr_rec_8ae_status pr_rec_res_08ae_00dc(pr_rec_8ae_state *state,
                                        uint16_t word_argument,
                                        pr_rec_8ae_result *result)
{
    if (result != 0) {
        result->stop_offset = 0x00dcu;
        result->completed_calls = 0;
    }
    if (state == 0) return PR_REC_8AE_ARGUMENT;

    if (state->byte_5372 == 2u) goto done;
    if (state->word_5374 != 0u)
        CALL(0x00f1u, PR_REC_8AE_CALL_137C, 0u, 0u);

    if (word_argument == 0u) {
        CALL(0x0100u, PR_REC_8AE_CALL_17AE, 1u, 0u);
        CALL(0x0107u, PR_REC_8AE_CALL_1742, 1u, 0u);
        store_181a(state, 0u);
    } else if (word_argument == 1u) {
        store_181a(state, 1u);
    } else if (word_argument == 0x00ffu) {
        CALL(0x012bu, PR_REC_8AE_CALL_17AE, 1u, 0u);
        CALL(0x0132u, PR_REC_8AE_CALL_1742, 1u, 0u);
    } else if (state->byte_5372 == 1u) {
        CALL(0x0144u, PR_REC_8AE_CALL_1742, 1u,
             (uint16_t)(word_argument - 1u));
    } else if (state->byte_5372 == 0u) {
        CALL(0x0153u, PR_REC_8AE_CALL_17AE, 1u,
             (uint16_t)(word_argument - 1u));
    }

    if (state->word_5374 != 0u) {
        CALL(0x0162u, PR_REC_8AE_CALL_006C, 1u, 10u);
        CALL(0x0169u, PR_REC_8AE_CALL_17AE, 1u, 0u);
        CALL(0x0170u, PR_REC_8AE_CALL_1742, 1u, 0u);
        CALL(0x0174u, PR_REC_8AE_CALL_13B1, 0u, 0u);
    }
done:
    if (result != 0) result->stop_offset = 0x017bu;
    return PR_REC_8AE_OK;
}
