/* Register transfer only; docs/re/U3C6_MOV_SP_BP_0062.md. */
#include "poolrad/recovered/overlay_u3c6_mov_sp_bp_0062.h"
pr_u3c6_0062_status pr_u3c6_mov_sp_bp_0062_fixture(
    uint8_t *ram,size_t ram_size,pr_u6c_3f31_cpu *c)
{
    if(!ram || !c || ram_size!=PR_U6C_3F31_RAM_SIZE || c->ip!=0x0062u)
        return PR_U3C6_0062_ARGUMENT;
    c->sp=c->bp;c->ip=0x0064u;
    return PR_U3C6_0062_NEXT_PENDING;
}
