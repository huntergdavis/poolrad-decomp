/* GAME.OVR U3C2 public entry 3, +02CC..+0421 (342 original bytes). */
#include "poolrad/recovered/overlay_u3c2_entry3_02cc.h"

#define RAM_MASK (PR_U3C2_02CC_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u /* OF SF ZF AF PF CF */

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & RAM_MASK;
}

static uint8_t read8(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return ram[physical(segment, offset)];
}

static uint16_t read16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    size_t a = physical(segment, offset);
    return (uint16_t)(ram[a] | ((uint16_t)ram[(a + 1u) & RAM_MASK] << 8));
}

static void write8(uint8_t *ram, uint16_t segment, uint16_t offset,
                   uint8_t value)
{
    ram[physical(segment, offset)] = value;
}

static void write16(uint8_t *ram, uint16_t segment, uint16_t offset,
                    uint16_t value)
{
    size_t a = physical(segment, offset);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & RAM_MASK] = (uint8_t)(value >> 8);
}

/* A far LES operand is four physically consecutive bytes. */
static void les(const uint8_t *ram, pr_u3c2_02cc_cpu *cpu, uint16_t offset)
{
    size_t a = physical(cpu->ss, offset);
    cpu->di = (uint16_t)(ram[a] | ((uint16_t)ram[(a + 1u) & RAM_MASK] << 8));
    cpu->es = (uint16_t)(ram[(a + 2u) & RAM_MASK] |
                         ((uint16_t)ram[(a + 3u) & RAM_MASK] << 8));
}

static void les_record(const uint8_t *ram, pr_u3c2_02cc_cpu *cpu)
{
    les(ram, cpu, (uint16_t)(cpu->bp - 7u));
}

static uint8_t record8(const uint8_t *ram, pr_u3c2_02cc_cpu *cpu,
                       uint16_t displacement)
{
    les_record(ram, cpu);
    return read8(ram, cpu->es, (uint16_t)(cpu->di + displacement));
}

static uint16_t record16(const uint8_t *ram, pr_u3c2_02cc_cpu *cpu,
                         uint16_t displacement)
{
    les_record(ram, cpu);
    return read16(ram, cpu->es, (uint16_t)(cpu->di + displacement));
}

static void add_record8(uint8_t *ram, pr_u3c2_02cc_cpu *cpu,
                        uint16_t displacement, uint8_t delta)
{
    uint8_t old;
    les_record(ram, cpu);
    old = read8(ram, cpu->es, (uint16_t)(cpu->di + displacement));
    write8(ram, cpu->es, (uint16_t)(cpu->di + displacement),
           (uint8_t)(old + delta));
}

static void add_record16(uint8_t *ram, pr_u3c2_02cc_cpu *cpu,
                         uint16_t displacement, uint16_t delta)
{
    uint16_t old;
    les_record(ram, cpu);
    old = read16(ram, cpu->es, (uint16_t)(cpu->di + displacement));
    write16(ram, cpu->es, (uint16_t)(cpu->di + displacement),
            (uint16_t)(old + delta));
}

static uint16_t sign_byte(uint8_t value)
{
    return (uint16_t)((value & 0x80u) != 0u ? (0xFF00u | value) : value);
}

static int32_t signed_word(uint16_t value)
{
    return value < 0x8000u ? (int32_t)value : (int32_t)value - 0x10000;
}

static uint8_t parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (uint8_t)((value & 1u) == 0u);
}

static void final_add_flags(pr_u3c2_02cc_cpu *cpu, uint16_t left,
                            uint16_t right, uint16_t result)
{
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if ((uint32_t)left + right > 0xFFFFu) cpu->flags |= 1u;
    if (parity((uint8_t)result)) cpu->flags |= 4u;
    if (((left ^ right ^ result) & 0x10u) != 0u) cpu->flags |= 0x10u;
    if (result == 0u) cpu->flags |= 0x40u;
    if ((result & 0x8000u) != 0u) cpu->flags |= 0x80u;
    if (((~(left ^ right)) & (left ^ result) & 0x8000u) != 0u)
        cpu->flags |= 0x800u;
}

static uint16_t pop16(const uint8_t *ram, pr_u3c2_02cc_cpu *cpu)
{
    uint16_t value = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return value;
}

pr_u3c2_02cc_status pr_u3c2_entry3_02cc(
    uint8_t *ram, size_t ram_size, pr_u3c2_02cc_cpu *cpu)
{
    uint16_t left, offset, record_offset;
    uint8_t table_byte;
    if (ram == NULL || cpu == NULL || ram_size < PR_U3C2_02CC_RAM_SIZE ||
        cpu->ip != 0x02CCu)
        return PR_U3C2_02CC_ARGUMENT;

    cpu->sp = (uint16_t)(cpu->sp - 2u); /* PUSH BP */
    write16(ram, cpu->ss, cpu->sp, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 8u);
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), 0u);
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 3u), 0u);
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u), 0u);
    les(ram, cpu, (uint16_t)(cpu->bp + 6u));
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 7u), cpu->di);
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 5u), cpu->es);

    cpu->ax = record16(ram, cpu, 0x0Au);
    if (signed_word(cpu->ax) > signed_word(record16(ram, cpu, 0x0Cu))) {
        cpu->ax = record16(ram, cpu, 0x14u);
        if (cpu->ax != record16(ram, cpu, 4u)) {
            cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                                 record8(ram, cpu, 0x12u));
            write8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u),
                   (uint8_t)cpu->ax);
            if (signed_word(record16(ram, cpu, 8u)) >= 0) {
                cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                                     record8(ram, cpu, 0x13u));
                write8(ram, cpu->ss, (uint16_t)(cpu->bp - 3u),
                       (uint8_t)cpu->ax);
                cpu->ax = sign_byte(record8(ram, cpu, 0x13u));
                add_record16(ram, cpu, 0x16u, cpu->ax);
                cpu->ax = record16(ram, cpu, 0x0Eu);
                add_record16(ram, cpu, 8u, cpu->ax);
                add_record8(ram, cpu, 0x18u, 3u);
            } else {
                cpu->ax = record16(ram, cpu, 0x10u);
                add_record16(ram, cpu, 8u, cpu->ax);
                add_record8(ram, cpu, 0x18u, 2u);
            }
            cpu->ax = sign_byte(record8(ram, cpu, 0x12u));
            add_record16(ram, cpu, 0x14u, cpu->ax);
            write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u), 1u);
        }
    } else {
        cpu->ax = record16(ram, cpu, 0x16u);
        if (cpu->ax != record16(ram, cpu, 6u)) {
            cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                                 record8(ram, cpu, 0x13u));
            write8(ram, cpu->ss, (uint16_t)(cpu->bp - 3u),
                   (uint8_t)cpu->ax);
            if (signed_word(record16(ram, cpu, 8u)) >= 0) {
                cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                                     record8(ram, cpu, 0x12u));
                write8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u),
                       (uint8_t)cpu->ax);
                cpu->ax = sign_byte(record8(ram, cpu, 0x12u));
                add_record16(ram, cpu, 0x14u, cpu->ax);
                cpu->ax = record16(ram, cpu, 0x0Eu);
                add_record16(ram, cpu, 8u, cpu->ax);
                add_record8(ram, cpu, 0x18u, 3u);
            } else {
                cpu->ax = record16(ram, cpu, 0x10u);
                add_record16(ram, cpu, 8u, cpu->ax);
                add_record8(ram, cpu, 0x18u, 2u);
            }
            cpu->ax = sign_byte(record8(ram, cpu, 0x13u));
            add_record16(ram, cpu, 0x16u, cpu->ax);
            write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u), 1u);
        }
    }

    cpu->ax = (uint16_t)(sign_byte(
        read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u))) + 1u);
    cpu->dx = cpu->ax;
    cpu->ax = (uint16_t)(sign_byte(
        read8(ram, cpu->ss, (uint16_t)(cpu->bp - 3u))) + 1u);
    cpu->di = cpu->ax;
    cpu->si = cpu->di;
    cpu->di = (uint16_t)(cpu->di << 1);
    cpu->di = (uint16_t)(cpu->di + cpu->si);
    left = cpu->di;
    cpu->di = (uint16_t)(cpu->di + cpu->dx);
    final_add_flags(cpu, left, cpu->dx, cpu->di);
    offset = (uint16_t)(cpu->di + 0x25C0u);
    table_byte = read8(ram, cpu->ds, offset);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | table_byte);
    les_record(ram, cpu);
    record_offset = (uint16_t)(cpu->di + 0x19u);
    write8(ram, cpu->es, record_offset, table_byte);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u)));
    cpu->sp = cpu->bp;
    cpu->bp = pop16(ram, cpu);
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 4u);
    return PR_U3C2_02CC_OK;
}
