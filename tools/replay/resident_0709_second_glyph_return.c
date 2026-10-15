#include "resident_0709_second_glyph_return.h"

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

static void set_al(pr_1816_caller_cpu *cpu, uint8_t value) {
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
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

int pr_01df_second_glyph_return(uint8_t *ram, size_t ram_size,
                                  pr_1816_caller_cpu *cpu) {
    uint16_t frame, font_off, font_seg, page_seg, draw_seg, offset;
    uint8_t ch, glyph, count, row, col, color;
    unsigned scanline, plane;
    if (ram == NULL || cpu == NULL || ram_size < PR_1816_CALLER_RAM_SIZE ||
        cpu->cs != 0x0709u || cpu->ip != 0x01DFu) return 1;

    frame = (uint16_t)(cpu->sp - 2u);
    col = rd8(ram, cpu->ss, (uint16_t)(frame + 0x10u));
    row = rd8(ram, cpu->ss, (uint16_t)(frame + 0x0Eu));
    font_off = rd16(ram, cpu->ds, 0x5E20u);
    font_seg = rd16(ram, cpu->ds, 0x5E22u);
    count = rd8(ram, cpu->ss, (uint16_t)(frame + 0x0Au));
    if (col > 39u || row > 24u || (font_off | font_seg) == 0u ||
        rd8(ram, cpu->ss, (uint16_t)(frame + 6u)) != 1u ||
        count != 1u || rd8(ram, cpu->ds, 0x4A1Eu) != 1u) return 2;

    push(ram, cpu, cpu->bp);
    cpu->bp = frame;
    cpu->sp = (uint16_t)(cpu->sp - 0x12u);

    /* 0203..0215, including the original SYSTEM 182A far-call frame. */
    ch = rd8(ram, cpu->ss, (uint16_t)(frame + 8u));
    set_al(cpu, ch);
    push(ram, cpu, cpu->ax);
    push(ram, cpu, cpu->cs);
    push(ram, cpu, 0x020Cu);
    cpu->bx = cpu->sp;
    if (ch >= (uint8_t)'a' && ch <= (uint8_t)'z')
        ch = (uint8_t)(ch - 0x20u);
    set_al(cpu, ch);
    cpu->ip = pop(ram, cpu);
    cpu->cs = pop(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    cpu->ax &= 0x00FFu;
    cpu->dx = (uint16_t)(cpu->ax / 0x40u);
    glyph = (uint8_t)(cpu->ax % 0x40u);
    cpu->ax = glyph;
    wr8(ram, cpu->ss, (uint16_t)(frame - 2u), glyph);
    set_al(cpu, count);
    wr8(ram, cpu->ss, (uint16_t)(frame - 0x11u), count);
    set_al(cpu, 1u);
    wr8(ram, cpu->ss, (uint16_t)(frame - 1u), 1u);

    /* 038E..0471: flat-memory result of the four EGA plane-mask passes. */
    cpu->bx = col;
    cpu->cx = 3u;
    offset = (uint16_t)((uint16_t)row * 320u + col);
    wr16(ram, cpu->ss, (uint16_t)(frame - 4u), offset);
    cpu->dx = 0x03C4u;
    wr8(ram, cpu->ss, (uint16_t)(frame - 0x0Fu), 0u);
    color = rd8(ram, cpu->ss, (uint16_t)(frame + 0x0Cu));
    page_seg = rd16(ram, cpu->ds, 0x4A16u);
    draw_seg = rd16(ram, cpu->ds, 0x4A18u);
    for (scanline = 0u; scanline < 8u; ++scanline) {
        wr8(ram, cpu->ss, (uint16_t)(frame - 0x0Fu), (uint8_t)scanline);
        for (plane = 0u; plane < 4u; ++plane) {
            uint8_t mask, value;
            wr8(ram, cpu->ss, (uint16_t)(frame - 0x0Cu), (uint8_t)plane);
            mask = rd8(ram, cpu->ds, (uint16_t)(0x25C9u + 3u - plane));
            value = (mask & color) ? rd8(ram, font_seg,
                (uint16_t)(font_off + (uint16_t)glyph * 8u + scanline)) : 0u;
            cpu->ax = page_seg;
            push(ram, cpu, cpu->ax);
            cpu->di = offset;
            cpu->es = pop(ram, cpu);
            wr8(ram, cpu->es, cpu->di, value);
            cpu->ax = draw_seg;
            push(ram, cpu, cpu->ax);
            cpu->di = offset;
            cpu->es = pop(ram, cpu);
            wr8(ram, cpu->es, cpu->di, value);
        }
        offset = (uint16_t)(offset + 40u);
        wr16(ram, cpu->ss, (uint16_t)(frame - 4u), offset);
    }
    cpu->dx = 0x03C5u;
    wr8(ram, cpu->ss, (uint16_t)(frame + 0x10u), (uint8_t)(col + 1u));
    set_al(cpu, 1u);
    cpu->flags = cmp8(cpu->flags, 1u, count);
    cpu->sp = cpu->bp;
    cpu->bp = pop(ram, cpu);
    cpu->ip = pop(ram, cpu);
    cpu->cs = pop(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 0x0Cu);
    return 0;
}

int pr_0694_final_printer_return(uint8_t *ram, size_t ram_size,
                                  pr_1816_caller_cpu *cpu) {
    uint8_t index, count, column;
    if (ram == NULL || cpu == NULL || ram_size < PR_1816_CALLER_RAM_SIZE ||
        cpu->cs != 0x0709u || cpu->ip != 0x0694u) return 1;
    index = rd8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x102u));
    count = rd8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x103u));
    if (index != count || count != 2u) return 2;
    column = rd8(ram, cpu->ss, (uint16_t)(cpu->bp + 0x0Eu));
    wr8(ram, cpu->ss, (uint16_t)(cpu->bp + 0x0Eu),
        (uint8_t)(column + 1u));
    set_al(cpu, index);
    cpu->flags = cmp8(cpu->flags, index, count);
    cpu->sp = cpu->bp;
    cpu->bp = pop(ram, cpu);
    cpu->ip = pop(ram, cpu);
    cpu->cs = pop(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 0x0Au);
    return 0;
}
