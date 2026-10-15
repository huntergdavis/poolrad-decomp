#include "poolrad/replay/slot_a_file_probe_02eb_abi.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) {fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;} } while (0)
static uint8_t ram[655360u], saved_ram[655360u];
static int unchanged(pr_slot_a_directory_state *s, pr_slot_a_file_probe_status expected)
{
    pr_slot_a_directory_cpu before = s->cpu;
    pr_slot_a_string_0356_report report;
    memset(&report, 0xA5, sizeof report); memcpy(saved_ram, ram, sizeof ram);
    CHECK(pr_slot_a_file_probe_02eb_step(s, &report) == expected);
    CHECK(memcmp(&before, &s->cpu, sizeof before) == 0);
    CHECK(memcmp(saved_ram, ram, sizeof ram) == 0);
    if (expected != PR_SLOT_A_FILE_PROBE_BLOCKED_RUNTIME) CHECK(!report.dependency_valid);
    return 0;
}
int main(void)
{
    pr_slot_a_directory_state s = {0};
    pr_slot_a_string_0356_report report, saved_report;
    const uint16_t gates[] = {0x310u, 0x31Fu, 0x32Au, 0x32Fu};
    const uint16_t targets[] = {0x13E2u, 0x1410u, 0x1491u, 0x4A2u};
    size_t i;
    s.ram = ram; s.ram_size = sizeof ram;
    s.cpu.cs = 0x2A8Eu; s.cpu.ip = 0x19A8u; s.cpu.sp = 2u;
    CHECK(unchanged(&s, PR_SLOT_A_FILE_PROBE_BLOCKED_MEMORY) == 0);
    s.cpu.sp = 0x100u;
    CHECK(pr_slot_a_file_probe_02eb_step(&s, &report) == PR_SLOT_A_FILE_PROBE_STEPPED);
    CHECK(s.cpu.cs == 0x1026u && s.cpu.ip == 0x2EBu && s.cpu.sp == 0xFCu);
    CHECK(ram[0xFCu] == 0xADu && ram[0xFDu] == 0x19u && ram[0xFEu] == 0x8Eu && ram[0xFFu] == 0x2Au);
    for (i = 0; i < sizeof gates / sizeof gates[0]; ++i) {
        s.cpu.ip = gates[i]; s.cpu.sp = 1u;
        CHECK(unchanged(&s, PR_SLOT_A_FILE_PROBE_BLOCKED_RUNTIME) == 0);
        CHECK(pr_slot_a_file_probe_02eb_step(&s, &report) == PR_SLOT_A_FILE_PROBE_BLOCKED_RUNTIME);
        CHECK(report.dependency_valid && report.caller == 0x802u && !report.caller_is_overlay);
        CHECK(report.call_pc == gates[i] && report.callee_segment == 0xAF8u);
        CHECK(report.callee_pc == targets[i] && report.return_pc == gates[i] + 5u);
    }
    s.cpu.ip = 0x2F2u; s.cpu.bp = 0xFFF7u;
    CHECK(unchanged(&s, PR_SLOT_A_FILE_PROBE_BLOCKED_MEMORY) == 0);
    s.cpu.ip = 0x300u; s.cpu.sp = 2u;
    CHECK(unchanged(&s, PR_SLOT_A_FILE_PROBE_BLOCKED_MEMORY) == 0);
    s.cpu.ip = 0x334u; s.cpu.ds = 0u; s.cpu.ax = 0xABCDu;
    CHECK(pr_slot_a_file_probe_02eb_step(&s, &report) == PR_SLOT_A_FILE_PROBE_STEPPED);
    CHECK(ram[0x49CEu] == 0xCDu && ram[0x49CFu] == 0xABu);
    s.cpu.ip = 0x334u; s.ram_size = 0x49CFu;
    CHECK(unchanged(&s, PR_SLOT_A_FILE_PROBE_BLOCKED_MEMORY) == 0);
    s.ram_size = sizeof ram; s.cpu.ip = 0x34Au; s.cpu.ss = 0xFFFFu; s.cpu.bp = 0u;
    CHECK(unchanged(&s, PR_SLOT_A_FILE_PROBE_BLOCKED_MEMORY) == 0);
    s.cpu.ss = 0u; s.cpu.ip = 0x353u; s.cpu.sp = 0xFFFFu;
    CHECK(unchanged(&s, PR_SLOT_A_FILE_PROBE_BLOCKED_MEMORY) == 0);
    s.cpu.cs = 0x131Cu; s.cpu.ip = 0xB42u; s.cpu.si = 0x100u; s.cpu.di = 0x103u; s.cpu.cx = 3u;
    s.ram_size = 0x105u;
    CHECK(unchanged(&s, PR_SLOT_A_FILE_PROBE_BLOCKED_MEMORY) == 0);
    s.ram_size = sizeof ram; s.cpu.cs = 0x2A8Eu; s.cpu.ip = 0x19ADu;
    CHECK(unchanged(&s, PR_SLOT_A_FILE_PROBE_BLOCKED_CALLER) == 0);
    s.cpu.ip = 0x19B1u;
    CHECK(unchanged(&s, PR_SLOT_A_FILE_PROBE_BLOCKED_PC) == 0);
    s.ram_size = 0x100001u;
    CHECK(unchanged(&s, PR_SLOT_A_FILE_PROBE_BLOCKED_MEMORY) == 0);
    s.ram_size = sizeof ram; s.ram = NULL;
    CHECK(unchanged(&s, PR_SLOT_A_FILE_PROBE_BLOCKED_MEMORY) == 0);
    memset(&report, 0xA5, sizeof report); saved_report = report;
    CHECK(pr_slot_a_file_probe_02eb_step(NULL, &report) == PR_SLOT_A_FILE_PROBE_ARGUMENT);
    CHECK(memcmp(&report, &saved_report, sizeof report) == 0);
    CHECK(pr_slot_a_file_probe_02eb_step(&s, NULL) == PR_SLOT_A_FILE_PROBE_ARGUMENT);
    puts("strict file-probe frames, writer, unknown-effect gates and bounds passed");
    return 0;
}
