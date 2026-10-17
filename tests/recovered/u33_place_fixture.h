/* Synthetic RAM for the u33 +1434/+1577 tests: no game bytes, only the far-call
 * site and stub bytes the loaded-state contract reads, and small DS tables. */
#ifndef POOLRAD_TESTS_U33_PLACE_FIXTURE_H
#define POOLRAD_TESTS_U33_PLACE_FIXTURE_H

#include <stdint.h>
#include <string.h>

#define PLACE_RAM (1u << 20)
#define PLACE_CS 0x3000u
#define PLACE_DS 0x1000u
#define PLACE_SS 0x2000u
#define PLACE_SCAN_CS 0x6000u
#define PLACE_GRID_SEG 0x8000u

static inline size_t place_at(uint16_t seg, uint16_t off) { return (((size_t)seg << 4) + off) & (PLACE_RAM - 1u); }
static inline void place_w8(uint8_t *ram, uint16_t seg, uint16_t off, uint8_t v) { ram[place_at(seg, off)] = v; }
static inline uint8_t place_r8(const uint8_t *ram, uint16_t seg, uint16_t off) { return ram[place_at(seg, off)]; }
static inline void place_w16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t v)
{
    place_w8(ram, seg, off, (uint8_t)v);
    place_w8(ram, seg, (uint16_t)(off + 1u), (uint8_t)(v >> 8));
}

/* The +150E far call to 03E0:0070 and the stub there jumping to +0CB9 of the scan unit. */
static inline void place_scan_link(uint8_t *ram)
{
    place_w8(ram, PLACE_CS, 0x150Eu, 0x9Au);
    place_w16(ram, PLACE_CS, 0x150Fu, 0x0070u);
    place_w16(ram, PLACE_CS, 0x1511u, 0x03E0u);
    place_w8(ram, 0x03E0u, 0x0070u, 0xEAu);
    place_w16(ram, 0x03E0u, 0x0071u, 0x0CB9u);
    place_w16(ram, 0x03E0u, 0x0073u, PLACE_SCAN_CS);
}

/* A world in which +0CB9 sees class 2 (weight `weight`) at every probe of slot 1 and the
 * occupant byte `occupant`; availability bytes are all `open`. */
static inline void place_world(uint8_t *ram, uint8_t open, uint8_t weight, uint8_t occupant)
{
    unsigned i;
    static const int8_t dx[9] = {0, 1, 1, 1, 0, -1, -1, -1, 0}, dy[9] = {-1, -1, 0, 1, 1, 1, 0, -1, 0};
    memset(ram, 0xA5, PLACE_RAM);
    place_scan_link(ram);
    for (i = 0; i < 0x210u; ++i) place_w8(ram, PLACE_DS, (uint16_t)(0x4440u + i), open);
    for (i = 0; i < 9u; ++i) {
        place_w8(ram, PLACE_DS, (uint16_t)(0x2736u + i), (uint8_t)dx[i]);
        place_w8(ram, PLACE_DS, (uint16_t)(0x273Fu + i), (uint8_t)dy[i]);
    }
    place_w8(ram, PLACE_DS, 0x5F2Au, 4u);                   /* slot count */
    for (i = 1; i < 8u; ++i) {
        place_w8(ram, PLACE_DS, (uint16_t)(0x5F2Au + 4u * i), 1u);   /* class 1 */
        place_w16(ram, PLACE_DS, (uint16_t)(0x65B9u + 4u * i), (uint16_t)(0x1100u + i));
        place_w16(ram, PLACE_DS, (uint16_t)(0x65BBu + 4u * i), 0x2222u);
    }
    for (i = 0; i < 0x40u; ++i) place_w8(ram, PLACE_DS, (uint16_t)(0x2844u + i), 0u);  /* zero probe offsets */
    place_w8(ram, PLACE_DS, 0x274Cu, weight);               /* weight of class 2 */
    place_w16(ram, PLACE_DS, 0x6716u, 0x0000u);
    place_w16(ram, PLACE_DS, 0x6718u, PLACE_GRID_SEG);
    for (i = 0; i < 50u * 25u; ++i) {
        place_w8(ram, PLACE_GRID_SEG, (uint16_t)(7u + i), 2u);
        place_w8(ram, PLACE_DS, (uint16_t)(0x60DBu + i), occupant);
    }
    place_w8(ram, PLACE_DS, 0x4658u, 0u);                   /* map 0 */
}

#endif
