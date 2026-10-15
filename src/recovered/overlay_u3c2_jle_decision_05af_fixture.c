/* Source-only bounded gate; no original/native execution. */
#include "poolrad/recovered/overlay_u3c2_jle_decision_05af_fixture.h"
pr_u3c2_jle_decision_05af_status pr_u3c2_jle_decision_05af_fixture(
    uint8_t *ram, size_t ram_size, pr_u6c_3f31_cpu *cpu)
{
    if (!ram || !cpu || ram_size < PR_U6C_3F31_RAM_SIZE || cpu->ip != PR_U3C2_05AF_ENTRY)
        return PR_U3C2_05AF_ARGUMENT;
    unsigned sf=(cpu->flags >> 7u)&1u, of=(cpu->flags >> 11u)&1u;
    cpu->ip = (cpu->flags & 0x0040u) || sf != of ? 0x05B8u : 0x05B1u;
    return PR_U3C2_05AF_OK;
}
