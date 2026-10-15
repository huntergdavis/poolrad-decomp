/* Bounded supplied-state source gate. */
#include "poolrad/recovered/overlay_u3c2_mov_dx_05e5_fixture.h"
pr_u3c2_mov_dx_05e5_status pr_u3c2_mov_dx_05e5_fixture(
    uint8_t *ram, size_t ram_size, pr_u6c_3f31_cpu *cpu, pr_u3c2_05e5_af_policy af_policy)
{
    if (!ram || !cpu || ram_size < PR_U6C_3F31_RAM_SIZE || cpu->ip != PR_U3C2_05E5_ENTRY)
        return PR_U3C2_05E5_ARGUMENT;
    if (af_policy != PR_U3C2_05E5_AF_PRESERVE_SUPPLIED) return PR_U3C2_05E5_ARGUMENT;
    cpu->dx=cpu->ax;
    cpu->ip=PR_U3C2_05E5_STOP;
    return PR_U3C2_05E5_OK;
}
