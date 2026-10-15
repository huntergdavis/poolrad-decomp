#include "poolrad/replay/slot_a_probe_return_19ad_abi.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) {fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;} } while (0)
static uint8_t ram[64];
static int unchanged(pr_slot_a_directory_state *s, pr_slot_a_probe_return_status expected)
{
    pr_slot_a_directory_cpu before = s->cpu;
    uint8_t copy[sizeof ram]; memcpy(copy, ram, sizeof ram);
    CHECK(pr_slot_a_probe_return_19ad_step(s) == expected);
    CHECK(memcmp(&before, &s->cpu, sizeof before) == 0 && memcmp(copy, ram, sizeof ram) == 0);
    return 0;
}
int main(void)
{
    pr_slot_a_directory_state s = {0}; s.ram = ram; s.ram_size = sizeof ram;
    s.cpu.cs = 0x2A8Eu; s.cpu.ip = 0x19ADu; s.cpu.ax = 0xA500u; s.cpu.flags = 0x7AD7u;
    CHECK(pr_slot_a_probe_return_19ad_step(&s) == PR_SLOT_A_PROBE_RETURN_STEPPED);
    CHECK(s.cpu.ax == 0xA500u && s.cpu.ip == 0x19AFu && s.cpu.flags == 0x7246u);
    CHECK(pr_slot_a_probe_return_19ad_step(&s) == PR_SLOT_A_PROBE_RETURN_STEPPED);
    CHECK(s.cpu.ip == 0x19EAu);
    CHECK(unchanged(&s, PR_SLOT_A_PROBE_RETURN_BLOCKED_LOOP_GATE) == 0);
    s.cpu.ip = 0x19ADu; s.cpu.ax = 0xA580u; s.cpu.flags = 0x7AD7u;
    CHECK(pr_slot_a_probe_return_19ad_step(&s) == PR_SLOT_A_PROBE_RETURN_STEPPED);
    CHECK(s.cpu.ax == 0xA580u && s.cpu.flags == 0x7282u);
    CHECK(pr_slot_a_probe_return_19ad_step(&s) == PR_SLOT_A_PROBE_RETURN_STEPPED);
    CHECK(s.cpu.ip == 0x19B1u);
    CHECK(unchanged(&s, PR_SLOT_A_PROBE_RETURN_BLOCKED_PRESENT_CONTINUATION) == 0);
    s.cpu.ip = 0x19B2u;
    CHECK(unchanged(&s, PR_SLOT_A_PROBE_RETURN_BLOCKED_PC) == 0);
    s.cpu.cs = 0x1026u; s.cpu.ip = 0x19ADu;
    CHECK(unchanged(&s, PR_SLOT_A_PROBE_RETURN_BLOCKED_PC) == 0);
    s.ram_size = 0x100001u;
    CHECK(unchanged(&s, PR_SLOT_A_PROBE_RETURN_BLOCKED_MEMORY) == 0);
    s.ram_size = sizeof ram; s.ram = NULL;
    CHECK(unchanged(&s, PR_SLOT_A_PROBE_RETURN_BLOCKED_MEMORY) == 0);
    CHECK(pr_slot_a_probe_return_19ad_step(NULL) == PR_SLOT_A_PROBE_RETURN_ARGUMENT);
    puts("strict caller return flags, branch boundaries and immutable blocks passed");
    return 0;
}
