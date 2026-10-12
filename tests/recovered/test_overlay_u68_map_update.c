#include "poolrad/recovered/overlay_u68_map_update.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

static uint8_t segment[65536];

int main(void)
{
    static const struct {
        uint8_t selector, keep, set;
    } cases[] = {
        {6, 0x3Fu, 0x40u},
        {4, 0xCFu, 0x10u},
        {2, 0xF3u, 0x04u},
        {0, 0xFCu, 0x01u}
    };
    size_t i;
    unsigned original;

    /* Each branch replaces only its own two-bit field. Exercise all
     * input byte values, including already-set and unrelated fields. */
    for (i = 0; i < sizeof cases / sizeof cases[0]; ++i)
        for (original = 0; original < 256u; ++original) {
            const uint16_t at = 0x0100u + 0x300u + 16u * 9u + 3u;
            memset(segment, 0xA5, sizeof segment);
            segment[at] = (uint8_t)original;
            CHECK(pr_u68_map_update(3, 9, cases[i].selector, 0x0100u,
                                     segment, sizeof segment) == PR_U68_MAP_UPDATE_OK);
            CHECK(segment[at] == (uint8_t)((original & cases[i].keep) | cases[i].set));
            CHECK(segment[at - 1u] == 0xA5 && segment[at + 1u] == 0xA5);
        }

    /* The 16-byte row stride and x/y order select distinct map bytes. */
    memset(segment, 0xA5, sizeof segment);
    CHECK(pr_u68_map_update(1, 2, 6, 0, segment, sizeof segment)
          == PR_U68_MAP_UPDATE_OK);
    CHECK(segment[0x300u + 33u] == (uint8_t)((0xA5u & 0x3Fu) | 0x40u));
    CHECK(segment[0x300u + 18u] == 0xA5);
    CHECK(pr_u68_map_update(2, 1, 4, 0, segment, sizeof segment)
          == PR_U68_MAP_UPDATE_OK);
    CHECK(segment[0x300u + 18u] == (uint8_t)((0xA5u & 0xCFu) | 0x10u));

    /* Coordinate 15,15 is admitted; effective DI + 0x300 wraps at 64 KiB. */
    memset(segment, 0xA5, sizeof segment);
    CHECK(pr_u68_map_update(15, 15, 0, 0xFC01u, segment, sizeof segment)
          == PR_U68_MAP_UPDATE_OK);
    CHECK(segment[0] == (uint8_t)((0xA5u & 0xFCu) | 1u));
    CHECK(segment[65535] == 0xA5);

    /* Signed-byte out-of-grid values and unsupported selectors do not
     * reach the far pointer, so a missing host view is accepted. */
    for (i = 0; i < 256u; ++i) {
        if (i > 15u) {
            CHECK(pr_u68_map_update((uint8_t)i, 0, 6, 0, NULL, 0)
                  == PR_U68_MAP_UPDATE_OK);
            CHECK(pr_u68_map_update(0, (uint8_t)i, 6, 0, NULL, 0)
                  == PR_U68_MAP_UPDATE_OK);
        }
        if (i != 0u && i != 2u && i != 4u && i != 6u)
            CHECK(pr_u68_map_update(0, 0, (uint8_t)i, 0, NULL, 0)
                  == PR_U68_MAP_UPDATE_OK);
    }

    /* The host guard fires only for a selected, in-grid map byte. */
    CHECK(pr_u68_map_update(0, 0, 0, 0, NULL, 0)
          == PR_U68_MAP_UPDATE_ARGUMENT);
    CHECK(pr_u68_map_update(0, 0, 2, 0, segment, sizeof segment - 1u)
          == PR_U68_MAP_UPDATE_ARGUMENT);
    CHECK(segment[0x300u] == 0xA5);

    puts("overlay_u68_map_update: ok");
    return 0;
}
