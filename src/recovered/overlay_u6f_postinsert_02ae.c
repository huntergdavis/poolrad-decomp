#include "poolrad/recovered/overlay_u6f_postinsert_02ae.h"

#include <string.h>

pr_u6f_postinsert_02ae_status pr_u6f_postinsert_02ae(
    uint8_t *live_pascal, size_t live_size,
    const uint8_t *message_prefix, size_t prefix_size,
    const uint8_t *message_suffix, size_t suffix_size,
    const pr_u6f_postinsert_02ae_hooks *hooks,
    size_t max_attempts, pr_u6f_postinsert_02ae_result *out)
{
    pr_u6f_postinsert_02ae_result state = {0};
    uint8_t path[128];
    uint8_t message[256], visible[41];
    size_t copied, length, suffix_length;
    uint16_t io_word;

    if (!out)
        return PR_U6F_POSTINSERT_02AE_ARGUMENT;
    state.next_offset = 0x02b5u;
    *out = state;
    if (!live_pascal || !hooks || max_attempts == 0u)
        return PR_U6F_POSTINSERT_02AE_ARGUMENT;

    for (;;) {
        /* +0x02B5..+0x02CB: Copy(DS:537A, 1, length-1). At length zero
         * the signed count is -1 and SYSTEM Copy clamps it to zero. */
        state.next_offset = 0x02b5u;
        if (live_size == 0u || (size_t)live_pascal[0] > live_size - 1u) {
            *out = state;
            return PR_U6F_POSTINSERT_02AE_BOUNDS;
        }
        length = live_pascal[0];
        copied = length == 0u ? 0u : length - 1u;

        /* SYSTEM 0xAF8:0x17D6 converts its Pascal argument to ASCIIZ,
         * truncating at 0x7F characters before DOS AH=0x39. The caller's
         * stack-local capacity is not reconstructed for oversized states. */
        if (copied > 0x7fu)
            copied = 0x7fu;
        memcpy(path, live_pascal + 1u, copied);
        path[copied] = 0u;
        state.next_offset = 0x02d0u;
        if (!hooks->make_directory) {
            *out = state;
            return PR_U6F_POSTINSERT_02AE_NEED_MKDIR;
        }
        ++state.attempts;
        if (hooks->make_directory(hooks->context, path, copied) != 0) {
            *out = state;
            return PR_U6F_POSTINSERT_02AE_HOST_FAILURE;
        }

        state.next_offset = 0x02d5u;
        if (!hooks->io_result) {
            *out = state;
            return PR_U6F_POSTINSERT_02AE_NEED_IO_RESULT;
        }
        if (hooks->io_result(hooks->context, &io_word) != 0) {
            *out = state;
            return PR_U6F_POSTINSERT_02AE_HOST_FAILURE;
        }
        state.last_io_result = io_word;
        state.has_io_result = 1u;
        if (io_word == 0u || io_word == 5u) {
            state.next_offset = 0x032eu;
            *out = state;
            return PR_U6F_POSTINSERT_02AE_OK;
        }

        /* +0x02EC..+0x0315: private prefix + raw DS:537B byte + private
         * suffix. Both literals are supplied by the host, never embedded.
         * The raw byte is read even when the Pascal string length is zero. */
        state.next_offset = 0x02ecu;
        if (!message_prefix || !message_suffix) {
            *out = state;
            return PR_U6F_POSTINSERT_02AE_NEED_MESSAGE;
        }
        if (prefix_size == 0u || suffix_size == 0u ||
            (size_t)message_prefix[0] > prefix_size - 1u ||
            (size_t)message_suffix[0] > suffix_size - 1u ||
            live_size < 2u) {
            *out = state;
            return PR_U6F_POSTINSERT_02AE_BOUNDS;
        }
        /* Pascal StrCat saturates at 255; retain the first characters. */
        length = message_prefix[0];
        memcpy(message + 1u, message_prefix + 1u, length);
        if (length < 255u)
            message[1u + length++] = live_pascal[1];
        suffix_length = message_suffix[0];
        if (suffix_length > 255u - length)
            suffix_length = 255u - length;
        memcpy(message + 1u + length, message_suffix + 1u,
               suffix_length);
        length += suffix_length;
        message[0] = (uint8_t)length;

        /* RES_0709:0BF5 stores at most 0x28 visible characters before
         * clearing the row, displaying color 0x0E, and reading one key. */
        if (length > 0x28u)
            length = 0x28u;
        visible[0] = (uint8_t)length;
        memcpy(visible + 1u, message + 1u, length);
        state.next_offset = 0x031au;
        if (!hooks->display_and_input) {
            *out = state;
            return PR_U6F_POSTINSERT_02AE_NEED_DISPLAY;
        }
        ++state.displays;
        if (hooks->display_and_input(hooks->context, visible,
                                     length + 1u, 0x0eu) != 0) {
            *out = state;
            return PR_U6F_POSTINSERT_02AE_HOST_FAILURE;
        }
        state.next_offset = 0x02b5u;
        if (state.attempts >= max_attempts) {
            *out = state;
            return PR_U6F_POSTINSERT_02AE_RETRY_LIMIT;
        }
    }
}
