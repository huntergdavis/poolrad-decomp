#include "poolrad/recovered/overlay_u6f_vector_merge_032f.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static size_t address(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & (PR_U6F_032F_RAM_SIZE - 1u);
}

static void put16(uint8_t *ram, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t at = address(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & (PR_U6F_032F_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static void pointer(uint8_t *ram, uint16_t segment, uint16_t offset,
                    uint16_t target_segment, uint16_t target_offset)
{
    put16(ram, segment, offset, target_offset);
    put16(ram, segment, (uint16_t)(offset + 2u), target_segment);
}

static pr_u6f_032f_cpu setup(uint8_t *ram, uint8_t mode)
{
    pr_u6f_032f_cpu c = {
        0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu,
        0x7777u, 0x8000u, 0x9000u, 0x032Fu, 0x1000u, 0x1111u,
        0xE000u, 0x7202u
    };
    memset(ram, 0xA5, PR_U6F_032F_RAM_SIZE);
    pointer(ram, c.ss, c.sp, 0x1234u, 0x5678u);
    pointer(ram, c.ss, 0x8004u, 0x4000u, 0x0100u);
    pointer(ram, c.ss, 0x8008u, 0x5000u, 0x0200u);
    ram[address(c.ds, 0x4A1Eu)] = mode;
    put16(ram, 0x5000u, 0x0211u, 5u);
    pointer(ram, 0x4000u, 0x0113u, 0x6000u, 0x0300u);
    pointer(ram, 0x5000u, 0x0213u, 0x7000u, 0x0400u);
    pointer(ram, c.ds, 0x5EB2u, 0x3000u, 0x0500u);
    put16(ram, 0x3000u, 0x0511u, 24u);
    for (unsigned i = 0; i < 8u; ++i) {
        ram[address(0x4000u, (uint16_t)(0x0117u + i))] = (uint8_t)(0x12u + 17u*i);
        ram[address(0x5000u, (uint16_t)(0x0217u + i))] = (uint8_t)(0x34u + 13u*i);
        ram[address(0x6000u, (uint16_t)(0x0300u + i))] = (uint8_t)(0xAAu ^ i);
        ram[address(0x7000u, (uint16_t)(0x0400u + i))] = (uint8_t)(0xF0u ^ i);
    }
    return c;
}

int main(void)
{
    uint8_t *ram = malloc(PR_U6F_032F_RAM_SIZE);
    uint8_t *before = malloc(PR_U6F_032F_RAM_SIZE);
    const uint8_t modes[] = {0u, 1u, 2u, 3u, 0x80u, 0xFFu};
    assert(ram != NULL && before != NULL);
    for (unsigned m = 0; m < sizeof modes; ++m) {
        uint8_t mode = modes[m];
        unsigned mask_count = mode == 1u ? 6u : 5u;
        int valid = mode <= 2u;
        pr_u6f_032f_cpu c = setup(ram, mode);
        memcpy(before, ram, PR_U6F_032F_RAM_SIZE);
        assert(pr_u6f_vector_merge_032f(ram, PR_U6F_032F_RAM_SIZE, &c) == PR_U6F_032F_OK);
        for (unsigned i = 0; i < 8u; ++i) {
            size_t dst = address(0x4000u, (uint16_t)(0x0117u + i));
            size_t src = address(0x5000u, (uint16_t)(0x0217u + i));
            size_t mask = address(0x6000u, (uint16_t)(0x0300u + i));
            size_t input = address(0x7000u, (uint16_t)(0x0400u + i));
            assert(ram[dst] == (valid && i < 5u ? (uint8_t)(before[dst] | before[src]) : before[dst]));
            assert(ram[mask] == (valid && i < mask_count ? (uint8_t)(before[mask] & before[input]) : before[mask]));
            assert(ram[src] == before[src] && ram[input] == before[input]);
        }
        assert(c.bp == 0x7777u && c.sp == 0x800Cu);
        assert(c.cs == 0x1234u && c.ip == 0x5678u);
        assert(c.bx == 0x3456u && c.cx == 0x5678u && c.si == 0x9ABCu);
        assert(c.ds == 0x1000u && c.ss == 0xE000u);
        if (valid) {
            assert(c.ax == mask_count - 1u && c.flags == 0x7246u);
            assert(c.es == 0x6000u && c.di == 0x0300u + mask_count - 1u);
        } else {
            assert(c.ax == (uint16_t)(0x1200u | mode));
            assert(c.dx == 0x789Au && c.es == 0x1111u && c.di == 0xBCDEu);
            assert(c.flags == (mode == 3u ? 0x7202u : mode == 0x80u ? 0x7A12u : 0x7282u));
        }
    }
    {
        pr_u6f_032f_cpu c = setup(ram, 0u), saved = c;
        memcpy(before, ram, PR_U6F_032F_RAM_SIZE);
        assert(pr_u6f_vector_merge_032f(NULL, PR_U6F_032F_RAM_SIZE, &c) == PR_U6F_032F_ARGUMENT);
        assert(pr_u6f_vector_merge_032f(ram, PR_U6F_032F_RAM_SIZE - 1u, &c) == PR_U6F_032F_ARGUMENT);
        assert(pr_u6f_vector_merge_032f(ram, PR_U6F_032F_RAM_SIZE, NULL) == PR_U6F_032F_ARGUMENT);
        assert(memcmp(&saved, &c, sizeof c) == 0);
        assert(memcmp(before, ram, PR_U6F_032F_RAM_SIZE) == 0);
    }
    {
        pr_u6f_032f_cpu c = setup(ram, 0u);
        put16(ram, 0x5000u, 0x0211u, 2u);
        pointer(ram, 0x4000u, 0x0113u, 0x5000u, 0x0213u);
        pointer(ram, 0x5000u, 0x0213u, 0x7000u, 0x0401u);
        ram[address(0x7000u, 0x0401u)] = 0xFEu;
        ram[address(0x7000u, 0x0402u)] = 0u;
        assert(pr_u6f_vector_merge_032f(ram, PR_U6F_032F_RAM_SIZE, &c) == PR_U6F_032F_OK);
        /* The first write changes source offset 0401 -> 0400; a stale LES
         * would read the zero at 0402 and clear this high offset byte. */
        assert(ram[address(0x5000u, 0x0213u)] == 0u);
        assert(ram[address(0x5000u, 0x0214u)] == 4u);
    }
    {
        pr_u6f_032f_cpu c = setup(ram, 1u);
        put16(ram, 0x5000u, 0x0211u, 1u);
        put16(ram, 0x3000u, 0x0511u, 0u);
        memset(ram + address(0x7000u, 0u), 0xFF, 65536u);
        assert(pr_u6f_vector_merge_032f(ram, PR_U6F_032F_RAM_SIZE, &c) == PR_U6F_032F_OK);
        assert(c.ax == 0xFFFFu && c.di == 0x02FFu && c.flags == 0x7246u);
    }
    free(before);
    free(ram);
    return 0;
}
