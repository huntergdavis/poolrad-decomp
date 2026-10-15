#include "poolrad/replay/slot_a_allocation_19b1_abi.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) {fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;} } while (0)
static uint8_t ram[655360u], copy[655360u];
static int unchanged(pr_slot_a_directory_state *s, pr_slot_a_allocation_status expected)
{
    pr_slot_a_directory_cpu before = s->cpu;
    pr_slot_a_string_0356_report report;
    memset(&report, 0xA5, sizeof report); memcpy(copy, ram, sizeof ram);
    CHECK(pr_slot_a_allocation_19b1_step(s, &report) == expected);
    CHECK(memcmp(&before, &s->cpu, sizeof before) == 0 && memcmp(copy, ram, sizeof ram) == 0);
    if (expected != PR_SLOT_A_ALLOCATION_BLOCKED_GETMEM) CHECK(!report.dependency_valid);
    return 0;
}
int main(void)
{
    pr_slot_a_directory_state s = {0};
    pr_slot_a_string_0356_report report, saved_report;
    s.ram = ram; s.ram_size = sizeof ram; s.cpu.cs = 0x2A8Eu;
    s.cpu.ip = 0x19B1u; s.cpu.ax = 0xFFFFu; s.cpu.flags = 0x7ED7u; s.cpu.sp = 0x100u;
    CHECK(pr_slot_a_allocation_19b1_step(&s, &report) == PR_SLOT_A_ALLOCATION_STEPPED);
    CHECK(s.cpu.ax == 0x11Du && s.cpu.flags == 0x7ED7u && s.cpu.ip == 0x19B4u && !report.dependency_valid);
    CHECK(pr_slot_a_allocation_19b1_step(&s, &report) == PR_SLOT_A_ALLOCATION_STEPPED);
    CHECK(s.cpu.sp == 0xFEu && ram[0xFEu] == 0x1Du && ram[0xFFu] == 1u);
    CHECK(s.cpu.flags == 0x7ED7u);
    CHECK(unchanged(&s, PR_SLOT_A_ALLOCATION_BLOCKED_GETMEM) == 0);
    CHECK(pr_slot_a_allocation_19b1_step(&s, &report) == PR_SLOT_A_ALLOCATION_BLOCKED_GETMEM);
    CHECK(report.dependency_valid && report.caller == 0x6Fu && report.caller_is_overlay);
    CHECK(report.call_pc == 0x19B5u && report.callee_segment == 0xAF8u && report.callee_pc == 0x23Fu && report.return_pc == 0x19BAu);
    s.cpu.ip = 0x19B4u; s.cpu.sp = 1u;
    CHECK(unchanged(&s, PR_SLOT_A_ALLOCATION_BLOCKED_MEMORY) == 0);
    s.cpu.sp = 0x100u; s.ram_size = 0xFFu;
    CHECK(unchanged(&s, PR_SLOT_A_ALLOCATION_BLOCKED_MEMORY) == 0);
    s.ram_size = sizeof ram; s.cpu.ss = 0xFFFFu; s.cpu.sp = 0x20u;
    CHECK(unchanged(&s, PR_SLOT_A_ALLOCATION_BLOCKED_MEMORY) == 0);
    s.cpu.ss = 0u; s.cpu.ip = 0x19ADu;
    CHECK(unchanged(&s, PR_SLOT_A_ALLOCATION_BLOCKED_PC) == 0);
    s.cpu.ip = 0x19BAu;
    CHECK(unchanged(&s, PR_SLOT_A_ALLOCATION_BLOCKED_PC) == 0);
    s.cpu.cs = 0x1026u; s.cpu.ip = 0x19B1u;
    CHECK(unchanged(&s, PR_SLOT_A_ALLOCATION_BLOCKED_PC) == 0);
    s.ram_size = 0x100001u;
    CHECK(unchanged(&s, PR_SLOT_A_ALLOCATION_BLOCKED_MEMORY) == 0);
    s.ram_size = sizeof ram; s.ram = NULL;
    CHECK(unchanged(&s, PR_SLOT_A_ALLOCATION_BLOCKED_MEMORY) == 0);
    memset(&report, 0xA5, sizeof report); saved_report = report;
    CHECK(pr_slot_a_allocation_19b1_step(NULL, &report) == PR_SLOT_A_ALLOCATION_ARGUMENT);
    CHECK(memcmp(&report, &saved_report, sizeof report) == 0);
    CHECK(pr_slot_a_allocation_19b1_step(&s, NULL) == PR_SLOT_A_ALLOCATION_ARGUMENT);
    puts("strict allocation setup flags, stack writer, immutable GetMem gate and bounds passed");
    return 0;
}
