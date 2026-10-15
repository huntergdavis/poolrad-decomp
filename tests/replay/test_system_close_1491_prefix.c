#include "poolrad/replay/system_close_1491_prefix.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);return 1;}}while(0)
static uint8_t ram[655360u],saved[655360u];
static int blocked(pr_slot_a_directory_state *s,pr_system_close_status status)
{
 pr_slot_a_directory_cpu before=s->cpu;pr_system_close_report r;
 memcpy(saved,ram,sizeof ram);memset(&r,0xA5,sizeof r);
 CHECK(pr_system_close_1491_step(s,&r)==status);
 CHECK(!memcmp(&before,&s->cpu,sizeof before) && !memcmp(saved,ram,sizeof ram));
 if(status!=PR_SYSTEM_CLOSE_BLOCKED_DOS)CHECK(!r.valid);
 return 0;
}
int main(void)
{
 pr_slot_a_directory_state s={0};pr_system_close_report r,old;unsigned i;
 s.ram=ram;s.ram_size=sizeof ram;s.cpu.cs=0x131Cu;s.cpu.ip=0x1491u;s.cpu.sp=0x100u;
 s.cpu.ax=0xAB1Du;s.cpu.flags=0x7206u;
 ram[0x104u]=0x20u;ram[0x106u]=0u;ram[0x107u]=0x10u;
 ram[0x10020u]=6u;ram[0x10022u]=0xB3u;ram[0x10023u]=0xD7u;
 for(i=0;i<11u;i++)CHECK(pr_system_close_1491_step(&s,&r)==PR_SYSTEM_CLOSE_STEPPED && !r.valid);
 CHECK(s.cpu.ip==0x14A6u && s.cpu.ax==0x3E1Du && s.cpu.bx==6u && s.cpu.flags==0x7202u);
 CHECK(s.cpu.sp==0x100u && ram[0xFEu]==0x9Au && ram[0xFFu]==0x14u);
 CHECK(blocked(&s,PR_SYSTEM_CLOSE_BLOCKED_DOS)==0);
 CHECK(pr_system_close_1491_step(&s,&r)==PR_SYSTEM_CLOSE_BLOCKED_DOS);
 CHECK(r.valid && r.original_segment==0xAF8u && r.call_pc==0x14A6u && r.return_pc==0x14A8u && r.ax==0x3E1Du && r.bx==6u);
 s.cpu.ip=0x1497u;s.cpu.sp=0u;
 CHECK(pr_system_close_1491_step(&s,&r)==PR_SYSTEM_CLOSE_STEPPED);
 CHECK(s.cpu.sp==0xFFFEu && ram[0xFFFEu]==0x9Au && ram[0xFFFFu]==0x14u);
 s.cpu.ip=0x14C4u;CHECK(pr_system_close_1491_step(&s,&r)==PR_SYSTEM_CLOSE_STEPPED);
 CHECK(s.cpu.sp==0u && s.cpu.ip==0x149Au);
 for(i=0;i<=4u;i++){
  s.cpu.ip=0x149Fu;s.cpu.bx=(uint16_t)i;s.cpu.flags=0x7AD7u;
  CHECK(pr_system_close_1491_step(&s,&r)==PR_SYSTEM_CLOSE_STEPPED);
  CHECK(pr_system_close_1491_step(&s,&r)==PR_SYSTEM_CLOSE_STEPPED && s.cpu.ip==0x14ADu);
  CHECK(blocked(&s,PR_SYSTEM_CLOSE_BLOCKED_RESERVED_TAIL)==0);
 }
 s.cpu.ip=0x14BEu;CHECK(blocked(&s,PR_SYSTEM_CLOSE_BLOCKED_SIGNATURE_ERROR)==0);
 s.cpu.ip=0x14B3u;CHECK(blocked(&s,PR_SYSTEM_CLOSE_BLOCKED_RETURN_TAIL)==0);
 s.cpu.ip=0x14A8u;CHECK(blocked(&s,PR_SYSTEM_CLOSE_BLOCKED_PC)==0);
 s.cpu.cs=0xAF8u;s.cpu.ip=0x1491u;CHECK(blocked(&s,PR_SYSTEM_CLOSE_BLOCKED_PC)==0);
 s.cpu.cs=0x131Cu;s.cpu.ip=0x1493u;s.cpu.bx=0xFFFAu;
 CHECK(blocked(&s,PR_SYSTEM_CLOSE_BLOCKED_MEMORY)==0);
 s.cpu.ip=0x1497u;s.cpu.sp=1u;CHECK(blocked(&s,PR_SYSTEM_CLOSE_BLOCKED_MEMORY)==0);
 s.cpu.ip=0x14C4u;s.cpu.sp=0xFFFFu;CHECK(blocked(&s,PR_SYSTEM_CLOSE_BLOCKED_MEMORY)==0);
 s.cpu.ip=0x14B6u;s.cpu.es=0u;s.cpu.di=0xFFFDu;CHECK(blocked(&s,PR_SYSTEM_CLOSE_BLOCKED_MEMORY)==0);
 s.cpu.ip=0x149Cu;s.cpu.di=0xFFFFu;CHECK(blocked(&s,PR_SYSTEM_CLOSE_BLOCKED_MEMORY)==0);
 s.cpu.es=0xFFFFu;CHECK(blocked(&s,PR_SYSTEM_CLOSE_BLOCKED_MEMORY)==0);
 s.ram_size=0x100001u;CHECK(blocked(&s,PR_SYSTEM_CLOSE_BLOCKED_MEMORY)==0);
 s.ram_size=sizeof ram;s.ram=NULL;CHECK(blocked(&s,PR_SYSTEM_CLOSE_BLOCKED_MEMORY)==0);
 memset(&r,0xA5,sizeof r);old=r;
 CHECK(pr_system_close_1491_step(NULL,&r)==PR_SYSTEM_CLOSE_ARGUMENT && !memcmp(&r,&old,sizeof r));
 CHECK(pr_system_close_1491_step(&s,NULL)==PR_SYSTEM_CLOSE_ARGUMENT);
 puts("Close1491 strict real frames, unsigned handles, AL preservation, typed stops and atomic bounds passed");return 0;
}
