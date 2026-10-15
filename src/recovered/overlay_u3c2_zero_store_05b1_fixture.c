/* One supplied-state byte store; no original/native execution. */
#include "poolrad/recovered/overlay_u3c2_zero_store_05b1_fixture.h"
pr_u3c2_zero_store_05b1_status pr_u3c2_zero_store_05b1_fixture(
    uint8_t *ram, size_t ram_size, pr_u6c_3f31_cpu *cpu, pr_u3c2_05b1_af_policy af_policy)
{
    if (!ram || !cpu || ram_size < PR_U6C_3F31_RAM_SIZE || cpu->ip != PR_U3C2_05B1_ENTRY)
        return PR_U3C2_05B1_ARGUMENT;
    if (af_policy != PR_U3C2_05B1_AF_PRESERVE_SUPPLIED) return PR_U3C2_05B1_ARGUMENT;
    size_t address=(((size_t)cpu->ss<<4u)+(uint16_t)(cpu->bp-1u))&0xFFFFFu;
    ram[address]=0;
    cpu->ip=PR_U3C2_05B1_STOP;
    return PR_U3C2_05B1_OK;
}
