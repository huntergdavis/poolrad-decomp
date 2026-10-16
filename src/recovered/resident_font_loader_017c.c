#include "poolrad/recovered/resident_font_loader_017c.h"

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & (PR_FONT017C_RAM_SIZE - 1u);
}

static uint16_t word_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] |
        ((uint16_t)ram[(at + 1u) & (PR_FONT017C_RAM_SIZE - 1u)] << 8));
}

static uint16_t read_word(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return word_at(ram, physical(segment, offset));
}

static void push(uint8_t *ram, pr_font017c_cpu *cpu, uint16_t value)
{
    size_t at;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    at = physical(cpu->ss, cpu->sp);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & (PR_FONT017C_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static uint16_t subtract_flags(uint16_t incoming, uint16_t left, uint16_t right)
{
    uint16_t result = (uint16_t)(left - right);
    uint16_t flags = incoming & (uint16_t)~0x08D5u;
    unsigned ones = 0u;
    for (unsigned bit = 0u; bit < 8u; ++bit)
        ones += (result >> bit) & 1u;
    if (left < right) flags |= 1u;
    if ((ones & 1u) == 0u) flags |= 4u;
    if (((left ^ right ^ result) & 0x10u) != 0u) flags |= 0x10u;
    if (result == 0u) flags |= 0x40u;
    if ((result & 0x8000u) != 0u) flags |= 0x80u;
    if (((left ^ right) & (left ^ result) & 0x8000u) != 0u)
        flags |= 0x0800u;
    return flags;
}

static void les(uint8_t *ram, pr_font017c_cpu *cpu,
                uint16_t segment, uint16_t offset)
{
    size_t at = physical(segment, offset);
    cpu->di = word_at(ram, at);
    cpu->es = word_at(ram, (at + 2u) & (PR_FONT017C_RAM_SIZE - 1u));
}

static pr_font017c_status invoke(
    uint8_t *ram, size_t size, pr_font017c_cpu *cpu, uint16_t bias,
    pr_font017c_callee callee, void *context, unsigned ordinal,
    uint16_t call_ip, uint16_t target_segment, uint16_t target_offset)
{
    pr_font017c_call call = {
        ordinal, call_ip, (uint16_t)(target_segment + bias),
        target_offset, cpu->cs, (uint16_t)(call_ip + 5u)
    };
    push(ram, cpu, call.return_cs);
    push(ram, cpu, call.return_ip);
    cpu->cs = call.target_segment;
    cpu->ip = call.target_offset;
    if (callee(context, &call, ram, size, cpu) != 0)
        return PR_FONT017C_CALLEE;
    if (cpu->cs != call.return_cs || cpu->ip != call.return_ip)
        return PR_FONT017C_RETURN;
    return PR_FONT017C_OK;
}

pr_font017c_status pr_resident_font_loader_017c(
    uint8_t *ram, size_t ram_size, pr_font017c_cpu *cpu,
    uint16_t load_segment, pr_font017c_callee callee, void *context)
{
    pr_font017c_status status;
    uint16_t size;
    unsigned ordinal = 0u;
    if (ram == NULL || cpu == NULL || callee == NULL ||
        ram_size < PR_FONT017C_RAM_SIZE || cpu->ip != 0x017Cu)
        return PR_FONT017C_ARGUMENT;
#define CALL(site, segment, offset) do { \
    status = invoke(ram, ram_size, cpu, load_segment, callee, context, \
                    ++ordinal, site, segment, offset); \
    if (status != PR_FONT017C_OK) return status; \
} while (0)

    push(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->flags = subtract_flags(cpu->flags, cpu->sp, 0x12u);
    cpu->sp = (uint16_t)(cpu->sp - 0x12u);
    cpu->di = (uint16_t)(cpu->bp - 0xCu);
    push(ram, cpu, cpu->ss); push(ram, cpu, cpu->di);
    cpu->di = 0x0171u;
    push(ram, cpu, cpu->cs); push(ram, cpu, cpu->di);
    CALL(0x018Cu, 0x0AF8u, 0x0B0Bu);
    cpu->di = (uint16_t)(cpu->bp - 0x11u);
    push(ram, cpu, cpu->ss); push(ram, cpu, cpu->di);
    cpu->di = 0x0177u;
    push(ram, cpu, cpu->cs); push(ram, cpu, cpu->di);
    CALL(0x019Bu, 0x0AF8u, 0x0B0Bu);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | 0xC9u);
    push(ram, cpu, cpu->ax);
    cpu->di = (uint16_t)(cpu->bp - 2u);
    push(ram, cpu, cpu->ss); push(ram, cpu, cpu->di);
    cpu->di = (uint16_t)(cpu->bp - 6u);
    push(ram, cpu, cpu->ss); push(ram, cpu, cpu->di);
    CALL(0x01ADu, 0x0802u, 0x0716u);
    size = read_word(ram, cpu->ss, (uint16_t)(cpu->bp - 2u));
    cpu->flags = subtract_flags(cpu->flags, size, 0u);
    if (size != 0u) {
        les(ram, cpu, cpu->ss, (uint16_t)(cpu->bp - 6u));
        push(ram, cpu, cpu->es); push(ram, cpu, cpu->di);
        les(ram, cpu, cpu->ds, 0x5E20u);
        push(ram, cpu, cpu->es); push(ram, cpu, cpu->di);
        push(ram, cpu, read_word(ram, cpu->ss, (uint16_t)(cpu->bp - 2u)));
        CALL(0x01C8u, 0x0AF8u, 0x1685u);
        push(ram, cpu, read_word(ram, cpu->ss, (uint16_t)(cpu->bp - 4u)));
        push(ram, cpu, read_word(ram, cpu->ss, (uint16_t)(cpu->bp - 6u)));
        push(ram, cpu, read_word(ram, cpu->ss, (uint16_t)(cpu->bp - 2u)));
        CALL(0x01D6u, 0x0AF8u, 0x0254u);
    }
    cpu->sp = cpu->bp;
    cpu->bp = read_word(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    cpu->ip = read_word(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    cpu->cs = read_word(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
#undef CALL
    return PR_FONT017C_OK;
}
