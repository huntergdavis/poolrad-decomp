#include "poolrad/replay/system_getmem_hook_b0b_join.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) {fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;} } while (0)
static uint8_t ram[655360u], saved[655360u];
static int blocked(pr_slot_a_directory_state *s, pr_getmem_hook_b0b_status expected)
{
    pr_slot_a_directory_cpu cpu = s->cpu;
    pr_system_getmem_report report;
    memcpy(saved, ram, sizeof ram);
    CHECK(pr_system_getmem_hook_b0b_step(s, &report) == expected);
    CHECK(memcmp(&cpu, &s->cpu, sizeof cpu) == 0 && memcmp(saved, ram, sizeof ram) == 0);
    return 0;
}
int main(void)
{
    pr_slot_a_directory_state s = {0};
    pr_system_getmem_report report, before;
    s.ram = ram; s.ram_size = sizeof ram; s.cpu.cs = 0x398Au; s.cpu.ip = 0x64u;
    s.cpu.sp = 0x100u;
    CHECK(blocked(&s, PR_GETMEM_HOOK_B0B_BLOCKED_PRINTER) == 0);
    CHECK(pr_system_getmem_hook_b0b_step(&s, &report) == PR_GETMEM_HOOK_B0B_BLOCKED_PRINTER);
    CHECK(report.dependency_valid && !report.call_is_near && report.call_pc == 0x64u);
    CHECK(report.callee_segment == 0x709u && report.callee_pc == 0xBF5u && report.return_pc == 0x69u);
    s.cpu.ip = 0x5Fu; s.cpu.sp = 2u;
    CHECK(blocked(&s, PR_GETMEM_HOOK_B0B_BLOCKED_MEMORY) == 0);
    s.cpu.sp = 0x100u; s.cpu.flags = 0x7ED7u;
    CHECK(pr_system_getmem_hook_b0b_step(&s, &report) == PR_GETMEM_HOOK_B0B_STEPPED);
    CHECK(s.cpu.cs == 0x131Cu && s.cpu.ip == 0xB0Bu && s.cpu.sp == 0xFCu);
    CHECK(ram[0xFC] == 0x64u && ram[0xFD] == 0 && ram[0xFE] == 0x8Au && ram[0xFF] == 0x39u);
    CHECK(pr_system_getmem_hook_b0b_step(&s, &report) == PR_GETMEM_HOOK_B0B_STEPPED);
    CHECK(!(s.cpu.flags & 0x400u));
    s.cpu.ip = 0xB1Eu; s.cpu.cx = 28u; s.cpu.si = 0xFFF0u; s.cpu.di = 0x200u;
    CHECK(blocked(&s, PR_GETMEM_HOOK_B0B_BLOCKED_MEMORY) == 0);
    s.cpu.si = 0x200u; s.cpu.di = 0xFFF0u;
    CHECK(blocked(&s, PR_GETMEM_HOOK_B0B_BLOCKED_MEMORY) == 0);
    s.cpu.ip = 0xB18u; s.cpu.flags |= 0x400u;
    CHECK(blocked(&s, PR_GETMEM_HOOK_B0B_BLOCKED_PC) == 0);
    s.cpu.ip = 0xB22u; s.cpu.sp = 0xFFFDu;
    CHECK(blocked(&s, PR_GETMEM_HOOK_B0B_BLOCKED_MEMORY) == 0);
    s.cpu.ip = 0x3C7u;
    for (unsigned al = 0; al < 256u; ++al) {
        s.cpu.ax = (uint16_t)(0xA500u | al);
        CHECK(blocked(&s, PR_GETMEM_HOOK_B0B_BLOCKED_POSTHOOK) == 0);
    }
    s.cpu.ip = 0x3B7u;
    CHECK(blocked(&s, PR_GETMEM_HOOK_B0B_BLOCKED_TARGET) == 0);
    s.cpu.ip = 0x248u;
    CHECK(blocked(&s, PR_GETMEM_HOOK_B0B_BLOCKED_WRAPPER) == 0);
    s.ram_size = 0x100001u;
    CHECK(blocked(&s, PR_GETMEM_HOOK_B0B_BLOCKED_MEMORY) == 0);
    s.ram_size = sizeof ram; s.ram = NULL;
    CHECK(blocked(&s, PR_GETMEM_HOOK_B0B_BLOCKED_MEMORY) == 0);
    memset(&report, 0xA5, sizeof report); before = report;
    CHECK(pr_system_getmem_hook_b0b_step(NULL, &report) == PR_GETMEM_HOOK_B0B_ARGUMENT);
    CHECK(memcmp(&report, &before, sizeof report) == 0);
    CHECK(pr_system_getmem_hook_b0b_step(&s, NULL) == PR_GETMEM_HOOK_B0B_ARGUMENT);
    puts("strict callback B0B frame, delegated atomic bounds and printer/post-hook gates passed");
    return 0;
}
