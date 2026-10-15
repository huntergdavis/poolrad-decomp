#include "resident_0709_first_glyph_return.h"

#define RAM_MASK (PR_1816_CALLER_RAM_SIZE - 1u)

static size_t addr(uint16_t seg, uint16_t off) {
    return (((size_t)seg << 4) + off) & RAM_MASK;
}

static uint8_t rd8(const uint8_t *ram, uint16_t seg, uint16_t off) {
    return ram[addr(seg, off)];
}

static uint16_t rd16(const uint8_t *ram, uint16_t seg, uint16_t off) {
    size_t at = addr(seg, off);
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & RAM_MASK] << 8));
}

static void wr8(uint8_t *ram, uint16_t seg, uint16_t off, uint8_t value) {
    ram[addr(seg, off)] = value;
}

static void wr16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value) {
    size_t at = addr(seg, off);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & RAM_MASK] = (uint8_t)(value >> 8);
}

static void push(uint8_t *ram, pr_1816_caller_cpu *cpu, uint16_t value) {
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    wr16(ram, cpu->ss, cpu->sp, value);
}

static uint16_t pop(const uint8_t *ram, pr_1816_caller_cpu *cpu) {
    uint16_t value = rd16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return value;
}

static uint16_t parity8(uint8_t value) {
    unsigned i;
    for (i = 0u; i < 4u; ++i) value ^= (uint8_t)(value >> (1u << i));
    return (value & 1u) ? 0u : 0x0004u;
}

static uint16_t cmp8(uint16_t old, uint8_t lhs, uint8_t rhs) {
    uint8_t result = (uint8_t)(lhs - rhs);
    uint16_t bits = parity8(result);
    if (lhs < rhs) bits |= 0x0001u;
    if (((lhs ^ rhs ^ result) & 0x10u) != 0u) bits |= 0x0010u;
    if (result == 0u) bits |= 0x0040u;
    if (result & 0x80u) bits |= 0x0080u;
    if (((lhs ^ rhs) & (lhs ^ result) & 0x80u) != 0u) bits |= 0x0800u;
    return (uint16_t)((old & (uint16_t)~0x08D5u) | bits);
}

static void store_byte(uint8_t *ram, pr_1816_caller_cpu *cpu,
                       uint16_t seg, uint16_t off, uint8_t value) {
    cpu->ax = seg;
    push(ram, cpu, cpu->ax);
    cpu->di = off;
    cpu->es = pop(ram, cpu);
    wr8(ram, cpu->es, cpu->di, value);
}

int pr_0415_first_glyph_return(uint8_t *ram, size_t ram_size,
                                pr_1816_caller_cpu *cpu) {
    uint16_t frame, font_off, font_seg, page_seg, draw_seg, off;
    uint8_t glyph, color, count, column, first_plane;
    unsigned scanline, plane;
    if (ram == NULL || cpu == NULL || ram_size < PR_1816_CALLER_RAM_SIZE ||
        cpu->cs != 0x0709u || cpu->ip != 0x0415u) return 1;
    frame = cpu->bp;
    first_plane = rd8(ram, cpu->ss, (uint16_t)(frame - 0x0Cu));
    color = rd8(ram, cpu->ss, (uint16_t)(frame + 0x0Cu));
    count = rd8(ram, cpu->ss, (uint16_t)(frame - 0x11u));
    font_off = rd16(ram, cpu->ds, 0x5E20u);
    font_seg = rd16(ram, cpu->ds, 0x5E22u);
    page_seg = rd16(ram, cpu->ds, 0x4A16u);
    draw_seg = rd16(ram, cpu->ds, 0x4A18u);
    off = rd16(ram, cpu->ss, (uint16_t)(frame - 4u));
    if (first_plane > 2u || count != 1u ||
        rd8(ram, cpu->ss, (uint16_t)(frame - 0x0Fu)) != 0u ||
        rd8(ram, cpu->ds, 0x4A1Eu) != 1u ||
        !(rd8(ram, cpu->ds, (uint16_t)(0x25C9u + 3u - first_plane)) & color) ||
        cpu->di != off || cpu->es != page_seg ||
        (font_off | font_seg) == 0u) return 2;
    glyph = rd8(ram, cpu->ss, (uint16_t)(frame - 2u));

    /* 0415..0438 finishes the first selected plane's draw-segment copy. */
    store_byte(ram, cpu, draw_seg, off,
               rd8(ram, font_seg, (uint16_t)(font_off +
                                            (uint16_t)glyph * 8u)));

    /* 0455/03BD iterate the remaining planes, then 045E/03B4 advance
     * scanlines. Port writes change DX but have no flat-RAM side effect. */
    for (scanline = 0u; scanline < 8u; ++scanline) {
        uint8_t font_byte = rd8(ram, font_seg,
            (uint16_t)(font_off + (uint16_t)glyph * 8u + scanline));
        if (scanline != 0u)
            wr8(ram, cpu->ss, (uint16_t)(frame - 0x0Fu), (uint8_t)scanline);
        for (plane = scanline == 0u ? (unsigned)first_plane + 1u : 0u;
             plane < 4u; ++plane) {
            uint8_t mask = rd8(ram, cpu->ds,
                (uint16_t)(0x25C9u + 3u - plane));
            uint8_t value = (mask & color) ? font_byte : 0u;
            wr8(ram, cpu->ss, (uint16_t)(frame - 0x0Cu), (uint8_t)plane);
            store_byte(ram, cpu, page_seg, off, value);
            store_byte(ram, cpu, draw_seg, off, value);
        }
        off = (uint16_t)(off + 40u);
        wr16(ram, cpu->ss, (uint16_t)(frame - 4u), off);
    }

    cpu->cx = 3u;
    cpu->dx = 0x03C5u;
    column = rd8(ram, cpu->ss, (uint16_t)(frame + 0x10u));
    wr8(ram, cpu->ss, (uint16_t)(frame + 0x10u),
        (uint8_t)(column + 1u));
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | 1u);
    cpu->flags = cmp8(cpu->flags, 1u, count);
    cpu->sp = cpu->bp;
    cpu->bp = pop(ram, cpu);
    cpu->ip = pop(ram, cpu);
    cpu->cs = pop(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 0x0Cu);
    return 0;
}
