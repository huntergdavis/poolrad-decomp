#include "poolrad/recovered/resident_464_loop_c6e.h"

#define ARITH_FLAGS 0x08d5u

static int address(const pr_slot_a_directory_state *s, uint16_t seg,
                   uint16_t off, size_t *out)
{
    size_t a = (((size_t)seg << 4) + off) & 0xfffffu;
    if (a >= s->ram_size) return 0;
    *out = a;
    return 1;
}

static int byte_at(const pr_slot_a_directory_state *s, uint16_t seg,
                   uint16_t off, uint8_t *out)
{
    size_t a;
    if (!address(s, seg, off, &a)) return 0;
    *out = s->ram[a];
    return 1;
}

static int word_at(const pr_slot_a_directory_state *s, uint16_t seg,
                   uint16_t off, uint16_t *out)
{
    uint8_t lo, hi;
    if (!byte_at(s, seg, off, &lo) ||
        !byte_at(s, seg, (uint16_t)(off + 1u), &hi)) return 0;
    *out = (uint16_t)(lo | ((uint16_t)hi << 8));
    return 1;
}

static int even_parity(uint8_t x)
{
    x ^= (uint8_t)(x >> 4);
    x ^= (uint8_t)(x >> 2);
    x ^= (uint8_t)(x >> 1);
    return !(x & 1u);
}

static uint16_t add_flags(uint16_t old, uint16_t a,
                          uint16_t b, uint16_t result)
{
    uint16_t f = (uint16_t)(old & (uint16_t)~ARITH_FLAGS);
    if ((unsigned)a + b > 0xffffu) f |= 0x0001u;
    if (even_parity((uint8_t)result)) f |= 0x0004u;
    if (((a ^ b ^ result) & 0x10u) != 0u) f |= 0x0010u;
    if (result == 0u) f |= 0x0040u;
    if (result & 0x8000u) f |= 0x0080u;
    if (((~(a ^ b) & (a ^ result)) & 0x8000u) != 0u) f |= 0x0800u;
    return f;
}

pr_r464_loop_status pr_r464_loop_c6e(pr_slot_a_directory_state *s)
{
    uint16_t counter, data_index, source_off, source_seg, next_counter, next_di;
    uint8_t remaining, selector, limit;
    size_t writes[6], later[5];
    unsigned i, j;

    if (!s || !s->ram || s->ram_size == 0u || s->ram_size > 0x100000u)
        return PR_R464_LOOP_MEMORY_BOUNDARY;
    if (s->cpu.cs != 0x0c88u || s->cpu.ip != 0x0c6eu)
        return PR_R464_LOOP_WRONG_ENTRY;

    /* +0C6E..+0C8A: counter/plane/data increments and two conditional
     * branches. Only the measured JNE/JNE path continues to +0C31. */
    if (!word_at(s, s->cpu.ss, (uint16_t)(s->cpu.bp - 8u), &counter) ||
        !byte_at(s, s->cpu.ss, (uint16_t)(s->cpu.bp - 0x11u), &remaining) ||
        !word_at(s, s->cpu.ss, (uint16_t)(s->cpu.bp - 0x0au), &data_index) ||
        !byte_at(s, s->cpu.ds, 0x70c7u, &selector) ||
        !byte_at(s, s->cpu.ss, (uint16_t)(s->cpu.bp - 0x19u), &limit))
        return PR_R464_LOOP_MEMORY_BOUNDARY;
    if ((uint8_t)(remaining - 1u) == 0u || selector == limit)
        return PR_R464_LOOP_OTHER_BRANCH;

    /* +0C31..+0C3D: increment selector, load source far pointer, and
     * add the incremented source index. Check all reached addresses first.
     * Reject real-mode aliases that would change a later read. */
    if (!word_at(s, s->cpu.ss, (uint16_t)(s->cpu.bp + 0x0au),
                 &source_off) ||
        !word_at(s, s->cpu.ss, (uint16_t)(s->cpu.bp + 0x0cu),
                 &source_seg) ||
        !address(s, s->cpu.ss, (uint16_t)(s->cpu.bp - 8u), &writes[0]) ||
        !address(s, s->cpu.ss, (uint16_t)(s->cpu.bp - 7u), &writes[1]) ||
        !address(s, s->cpu.ss, (uint16_t)(s->cpu.bp - 0x11u), &writes[2]) ||
        !address(s, s->cpu.ss, (uint16_t)(s->cpu.bp - 0x0au), &writes[3]) ||
        !address(s, s->cpu.ss, (uint16_t)(s->cpu.bp - 9u), &writes[4]) ||
        !address(s, s->cpu.ds, 0x70c7u, &writes[5]) ||
        !address(s, s->cpu.ss, (uint16_t)(s->cpu.bp - 0x19u), &later[0]) ||
        !address(s, s->cpu.ss, (uint16_t)(s->cpu.bp + 0x0au), &later[1]) ||
        !address(s, s->cpu.ss, (uint16_t)(s->cpu.bp + 0x0bu), &later[2]) ||
        !address(s, s->cpu.ss, (uint16_t)(s->cpu.bp + 0x0cu), &later[3]) ||
        !address(s, s->cpu.ss, (uint16_t)(s->cpu.bp + 0x0du), &later[4]))
        return PR_R464_LOOP_MEMORY_BOUNDARY;
    for (i = 0; i < 6u; ++i) {
        for (j = i + 1u; j < 6u; ++j)
            if (writes[i] == writes[j]) return PR_R464_LOOP_ALIAS_BOUNDARY;
        for (j = 0; j < 5u; ++j)
            if (writes[i] == later[j]) return PR_R464_LOOP_ALIAS_BOUNDARY;
    }
    next_counter = (uint16_t)(counter + 1u);
    next_di = (uint16_t)(source_off + next_counter);
    s->ram[writes[0]] = (uint8_t)next_counter;
    s->ram[writes[1]] = (uint8_t)(next_counter >> 8);
    s->ram[writes[2]] = (uint8_t)(remaining - 1u);
    ++data_index;
    s->ram[writes[3]] = (uint8_t)data_index;
    s->ram[writes[4]] = (uint8_t)(data_index >> 8);
    s->ram[writes[5]] = (uint8_t)(selector + 1u);
    s->cpu.ax = next_counter;
    s->cpu.di = next_di;
    s->cpu.es = source_seg;
    s->cpu.flags = add_flags(s->cpu.flags, source_off, next_counter, next_di);
    s->cpu.ip = 0x0c3du;
    return PR_R464_LOOP_NEXT_MASKED_BYTE;
}
