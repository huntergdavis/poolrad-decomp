#include "poolrad/replay/slot_a_string_0356_abi.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #x); return 1; \
} } while (0)

static uint8_t ram[655360u], before_ram[655360u];

static int unchanged(pr_slot_a_directory_state *s,
                     pr_slot_a_string_0356_status status)
{
    pr_slot_a_directory_cpu before = s->cpu;
    pr_slot_a_string_0356_report report;
    memcpy(before_ram, ram, sizeof ram);
    CHECK((status == PR_SLOT_A_STRING_0356_BLOCKED_DELETE
        ? pr_slot_a_string_0356_prefix_step(s, &report)
        : pr_slot_a_string_0356_step(s, &report)) == status);
    CHECK(memcmp(&s->cpu, &before, sizeof before) == 0);
    CHECK(memcmp(ram, before_ram, sizeof ram) == 0);
    return 0;
}

int main(void)
{
    pr_slot_a_directory_state s = {0};
    pr_slot_a_string_0356_report report;
    s.ram = ram; s.ram_size = sizeof ram;
    s.cpu.cs = 0x2A8Eu; s.cpu.ip = 0x196Fu;
    CHECK(unchanged(&s, PR_SLOT_A_STRING_0356_BLOCKED_0425_ABI) == 0);
    CHECK(pr_slot_a_string_0356_step(&s, &report) == PR_SLOT_A_STRING_0356_BLOCKED_0425_ABI);
    CHECK(report.dependency_valid && report.caller_is_overlay);
    CHECK(report.caller == 0x6Fu && report.call_pc == 0x196Fu);
    CHECK(report.callee_segment == 0x0802u && report.callee_pc == 0x0425u);
    CHECK(report.return_pc == 0x1974u);
    s.cpu.cs = 0x1026u; s.cpu.ip = 0x03FCu;
    CHECK(unchanged(&s, PR_SLOT_A_STRING_0356_BLOCKED_DELETE) == 0);
    CHECK(pr_slot_a_string_0356_prefix_step(&s, &report) == PR_SLOT_A_STRING_0356_BLOCKED_DELETE);
    CHECK(report.dependency_valid && !report.caller_is_overlay);
    CHECK(report.caller == 0x0802u && report.call_pc == 0x03FCu);
    CHECK(report.callee_segment == 0x0AF8u && report.callee_pc == 0x0CBCu);
    CHECK(report.return_pc == 0x0401u);
    s.cpu.ip = 0x03FDu;
    CHECK(unchanged(&s, PR_SLOT_A_STRING_0356_BLOCKED_PC) == 0);
    CHECK(pr_slot_a_string_0356_step(&s, &report) == PR_SLOT_A_STRING_0356_BLOCKED_PC);
    CHECK(report.dependency_valid == 0u);

    s.cpu.cs = 0x2A8Eu; s.cpu.ip = 0x196Au; s.cpu.sp = 2u;
    CHECK(unchanged(&s, PR_SLOT_A_STRING_0356_BLOCKED_MEMORY) == 0);
    s.cpu.cs = 0x1026u; s.cpu.ip = 0x035Du; s.cpu.bp = 0x10u;
    s.cpu.ss = 0u; s.ram_size = 0x1Cu; /* only first half of LES operand fits */
    CHECK(unchanged(&s, PR_SLOT_A_STRING_0356_BLOCKED_MEMORY) == 0);

    s.cpu.cs = 0x131Cu; s.cpu.ip = 0x0B42u;
    s.cpu.ds = s.cpu.es = 0u; s.cpu.si = 0x100u; s.cpu.di = 0x104u;
    s.cpu.cx = 2u; s.ram_size = 0x105u;
    CHECK(unchanged(&s, PR_SLOT_A_STRING_0356_BLOCKED_MEMORY) == 0);
    s.ram_size = sizeof ram; s.cpu.si = 0xFFFFu;
    CHECK(unchanged(&s, PR_SLOT_A_STRING_0356_BLOCKED_MEMORY) == 0);
    s.cpu.si = 0x100u; s.cpu.di = 0x101u; s.cpu.cx = 3u;
    ram[0x100u] = 'A'; ram[0x101u] = 'B'; ram[0x102u] = 'C';
    CHECK(pr_slot_a_string_0356_step(&s, &report) == PR_SLOT_A_STRING_0356_STEPPED);
    CHECK(ram[0x101u] == 'A' && ram[0x102u] == 'A' && ram[0x103u] == 'A');
    CHECK(s.cpu.si == 0x103u && s.cpu.di == 0x104u && s.cpu.cx == 0u);
    s.cpu.ip = 0x0B42u; s.cpu.cx = 0u;
    s.cpu.ds = s.cpu.es = 0xFFFFu; s.cpu.si = s.cpu.di = 0xFFFFu;
    CHECK(pr_slot_a_string_0356_step(&s, &report) == PR_SLOT_A_STRING_0356_STEPPED);
    CHECK(s.cpu.si == 0xFFFFu && s.cpu.di == 0xFFFFu);
    s.cpu.ip = 0x0B42u; s.cpu.flags |= 0x0400u;
    CHECK(unchanged(&s, PR_SLOT_A_STRING_0356_BLOCKED_PC) == 0);
    s.ram = NULL;
    CHECK(unchanged(&s, PR_SLOT_A_STRING_0356_BLOCKED_MEMORY) == 0);
    CHECK(pr_slot_a_string_0356_step(NULL, &report) == PR_SLOT_A_STRING_0356_ARGUMENT);
    CHECK(pr_slot_a_string_0356_step(&s, NULL) == PR_SLOT_A_STRING_0356_ARGUMENT);
    puts("strict resident +0356 ABI blockers, bounds and ordered-copy checks passed");
    return 0;
}
