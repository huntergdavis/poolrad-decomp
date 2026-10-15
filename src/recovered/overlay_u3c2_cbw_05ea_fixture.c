/* Bounded supplied-state source gate. */
#include "poolrad/recovered/overlay_u3c2_cbw_05ea_fixture.h"
pr_u3c2_cbw_05ea_status pr_u3c2_cbw_05ea_fixture(
    uint8_t *ram, size_t ram_size, pr_u6c_3f31_cpu *cpu, pr_u3c2_05ea_af_policy af_policy)
{
    if (!ram || !cpu || ram_size < PR_U6C_3F31_RAM_SIZE || cpu->ip != PR_U3C2_05EA_ENTRY)
        return PR_U3C2_05EA_ARGUMENT;
    if (af_policy != PR_U3C2_05EA_AF_PRESERVE_SUPPLIED) return PR_U3C2_05EA_ARGUMENT;
    uint16_t low=(uint16_t)(cpu->ax & 0x00FFu);
    cpu->ax=low<0x80u?low:(uint16_t)(low|0xFF00u);
    cpu->ip=PR_U3C2_05EA_STOP;
    return PR_U3C2_05EA_OK;
}
