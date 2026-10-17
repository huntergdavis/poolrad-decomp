#include "poolrad/recovered/overlay_u33_stamp_square_0377.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "u33 +0377 line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
#define RAM (1u << 20)
#define LINK 0x8100u

static size_t at(uint16_t seg, uint16_t off) { return (((size_t)seg << 4) + off) & (RAM - 1u); }

static pr_u33_0377_cpu enter(uint8_t *ram, uint8_t selector)
{
    pr_u33_0377_cpu cpu = {0x5A34u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu,
                           0x7777u, 0x8000u, 0x3000u, 0x0377u, 0x1000u, 0x1111u, 0x2000u, 0x0202u};
    memset(ram, 0xA5, RAM);
    ram[at(0x1000u, 0x6716u)] = 0x00; ram[at(0x1000u, 0x6717u)] = 0x00;
    ram[at(0x1000u, 0x6718u)] = 0x00; ram[at(0x1000u, 0x6719u)] = 0x50;
    memset(ram + at(0x5000u, 0), 0, 0x4F0u);
    ram[at(0x2000u, LINK - 7u)] = selector;
    ram[at(0x2000u, LINK - 2u)] = 0u;
    ram[at(0x2000u, LINK - 1u)] = 0u;
    ram[at(0x2000u, 0x8000u)] = 0x00; ram[at(0x2000u, 0x8001u)] = 0x70;
    ram[at(0x2000u, 0x8002u)] = 0x01; ram[at(0x2000u, 0x8003u)] = 0x70;
    ram[at(0x2000u, 0x8004u)] = (uint8_t)LINK; ram[at(0x2000u, 0x8005u)] = (uint8_t)(LINK >> 8);
    return cpu;
}

static uint8_t cell(const uint8_t *ram, unsigned column, unsigned row)
{
    return ram[at(0x5000u, (uint16_t)(50u * (0x0Au + row) + 0x15u + column + 7u))];
}

int main(void)
{
    uint8_t *ram = malloc(RAM);
    pr_u33_0377_cpu cpu;
    unsigned i, written;
    CHECK(ram != NULL);

    /* Selector 1: glyph 5 (+1) on row 0 and 0Ah (+1) on row 1, columns 3 and 4. */
    cpu = enter(ram, 1u);
    CHECK(pr_u33_stamp_square_0377(ram, RAM, &cpu) == PR_U33_0377_OK);
    CHECK(cell(ram, 3, 0) == 6u && cell(ram, 4, 0) == 6u && cell(ram, 3, 1) == 0x0Bu && cell(ram, 4, 1) == 0x0Bu);
    CHECK(cpu.ip == 0x7000u && cpu.cs == 0x7001u && cpu.sp == 0x8006u && cpu.bp == 0x7777u && cpu.di != LINK);

    /* Any other selector: glyph 16h (+1) at the same four cells and nowhere else. */
    cpu = enter(ram, 0u);
    CHECK(pr_u33_stamp_square_0377(ram, RAM, &cpu) == PR_U33_0377_OK);
    CHECK(cell(ram, 3, 0) == 0x17u && cell(ram, 4, 0) == 0x17u && cell(ram, 3, 1) == 0x17u && cell(ram, 4, 1) == 0x17u);
    for (written = 0, i = 0; i < 25u * 50u; ++i)
        written += ram[at(0x5000u, (uint16_t)(7u + i))] != 0u;
    CHECK(written == 4u);

    {
        pr_u33_0377_cpu before = enter(ram, 1u), after = before;
        CHECK(pr_u33_stamp_square_0377(NULL, RAM, &after) == PR_U33_0377_ARGUMENT);
        CHECK(pr_u33_stamp_square_0377(ram, RAM - 1u, &after) == PR_U33_0377_ARGUMENT);
        CHECK(pr_u33_stamp_square_0377(ram, RAM, NULL) == PR_U33_0377_ARGUMENT);
        CHECK(memcmp(&before, &after, sizeof before) == 0);
    }
    free(ram);
    printf("u33 +0377: selector 1 and other, extent and guard cases passed\n");
    return 0;
}
