#include "poolrad/replay/system_getmem_23f_abi.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) {fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;} } while (0)
static uint8_t ram[655360u], saved_ram[655360u];
static int unchanged(pr_slot_a_directory_state *s, pr_system_getmem_status expected)
{
    pr_slot_a_directory_cpu before = s->cpu;
    pr_system_getmem_report report;
    memset(&report, 0xA5, sizeof report); memcpy(saved_ram, ram, sizeof ram);
    CHECK(pr_system_getmem_23f_step(s, &report) == expected);
    CHECK(memcmp(&before, &s->cpu, sizeof before) == 0 && memcmp(saved_ram, ram, sizeof ram) == 0);
    if (expected != PR_SYSTEM_GETMEM_BLOCKED_CORE && expected != PR_SYSTEM_GETMEM_BLOCKED_NORMALIZER)
        CHECK(!report.dependency_valid);
    return 0;
}
int main(void)
{
    pr_slot_a_directory_state s = {0};
    pr_system_getmem_report report, old_report;
    s.ram = ram; s.ram_size = sizeof ram; s.cpu.cs = 0x2A8Eu; s.cpu.ip = 0x19B5u; s.cpu.sp = 2u;
    CHECK(unchanged(&s, PR_SYSTEM_GETMEM_BLOCKED_MEMORY) == 0);
    s.cpu.sp = 0x100u;
    CHECK(pr_system_getmem_23f_step(&s, &report) == PR_SYSTEM_GETMEM_STEPPED);
    CHECK(s.cpu.cs == 0x131Cu && s.cpu.ip == 0x23Fu && s.cpu.sp == 0xFCu);
    CHECK(ram[0xFCu] == 0xBAu && ram[0xFDu] == 0x19u && ram[0xFEu] == 0x8Eu && ram[0xFFu] == 0x2Au);
    s.cpu.ip = 0x245u; s.cpu.sp = 1u;
    CHECK(unchanged(&s, PR_SYSTEM_GETMEM_BLOCKED_CORE) == 0);
    CHECK(pr_system_getmem_23f_step(&s, &report) == PR_SYSTEM_GETMEM_BLOCKED_CORE);
    CHECK(report.dependency_valid && report.call_is_near && report.call_pc == 0x245u);
    CHECK(report.callee_segment == 0xAF8u && report.callee_pc == 0x308u && report.return_pc == 0x248u);
    s.cpu.ip = 0x30Fu;
    CHECK(unchanged(&s, PR_SYSTEM_GETMEM_BLOCKED_NORMALIZER) == 0);
    CHECK(pr_system_getmem_23f_step(&s, &report) == PR_SYSTEM_GETMEM_BLOCKED_NORMALIZER);
    CHECK(report.call_is_near && report.callee_pc == 0x491u && report.return_pc == 0x312u);
    s.cpu.ip = 0x242u; s.cpu.bp = 0xFFF9u;
    CHECK(unchanged(&s, PR_SYSTEM_GETMEM_BLOCKED_MEMORY) == 0);
    s.cpu.ip = 0x363u; s.cpu.sp = 0x100u; s.cpu.di = 0xFFFFu;
    CHECK(unchanged(&s, PR_SYSTEM_GETMEM_BLOCKED_MEMORY) == 0);
    s.cpu.sp = 0xFFFFu; s.cpu.di = 0x100u;
    CHECK(unchanged(&s, PR_SYSTEM_GETMEM_BLOCKED_MEMORY) == 0);
    s.cpu.sp = 0x100u; s.cpu.di = 0x200u; s.ram_size = 0x201u;
    CHECK(unchanged(&s, PR_SYSTEM_GETMEM_BLOCKED_MEMORY) == 0);
    s.ram_size = sizeof ram; s.cpu.ip = 0x36Eu; s.cpu.di = 0xFFF9u;
    CHECK(unchanged(&s, PR_SYSTEM_GETMEM_BLOCKED_MEMORY) == 0);
    s.cpu.ip = 0x37Bu; s.cpu.di = 0xFFFDu;
    CHECK(unchanged(&s, PR_SYSTEM_GETMEM_BLOCKED_MEMORY) == 0);
    s.cpu.ip = 0x30Cu; s.cpu.ax = 0xCAFEu;
    CHECK(pr_system_getmem_23f_step(&s, &report) == PR_SYSTEM_GETMEM_STEPPED);
    CHECK(ram[0x8754u] == 0xFEu && ram[0x8755u] == 0xCAu);
    s.cpu.ip = 0x30Cu; s.ram_size = 0x8755u;
    CHECK(unchanged(&s, PR_SYSTEM_GETMEM_BLOCKED_MEMORY) == 0);
    s.ram_size = sizeof ram; s.cpu.ip = 0x382u; s.cpu.sp = 0xFFFFu;
    CHECK(unchanged(&s, PR_SYSTEM_GETMEM_BLOCKED_MEMORY) == 0);
    s.cpu.ip = 0x24Bu; s.cpu.sp = 0xFFFDu;
    CHECK(unchanged(&s, PR_SYSTEM_GETMEM_BLOCKED_MEMORY) == 0);
    s.cpu.ip = 0xE2u;
    CHECK(unchanged(&s, PR_SYSTEM_GETMEM_BLOCKED_RUNTIME_ERROR) == 0);
    s.cpu.ip = 0x3A6u;
    CHECK(unchanged(&s, PR_SYSTEM_GETMEM_BLOCKED_PC) == 0);
    s.cpu.cs = 0x2A8Eu; s.cpu.ip = 0x19BAu;
    CHECK(unchanged(&s, PR_SYSTEM_GETMEM_BLOCKED_CALLER) == 0);
    s.ram_size = 0x100001u;
    CHECK(unchanged(&s, PR_SYSTEM_GETMEM_BLOCKED_MEMORY) == 0);
    s.ram_size = sizeof ram; s.ram = NULL;
    CHECK(unchanged(&s, PR_SYSTEM_GETMEM_BLOCKED_MEMORY) == 0);
    memset(&report, 0xA5, sizeof report); old_report = report;
    CHECK(pr_system_getmem_23f_step(NULL, &report) == PR_SYSTEM_GETMEM_ARGUMENT);
    CHECK(memcmp(&report, &old_report, sizeof report) == 0);
    CHECK(pr_system_getmem_23f_step(&s, NULL) == PR_SYSTEM_GETMEM_ARGUMENT);
    puts("strict GetMem ABI frames, near gates, atomic split stores and bounds passed");
    return 0;
}
