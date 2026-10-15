/* DOS v1.3 GAME.OVR U3B0 +0887: 8B 7E 06. */
#include "poolrad/recovered/overlay_u3b0_0887_ss_di_read.h"

#define DEFINED_XOR_FLAGS 0x08C5u /* OF SF ZF PF CF; AF undefined */

pr_u3b0_0887_ss_di_read_status pr_u3b0_0887_ss_di_read(
    uint8_t *ram, size_t ram_size, pr_u3b0_cpu *cpu)
{
    uint16_t offset, high_offset;
    uint32_t code_base, base, low_physical, high_physical;

    if (ram == NULL || cpu == NULL || ram_size < PR_U3B0_RAM_SIZE ||
        cpu->ip != 0x0887u || (cpu->ax & 0xFF00u) != 0u ||
        cpu->dx != cpu->ax ||
        (cpu->flags & DEFINED_XOR_FLAGS) != 0x0044u)
        return PR_U3B0_0887_SS_DI_READ_ARGUMENT;

    code_base = (uint32_t)cpu->cs << 4;
    if (ram[(code_base + 0x0887u) & 0xFFFFFu] != 0x8Bu ||
        ram[(code_base + 0x0888u) & 0xFFFFFu] != 0x7Eu ||
        ram[(code_base + 0x0889u) & 0xFFFFFu] != 0x06u)
        return PR_U3B0_0887_SS_DI_READ_ARGUMENT;

    offset = (uint16_t)(cpu->bp + 6u);
    high_offset = (uint16_t)(offset + 1u);
    base = (uint32_t)cpu->ss << 4;
    low_physical = (base + offset) & 0xFFFFFu;
    high_physical = (base + high_offset) & 0xFFFFFu;
    cpu->di = (uint16_t)((uint16_t)ram[low_physical] |
                         ((uint16_t)ram[high_physical] << 8));
    cpu->ip = 0x088Au;
    return PR_U3B0_0887_SS_DI_READ_OK;
}
