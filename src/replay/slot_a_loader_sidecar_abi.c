#include "poolrad/replay/slot_a_loader_sidecar_abi.h"
#include "poolrad/replay/slot_a_saved_copy_1965.h"
#include "poolrad/replay/system_b98_abi.h"
static uint8_t *span(pr_slot_a_directory_state *s,uint16_t seg,uint16_t off,size_t n)
{
 size_t a=((size_t)seg<<4)+off;
 if((size_t)off+n>0x10000u || a>s->ram_size || n>s->ram_size-a)return NULL;
 return s->ram+a;
}
static void put(uint8_t *p,uint16_t v)
{p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static pr_slot_a_sidecar_status push(pr_slot_a_directory_state *s,uint16_t v,uint16_t next)
{
 uint16_t off=(uint16_t)(s->cpu.sp-2u);uint8_t *p=span(s,s->cpu.ss,off,2u);
 if(!p)return PR_SLOT_A_SIDECAR_BLOCKED_MEMORY;
 put(p,v);s->cpu.sp=off;s->cpu.ip=next;return PR_SLOT_A_SIDECAR_STEPPED;
}
pr_slot_a_sidecar_status pr_slot_a_loader_sidecar_prefix_step(
 pr_slot_a_directory_state *s,pr_slot_a_sidecar_report *r)
{
 pr_slot_a_directory_cpu *c;uint16_t value,target_cs,off;uint8_t *p;
 if(!s || !r)return PR_SLOT_A_SIDECAR_ARGUMENT;
 *r=(pr_slot_a_sidecar_report){0};
 if(!s->ram || s->ram_size>0x100000u)return PR_SLOT_A_SIDECAR_BLOCKED_MEMORY;
 c=&s->cpu;
 if(c->cs==0x131Cu && c->ip>=0xB0Bu && c->ip<=0xB22u){
  pr_slot_a_saved_copy_report child;pr_slot_a_saved_copy_status status=pr_slot_a_saved_copy_1965_step(s,&child);
  if(status==PR_SLOT_A_SAVED_COPY_STEPPED)return PR_SLOT_A_SIDECAR_STEPPED;
  if(status==PR_SLOT_A_SAVED_COPY_BLOCKED_MEMORY)return PR_SLOT_A_SIDECAR_BLOCKED_MEMORY;
  return PR_SLOT_A_SIDECAR_BLOCKED_PC;
 }
 if(c->cs==0x131Cu && c->ip>=0xB98u && c->ip<=0xBC1u){
  pr_system_b98_status status=pr_system_b98_step(s);
  if(status==PR_SYSTEM_B98_STEPPED)return PR_SLOT_A_SIDECAR_STEPPED;
  if(status==PR_SYSTEM_B98_BLOCKED_MEMORY)return PR_SLOT_A_SIDECAR_BLOCKED_MEMORY;
  return PR_SLOT_A_SIDECAR_BLOCKED_PC;
 }
 if((c->cs==0x1026u && c->ip>=0x2EBu && c->ip<=0x310u) ||
    (c->cs==0x131Cu && c->ip>=0xB25u && c->ip<0xB49u)){
  pr_slot_a_file_probe_status status=pr_slot_a_file_probe_02eb_step(s,r);
  if(status==PR_SLOT_A_FILE_PROBE_STEPPED)return PR_SLOT_A_SIDECAR_STEPPED;
  if(status==PR_SLOT_A_FILE_PROBE_BLOCKED_MEMORY)return PR_SLOT_A_SIDECAR_BLOCKED_MEMORY;
  if(status==PR_SLOT_A_FILE_PROBE_BLOCKED_RUNTIME)return PR_SLOT_A_SIDECAR_BLOCKED_ASSIGN;
  return PR_SLOT_A_SIDECAR_BLOCKED_PC;
 }
 if(c->cs!=0x2A8Eu)return PR_SLOT_A_SIDECAR_BLOCKED_PC;
 switch(c->ip){
 case 0xC8Fu:case 0xD8Fu:c->di=(uint16_t)(c->bp-0x2B9u);c->ip=(uint16_t)(c->ip+4u);break;
 case 0xC95u:case 0xD95u:c->di=(uint16_t)(c->bp-0x100u);c->ip=(uint16_t)(c->ip+4u);break;
 case 0xC93u:case 0xC99u:case 0xD93u:case 0xD99u:value=c->ss;goto push_word;
 case 0xC94u:case 0xC9Au:case 0xCA4u:case 0xD94u:case 0xD9Au:case 0xDA4u:value=c->di;goto push_word;
 case 0xCA0u:case 0xDA0u:c->di=c->ip==0xCA0u?0xAE2u:0xAE7u;c->ip=(uint16_t)(c->ip+3u);break;
 case 0xCA3u:case 0xDA3u:value=c->cs;goto push_word;
 case 0xC9Bu:case 0xD9Bu:value=0xB0Bu;goto far_call;
 case 0xCA5u:case 0xDA5u:value=0xB98u;goto far_call;
 case 0xCAAu:case 0xDAAu:value=0x2EBu;goto far_call;
 default:return PR_SLOT_A_SIDECAR_BLOCKED_PC;
 }
 return PR_SLOT_A_SIDECAR_STEPPED;
push_word:return push(s,value,(uint16_t)(c->ip+1u));
far_call:
 off=(uint16_t)(c->sp-4u);p=span(s,c->ss,off,4u);if(!p)return PR_SLOT_A_SIDECAR_BLOCKED_MEMORY;
 target_cs=value==0x2EBu?0x1026u:0x131Cu;put(p,(uint16_t)(c->ip+5u));put(p+2u,c->cs);
 c->sp=off;c->cs=target_cs;c->ip=value;return PR_SLOT_A_SIDECAR_STEPPED;
}
pr_slot_a_sidecar_status pr_slot_a_loader_sidecar_return_step(
 pr_slot_a_directory_state *s,pr_slot_a_sidecar_report *r)
{
 pr_slot_a_directory_cpu *c;unsigned v,p;uint16_t flags;
 if(!s || !r)return PR_SLOT_A_SIDECAR_ARGUMENT;
 *r=(pr_slot_a_sidecar_report){0};
 if(!s->ram || s->ram_size>0x100000u)return PR_SLOT_A_SIDECAR_BLOCKED_MEMORY;
 c=&s->cpu;if(c->cs!=0x2A8Eu)return PR_SLOT_A_SIDECAR_BLOCKED_PC;
 switch(c->ip){
 case 0xCAFu:case 0xDAFu:
  v=(uint8_t)c->ax;p=v;p^=p>>4;p^=p>>2;p^=p>>1;
  flags=(uint16_t)(c->flags&(uint16_t)~0x8D5u);
  if(!(p&1u))flags|=4u;
  if(!v)flags|=0x40u;
  if(v&0x80u)flags|=0x80u;
  c->flags=flags;c->ip=(uint16_t)(c->ip+2u);break;
 case 0xCB1u:c->ip=c->flags&0x40u?0xCB3u:0xCB6u;break;
 case 0xDB1u:c->ip=c->flags&0x40u?0xDB3u:0xDB6u;break;
 case 0xCB3u:c->ip=0xD8Fu;break;
 case 0xDB3u:c->ip=0xE8Du;break;
 case 0xCB6u:case 0xDB6u:c->ax=c->ip==0xCB6u?0x3Fu:9u;c->ip=(uint16_t)(c->ip+3u);break;
 case 0xCB9u:case 0xDB9u:return push(s,c->ax,(uint16_t)(c->ip+1u));
 case 0xCBAu:case 0xDBAu:
  *r=(pr_slot_a_sidecar_report){1u,0x6Fu,c->ip,0xAF8u,0x23Fu,(uint16_t)(c->ip+5u),1u};
  return PR_SLOT_A_SIDECAR_BLOCKED_ALLOCATOR;
 case 0xE8Du:
  *r=(pr_slot_a_sidecar_report){1u,0x6Fu,0xE8Du,0x899u,0x104u,0xE92u,1u};
  return PR_SLOT_A_SIDECAR_BLOCKED_PRINTER;
 case 0xD8Fu:return PR_SLOT_A_SIDECAR_BLOCKED_NEXT_CONTEXT;
 default:return PR_SLOT_A_SIDECAR_BLOCKED_PC;
 }
 return PR_SLOT_A_SIDECAR_STEPPED;
}
