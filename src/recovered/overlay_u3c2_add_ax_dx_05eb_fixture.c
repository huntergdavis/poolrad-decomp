/* Bounded supplied-state arithmetic, without guest execution. */
#include "poolrad/recovered/overlay_u3c2_add_ax_dx_05eb_fixture.h"
pr_u3c2_add_ax_dx_05eb_status pr_u3c2_add_ax_dx_05eb_fixture(
    uint8_t *ram, size_t ram_size, pr_u6c_3f31_cpu *cpu, pr_u3c2_05eb_af_policy af_policy)
{
    if (!ram || !cpu || ram_size < PR_U6C_3F31_RAM_SIZE || cpu->ip != PR_U3C2_05EB_ENTRY)
        return PR_U3C2_05EB_ARGUMENT;
    if (af_policy != PR_U3C2_05EB_AF_ADD_DEFINED) return PR_U3C2_05EB_ARGUMENT;
    uint16_t a=cpu->ax, b=cpu->dx;
    uint32_t sum=(uint32_t)a+b;
    uint16_t result=(uint16_t)sum;
    uint8_t parity=(uint8_t)result;
    parity^=(uint8_t)(parity>>4u);
    parity^=(uint8_t)(parity>>2u);
    parity^=(uint8_t)(parity>>1u);
    cpu->flags&=(uint16_t)~0x08D5u;
    if (sum>0xFFFFu) cpu->flags|=0x0001u;
    if (!(parity&1u)) cpu->flags|=0x0004u;
    if (((a^b^result)&0x10u)!=0u) cpu->flags|=0x0010u;
    if (result==0u) cpu->flags|=0x0040u;
    if ((result&0x8000u)!=0u) cpu->flags|=0x0080u;
    if (((~(a^b))&(a^result)&0x8000u)!=0u) cpu->flags|=0x0800u;
    cpu->ax=result;
    cpu->ip=PR_U3C2_05EB_STOP;
    return PR_U3C2_05EB_OK;
}
