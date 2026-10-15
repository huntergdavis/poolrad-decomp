#include "poolrad/replay/system_read_14fb_prefix.h"
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
static unsigned even(uint32_t v)
{v^=v>>16;v^=v>>8;v^=v>>4;v^=v>>2;v^=v>>1;return (unsigned)((v&1u)^1u);}
static uint16_t cmp_flags(uint16_t old,unsigned a,unsigned b)
{
 unsigned v=(a-b)&65535u;uint16_t f=(uint16_t)(old&(uint16_t)~0x8D5u);
 if(even(v&255u))f|=4u;
 if(!v)f|=0x40u;
 if(v&0x8000u)f|=0x80u;
 if(a<b)f|=1u;
 if((a^b^v)&0x10u)f|=0x10u;
 if((a^b)&(a^v)&0x8000u)f|=0x800u;
 return f;
}
pr_system_read_status pr_system_read_14fb_step(
 pr_slot_a_directory_state *s,pr_system_read_report *r)
{
 pr_slot_a_directory_cpu *c;uint8_t *p;uint16_t off,v,next;uint32_t product;
 if(!s || !r)return PR_SYSTEM_READ_ARGUMENT;
 *r=(pr_system_read_report){0};
 if(!s->ram || s->ram_size>0x100000u)return PR_SYSTEM_READ_BLOCKED_MEMORY;
 c=&s->cpu;if(c->cs!=0x131Cu)return PR_SYSTEM_READ_BLOCKED_PC;
 switch(c->ip){
 case 0x14FBu:c->bx=(uint16_t)((c->bx&0xFF00u)|0x3Fu);c->ip=0x14FDu;break;
 case 0x14FDu:c->cx=0x64u;c->ip=0x1500u;break;
 case 0x1500u:c->ip=0x1507u;break;
 case 0x1507u:v=c->bp;next=0x1508u;goto push;
 case 0x1508u:c->bp=c->sp;c->ip=0x150Au;break;
 case 0x150Au:
  p=span(s,c->ss,(uint16_t)(c->bp+0x10u),4u);if(!p)return PR_SYSTEM_READ_BLOCKED_MEMORY;
  c->di=word(p);c->es=word(p+2u);c->ip=0x150Du;break;
 case 0x150Du:v=0x1510u;next=0x14B6u;goto push;
 case 0x14B6u:
  p=span(s,c->es,(uint16_t)(c->di+2u),2u);if(!p)return PR_SYSTEM_READ_BLOCKED_MEMORY;
  c->flags=cmp_flags(c->flags,word(p),0xD7B3u);c->ip=0x14BCu;break;
 case 0x14BCu:c->ip=c->flags&0x40u?0x14C4u:0x14BEu;break;
 case 0x14C4u:
  p=span(s,c->ss,c->sp,2u);if(!p)return PR_SYSTEM_READ_BLOCKED_MEMORY;
  v=word(p);c->sp=(uint16_t)(c->sp+2u);c->ip=v;break;
 case 0x1510u:c->ip=c->flags&0x40u?0x1512u:0x1551u;break;
 case 0x1512u:
  p=span(s,c->ss,(uint16_t)(c->bp+0xAu),2u);if(!p)return PR_SYSTEM_READ_BLOCKED_MEMORY;
  c->ax=word(p);c->ip=0x1515u;break;
 case 0x1515u:
  c->flags=(uint16_t)(c->flags&(uint16_t)~0x8D5u);
  if(even(c->ax&255u))c->flags|=4u;
  if(!c->ax)c->flags|=0x40u;
  if(c->ax&0x8000u)c->flags|=0x80u;
  c->ip=0x1517u;break;
 case 0x1517u:c->ip=c->flags&0x40u?0x1535u:0x1519u;break;
 case 0x1519u:v=c->ds;next=0x151Au;goto push;
 case 0x151Au:v=c->cx;next=0x151Bu;goto push;
 case 0x151Bu:
  p=span(s,c->es,(uint16_t)(c->di+4u),2u);if(!p)return PR_SYSTEM_READ_BLOCKED_MEMORY;
  product=(uint32_t)c->ax*word(p);c->ax=(uint16_t)product;c->dx=(uint16_t)(product>>16);
  c->flags=(uint16_t)(c->flags&(uint16_t)~0x845u);
  if(!c->ax)c->flags|=0x40u;
  if(even(product))c->flags|=4u;
  if(c->dx)c->flags|=0x801u;
  c->ip=0x151Fu;break;
 case 0x151Fu:c->cx=c->ax;c->ip=0x1521u;break;
 case 0x1521u:
  p=span(s,c->ss,(uint16_t)(c->bp+0xCu),4u);if(!p)return PR_SYSTEM_READ_BLOCKED_MEMORY;
  c->dx=word(p);c->ds=word(p+2u);c->ip=0x1524u;break;
 case 0x1524u:c->ax=(uint16_t)((c->ax&0xFFu)|((c->bx&0xFFu)<<8));c->ip=0x1526u;break;
 case 0x1526u:
  p=span(s,c->es,c->di,2u);if(!p)return PR_SYSTEM_READ_BLOCKED_MEMORY;
  c->bx=word(p);c->ip=0x1529u;break;
 case 0x1529u:
  *r=(pr_system_read_report){1u,0xAF8u,0x1529u,0x152Bu,c->ax,c->bx,c->cx,c->ds,c->dx};
  return PR_SYSTEM_READ_BLOCKED_DOS;
 case 0x1535u:return PR_SYSTEM_READ_BLOCKED_ZERO_TAIL;
 case 0x14BEu:return PR_SYSTEM_READ_BLOCKED_SIGNATURE_ERROR;
 case 0x1551u:return PR_SYSTEM_READ_BLOCKED_ERROR_TAIL;
 default:return PR_SYSTEM_READ_BLOCKED_PC;
 }
 return PR_SYSTEM_READ_STEPPED;
push:
 off=(uint16_t)(c->sp-2u);p=span(s,c->ss,off,2u);if(!p)return PR_SYSTEM_READ_BLOCKED_MEMORY;
 put(p,v);c->sp=off;c->ip=next;return PR_SYSTEM_READ_STEPPED;
}
