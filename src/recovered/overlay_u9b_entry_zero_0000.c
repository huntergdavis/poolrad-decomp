/* GAME.OVR unit 0x9B:+0x0000..+0x01AE.
 * Evidence: docs/re/U9B_ENTRY_ZERO_COMPOSITION.md. */
#include "poolrad/recovered/overlay_u9b_entry_zero_0000.h"
#include "poolrad/recovered/overlay_u9b_record_fields_0329.h"
#include "poolrad/recovered/overlay_u9b_record_table_0255.h"

#include <string.h>

#define RAM_MASK (PR_U9B_0000_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u /* OF SF ZF AF PF CF */

_Static_assert(sizeof(pr_u9b_0000_cpu) == sizeof(pr_u9b_0255_cpu),
               "u9B +0255 CPU state layout differs");
_Static_assert(sizeof(pr_u9b_0000_cpu) == sizeof(pr_u9b_0329_cpu),
               "u9B +0329 CPU state layout differs");

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & RAM_MASK;
}

static uint8_t read8(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return ram[physical(segment, offset)];
}

static void write8(uint8_t *ram, uint16_t segment, uint16_t offset,
                   uint8_t value)
{
    ram[physical(segment, offset)] = value;
}

static uint16_t read16_at(const uint8_t *ram, size_t address)
{
    return (uint16_t)((uint16_t)ram[address] |
                      ((uint16_t)ram[(address + 1u) & RAM_MASK] << 8));
}

static uint16_t read16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return read16_at(ram, physical(segment, offset));
}

static void write16(uint8_t *ram, uint16_t segment, uint16_t offset,
                    uint16_t value)
{
    size_t address = physical(segment, offset);
    ram[address] = (uint8_t)value;
    ram[(address + 1u) & RAM_MASK] = (uint8_t)(value >> 8);
}

static void set_al(pr_u9b_0000_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static void clear_ah(pr_u9b_0000_cpu *cpu)
{
    cpu->ax &= 0x00FFu;
}

static void cbw(pr_u9b_0000_cpu *cpu)
{
    cpu->ax = (uint16_t)(int16_t)(int8_t)(uint8_t)cpu->ax;
}

static void les(uint8_t *ram, pr_u9b_0000_cpu *cpu, uint16_t offset)
{
    size_t address = physical(cpu->ss, offset);
    cpu->di = read16_at(ram, address);
    cpu->es = read16_at(ram, (address + 2u) & RAM_MASK);
}

static void les_arg(uint8_t *ram, pr_u9b_0000_cpu *cpu)
{
    les(ram, cpu, (uint16_t)(cpu->bp + 6u));
}

static uint8_t local(const uint8_t *ram, const pr_u9b_0000_cpu *cpu,
                     uint16_t back)
{
    return read8(ram, cpu->ss, (uint16_t)(cpu->bp - back));
}

static void set_local(uint8_t *ram, const pr_u9b_0000_cpu *cpu,
                      uint16_t back, uint8_t value)
{
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - back), value);
}

static void inc_local(uint8_t *ram, const pr_u9b_0000_cpu *cpu,
                      uint16_t back)
{
    set_local(ram, cpu, back, (uint8_t)(local(ram, cpu, back) + 1u));
}

static void push16(uint8_t *ram, pr_u9b_0000_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static void retf4(uint8_t *ram, pr_u9b_0000_cpu *cpu)
{
    size_t address = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, address);
    cpu->cs = read16_at(ram, (address + 2u) & RAM_MASK);
    cpu->sp = (uint16_t)(cpu->sp + 8u);
}

static void finish_frame(uint8_t *ram, pr_u9b_0000_cpu *cpu)
{
    cpu->sp = cpu->bp;
    cpu->bp = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    retf4(ram, cpu);
}

static unsigned even_parity(uint8_t value)
{
    unsigned parity = 0u;
    while (value != 0u) {
        parity ^= value & 1u;
        value >>= 1u;
    }
    return parity == 0u;
}

static void set_cmp8_flags(pr_u9b_0000_cpu *cpu, uint8_t left,
                           uint8_t right)
{
    uint8_t result = (uint8_t)(left - right);
    uint16_t flags = (uint16_t)(cpu->flags & ~ARITH_FLAGS);
    if (left < right)
        flags |= 0x0001u;
    if (even_parity(result))
        flags |= 0x0004u;
    if (((left ^ right ^ result) & 0x10u) != 0u)
        flags |= 0x0010u;
    if (result == 0u)
        flags |= 0x0040u;
    if ((result & 0x80u) != 0u)
        flags |= 0x0080u;
    if (((left ^ right) & (left ^ result) & 0x80u) != 0u)
        flags |= 0x0800u;
    cpu->flags = flags;
}

static void set_inc8_flags(pr_u9b_0000_cpu *cpu, uint8_t before)
{
    uint8_t result = (uint8_t)(before + 1u);
    uint16_t flags = (uint16_t)(cpu->flags & ~(ARITH_FLAGS & ~0x0001u));
    if (even_parity(result))
        flags |= 0x0004u;
    if ((before & 0x0Fu) == 0x0Fu)
        flags |= 0x0010u;
    if (result == 0u)
        flags |= 0x0040u;
    if ((result & 0x80u) != 0u)
        flags |= 0x0080u;
    if (before == 0x7Fu)
        flags |= 0x0800u;
    cpu->flags = flags;
}

/* +01AF has a segment-view effect API, but its near caller also needs the
 * original PUSH BP, cached SS far pointer, LES register, and RETF effects. */
static void call_01af(uint8_t *ram, pr_u9b_0000_cpu *cpu)
{
    static const uint8_t thresholds[5] = {
        0x0Cu, 0x0Du, 0x0Eu, 0x0Fu, 0x10u
    };
    static const uint16_t fields[5] = {
        0xB2u, 0xB2u, 0xB3u, 0xB3u, 0xB4u
    };
    unsigned step;

    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 4u);
    les_arg(ram, cpu);
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u), cpu->di);
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), cpu->es);

    for (step = 0u; step < 5u; ++step) {
        uint8_t threshold_value;
        les(ram, cpu, (uint16_t)(cpu->bp - 4u));
        threshold_value = read8(ram, cpu->es,
                                (uint16_t)(cpu->di + 0x12u));
        set_cmp8_flags(cpu, threshold_value, thresholds[step]);
        if (threshold_value > thresholds[step]) {
            uint8_t counter_value;
            les(ram, cpu, (uint16_t)(cpu->bp - 4u));
            counter_value = read8(ram, cpu->es,
                                  (uint16_t)(cpu->di + fields[step]));
            set_cmp8_flags(cpu, counter_value, 0u);
            if (counter_value > 0u) {
                uint16_t address;
                uint8_t before;
                les(ram, cpu, (uint16_t)(cpu->bp - 4u));
                address = (uint16_t)(cpu->di + fields[step]);
                before = read8(ram, cpu->es, address);
                write8(ram, cpu->es, address, (uint8_t)(before + 1u));
                set_inc8_flags(cpu, before);
            }
        }
    }
    finish_frame(ram, cpu);
}

static void push_internal_frame(uint8_t *ram, pr_u9b_0000_cpu *cpu,
                                uint16_t return_ip, uint16_t entry_ip)
{
    /* +00E5/+018A/+019F: each source word is reread after the previous
     * push because SS arguments may alias those pushes. */
    push16(ram, cpu, read16(ram, cpu->ss,
                           (uint16_t)(cpu->bp + 8u)));
    push16(ram, cpu, read16(ram, cpu->ss,
                           (uint16_t)(cpu->bp + 6u)));
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, return_ip);
    cpu->ip = entry_ip;
}

static pr_u9b_0000_status call_0255(uint8_t *ram, size_t ram_size,
                                   pr_u9b_0000_cpu *cpu)
{
    pr_u9b_0255_cpu leaf;
    memcpy(&leaf, cpu, sizeof leaf);
    if (pr_u9b_record_table_0255(ram, ram_size, &leaf) != PR_U9B_0255_OK)
        return PR_U9B_0000_ARGUMENT;
    memcpy(cpu, &leaf, sizeof leaf);
    return PR_U9B_0000_OK;
}

static pr_u9b_0000_status call_0329(uint8_t *ram, size_t ram_size,
                                   pr_u9b_0000_cpu *cpu)
{
    pr_u9b_0329_cpu leaf;
    memcpy(&leaf, cpu, sizeof leaf);
    if (pr_u9b_record_fields_0329(ram, ram_size, &leaf) != PR_U9B_0329_OK)
        return PR_U9B_0000_ARGUMENT;
    memcpy(cpu, &leaf, sizeof leaf);
    return PR_U9B_0000_OK;
}

static void set_cmp_zero_flags(pr_u9b_0000_cpu *cpu, uint8_t value)
{
    set_cmp8_flags(cpu, value, 0u);
}

/* +0028..+003B / +0048..+005B: signed class, unsigned multiplication. */
static void table_index_3c7c(uint8_t *ram, pr_u9b_0000_cpu *cpu)
{
    uint32_t product;
    set_al(cpu, local(ram, cpu, 3u));
    cbw(cpu);
    cpu->cx = cpu->ax;
    set_al(cpu, local(ram, cpu, 1u));
    cbw(cpu);
    cpu->dx = 0x000Bu;
    product = (uint32_t)cpu->ax * cpu->dx;
    cpu->ax = (uint16_t)product;
    cpu->dx = (uint16_t)(product >> 16);
    cpu->di = cpu->ax;
    cpu->di = (uint16_t)(cpu->di + cpu->cx);
    set_al(cpu, read8(ram, cpu->ds, (uint16_t)(cpu->di + 0x3C7Cu)));
}

/* +00B9..+00CD / +0155..+0169: signed class times three plus local. */
static void table_index_three(uint8_t *ram, pr_u9b_0000_cpu *cpu,
                              uint16_t local_back, uint16_t table_base)
{
    set_al(cpu, local(ram, cpu, local_back));
    cbw(cpu);
    cpu->dx = cpu->ax;
    set_al(cpu, local(ram, cpu, 3u));
    cbw(cpu);
    cpu->di = cpu->ax;
    cpu->si = cpu->di;
    cpu->di = (uint16_t)(cpu->di << 1);
    cpu->di = (uint16_t)(cpu->di + cpu->si);
    cpu->di = (uint16_t)(cpu->di + cpu->dx);
    cpu->dx = (uint16_t)((cpu->dx & 0xFF00u) |
                         read8(ram, cpu->ds,
                               (uint16_t)(cpu->di + table_base)));
}

static void write_indexed_record(uint8_t *ram, pr_u9b_0000_cpu *cpu,
                                 uint16_t index_local, uint16_t field)
{
    set_al(cpu, local(ram, cpu, index_local));
    cbw(cpu);
    les_arg(ram, cpu);
    cpu->di = (uint16_t)(cpu->di + cpu->ax);
    write8(ram, cpu->es, (uint16_t)(cpu->di + field),
           (uint8_t)cpu->dx);
}

pr_u9b_0000_status pr_u9b_entry_zero_0000(
    uint8_t *ram, size_t ram_size, pr_u9b_0000_cpu *cpu)
{
    int first_outer = 1;
    if (ram == NULL || ram_size < PR_U9B_0000_RAM_SIZE || cpu == NULL)
        return PR_U9B_0000_ARGUMENT;

    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 8u);
    les_arg(ram, cpu);
    write8(ram, cpu->es, (uint16_t)(cpu->di + 0x2Du), 0u);
    set_local(ram, cpu, 1u, 0u);

    for (;;) {
        if (!first_outer)
            inc_local(ram, cpu, 1u);
        first_outer = 0;

        /* +0017..+0025: record class is cached in local [BP-3]. */
        set_al(cpu, local(ram, cpu, 1u));
        cbw(cpu);
        les_arg(ram, cpu);
        cpu->di = (uint16_t)(cpu->di + cpu->ax);
        set_al(cpu, read8(ram, cpu->es,
                          (uint16_t)(cpu->di + 0x96u)));
        set_local(ram, cpu, 3u, (uint8_t)cpu->ax);

        table_index_3c7c(ram, cpu);
        les_arg(ram, cpu);
        if ((uint8_t)cpu->ax >
            read8(ram, cpu->es, (uint16_t)(cpu->di + 0x2Du))) {
            table_index_3c7c(ram, cpu);
            les_arg(ram, cpu);
            write8(ram, cpu->es, (uint16_t)(cpu->di + 0x2Du),
                   (uint8_t)cpu->ax);
        }

        les_arg(ram, cpu);
        set_al(cpu, read8(ram, cpu->es,
                          (uint16_t)(cpu->di + 0x73u)));
        if ((int8_t)(uint8_t)cpu->ax <
            (int8_t)local(ram, cpu, 3u)) {
            set_al(cpu, local(ram, cpu, 3u));
            les_arg(ram, cpu);
            write8(ram, cpu->es, (uint16_t)(cpu->di + 0x73u),
                   (uint8_t)cpu->ax);
        }

        set_al(cpu, local(ram, cpu, 1u));
        if ((uint8_t)cpu->ax == 2u) {
            les_arg(ram, cpu);
            write8(ram, cpu->es, (uint16_t)(cpu->di + 0xA1u),
                   (int8_t)local(ram, cpu, 3u) > 6 ? 3u : 2u);
        } else if ((uint8_t)cpu->ax == 0u) {
            if ((int8_t)local(ram, cpu, 3u) > 1) {
                int first_inner = 1;
                uint16_t call_cs;
                set_local(ram, cpu, 2u, 1u);
                for (;;) {
                    if (!first_inner)
                        inc_local(ram, cpu, 2u);
                    first_inner = 0;
                    table_index_three(ram, cpu, 2u, 0x40BCu);
                    write_indexed_record(ram, cpu, 2u, 0xB1u);
                    if (local(ram, cpu, 2u) == 3u)
                        break;
                }

                call_cs = cpu->cs;
                push_internal_frame(ram, cpu, 0x00EFu, 0x01AFu);
                call_01af(ram, cpu);
                if (cpu->ip != 0x00EFu || cpu->cs != call_cs)
                    return PR_U9B_0000_TRANSFER;

                set_local(ram, cpu, 4u, 1u);
                {
                    int first_scan = 1;
                    for (;;) {
                        if (!first_scan)
                            inc_local(ram, cpu, 4u);
                        first_scan = 0;
                        set_al(cpu, local(ram, cpu, 4u));
                        clear_ah(cpu);
                        cpu->di = cpu->ax;
                        cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | 4u);
                        cpu->di = (uint16_t)(cpu->di << 4);
                        cpu->di = (uint16_t)(cpu->di + 0x31FAu);
                        write16(ram, cpu->ss,
                                (uint16_t)(cpu->bp - 8u), cpu->di);
                        write16(ram, cpu->ss,
                                (uint16_t)(cpu->bp - 6u), cpu->ds);
                        les(ram, cpu, (uint16_t)(cpu->bp - 8u));
                        if (read8(ram, cpu->es, cpu->di) == 0u) {
                            les(ram, cpu, (uint16_t)(cpu->bp - 8u));
                            set_al(cpu, read8(ram, cpu->es,
                                              (uint16_t)(cpu->di + 1u)));
                            cbw(cpu);
                            les_arg(ram, cpu);
                            cpu->di = (uint16_t)(cpu->di + cpu->ax);
                            if (read8(ram, cpu->es,
                                      (uint16_t)(cpu->di + 0xB1u)) > 0u) {
                                set_al(cpu, local(ram, cpu, 4u));
                                clear_ah(cpu);
                                les_arg(ram, cpu);
                                cpu->di = (uint16_t)(cpu->di + cpu->ax);
                                write8(ram, cpu->es,
                                       (uint16_t)(cpu->di + 0x32u), 1u);
                            }
                        }
                        if (local(ram, cpu, 4u) == 0x38u)
                            break;
                    }
                }
            }
        } else if ((uint8_t)cpu->ax == 5u &&
                   (int8_t)local(ram, cpu, 3u) > 1) {
            int first_inner = 1;
            set_local(ram, cpu, 2u, 1u);
            for (;;) {
                if (!first_inner)
                    inc_local(ram, cpu, 2u);
                first_inner = 0;
                table_index_three(ram, cpu, 2u, 0x41D4u);
                write_indexed_record(ram, cpu, 2u, 0xB4u);
                if (local(ram, cpu, 2u) == 3u)
                    break;
            }
        }

        if (local(ram, cpu, 1u) == 7u)
            break;
    }

    {
        uint16_t call_cs = cpu->cs;
        push_internal_frame(ram, cpu, 0x0194u, 0x0255u);
        if (call_0255(ram, ram_size, cpu) != PR_U9B_0000_OK)
            return PR_U9B_0000_ARGUMENT;
        if (cpu->ip != 0x0194u || cpu->cs != call_cs)
            return PR_U9B_0000_TRANSFER;
    }

    les_arg(ram, cpu);
    {
        uint8_t gate = read8(ram, cpu->es,
                             (uint16_t)(cpu->di + 0x9Cu));
        set_cmp_zero_flags(cpu, gate);
        if ((int8_t)gate > 0) {
            uint16_t call_cs = cpu->cs;
            push_internal_frame(ram, cpu, 0x01A9u, 0x0329u);
            if (call_0329(ram, ram_size, cpu) != PR_U9B_0000_OK)
                return PR_U9B_0000_ARGUMENT;
            if (cpu->ip != 0x01A9u || cpu->cs != call_cs)
                return PR_U9B_0000_TRANSFER;
        }
    }

    finish_frame(ram, cpu);
    return PR_U9B_0000_OK;
}
