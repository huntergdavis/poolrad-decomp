#include "poolrad/replay/slot_a_loader_caf_gate.h"
static uint16_t logic8(uint16_t old,unsigned value)
{
 unsigned parity=value;uint16_t f=(uint16_t)(old&(uint16_t)~0x8D5u);
 parity^=parity>>4;parity^=parity>>2;parity^=parity>>1;
 if(!(parity&1u))f|=4u;
 if(!value)f|=0x40u;
 if(value&0x80u)f|=0x80u;
 return f;
}
pr_loader_caf_status pr_slot_a_loader_caf_step(
 pr_slot_a_directory_state *s,pr_slot_a_string_0356_report *r)
{
 pr_slot_a_directory_cpu *c;uint16_t sp;size_t a;
 if(!s || !r)return PR_LOADER_CAF_ARGUMENT;
 *r=(pr_slot_a_string_0356_report){0};
 if(!s->ram || s->ram_size>0x100000u)return PR_LOADER_CAF_BLOCKED_MEMORY;
 c=&s->cpu;
 if(c->cs!=0x2A8Eu)return PR_LOADER_CAF_BLOCKED_PC;
 switch(c->ip){
 case 0xCAFu:c->flags=logic8(c->flags,c->ax&255u);c->ip=0xCB1u;break;
 case 0xCB1u:c->ip=c->flags&0x40u?0xCB3u:0xCB6u;break;
 case 0xCB3u:c->ip=0xD8Fu;break;
 case 0xCB6u:c->ax=0x3Fu;c->ip=0xCB9u;break;
 case 0xCB9u:
  sp=(uint16_t)(c->sp-2u);a=((size_t)c->ss<<4)+sp;
  if(sp==0xFFFFu || a>s->ram_size || 2u>s->ram_size-a)
   return PR_LOADER_CAF_BLOCKED_MEMORY;
  s->ram[a]=(uint8_t)c->ax;s->ram[a+1u]=(uint8_t)(c->ax>>8);
  c->sp=sp;c->ip=0xCBAu;break;
 case 0xCBAu:
  *r=(pr_slot_a_string_0356_report){1u,0x6Fu,0xCBAu,0xAF8u,0x23Fu,0xCBFu,1u};
  return PR_LOADER_CAF_BLOCKED_GETMEM;
 case 0xD8Fu:return PR_LOADER_CAF_BLOCKED_NEXT_CONTEXT;
 default:return PR_LOADER_CAF_BLOCKED_PC;
 }
 return PR_LOADER_CAF_STEPPED;
}
