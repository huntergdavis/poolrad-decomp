/* DOS v1.3 GAME.OVR U3B0 +088A: 36 8A 45 1E. */
#include "poolrad/recovered/overlay_u3b0_088a_ss_al_read.h"

#define DEFINED_XOR_FLAGS 0x08C5u /* OF SF ZF PF CF; AF undefined */

pr_u3b0_088a_ss_al_read_status pr_u3b0_088a_ss_al_read(
    uint8_t *ram, size_t ram_size, pr_u3b0_cpu *cpu)
{
    uint16_t offset;
    uint32_t code_base, physical;

    if (ram == NULL || cpu == NULL || ram_size < PR_U3B0_RAM_SIZE ||
        cpu->ip != 0x088Au || (cpu->ax & 0xFF00u) != 0u ||
        cpu->dx != cpu->ax ||
        (cpu->flags & DEFINED_XOR_FLAGS) != 0x0044u)
        return PR_U3B0_088A_SS_AL_READ_ARGUMENT;

    code_base = (uint32_t)cpu->cs << 4;
    if (ram[(code_base + 0x088Au) & 0xFFFFFu] != 0x36u ||
        ram[(code_base + 0x088Bu) & 0xFFFFFu] != 0x8Au ||
        ram[(code_base + 0x088Cu) & 0xFFFFFu] != 0x45u ||
        ram[(code_base + 0x088Du) & 0xFFFFFu] != 0x1Eu)
        return PR_U3B0_088A_SS_AL_READ_ARGUMENT;

    offset = (uint16_t)(cpu->di + 0x1Eu);
    physical = (((uint32_t)cpu->ss << 4) + offset) & 0xFFFFFu;
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | ram[physical]);
    cpu->ip = 0x088Eu;
    return PR_U3B0_088A_SS_AL_READ_OK;
}
