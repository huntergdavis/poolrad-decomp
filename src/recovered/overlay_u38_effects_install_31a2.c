/* Unit 38:+31A2..+38A8; docs/re/U38_EFFECTS_INSTALL_31A2.md. */
#include "poolrad/recovered/overlay_u38_effects_install_31a2.h"

#define MASK (PR_U38_31A2_RAM_SIZE - 1u)

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint16_t read16_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static void write16(uint8_t *ram, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

pr_u38_31a2_status pr_u38_effects_install_31a2(
    uint8_t *ram, size_t ram_size, pr_u38_31a2_cpu *cpu)
{
    static const uint16_t targets[138] = {
        0x002A, 0x002F, 0x0034, 0x0039, 0x0264, 0x003E, 0x0043, 0x0048,
        0x004D, 0x0052, 0x0057, 0x005C, 0x0264, 0x0061, 0x0066, 0x006B,
        0x0070, 0x0075, 0x0264, 0x007A, 0x007F, 0x0084, 0x0089, 0x0264,
        0x008E, 0x0093, 0x0264, 0x0098, 0x009D, 0x00A2, 0x00F7, 0x00A7,
        0x00AC, 0x00B1, 0x00B6, 0x00BB, 0x00C0, 0x005C, 0x00C5, 0x00CA,
        0x00CF, 0x00D4, 0x00D9, 0x00DE, 0x0048, 0x004D, 0x00E3, 0x00E8,
        0x00ED, 0x00F2, 0x00F7, 0x00F7, 0x00F7, 0x00FC, 0x0101, 0x0106,
        0x010B, 0x0110, 0x0115, 0x0264, 0x011A, 0x011F, 0x0264, 0x0124,
        0x0129, 0x012E, 0x0133, 0x0138, 0x013D, 0x0142, 0x0147, 0x014C,
        0x008E, 0x0093, 0x0151, 0x0156, 0x015B, 0x0160, 0x0165, 0x016A,
        0x016F, 0x0174, 0x0179, 0x017E, 0x0183, 0x0188, 0x018D, 0x0192,
        0x0197, 0x019C, 0x0264, 0x01A1, 0x01A6, 0x01AB, 0x01B0, 0x01B5,
        0x01BA, 0x01BF, 0x01C4, 0x01C9, 0x01CE, 0x01D3, 0x01D8, 0x01DD,
        0x01E2, 0x01E7, 0x01EC, 0x01F1, 0x01F6, 0x01FB, 0x0200, 0x0205,
        0x020A, 0x020F, 0x0214, 0x0219, 0x021E, 0x0223, 0x0228, 0x022D,
        0x0232, 0x0237, 0x023C, 0x0241, 0x0246, 0x0264, 0x024B, 0x024B,
        0x024B, 0x0250, 0x0255, 0x024B, 0x024B, 0x025A, 0x024B, 0x025F,
        0x024B, 0x024B
    };
    size_t at;
    unsigned i;
    if (!ram || !cpu || ram_size < PR_U38_31A2_RAM_SIZE)
        return PR_U38_31A2_ARGUMENT;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, cpu->bp);
    cpu->bp = cpu->sp;
    for (i = 0; i < 138u; ++i) {
        uint16_t destination = (uint16_t)(0x682Cu + 4u * i + (i >= 87u ? 4u : 0u));
        cpu->ax = targets[i];
        cpu->dx = read16_at(ram, physical(cpu->cs, (uint16_t)(0x31A9u + 13u * i)));
        write16(ram, cpu->ds, destination, cpu->ax);
        write16(ram, cpu->ds, (uint16_t)(destination + 2u), cpu->dx);
    }
    /* All body instructions are MOV; no emulated arithmetic flags change. */
    cpu->bp = read16_at(ram, physical(cpu->ss, cpu->sp));
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u);
    return PR_U38_31A2_OK;
}
