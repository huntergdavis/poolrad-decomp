#include "poolrad/replay/system_cbc_abi.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) {fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;} } while (0)
static uint8_t ram[655360u],before_ram[655360u];
static int unchanged(pr_slot_a_directory_state *s,pr_system_b98_status expected)
{
    pr_slot_a_directory_cpu before=s->cpu;memcpy(before_ram,ram,sizeof ram);
    CHECK(pr_system_cbc_step(s)==expected);
    CHECK(memcmp(&before,&s->cpu,sizeof before)==0 && memcmp(before_ram,ram,sizeof ram)==0);
    return 0;
}
int main(void)
{
    pr_slot_a_directory_state s={0};s.ram=ram;s.ram_size=sizeof ram;
    s.cpu.cs=0x1026u;s.cpu.ip=0x3FCu;s.cpu.sp=2u;
    CHECK(unchanged(&s,PR_SYSTEM_B98_BLOCKED_MEMORY)==0);
    s.cpu.cs=0x131Cu;s.cpu.ip=0xCBCu;s.cpu.sp=1u;
    CHECK(unchanged(&s,PR_SYSTEM_B98_BLOCKED_MEMORY)==0);
    s.cpu.ip=0xCC3u;s.cpu.bp=0xFFF9u;
    CHECK(unchanged(&s,PR_SYSTEM_B98_BLOCKED_MEMORY)==0);
    s.cpu.ip=0xCFFu;s.cpu.bp=0xFFF5u;
    CHECK(unchanged(&s,PR_SYSTEM_B98_BLOCKED_MEMORY)==0);
    s.cpu.ip=0xCEFu;s.cpu.bp=0xFFF9u;
    CHECK(unchanged(&s,PR_SYSTEM_B98_BLOCKED_MEMORY)==0);
    s.cpu.ip=0xCDEu;s.cpu.sp=1u;
    CHECK(unchanged(&s,PR_SYSTEM_B98_BLOCKED_MEMORY)==0);
    s.cpu.ip=0xD0Fu;s.cpu.sp=0xFFFFu;
    CHECK(unchanged(&s,PR_SYSTEM_B98_BLOCKED_MEMORY)==0);
    s.cpu.ip=0xB42u;s.cpu.ds=s.cpu.es=0u;s.cpu.si=0x100u;s.cpu.di=0x103u;s.cpu.cx=3u;s.ram_size=0x105u;
    CHECK(unchanged(&s,PR_SYSTEM_B98_BLOCKED_MEMORY)==0);
    s.cpu.ip=0xBBDu;
    CHECK(unchanged(&s,PR_SYSTEM_B98_BLOCKED_MEMORY)==0);
    s.ram_size=sizeof ram;s.cpu.ip=0xCBFu+1u;
    CHECK(unchanged(&s,PR_SYSTEM_B98_BLOCKED_PC)==0);
    s.ram_size=0x100001u;
    CHECK(unchanged(&s,PR_SYSTEM_B98_BLOCKED_MEMORY)==0);
    s.ram=NULL;
    CHECK(unchanged(&s,PR_SYSTEM_B98_BLOCKED_MEMORY)==0);
    CHECK(pr_system_cbc_step(NULL)==PR_SYSTEM_B98_ARGUMENT);
    puts("strict CBC complete-body host bounds passed");return 0;
}
