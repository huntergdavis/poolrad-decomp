#include "poolrad/replay/slot_a_string_0425_abi.h"
#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #x); return 1; \
} } while (0)
static uint8_t ram[655360u], before_ram[655360u];

static int unchanged(pr_slot_a_directory_state *s,
                     pr_slot_a_string_0425_status status)
{
    pr_slot_a_directory_cpu before = s->cpu;
    pr_slot_a_string_0425_report report;
    memcpy(before_ram, ram, sizeof ram);
    CHECK(pr_slot_a_string_0425_step(s, &report) == status);
    CHECK(memcmp(&s->cpu, &before, sizeof before) == 0);
    CHECK(memcmp(ram, before_ram, sizeof ram) == 0);
    return 0;
}

int main(void)
{
    pr_slot_a_directory_state s = {0};
    pr_slot_a_string_0425_report report;
    s.ram = ram; s.ram_size = sizeof ram;
    s.cpu.cs = 0x2A8Eu; s.cpu.ip = 0x1974u;
    CHECK(unchanged(&s, PR_SLOT_A_STRING_0425_BLOCKED_CALLER_CONTINUATION) == 0);
    CHECK(pr_slot_a_string_0425_step(&s, &report) == PR_SLOT_A_STRING_0425_BLOCKED_CALLER_CONTINUATION);
    CHECK(report.dependency_valid && report.caller_is_overlay);
    CHECK(report.caller == 0x6Fu && report.call_pc == 0x197Eu);
    CHECK(report.callee_segment == 0x0AF8u && report.callee_pc == 0x0B25u);
    CHECK(report.return_pc == 0x1983u);
    s.cpu.cs = 0x1026u; s.cpu.ip = 0x03FCu;
    CHECK(unchanged(&s, PR_SLOT_A_STRING_0425_BLOCKED_DELETE) == 0);
    CHECK(pr_slot_a_string_0425_step(&s, &report) == PR_SLOT_A_STRING_0425_BLOCKED_DELETE);
    CHECK(report.dependency_valid && !report.caller_is_overlay);
    CHECK(report.call_pc == 0x03FCu && report.callee_pc == 0x0CBCu);
    s.cpu.ip = 0x0427u;
    CHECK(unchanged(&s, PR_SLOT_A_STRING_0425_BLOCKED_PC) == 0);
    CHECK(pr_slot_a_string_0425_step(&s, &report) == PR_SLOT_A_STRING_0425_BLOCKED_PC);
    CHECK(!report.dependency_valid);
    s.cpu.cs = 0x2A8Eu; s.cpu.ip = 0x196Fu; s.cpu.sp = 2u;
    CHECK(unchanged(&s, PR_SLOT_A_STRING_0425_BLOCKED_MEMORY) == 0);
    s.cpu.cs = 0x1026u; s.cpu.ip = 0x0470u; s.ram_size = 1u;
    CHECK(unchanged(&s, PR_SLOT_A_STRING_0425_BLOCKED_MEMORY) == 0);
    s.cpu.ip = 0x042Cu; s.cpu.bp = 0x10u; s.ram_size = 0x18u;
    CHECK(unchanged(&s, PR_SLOT_A_STRING_0425_BLOCKED_MEMORY) == 0);
    s.cpu.ip = 0x0465u; s.cpu.ds = 0u; s.cpu.di = 1u; s.ram_size = 0xD1Au;
    CHECK(unchanged(&s, PR_SLOT_A_STRING_0425_BLOCKED_MEMORY) == 0);
    s.cpu.ip = 0x04E7u; s.cpu.bp = 0x100u; s.cpu.di = 1u; s.ram_size = 1u;
    CHECK(unchanged(&s, PR_SLOT_A_STRING_0425_BLOCKED_MEMORY) == 0);
    s.cpu.cs = 0x131Cu; s.cpu.ip = 0x0C44u; s.cpu.es = 0u; s.cpu.di = 1u;
    CHECK(unchanged(&s, PR_SLOT_A_STRING_0425_BLOCKED_MEMORY) == 0);
    s.ram_size = sizeof ram; s.cpu.flags = 0x402u;
    CHECK(unchanged(&s, PR_SLOT_A_STRING_0425_BLOCKED_PC) == 0);
    s.cpu.ip = 0x0C3Eu; s.cpu.bx = 0xFFF9u;
    CHECK(unchanged(&s, PR_SLOT_A_STRING_0425_BLOCKED_MEMORY) == 0);
    s.ram_size = 0x100001u;
    CHECK(unchanged(&s, PR_SLOT_A_STRING_0425_BLOCKED_MEMORY) == 0);
    s.ram = NULL;
    CHECK(unchanged(&s, PR_SLOT_A_STRING_0425_BLOCKED_MEMORY) == 0);
    memset(&report, 0xA5, sizeof report);
    {
        pr_slot_a_string_0425_report before = report;
        CHECK(pr_slot_a_string_0425_step(NULL, &report) == PR_SLOT_A_STRING_0425_ARGUMENT);
        CHECK(memcmp(&report, &before, sizeof report) == 0);
    }
    CHECK(pr_slot_a_string_0425_step(&s, NULL) == PR_SLOT_A_STRING_0425_ARGUMENT);
    puts("strict +0425 ABI blockers and atomic host bounds passed");
    return 0;
}
