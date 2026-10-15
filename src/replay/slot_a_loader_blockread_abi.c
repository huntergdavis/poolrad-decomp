#include "poolrad/replay/slot_a_loader_blockread_abi.h"
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
pr_slot_a_blockread_status pr_slot_a_loader_blockread_step(
 pr_slot_a_directory_state *s,pr_slot_a_blockread_report *r)
{
 pr_slot_a_directory_cpu *c;uint8_t *p;uint16_t off,value,next;
 if(!s || !r)return PR_SLOT_A_BLOCKREAD_ARGUMENT;
 *r=(pr_slot_a_blockread_report){0};
 if(!s->ram || s->ram_size>0x100000u)return PR_SLOT_A_BLOCKREAD_BLOCKED_MEMORY;
 c=&s->cpu;if(c->cs!=0x2A8Eu)return PR_SLOT_A_BLOCKREAD_BLOCKED_PC;
 next=(uint16_t)(c->ip+1u);
 switch(c->ip){
 case 0xC2Bu:
  p=span(s,c->ss,(uint16_t)(c->bp-0x1B9u),1u);if(!p)return PR_SLOT_A_BLOCKREAD_BLOCKED_MEMORY;
  *p=(uint8_t)c->ax;c->ip=0xC2Fu;break;
 case 0xC2Fu:case 0xC49u:case 0xC3Eu:
  c->di=(uint16_t)(c->bp-(c->ip==0xC3Eu?0x182u:0x180u));c->ip=(uint16_t)(c->ip+4u);break;
 case 0xC33u:case 0xC42u:case 0xC4Du:value=c->ss;goto push;
 case 0xC34u:case 0xC39u:case 0xC43u:case 0xC4Eu:value=c->di;goto push;
 case 0xC38u:value=c->es;goto push;
 case 0xC3Du:value=c->ax;goto push;
 case 0xC35u:case 0xC54u:case 0xC63u:case 0xC72u:case 0xC80u:
  p=span(s,c->ss,(uint16_t)(c->bp+0xAu),4u);if(!p)return PR_SLOT_A_BLOCKREAD_BLOCKED_MEMORY;
  c->di=word(p);c->es=word(p+2u);c->ip=(uint16_t)(c->ip+3u);break;
 case 0xC3Au:c->ax=0x11Du;c->ip=0xC3Du;break;
 case 0xC44u:case 0xC4Fu:
  *r=(pr_slot_a_blockread_report){1u,0x6Fu,c->ip,0xAF8u,(uint16_t)(c->ip==0xC44u?0x14FBu:0x1491u),(uint16_t)(c->ip+5u),1u};
  return c->ip==0xC44u?PR_SLOT_A_BLOCKREAD_BLOCKED_READ:PR_SLOT_A_BLOCKREAD_BLOCKED_CLOSE;
 case 0xC57u:case 0xC66u:case 0xC75u:case 0xC83u:
  c->ax=0u;c->flags=(uint16_t)((c->flags&(uint16_t)~0x8D5u)|0x44u);c->ip=(uint16_t)(c->ip+2u);break;
 case 0xC59u:off=0x104u;goto store;
 case 0xC5Eu:off=0x106u;goto store;
 case 0xC68u:off=0xC8u;goto store;
 case 0xC6Du:off=0xCAu;goto store;
 case 0xC77u:off=0x7Fu;goto store;
 case 0xC7Bu:off=0x81u;goto store;
 case 0xC85u:off=0x108u;goto store;
 case 0xC8Au:off=0x10Au;goto store;
 case 0xC8Fu:return PR_SLOT_A_BLOCKREAD_BLOCKED_SIDECAR;
 default:return PR_SLOT_A_BLOCKREAD_BLOCKED_PC;
 }
 return PR_SLOT_A_BLOCKREAD_STEPPED;
store:
 p=span(s,c->es,(uint16_t)(c->di+off),2u);if(!p)return PR_SLOT_A_BLOCKREAD_BLOCKED_MEMORY;
 put(p,c->ax);c->ip=(uint16_t)(c->ip+(c->ip==0xC77u?4u:5u));return PR_SLOT_A_BLOCKREAD_STEPPED;
push:
 off=(uint16_t)(c->sp-2u);p=span(s,c->ss,off,2u);if(!p)return PR_SLOT_A_BLOCKREAD_BLOCKED_MEMORY;
 put(p,value);c->sp=off;c->ip=next;return PR_SLOT_A_BLOCKREAD_STEPPED;
}
