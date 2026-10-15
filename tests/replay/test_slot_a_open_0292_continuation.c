#include "poolrad/replay/slot_a_open_0292_continuation.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);return 1;}}while(0)
static uint8_t ram[655360u],saved_ram[655360u];
static int blocked(pr_slot_a_directory_state *s,pr_slot_a_open_0292_status expected)
{
 pr_slot_a_directory_cpu before=s->cpu;pr_slot_a_open_0292_report r;
 memcpy(saved_ram,ram,sizeof ram);memset(&r,0xA5,sizeof r);
 CHECK(pr_slot_a_open_0292_step(s,&r)==expected);
 CHECK(!memcmp(&before,&s->cpu,sizeof before) && !memcmp(ram,saved_ram,sizeof ram));
 if(expected!=PR_SLOT_A_OPEN_0292_BLOCKED_ASSIGN && expected!=PR_SLOT_A_OPEN_0292_BLOCKED_RESET && expected!=PR_SLOT_A_OPEN_0292_BLOCKED_MESSAGE)CHECK(!r.dependency_valid);
 return 0;
}
int main(void)
{
 pr_slot_a_directory_state s={0};pr_slot_a_open_0292_report r,save;
 s.ram=ram;s.ram_size=sizeof ram;s.cpu.cs=0x1026u;s.cpu.ip=0x2C9u;s.cpu.sp=0x100u;
 CHECK(blocked(&s,PR_SLOT_A_OPEN_0292_BLOCKED_ASSIGN)==0);
 CHECK(pr_slot_a_open_0292_step(&s,&r)==PR_SLOT_A_OPEN_0292_BLOCKED_ASSIGN);
 CHECK(r.dependency_valid && r.caller==0x802u && r.call_pc==0x2C9u && r.callee_segment==0xAF8u && r.callee_pc==0x13E2u && r.return_pc==0x2CEu);
 s.cpu.ip=0x2D7u;CHECK(blocked(&s,PR_SLOT_A_OPEN_0292_BLOCKED_RESET)==0);
 CHECK(pr_slot_a_open_0292_step(&s,&r)==PR_SLOT_A_OPEN_0292_BLOCKED_RESET && r.callee_pc==0x1410u && r.return_pc==0x2DCu);
 s.cpu.ip=0x2AAu;CHECK(blocked(&s,PR_SLOT_A_OPEN_0292_BLOCKED_MESSAGE)==0);
 CHECK(pr_slot_a_open_0292_step(&s,&r)==PR_SLOT_A_OPEN_0292_BLOCKED_MESSAGE && r.callee_segment==0x802u && r.callee_pc==0x193u && r.return_pc==0x2ADu);
 s.cpu.ip=0x289u;CHECK(blocked(&s,PR_SLOT_A_OPEN_0292_BLOCKED_RETRY)==0);
 s.cpu.ip=0x2BFu;s.cpu.bp=0xFFF8u;CHECK(blocked(&s,PR_SLOT_A_OPEN_0292_BLOCKED_MEMORY)==0);
 s.cpu.ip=0x2C2u;s.cpu.sp=1u;CHECK(blocked(&s,PR_SLOT_A_OPEN_0292_BLOCKED_MEMORY)==0);
 s.cpu.ip=0x2E8u;s.cpu.sp=0xFFFEu;CHECK(blocked(&s,PR_SLOT_A_OPEN_0292_BLOCKED_MEMORY)==0);
 s.cpu.sp=0x100u;ram[0x100u]=0x2Bu;ram[0x101u]=0xCu;ram[0x102u]=0x8Eu;ram[0x103u]=0x2Au;
 CHECK(pr_slot_a_open_0292_step(&s,&r)==PR_SLOT_A_OPEN_0292_STEPPED);
 CHECK(s.cpu.cs==0x2A8Eu && s.cpu.ip==0xC2Bu && s.cpu.sp==0x112u);
 CHECK(blocked(&s,PR_SLOT_A_OPEN_0292_BLOCKED_CALLER)==0);
 s.cpu.cs=0x1026u;s.cpu.ip=0x2DCu;s.cpu.bp=0x200u;s.cpu.ax=0xAB00u;ram[0x1ACu]=0x80u;
 CHECK(pr_slot_a_open_0292_step(&s,&r)==PR_SLOT_A_OPEN_0292_STEPPED && s.cpu.ax==0xAB80u);
 s.cpu.ip=0x295u;s.cpu.flags=0xFFFFu;
 CHECK(pr_slot_a_open_0292_step(&s,&r)==PR_SLOT_A_OPEN_0292_STEPPED && (s.cpu.flags&0x8D5u)==0x80u);
 s.cpu.ip=0x2EBu;CHECK(blocked(&s,PR_SLOT_A_OPEN_0292_BLOCKED_PC)==0);
 s.ram_size=0x100001u;CHECK(blocked(&s,PR_SLOT_A_OPEN_0292_BLOCKED_MEMORY)==0);
 s.ram_size=sizeof ram;s.ram=NULL;CHECK(blocked(&s,PR_SLOT_A_OPEN_0292_BLOCKED_MEMORY)==0);
 memset(&r,0xA5,sizeof r);save=r;
 CHECK(pr_slot_a_open_0292_step(NULL,&r)==PR_SLOT_A_OPEN_0292_ARGUMENT && !memcmp(&r,&save,sizeof r));
 CHECK(pr_slot_a_open_0292_step(&s,NULL)==PR_SLOT_A_OPEN_0292_ARGUMENT);
 puts("Caller0292 strict dependencies, full flags/AH, bounds and actual RETF0E passed");return 0;
}
