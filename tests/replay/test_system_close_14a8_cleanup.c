#include "poolrad/replay/system_close_14a8_cleanup.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);return 1;}}while(0)
static uint8_t ram[655360u],saved[655360u];
static int blocked(pr_slot_a_directory_state *s,pr_system_close_cleanup_status status)
{
 pr_slot_a_directory_cpu before=s->cpu;pr_system_close_cleanup_report r;
 memcpy(saved,ram,sizeof ram);memset(&r,0xA5,sizeof r);
 CHECK(pr_system_close_14a8_step(s,&r)==status);
 CHECK(!memcmp(&before,&s->cpu,sizeof before) && !memcmp(saved,ram,sizeof ram));
 if(status!=PR_SYSTEM_CLOSE_CLEANUP_BLOCKED_LOADER)CHECK(!r.valid);
 return 0;
}
int main(void)
{
 pr_slot_a_directory_state s={0};pr_system_close_cleanup_report r,old;unsigned i;
 s.ram=ram;s.ram_size=sizeof ram;s.cpu.cs=0x131Cu;s.cpu.ip=0x14A8u;s.cpu.sp=0x100u;
 s.cpu.es=0x1000u;s.cpu.di=0x20u;s.cpu.ax=0xABCDu;s.cpu.flags=0x7AD7u;
 ram[0x100u]=0x54u;ram[0x101u]=0xCu;ram[0x102u]=0x8Eu;ram[0x103u]=0x2Au;
 ram[0x10022u]=0xB3u;ram[0x10023u]=0xD7u;
 for(i=0;i<4u;i++)CHECK(pr_system_close_14a8_step(&s,&r)==PR_SYSTEM_CLOSE_CLEANUP_STEPPED && !r.valid);
 CHECK(s.cpu.cs==0x2A8Eu && s.cpu.ip==0xC54u && s.cpu.sp==0x108u);
 CHECK(s.cpu.ax==0xABCDu && s.cpu.flags==0x7AD7u);
 CHECK(ram[0x4426u]==0xCDu && ram[0x4427u]==0xABu && ram[0x10022u]==0xB0u && ram[0x10023u]==0xD7u);
 CHECK(blocked(&s,PR_SYSTEM_CLOSE_CLEANUP_BLOCKED_LOADER)==0);
 CHECK(pr_system_close_14a8_step(&s,&r)==PR_SYSTEM_CLOSE_CLEANUP_BLOCKED_LOADER && r.valid && r.return_segment==0x2A8Eu && r.return_pc==0xC54u);
 s.cpu.cs=0x131Cu;s.cpu.ip=0x14B3u;s.cpu.sp=0xFFFCu;
 ram[0xFFFCu]=0x54u;ram[0xFFFDu]=0xCu;ram[0xFFFEu]=0x8Eu;ram[0xFFFFu]=0x2Au;
 CHECK(pr_system_close_14a8_step(&s,&r)==PR_SYSTEM_CLOSE_CLEANUP_STEPPED && s.cpu.sp==4u);
 s.cpu.cs=0x131Cu;s.cpu.ip=0x14A8u;s.cpu.flags=0x7202u;
 CHECK(pr_system_close_14a8_step(&s,&r)==PR_SYSTEM_CLOSE_CLEANUP_STEPPED && s.cpu.ip==0x14ADu);
 s.cpu.ip=0x14A6u;CHECK(blocked(&s,PR_SYSTEM_CLOSE_CLEANUP_BLOCKED_PC)==0);
 s.cpu.ip=0x14BEu;CHECK(blocked(&s,PR_SYSTEM_CLOSE_CLEANUP_BLOCKED_PC)==0);
 s.cpu.cs=0u;s.cpu.ip=0xC54u;CHECK(blocked(&s,PR_SYSTEM_CLOSE_CLEANUP_BLOCKED_PC)==0);
 s.cpu.cs=0x131Cu;s.cpu.ip=0x14AAu;s.ram_size=0x4427u;
 CHECK(blocked(&s,PR_SYSTEM_CLOSE_CLEANUP_BLOCKED_MEMORY)==0);
 s.ram_size=sizeof ram;s.cpu.ip=0x14ADu;s.cpu.es=0u;s.cpu.di=0xFFFDu;
 CHECK(blocked(&s,PR_SYSTEM_CLOSE_CLEANUP_BLOCKED_MEMORY)==0);
 s.cpu.ip=0x14B3u;s.cpu.sp=0xFFFEu;CHECK(blocked(&s,PR_SYSTEM_CLOSE_CLEANUP_BLOCKED_MEMORY)==0);
 s.cpu.ip=0x14AAu;s.cpu.ds=0xFFFFu;CHECK(blocked(&s,PR_SYSTEM_CLOSE_CLEANUP_BLOCKED_MEMORY)==0);
 s.ram_size=0x100001u;CHECK(blocked(&s,PR_SYSTEM_CLOSE_CLEANUP_BLOCKED_MEMORY)==0);
 s.ram_size=sizeof ram;s.ram=NULL;CHECK(blocked(&s,PR_SYSTEM_CLOSE_CLEANUP_BLOCKED_MEMORY)==0);
 memset(&r,0xA5,sizeof r);old=r;
 CHECK(pr_system_close_14a8_step(NULL,&r)==PR_SYSTEM_CLOSE_CLEANUP_ARGUMENT && !memcmp(&r,&old,sizeof r));
 CHECK(pr_system_close_14a8_step(&s,NULL)==PR_SYSTEM_CLOSE_CLEANUP_ARGUMENT);
 puts("Close14A8 strict whole-word error/cleanup, real RETF4, immutable stops and atomic bounds passed");return 0;
}
