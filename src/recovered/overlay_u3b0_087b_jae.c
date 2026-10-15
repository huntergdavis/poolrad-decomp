/* DOS v1.3 GAME.OVR U3B0 +087B: 73 3D. */
#include "poolrad/recovered/overlay_u3b0_087b_jae.h"

pr_u3b0_087b_jae_status pr_u3b0_087b_jae(
    uint8_t *ram, size_t ram_size, pr_u3b0_cpu *cpu)
{
    uint32_t code_base;

    if (ram == NULL || cpu == NULL || ram_size < PR_U3B0_RAM_SIZE ||
        cpu->ip != 0x087Bu)
        return PR_U3B0_087B_JAE_ARGUMENT;

    code_base = (uint32_t)cpu->cs << 4;
    if (ram[(code_base + 0x087Bu) & 0xFFFFFu] != 0x73u ||
        ram[(code_base + 0x087Cu) & 0xFFFFFu] != 0x3Du)
        return PR_U3B0_087B_JAE_ARGUMENT;

    cpu->ip = (cpu->flags & 0x0001u) == 0u ? 0x08BAu : 0x087Du;
    return PR_U3B0_087B_JAE_OK;
}
