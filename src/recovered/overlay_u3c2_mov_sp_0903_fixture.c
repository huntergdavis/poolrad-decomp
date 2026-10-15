/* Bounded supplied-state source gate. */
#include "poolrad/recovered/overlay_u3c2_mov_sp_0903_fixture.h"
pr_u3c2_mov_sp_0903_status pr_u3c2_mov_sp_0903_fixture(
    uint8_t *ram, size_t ram_size, pr_u6c_3f31_cpu *cpu, pr_u3c2_0903_af_policy af_policy)
{
    if (!ram || !cpu || ram_size < PR_U6C_3F31_RAM_SIZE || cpu->ip != PR_U3C2_0903_ENTRY)
        return PR_U3C2_0903_ARGUMENT;
    if (af_policy != PR_U3C2_0903_AF_PRESERVE_SUPPLIED) return PR_U3C2_0903_ARGUMENT;
    cpu->sp=cpu->bp;
    cpu->ip=PR_U3C2_0903_STOP;
    return PR_U3C2_0903_OK;
}
