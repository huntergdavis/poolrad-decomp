#include "poolrad/recovered/overlay_u3c2_jump_successor_0913.h"

#include <assert.h>
#include <string.h>

int main(void)
{
    static const uint16_t invalid[] = {0, 0x0909, 0x0912, 0x0914, 0x0918, 0xFFFF};
    unsigned flags, i;
    assert(pr_u3c2_jump_successor_0913(NULL) == PR_U3C2_0913_JUMP_ARGUMENT);
    for (flags = 0; flags < 65536u; ++flags) {
        pr_u6c_3f31_cpu cpu = {
            (uint16_t)flags, (uint16_t)~flags, 0xFFFF, 0, 0x8000, 0x7FFF,
            (uint16_t)(flags * 17u), (uint16_t)(flags * 31u),
            (uint16_t)(flags ^ 0xF000u), 0x0913,
            (uint16_t)(flags ^ 0x1000u), 0x3000, (uint16_t)(flags ^ 0xFFFFu),
            (uint16_t)flags
        }, want = cpu;
        want.ip = 0x0918;
        assert(pr_u3c2_jump_successor_0913(&cpu) == PR_U3C2_0913_JUMP_OK);
        assert(memcmp(&cpu, &want, sizeof(cpu)) == 0);
    }
    for (i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
        pr_u6c_3f31_cpu cpu = {0}, before;
        cpu.ip = invalid[i];
        cpu.flags = 0xFFFF;
        before = cpu;
        assert(pr_u3c2_jump_successor_0913(&cpu) == PR_U3C2_0913_JUMP_ARGUMENT);
        assert(memcmp(&cpu, &before, sizeof(cpu)) == 0);
    }
    return 0;
}
