#include "poolrad/recovered/overlay_u33_stamp_block_02ba.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "u33 +02BA line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
#define RAM (1u << 20)
#define LINK 0x8100u

static size_t at(uint16_t seg, uint16_t off) { return (((size_t)seg << 4) + off) & (RAM - 1u); }

/* A 50x25 grid at 5000:0000 (cells from +7) and a caller frame whose L-8 selector and
 * L-2/L-1 column/row bytes are given; entered with the link word above the return frame. */
static pr_u33_02ba_cpu enter(uint8_t *ram, uint8_t selector, uint8_t fx, uint8_t fy)
{
    pr_u33_02ba_cpu cpu = {0x5A34u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu,
                           0x7777u, 0x8000u, 0x3000u, 0x02BAu, 0x1000u, 0x1111u, 0x2000u, 0x0202u};
    memset(ram, 0xA5, RAM);
    ram[at(0x1000u, 0x6716u)] = 0x00; ram[at(0x1000u, 0x6717u)] = 0x00;
    ram[at(0x1000u, 0x6718u)] = 0x00; ram[at(0x1000u, 0x6719u)] = 0x50;
    memset(ram + at(0x5000u, 0), 0, 0x4F0u);
    ram[at(0x2000u, LINK - 8u)] = selector;
    ram[at(0x2000u, LINK - 2u)] = fx;
    ram[at(0x2000u, LINK - 1u)] = fy;
    ram[at(0x2000u, 0x8000u)] = 0x00; ram[at(0x2000u, 0x8001u)] = 0x70;
    ram[at(0x2000u, 0x8002u)] = 0x01; ram[at(0x2000u, 0x8003u)] = 0x70;
    ram[at(0x2000u, 0x8004u)] = (uint8_t)LINK; ram[at(0x2000u, 0x8005u)] = (uint8_t)(LINK >> 8);
    return cpu;
}

/* Cell (column, row) of the block with fx = fy = 0: x = 15h + column, y = 0Ah + row. */
static uint8_t cell(const uint8_t *ram, unsigned column, unsigned row)
{
    return ram[at(0x5000u, (uint16_t)(50u * (0x0Au + row) + 0x15u + column + 7u))];
}

int main(void)
{
    uint8_t *ram = malloc(RAM);
    pr_u33_02ba_cpu cpu;
    unsigned row, column, written;
    CHECK(ram != NULL);

    /* Selector 1: the 3x6 block of glyph 16h (+1 written) and, per row 2..4, glyphs 4/3/0Dh
     * (+1) at columns row-1, row and row+1. The final flags are CMP [BP-2],4's (equal). */
    cpu = enter(ram, 1u, 0u, 0u);
    CHECK(pr_u33_stamp_block_02ba(ram, RAM, &cpu) == PR_U33_02BA_OK);
    for (row = 2; row <= 4; ++row)
        for (column = 0; column <= 5; ++column) {
            uint8_t want = column + 1u == row ? 5u : column == row ? 4u : column == row + 1u ? 0x0Eu : 0x17u;
            CHECK(cell(ram, column, row) == want);
        }
    CHECK(cell(ram, 0, 1) == 0u && cell(ram, 6, 4) == 0u && cell(ram, 0, 5) == 0u);
    CHECK((cpu.flags & 0x08D5u) == 0x0044u);
    CHECK(cpu.ip == 0x7000u && cpu.cs == 0x7001u && cpu.sp == 0x8006u && cpu.bp == 0x7777u);

    /* Selector 3: the block plus glyph 8 (+1) at (2, 1) and 0 (+1) at (4, 5). */
    cpu = enter(ram, 3u, 0u, 0u);
    CHECK(pr_u33_stamp_block_02ba(ram, RAM, &cpu) == PR_U33_02BA_OK);
    CHECK(cell(ram, 1, 2) == 9u && cell(ram, 5, 4) == 1u && cell(ram, 0, 2) == 0x17u && cell(ram, 4, 4) == 0x17u);

    /* Any other selector: only the block. */
    cpu = enter(ram, 2u, 0u, 0u);
    CHECK(pr_u33_stamp_block_02ba(ram, RAM, &cpu) == PR_U33_02BA_OK);
    for (written = 0, row = 0; row < 25u * 50u; ++row)
        written += ram[at(0x5000u, (uint16_t)(7u + row))] != 0u;
    CHECK(written == 18u && cell(ram, 1, 2) == 0x17u);

    /* A caller column byte of 30h puts every position past column 31h: nothing is written. */
    cpu = enter(ram, 1u, 0x30u, 0u);
    CHECK(pr_u33_stamp_block_02ba(ram, RAM, &cpu) == PR_U33_02BA_OK);
    for (written = 0, row = 0; row < 25u * 50u; ++row)
        written += ram[at(0x5000u, (uint16_t)(7u + row))] != 0u;
    CHECK(written == 0u);

    {
        pr_u33_02ba_cpu before = enter(ram, 1u, 0u, 0u), after = before;
        CHECK(pr_u33_stamp_block_02ba(NULL, RAM, &after) == PR_U33_02BA_ARGUMENT);
        CHECK(pr_u33_stamp_block_02ba(ram, RAM - 1u, &after) == PR_U33_02BA_ARGUMENT);
        CHECK(pr_u33_stamp_block_02ba(ram, RAM, NULL) == PR_U33_02BA_ARGUMENT);
        CHECK(memcmp(&before, &after, sizeof before) == 0);
    }
    free(ram);
    printf("u33 +02BA: selector 1, 3 and other, clipping and guard cases passed\n");
    return 0;
}
