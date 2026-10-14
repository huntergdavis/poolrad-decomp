/* GAME.OVR unit 0x6A:+14A9..+1600; docs/re/U6A_PARTY_BEFORE_14A9.md. */
#include "poolrad/recovered/overlay_u6a_party_before_14a9.h"
#define MASK (PR_U6A_14A9_RAM_SIZE - 1u)
static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}
static uint16_t word_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}
static uint16_t word(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return word_at(ram, physical(segment, offset));
}
static void put(uint8_t *ram, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8u);
}
static uint16_t local(const uint8_t *ram, const pr_u6a_14a9_cpu *cpu, uint16_t distance)
{
    return word(ram, cpu->ss, (uint16_t)(cpu->bp - distance));
}
static void store(uint8_t *ram, const pr_u6a_14a9_cpu *cpu, uint16_t distance, uint16_t value)
{
    put(ram, cpu->ss, (uint16_t)(cpu->bp - distance), value);
}
static void les_at(const uint8_t *ram, pr_u6a_14a9_cpu *cpu, uint16_t segment, uint16_t offset)
{
    size_t at = physical(segment, offset);
    cpu->di = word_at(ram, at);
    cpu->es = word_at(ram, (at + 2u) & MASK);
}
static void les_local(const uint8_t *ram, pr_u6a_14a9_cpu *cpu, uint16_t distance)
{
    les_at(ram, cpu, cpu->ss, (uint16_t)(cpu->bp - distance));
}
static void flags(pr_u6a_14a9_cpu *cpu, uint16_t result)
{
    uint8_t parity = (uint8_t)result;
    parity ^= (uint8_t)(parity >> 4u);
    parity ^= (uint8_t)(parity >> 2u);
    parity ^= (uint8_t)(parity >> 1u);
    cpu->flags &= (uint16_t)~0x08D5u;
    if ((parity & 1u) == 0u) cpu->flags |= 4u;
    if (result == 0u) cpu->flags |= 0x40u;
    if (result & 0x8000u) cpu->flags |= 0x80u;
}
static int equal(pr_u6a_14a9_cpu *cpu, uint16_t left, uint16_t right)
{
    uint16_t result = (uint16_t)(left - right);
    flags(cpu, result);
    if (left < right) cpu->flags |= 1u;
    if ((left ^ right ^ result) & 0x10u) cpu->flags |= 0x10u;
    if ((left ^ right) & (left ^ result) & 0x8000u) cpu->flags |= 0x800u;
    return result == 0u;
}
static void read_next(const uint8_t *ram, pr_u6a_14a9_cpu *cpu)
{
    cpu->ax = word(ram, cpu->es, (uint16_t)(cpu->di + 0x104u));
    cpu->dx = word(ram, cpu->es, (uint16_t)(cpu->di + 0x106u));
}
static void write_next(uint8_t *ram, const pr_u6a_14a9_cpu *cpu)
{
    put(ram, cpu->es, (uint16_t)(cpu->di + 0x104u), cpu->ax);
    put(ram, cpu->es, (uint16_t)(cpu->di + 0x106u), cpu->dx);
}

pr_u6a_14a9_status pr_u6a_party_before_14a9(
    uint8_t *ram, size_t ram_size, pr_u6a_14a9_cpu *cpu)
{
    size_t at;
    if (ram == NULL || cpu == NULL || ram_size != PR_U6A_14A9_RAM_SIZE)
        return PR_U6A_14A9_ARGUMENT;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    put(ram, cpu->ss, cpu->sp, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 12u);
    cpu->ax = word(ram, cpu->ds, 0x5D96u);
    cpu->dx = word(ram, cpu->ds, 0x5D98u);
    store(ram, cpu, 8u, cpu->ax);
    store(ram, cpu, 6u, cpu->dx);
    cpu->ax = 0u;
    flags(cpu, 0u);
    store(ram, cpu, 4u, cpu->ax);
    store(ram, cpu, 2u, cpu->ax);

    /* +14C4..+14FA: if current=head=active, find the last member. */
    cpu->ax = local(ram, cpu, 8u);
    cpu->dx = local(ram, cpu, 6u);
    if (equal(cpu, cpu->dx, word(ram, cpu->ds, 0x5D94u)) &&
        equal(cpu, cpu->ax, word(ram, cpu->ds, 0x5D92u))) {
        for (;;) {
            les_local(ram, cpu, 8u);
            cpu->ax = word(ram, cpu->es, (uint16_t)(cpu->di + 0x104u));
            cpu->ax = (uint16_t)(cpu->ax |
                word(ram, cpu->es, (uint16_t)(cpu->di + 0x106u)));
            flags(cpu, cpu->ax);
            if (cpu->ax == 0u) break;
            les_local(ram, cpu, 8u);
            read_next(ram, cpu);
            store(ram, cpu, 8u, cpu->ax);
            store(ram, cpu, 6u, cpu->dx);
        }
    } else {
        /* +14FC..+1534: find the predecessor and retain its predecessor. */
        for (;;) {
            les_local(ram, cpu, 8u);
            read_next(ram, cpu);
            if (equal(cpu, cpu->dx, word(ram, cpu->ds, 0x5D94u)) &&
                equal(cpu, cpu->ax, word(ram, cpu->ds, 0x5D92u))) break;
            cpu->ax = local(ram, cpu, 8u);
            cpu->dx = local(ram, cpu, 6u);
            store(ram, cpu, 4u, cpu->ax);
            store(ram, cpu, 2u, cpu->dx);
            les_local(ram, cpu, 8u);
            read_next(ram, cpu);
            store(ram, cpu, 8u, cpu->ax);
            store(ram, cpu, 6u, cpu->dx);
        }
    }

    /* +1536..+1547: save active.next before any list mutation. */
    les_at(ram, cpu, cpu->ds, 0x5D92u);
    read_next(ram, cpu);
    store(ram, cpu, 12u, cpu->ax);
    store(ram, cpu, 10u, cpu->dx);
    cpu->ax = word(ram, cpu->ds, 0x5D96u);
    cpu->dx = word(ram, cpu->ds, 0x5D98u);
    if (equal(cpu, cpu->dx, word(ram, cpu->ds, 0x5D94u)) &&
        equal(cpu, cpu->ax, word(ram, cpu->ds, 0x5D92u))) {
        /* +155D..+1596: head becomes successor, tail links active,
         * then both words of active.next are cleared using AX, not DX. */
        cpu->ax = local(ram, cpu, 12u);
        cpu->ax = (uint16_t)(cpu->ax | local(ram, cpu, 10u));
        flags(cpu, cpu->ax);
        if (cpu->ax != 0u) {
            cpu->ax = local(ram, cpu, 12u);
            cpu->dx = local(ram, cpu, 10u);
            put(ram, cpu->ds, 0x5D96u, cpu->ax);
            put(ram, cpu->ds, 0x5D98u, cpu->dx);
        }
        cpu->ax = word(ram, cpu->ds, 0x5D92u);
        cpu->dx = word(ram, cpu->ds, 0x5D94u);
        les_local(ram, cpu, 8u);
        write_next(ram, cpu);
        les_at(ram, cpu, cpu->ds, 0x5D92u);
        cpu->ax = 0u;
        flags(cpu, 0u);
        put(ram, cpu->es, (uint16_t)(cpu->di + 0x104u), cpu->ax);
        put(ram, cpu->es, (uint16_t)(cpu->di + 0x106u), cpu->ax);
    } else {
        int head;
        /* +1598..+15BA: predecessor bypasses active; active links predecessor. */
        cpu->ax = local(ram, cpu, 12u);
        cpu->dx = local(ram, cpu, 10u);
        les_local(ram, cpu, 8u);
        write_next(ram, cpu);
        cpu->ax = local(ram, cpu, 8u);
        cpu->dx = local(ram, cpu, 6u);
        les_at(ram, cpu, cpu->ds, 0x5D92u);
        write_next(ram, cpu);
        cpu->ax = local(ram, cpu, 8u);
        cpu->dx = local(ram, cpu, 6u);
        head = equal(cpu, cpu->dx, word(ram, cpu->ds, 0x5D98u)) &&
               equal(cpu, cpu->ax, word(ram, cpu->ds, 0x5D96u));
        if (!head) {
            cpu->ax = local(ram, cpu, 4u);
            cpu->ax = (uint16_t)(cpu->ax | local(ram, cpu, 2u));
            flags(cpu, cpu->ax);
            head = cpu->ax == 0u;
        }
        cpu->ax = word(ram, cpu->ds, 0x5D92u);
        cpu->dx = word(ram, cpu->ds, 0x5D94u);
        if (head) {
            put(ram, cpu->ds, 0x5D96u, cpu->ax);
            put(ram, cpu->ds, 0x5D98u, cpu->dx);
        } else {
            les_local(ram, cpu, 4u);
            write_next(ram, cpu);
        }
    }
    /* +15FD..+1600: restore frame and consume contiguous far return words. */
    cpu->sp = cpu->bp;
    cpu->bp = word(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = word_at(ram, at);
    cpu->cs = word_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u);
    return PR_U6A_14A9_OK;
}
