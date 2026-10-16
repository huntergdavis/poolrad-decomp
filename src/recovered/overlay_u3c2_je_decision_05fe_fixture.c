/* Source-only bounded gate; no original/native execution. */
#include "poolrad/recovered/overlay_u3c2_je_decision_05fe_fixture.h"
pr_u3c2_je_decision_05fe_status pr_u3c2_je_decision_05fe_fixture(
    uint8_t *ram, size_t ram_size, pr_u6c_3f31_cpu *cpu)
{
    if (!ram || !cpu || ram_size < PR_U6C_3F31_RAM_SIZE || cpu->ip != PR_U3C2_05FE_ENTRY)
        return PR_U3C2_05FE_ARGUMENT;
    cpu->ip = (cpu->flags & 0x0040u) != 0u ? 0x0610u : 0x0600u;
    return PR_U3C2_05FE_OK;
}
