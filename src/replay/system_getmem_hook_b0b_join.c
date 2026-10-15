#include "poolrad/replay/system_getmem_hook_b0b_join.h"
#include "poolrad/replay/slot_a_saved_copy_1965.h"
static uint8_t *frame(pr_slot_a_directory_state *s, uint16_t off)
{
    size_t start = ((size_t)s->cpu.ss << 4) + off;
    if ((size_t)off + 4u > 0x10000u || start > s->ram_size || 4u > s->ram_size - start) return NULL;
    return s->ram + start;
}
static void put(uint8_t *p, uint16_t value)
{ p[0] = (uint8_t)value; p[1] = (uint8_t)(value >> 8); }
pr_getmem_hook_b0b_status pr_system_getmem_hook_b0b_step(
    pr_slot_a_directory_state *s, pr_system_getmem_report *report)
{
    pr_slot_a_saved_copy_report helper_report;
    pr_slot_a_saved_copy_status helper_status;
    pr_getmem_real_hook_status hook_status;
    uint8_t *p;
    if (!s || !report) return PR_GETMEM_HOOK_B0B_ARGUMENT;
    *report = (pr_system_getmem_report){0};
    if (!s->ram || s->ram_size > 0x100000u) return PR_GETMEM_HOOK_B0B_BLOCKED_MEMORY;
    if (s->cpu.cs == 0x398Au && s->cpu.ip == 0x64u) {
        *report = (pr_system_getmem_report){1u, 0u, 0x64u, 0x0709u, 0xBF5u, 0x69u};
        return PR_GETMEM_HOOK_B0B_BLOCKED_PRINTER;
    }
    if (s->cpu.cs == 0x398Au && s->cpu.ip == 0x5Fu) {
        p = frame(s, (uint16_t)(s->cpu.sp - 4u));
        if (!p) return PR_GETMEM_HOOK_B0B_BLOCKED_MEMORY;
        put(p, 0x64u); put(p + 2u, s->cpu.cs);
        s->cpu.sp = (uint16_t)(s->cpu.sp - 4u);
        s->cpu.cs = 0x131Cu; s->cpu.ip = 0xB0Bu;
        return PR_GETMEM_HOOK_B0B_STEPPED;
    }
    if (s->cpu.cs == 0x131Cu && s->cpu.ip >= 0xB0Bu && s->cpu.ip <= 0xB22u) {
        helper_status = pr_slot_a_saved_copy_1965_step(s, &helper_report);
        switch (helper_status) {
        case PR_SLOT_A_SAVED_COPY_STEPPED: return PR_GETMEM_HOOK_B0B_STEPPED;
        case PR_SLOT_A_SAVED_COPY_BLOCKED_MEMORY: return PR_GETMEM_HOOK_B0B_BLOCKED_MEMORY;
        case PR_SLOT_A_SAVED_COPY_ARGUMENT: return PR_GETMEM_HOOK_B0B_ARGUMENT;
        default: return PR_GETMEM_HOOK_B0B_BLOCKED_PC;
        }
    }
    hook_status = pr_system_getmem_real_hook_step(s, report);
    switch (hook_status) {
    case PR_GETMEM_REAL_HOOK_STEPPED: return PR_GETMEM_HOOK_B0B_STEPPED;
    case PR_GETMEM_REAL_HOOK_BLOCKED_POSTHOOK: return PR_GETMEM_HOOK_B0B_BLOCKED_POSTHOOK;
    case PR_GETMEM_REAL_HOOK_BLOCKED_WRAPPER: return PR_GETMEM_HOOK_B0B_BLOCKED_WRAPPER;
    case PR_GETMEM_REAL_HOOK_BLOCKED_TARGET: return PR_GETMEM_HOOK_B0B_BLOCKED_TARGET;
    case PR_GETMEM_REAL_HOOK_BLOCKED_MEMORY: return PR_GETMEM_HOOK_B0B_BLOCKED_MEMORY;
    case PR_GETMEM_REAL_HOOK_ARGUMENT: return PR_GETMEM_HOOK_B0B_ARGUMENT;
    default: *report = (pr_system_getmem_report){0}; return PR_GETMEM_HOOK_B0B_BLOCKED_PC;
    }
}
