#include "poolrad/recovered/overlay_u3c2_jump_successor_0913.h"

pr_u3c2_0913_jump_status pr_u3c2_jump_successor_0913(pr_u6c_3f31_cpu *cpu)
{
    if (!cpu || cpu->ip != 0x0913u)
        return PR_U3C2_0913_JUMP_ARGUMENT;
    cpu->ip = 0x0918u;
    return PR_U3C2_0913_JUMP_OK;
}
