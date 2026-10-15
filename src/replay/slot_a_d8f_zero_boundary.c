#include "poolrad/replay/slot_a_d8f_zero_boundary.h"
pr_d8f_zero_status pr_slot_a_d8f_zero_step(
 pr_slot_a_directory_state *s,pr_slot_a_string_0356_report *r)
{
 pr_slot_a_directory_cpu *c;uint16_t sp,value,next;size_t a;
 if(!s || !r)return PR_D8F_ZERO_ARGUMENT;
 *r=(pr_slot_a_string_0356_report){0};
 if(!s->ram || s->ram_size>0x100000u)return PR_D8F_ZERO_BLOCKED_MEMORY;
 c=&s->cpu;if(c->cs!=0x2A8Eu)return PR_D8F_ZERO_BLOCKED_PC;
 switch(c->ip){
 case 0xD8Fu:c->di=(uint16_t)(c->bp-0x2B9u);c->ip=0xD93u;return PR_D8F_ZERO_STEPPED;
 case 0xD95u:c->di=(uint16_t)(c->bp-0x100u);c->ip=0xD99u;return PR_D8F_ZERO_STEPPED;
 case 0xD93u:value=c->ss;next=0xD94u;break;
 case 0xD94u:value=c->di;next=0xD95u;break;
 case 0xD99u:value=c->ss;next=0xD9Au;break;
 case 0xD9Au:value=c->di;next=0xD9Bu;break;
 case 0xD9Bu:
  *r=(pr_slot_a_string_0356_report){1u,0x6Fu,0xD9Bu,0xAF8u,0xB0Bu,0xDA0u,1u};
  return PR_D8F_ZERO_BLOCKED_COPY;
 default:return PR_D8F_ZERO_BLOCKED_PC;
 }
 sp=(uint16_t)(c->sp-2u);a=((size_t)c->ss<<4)+sp;
 if(sp==0xFFFFu || a>s->ram_size || 2u>s->ram_size-a)return PR_D8F_ZERO_BLOCKED_MEMORY;
 s->ram[a]=(uint8_t)value;s->ram[a+1u]=(uint8_t)(value>>8);
 c->sp=sp;c->ip=next;return PR_D8F_ZERO_STEPPED;
}
