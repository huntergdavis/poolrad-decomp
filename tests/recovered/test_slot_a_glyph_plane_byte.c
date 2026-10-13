#include "poolrad/recovered/slot_a_glyph_plane_byte.h"

#include <stdio.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", \
    __FILE__, __LINE__, #x); return 1; } } while (0)

int main(void) {
    uint8_t block[PR_FONT201_BLOCK_SIZE] = {0};
    pr_font201 font;
    uint8_t byte = 0xEEu;
    unsigned plane;

    /* Synthetic glyph 20 is the upcased/mod-64 index of character 0x54. */
    block[20u * 8u] = 0xA5u;
    block[20u * 8u + 7u] = 0x3Cu;
    block[0x54u * 8u] = 0x96u;
    CHECK(pr_font201_open(block, sizeof block, &font) == PR_FONT201_OK);
    for (plane = 0u; plane < 4u; ++plane) {
        uint8_t expected = (plane & 1u) != 0u ? 0xA5u : 0u;
        CHECK(pr_slot_a_glyph_plane_byte(&font, 0x54u, 1, 0u, 0x0Au,
                                         plane, &byte) == PR_FONT201_OK);
        CHECK(byte == expected);
    }
    CHECK(pr_slot_a_glyph_plane_byte(&font, 0x54u, 0, 0u, 0x01u,
                                     0u, &byte) == PR_FONT201_OK && byte == 0x96u);
    CHECK(pr_slot_a_glyph_plane_byte(&font, 0x54u, 1, 7u, 0x0Fu,
                                     3u, &byte) == PR_FONT201_OK && byte == 0x3Cu);
    CHECK(pr_slot_a_glyph_plane_byte(&font, 0x54u, 1, 0u, 0u,
                                     0u, &byte) == PR_FONT201_OK && byte == 0u);
    byte = 0xEEu;
    CHECK(pr_slot_a_glyph_plane_byte(&font, 0x54u, 1, 8u, 0x0Fu,
                                     0u, &byte) == PR_FONT201_ARGUMENT && byte == 0xEEu);
    CHECK(pr_slot_a_glyph_plane_byte(&font, 0x54u, 1, 0u, 0x0Fu,
                                     4u, &byte) == PR_FONT201_ARGUMENT && byte == 0xEEu);
    CHECK(pr_slot_a_glyph_plane_byte(NULL, 0x54u, 1, 0u, 0x0Fu,
                                     0u, &byte) == PR_FONT201_ARGUMENT && byte == 0xEEu);
    CHECK(pr_slot_a_glyph_plane_byte(&font, 0x54u, 1, 0u, 0x0Fu,
                                     0u, NULL) == PR_FONT201_ARGUMENT);
    return 0;
}
