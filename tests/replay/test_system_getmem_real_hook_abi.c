#include "poolrad/replay/system_getmem_real_hook_abi.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) {fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;} } while (0)
static uint8_t ram[655360u], saved[655360u];
static int blocked(pr_slot_a_directory_state *s, pr_getmem_real_hook_status status)
{
    pr_slot_a_directory_cpu cpu = s->cpu;
    pr_system_getmem_report report;
    memcpy(saved, ram, sizeof ram);
    CHECK(pr_system_getmem_real_hook_step(s, &report) == status);
    CHECK(memcmp(&cpu, &s->cpu, sizeof cpu) == 0 && memcmp(saved, ram, sizeof ram) == 0);
    return 0;
}
int main(void)
{
    pr_slot_a_directory_state s = {0};
    pr_system_getmem_report report, before;
    s.ram = ram; s.ram_size = sizeof ram; s.cpu.cs = 0x131Cu; s.cpu.ip = 0x3B7u;
    s.cpu.sp = 0x100u;
    CHECK(blocked(&s, PR_GETMEM_REAL_HOOK_BLOCKED_TARGET) == 0);
    CHECK(pr_system_getmem_real_hook_step(&s, &report) == PR_GETMEM_REAL_HOOK_BLOCKED_TARGET);
    CHECK(report.dependency_valid && report.callee_segment == 0u && report.return_pc == 0x3BBu);
    ram[0x4414] = 0x25u; ram[0x4416] = 0x3Au; ram[0x4417] = 8u;
    s.cpu.sp = 2u;
    CHECK(blocked(&s, PR_GETMEM_REAL_HOOK_BLOCKED_MEMORY) == 0);
    s.cpu.sp = 0x100u;
    CHECK(pr_system_getmem_real_hook_step(&s, &report) == PR_GETMEM_REAL_HOOK_STEPPED);
    CHECK(s.cpu.sp == 0xFCu && s.cpu.cs == 0x83Au && s.cpu.ip == 0x25u);
    CHECK(ram[0xFC] == 0xBBu && ram[0xFD] == 3u && ram[0xFE] == 0x1Cu && ram[0xFF] == 0x13u);
    s.cpu.cs = 0x398Au; s.cpu.ip = 0x5Fu;
    CHECK(blocked(&s, PR_GETMEM_REAL_HOOK_BLOCKED_STRING_COPY) == 0);
    CHECK(pr_system_getmem_real_hook_step(&s, &report) == PR_GETMEM_REAL_HOOK_BLOCKED_STRING_COPY);
    CHECK(report.dependency_valid && !report.call_is_near && report.callee_segment == 0xAF8u);
    CHECK(report.callee_pc == 0xB0Bu && report.return_pc == 0x64u);
    s.cpu.cs = 0x131Cu; s.cpu.ip = 0x3C7u;
    for (unsigned al = 0; al < 256u; ++al) {
        s.cpu.ax = (uint16_t)(0xA500u | al);
        CHECK(blocked(&s, PR_GETMEM_REAL_HOOK_BLOCKED_POSTHOOK) == 0);
    }
    s.cpu.ip = 0x248u;
    CHECK(blocked(&s, PR_GETMEM_REAL_HOOK_BLOCKED_WRAPPER) == 0);
    s.cpu.cs = 0x398Au; s.cpu.ip = 0x48u; s.cpu.bp = 0xFFF9u;
    CHECK(blocked(&s, PR_GETMEM_REAL_HOOK_BLOCKED_MEMORY) == 0);
    s.cpu.ip = 0xA3u; s.cpu.bp = 1u;
    CHECK(blocked(&s, PR_GETMEM_REAL_HOOK_BLOCKED_MEMORY) == 0);
    s.cpu.ip = 0xA9u; s.cpu.sp = 0xFFFDu;
    CHECK(blocked(&s, PR_GETMEM_REAL_HOOK_BLOCKED_MEMORY) == 0);
    s.cpu.ip = 0x64u;
    CHECK(blocked(&s, PR_GETMEM_REAL_HOOK_BLOCKED_PC) == 0);
    s.ram_size = 0x100001u;
    CHECK(blocked(&s, PR_GETMEM_REAL_HOOK_BLOCKED_MEMORY) == 0);
    s.ram_size = sizeof ram; s.ram = NULL;
    CHECK(blocked(&s, PR_GETMEM_REAL_HOOK_BLOCKED_MEMORY) == 0);
    memset(&report, 0xA5, sizeof report); before = report;
    CHECK(pr_system_getmem_real_hook_step(NULL, &report) == PR_GETMEM_REAL_HOOK_ARGUMENT);
    CHECK(memcmp(&report, &before, sizeof report) == 0);
    CHECK(pr_system_getmem_real_hook_step(&s, NULL) == PR_GETMEM_REAL_HOOK_ARGUMENT);
    puts("strict real-hook target/frame bounds, dependency reports and all-AL post-hook gates passed");
    return 0;
}
