/* One bounded supplied-state instruction, no guest execution. */
#include "poolrad/recovered/overlay_u3c2_cmp_al_zero_061a_fixture.h"
pr_u3c2_cmp_al_zero_061a_status pr_u3c2_cmp_al_zero_061a_fixture(
    uint8_t *ram, size_t ram_size, pr_u6c_3f31_cpu *cpu, pr_u3c2_061a_af_policy af_policy)
{
    if (!ram || !cpu || ram_size < PR_U6C_3F31_RAM_SIZE || cpu->ip != PR_U3C2_061A_ENTRY)
        return PR_U3C2_061A_ARGUMENT;
    if (af_policy != PR_U3C2_061A_AF_CMP_DEFINED) return PR_U3C2_061A_ARGUMENT;
    uint8_t r=(uint8_t)cpu->ax;
    unsigned parity=(unsigned)(r ^ (r >> 4u)) & 15u;
    uint16_t flags=(uint16_t)(cpu->flags & (uint16_t)~0x08D5u);
    if ((0x9669u >> parity) & 1u) flags |= 4u;
    if (r == 0) flags |= 0x40u;
    if (r & 0x80u) flags |= 0x80u;
    cpu->flags=flags;
    cpu->ip=PR_U3C2_061A_STOP;
    return PR_U3C2_061A_OK;
}
