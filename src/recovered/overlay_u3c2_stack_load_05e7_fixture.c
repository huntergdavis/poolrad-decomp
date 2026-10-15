/* One bounded supplied-state instruction, no guest execution. */
#include "poolrad/recovered/overlay_u3c2_stack_load_05e7_fixture.h"
pr_u3c2_stack_load_05e7_status pr_u3c2_stack_load_05e7_fixture(
    uint8_t *ram, size_t ram_size, pr_u6c_3f31_cpu *cpu, pr_u3c2_05e7_af_policy af_policy)
{
    if (!ram || !cpu || ram_size < PR_U6C_3F31_RAM_SIZE || cpu->ip != PR_U3C2_05E7_ENTRY)
        return PR_U3C2_05E7_ARGUMENT;
    if (af_policy != PR_U3C2_05E7_AF_PRESERVE_SUPPLIED) return PR_U3C2_05E7_ARGUMENT;
    size_t at=(((size_t)cpu->ss << 4u)+(uint16_t)(cpu->bp+12u)) &
              (PR_U6C_3F31_RAM_SIZE-1u);
    cpu->ax=(uint16_t)((cpu->ax & 0xFF00u) | ram[at]);
    cpu->ip=0x05EAu;
    return PR_U3C2_05E7_OK;
}
