/* Shared synthetic fixture for the three dice-caller tests: a loaded-state
 * call site and stub, the entry 8/9 and SYSTEM segments the merged chain
 * expects, the SYSTEM multiplier word, and the generator's LCG for the
 * expected rolls. No original code bytes are needed by the models. */
#ifndef POOLRAD_TESTS_DICE_CHAIN_FIXTURE_H
#define POOLRAD_TESTS_DICE_CHAIN_FIXTURE_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define DICE_RAM (1u << 20)
#define DICE_CS 0x4000u
#define DICE_STUB_PARAGRAPH 0x00B0u

static size_t dice_physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & (DICE_RAM - 1u);
}

static void dice_put16(uint8_t *ram, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t at = dice_physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & (DICE_RAM - 1u)] = (uint8_t)(value >> 8);
}

/* A far-call site `9A stub_off 00 B0 00` at cs:site and the loaded stub
 * `EA entry 00 40` at 00B0:stub_off. */
static void dice_site(uint8_t *ram, uint16_t cs, uint16_t site, uint8_t stub_off, uint16_t entry)
{
    ram[dice_physical(cs, site)] = 0x9Au;
    dice_put16(ram, cs, (uint16_t)(site + 1u), stub_off);
    dice_put16(ram, cs, (uint16_t)(site + 3u), DICE_STUB_PARAGRAPH);
    ram[dice_physical(DICE_STUB_PARAGRAPH, stub_off)] = 0xEAu;
    dice_put16(ram, DICE_STUB_PARAGRAPH, (uint16_t)(stub_off + 1u), entry);
    dice_put16(ram, DICE_STUB_PARAGRAPH, (uint16_t)(stub_off + 3u), DICE_CS);
}

static void dice_system(uint8_t *ram, uint16_t ds, uint32_t seed)
{
    dice_put16(ram, 0x0AF8u, 0x1245u, 0x8405u);
    dice_put16(ram, ds, 0x4428u, (uint16_t)seed);
    dice_put16(ram, ds, 0x442Au, (uint16_t)(seed >> 16));
}

static uint32_t dice_seed(const uint8_t *ram, uint16_t ds)
{
    size_t at = dice_physical(ds, 0x4428u);
    return (uint32_t)ram[at] | ((uint32_t)ram[at + 1u] << 8) | ((uint32_t)ram[at + 2u] << 16) | ((uint32_t)ram[at + 3u] << 24);
}

/* The generator: seed := seed * 08088405h + 1; a (count, range) roll sums
 * count values of (high word mod range) + 1 into an 8-bit total. */
static uint8_t dice_roll(uint32_t *seed, unsigned count, unsigned range)
{
    uint8_t total = 0u;
    unsigned i;
    for (i = 0u; i < count; ++i) {
        *seed = (uint32_t)(*seed * 0x08088405u + 1u);
        total = (uint8_t)(total + (uint8_t)((range ? ((*seed >> 16) % range) : 0u) + 1u));
    }
    return total;
}

#endif
