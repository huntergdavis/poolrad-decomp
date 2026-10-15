#include "poolrad/recovered/resident_464_2632_fourth_fillchar.h"

static size_t physical(uint16_t segment, uint16_t offset)
{
    return ((size_t)segment << 4) + offset;
}

static uint16_t word_at(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    size_t address = physical(segment, offset);
    return (uint16_t)(ram[address] | ((uint16_t)ram[address + 1u] << 8));
}

static void put_word(uint8_t *ram, uint16_t segment, uint16_t offset,
                     uint16_t value)
{
    size_t address = physical(segment, offset);
    ram[address] = (uint8_t)value;
    ram[address + 1u] = (uint8_t)(value >> 8);
}

static void push(uint8_t *ram, pr_r464_2632_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    put_word(ram, cpu->ss, cpu->sp, value);
}

static void observe(const uint8_t *ram, const pr_r464_2632_cpu *cpu,
                    pr_r464_2632_observer observer, void *context)
{
    if (observer != NULL) observer(cpu->ip, ram, cpu, context);
}

pr_r464_2632_status pr_r464_2632_fourth_fillchar(
    uint8_t *ram, size_t ram_size, pr_r464_2632_cpu *cpu,
    pr_r464_2632_observer observer, void *context)
{
    uint16_t return_ip, return_cs;
    uint8_t value;
    if (ram == NULL || cpu == NULL || ram_size < PR_R464_2632_RAM_BYTES)
        return PR_R464_2632_INVALID_HOST;
    if (cpu->cs != 0x0C88u || cpu->ip != 0x291Cu ||
        cpu->ax != 0u || cpu->bx != 0u || cpu->cx != 104u ||
        cpu->dx != 0u || cpu->si != 0x2612u ||
        cpu->di != 0x3E64u || cpu->bp != 0x3E80u ||
        cpu->sp != 0x3E54u || cpu->ds != 0x14A0u ||
        cpu->es != 0x3DE4u || cpu->ss != 0x1D16u ||
        word_at(ram, cpu->ss, 0x3E54u) != 0u ||
        word_at(ram, cpu->ss, 0x3E56u) != 4u ||
        word_at(ram, cpu->ss, 0x3E58u) != 0x3E64u ||
        word_at(ram, cpu->ss, 0x3E5Au) != 0x1D16u)
        return PR_R464_2632_UNSUPPORTED_PATH;

    observe(ram, cpu, observer, context); /* caller +291C */
    push(ram, cpu, cpu->cs);
    push(ram, cpu, 0x2921u);
    cpu->cs = 0x131Cu;
    cpu->ip = 0x16A8u;
    observe(ram, cpu, observer, context);

    cpu->bx = cpu->sp;
    cpu->ip = 0x16AAu;
    observe(ram, cpu, observer, context);
    cpu->di = word_at(ram, cpu->ss, (uint16_t)(cpu->bx + 8u));
    cpu->es = word_at(ram, cpu->ss, (uint16_t)(cpu->bx + 0x0Au));
    cpu->ip = 0x16AEu;
    observe(ram, cpu, observer, context);
    cpu->cx = word_at(ram, cpu->ss, (uint16_t)(cpu->bx + 6u));
    cpu->ip = 0x16B2u;
    observe(ram, cpu, observer, context);
    value = ram[physical(cpu->ss, (uint16_t)(cpu->bx + 4u))];
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
    cpu->ip = 0x16B6u;
    observe(ram, cpu, observer, context);
    cpu->flags &= (uint16_t)~0x0400u;
    cpu->ip = 0x16B7u;
    observe(ram, cpu, observer, context);
    while (cpu->cx != 0u) {
        ram[physical(cpu->es, cpu->di)] = value;
        cpu->di = (uint16_t)(cpu->di + 1u);
        cpu->cx = (uint16_t)(cpu->cx - 1u);
        observe(ram, cpu, observer, context);
    }
    cpu->ip = 0x16B9u;
    observe(ram, cpu, observer, context);
    return_ip = word_at(ram, cpu->ss, cpu->sp);
    return_cs = word_at(ram, cpu->ss, (uint16_t)(cpu->sp + 2u));
    cpu->sp = (uint16_t)(cpu->sp + 0x0Cu);
    cpu->ip = return_ip;
    cpu->cs = return_cs;
    observe(ram, cpu, observer, context);
    return PR_R464_2632_OK;
}
