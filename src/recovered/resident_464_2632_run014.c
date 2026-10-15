#include "poolrad/recovered/resident_464_2632_run014.h"

enum {
    CF = 0x0001u, PF = 0x0004u, AF = 0x0010u,
    ZF = 0x0040u, SF = 0x0080u, OF = 0x0800u,
    ARITH = CF | PF | AF | ZF | SF | OF
};

static uint32_t physical(uint16_t segment, uint16_t offset)
{
    return (((uint32_t)segment << 4) + offset) & 0xFFFFFu;
}

static uint8_t byte_at(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return ram[physical(segment, offset)];
}

static void put_byte(uint8_t *ram, uint16_t segment, uint16_t offset,
                     uint8_t value)
{
    ram[physical(segment, offset)] = value;
}

static uint16_t word_at(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return (uint16_t)(byte_at(ram, segment, offset) |
        ((uint16_t)byte_at(ram, segment, (uint16_t)(offset + 1u)) << 8));
}

static void put_word(uint8_t *ram, uint16_t segment, uint16_t offset,
                     uint16_t value)
{
    put_byte(ram, segment, offset, (uint8_t)value);
    put_byte(ram, segment, (uint16_t)(offset + 1u), (uint8_t)(value >> 8));
}

static uint16_t local(const uint8_t *ram, const pr_r464_2632_cpu *cpu,
                      uint16_t delta)
{
    return word_at(ram, cpu->ss, (uint16_t)(cpu->bp + delta));
}

static void set_local(uint8_t *ram, const pr_r464_2632_cpu *cpu,
                      uint16_t delta, uint16_t value)
{
    put_word(ram, cpu->ss, (uint16_t)(cpu->bp + delta), value);
}

static void push(uint8_t *ram, pr_r464_2632_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    put_word(ram, cpu->ss, cpu->sp, value);
}

static uint8_t even_parity(uint8_t value)
{
    unsigned bits = 0;
    while (value != 0u) {
        bits ^= value & 1u;
        value >>= 1;
    }
    return (uint8_t)(bits == 0u);
}

static uint16_t sub_flags16(uint16_t flags, uint16_t left, uint16_t right)
{
    uint16_t result = (uint16_t)(left - right);
    flags &= (uint16_t)~ARITH;
    if (left < right) flags |= CF;
    if (even_parity((uint8_t)result)) flags |= PF;
    if (((left ^ right ^ result) & 0x10u) != 0u) flags |= AF;
    if (result == 0u) flags |= ZF;
    if ((result & 0x8000u) != 0u) flags |= SF;
    if (((left ^ right) & (left ^ result) & 0x8000u) != 0u) flags |= OF;
    return flags;
}

static uint16_t sub_flags8(uint16_t flags, uint8_t left, uint8_t right)
{
    uint8_t result = (uint8_t)(left - right);
    flags &= (uint16_t)~ARITH;
    if (left < right) flags |= CF;
    if (even_parity(result)) flags |= PF;
    if (((left ^ right ^ result) & 0x10u) != 0u) flags |= AF;
    if (result == 0u) flags |= ZF;
    if ((result & 0x80u) != 0u) flags |= SF;
    if (((left ^ right) & (left ^ result) & 0x80u) != 0u) flags |= OF;
    return flags;
}

static uint16_t add_flags16(uint16_t flags, uint16_t left, uint16_t right)
{
    uint32_t full = (uint32_t)left + right;
    uint16_t result = (uint16_t)full;
    flags &= (uint16_t)~ARITH;
    if (full > 0xFFFFu) flags |= CF;
    if (even_parity((uint8_t)result)) flags |= PF;
    if (((left ^ right ^ result) & 0x10u) != 0u) flags |= AF;
    if (result == 0u) flags |= ZF;
    if ((result & 0x8000u) != 0u) flags |= SF;
    if (((~(left ^ right)) & (left ^ result) & 0x8000u) != 0u) flags |= OF;
    return flags;
}

static void observe(uint16_t ip, const uint8_t *ram, pr_r464_2632_cpu *cpu,
                    pr_r464_2632_observer observer, void *context)
{
    cpu->ip = ip;
    if (observer != NULL) observer(ip, ram, cpu, context);
}

/* The first call uses Pascal Move at original 0AF8:0A3F. The row loop
 * uses the overlap-aware entry at original 0AF8:1685. Both include
 * call/return stack residue. The earlier local audit verified +0A3F. */
static void runtime_move(uint8_t *ram, pr_r464_2632_cpu *cpu,
                         uint16_t next_ip, uint8_t overlap_aware)
{
    uint16_t saved_ds, return_cs, return_ip;
    push(ram, cpu, cpu->cs);
    push(ram, cpu, next_ip);
    cpu->cs = 0x131Cu;
    cpu->ip = overlap_aware ? 0x1685u : 0x0A3Fu;
    cpu->bx = cpu->sp;
    cpu->dx = cpu->ds;
    saved_ds = cpu->ds;
    cpu->si = word_at(ram, cpu->ss, (uint16_t)(cpu->bx + 0x0Au));
    cpu->ds = word_at(ram, cpu->ss, (uint16_t)(cpu->bx + 0x0Cu));
    cpu->di = word_at(ram, cpu->ss, (uint16_t)(cpu->bx + 6u));
    cpu->es = word_at(ram, cpu->ss, (uint16_t)(cpu->bx + 8u));
    cpu->cx = word_at(ram, cpu->ss, (uint16_t)(cpu->bx + 4u));
    cpu->flags &= (uint16_t)~0x0400u;
    if (overlap_aware)
        cpu->flags = sub_flags16(cpu->flags, cpu->si, cpu->di);
    while (cpu->cx != 0u) {
        put_byte(ram, cpu->es, cpu->di, byte_at(ram, cpu->ds, cpu->si));
        cpu->si = (uint16_t)(cpu->si + 1u);
        cpu->di = (uint16_t)(cpu->di + 1u);
        cpu->cx = (uint16_t)(cpu->cx - 1u);
    }
    cpu->ds = saved_ds;
    return_ip = word_at(ram, cpu->ss, cpu->sp);
    return_cs = word_at(ram, cpu->ss, (uint16_t)(cpu->sp + 2u));
    cpu->sp = (uint16_t)(cpu->sp + 0x0Eu);
    cpu->ip = return_ip;
    cpu->cs = return_cs;
}

/* Pascal FillChar at original 0AF8:16A8. The reached body writes one byte
 * repeatedly through ES:DI and returns with RETF 8. */
static void runtime_fill(uint8_t *ram, pr_r464_2632_cpu *cpu)
{
    uint16_t return_cs, return_ip;
    uint8_t value;
    push(ram, cpu, cpu->cs);
    push(ram, cpu, 0x2921u);
    cpu->cs = 0x131Cu;
    cpu->ip = 0x16A8u;
    cpu->bx = cpu->sp;
    cpu->di = word_at(ram, cpu->ss, (uint16_t)(cpu->bx + 8u));
    cpu->es = word_at(ram, cpu->ss, (uint16_t)(cpu->bx + 0x0Au));
    cpu->cx = word_at(ram, cpu->ss, (uint16_t)(cpu->bx + 6u));
    value = (uint8_t)word_at(ram, cpu->ss, (uint16_t)(cpu->bx + 4u));
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
    cpu->flags &= (uint16_t)~0x0400u;
    while (cpu->cx != 0u) {
        put_byte(ram, cpu->es, cpu->di, value);
        cpu->di = (uint16_t)(cpu->di + 1u);
        cpu->cx = (uint16_t)(cpu->cx - 1u);
    }
    return_ip = word_at(ram, cpu->ss, cpu->sp);
    return_cs = word_at(ram, cpu->ss, (uint16_t)(cpu->sp + 2u));
    cpu->sp = (uint16_t)(cpu->sp + 0x0Cu);
    cpu->ip = return_ip;
    cpu->cs = return_cs;
}

pr_r464_2632_status pr_r464_2632_run014(
    uint8_t *ram, size_t ram_size, pr_r464_2632_cpu *cpu,
    pr_r464_2632_observer observer, void *context)
{
    uint16_t bp, object_off, object_seg, source_off, source_seg;
    uint16_t mask, count, row, x, width, height, row_slot;
    uint16_t pixel_start, pixel_end, row_base, tail;
    uint16_t counter, i, return_ip, return_cs;

    if (ram == NULL || cpu == NULL || ram_size < PR_R464_2632_RAM_BYTES)
        return PR_R464_2632_INVALID_HOST;
    if (cpu->cs != 0x0C88u || cpu->ip != 0x2632u || cpu->sp < 0x40u)
        return PR_R464_2632_UNSUPPORTED_PATH;
    bp = (uint16_t)(cpu->sp - 2u);
    object_off = word_at(ram, cpu->ss, (uint16_t)(bp + 6u));
    object_seg = word_at(ram, cpu->ss, (uint16_t)(bp + 8u));
    source_off = word_at(ram, cpu->ss, (uint16_t)(bp + 0x0Au));
    source_seg = word_at(ram, cpu->ss, (uint16_t)(bp + 0x0Cu));
    mask = word_at(ram, cpu->ss, (uint16_t)(bp + 0x0Eu));
    count = word_at(ram, cpu->ss, (uint16_t)(bp + 0x12u));
    row = word_at(ram, cpu->ss, (uint16_t)(bp + 0x14u));
    x = word_at(ram, cpu->ss, (uint16_t)(bp + 0x16u));
    width = word_at(ram, object_seg, (uint16_t)(object_off + 2u));
    height = word_at(ram, object_seg, object_off);
    row_slot = (uint16_t)(row * 2u);
    row_base = (uint16_t)(row * width);
    pixel_start = (uint16_t)((row_base + x) * 4u);
    pixel_end = (uint16_t)(pixel_start + count * 4u - 1u);
    tail = (uint16_t)((width - count - x) * 4u);
    /* These guards describe the directly observed branch successors. They
     * run before the first push, leaving unsupported paths untouched. */
    if ((object_off | object_seg) == 0u ||
        word_at(ram, cpu->ss, (uint16_t)(bp + 0x10u)) != 1u ||
        row > 255u || x == 0u || x > 0x7FFFu ||
        count == 0u || count > 64u ||
        (uint32_t)row + 1u > height ||
        (uint32_t)x + count > width ||
        (uint32_t)(row * (uint32_t)width + x + count) * 4u > 0xFFFFu ||
        (uint32_t)object_off + pixel_start + 0x17u +
            ((uint32_t)count - 1u) * 4u >
            (uint16_t)(bp - 0x1Cu) ||
        byte_at(ram, cpu->ds, 0x4A1Eu) != 1u ||
        word_at(ram, cpu->ds, (uint16_t)(0x4C20u + row_slot)) <= pixel_start ||
        word_at(ram, cpu->ds, (uint16_t)(0x4D70u + row_slot)) >= pixel_end)
        return PR_R464_2632_UNSUPPORTED_PATH;
    for (i = 0; i < 4u; ++i) {
        if ((byte_at(ram, cpu->ds, (uint16_t)(0x25F6u + i)) & mask) == 0u)
            return PR_R464_2632_UNSUPPORTED_PATH;
    }

    observe(0x2632u, ram, cpu, observer, context);
    /* +2632..+2646: frame and source-to-local eight-byte Move arguments. */
    push(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->flags = sub_flags16(cpu->flags, cpu->sp, 0x24u);
    cpu->sp = (uint16_t)(cpu->sp - 0x24u);
    cpu->es = source_seg;
    cpu->di = source_off;
    push(ram, cpu, cpu->es);
    push(ram, cpu, cpu->di);
    cpu->di = (uint16_t)(cpu->bp - 8u);
    push(ram, cpu, cpu->ss);
    push(ram, cpu, cpu->di);
    cpu->ax = 8u;
    push(ram, cpu, cpu->ax);
    observe(0x2646u, ram, cpu, observer, context);
    runtime_move(ram, cpu, 0x264Bu, 0u);
    observe(0x264Bu, ram, cpu, observer, context);

    /* Measured positive-X and nonnegative-row route, +264B..+28E0. */
    set_local(ram, cpu, (uint16_t)-0x0A, 0u);
    set_local(ram, cpu, (uint16_t)-0x0E, x);
    cpu->bx = 0u;
    cpu->cx = (uint16_t)(count + x);
    cpu->dx = 0u;
    cpu->di = object_off;
    cpu->es = object_seg;
    set_local(ram, cpu, (uint16_t)-0x0C, 0u);
    set_local(ram, cpu, (uint16_t)-0x10,
              (uint16_t)(width - count - x));
    set_local(ram, cpu, (uint16_t)-0x1E, row);
    set_local(ram, cpu, (uint16_t)-0x16, row_base);
    set_local(ram, cpu, (uint16_t)-0x14, 0u);
    cpu->cx = (uint16_t)(row + 1u);
    cpu->ax = 1u;
    cpu->flags = sub_flags8(cpu->flags, 1u, 1u);
    observe(0x28E0u, ram, cpu, observer, context);

    /* +28E0..+291C: scale the row/column locals and push FillChar args. */
    set_local(ram, cpu, (uint16_t)-0x20, x);
    set_local(ram, cpu, (uint16_t)-0x12, count);
    set_local(ram, cpu, (uint16_t)-0x0E, (uint16_t)(x * 4u));
    set_local(ram, cpu, (uint16_t)-0x10, tail);
    set_local(ram, cpu, (uint16_t)-0x16,
              (uint16_t)(row_base * 4u));
    cpu->flags = add_flags16(cpu->flags, (uint16_t)(row_base * 2u),
                              (uint16_t)(row_base * 2u));
    cpu->di = (uint16_t)(cpu->bp - 0x1Cu);
    push(ram, cpu, cpu->ss);
    push(ram, cpu, cpu->di);
    push(ram, cpu, 4u);
    cpu->ax = 0u;
    push(ram, cpu, cpu->ax);
    observe(0x291Cu, ram, cpu, observer, context);
    runtime_fill(ram, cpu);
    observe(0x2921u, ram, cpu, observer, context);

    /* +2921..+294D: four table/mask tests produce the fill pattern. */
    put_byte(ram, cpu->ss, (uint16_t)(cpu->bp - 0x21u), 0u);
    for (i = 0; i < 4u; ++i) {
        put_byte(ram, cpu->ss, (uint16_t)(cpu->bp - 0x21u), (uint8_t)i);
        put_byte(ram, cpu->ss,
                 (uint16_t)(cpu->bp - 0x1Cu + i), 0xFFu);
    }
    cpu->ax = 3u;
    cpu->di = 3u;
    cpu->flags = sub_flags8(cpu->flags, 3u, 3u);
    observe(0x294Du, ram, cpu, observer, context);

    /* +294D..+2999: one reached row and its dirty metadata. */
    set_local(ram, cpu, (uint16_t)-0x23, 0u);
    set_local(ram, cpu, (uint16_t)-0x18, 0u);
    put_byte(ram, cpu->ds, (uint16_t)(0x4A28u + row), 1u);
    set_local(ram, cpu, (uint16_t)-0x16, pixel_start);
    put_word(ram, cpu->ds, (uint16_t)(0x4AD0u + row_slot), x);
    put_word(ram, cpu->ds, (uint16_t)(0x4C20u + row_slot), pixel_start);
    cpu->di = row_slot;
    cpu->ax = pixel_start;
    cpu->flags = add_flags16(cpu->flags, row, row);
    observe(0x2999u, ram, cpu, observer, context);

    /* +2999..+29D7: one byte-counted row of four-byte runtime Moves. */
    put_byte(ram, cpu->ss, (uint16_t)(cpu->bp - 0x24u), (uint8_t)count);
    counter = 1u;
    for (;;) {
        put_byte(ram, cpu->ds, 0x70C7u, (uint8_t)counter);
        cpu->di = (uint16_t)(cpu->bp - 0x1Cu);
        push(ram, cpu, cpu->ss);
        push(ram, cpu, cpu->di);
        cpu->ax = local(ram, cpu, (uint16_t)-0x16);
        cpu->di = (uint16_t)(object_off + cpu->ax);
        cpu->es = object_seg;
        cpu->flags = add_flags16(cpu->flags, cpu->di, 0x17u);
        cpu->di = (uint16_t)(cpu->di + 0x17u);
        push(ram, cpu, cpu->es);
        push(ram, cpu, cpu->di);
        cpu->ax = 4u;
        push(ram, cpu, cpu->ax);
        if (counter == 1u) observe(0x29C6u, ram, cpu, observer, context);
        runtime_move(ram, cpu, 0x29CBu, 1u);
        if (counter == 1u) observe(0x29CBu, ram, cpu, observer, context);
        set_local(ram, cpu, (uint16_t)-0x16,
                  (uint16_t)(local(ram, cpu, (uint16_t)-0x16) + 4u));
        cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | (uint8_t)counter);
        cpu->flags = sub_flags8(cpu->flags, (uint8_t)counter, (uint8_t)count);
        if (counter == count) break;
        ++counter;
    }
    observe(0x29D7u, ram, cpu, observer, context);

    /* +29D7..+2A07: upper bound, row/tail advance, equal outer counters. */
    put_word(ram, cpu->ds, (uint16_t)(0x4D70u + row_slot), pixel_end);
    set_local(ram, cpu, (uint16_t)-0x16,
              (uint16_t)(local(ram, cpu, (uint16_t)-0x16) + tail));
    set_local(ram, cpu, (uint16_t)-0x1E, (uint16_t)(row + 1u));
    cpu->di = row_slot;
    cpu->ax = local(ram, cpu, (uint16_t)-0x18);
    cpu->flags = sub_flags16(cpu->flags, cpu->ax,
                              local(ram, cpu, (uint16_t)-0x23));
    observe(0x2A02u, ram, cpu, observer, context);
    observe(0x2A07u, ram, cpu, observer, context);

    /* +2A07 MOV SP,BP; POP BP; RETF 12h to overlay +01C3. */
    cpu->sp = cpu->bp;
    cpu->bp = word_at(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return_ip = word_at(ram, cpu->ss, cpu->sp);
    return_cs = word_at(ram, cpu->ss, (uint16_t)(cpu->sp + 2u));
    cpu->sp = (uint16_t)(cpu->sp + 0x16u);
    cpu->cs = return_cs;
    observe(return_ip, ram, cpu, observer, context);
    return PR_R464_2632_OK;
}
