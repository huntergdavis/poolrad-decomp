/* GAME.OVR unit 0x33:+012B..+01AC; docs/re/U33_CELL_LOOKUP_012B.md. */
#include "poolrad/recovered/overlay_u33_cell_lookup_012b.h"
#include "poolrad/recovered/overlay_u3bd_lookup_fullstate.h"

#include <string.h>

#define MASK (PR_U33_012B_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u
#define PARTY_Y 0x6AAEu
#define FIELD_SITE 0x2Au    /* +0155: LCALL 03D5:002F */
#define NIBBLE_SITE 0x45u   /* +0170: LCALL 03D5:002A */
#define FIELD_STUB 0x2Fu
#define NIBBLE_STUB 0x2Au

_Static_assert(sizeof(pr_u33_012b_cpu) == sizeof(pr_u3bd_lookup_cpu),
               "u3BD lookup CPU state layout differs");

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint8_t read8(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return ram[physical(segment, offset)];
}

static void write8(uint8_t *ram, uint16_t segment, uint16_t offset, uint8_t value)
{
    ram[physical(segment, offset)] = value;
}

static uint16_t read16_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static uint16_t read16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return read16_at(ram, physical(segment, offset));
}

static void write16(uint8_t *ram, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static void push16(uint8_t *ram, pr_u33_012b_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static void set_al(pr_u33_012b_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static uint8_t arg(const uint8_t *ram, const pr_u33_012b_cpu *cpu, uint16_t forward)
{
    return read8(ram, cpu->ss, (uint16_t)(cpu->bp + forward));
}

static unsigned even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

static void cmp8(pr_u33_012b_cpu *cpu, uint8_t left, uint8_t right)
{
    uint8_t result = (uint8_t)(left - right);
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (left < right) cpu->flags |= 0x0001u;
    if (even_parity(result)) cpu->flags |= 0x0004u;
    if ((left ^ right ^ result) & 0x10u) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x80u) cpu->flags |= 0x0080u;
    if ((left ^ right) & (left ^ result) & 0x80u) cpu->flags |= 0x0800u;
}

/* OR AL,AL: CF/OF clear, undefined AF represented as zero. */
static void or8(pr_u33_012b_cpu *cpu, uint8_t result)
{
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (even_parity(result)) cpu->flags |= 0x0004u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x80u) cpu->flags |= 0x0080u;
}

/* Push x, y and facing as the live AX words (AH is whatever it was). */
static void push_arguments(uint8_t *ram, pr_u33_012b_cpu *cpu)
{
    set_al(cpu, arg(ram, cpu, 0x0Cu));
    push16(ram, cpu, cpu->ax);
    set_al(cpu, arg(ram, cpu, 0x0Au));
    push16(ram, cpu, cpu->ax);
    set_al(cpu, arg(ram, cpu, 0x08u));
    push16(ram, cpu, cpu->ax);
}

/* LCALL through a loaded overlay stub. The site holds 9A, the stub
 * offset word and the relocated segment word; the stub holds EA, the
 * callee offset and the callee segment. */
static pr_u33_012b_status far_call(uint8_t *ram, size_t ram_size, pr_u33_012b_cpu *cpu,
                                   uint16_t entry, uint16_t site, uint16_t stub_offset,
                                   pr_u3bd_lookup_status (*callee)(uint8_t *, size_t, pr_u3bd_lookup_cpu *))
{
    uint16_t caller_cs = cpu->cs;
    uint16_t return_ip = (uint16_t)(entry + site + 5u);
    uint16_t stub_segment;
    pr_u3bd_lookup_cpu leaf;
    pr_u3bd_lookup_status status;
    if (read8(ram, cpu->cs, (uint16_t)(entry + site)) != 0x9Au ||
        read16(ram, cpu->cs, (uint16_t)(entry + site + 1u)) != stub_offset)
        return PR_U33_012B_CODE;
    stub_segment = read16(ram, cpu->cs, (uint16_t)(entry + site + 3u));
    if (read8(ram, stub_segment, stub_offset) != 0xEAu)
        return PR_U33_012B_CODE;
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, return_ip);
    cpu->ip = read16(ram, stub_segment, (uint16_t)(stub_offset + 1u));
    cpu->cs = read16(ram, stub_segment, (uint16_t)(stub_offset + 3u));
    memcpy(&leaf, cpu, sizeof leaf);
    status = callee(ram, ram_size, &leaf);
    memcpy(cpu, &leaf, sizeof leaf);
    if (status == PR_U3BD_LOOKUP_ARGUMENT)
        return PR_U33_012B_ARGUMENT;
    if (status == PR_U3BD_LOOKUP_TRANSFER || cpu->ip != return_ip || cpu->cs != caller_cs)
        return PR_U33_012B_TRANSFER;
    return PR_U33_012B_OK;
}

static void leave(uint8_t *ram, pr_u33_012b_cpu *cpu)
{
    size_t at;
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u)));
    cpu->sp = cpu->bp;
    cpu->bp = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 12u);
}

pr_u33_012b_status pr_u33_cell_lookup_012b(
    uint8_t *ram, size_t ram_size, pr_u33_012b_cpu *cpu)
{
    uint16_t entry;
    uint8_t value;
    pr_u33_012b_status status;
    if (ram == NULL || cpu == NULL || ram_size < PR_U33_012B_RAM_SIZE)
        return PR_U33_012B_ARGUMENT;
    entry = cpu->ip;
    /* +012B..+012E */
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    /* +0131..+0147: four signed range compares on the live x and y bytes. */
    value = arg(ram, cpu, 0x0Cu);
    cmp8(cpu, value, 0u);
    if ((int8_t)value >= 0) {
        value = arg(ram, cpu, 0x0Cu);
        cmp8(cpu, value, 0x0Fu);
        if ((int8_t)value <= 15) {
            value = arg(ram, cpu, 0x0Au);
            cmp8(cpu, value, 0u);
            if ((int8_t)value >= 0) {
                value = arg(ram, cpu, 0x0Au);
                cmp8(cpu, value, 0x0Fu);
                if ((int8_t)value <= 15) {
                    /* +0149..+015C: entry 3 on x, y, facing; zero field returns 1. */
                    push_arguments(ram, cpu);
                    status = far_call(ram, ram_size, cpu, entry, FIELD_SITE, FIELD_STUB,
                                      pr_u3bd_field_0350_fullstate);
                    if (status != PR_U33_012B_OK)
                        return status;
                    or8(cpu, (uint8_t)cpu->ax);
                    if ((uint8_t)cpu->ax == 0u) {
                        write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u), 1u);   /* +015E */
                        leave(ram, cpu);
                        return PR_U33_012B_OK;
                    }
                    /* +0164..+0177: entry 2 on x, y, facing; zero nibble 0, else 3. */
                    push_arguments(ram, cpu);
                    status = far_call(ram, ram_size, cpu, entry, NIBBLE_SITE, NIBBLE_STUB,
                                      pr_u3bd_nibble_047b_fullstate);
                    if (status != PR_U33_012B_OK)
                        return status;
                    or8(cpu, (uint8_t)cpu->ax);
                    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u), (uint8_t)cpu->ax == 0u ? 0u : 3u);
                    leave(ram, cpu);
                    return PR_U33_012B_OK;
                }
            }
        }
    }
    /* +0185..+01A0: out of the grid; 0 only on the party row facing 2 or 6. */
    set_al(cpu, arg(ram, cpu, 0x0Au));
    value = read8(ram, cpu->ds, PARTY_Y);
    cmp8(cpu, (uint8_t)cpu->ax, value);
    if ((uint8_t)cpu->ax == value) {
        value = arg(ram, cpu, 0x08u);
        cmp8(cpu, value, 2u);
        if (value != 2u) {
            value = arg(ram, cpu, 0x08u);
            cmp8(cpu, value, 6u);
        }
        if (value == 2u || value == 6u) {
            write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u), 0u);   /* +019A */
            leave(ram, cpu);
            return PR_U33_012B_OK;
        }
    }
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u), 1u);   /* +01A0 */
    leave(ram, cpu);
    return PR_U33_012B_OK;
}
