#include "poolrad/recovered/resident_string_0356.h"

#include <string.h>

pr_rec_0802_0356_status pr_rec_res_0802_0356(
    const uint8_t *source, size_t source_size,
    const uint8_t *needle, size_t needle_size,
    uint8_t *output, size_t output_capacity)
{
    uint8_t work[256], key[256];
    unsigned len, key_len, limit, i;

    if (!source || !needle || !output) return PR_REC_0802_0356_ARGUMENT;
    if (source_size == 0u || needle_size == 0u ||
        source_size - 1u < source[0] || needle_size - 1u < needle[0])
        return PR_REC_0802_0356_BOUNDS;

    /* The first two SYSTEM 0x0B25 calls copy into separate 255-cap locals. */
    len = source[0];
    key_len = needle[0];
    memcpy(work, source, len + 1u);
    memcpy(key, needle, key_len + 1u);

    /* +0x083AD..+0x083CA stores only AL. The unsigned byte limit is fixed
     * before deletion and is never recomputed from the shortened string. */
    limit = (uint8_t)(len - key_len + 1u);
    for (i = 1u; i <= limit; ++i) {
        unsigned tail;
        /* SYSTEM 0x0B57 Copy returns an empty Pascal string when i lies
         * beyond the current length. 0x0C10 tests exact equality. An empty
         * key can compare equal, but SYSTEM 0x0CBC Delete(count=0) is inert. */
        if (key_len == 0u || i > len || key_len > len - i + 1u ||
            memcmp(work + i, key + 1u, key_len) != 0)
            continue;

        /* 0x0CBC constructs prefix+suffix and stores it back. Here both
         * substrings fit the same local, so an in-place shift is equivalent.
         * The next iteration uses i+1, not the newly shifted character at i. */
        tail = len - i - key_len + 1u;
        if (tail != 0u)
            memmove(work + i, work + i + key_len, tail);
        len -= key_len;
        work[0] = (uint8_t)len;
    }

    /* The final SYSTEM 0x0B25 writes only the length byte and live text. */
    if (output_capacity < len + 1u) return PR_REC_0802_0356_BOUNDS;
    memcpy(output, work, len + 1u);
    return PR_REC_0802_0356_OK;
}
