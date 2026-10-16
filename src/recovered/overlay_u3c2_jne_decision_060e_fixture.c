/* Source-only bounded gate; no original/native execution. */
#include "poolrad/recovered/overlay_u3c2_jne_decision_060e_fixture.h"
pr_u3c2_jne_decision_060e_status pr_u3c2_jne_decision_060e_fixture(
    uint8_t *ram, size_t ram_size, pr_u6c_3f31_cpu *cpu)
{
    if (!ram || !cpu || ram_size < PR_U6C_3F31_RAM_SIZE || cpu->ip != PR_U3C2_060E_ENTRY)
        return PR_U3C2_060E_ARGUMENT;
    cpu->ip = (cpu->flags & 0x0040u) == 0u ? 0x0617u : 0x0610u;
    return PR_U3C2_060E_OK;
}
