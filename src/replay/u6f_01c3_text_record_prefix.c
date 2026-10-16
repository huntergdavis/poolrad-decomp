#include "poolrad/replay/u6f_01c3_text_record_prefix.h"
#include "poolrad/replay/slot_a_saved_copy_1965.h"
#include "poolrad/replay/system_b98_abi.h"

static uint8_t *span(pr_slot_a_directory_state *s,uint16_t off,size_t n)
{
    size_t p=((size_t)s->cpu.ss<<4)+off;
    if((size_t)off+n>0x10000u || p>s->ram_size || n>s->ram_size-p) return NULL;
    return s->ram+p;
}
static void put(uint8_t *p,uint16_t v)
{ p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8); }

pr_u6f_01c3_status pr_u6f_01c3_text_record_step(pr_slot_a_directory_state *s)
{
    pr_slot_a_directory_cpu *c; uint8_t *p;
    uint16_t value=0,next=0,target=0,pc; unsigned push=0;
    if(!s) return PR_U6F_01C3_ARGUMENT;
    if(!s->ram || s->ram_size>0x100000u) return PR_U6F_01C3_BLOCKED_MEMORY;
    c=&s->cpu; pc=c->ip;
    if(c->cs==0x131cu && pc>=0x0b0bu && pc<=0x0b22u){
        pr_slot_a_saved_copy_report report;
        pr_slot_a_saved_copy_status r=pr_slot_a_saved_copy_1965_step(s,&report);
        if(r==PR_SLOT_A_SAVED_COPY_BLOCKED_MEMORY) return PR_U6F_01C3_BLOCKED_MEMORY;
        if(r!=PR_SLOT_A_SAVED_COPY_STEPPED) return PR_U6F_01C3_BLOCKED_PC;
        if(pc==0x0b22u && (c->cs!=0x2a8eu || c->ip!=0x01d9u)) return PR_U6F_01C3_TRANSFER;
        return PR_U6F_01C3_STEPPED;
    }
    if(c->cs==0x131cu && pc>=0x0b98u && pc<=0x0bc1u){
        pr_system_b98_status r=pr_system_b98_step(s);
        if(r==PR_SYSTEM_B98_BLOCKED_MEMORY) return PR_U6F_01C3_BLOCKED_MEMORY;
        if(r!=PR_SYSTEM_B98_STEPPED) return PR_U6F_01C3_BLOCKED_PC;
        if(pc==0x0bc1u){
            if(c->cs==0x2a8eu && c->ip==0x01e3u) return PR_U6F_01C3_AT_04F7;
            return PR_U6F_01C3_TRANSFER;
        }
        return PR_U6F_01C3_STEPPED;
    }
    if(c->cs!=0x2a8eu) return PR_U6F_01C3_BLOCKED_PC;
    switch(pc){
    case 0x01c3: c->di=(uint16_t)(c->bp-0x104u); next=0x01c7; break;
    case 0x01c7: value=c->ss; push=1; next=0x01c8; break;
    case 0x01c8: value=c->di; push=1; next=0x01c9; break;
    case 0x01c9: c->di=(uint16_t)(c->bp-0x206u); next=0x01cd; break;
    case 0x01cd: value=c->ss; push=1; next=0x01ce; break;
    case 0x01ce: value=c->di; push=1; next=0x01cf; break;
    case 0x01cf: c->di=0x537a; next=0x01d2; break;
    case 0x01d2: value=c->ds; push=1; next=0x01d3; break;
    case 0x01d3: value=c->di; push=1; next=0x01d4; break;
    case 0x01d4: target=0x0b0b; next=0x01d9; break;
    case 0x01d9: c->di=0x0197; next=0x01dc; break;
    case 0x01dc: value=c->cs; push=1; next=0x01dd; break;
    case 0x01dd: value=c->di; push=1; next=0x01de; break;
    case 0x01de: target=0x0b98; next=0x01e3; break;
    case 0x01e3: return PR_U6F_01C3_AT_04F7;
    default: return PR_U6F_01C3_BLOCKED_PC;
    }
    if(push){
        p=span(s,(uint16_t)(c->sp-2u),2u);
        if(!p) return PR_U6F_01C3_BLOCKED_MEMORY;
        put(p,value); c->sp=(uint16_t)(c->sp-2u);
    }else if(target){
        p=span(s,(uint16_t)(c->sp-4u),4u);
        if(!p) return PR_U6F_01C3_BLOCKED_MEMORY;
        put(p+2u,c->cs); put(p,next);
        c->sp=(uint16_t)(c->sp-4u); c->cs=0x131c; next=target;
    }
    c->ip=next; return PR_U6F_01C3_STEPPED;
}
