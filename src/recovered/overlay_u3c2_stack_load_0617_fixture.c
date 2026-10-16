/* One bounded supplied-state instruction, no guest execution. */
#include "poolrad/recovered/overlay_u3c2_stack_load_0617_fixture.h"
pr_u3c2_stack_load_0617_status pr_u3c2_stack_load_0617_fixture(
    uint8_t *ram, size_t ram_size, pr_u6c_3f31_cpu *cpu, pr_u3c2_0617_af_policy af_policy)
{
    if (!ram || !cpu || ram_size < PR_U6C_3F31_RAM_SIZE || cpu->ip != PR_U3C2_0617_ENTRY)
        return PR_U3C2_0617_ARGUMENT;
    if (af_policy != PR_U3C2_0617_AF_PRESERVE_SUPPLIED) return PR_U3C2_0617_ARGUMENT;
    size_t at=(((size_t)cpu->ss << 4u)+(uint16_t)(cpu->bp+6u)) &
              (PR_U6C_3F31_RAM_SIZE-1u);
    cpu->ax=(uint16_t)((cpu->ax & 0xFF00u) | ram[at]);
    cpu->ip=0x061Au;
    return PR_U3C2_0617_OK;
}
