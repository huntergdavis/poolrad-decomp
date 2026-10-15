/* DOS v1.3 GAME.OVR U3B0 +08CE: E9 3B FF -> +080C. */
#include "poolrad/recovered/overlay_u3b0_08ce_jmp.h"

pr_u3b0_08ce_jmp_status pr_u3b0_08ce_jmp(
    uint8_t *ram, size_t ram_size, pr_u3b0_cpu *cpu)
{
    uint32_t code_base;

    if (ram == NULL || cpu == NULL || ram_size < PR_U3B0_RAM_SIZE ||
        cpu->ip != 0x08CEu || (cpu->flags & 0x0001u) != 0u)
        return PR_U3B0_08CE_JMP_ARGUMENT;

    code_base = (uint32_t)cpu->cs << 4;
    if (ram[(code_base + 0x08CEu) & 0xFFFFFu] != 0xE9u ||
        ram[(code_base + 0x08CFu) & 0xFFFFFu] != 0x3Bu ||
        ram[(code_base + 0x08D0u) & 0xFFFFFu] != 0xFFu)
        return PR_U3B0_08CE_JMP_ARGUMENT;

    cpu->ip = (uint16_t)(0x08D1u + 0xFF3Bu);
    return PR_U3B0_08CE_JMP_OK;
}
