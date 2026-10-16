/* GAME.OVR U9E entry8 +0E0E LCALL SYSTEM +11C0; see evidence note. */
#include "poolrad/recovered/overlay_u9e_entry8_system_call_0e0e.h"

#define MASK (PR_SYSTEM_120F_RAM_SIZE - 1u)
#define SYSTEM_CS 0x0AF8u

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static void write16(uint8_t *ram, uint16_t segment, uint16_t offset,
                    uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static int in_code(size_t at, uint16_t cs, unsigned first, unsigned last)
{
    unsigned offset;
    for (offset = first; offset <= last; ++offset)
        if (at == physical(cs, (uint16_t)offset)) return 1;
    return 0;
}

static int span_aliases_code(const pr_u9e_entry8_call_cpu *cpu,
                             uint16_t segment, uint16_t offset, unsigned size)
{
    size_t first = physical(segment, offset);
    unsigned i;
    for (i = 0u; i < size; ++i) {
        size_t at = (first + i) & MASK;
        if (in_code(at, cpu->cs, 0x0DE3u, 0x0E2Du) ||
            in_code(at, SYSTEM_CS, 0x11C0u, 0x11D5u) ||
            in_code(at, SYSTEM_CS, 0x120Fu, 0x1244u))
            return 1;
    }
    return 0;
}

static int aliases_code(const pr_u9e_entry8_call_cpu *cpu)
{
    /* The far call writes CS then IP; SYSTEM writes its near return word
     * and the DS seed DWORD. Each WORD's high byte is physically adjacent. */
    return span_aliases_code(cpu, cpu->ss,
                             (uint16_t)(cpu->sp - 2u), 2u) ||
           span_aliases_code(cpu, cpu->ss,
                             (uint16_t)(cpu->sp - 4u), 2u) ||
           span_aliases_code(cpu, cpu->ss,
                             (uint16_t)(cpu->sp - 6u), 2u) ||
           span_aliases_code(cpu, cpu->ds, 0x4428u, 4u);
}

pr_u9e_entry8_call_status pr_u9e_entry8_system_call_0e0e(
    uint8_t *ram, size_t ram_size, pr_u9e_entry8_call_cpu *cpu)
{
    uint16_t source_cs;
    pr_system_11c0_status result;
    if (ram == NULL || cpu == NULL || ram_size != PR_SYSTEM_120F_RAM_SIZE ||
        cpu->ip != 0x0E0Eu || aliases_code(cpu))
        return PR_U9E_ENTRY8_CALL_ARGUMENT;

    source_cs = cpu->cs;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, source_cs);
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, 0x0E13u);
    cpu->cs = SYSTEM_CS;
    cpu->ip = 0x11C0u;

    result = pr_system_random_fullstate_11c0(ram, ram_size, cpu);
    if (result == PR_SYSTEM_11C0_ARGUMENT)
        return PR_U9E_ENTRY8_CALL_SYSTEM_ARGUMENT;
    if (result == PR_SYSTEM_11C0_CALLEE_FAILED)
        return PR_U9E_ENTRY8_CALL_SYSTEM_FAILED;
    if (result == PR_SYSTEM_11C0_CALLEE_TRANSFER)
        return PR_U9E_ENTRY8_CALL_SYSTEM_TRANSFER;
    if (cpu->cs != source_cs || cpu->ip != 0x0E13u)
        return PR_U9E_ENTRY8_CALL_RETURN_TRANSFER;
    return PR_U9E_ENTRY8_CALL_RETURNED;
}
