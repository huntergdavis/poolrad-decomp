#include "poolrad/replay/slot_a_selector_0334_gate.h"
static uint8_t *span(pr_slot_a_directory_state *s,uint16_t seg,uint16_t off,size_t n)
{
 size_t a=((size_t)seg<<4)+off;
 if((size_t)off+n>0x10000u || a>s->ram_size || n>s->ram_size-a)return NULL;
 return s->ram+a;
}
static uint16_t word(const uint8_t *p)
{return (uint16_t)((unsigned)p[0]|((unsigned)p[1]<<8));}
static void put(uint8_t *p,uint16_t v)
{p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static uint16_t cmp_zero(uint16_t old,unsigned value,unsigned sign)
{
 unsigned parity=value&255u;uint16_t f=(uint16_t)(old&(uint16_t)~0x8D5u);
 parity^=parity>>4;parity^=parity>>2;parity^=parity>>1;
 if(!(parity&1u))f|=4u;
 if(!value)f|=0x40u;
 if(value&sign)f|=0x80u;
 return f;
}
pr_selector_0334_status pr_slot_a_selector_0334_step(
 pr_slot_a_directory_state *s,pr_selector_0334_report *r)
{
 pr_slot_a_directory_cpu *c;uint8_t *p;uint16_t ip,cs,value;
 if(!s || !r)return PR_SELECTOR_0334_ARGUMENT;
 *r=(pr_selector_0334_report){0};
 if(!s->ram || s->ram_size>0x100000u)return PR_SELECTOR_0334_BLOCKED_MEMORY;
 c=&s->cpu;
 if(c->cs==0x2A8Eu && c->ip==0xCAFu){
  *r=(pr_selector_0334_report){1u,c->cs,c->ip};return PR_SELECTOR_0334_BLOCKED_LOADER;
 }
 if(c->cs!=0x1026u)return PR_SELECTOR_0334_BLOCKED_PC;
 switch(c->ip){
 case 0x334u:
  p=span(s,c->ds,0x49CEu,2u);if(!p)return PR_SELECTOR_0334_BLOCKED_MEMORY;
  put(p,c->ax);c->ip=0x337u;break;
 case 0x337u:
  p=span(s,c->ds,0x49CEu,2u);if(!p)return PR_SELECTOR_0334_BLOCKED_MEMORY;
  c->flags=cmp_zero(c->flags,word(p),0x8000u);c->ip=0x33Cu;break;
 case 0x33Cu:c->ip=c->flags&0x40u?0x33Eu:0x344u;break;
 case 0x33Eu:
  p=span(s,c->ss,(uint16_t)(c->bp-0x2Au),1u);if(!p)return PR_SELECTOR_0334_BLOCKED_MEMORY;
  c->flags=cmp_zero(c->flags,*p,0x80u);c->ip=0x342u;break;
 case 0x342u:c->ip=c->flags&0x40u?0x344u:0x348u;break;
 case 0x344u:c->ax&=0xFF00u;c->ip=0x346u;break;
 case 0x346u:c->ip=0x34Au;break;
 case 0x348u:c->ax=(uint16_t)((c->ax&0xFF00u)|1u);c->ip=0x34Au;break;
 case 0x34Au:
  p=span(s,c->ss,(uint16_t)(c->bp-1u),1u);if(!p)return PR_SELECTOR_0334_BLOCKED_MEMORY;
  *p=(uint8_t)c->ax;c->ip=0x34Du;break;
 case 0x34Du:
  p=span(s,c->ss,(uint16_t)(c->bp-1u),1u);if(!p)return PR_SELECTOR_0334_BLOCKED_MEMORY;
  c->ax=(uint16_t)((c->ax&0xFF00u)|*p);c->ip=0x350u;break;
 case 0x350u:c->sp=c->bp;c->ip=0x352u;break;
 case 0x352u:
  p=span(s,c->ss,c->sp,2u);if(!p)return PR_SELECTOR_0334_BLOCKED_MEMORY;
  value=word(p);c->sp=(uint16_t)(c->sp+2u);c->bp=value;c->ip=0x353u;break;
 case 0x353u:
  p=span(s,c->ss,c->sp,4u);if(!p)return PR_SELECTOR_0334_BLOCKED_MEMORY;
  ip=word(p);cs=word(p+2u);c->sp=(uint16_t)(c->sp+8u);c->ip=ip;c->cs=cs;break;
 default:return PR_SELECTOR_0334_BLOCKED_PC;
 }
 return PR_SELECTOR_0334_STEPPED;
}
