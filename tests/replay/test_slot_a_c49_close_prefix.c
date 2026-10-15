#include "poolrad/replay/slot_a_c49_close_prefix.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);return 1;}}while(0)
static uint8_t ram[655360u],saved[655360u];
static int blocked(pr_slot_a_directory_state *s,pr_c49_close_status status)
{
 pr_slot_a_directory_cpu before=s->cpu;pr_c49_close_report r;
 memcpy(saved,ram,sizeof ram);memset(&r,0xA5,sizeof r);
 CHECK(pr_slot_a_c49_close_step(s,&r)==status);
 CHECK(!memcmp(&before,&s->cpu,sizeof before) && !memcmp(saved,ram,sizeof ram));
 if(status!=PR_C49_CLOSE_BLOCKED_DOS)CHECK(!r.valid);
 return 0;
}
int main(void)
{
 pr_slot_a_directory_state s={0};pr_c49_close_report r,old;unsigned i;
 s.ram=ram;s.ram_size=sizeof ram;s.cpu.cs=0x2A8Eu;s.cpu.ip=0xC49u;s.cpu.sp=0x100u;s.cpu.bp=0x200u;
 s.cpu.ax=0xAB1Du;s.cpu.flags=0x7206u;ram[0x80u]=6u;ram[0x82u]=0xB3u;ram[0x83u]=0xD7u;
 for(i=0;i<15u;i++)CHECK(pr_slot_a_c49_close_step(&s,&r)==PR_C49_CLOSE_STEPPED && !r.valid);
 CHECK(s.cpu.cs==0x131Cu && s.cpu.ip==0x14A6u && s.cpu.sp==0xF8u && s.cpu.bp==0x200u);
 CHECK(s.cpu.ax==0x3E1Du && s.cpu.bx==6u && s.cpu.di==0x80u && s.cpu.flags==0x7202u);
 CHECK(ram[0xF8u]==0x54u && ram[0xF9u]==0xCu && ram[0xFAu]==0x8Eu && ram[0xFBu]==0x2Au);
 CHECK(ram[0xFCu]==0x80u && ram[0xFDu]==0u && ram[0xFEu]==0u && ram[0xFFu]==0u);
 CHECK(ram[0xF6u]==0x9Au && ram[0xF7u]==0x14u && blocked(&s,PR_C49_CLOSE_BLOCKED_DOS)==0);
 CHECK(pr_slot_a_c49_close_step(&s,&r)==PR_C49_CLOSE_BLOCKED_DOS);
 CHECK(r.valid && r.original_segment==0xAF8u && r.call_pc==0x14A6u && r.return_pc==0x14A8u && r.ax==0x3E1Du && r.bx==6u);
 s.cpu.cs=0x2A8Eu;s.cpu.ip=0xC4Fu;s.cpu.sp=2u;CHECK(blocked(&s,PR_C49_CLOSE_BLOCKED_MEMORY)==0);
 s.cpu.sp=0u;CHECK(pr_slot_a_c49_close_step(&s,&r)==PR_C49_CLOSE_STEPPED && s.cpu.sp==0xFFFCu);
 CHECK(s.cpu.cs==0x131Cu && s.cpu.ip==0x1491u && ram[0xFFFCu]==0x54u && ram[0xFFFFu]==0x2Au);
 s.cpu.cs=0x2A8Eu;s.cpu.ip=0xC4Du;s.cpu.sp=1u;CHECK(blocked(&s,PR_C49_CLOSE_BLOCKED_MEMORY)==0);
 s.cpu.cs=0x131Cu;s.cpu.ip=0x1493u;s.cpu.bx=0xFFFAu;CHECK(blocked(&s,PR_C49_CLOSE_BLOCKED_MEMORY)==0);
 s.cpu.ip=0x1497u;s.cpu.sp=1u;CHECK(blocked(&s,PR_C49_CLOSE_BLOCKED_MEMORY)==0);
 s.cpu.ip=0x14C4u;s.cpu.sp=0xFFFFu;CHECK(blocked(&s,PR_C49_CLOSE_BLOCKED_MEMORY)==0);
 s.cpu.ip=0x14B6u;s.cpu.es=0u;s.cpu.di=0xFFFDu;CHECK(blocked(&s,PR_C49_CLOSE_BLOCKED_MEMORY)==0);
 s.cpu.ip=0x149Cu;s.cpu.di=0xFFFFu;CHECK(blocked(&s,PR_C49_CLOSE_BLOCKED_MEMORY)==0);
 s.cpu.ip=0x14ADu;CHECK(blocked(&s,PR_C49_CLOSE_BLOCKED_RESERVED_TAIL)==0);
 s.cpu.ip=0x14BEu;CHECK(blocked(&s,PR_C49_CLOSE_BLOCKED_SIGNATURE_ERROR)==0);
 s.cpu.ip=0x14B3u;CHECK(blocked(&s,PR_C49_CLOSE_BLOCKED_RETURN_TAIL)==0);
 s.cpu.ip=0x14A8u;CHECK(blocked(&s,PR_C49_CLOSE_BLOCKED_PC)==0);
 s.cpu.cs=0x2A8Eu;s.cpu.ip=0xC54u;CHECK(blocked(&s,PR_C49_CLOSE_BLOCKED_PC)==0);
 s.ram_size=0x100001u;CHECK(blocked(&s,PR_C49_CLOSE_BLOCKED_MEMORY)==0);
 s.ram_size=sizeof ram;s.ram=NULL;CHECK(blocked(&s,PR_C49_CLOSE_BLOCKED_MEMORY)==0);
 memset(&r,0xA5,sizeof r);old=r;
 CHECK(pr_slot_a_c49_close_step(NULL,&r)==PR_C49_CLOSE_ARGUMENT && !memcmp(&r,&old,sizeof r));
 CHECK(pr_slot_a_c49_close_step(&s,NULL)==PR_C49_CLOSE_ARGUMENT);
 puts("C49 contiguous real far/near frames, AL retention, typed DOS stops and atomic spans passed");return 0;
}
