#include "poolrad/replay/slot_a_runner_first_dispatch.h"
#include <string.h>
#include <stdlib.h>

static size_t at(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & 0xfffffu;
}
pr_slot_a_runner_prefix_status pr_slot_a_runner_first_dispatch(
    uint8_t *ram, size_t size, const uint8_t *code, size_t code_size,
    pr_u1c_3171_cpu *cpu)
{
    size_t base;
    uint8_t *scratch;
    pr_u1c_3171_cpu staged;
    int result;
    if (!ram || !code || !cpu || size != 0x100000u || code_size != 66u ||
        cpu->ip != 0x3171u || ram[at(cpu->ds,0x49ffu)] != 0u)
        return PR_SLOT_A_RUNNER_REFUSED;
    base = at(cpu->cs,0x3171u);
    if (base > size - 66u || memcmp(ram + base, code, 66u))
        return PR_SLOT_A_RUNNER_REFUSED;
    /* With both runner stop gates clear, the existing model reaches the
     * NULL-dispatch boundary after pushing CS and return IP. */
    scratch = malloc(size);
    if (!scratch) return PR_SLOT_A_RUNNER_REFUSED;
    memcpy(scratch, ram, size);
    staged = *cpu;
    result = pr_u1c_run_3171(scratch, size, &staged, 0u, NULL, NULL);
    if (result != 1) {
        free(scratch);
        return PR_SLOT_A_RUNNER_REFUSED;
    }
    staged.ip = 0x2f2du;
    memcpy(ram, scratch, size);
    *cpu = staged;
    free(scratch);
    return PR_SLOT_A_RUNNER_FIRST_DISPATCH;
}
