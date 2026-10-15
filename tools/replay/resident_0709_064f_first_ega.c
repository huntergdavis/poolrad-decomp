#include "resident_0709_064f_first_ega.h"

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

static uint16_t parity(uint16_t result) {
    uint8_t byte = (uint8_t)result;
    unsigned i;
    for (i = 0u; i < 4u; ++i) byte ^= (uint8_t)(byte >> (1u << i));
    return (byte & 1u) ? 0u : 0x0004u;
}

static uint16_t logic8(uint16_t old, uint8_t result) {
    uint16_t bits = parity(result);
    if (result == 0u) bits |= 0x0040u;
    if (result & 0x80u) bits |= 0x0080u;
    return (uint16_t)((old & (uint16_t)~0x08C5u) | bits);
}

static uint16_t cmp8(uint16_t old, uint8_t lhs, uint8_t rhs) {
    uint8_t result = (uint8_t)(lhs - rhs);
    uint16_t bits = parity(result);
    if (lhs < rhs) bits |= 0x0001u;
    if (((lhs ^ rhs ^ result) & 0x10u) != 0u) bits |= 0x0010u;
    if (result == 0u) bits |= 0x0040u;
    if (result & 0x80u) bits |= 0x0080u;
    if (((lhs ^ rhs) & (lhs ^ result) & 0x80u) != 0u) bits |= 0x0800u;
    return (uint16_t)((old & (uint16_t)~0x08D5u) | bits);
}

static uint16_t sub8(uint16_t old, uint8_t lhs, uint8_t rhs) {
    return cmp8(old, lhs, rhs);
}

static uint16_t add16(uint16_t old, uint16_t lhs, uint16_t rhs) {
    uint32_t wide = (uint32_t)lhs + rhs;
    uint16_t result = (uint16_t)wide;
    uint16_t bits = parity(result);
    if (wide > 0xFFFFu) bits |= 0x0001u;
    if (((lhs ^ rhs ^ result) & 0x0010u) != 0u) bits |= 0x0010u;
    if (result == 0u) bits |= 0x0040u;
    if (result & 0x8000u) bits |= 0x0080u;
    if (((~(lhs ^ rhs)) & (lhs ^ result) & 0x8000u) != 0u)
        bits |= 0x0800u;
    return (uint16_t)((old & (uint16_t)~0x08D5u) | bits);
}

static void set_al(pr_1816_caller_cpu *cpu, uint8_t value) {
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

int pr_064f_first_glyph_entry(uint8_t *ram, size_t ram_size,
                              pr_1816_caller_cpu *cpu) {
    uint8_t count;
    if (ram == NULL || cpu == NULL || ram_size < PR_1816_CALLER_RAM_SIZE ||
        cpu->cs != 0x0709u || cpu->ip != 0x064Fu) return 1;
    count = rd8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x100u));
    set_al(cpu, count);
    wr8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x101u), count);
    wr8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x103u), count);
    set_al(cpu, 1u);
    cpu->flags = cmp8(cpu->flags, 1u, count);
    if (count == 0u) return 2;
    wr8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x102u), 1u);
    set_al(cpu, rd8(ram, cpu->ss, (uint16_t)(cpu->bp + 0x0Eu)));
    push(ram, cpu, cpu->ax);
    set_al(cpu, rd8(ram, cpu->ss, (uint16_t)(cpu->bp + 0x0Cu)));
    push(ram, cpu, cpu->ax);
    set_al(cpu, rd8(ram, cpu->ss, (uint16_t)(cpu->bp + 0x0Au)));
    push(ram, cpu, cpu->ax);
    set_al(cpu, 1u);
    push(ram, cpu, cpu->ax);
    set_al(cpu, 1u);
    cpu->ax &= 0x00FFu; /* XOR AH,AH */
    cpu->flags = logic8(cpu->flags, 0u);
    cpu->di = cpu->ax;
    set_al(cpu, rd8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x100u + cpu->di)));
    push(ram, cpu, cpu->ax);
    set_al(cpu, 1u);
    push(ram, cpu, cpu->ax);
    push(ram, cpu, cpu->cs);
    push(ram, cpu, 0x0694u);
    cpu->ip = 0x01DFu;
    return 0;
}

int pr_064f_first_ega_change(uint8_t *ram, size_t ram_size,
                              pr_1816_caller_cpu *cpu,
                              pr_064f_ega_report *report) {
    uint8_t ch, glyph, row, col, color, mask, font_byte;
    uint16_t font_off, font_seg, page_off, page_seg, draw_seg, first_sp;
    unsigned plane;
    if (ram == NULL || cpu == NULL || report == NULL ||
        ram_size < PR_1816_CALLER_RAM_SIZE ||
        cpu->cs != 0x0709u || cpu->ip != 0x01DFu) return 1;
    *report = (pr_064f_ega_report){0};
    push(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 0x12u);
    col = rd8(ram, cpu->ss, (uint16_t)(cpu->bp + 0x10u));
    cpu->flags = cmp8(cpu->flags, col, 0x27u);
    if (col > 0x27u) return 2;
    row = rd8(ram, cpu->ss, (uint16_t)(cpu->bp + 0x0Eu));
    cpu->flags = cmp8(cpu->flags, row, 0x18u);
    if (row > 0x18u) return 2;
    font_off = rd16(ram, cpu->ds, 0x5E20u);
    font_seg = rd16(ram, cpu->ds, 0x5E22u);
    cpu->ax = (uint16_t)(font_off | font_seg);
    if (cpu->ax == 0u ||
        rd8(ram, cpu->ss, (uint16_t)(cpu->bp + 6u)) != 1u) return 2;

    /* 0203..0218: SYSTEM 182A uppercases ASCII via its real far-call ABI. */
    ch = rd8(ram, cpu->ss, (uint16_t)(cpu->bp + 8u));
    set_al(cpu, ch);
    push(ram, cpu, cpu->ax);
    push(ram, cpu, cpu->cs);
    push(ram, cpu, 0x020Cu);
    cpu->cs = 0x0AF8u;
    cpu->ip = 0x182Au;
    first_sp = cpu->sp;
    cpu->bx = first_sp;
    set_al(cpu, rd8(ram, cpu->ss, (uint16_t)(first_sp + 4u)));
    ch = (uint8_t)cpu->ax;
    cpu->flags = cmp8(cpu->flags, ch, 0x61u);
    if (ch >= 0x61u) {
        cpu->flags = cmp8(cpu->flags, ch, 0x7Au);
        if (ch <= 0x7Au) {
            cpu->flags = sub8(cpu->flags, ch, 0x20u);
            set_al(cpu, (uint8_t)(ch - 0x20u));
        }
    }
    cpu->ip = pop(ram, cpu);
    cpu->cs = pop(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    cpu->ax &= 0x00FFu; /* XOR AH,AH */
    cpu->flags = logic8(cpu->flags, 0u);
    cpu->dx = 0u; /* CWD on zero-extended ASCII */
    cpu->cx = 0x40u;
    glyph = (uint8_t)(cpu->ax % cpu->cx); /* IDIV, XCHG DX,AX */
    cpu->dx = (uint16_t)(cpu->ax / cpu->cx);
    cpu->ax = glyph;
    wr8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), glyph);
    set_al(cpu, rd8(ram, cpu->ss, (uint16_t)(cpu->bp + 0x0Au)));
    wr8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x11u), (uint8_t)cpu->ax);
    set_al(cpu, 1u);
    cpu->flags = cmp8(cpu->flags, 1u,
                      rd8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x11u)));
    if (cpu->flags & 1u) return 2;
    wr8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u), 1u);
    set_al(cpu, rd8(ram, cpu->ds, 0x4A1Eu));
    cpu->flags = cmp8(cpu->flags, (uint8_t)cpu->ax, 0u);
    cpu->flags = cmp8(cpu->flags, (uint8_t)cpu->ax, 1u);
    if ((uint8_t)cpu->ax != 1u) return 2;

    cpu->ax = col;
    cpu->flags = logic8(cpu->flags, 0u);
    cpu->bx = cpu->ax;
    cpu->ax = row;
    cpu->flags = logic8(cpu->flags, 0u);
    cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | 3u);
    cpu->ax = (uint16_t)(cpu->ax << 3u);
    cpu->dx = 0x28u;
    cpu->ax = (uint16_t)(cpu->ax * cpu->dx);
    cpu->dx = 0u;
    page_off = (uint16_t)(cpu->ax + cpu->bx);
    cpu->flags = add16(cpu->flags, cpu->ax, cpu->bx);
    cpu->ax = page_off;
    wr16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u), page_off);
    set_al(cpu, 2u);
    cpu->dx = 0x03C4u; /* OUT index,AL */
    wr8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x0Fu), 0u);
    wr8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x0Cu), 0u);
    color = rd8(ram, cpu->ss, (uint16_t)(cpu->bp + 0x0Cu));
    page_seg = rd16(ram, cpu->ds, 0x4A16u);
    draw_seg = rd16(ram, cpu->ds, 0x4A18u);

    for (plane = 0u; plane < 4u; ++plane) {
        wr8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x0Cu), (uint8_t)plane);
        cpu->ax = (uint16_t)(3u - plane);
        cpu->dx = (uint16_t)plane;
        cpu->di = cpu->ax;
        mask = rd8(ram, cpu->ds, (uint16_t)(0x25C9u + cpu->di));
        set_al(cpu, mask);
        cpu->dx = 0x03C5u; /* OUT plane mask. */
        cpu->ax = (uint16_t)(3u - plane);
        cpu->dx = (uint16_t)plane;
        cpu->di = cpu->ax;
        set_al(cpu, mask);
        mask = (uint8_t)(mask & color);
        set_al(cpu, mask);
        cpu->flags = logic8(cpu->flags, mask);
        cpu->flags = logic8(cpu->flags, mask); /* OR AL,AL */
        if (mask == 0u) {
            /* This slice stops at the first value change. A preceding
             * cleared plane with a nonzero old byte needs another gate. */
            if (rd8(ram, page_seg, page_off) != 0u ||
                rd8(ram, draw_seg, page_off) != 0u) return 2;
            cpu->ax = page_seg;
            push(ram, cpu, cpu->ax);
            cpu->di = page_off;
            cpu->es = pop(ram, cpu);
            wr8(ram, cpu->es, cpu->di, 0u);
            cpu->ax = draw_seg;
            push(ram, cpu, cpu->ax);
            cpu->di = page_off;
            cpu->es = pop(ram, cpu);
            wr8(ram, cpu->es, cpu->di, 0u);
            report->stores = (uint8_t)(report->stores + 2u);
            continue;
        }
        cpu->ax = glyph;
        cpu->dx = 0u;
        cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | 3u);
        cpu->ax = (uint16_t)(cpu->ax << 3u);
        cpu->di = font_off;
        cpu->es = font_seg;
        cpu->flags = add16(cpu->flags, cpu->di, cpu->ax);
        cpu->di = (uint16_t)(cpu->di + cpu->ax);
        cpu->flags = add16(cpu->flags, cpu->di, cpu->dx);
        cpu->di = (uint16_t)(cpu->di + cpu->dx);
        font_byte = rd8(ram, cpu->es, cpu->di);
        cpu->dx = (uint16_t)((cpu->dx & 0xFF00u) | font_byte);
        cpu->ax = page_seg;
        push(ram, cpu, cpu->ax);
        cpu->di = page_off;
        cpu->es = pop(ram, cpu);
        if (rd8(ram, cpu->es, cpu->di) == font_byte) return 2;
        wr8(ram, cpu->es, cpu->di, font_byte);
        report->glyph_index = glyph;
        report->color = color;
        report->plane = (uint8_t)plane;
        report->stores = (uint8_t)(report->stores + 1u);
        report->page_offset = page_off;
        report->page_segment = page_seg;
        cpu->ip = 0x0415u;
        return 0;
    }
    return 2;
}
