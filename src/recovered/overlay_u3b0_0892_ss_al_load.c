/* DOS v1.3 GAME.OVR U3B0 +0892: 8A 46 F7 (MOV AL,SS:[BP-9]). */
#include "poolrad/recovered/overlay_u3b0_0892_ss_al_load.h"

pr_u3b0_0892_ss_al_load_status pr_u3b0_0892_ss_al_load(
    uint8_t *ram, size_t ram_size, pr_u3b0_cpu *cpu)
{
    uint16_t offset;
    uint32_t code_base, physical;

    if (ram == NULL || cpu == NULL || ram_size < PR_U3B0_RAM_SIZE ||
        cpu->ip != 0x0892u)
        return PR_U3B0_0892_SS_AL_LOAD_ARGUMENT;

    code_base = (uint32_t)cpu->cs << 4;
    if (ram[(code_base + 0x0892u) & 0xFFFFFu] != 0x8Au ||
        ram[(code_base + 0x0893u) & 0xFFFFFu] != 0x46u ||
        ram[(code_base + 0x0894u) & 0xFFFFFu] != 0xF7u)
        return PR_U3B0_0892_SS_AL_LOAD_ARGUMENT;

    offset = (uint16_t)(cpu->bp - 9u);
    physical = (((uint32_t)cpu->ss << 4) + offset) & 0xFFFFFu;
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | ram[physical]);
    cpu->ip = 0x0895u;
    return PR_U3B0_0892_SS_AL_LOAD_OK;
}
