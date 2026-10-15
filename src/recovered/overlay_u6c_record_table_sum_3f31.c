/* Unit 6C:+3F31..+3FB2; docs/re/U6C_RECORD_TABLE_SUM_3F31.md. */
#include "poolrad/recovered/overlay_u6c_record_table_sum_3f31.h"

#define MASK (PR_U6C_3F31_RAM_SIZE - 1u)

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint16_t word_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static void put_word(uint8_t *ram, size_t at, uint16_t value)
{
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8u);
}

static uint8_t read_byte(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return ram[physical(segment, offset)];
}

static size_t local_at(const pr_u6c_3f31_cpu *cpu, uint16_t distance)
{
    return physical(cpu->ss, (uint16_t)(cpu->bp - distance));
}

static void load_far(const uint8_t *ram, pr_u6c_3f31_cpu *cpu, uint16_t offset)
{
    size_t at = physical(cpu->ss, offset);
    cpu->di = word_at(ram, at);
    cpu->es = word_at(ram, (at + 2u) & MASK);
}

static void load_saved_record(const uint8_t *ram, pr_u6c_3f31_cpu *cpu)
{
    load_far(ram, cpu, (uint16_t)(cpu->bp - 8u));
}

pr_u6c_3f31_status pr_u6c_record_table_sum_3f31(
    uint8_t *ram, size_t ram_size, pr_u6c_3f31_cpu *cpu)
{
    size_t at, total, index;
    uint8_t value;
    if (!ram || !cpu || ram_size < PR_U6C_3F31_RAM_SIZE)
        return PR_U6C_3F31_ARGUMENT;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    put_word(ram, physical(cpu->ss, cpu->sp), cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 8u);
    total = local_at(cpu, 2u);
    index = local_at(cpu, 3u);
    ram[total] = 0;
    load_far(ram, cpu, (uint16_t)(cpu->bp + 6u));
    put_word(ram, local_at(cpu, 8u), cpu->di);
    put_word(ram, local_at(cpu, 6u), cpu->es);
    ram[index] = 0;
    for (;;) {
        /* Opcode 98 in 16-bit mode is CBW. The live index is 0..7. */
        value = ram[index];
        cpu->ax = value < 0x80u ? value : (uint16_t)(0xFF00u | value);
        load_saved_record(ram, cpu);
        cpu->di = (uint16_t)(cpu->di + cpu->ax);
        value = read_byte(ram, cpu->es, (uint16_t)(cpu->di + 0x96u));
        if (value != 0u && value < 0x80u) {
            load_saved_record(ram, cpu);
            cpu->ax = read_byte(ram, cpu->es, (uint16_t)(cpu->di + 0x14u));
            cpu->di = cpu->ax;
            cpu->ax = read_byte(ram, cpu->ds, (uint16_t)(cpu->di + 0x408Eu));
            ram[total] = (uint8_t)(ram[total] + (uint8_t)cpu->ax);
            load_saved_record(ram, cpu);
            if (read_byte(ram, cpu->es, (uint16_t)(cpu->di + 0x14u)) > 0x10u) {
                load_saved_record(ram, cpu);
                if (read_byte(ram, cpu->es, (uint16_t)(cpu->di + 0x2Fu)) == 2u)
                    ram[total] = (uint8_t)(ram[total] + 1u);
            }
            load_saved_record(ram, cpu);
            if (read_byte(ram, cpu->es, (uint16_t)(cpu->di + 0x14u)) > 0x11u) {
                load_saved_record(ram, cpu);
                if (read_byte(ram, cpu->es, (uint16_t)(cpu->di + 0x2Fu)) == 2u)
                    ram[total] = (uint8_t)(ram[total] + 1u);
            }
        }
        /* Final CMP [BP-3],7 defines all arithmetic flags. No later
         * instruction changes flags. Only INC writes the initialized index. */
        if (ram[index] == 7u)
            break;
        ram[index] = (uint8_t)(ram[index] + 1u);
    }
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~0x08D5u) | 0x0044u);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | ram[total]);
    ram[local_at(cpu, 1u)] = (uint8_t)cpu->ax;
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | ram[local_at(cpu, 1u)]);
    cpu->sp = cpu->bp;
    cpu->bp = word_at(ram, physical(cpu->ss, cpu->sp));
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = word_at(ram, at);
    cpu->cs = word_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 8u);
    return PR_U6C_3F31_OK;
}
