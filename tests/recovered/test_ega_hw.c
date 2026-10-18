/* Focused test for the EGA planar-mode model (docs/re/EGA_HW_MODEL.md), with hand-predicted register, latch, write-mode
 * and read-mode effects. */
#include "poolrad/recovered/ega_hw.h"

#include <stdio.h>
#include <stdlib.h>

#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "ega line %d: %s\n", __LINE__, #x);                   \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

static unsigned checks;

static void gc(pr_ega *ega, uint8_t index, uint8_t value)
{
    CHECK(pr_ega_out(ega, 0x3CEu, (uint16_t)(index | (value << 8)), 2u) == PR_EGA_OK);
}

int main(void)
{
    pr_ega *ega = malloc(sizeof *ega);
    if (ega == NULL)
        return 1;
    pr_ega_reset(ega);
    CHECK(ega->seq[2] == 0x0F && ega->gc[7] == 0x0F && ega->gc[8] == 0xFF && ega->gc[5] == 0);

    /* Byte OUTs to index then data, and a word OUT doing both; unknown ports and indexes are ignored. */
    CHECK(pr_ega_out(ega, 0x3C4u, 2u, 1u) == PR_EGA_OK && pr_ega_out(ega, 0x3C5u, 0x05u, 1u) == PR_EGA_OK && ega->seq[2] == 0x05);
    gc(ega, 8, 0xF0);
    CHECK(ega->gc_index == 8 && ega->gc[8] == 0xF0);
    CHECK(pr_ega_out(ega, 0x3D4u, 1u, 1u) == PR_EGA_IGNORED);
    CHECK(pr_ega_out(ega, 0x3CEu, 9u, 1u) == PR_EGA_OK && pr_ega_out(ega, 0x3CFu, 1u, 1u) == PR_EGA_IGNORED);

    /* Write mode 0, map mask 0101b, bit mask F0h, latches zero: planes 0 and 2 get A5h AND F0h = A0h. */
    CHECK(pr_ega_write(ega, 0x0010u, 0xA5u) == PR_EGA_OK);
    CHECK(ega->plane[0][0x10] == 0xA0 && ega->plane[1][0x10] == 0 && ega->plane[2][0x10] == 0xA0 && ega->plane[3][0x10] == 0);

    /* A read loads all latches; read mode 0 returns the read-map plane. */
    gc(ega, 4, 2);
    CHECK(pr_ega_read(ega, 0x0010u) == 0xA0 && ega->latch[0] == 0xA0 && ega->latch[1] == 0);

    /* Rotate right by 4 and XOR with the latches under bit mask FFh: plane 0 = 5Ah XOR A0h = FAh. */
    gc(ega, 8, 0xFF);
    gc(ega, 3, (uint8_t)(0x18u | 4u));
    CHECK(pr_ega_write(ega, 0x0010u, 0xA5u) == PR_EGA_OK && ega->plane[0][0x10] == 0xFA && ega->plane[2][0x10] == 0xFA);

    /* Enable set/reset on plane 2 (set/reset 0) with function AND: plane 2 = 00h AND A0h = 0; plane 0 rotates. */
    gc(ega, 3, 0x08u);
    gc(ega, 1, 0x04u);
    gc(ega, 0, 0x00u);
    CHECK(pr_ega_write(ega, 0x0010u, 0xFFu) == PR_EGA_OK && ega->plane[2][0x10] == 0x00 && ega->plane[0][0x10] == 0xA0);

    /* Write mode 1 copies the latches (A0h, 0, A0h, 0) to the mapped planes regardless of the bit mask. */
    pr_ega_out(ega, 0x3C4u, 0x0F02u, 2u);
    gc(ega, 8, 0x00);
    gc(ega, 5, 0x01);
    CHECK(pr_ega_write(ega, 0x0020u, 0x55u) == PR_EGA_OK);
    CHECK(ega->plane[0][0x20] == 0xA0 && ega->plane[1][0x20] == 0 && ega->plane[2][0x20] == 0xA0 && ega->plane[3][0x20] == 0);

    /* Write mode 2, function replace, bit mask 0Fh: color 1010b fills planes 1 and 3 in the low nibble over the latches. */
    gc(ega, 5, 0x02);
    gc(ega, 3, 0x00);
    gc(ega, 8, 0x0F);
    CHECK(pr_ega_write(ega, 0x0030u, 0x0Au) == PR_EGA_OK);
    CHECK(ega->plane[0][0x30] == 0xA0 && ega->plane[1][0x30] == 0x0F && ega->plane[2][0x30] == 0xA0 && ega->plane[3][0x30] == 0x0F);

    /* Read mode 1: color compare 1010b on all planes matches the low nibble of offset 30h. */
    gc(ega, 5, 0x08);
    gc(ega, 2, 0x0A);
    CHECK(pr_ega_read(ega, 0x0030u) == 0x0F);
    gc(ega, 7, 0x00);
    CHECK(pr_ega_read(ega, 0x0030u) == 0xFF);

    /* Write mode 3 is VGA-only and refused. */
    gc(ega, 5, 0x03);
    CHECK(pr_ega_write(ega, 0x0040u, 0xFFu) == PR_EGA_UNSUPPORTED && ega->plane[0][0x40] == 0);

    free(ega);
    printf("ega hw: %u checks passed\n", checks);
    return 0;
}
