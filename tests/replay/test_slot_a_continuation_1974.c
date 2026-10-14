#include "poolrad/replay/slot_a_continuation_1974.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr,"check failed at line %d: %s\n",__LINE__,#x); return 1; \
} } while (0)
static uint8_t ram[655360u], before_ram[655360u];
static int unchanged(pr_slot_a_directory_state *s, pr_slot_a_continuation_1974_status status)
{
    pr_slot_a_directory_cpu before = s->cpu;
    pr_slot_a_continuation_1974_report report;
    memcpy(before_ram,ram,sizeof ram);
    CHECK(pr_slot_a_continuation_1974_step(s,&report) == status);
    CHECK(memcmp(&s->cpu,&before,sizeof before) == 0);
    CHECK(memcmp(ram,before_ram,sizeof ram) == 0);
    return 0;
}
int main(void)
{
    pr_slot_a_directory_state s = {0};
    pr_slot_a_continuation_1974_report report;
    s.ram=ram; s.ram_size=sizeof ram;
    s.cpu.cs=0x2A8Eu; s.cpu.ip=0x1999u;
    CHECK(unchanged(&s,PR_SLOT_A_CONTINUATION_1974_BLOCKED_B98_ABI) == 0);
    CHECK(pr_slot_a_continuation_1974_step(&s,&report) == PR_SLOT_A_CONTINUATION_1974_BLOCKED_B98_ABI);
    CHECK(report.dependency_valid && report.caller_is_overlay);
    CHECK(report.caller==0x6Fu && report.call_pc==0x1999u);
    CHECK(report.callee_segment==0x0AF8u && report.callee_pc==0x0B98u && report.return_pc==0x199Eu);
    s.cpu.ip=0x1975u;
    CHECK(unchanged(&s,PR_SLOT_A_CONTINUATION_1974_BLOCKED_PC) == 0);
    CHECK(pr_slot_a_continuation_1974_step(&s,&report) == PR_SLOT_A_CONTINUATION_1974_BLOCKED_PC);
    CHECK(!report.dependency_valid);
    s.cpu.ip=0x197Eu; s.cpu.sp=2u;
    CHECK(unchanged(&s,PR_SLOT_A_CONTINUATION_1974_BLOCKED_MEMORY) == 0);
    s.cpu.ip=0x1978u; s.cpu.sp=1u;
    CHECK(unchanged(&s,PR_SLOT_A_CONTINUATION_1974_BLOCKED_MEMORY) == 0);
    s.cpu.cs=0x131Cu; s.cpu.ip=0x0B10u; s.cpu.bx=0xFFF7u;
    CHECK(unchanged(&s,PR_SLOT_A_CONTINUATION_1974_BLOCKED_MEMORY) == 0);
    s.cpu.ip=0x0B42u; s.cpu.ds=s.cpu.es=0u; s.cpu.si=0x100u;
    s.cpu.di=0x103u; s.cpu.cx=3u; s.ram_size=0x105u;
    CHECK(unchanged(&s,PR_SLOT_A_CONTINUATION_1974_BLOCKED_MEMORY) == 0);
    s.cpu.ip=0x0B1Eu; s.ram_size=sizeof ram; s.cpu.si=0xFFFFu; s.cpu.cx=2u;
    CHECK(unchanged(&s,PR_SLOT_A_CONTINUATION_1974_BLOCKED_MEMORY) == 0);
    s.cpu.flags|=0x400u;
    CHECK(unchanged(&s,PR_SLOT_A_CONTINUATION_1974_BLOCKED_PC) == 0);
    s.cpu.ip=0x0B0Du;
    CHECK(unchanged(&s,PR_SLOT_A_CONTINUATION_1974_BLOCKED_PC) == 0);
    s.cpu.ip=0x0B57u;
    CHECK(unchanged(&s,PR_SLOT_A_CONTINUATION_1974_BLOCKED_PC) == 0);
    s.ram_size=0x100001u;
    CHECK(unchanged(&s,PR_SLOT_A_CONTINUATION_1974_BLOCKED_MEMORY) == 0);
    s.ram=NULL;
    CHECK(unchanged(&s,PR_SLOT_A_CONTINUATION_1974_BLOCKED_MEMORY) == 0);
    memset(&report,0xA5,sizeof report);
    {
        pr_slot_a_continuation_1974_report before=report;
        CHECK(pr_slot_a_continuation_1974_step(NULL,&report)==PR_SLOT_A_CONTINUATION_1974_ARGUMENT);
        CHECK(memcmp(&report,&before,sizeof report)==0);
    }
    CHECK(pr_slot_a_continuation_1974_step(&s,NULL)==PR_SLOT_A_CONTINUATION_1974_ARGUMENT);
    puts("strict +1974 continuation blockers and atomic bounds passed");
    return 0;
}
