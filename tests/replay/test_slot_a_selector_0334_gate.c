#include "poolrad/replay/slot_a_selector_0334_gate.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);return 1;}}while(0)
static uint8_t ram[655360u],saved[655360u];
static void put(size_t a,uint16_t v){ram[a]=(uint8_t)v;ram[a+1u]=(uint8_t)(v>>8);}
static int blocked(pr_slot_a_directory_state *s,pr_selector_0334_status status)
{
 pr_slot_a_directory_cpu before=s->cpu;pr_selector_0334_report r;
 memcpy(saved,ram,sizeof ram);memset(&r,0xA5,sizeof r);
 CHECK(pr_slot_a_selector_0334_step(s,&r)==status);
 CHECK(!memcmp(&before,&s->cpu,sizeof before) && !memcmp(saved,ram,sizeof ram));
 if(status!=PR_SELECTOR_0334_BLOCKED_LOADER)CHECK(!r.valid);
 return 0;
}
int main(void)
{
 pr_slot_a_directory_state s={0};pr_selector_0334_report r,old;unsigned i;
 s.ram=ram;s.ram_size=sizeof ram;s.cpu.cs=0x1026u;s.cpu.ip=0x334u;s.cpu.sp=0x80u;s.cpu.bp=0x100u;s.cpu.flags=0x7246u;
 ram[0xD6u]=23u;put(0x100u,0x200u);put(0x102u,0xCAFu);put(0x104u,0x2A8Eu);
 for(i=0;i<11u;i++)CHECK(pr_slot_a_selector_0334_step(&s,&r)==PR_SELECTOR_0334_STEPPED && !r.valid);
 CHECK(s.cpu.cs==0x2A8Eu && s.cpu.ip==0xCAFu && s.cpu.sp==0x10Au && s.cpu.bp==0x200u);
 CHECK(s.cpu.ax==1u && s.cpu.flags==0x7206u && ram[0xFFu]==1u && ram[0x49CEu]==0u);
 CHECK(blocked(&s,PR_SELECTOR_0334_BLOCKED_LOADER)==0);
 CHECK(pr_slot_a_selector_0334_step(&s,&r)==PR_SELECTOR_0334_BLOCKED_LOADER && r.valid && r.return_segment==0x2A8Eu && r.return_pc==0xCAFu);
 s.cpu.cs=0x1026u;s.cpu.ip=0x334u;s.cpu.bp=0x100u;s.cpu.sp=0x80u;s.cpu.ax=0x100u;s.cpu.flags=0x7AD7u;
 for(i=0;i<10u;i++)CHECK(pr_slot_a_selector_0334_step(&s,&r)==PR_SELECTOR_0334_STEPPED);
 CHECK(s.cpu.ax==0x100u && ram[0x49CEu]==0u && ram[0x49CFu]==1u && ram[0xFFu]==0u);
 s.cpu.cs=0x1026u;s.cpu.ip=0x33Eu;s.cpu.bp=0x100u;s.cpu.flags=0x7202u;ram[0xD6u]=0x80u;
 CHECK(pr_slot_a_selector_0334_step(&s,&r)==PR_SELECTOR_0334_STEPPED && s.cpu.flags==0x7282u);
 s.cpu.ip=0x34Du;s.cpu.ax=0xAB00u;ram[0xFFu]=0xCDu;
 CHECK(pr_slot_a_selector_0334_step(&s,&r)==PR_SELECTOR_0334_STEPPED && s.cpu.ax==0xABCDu);
 s.cpu.ip=0x353u;s.cpu.sp=0xFFFCu;put(0xFFFCu,0xCAFu);put(0xFFFEu,0x2A8Eu);
 CHECK(pr_slot_a_selector_0334_step(&s,&r)==PR_SELECTOR_0334_STEPPED && s.cpu.sp==4u);
 s.cpu.cs=0x1026u;s.cpu.ip=0x334u;s.ram_size=0x49CFu;CHECK(blocked(&s,PR_SELECTOR_0334_BLOCKED_MEMORY)==0);
 s.ram_size=sizeof ram;s.cpu.ip=0x337u;s.cpu.ds=0xFFFFu;CHECK(blocked(&s,PR_SELECTOR_0334_BLOCKED_MEMORY)==0);
 s.cpu.ip=0x352u;s.cpu.sp=0xFFFFu;CHECK(blocked(&s,PR_SELECTOR_0334_BLOCKED_MEMORY)==0);
 s.cpu.ip=0x353u;s.cpu.sp=0xFFFEu;CHECK(blocked(&s,PR_SELECTOR_0334_BLOCKED_MEMORY)==0);
 s.cpu.ip=0x332u;CHECK(blocked(&s,PR_SELECTOR_0334_BLOCKED_PC)==0);
 s.cpu.cs=0x2A8Eu;s.cpu.ip=0xDAFu;CHECK(blocked(&s,PR_SELECTOR_0334_BLOCKED_PC)==0);
 s.ram_size=0x100001u;CHECK(blocked(&s,PR_SELECTOR_0334_BLOCKED_MEMORY)==0);
 s.ram_size=sizeof ram;s.ram=NULL;CHECK(blocked(&s,PR_SELECTOR_0334_BLOCKED_MEMORY)==0);
 memset(&r,0xA5,sizeof r);old=r;
 CHECK(pr_slot_a_selector_0334_step(NULL,&r)==PR_SELECTOR_0334_ARGUMENT && !memcmp(&r,&old,sizeof r));
 CHECK(pr_slot_a_selector_0334_step(&s,NULL)==PR_SELECTOR_0334_ARGUMENT);
 puts("Selector0334 strict full-word store, byte flags, AH retention, real RETF4 and atomic stops passed");return 0;
}
