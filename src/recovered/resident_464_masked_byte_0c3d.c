#include "poolrad/recovered/resident_464_masked_byte_0c3d.h"

#define CF 0x0001u
#define PF 0x0004u
#define AF 0x0010u
#define ZF 0x0040u
#define SF 0x0080u
#define OF 0x0800u
#define ARITH_FLAGS (CF | PF | AF | ZF | SF | OF)

static int address(const pr_slot_a_directory_state *s, uint16_t seg,
                   uint16_t off, size_t *out)
{
    size_t linear = (((size_t)seg << 4) + off) & 0xfffffu;
    if (linear >= s->ram_size) return 0;
    *out = linear;
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

static unsigned even_parity(uint8_t x)
{
    x ^= (uint8_t)(x >> 4);
    x ^= (uint8_t)(x >> 2);
    x ^= (uint8_t)(x >> 1);
    return !(x & 1u);
}

static uint16_t add_flags(uint16_t old_flags, uint16_t a,
                          uint16_t b, uint16_t result)
{
    uint16_t f = (uint16_t)(old_flags & (uint16_t)~ARITH_FLAGS);
    if ((unsigned)a + b > 0xffffu) f |= CF;
    if (even_parity((uint8_t)result)) f |= PF;
    if (((a ^ b ^ result) & 0x10u) != 0u) f |= AF;
    if (result == 0u) f |= ZF;
    if (result & 0x8000u) f |= SF;
    if (((~(a ^ b) & (a ^ result)) & 0x8000u) != 0u) f |= OF;
    return f;
}

pr_r464_masked_byte_status pr_r464_masked_byte_0c3d(
    pr_slot_a_directory_state *s)
{
    uint16_t mask_offset, data_offset, src_off, src_seg;
    uint16_t mask_off, mask_seg, dest_off, dest_seg, final_di;
    uint8_t source, mask, old, result;
    size_t dest_address;

    if (!s || !s->ram || s->ram_size == 0u || s->ram_size > 0x100000u)
        return PR_R464_MASKED_BYTE_MEMORY_BOUNDARY;
    if (s->cpu.cs != 0x0c88u || s->cpu.ip != 0x0c3du)
        return PR_R464_MASKED_BYTE_WRONG_ENTRY;

    /* +0C3D..+0C4D: source byte and auxiliary-mask far pointer. LES reads
     * each far pointer as offset followed by segment. Offsets wrap at 16 bits. */
    if (!byte_at(s, s->cpu.es, (uint16_t)(s->cpu.di + 0x17u), &source) ||
        !word_at(s, s->cpu.ss, (uint16_t)(s->cpu.bp - 0x10u), &mask_offset) ||
        !word_at(s, s->cpu.ss, (uint16_t)(s->cpu.bp + 0x0au), &src_off) ||
        !word_at(s, s->cpu.ss, (uint16_t)(s->cpu.bp + 0x0cu), &src_seg) ||
        !word_at(s, src_seg, (uint16_t)(src_off + 0x13u), &mask_off) ||
        !word_at(s, src_seg, (uint16_t)(src_off + 0x15u), &mask_seg) ||
        !byte_at(s, mask_seg, (uint16_t)(mask_off + mask_offset), &mask))
        return PR_R464_MASKED_BYTE_MEMORY_BOUNDARY;

    /* +0C50..+0C6A: destination far pointer, masked merge and one store. */
    if (!word_at(s, s->cpu.ss, (uint16_t)(s->cpu.bp - 0x0au), &data_offset) ||
        !word_at(s, s->cpu.ss, (uint16_t)(s->cpu.bp + 6u), &dest_off) ||
        !word_at(s, s->cpu.ss, (uint16_t)(s->cpu.bp + 8u), &dest_seg))
        return PR_R464_MASKED_BYTE_MEMORY_BOUNDARY;
    final_di = (uint16_t)(dest_off + data_offset);
    if (!byte_at(s, dest_seg, (uint16_t)(final_di + 0x17u), &old) ||
        !address(s, dest_seg, (uint16_t)(final_di + 0x17u), &dest_address))
        return PR_R464_MASKED_BYTE_MEMORY_BOUNDARY;
    result = (uint8_t)((old & mask) | source);

    s->ram[dest_address] = result;
    s->cpu.cx = (uint16_t)((s->cpu.cx & 0xff00u) | source);
    s->cpu.dx = (uint16_t)((s->cpu.dx & 0xff00u) | result);
    s->cpu.ax = data_offset;
    s->cpu.di = final_di;
    s->cpu.es = dest_seg;
    /* The final +0C68 ADD DI,AX owns the flags at the +0C6E stop. */
    s->cpu.flags = add_flags(s->cpu.flags, dest_off, data_offset, final_di);
    s->cpu.ip = 0x0c6eu;
    return PR_R464_MASKED_BYTE_AT_0C6E;
}
