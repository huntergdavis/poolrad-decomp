/* GAME.OVR unit 0x7D:+02F2..+03A2; docs/re/U7D_CARRY_02F2.md. */
#include "poolrad/recovered/overlay_u7d_carry_02f2.h"
#define MASK (PR_U7D_02F2_RAM_SIZE - 1u)
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
static uint16_t local(const uint8_t *ram, const pr_u7d_02f2_cpu *cpu, uint16_t distance)
{
    return word(ram, cpu->ss, (uint16_t)(cpu->bp - distance));
}
static void store(uint8_t *ram, const pr_u7d_02f2_cpu *cpu, uint16_t distance, uint16_t value)
{
    put(ram, cpu->ss, (uint16_t)(cpu->bp - distance), value);
}
static void les(const uint8_t *ram, pr_u7d_02f2_cpu *cpu, uint16_t segment, uint16_t offset)
{
    size_t at = physical(segment, offset);
    cpu->di = word_at(ram, at);
    cpu->es = word_at(ram, (at + 2u) & MASK);
}
static uint8_t index_byte(const uint8_t *ram, const pr_u7d_02f2_cpu *cpu)
{
    return ram[physical(cpu->ss, (uint16_t)(cpu->bp - 1u))];
}
static void index_ax(const uint8_t *ram, pr_u7d_02f2_cpu *cpu)
{
    uint8_t value = index_byte(ram, cpu);
    cpu->ax = value < 0x80u ? value : (uint16_t)(0xFF00u | value);
}
static void array_element(const uint8_t *ram, pr_u7d_02f2_cpu *cpu)
{
    cpu->ax = (uint16_t)(cpu->ax << 1u);
    les(ram, cpu, cpu->ss, (uint16_t)(cpu->bp + 6u));
    cpu->di = (uint16_t)(cpu->di + cpu->ax);
}

pr_u7d_02f2_status pr_u7d_carry_02f2(
    uint8_t *ram, size_t ram_size, pr_u7d_02f2_cpu *cpu)
{
    size_t at;
    if (ram == NULL || cpu == NULL || ram_size != PR_U7D_02F2_RAM_SIZE)
        return PR_U7D_02F2_ARGUMENT;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    put(ram, cpu->ss, cpu->sp, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 6u);
    ram[physical(cpu->ss, (uint16_t)(cpu->bp - 1u))] = 0u;
    for (;;) {
        /* +0301..+031D: signed byte index, unsigned word threshold. */
        index_ax(ram, cpu);
        cpu->di = (uint16_t)(cpu->ax << 1u);
        cpu->dx = word(ram, cpu->ds, (uint16_t)(cpu->di + 0x363Au));
        index_ax(ram, cpu);
        array_element(ram, cpu);
        cpu->ax = word(ram, cpu->es, cpu->di);
        if (cpu->ax >= cpu->dx) {
            if (index_byte(ram, cpu) != 6u) {
                /* +0325..+0361: increment the next element, then reload
                 * index/table/argument before subtracting the threshold. */
                uint8_t next = (uint8_t)(index_byte(ram, cpu) + 1u);
                cpu->ax = next < 0x80u ? next : (uint16_t)(0xFF00u | next);
                array_element(ram, cpu);
                put(ram, cpu->es, cpu->di,
                    (uint16_t)(word(ram, cpu->es, cpu->di) + 1u));
                index_ax(ram, cpu);
                cpu->di = (uint16_t)(cpu->ax << 1u);
                cpu->dx = word(ram, cpu->ds, (uint16_t)(cpu->di + 0x363Au));
                index_ax(ram, cpu);
                array_element(ram, cpu);
                cpu->ax = word(ram, cpu->es, cpu->di);
                cpu->ax = (uint16_t)(cpu->ax - cpu->dx);
                cpu->dx = cpu->ax;
                index_ax(ram, cpu);
                array_element(ram, cpu);
                put(ram, cpu->es, cpu->di, cpu->dx);
            } else {
                /* +0363..+0392: keep the unaligned pointer locals in RAM;
                 * the counter write can change the subsequent LES. */
                cpu->ax = word(ram, cpu->ds, 0x5D96u);
                cpu->dx = word(ram, cpu->ds, 0x5D98u);
                store(ram, cpu, 5u, cpu->ax);
                store(ram, cpu, 3u, cpu->dx);
                for (;;) {
                    cpu->ax = local(ram, cpu, 5u);
                    cpu->ax = (uint16_t)(cpu->ax | local(ram, cpu, 3u));
                    if (cpu->ax == 0u) break;
                    les(ram, cpu, cpu->ss, (uint16_t)(cpu->bp - 5u));
                    at = physical(cpu->es, (uint16_t)(cpu->di + 0x30u));
                    put(ram, cpu->es, (uint16_t)(cpu->di + 0x30u),
                        (uint16_t)(word_at(ram, at) + 1u));
                    les(ram, cpu, cpu->ss, (uint16_t)(cpu->bp - 5u));
                    cpu->ax = word(ram, cpu->es, (uint16_t)(cpu->di + 0x104u));
                    cpu->dx = word(ram, cpu->es, (uint16_t)(cpu->di + 0x106u));
                    store(ram, cpu, 5u, cpu->ax);
                    store(ram, cpu, 3u, cpu->dx);
                }
            }
        }
        if (index_byte(ram, cpu) == 6u) break;
        at = physical(cpu->ss, (uint16_t)(cpu->bp - 1u));
        ram[at] = (uint8_t)(ram[at] + 1u);
    }
    /* Every returning path ends with CMP byte [BP-1],6 equal. Its
     * defined arithmetic flags replace all earlier arithmetic flags. */
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~0x08D5u) | 0x44u);
    cpu->sp = cpu->bp;
    cpu->bp = word(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = word_at(ram, at);
    cpu->cs = word_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 8u);
    return PR_U7D_02F2_OK;
}
