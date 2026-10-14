#include "poolrad/replay/system_b98_abi.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); return 1; } } while (0)
static uint8_t ram[655360u],before_ram[655360u];
static int unchanged(pr_slot_a_directory_state *s,pr_system_b98_status expected,int route)
{
    pr_slot_a_directory_cpu before=s->cpu;
    pr_slot_a_string_0356_report report;
    memcpy(before_ram,ram,sizeof ram);
    CHECK((route?pr_slot_a_concat_1999_step(s,&report):pr_system_b98_step(s))==expected);
    CHECK(memcmp(&before,&s->cpu,sizeof before)==0);
    CHECK(memcmp(before_ram,ram,sizeof ram)==0);
    return 0;
}
int main(void)
{
    pr_slot_a_directory_state s={0};pr_slot_a_string_0356_report report;
    s.ram=ram;s.ram_size=sizeof ram;s.cpu.cs=0x2A8Eu;s.cpu.ip=0x19A8u;
    CHECK(unchanged(&s,PR_SYSTEM_B98_BLOCKED_FILE_PROBE,1)==0);
    CHECK(pr_slot_a_concat_1999_step(&s,&report)==PR_SYSTEM_B98_BLOCKED_FILE_PROBE);
    CHECK(report.dependency_valid && report.caller_is_overlay && report.caller==0x6Fu);
    CHECK(report.call_pc==0x19A8u && report.callee_segment==0x802u && report.callee_pc==0x2EBu && report.return_pc==0x19ADu);
    s.cpu.ip=0x199Au;CHECK(unchanged(&s,PR_SYSTEM_B98_BLOCKED_PC,1)==0);
    CHECK(pr_slot_a_concat_1999_step(&s,&report)==PR_SYSTEM_B98_BLOCKED_PC && !report.dependency_valid);
    s.cpu.ip=0x1999u;s.cpu.sp=2u;CHECK(unchanged(&s,PR_SYSTEM_B98_BLOCKED_MEMORY,1)==0);
    s.cpu.ip=0x19A1u;s.cpu.sp=1u;CHECK(unchanged(&s,PR_SYSTEM_B98_BLOCKED_MEMORY,1)==0);
    s.cpu.cs=0x131Cu;s.cpu.ip=0xB9Du;s.cpu.bx=0xFFF7u;
    CHECK(unchanged(&s,PR_SYSTEM_B98_BLOCKED_MEMORY,0)==0);
    s.cpu.ip=0xBC1u;s.cpu.sp=0xFFFFu;CHECK(unchanged(&s,PR_SYSTEM_B98_BLOCKED_MEMORY,0)==0);
    s.cpu.ip=0xBABu;s.cpu.es=0xFFFFu;s.cpu.di=0xFFFFu;
    CHECK(unchanged(&s,PR_SYSTEM_B98_BLOCKED_MEMORY,0)==0);
    s.cpu.ip=0xBBDu;s.cpu.ds=s.cpu.es=0u;s.cpu.si=0x100u;s.cpu.di=0x103u;s.cpu.cx=3u;s.ram_size=0x105u;
    CHECK(unchanged(&s,PR_SYSTEM_B98_BLOCKED_MEMORY,0)==0);
    s.ram_size=sizeof ram;s.cpu.si=0xFFFFu;s.cpu.cx=2u;
    CHECK(unchanged(&s,PR_SYSTEM_B98_BLOCKED_MEMORY,0)==0);
    s.cpu.cx=0u;s.cpu.ds=s.cpu.es=0xFFFFu;s.cpu.si=s.cpu.di=0xFFFFu;
    CHECK(pr_system_b98_step(&s)==PR_SYSTEM_B98_STEPPED && s.cpu.si==0xFFFFu && s.cpu.di==0xFFFFu);
    s.cpu.ip=0xBBDu;s.cpu.flags|=0x400u;CHECK(unchanged(&s,PR_SYSTEM_B98_BLOCKED_PC,0)==0);
    s.cpu.ip=0xBAAu;CHECK(unchanged(&s,PR_SYSTEM_B98_BLOCKED_PC,0)==0);
    s.ram_size=0x100001u;CHECK(unchanged(&s,PR_SYSTEM_B98_BLOCKED_MEMORY,0)==0);
    s.ram=NULL;CHECK(unchanged(&s,PR_SYSTEM_B98_BLOCKED_MEMORY,0)==0);
    CHECK(pr_system_b98_step(NULL)==PR_SYSTEM_B98_ARGUMENT);
    memset(&report,0xA5,sizeof report);
    {pr_slot_a_string_0356_report before=report;
     CHECK(pr_slot_a_concat_1999_step(NULL,&report)==PR_SYSTEM_B98_ARGUMENT);
     CHECK(memcmp(&before,&report,sizeof before)==0);}
    CHECK(pr_slot_a_concat_1999_step(&s,NULL)==PR_SYSTEM_B98_ARGUMENT);
    puts("strict B98 helper bounds and immutable file-probe gate passed");return 0;
}
