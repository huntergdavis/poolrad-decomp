/* One bounded supplied-state instruction, no guest execution. */
#include "poolrad/recovered/overlay_u3c2_cmp_al_stack_05f3_fixture.h"
pr_u3c2_cmp_al_stack_05f3_status pr_u3c2_cmp_al_stack_05f3_fixture(
    uint8_t *ram, size_t ram_size, pr_u6c_3f31_cpu *cpu, pr_u3c2_05f3_af_policy af_policy)
{
    if (!ram || !cpu || ram_size < PR_U6C_3F31_RAM_SIZE || cpu->ip != PR_U3C2_05F3_ENTRY)
        return PR_U3C2_05F3_ARGUMENT;
    if (af_policy != PR_U3C2_05F3_AF_CMP_DEFINED) return PR_U3C2_05F3_ARGUMENT;
    size_t at=(((size_t)cpu->ss << 4u)+(uint16_t)(cpu->bp+10u)) &
              (PR_U6C_3F31_RAM_SIZE-1u);
    uint8_t a=(uint8_t)cpu->ax,b=ram[at],r=(uint8_t)(a-b);
    unsigned parity=(unsigned)(r ^ (r >> 4u)) & 15u;
    uint16_t flags=(uint16_t)(cpu->flags & (uint16_t)~0x08D5u);
    if (a < b) flags |= 1u;
    if ((0x9669u >> parity) & 1u) flags |= 4u;
    if ((a ^ b ^ r) & 0x10u) flags |= 0x10u;
    if (r == 0) flags |= 0x40u;
    if (r & 0x80u) flags |= 0x80u;
    if ((a ^ b) & (a ^ r) & 0x80u) flags |= 0x800u;
    cpu->flags=flags;
    cpu->ip=PR_U3C2_05F3_STOP;
    return PR_U3C2_05F3_OK;
}
