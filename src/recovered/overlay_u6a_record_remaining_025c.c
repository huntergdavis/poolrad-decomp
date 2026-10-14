/* GAME.OVR unit 0x6A:+025C..+0301.
 * Evidence: docs/re/U6A_RECORD_REMAINING_025C.md. */
#include "poolrad/recovered/overlay_u6a_record_remaining_025c.h"

#define RAM_MASK (PR_U6A_025C_RAM_SIZE - 1u)

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & RAM_MASK;
}

static uint8_t read8(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return ram[physical(segment, offset)];
}

static uint16_t word_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & RAM_MASK] << 8));
}

static void set_local(uint8_t *ram, const pr_u6a_025c_cpu *cpu,
                      uint16_t distance, uint8_t value)
{
    ram[physical(cpu->ss, (uint16_t)(cpu->bp - distance))] = value;
}

static uint8_t local(const uint8_t *ram, const pr_u6a_025c_cpu *cpu,
                      uint16_t distance)
{
    return read8(ram, cpu->ss, (uint16_t)(cpu->bp - distance));
}

static void les_record(const uint8_t *ram, pr_u6a_025c_cpu *cpu)
{
    size_t at = physical(cpu->ds, 0x5D92u);
    cpu->di = word_at(ram, at);
    cpu->es = word_at(ram, (at + 2u) & RAM_MASK);
}

static void indexed_record(const uint8_t *ram, pr_u6a_025c_cpu *cpu)
{
    cpu->ax = local(ram, cpu, 3u);
    les_record(ram, cpu);
    cpu->di = (uint16_t)(cpu->di + cpu->ax);
}

static uint8_t descriptor(const uint8_t *ram, pr_u6a_025c_cpu *cpu,
                           uint16_t base)
{
    indexed_record(ram, cpu);
    cpu->ax = (uint16_t)(read8(ram, cpu->es,
                             (uint16_t)(cpu->di + 0x17u)) & 0x7Fu);
    cpu->di = cpu->ax;
    cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | 4u);
    cpu->di = (uint16_t)(cpu->di << 4u);
    cpu->ax = read8(ram, cpu->ds, (uint16_t)(cpu->di + base));
    return (uint8_t)cpu->ax;
}

static uint16_t sign_extend(uint8_t value)
{
    return value < 0x80u ? value : (uint16_t)(0xFF00u | value);
}

static void subtract_flags(pr_u6a_025c_cpu *cpu, uint16_t left,
                            uint16_t right, uint16_t result)
{
    uint8_t parity = (uint8_t)result;
    parity ^= (uint8_t)(parity >> 4u);
    parity ^= (uint8_t)(parity >> 2u);
    parity ^= (uint8_t)(parity >> 1u);
    cpu->flags &= (uint16_t)~0x08D5u;
    if (left < right) cpu->flags |= 0x0001u;
    if ((parity & 1u) == 0u) cpu->flags |= 0x0004u;
    if (((left ^ right ^ result) & 0x0010u) != 0u) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if ((result & 0x8000u) != 0u) cpu->flags |= 0x0080u;
    if (((left ^ right) & (left ^ result) & 0x8000u) != 0u)
        cpu->flags |= 0x0800u;
}

pr_u6a_025c_status pr_u6a_record_remaining_025c(
    uint8_t *ram, size_t ram_size, pr_u6a_025c_cpu *cpu)
{
    size_t at;
    uint16_t left;
    if (ram == NULL || cpu == NULL || ram_size != PR_U6A_025C_RAM_SIZE)
        return PR_U6A_025C_ARGUMENT;

    /* +025C..+026A: saved BP, four-byte frame, count and index. */
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    at = physical(cpu->ss, cpu->sp);
    ram[at] = (uint8_t)cpu->bp;
    ram[(at + 1u) & RAM_MASK] = (uint8_t)(cpu->bp >> 8u);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 4u);
    set_local(ram, cpu, 2u, 0u);
    set_local(ram, cpu, 3u, 0u);

    for (;;) {
        /* +026F..+02C5: every original LES reloads DS:[5D92], and
         * every descriptor and argument read occurs at its source site. */
        indexed_record(ram, cpu);
        if (read8(ram, cpu->es, (uint16_t)(cpu->di + 0x17u)) != 0u) {
            uint8_t high = descriptor(ram, cpu, 0x31FBu);
            if (high == read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8u))) {
                uint8_t low = descriptor(ram, cpu, 0x31FAu);
                if (low == read8(ram, cpu->ss, (uint16_t)(cpu->bp + 6u)))
                    set_local(ram, cpu, 2u, (uint8_t)(local(ram, cpu, 2u) + 1u));
            }
        }
        /* +02C8/+026C: 21 iterations. Only this INC writes the index. */
        if (local(ram, cpu, 3u) == 20u)
            break;
        set_local(ram, cpu, 3u, (uint8_t)(local(ram, cpu, 3u) + 1u));
    }

    /* +02CE..+02F6: CBW sign-extends each low argument byte. The
     * selector arithmetic wraps at 16 bits; SUB preserves the high
     * byte of its 16-bit difference when AL is copied through BP-1. */
    cpu->ax = local(ram, cpu, 2u);
    cpu->cx = cpu->ax;
    cpu->ax = sign_extend(read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8u)));
    cpu->dx = cpu->ax;
    cpu->ax = sign_extend(read8(ram, cpu->ss, (uint16_t)(cpu->bp + 6u)));
    cpu->si = cpu->ax;
    cpu->ax = (uint16_t)(cpu->ax << 1u);
    cpu->ax = (uint16_t)(cpu->ax + cpu->si);
    les_record(ram, cpu);
    cpu->di = (uint16_t)(cpu->di + cpu->ax);
    cpu->di = (uint16_t)(cpu->di + cpu->dx);
    left = read8(ram, cpu->es, (uint16_t)(cpu->di + 0xB1u));
    cpu->ax = (uint16_t)(left - cpu->cx);
    subtract_flags(cpu, left, cpu->cx, cpu->ax);
    set_local(ram, cpu, 1u, (uint8_t)cpu->ax);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | local(ram, cpu, 1u));

    /* +02FC..+0301: POP BP, contiguous four-byte RETF frame, RETF 4. */
    cpu->sp = cpu->bp;
    cpu->bp = word_at(ram, physical(cpu->ss, cpu->sp));
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = word_at(ram, at);
    cpu->cs = word_at(ram, (at + 2u) & RAM_MASK);
    cpu->sp = (uint16_t)(cpu->sp + 8u);
    return PR_U6A_025C_OK;
}
