/* GAME.OVR unit 0x6A:+1601..+171F; docs/re/U6A_PARTY_AFTER_1601.md. */
#include "poolrad/recovered/overlay_u6a_party_after_1601.h"
#define MASK (PR_U6A_1601_RAM_SIZE - 1u)
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
static uint16_t local(const uint8_t *ram, const pr_u6a_1601_cpu *cpu, uint16_t distance)
{
    return word(ram, cpu->ss, (uint16_t)(cpu->bp - distance));
}
static void store(uint8_t *ram, const pr_u6a_1601_cpu *cpu, uint16_t distance, uint16_t value)
{
    put(ram, cpu->ss, (uint16_t)(cpu->bp - distance), value);
}
static void les_at(const uint8_t *ram, pr_u6a_1601_cpu *cpu, uint16_t segment, uint16_t offset)
{
    size_t at = physical(segment, offset);
    cpu->di = word_at(ram, at);
    cpu->es = word_at(ram, (at + 2u) & MASK);
}
static void les_local(const uint8_t *ram, pr_u6a_1601_cpu *cpu, uint16_t distance)
{
    les_at(ram, cpu, cpu->ss, (uint16_t)(cpu->bp - distance));
}
static void flags(pr_u6a_1601_cpu *cpu, uint16_t result)
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
static int equal(pr_u6a_1601_cpu *cpu, uint16_t left, uint16_t right)
{
    uint16_t result = (uint16_t)(left - right);
    flags(cpu, result);
    if (left < right) cpu->flags |= 1u;
    if ((left ^ right ^ result) & 0x10u) cpu->flags |= 0x10u;
    if ((left ^ right) & (left ^ result) & 0x8000u) cpu->flags |= 0x800u;
    return result == 0u;
}
static void read_next(const uint8_t *ram, pr_u6a_1601_cpu *cpu)
{
    cpu->ax = word(ram, cpu->es, (uint16_t)(cpu->di + 0x104u));
    cpu->dx = word(ram, cpu->es, (uint16_t)(cpu->di + 0x106u));
}
static void write_next(uint8_t *ram, const pr_u6a_1601_cpu *cpu)
{
    put(ram, cpu->es, (uint16_t)(cpu->di + 0x104u), cpu->ax);
    put(ram, cpu->es, (uint16_t)(cpu->di + 0x106u), cpu->dx);
}

pr_u6a_1601_status pr_u6a_party_after_1601(
    uint8_t *ram, size_t ram_size, pr_u6a_1601_cpu *cpu)
{
    size_t at;
    if (ram == NULL || cpu == NULL || ram_size != PR_U6A_1601_RAM_SIZE)
        return PR_U6A_1601_ARGUMENT;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    put(ram, cpu->ss, cpu->sp, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 8u);
    cpu->ax = word(ram, cpu->ds, 0x5D96u);
    cpu->dx = word(ram, cpu->ds, 0x5D98u);
    store(ram, cpu, 4u, cpu->ax);
    store(ram, cpu, 2u, cpu->dx);
    cpu->ax = local(ram, cpu, 4u);
    cpu->dx = local(ram, cpu, 2u);
    if (!(equal(cpu, cpu->dx, word(ram, cpu->ds, 0x5D94u)) &&
          equal(cpu, cpu->ax, word(ram, cpu->ds, 0x5D92u)))) {
        /* +1626..+1652: retain the member immediately before active. */
        for (;;) {
            les_local(ram, cpu, 4u);
            read_next(ram, cpu);
            if (equal(cpu, cpu->dx, word(ram, cpu->ds, 0x5D94u)) &&
                equal(cpu, cpu->ax, word(ram, cpu->ds, 0x5D92u))) break;
            les_local(ram, cpu, 4u);
            read_next(ram, cpu);
            store(ram, cpu, 4u, cpu->ax);
            store(ram, cpu, 2u, cpu->dx);
        }
    }
    /* +1654..+166E: save active.next and select the tail-wrap path. */
    les_at(ram, cpu, cpu->ds, 0x5D92u);
    read_next(ram, cpu);
    store(ram, cpu, 8u, cpu->ax);
    store(ram, cpu, 6u, cpu->dx);
    cpu->ax = local(ram, cpu, 8u);
    cpu->ax = (uint16_t)(cpu->ax | local(ram, cpu, 6u));
    flags(cpu, cpu->ax);
    if (cpu->ax == 0u) {
        /* +1670..+16A6: active links old head; head becomes active;
         * predecessor then links the freshly reread successor locals. */
        cpu->ax = word(ram, cpu->ds, 0x5D96u);
        cpu->dx = word(ram, cpu->ds, 0x5D98u);
        les_at(ram, cpu, cpu->ds, 0x5D92u);
        write_next(ram, cpu);
        cpu->ax = word(ram, cpu->ds, 0x5D92u);
        cpu->dx = word(ram, cpu->ds, 0x5D94u);
        put(ram, cpu->ds, 0x5D96u, cpu->ax);
        put(ram, cpu->ds, 0x5D98u, cpu->dx);
        cpu->ax = local(ram, cpu, 8u);
        cpu->dx = local(ram, cpu, 6u);
        les_local(ram, cpu, 4u);
        write_next(ram, cpu);
    } else {
        /* +16A8..+16C0: active bypasses its successor. */
        les_local(ram, cpu, 8u);
        read_next(ram, cpu);
        les_at(ram, cpu, cpu->ds, 0x5D92u);
        write_next(ram, cpu);
        cpu->ax = word(ram, cpu->ds, 0x5D96u);
        cpu->dx = word(ram, cpu->ds, 0x5D98u);
        if (equal(cpu, cpu->dx, word(ram, cpu->ds, 0x5D94u)) &&
            equal(cpu, cpu->ax, word(ram, cpu->ds, 0x5D92u))) {
            cpu->ax = local(ram, cpu, 8u);
            cpu->dx = local(ram, cpu, 6u);
            put(ram, cpu->ds, 0x5D96u, cpu->ax);
            put(ram, cpu->ds, 0x5D98u, cpu->dx);
        }
        cpu->ax = local(ram, cpu, 4u);
        cpu->dx = local(ram, cpu, 2u);
        if (!(equal(cpu, cpu->dx, word(ram, cpu->ds, 0x5D94u)) &&
              equal(cpu, cpu->ax, word(ram, cpu->ds, 0x5D92u)))) {
            cpu->ax = local(ram, cpu, 8u);
            cpu->dx = local(ram, cpu, 6u);
            les_local(ram, cpu, 4u);
            write_next(ram, cpu);
        }
        cpu->ax = word(ram, cpu->ds, 0x5D92u);
        cpu->dx = word(ram, cpu->ds, 0x5D94u);
        les_local(ram, cpu, 8u);
        write_next(ram, cpu);
    }
    cpu->sp = cpu->bp;
    cpu->bp = word(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = word_at(ram, at);
    cpu->cs = word_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u);
    return PR_U6A_1601_OK;
}
