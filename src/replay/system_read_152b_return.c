#include "poolrad/replay/system_read_152b_return.h"
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
static unsigned even(unsigned v)
{v^=v>>8;v^=v>>4;v^=v>>2;v^=v>>1;return (v&1u)^1u;}
static uint16_t logic_flags(uint16_t old,uint16_t v)
{
 uint16_t f=(uint16_t)(old&(uint16_t)~0x8D5u);
 if(even(v&255u))f|=4u;
 if(!v)f|=0x40u;
 if(v&0x8000u)f|=0x80u;
 return f;
}
static uint16_t cmp_flags(uint16_t old,unsigned a,unsigned b)
{
 unsigned v=(a-b)&65535u;uint16_t f=logic_flags(old,(uint16_t)v);
 if(a<b)f|=1u;
 if((a^b^v)&0x10u)f|=0x10u;
 if((a^b)&(a^v)&0x8000u)f|=0x800u;
 return f;
}
pr_system_read_return_status pr_system_read_152b_step(
 pr_slot_a_directory_state *s,pr_system_read_return_report *r)
{
 pr_slot_a_directory_cpu *c;uint8_t *p;uint16_t v,ip,cs,next;
 uint32_t num,quo;unsigned divisor,rem;
 if(!s || !r)return PR_SYSTEM_READ_RETURN_ARGUMENT;
 *r=(pr_system_read_return_report){0};
 if(!s->ram || s->ram_size>0x100000u)return PR_SYSTEM_READ_RETURN_BLOCKED_MEMORY;
 c=&s->cpu;
 if((c->cs==0x2A8Eu && c->ip==0xC49u) || (c->cs==0x1026u && c->ip==0x334u)){
  *r=(pr_system_read_return_report){1u,c->cs,c->ip};
  return c->cs==0x2A8Eu?PR_SYSTEM_READ_RETURN_BLOCKED_LOADER:PR_SYSTEM_READ_RETURN_BLOCKED_IO_CALLER;
 }
 if(c->cs!=0x131Cu)return PR_SYSTEM_READ_RETURN_BLOCKED_PC;
 switch(c->ip){
 case 0x152Bu:case 0x152Cu:case 0x155Fu:
  p=span(s,c->ss,c->sp,2u);if(!p)return PR_SYSTEM_READ_RETURN_BLOCKED_MEMORY;
  v=word(p);
  if(c->ip==0x152Bu)c->cx=v;else if(c->ip==0x152Cu)c->ds=v;else c->bp=v;
  c->sp=(uint16_t)(c->sp+2u);c->ip=(uint16_t)(c->ip+1u);break;
 case 0x152Du:c->ip=c->flags&1u?0x154Eu:0x152Fu;break;
 case 0x152Fu:c->dx=0u;c->flags=logic_flags(c->flags,0u);c->ip=0x1531u;break;
 case 0x1531u:
  p=span(s,c->es,(uint16_t)(c->di+4u),2u);if(!p)return PR_SYSTEM_READ_RETURN_BLOCKED_MEMORY;
  divisor=word(p);if(!divisor)return PR_SYSTEM_READ_RETURN_BLOCKED_DIVIDE;
  num=((uint32_t)c->dx<<16)|c->ax;quo=num/divisor;
  if(quo>65535u)return PR_SYSTEM_READ_RETURN_BLOCKED_DIVIDE;
  rem=(unsigned)(num%divisor);c->ax=(uint16_t)quo;c->dx=(uint16_t)rem;
  c->flags=(uint16_t)(c->flags&(uint16_t)~0x8D5u);
  if(!rem && (quo&1u))c->flags|=0x40u;
  if((rem&3u)==1u || (rem&3u)==2u)c->flags|=1u;
  if(even((unsigned)quo^rem))c->flags|=4u;
  c->ip=0x1535u;break;
 case 0x1535u:case 0x1551u:
  p=span(s,c->ss,(uint16_t)(c->bp+6u),4u);if(!p)return PR_SYSTEM_READ_RETURN_BLOCKED_MEMORY;
  c->di=word(p);c->es=word(p+2u);c->ip=(uint16_t)(c->ip+3u);break;
 case 0x1538u:case 0x1554u:c->dx=c->es;c->ip=(uint16_t)(c->ip+2u);break;
 case 0x153Au:case 0x1556u:
  c->dx=(uint16_t)(c->dx|c->di);c->flags=logic_flags(c->flags,c->dx);c->ip=(uint16_t)(c->ip+2u);break;
 case 0x153Cu:c->ip=c->flags&0x40u?0x1543u:0x153Eu;break;
 case 0x1558u:c->ip=c->flags&0x40u?0x155Fu:0x155Au;break;
 case 0x153Eu:case 0x155Cu:
  p=span(s,c->es,c->di,2u);if(!p)return PR_SYSTEM_READ_RETURN_BLOCKED_MEMORY;
  put(p,c->ax);c->ip=(uint16_t)(c->ip+3u);break;
 case 0x1541u:case 0x154Cu:c->ip=0x155Fu;break;
 case 0x1543u:
  p=span(s,c->ss,(uint16_t)(c->bp+0xAu),2u);if(!p)return PR_SYSTEM_READ_RETURN_BLOCKED_MEMORY;
  c->flags=cmp_flags(c->flags,c->ax,word(p));c->ip=0x1546u;break;
 case 0x1546u:c->ip=c->flags&0x40u?0x155Fu:0x1548u;break;
 case 0x1548u:v=c->cx;next=0x154Cu;goto error_store;
 case 0x154Eu:v=c->ax;next=0x1551u;goto error_store;
 case 0x155Au:case 0x4A2u:
  c->ax=0u;c->flags=logic_flags(c->flags,0u);c->ip=(uint16_t)(c->ip+2u);break;
 case 0x4A4u:
  p=span(s,c->ds,0x4426u,2u);if(!p)return PR_SYSTEM_READ_RETURN_BLOCKED_MEMORY;
  v=word(p);put(p,c->ax);c->ax=v;c->ip=0x4A8u;break;
 case 0x1560u:case 0x4A8u:
  p=span(s,c->ss,c->sp,4u);if(!p)return PR_SYSTEM_READ_RETURN_BLOCKED_MEMORY;
  ip=word(p);cs=word(p+2u);c->sp=(uint16_t)(c->sp+(c->ip==0x1560u?18u:4u));c->ip=ip;c->cs=cs;break;
 default:return PR_SYSTEM_READ_RETURN_BLOCKED_PC;
 }
 return PR_SYSTEM_READ_RETURN_STEPPED;
error_store:
 p=span(s,c->ds,0x4426u,2u);if(!p)return PR_SYSTEM_READ_RETURN_BLOCKED_MEMORY;
 put(p,v);c->ip=next;return PR_SYSTEM_READ_RETURN_STEPPED;
}
