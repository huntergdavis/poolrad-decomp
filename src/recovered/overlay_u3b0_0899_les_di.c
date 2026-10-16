/* DOS v1.3 GAME.OVR U3B0 +0899: C4 7E FC. */
#include "poolrad/recovered/overlay_u3b0_0899_les_di.h"

pr_u3b0_0899_les_di_status pr_u3b0_0899_les_di(
    uint8_t *ram, size_t ram_size, pr_u3b0_cpu *cpu)
{
    uint16_t offset;
    uint32_t base, physical;
    uint8_t bytes[4];
    size_t i;

    if (ram == NULL || cpu == NULL || ram_size < PR_U3B0_RAM_SIZE ||
        cpu->ip != 0x0899u)
        return PR_U3B0_0899_LES_DI_ARGUMENT;

    if (ram[((((uint32_t)cpu->cs << 4) + 0x0899u) & 0xFFFFFu)] != 0xC4u ||
        ram[((((uint32_t)cpu->cs << 4) + 0x089Au) & 0xFFFFFu)] != 0x7Eu ||
        ram[((((uint32_t)cpu->cs << 4) + 0x089Bu) & 0xFFFFFu)] != 0xFCu)
        return PR_U3B0_0899_LES_DI_ARGUMENT;

    offset = (uint16_t)(cpu->bp - 4u);
    base = (uint32_t)cpu->ss << 4;
    for (i = 0; i < 4u; ++i) {
        physical = (base + (uint16_t)(offset + i)) & 0xFFFFFu;
        bytes[i] = ram[physical];
    }
    cpu->di = (uint16_t)((uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8));
    cpu->es = (uint16_t)((uint16_t)bytes[2] | ((uint16_t)bytes[3] << 8));
    cpu->ip = 0x089Cu;
    return PR_U3B0_0899_LES_DI_OK;
}
