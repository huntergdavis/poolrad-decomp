#include "poolrad/replay/u25_fourth_classifier_entry.h"
#include "poolrad/replay/u1c_fourth_store_entry.h"
#include "poolrad/replay/u25_fourth_value_return.h"
#include "poolrad/replay/u25_fourth_pair_return.h"
#include "poolrad/replay/u25_fourth_pair_entry.h"
#include "poolrad/replay/u25_fourth_slot_dispatch.h"
#include "poolrad/replay/u1c_fourth_save_branch.h"
#include "poolrad/replay/u25_fourth_loader_return.h"
#include "poolrad/replay/u1c_fourth_save_entry.h"
#include "poolrad/replay/u1c_fourth_dispatch.h"
#include "poolrad/replay/u1c_third_runner_next.h"
#include "poolrad/replay/u1c_third_dispatch_return.h"
#include "poolrad/replay/u1c_third_handler_return.h"
#include "poolrad/replay/u25_third_store_return.h"
#include "poolrad/replay/u25_third_class4_low.h"
#include "poolrad/replay/u25_third_store_dispatch.h"
#include "poolrad/replay/u25_third_classifier_return.h"
#include "poolrad/replay/u25_third_classifier_entry.h"
#include "poolrad/replay/u1c_third_store_entry.h"
#include "poolrad/replay/u25_third_value_return.h"
#include "poolrad/replay/u25_third_pair_return.h"
#include "poolrad/replay/u25_third_pair_entry.h"
#include "poolrad/replay/u25_third_slot_dispatch.h"
#include "poolrad/replay/u1c_third_save_branch.h"
#include "poolrad/replay/u25_third_loader_return.h"
#include "poolrad/replay/u1c_third_save_entry.h"
#include "poolrad/replay/u1c_third_dispatch.h"
#include "poolrad/replay/u1c_second_runner_next.h"
#include "poolrad/replay/u1c_second_dispatch_return.h"
#include "poolrad/replay/u1c_second_handler_return.h"
#include "poolrad/replay/u25_second_store_return.h"
#include "poolrad/replay/u25_second_class4_low.h"
#include "poolrad/replay/u25_second_store_dispatch.h"
#include "poolrad/replay/u25_second_classifier_return.h"
#include "poolrad/replay/u25_second_classifier_entry.h"
#include "poolrad/replay/u1c_second_store_entry.h"
#include "poolrad/replay/u25_second_value_return.h"
#include "poolrad/replay/u25_second_pair_return.h"
#include "poolrad/replay/u25_second_pair_entry.h"
#include "poolrad/replay/u25_second_slot_dispatch.h"
#include "poolrad/recovered/system_runtime_leaves_cpu.h"
#include "poolrad/replay/resident_1037_guarded.h"
#include <string.h>
#define MASK (PR_BUS_RAM_SIZE-1u)
static size_t at(uint16_t s,uint16_t o){return (((size_t)s<<4u)+o)&MASK;}
static uint8_t rb(pr_bus*b,uint16_t s,uint16_t o){return pr_bus_read8(b,at(s,o));}
static uint16_t rw(pr_bus*b,uint16_t s,uint16_t o){size_t p=at(s,o);uint8_t l=pr_bus_read8(b,p),h=pr_bus_read8(b,(p+1u)&MASK);return (uint16_t)(l|((uint16_t)h<<8u));}
static void wb(pr_bus*b,uint16_t s,uint16_t o,uint8_t v){pr_bus_write8(b,at(s,o),v);}
static void ww(pr_bus*b,uint16_t s,uint16_t o,uint16_t v){size_t p=at(s,o);pr_bus_write8(b,p,(uint8_t)v);pr_bus_write8(b,(p+1u)&MASK,(uint8_t)(v>>8u));}
static void push(pr_bus*b,pr_r0899_cpu*c,uint16_t v){c->sp=(uint16_t)(c->sp-2u);ww(b,c->ss,c->sp,v);}
static int ordinary(uint16_t s){return s<0x9000u||s>=0xb000u;}
static void cmp8(pr_r0899_cpu*c,uint8_t a,uint8_t b){unsigned r=(a-b)&255u,p=r;p^=p>>4;p^=p>>2;p^=p>>1;
    c->flags=(uint16_t)((c->flags&~0x08d5u)|(a<b?1u:0u)|((p&1u)?0u:4u)|(((a^b^r)&0x10u)?0x10u:0u)|(r==0?0x40u:0u)|(r&0x80u?0x80u:0u)|(((a^b)&(a^r)&0x80u)?0x800u:0u));}
#define FIELDS(X) X(ax) X(bx) X(cx) X(dx) X(si) X(di) X(bp) X(sp) X(cs) X(ip) X(ds) X(es) X(ss) X(flags)
#define TO(f) t.f=c->f;
#define FROM(f) c->f=t.f;
/* Stage 1=word formatter, 2=first store, 3=literal load,
 * 4=concatenation, 5=padding store. The flat leaves' live segment
 * arguments are checked only after the source far-call frame is built. */
static pr_290b_first_status string_call(pr_bus*b,size_t n,pr_r0899_cpu*c,pr_290b_first_diagnostic*d,uint16_t site,uint16_t target,int stage){
    uint16_t cs,sp,src,dst;int status;pr_system_leaf_cpu t;pr_system_0b25_cpu u;
    d->stage=stage;c->ip=site;if(rb(b,c->cs,site)!=0x9au||rw(b,c->cs,(uint16_t)(site+1u))!=target)return PR_290B_FIRST_CODE;
    cs=c->cs;sp=c->sp;push(b,c,cs);push(b,c,(uint16_t)(site+5u));c->cs=rw(b,cs,(uint16_t)(site+3u));c->ip=target;
    if(target==0x0b25u){src=rw(b,c->ss,(uint16_t)(c->sp+0x0cu));dst=rw(b,c->ss,(uint16_t)(c->sp+8u));}
    else{src=rw(b,c->ss,(uint16_t)(c->sp+6u));dst=rw(b,c->ss,(uint16_t)(c->sp+0x0au));}
    if(!ordinary(c->cs)||!ordinary(src)||!ordinary(dst))return PR_290B_FIRST_PROFILE;
    if(target==0x0b25u){
#define TO_STORE(f) u.f=c->f;
#define FROM_STORE(f) c->f=u.f;
        {FIELDS(TO_STORE)}status=(int)pr_system_strstore_0b25(b->ram,n,&u);{FIELDS(FROM_STORE)}
#undef TO_STORE
#undef FROM_STORE
    }else{{FIELDS(TO)}status=(int)(target==0x0b0bu?pr_system_strload_0b0b(b->ram,n,&t):pr_system_strcat_0b98(b->ram,n,&t));{FIELDS(FROM)}}
    d->status=status;if(status!=0)return PR_290B_FIRST_CALLEE;
    if(c->cs!=cs||c->ip!=(uint16_t)(site+5u)||c->sp!=(uint16_t)(sp+(target==0x0b25u?10u:4u)))return PR_290B_FIRST_TRANSFER;
    return PR_290B_FIRST_DONE;
}
pr_290b_first_status pr_290b_first_field(pr_bus*b,size_t n,pr_r0899_cpu*c,pr_290b_first_diagnostic*d){
    uint16_t cs,sp,rel,off;uint8_t v;pr_res_1073_cpu t;pr_res_1073_status fs;pr_290b_first_status st;
    if(!b||!b->ram||!b->ega||!c||!d||n!=PR_BUS_RAM_SIZE||c->ip!=0x290bu||c->sp<0x043eu||c->sp>0xfff0u||
       !ordinary(c->cs)||!ordinary(c->ds)||!ordinary(c->ss))return PR_290B_FIRST_ARGUMENT;
    memset(d,0,sizeof *d);d->formatter_callee=PR_RES_1037_NO_CALLEE;d->formatter_status=-1;
    push(b,c,c->bp);c->bp=c->sp;c->sp=(uint16_t)(c->sp-0x0232u);
    wb(b,c->ss,(uint16_t)(c->bp-0x31u),0);c->ax=(uint16_t)((c->ax&0xff00u)|rb(b,c->ds,0x6aadu));wb(b,c->ss,(uint16_t)(c->bp-1u),(uint8_t)c->ax);
    c->ax=(uint16_t)((c->ax&0xff00u)|rb(b,c->ds,0x6aaeu));wb(b,c->ss,(uint16_t)(c->bp-2u),(uint8_t)c->ax);
    v=rb(b,c->ds,0x49fau);cmp8(c,v,1u);if(v>1u){c->ip=0x2929u;return PR_290B_FIRST_OUTSIDE;}
    c->di=(uint16_t)(c->bp-0x0131u);push(b,c,c->ss);push(b,c,c->di);
    c->di=rw(b,c->ds,0x49d2u);c->es=rw(b,c->ds,0x49d4u);off=(uint16_t)(c->di+0x0192u);
    if(off==0xffffu){c->ip=0x2961u;return PR_290B_FIRST_PROFILE;}
    push(b,c,rw(b,c->es,off));push(b,c,c->cs);c->ip=0x2967u;d->stage=1;rel=rw(b,c->cs,0x2968u);
    if(rb(b,c->cs,0x2967u)!=0xe8u||(uint16_t)(0x296au+rel)!=0x1073u)return PR_290B_FIRST_CODE;
    cs=c->cs;sp=(uint16_t)(c->sp+4u);push(b,c,0x296au);c->ip=0x1073u;
    {FIELDS(TO)}fs=pr_res_00ba_1073_cpu(b,n,&t,&d->formatter_callee,&d->formatter_status);{FIELDS(FROM)}
    d->status=(int)fs;if(fs!=PR_RES_1073_OK)return PR_290B_FIRST_CALLEE;
    if(c->cs!=cs||c->ip!=0x296au||c->sp!=sp)return PR_290B_FIRST_TRANSFER;
    c->di=(uint16_t)(c->bp-5u);push(b,c,c->ss);push(b,c,c->di);c->ax=2;push(b,c,c->ax);
    st=string_call(b,n,c,d,0x2973u,0x0b25u,2);if(st)return st;
    v=rb(b,c->ss,(uint16_t)(c->bp-5u));cmp8(c,v,2u);
    if(v<2u){
        c->di=(uint16_t)(c->bp-0x0131u);push(b,c,c->ss);push(b,c,c->di);c->di=0x28f2u;push(b,c,c->cs);push(b,c,c->di);
        st=string_call(b,n,c,d,0x2989u,0x0b0bu,3);if(st)return st;
        c->di=(uint16_t)(c->bp-5u);push(b,c,c->ss);push(b,c,c->di);
        st=string_call(b,n,c,d,0x2993u,0x0b98u,4);if(st)return st;
        c->di=(uint16_t)(c->bp-5u);push(b,c,c->ss);push(b,c,c->di);c->ax=2;push(b,c,c->ax);
        st=string_call(b,n,c,d,0x29a1u,0x0b25u,5);if(st)return st;
    }
    c->ip=0x29a6u;return PR_290B_FIRST_DONE;
}
void pr_u68_290b_init(pr_u68_290b*s){if(s){memset(s,0,sizeof *s);pr_u68_prompt_init(&s->u68);}}
static pr_u68_290b_status next(pr_u68_290b*s,pr_u68_290b_diagnostic*d,pr_u68_prompt_status st){
    pr_r0899_cpu*c=s->cpu;pr_bus*b=s->bus;uint16_t cs;
    d->u68_status=st;if(st==PR_U68_PROMPT_YIELD){s->phase=PR_U68_290B_WAITING;return PR_U68_290B_YIELD;}
    s->phase=PR_U68_290B_FINISHED;if(st!=PR_U68_PROMPT_DISPLAY)return PR_U68_290B_STOP;
    if(c->ip!=0x0b9bu||rb(b,c->cs,0x0b9bu)!=0x9au||rw(b,c->cs,0x0b9cu)!=0x290bu)return PR_U68_290B_CODE;
    cs=c->cs;push(b,c,cs);push(b,c,0x0ba0u);c->cs=rw(b,cs,0x0b9eu);c->ip=0x290bu;
    if(!ordinary(c->cs))return PR_U68_290B_PROFILE;
    d->first_status=pr_290b_first_field(b,PR_BUS_RAM_SIZE,c,&d->first);
    return d->first_status==PR_290B_FIRST_DONE?PR_U68_290B_FIRST_FIELD:PR_U68_290B_STOP;
}
pr_u68_290b_status pr_u68_290b_start(pr_u68_290b*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,pr_u68_290b_diagnostic*d){
    pr_u68_prompt_status st;if(!s||!b||!c||!e||!d||s->phase!=PR_U68_290B_EMPTY)return PR_U68_290B_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_u68_prompt_start(&s->u68,b,n,c,e,&d->u68);return next(s,d,st);
}
pr_u68_290b_status pr_u68_290b_resume(pr_u68_290b*s,pr_u68_290b_diagnostic*d){pr_u68_prompt_status st;
    if(!s||!d||s->phase!=PR_U68_290B_WAITING)return PR_U68_290B_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_u68_prompt_resume(&s->u68,&d->u68);
    if(st==PR_U68_PROMPT_ARGUMENT)return PR_U68_290B_ARGUMENT;
    return next(s,d,st);
}

/* Pinned normal/386 MULW: SF/AF retained, ZF from AX, PF from all four
 * product bytes, CF/OF set iff DX nonzero (the later ADD normally kills it). */
static void second_mul(pr_r0899_cpu*c,uint16_t value){uint32_t p=(uint32_t)c->ax*value,fold=p;
    c->ax=(uint16_t)p;c->dx=(uint16_t)(p>>16);fold^=fold>>16;fold^=fold>>8;fold^=fold>>4;fold^=fold>>2;fold^=fold>>1;
    c->flags=(uint16_t)((c->flags&~0x0845u)|(c->dx?0x0801u:0u)|(c->ax==0?0x0040u:0u)|((fold&1u)?0u:0x0004u));}
static uint16_t second_add(pr_r0899_cpu*c,uint16_t a,uint16_t b){unsigned wide=(unsigned)a+b,r=wide&0xffffu,p=r&255u;
    p^=p>>4;p^=p>>2;p^=p>>1;c->flags=(uint16_t)((c->flags&~0x08d5u)|(wide>0xffffu?1u:0u)|((p&1u)?0u:4u)|
        (((a^b^r)&0x10u)?0x10u:0u)|(r==0?0x40u:0u)|(r&0x8000u?0x80u:0u)|((~(a^b)&(a^r)&0x8000u)?0x800u:0u));return (uint16_t)r;}
pr_290b_first_status pr_290b_second_field(pr_bus*b,size_t n,pr_r0899_cpu*c,pr_290b_first_diagnostic*d){
    uint16_t off,cs,sp,rel;uint8_t v;pr_res_1073_cpu t;pr_res_1073_status fs;pr_290b_first_status st;
    if(!b||!b->ram||!b->ega||!c||!d||n!=PR_BUS_RAM_SIZE||c->ip!=0x29a6u||c->bp<0x043cu||c->bp>0xffeeu||
       c->sp!=(uint16_t)(c->bp-0x0232u)||!ordinary(c->cs)||!ordinary(c->ds)||!ordinary(c->ss))return PR_290B_FIRST_ARGUMENT;
    memset(d,0,sizeof *d);d->formatter_callee=PR_RES_1037_NO_CALLEE;d->formatter_status=-1;
    c->di=(uint16_t)(c->bp-0x0131u);push(b,c,c->ss);push(b,c,c->di);
    c->di=rw(b,c->ds,0x49d2u);c->es=rw(b,c->ds,0x49d4u);c->ax=10u;off=(uint16_t)(c->di+0x0190u);
    if(off==0xffffu){c->ip=0x29b3u;return PR_290B_FIRST_PROFILE;}
    second_mul(c,rw(b,c->es,off));c->di=rw(b,c->ds,0x49d2u);c->es=rw(b,c->ds,0x49d4u);off=(uint16_t)(c->di+0x018eu);
    if(off==0xffffu){c->ip=0x29bcu;return PR_290B_FIRST_PROFILE;}
    c->ax=second_add(c,c->ax,rw(b,c->es,off));push(b,c,c->ax);push(b,c,c->cs);c->ip=0x29c3u;d->stage=1;rel=rw(b,c->cs,0x29c4u);
    if(rb(b,c->cs,0x29c3u)!=0xe8u||(uint16_t)(0x29c6u+rel)!=0x1073u)return PR_290B_FIRST_CODE;
    cs=c->cs;sp=(uint16_t)(c->sp+4u);push(b,c,0x29c6u);c->ip=0x1073u;
    {FIELDS(TO)}fs=pr_res_00ba_1073_cpu(b,n,&t,&d->formatter_callee,&d->formatter_status);{FIELDS(FROM)}
    d->status=(int)fs;if(fs!=PR_RES_1073_OK)return PR_290B_FIRST_CALLEE;
    if(c->cs!=cs||c->ip!=0x29c6u||c->sp!=sp)return PR_290B_FIRST_TRANSFER;
    c->di=(uint16_t)(c->bp-8u);push(b,c,c->ss);push(b,c,c->di);c->ax=2;push(b,c,c->ax);
    st=string_call(b,n,c,d,0x29cfu,0x0b25u,2);if(st)return st;
    v=rb(b,c->ss,(uint16_t)(c->bp-8u));cmp8(c,v,2u);
    if(v<2u){
        c->di=(uint16_t)(c->bp-0x0131u);push(b,c,c->ss);push(b,c,c->di);c->di=0x28f2u;push(b,c,c->cs);push(b,c,c->di);
        st=string_call(b,n,c,d,0x29e5u,0x0b0bu,3);if(st)return st;
        c->di=(uint16_t)(c->bp-8u);push(b,c,c->ss);push(b,c,c->di);
        st=string_call(b,n,c,d,0x29efu,0x0b98u,4);if(st)return st;
        c->di=(uint16_t)(c->bp-8u);push(b,c,c->ss);push(b,c,c->di);c->ax=2;push(b,c,c->ax);
        st=string_call(b,n,c,d,0x29fdu,0x0b25u,5);if(st)return st;
    }
    c->ip=0x2a02u;return PR_290B_FIRST_DONE;
}
void pr_u68_290b_two_init(pr_u68_290b_two*s){if(s){memset(s,0,sizeof *s);pr_u68_290b_init(&s->first);}}
static pr_u68_290b_two_status two_next(pr_u68_290b_two*s,pr_u68_290b_two_diagnostic*d,pr_u68_290b_status st){
    d->first_status=st;if(st==PR_U68_290B_YIELD){s->phase=PR_U68_290B_TWO_WAITING;return PR_U68_290B_TWO_YIELD;}
    s->phase=PR_U68_290B_TWO_FINISHED;if(st!=PR_U68_290B_FIRST_FIELD)return PR_U68_290B_TWO_STOP;
    d->second_status=pr_290b_second_field(s->bus,PR_BUS_RAM_SIZE,s->cpu,&d->second);
    return d->second_status==PR_290B_FIRST_DONE?PR_U68_290B_TWO_FIELDS:PR_U68_290B_TWO_STOP;
}
pr_u68_290b_two_status pr_u68_290b_two_start(pr_u68_290b_two*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,pr_u68_290b_two_diagnostic*d){
    pr_u68_290b_status st;if(!s||!b||!c||!e||!d||s->phase!=PR_U68_290B_TWO_EMPTY)return PR_U68_290B_TWO_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_u68_290b_start(&s->first,b,n,c,e,&d->first);return two_next(s,d,st);
}
pr_u68_290b_two_status pr_u68_290b_two_resume(pr_u68_290b_two*s,pr_u68_290b_two_diagnostic*d){pr_u68_290b_status st;
    if(!s||!d||s->phase!=PR_U68_290B_TWO_WAITING)return PR_U68_290B_TWO_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_u68_290b_resume(&s->first,&d->first);
    if(st==PR_U68_290B_ARGUMENT)return PR_U68_290B_TWO_ARGUMENT;
    return two_next(s,d,st);
}

/* CMP word with zero: CF/OF/AF clear; SF/ZF/PF from the result. */
static void branch_cmp_zero(pr_r0899_cpu*c,uint16_t v){unsigned p=v&255u;p^=p>>4;p^=p>>2;p^=p>>1;
    c->flags=(uint16_t)((c->flags&~0x08d5u)|(v&0x8000u?0x80u:0u)|(v==0?0x40u:0u)|((p&1u)?0u:4u));}
static pr_290b_first_status byte_call(pr_bus*b,size_t n,pr_r0899_cpu*c,pr_290b_first_diagnostic*d,uint16_t site,int stage){
    uint16_t cs,sp,rel;pr_res_1037_cpu t;pr_res_1037_guarded_status st;
    d->stage=stage;c->ip=site;rel=rw(b,c->cs,(uint16_t)(site+1u));
    if(rb(b,c->cs,site)!=0xe8u||(uint16_t)(site+3u+rel)!=0x1037u)return PR_290B_FIRST_CODE;
    cs=c->cs;sp=(uint16_t)(c->sp+4u);push(b,c,(uint16_t)(site+3u));c->ip=0x1037u;
    {FIELDS(TO)}st=pr_res_00ba_1037_guarded(b,n,&t,&d->formatter_callee,&d->formatter_status);{FIELDS(FROM)}
    d->status=(int)st;if(st==PR_RES_1037_GUARDED_PROFILE)return PR_290B_FIRST_PROFILE;
    if(st!=PR_RES_1037_GUARDED_OK)return PR_290B_FIRST_CALLEE;
    if(c->cs!=cs||c->ip!=(uint16_t)(site+3u)||c->sp!=sp)return PR_290B_FIRST_TRANSFER;
    return PR_290B_FIRST_DONE;
}
pr_290b_first_status pr_290b_record_branch(pr_bus*b,size_t n,pr_r0899_cpu*c,pr_290b_first_diagnostic*d){
    uint16_t off,v;pr_290b_first_status st;
    if(!b||!b->ram||!b->ega||!c||!d||n!=PR_BUS_RAM_SIZE||c->ip!=0x2a02u||c->bp<0x043cu||c->bp>0xffeeu||
       c->sp!=(uint16_t)(c->bp-0x0232u)||!ordinary(c->cs)||!ordinary(c->ds)||!ordinary(c->ss))return PR_290B_FIRST_ARGUMENT;
    memset(d,0,sizeof *d);d->formatter_callee=PR_RES_1037_NO_CALLEE;d->formatter_status=-1;
    c->di=rw(b,c->ds,0x49d2u);c->es=rw(b,c->ds,0x49d4u);off=(uint16_t)(c->di+0x01f6u);
    if(off==0xffffu){c->ip=0x2a06u;return PR_290B_FIRST_PROFILE;}
    v=rw(b,c->es,off);branch_cmp_zero(c,v);if(v){c->ip=0x2a51u;return PR_290B_FIRST_DONE;}
    c->di=(uint16_t)(c->bp-0x0131u);push(b,c,c->ss);push(b,c,c->di);c->ax=(uint16_t)((c->ax&0xff00u)|rb(b,c->ss,(uint16_t)(c->bp-1u)));push(b,c,c->ax);push(b,c,c->cs);
    st=byte_call(b,n,c,d,0x2a19u,1);if(st)return st;
    c->di=0x28f4u;push(b,c,c->cs);push(b,c,c->di);st=string_call(b,n,c,d,0x2a21u,0x0b98u,2);if(st)return st;
    c->di=(uint16_t)(c->bp-0x0231u);push(b,c,c->ss);push(b,c,c->di);c->ax=(uint16_t)((c->ax&0xff00u)|rb(b,c->ss,(uint16_t)(c->bp-2u)));push(b,c,c->ax);push(b,c,c->cs);
    st=byte_call(b,n,c,d,0x2a31u,3);if(st)return st;
    st=string_call(b,n,c,d,0x2a34u,0x0b98u,4);if(st)return st;
    c->di=0x28f6u;push(b,c,c->cs);push(b,c,c->di);st=string_call(b,n,c,d,0x2a3eu,0x0b98u,5);if(st)return st;
    c->di=(uint16_t)(c->bp-0x31u);push(b,c,c->ss);push(b,c,c->di);c->ax=0x28u;push(b,c,c->ax);
    st=string_call(b,n,c,d,0x2a4cu,0x0b25u,6);if(st)return st;
    c->ip=0x2a51u;return PR_290B_FIRST_DONE;
}

void pr_u68_290b_branch_init(pr_u68_290b_branch*s){if(s){memset(s,0,sizeof *s);pr_u68_290b_two_init(&s->two);}}
static pr_u68_290b_branch_status branch_next(pr_u68_290b_branch*s,pr_u68_290b_branch_diagnostic*d,pr_u68_290b_two_status st){
    d->two_status=st;if(st==PR_U68_290B_TWO_YIELD){s->phase=PR_U68_290B_BRANCH_WAITING;return PR_U68_290B_BRANCH_YIELD;}
    s->phase=PR_U68_290B_BRANCH_FINISHED;if(st!=PR_U68_290B_TWO_FIELDS)return PR_U68_290B_BRANCH_STOP;
    d->branch_status=pr_290b_record_branch(s->bus,PR_BUS_RAM_SIZE,s->cpu,&d->branch);
    return d->branch_status==PR_290B_FIRST_DONE?PR_U68_290B_BRANCH_REACHED:PR_U68_290B_BRANCH_STOP;
}
pr_u68_290b_branch_status pr_u68_290b_branch_start(pr_u68_290b_branch*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,pr_u68_290b_branch_diagnostic*d){
    pr_u68_290b_two_status st;if(!s||!b||!c||!e||!d||s->phase!=PR_U68_290B_BRANCH_EMPTY)return PR_U68_290B_BRANCH_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_u68_290b_two_start(&s->two,b,n,c,e,&d->two);return branch_next(s,d,st);
}
pr_u68_290b_branch_status pr_u68_290b_branch_resume(pr_u68_290b_branch*s,pr_u68_290b_branch_diagnostic*d){pr_u68_290b_two_status st;
    if(!s||!d||s->phase!=PR_U68_290B_BRANCH_WAITING)return PR_U68_290B_BRANCH_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_u68_290b_two_resume(&s->two,&d->two);
    if(st==PR_U68_290B_TWO_ARGUMENT)return PR_U68_290B_BRANCH_ARGUMENT;
    return branch_next(s,d,st);
}

pr_290b_first_status pr_290b_assembly(pr_bus*b,size_t n,pr_r0899_cpu*c,pr_290b_first_diagnostic*d){
    pr_290b_first_status st;
    if(!b||!b->ram||!b->ega||!c||!d||n!=PR_BUS_RAM_SIZE||c->ip!=0x2a51u||c->bp<0x043cu||c->bp>0xffeeu||
       c->sp!=(uint16_t)(c->bp-0x0232u)||!ordinary(c->cs)||!ordinary(c->ds)||!ordinary(c->ss))return PR_290B_FIRST_ARGUMENT;
    memset(d,0,sizeof *d);d->formatter_callee=PR_RES_1037_NO_CALLEE;d->formatter_status=-1;
    c->di=(uint16_t)(c->bp-0x0131u);push(b,c,c->ss);push(b,c,c->di);c->di=(uint16_t)(c->bp-0x31u);push(b,c,c->ss);push(b,c,c->di);
    st=string_call(b,n,c,d,0x2a5cu,0x0b0bu,1);if(st)return st;
    c->ax=rb(b,c->ds,0x6aafu);c->di=c->ax;c->si=c->di;c->di=(uint16_t)(c->di*3u);c->di=second_add(c,c->di,0x25a8u);
    push(b,c,c->ds);push(b,c,c->di);st=string_call(b,n,c,d,0x2a74u,0x0b98u,2);if(st)return st;
    c->di=0x28f6u;push(b,c,c->cs);push(b,c,c->di);st=string_call(b,n,c,d,0x2a7eu,0x0b98u,3);if(st)return st;
    c->di=(uint16_t)(c->bp-0x31u);push(b,c,c->ss);push(b,c,c->di);c->ax=0x28u;push(b,c,c->ax);
    st=string_call(b,n,c,d,0x2a8cu,0x0b25u,4);if(st)return st;
    c->di=(uint16_t)(c->bp-0x0131u);push(b,c,c->ss);push(b,c,c->di);c->di=(uint16_t)(c->bp-0x31u);push(b,c,c->ss);push(b,c,c->di);
    st=string_call(b,n,c,d,0x2a9cu,0x0b0bu,5);if(st)return st;
    c->di=(uint16_t)(c->bp-5u);push(b,c,c->ss);push(b,c,c->di);st=string_call(b,n,c,d,0x2aa6u,0x0b98u,6);if(st)return st;
    c->di=0x28f8u;push(b,c,c->cs);push(b,c,c->di);st=string_call(b,n,c,d,0x2ab0u,0x0b98u,7);if(st)return st;
    c->di=(uint16_t)(c->bp-8u);push(b,c,c->ss);push(b,c,c->di);st=string_call(b,n,c,d,0x2abau,0x0b98u,8);if(st)return st;
    c->di=(uint16_t)(c->bp-0x31u);push(b,c,c->ss);push(b,c,c->di);c->ax=0x28u;push(b,c,c->ax);
    st=string_call(b,n,c,d,0x2ac8u,0x0b25u,9);if(st)return st;
    c->ip=0x2acdu;return PR_290B_FIRST_DONE;
}
void pr_u68_290b_assembly_init(pr_u68_290b_assembly*s){if(s){memset(s,0,sizeof *s);pr_u68_290b_branch_init(&s->branch);}}
static pr_u68_290b_assembly_status assembly_next(pr_u68_290b_assembly*s,pr_u68_290b_assembly_diagnostic*d,pr_u68_290b_branch_status st){
    d->branch_status=st;if(st==PR_U68_290B_BRANCH_YIELD){s->phase=PR_U68_290B_ASSEMBLY_WAITING;return PR_U68_290B_ASSEMBLY_YIELD;}
    s->phase=PR_U68_290B_ASSEMBLY_FINISHED;if(st!=PR_U68_290B_BRANCH_REACHED)return PR_U68_290B_ASSEMBLY_STOP;
    d->assembly_status=pr_290b_assembly(s->bus,PR_BUS_RAM_SIZE,s->cpu,&d->assembly);
    return d->assembly_status==PR_290B_FIRST_DONE?PR_U68_290B_ASSEMBLY_REACHED:PR_U68_290B_ASSEMBLY_STOP;
}
pr_u68_290b_assembly_status pr_u68_290b_assembly_start(pr_u68_290b_assembly*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,pr_u68_290b_assembly_diagnostic*d){
    pr_u68_290b_branch_status st;if(!s||!b||!c||!e||!d||s->phase!=PR_U68_290B_ASSEMBLY_EMPTY)return PR_U68_290B_ASSEMBLY_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_u68_290b_branch_start(&s->branch,b,n,c,e,&d->branch);return assembly_next(s,d,st);
}
pr_u68_290b_assembly_status pr_u68_290b_assembly_resume(pr_u68_290b_assembly*s,pr_u68_290b_assembly_diagnostic*d){pr_u68_290b_branch_status st;
    if(!s||!d||s->phase!=PR_U68_290B_ASSEMBLY_WAITING)return PR_U68_290B_ASSEMBLY_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_u68_290b_branch_resume(&s->branch,&d->branch);
    if(st==PR_U68_290B_BRANCH_ARGUMENT)return PR_U68_290B_ASSEMBLY_ARGUMENT;
    return assembly_next(s,d,st);
}

pr_290b_first_status pr_290b_mode(pr_bus*b,size_t n,pr_r0899_cpu*c,pr_290b_first_diagnostic*d){
    uint8_t mode;uint16_t off,literal,site;pr_290b_first_status st;
    if(!b||!b->ram||!b->ega||!c||!d||n!=PR_BUS_RAM_SIZE||c->ip!=0x2acdu||c->bp<0x043cu||c->bp>0xffeeu||
       c->sp!=(uint16_t)(c->bp-0x0232u)||!ordinary(c->cs)||!ordinary(c->ds)||!ordinary(c->ss))return PR_290B_FIRST_ARGUMENT;
    memset(d,0,sizeof *d);d->formatter_callee=PR_RES_1037_NO_CALLEE;d->formatter_status=-1;
    mode=rb(b,c->ds,0x49f3u);cmp8(c,mode,2u);
    if(mode==2u){literal=0x28fau;site=0x2adfu;}
    else{
        c->di=rw(b,c->ds,0x49d6u);c->es=rw(b,c->ds,0x49d8u);off=(uint16_t)(c->di+0x0594u);
        if(off==0xffffu){c->ip=0x2b02u;return PR_290B_FIRST_PROFILE;}
        c->ax=(uint16_t)(rw(b,c->es,off)&1u);
        c->flags=(uint16_t)((c->flags&~0x08d5u)|(c->ax==0?0x40u:0u)|(c->ax==0?4u:0u));
        if(c->ax==0){c->ip=0x2b36u;return PR_290B_FIRST_DONE;}
        literal=0x2903u;site=0x2b19u;
    }
    c->di=(uint16_t)(c->bp-0x0131u);push(b,c,c->ss);push(b,c,c->di);c->di=(uint16_t)(c->bp-0x31u);push(b,c,c->ss);push(b,c,c->di);
    st=string_call(b,n,c,d,site,0x0b0bu,1);if(st)return st;
    c->di=literal;push(b,c,c->cs);push(b,c,c->di);st=string_call(b,n,c,d,(uint16_t)(site+10u),0x0b98u,2);if(st)return st;
    c->di=(uint16_t)(c->bp-0x31u);push(b,c,c->ss);push(b,c,c->di);c->ax=0x28u;push(b,c,c->ax);
    st=string_call(b,n,c,d,(uint16_t)(site+24u),0x0b25u,3);if(st)return st;
    c->ip=0x2b36u;return PR_290B_FIRST_DONE;
}
void pr_u68_290b_mode_init(pr_u68_290b_mode*s){if(s){memset(s,0,sizeof *s);pr_u68_290b_assembly_init(&s->assembly);}}
static pr_u68_290b_mode_status mode_next(pr_u68_290b_mode*s,pr_u68_290b_mode_diagnostic*d,pr_u68_290b_assembly_status st){
    d->assembly_status=st;if(st==PR_U68_290B_ASSEMBLY_YIELD){s->phase=PR_U68_290B_MODE_WAITING;return PR_U68_290B_MODE_YIELD;}
    s->phase=PR_U68_290B_MODE_FINISHED;if(st!=PR_U68_290B_ASSEMBLY_REACHED)return PR_U68_290B_MODE_STOP;
    d->mode_status=pr_290b_mode(s->bus,PR_BUS_RAM_SIZE,s->cpu,&d->mode);
    return d->mode_status==PR_290B_FIRST_DONE?PR_U68_290B_MODE_REACHED:PR_U68_290B_MODE_STOP;
}
pr_u68_290b_mode_status pr_u68_290b_mode_start(pr_u68_290b_mode*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,pr_u68_290b_mode_diagnostic*d){
    pr_u68_290b_assembly_status st;if(!s||!b||!c||!e||!d||s->phase!=PR_U68_290B_MODE_EMPTY)return PR_U68_290B_MODE_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_u68_290b_assembly_start(&s->assembly,b,n,c,e,&d->assembly);return mode_next(s,d,st);
}
pr_u68_290b_mode_status pr_u68_290b_mode_resume(pr_u68_290b_mode*s,pr_u68_290b_mode_diagnostic*d){pr_u68_290b_assembly_status st;
    if(!s||!d||s->phase!=PR_U68_290B_MODE_WAITING)return PR_U68_290B_MODE_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_u68_290b_assembly_resume(&s->assembly,&d->assembly);
    if(st==PR_U68_290B_ASSEMBLY_ARGUMENT)return PR_U68_290B_MODE_ARGUMENT;
    return mode_next(s,d,st);
}

pr_290b_first_status pr_290b_clear(pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_290b_clear_limits*l,pr_290b_clear_diagnostic*d){
    uint16_t cs,sp;pr_r03f1_clear_cpu t;
    if(!b||!b->ram||!b->ega||!c||!l||!d||n!=PR_BUS_RAM_SIZE||c->ip!=0x2b36u||c->bp<0x043cu||c->bp>0xffeeu||
       c->sp!=(uint16_t)(c->bp-0x0232u)||!ordinary(c->cs)||!ordinary(c->ds)||!ordinary(c->ss))return PR_290B_FIRST_ARGUMENT;
    memset(d,0,sizeof *d);d->status=-1;
    c->ax=(uint16_t)((c->ax&0xff00u)|0x11u);push(b,c,c->ax);c->ax=(uint16_t)((c->ax&0xff00u)|0x0fu);push(b,c,c->ax);
    c->ax=(uint16_t)((c->ax&0xff00u)|0x26u);push(b,c,c->ax);c->ax=(uint16_t)((c->ax&0xff00u)|0x0fu);push(b,c,c->ax);
    c->ip=0x2b42u;if(rb(b,c->cs,c->ip)!=0x9au||rw(b,c->cs,0x2b43u)!=0x0137u)return PR_290B_FIRST_CODE;
    cs=c->cs;sp=(uint16_t)(c->sp+8u);push(b,c,cs);push(b,c,0x2b47u);c->cs=rw(b,cs,0x2b45u);c->ip=0x0137u;
    if(!ordinary(c->cs))return PR_290B_FIRST_PROFILE;
    {FIELDS(TO)}d->status=(int)pr_r03f1_clear_0137_cpu(b,n,&t,l->clear_loopbacks,l->rect_loopbacks,&d->clear);{FIELDS(FROM)}
    if(d->status!=PR_R03F1_CLEAR_CPU_OK)return PR_290B_FIRST_CALLEE;
    if(c->cs!=cs||c->ip!=0x2b47u||c->sp!=sp)return PR_290B_FIRST_TRANSFER;
    return PR_290B_FIRST_DONE;
}
void pr_u68_290b_clear_init(pr_u68_290b_clear*s){if(s){memset(s,0,sizeof *s);pr_u68_290b_mode_init(&s->mode);}}
static pr_u68_290b_clear_status clear_next(pr_u68_290b_clear*s,pr_u68_290b_clear_diagnostic*d,pr_u68_290b_mode_status st){
    d->mode_status=st;if(st==PR_U68_290B_MODE_YIELD){s->phase=PR_U68_290B_CLEAR_WAITING;return PR_U68_290B_CLEAR_YIELD;}
    s->phase=PR_U68_290B_CLEAR_FINISHED;if(st!=PR_U68_290B_MODE_REACHED)return PR_U68_290B_CLEAR_STOP;
    d->clear_status=pr_290b_clear(s->bus,PR_BUS_RAM_SIZE,s->cpu,&s->limits,&d->clear);
    return d->clear_status==PR_290B_FIRST_DONE?PR_U68_290B_CLEAR_REACHED:PR_U68_290B_CLEAR_STOP;
}
pr_u68_290b_clear_status pr_u68_290b_clear_start(pr_u68_290b_clear*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*l,pr_u68_290b_clear_diagnostic*d){
    pr_u68_290b_mode_status st;if(!s||!b||!c||!e||!l||!d||s->phase!=PR_U68_290B_CLEAR_EMPTY)return PR_U68_290B_CLEAR_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;s->limits=*l;st=pr_u68_290b_mode_start(&s->mode,b,n,c,e,&d->mode);return clear_next(s,d,st);
}
pr_u68_290b_clear_status pr_u68_290b_clear_resume(pr_u68_290b_clear*s,pr_u68_290b_clear_diagnostic*d){pr_u68_290b_mode_status st;
    if(!s||!d||s->phase!=PR_U68_290B_CLEAR_WAITING)return PR_U68_290B_CLEAR_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_u68_290b_mode_resume(&s->mode,&d->mode);
    if(st==PR_U68_290B_MODE_ARGUMENT)return PR_U68_290B_CLEAR_ARGUMENT;
    return clear_next(s,d,st);
}

pr_290b_first_status pr_290b_print_return(pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_290b_print_limits*l,const pr_290b_return_target*x,pr_290b_print_diagnostic*d){
    uint16_t cs,sp;pr_r0709_printer_cpu t;
    if(!b||!b->ram||!b->ega||!c||!l||!x||!d||n!=PR_BUS_RAM_SIZE||c->ip!=0x2b47u||c->bp<0x043cu||c->bp>0xffeeu||
       c->sp!=(uint16_t)(c->bp-0x0232u)||!ordinary(c->cs)||!ordinary(c->ds)||!ordinary(c->ss))return PR_290B_FIRST_ARGUMENT;
    memset(d,0,sizeof *d);d->status=-1;
    c->ax=(uint16_t)((c->ax&0xff00u)|0x11u);push(b,c,c->ax);c->ax=(uint16_t)((c->ax&0xff00u)|0x0fu);push(b,c,c->ax);
    c->ax=(uint16_t)((c->ax&0xff00u)|0x0au);push(b,c,c->ax);c->di=(uint16_t)(c->bp-0x31u);push(b,c,c->ss);push(b,c,c->di);
    c->ip=0x2b55u;if(rb(b,c->cs,c->ip)!=0x9au||rw(b,c->cs,0x2b56u)!=0x0626u)return PR_290B_FIRST_CODE;
    cs=c->cs;sp=(uint16_t)(c->sp+10u);push(b,c,cs);push(b,c,0x2b5au);c->cs=rw(b,cs,0x2b58u);c->ip=0x0626u;
    if(!ordinary(c->cs))return PR_290B_FIRST_PROFILE;
    {FIELDS(TO)}d->status=(int)pr_r0709_printer_0626_cpu(b,n,&t,l->printer_loopbacks,l->glyph_loopbacks,&d->printer);{FIELDS(FROM)}
    if(d->status!=PR_R0709_PRINTER_CPU_OK)return PR_290B_FIRST_CALLEE;
    if(c->cs!=cs||c->ip!=0x2b5au||c->sp!=sp)return PR_290B_FIRST_TRANSFER;
    if(c->bp>0xfffau){c->ip=0x2b5au;return PR_290B_FIRST_PROFILE;}
    c->sp=c->bp;c->bp=rw(b,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u);
    c->ip=rw(b,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u);c->cs=rw(b,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u);
    if(c->cs!=x->cs||c->ip!=x->ip||c->sp!=x->sp||c->bp!=x->bp||c->ss!=x->ss)return PR_290B_FIRST_TRANSFER;
    return PR_290B_FIRST_DONE;
}
void pr_u68_290b_print_init(pr_u68_290b_print*s){if(s){memset(s,0,sizeof *s);pr_u68_290b_clear_init(&s->clear);}}
static pr_u68_290b_print_status print_next(pr_u68_290b_print*s,pr_u68_290b_print_diagnostic*d,pr_u68_290b_clear_status st){
    d->clear_status=st;if(st==PR_U68_290B_CLEAR_YIELD){s->phase=PR_U68_290B_PRINT_WAITING;return PR_U68_290B_PRINT_YIELD;}
    s->phase=PR_U68_290B_PRINT_FINISHED;if(st!=PR_U68_290B_CLEAR_REACHED)return PR_U68_290B_PRINT_STOP;
    d->print_status=pr_290b_print_return(s->bus,PR_BUS_RAM_SIZE,s->cpu,&s->limits,&s->target,&d->print);
    return d->print_status==PR_290B_FIRST_DONE?PR_U68_290B_PRINT_RETURNED:PR_U68_290B_PRINT_STOP;
}
pr_u68_290b_print_status pr_u68_290b_print_start(pr_u68_290b_print*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,pr_u68_290b_print_diagnostic*d){
    pr_u68_290b_clear_status st;if(!s||!b||!c||!e||!cl||!pl||!d||s->phase!=PR_U68_290B_PRINT_EMPTY)return PR_U68_290B_PRINT_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;s->limits=*pl;s->target=(pr_290b_return_target){c->cs,0x0ba0u,c->sp,c->bp,c->ss};
    st=pr_u68_290b_clear_start(&s->clear,b,n,c,e,cl,&d->clear);return print_next(s,d,st);
}
pr_u68_290b_print_status pr_u68_290b_print_resume(pr_u68_290b_print*s,pr_u68_290b_print_diagnostic*d){pr_u68_290b_clear_status st;
    if(!s||!d||s->phase!=PR_U68_290B_PRINT_WAITING)return PR_U68_290B_PRINT_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_u68_290b_clear_resume(&s->clear,&d->clear);
    if(st==PR_U68_290B_CLEAR_ARGUMENT)return PR_U68_290B_PRINT_ARGUMENT;
    return print_next(s,d,st);
}

pr_290b_first_status pr_u68_post_display_exit(pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_290b_clear_limits*l,const pr_290b_return_target*x,pr_290b_clear_diagnostic*d){
    uint16_t cs,sp;uint8_t v;pr_r03f1_clear_cpu t;
    if(!b||!b->ram||!b->ega||!c||!l||!x||!d||n!=PR_BUS_RAM_SIZE||c->ip!=0x0ba0u||c->bp<0x0090u||c->bp>0xfffau||
       c->sp!=(uint16_t)(c->bp-0x003cu)||!ordinary(c->cs)||!ordinary(c->ds)||!ordinary(c->ss))return PR_290B_FIRST_ARGUMENT;
    memset(d,0,sizeof *d);d->status=-1;
    v=rb(b,c->ss,(uint16_t)(c->bp-2u));cmp8(c,v,0);if(!v){c->ip=0x09a8u;return PR_290B_FIRST_OUTSIDE;}
    c->ax=(uint16_t)((c->ax&0xff00u)|rb(b,c->ds,0x49fau));wb(b,c->ds,0x49f9u,(uint8_t)c->ax);
    v=rb(b,c->ds,0x84e4u);cmp8(c,v,0);
    if(!v){
        c->ax=(uint16_t)((c->ax&0xff00u)|1u);push(b,c,c->ax);c->ax=(uint16_t)((c->ax&0xff00u)|0x11u);push(b,c,c->ax);
        c->ax=(uint16_t)((c->ax&0xff00u)|0x26u);push(b,c,c->ax);c->ax=(uint16_t)((c->ax&0xff00u)|0x16u);push(b,c,c->ax);
        c->ip=0x0d35u;if(rb(b,c->cs,c->ip)!=0x9au||rw(b,c->cs,0x0d36u)!=0x0137u)return PR_290B_FIRST_CODE;
        cs=c->cs;sp=(uint16_t)(c->sp+8u);push(b,c,cs);push(b,c,0x0d3au);c->cs=rw(b,cs,0x0d38u);c->ip=0x0137u;
        if(!ordinary(c->cs))return PR_290B_FIRST_PROFILE;
        {FIELDS(TO)}d->status=(int)pr_r03f1_clear_0137_cpu(b,n,&t,l->clear_loopbacks,l->rect_loopbacks,&d->clear);{FIELDS(FROM)}
        if(d->status!=PR_R03F1_CLEAR_CPU_OK)return PR_290B_FIRST_CALLEE;
        if(c->cs!=cs||c->ip!=0x0d3au||c->sp!=sp)return PR_290B_FIRST_TRANSFER;
        wb(b,c->ds,0x84e4u,1);
    }
    c->ax=(uint16_t)((c->ax&0xff00u)|rb(b,c->ss,(uint16_t)(c->bp-3u)));wb(b,c->ss,(uint16_t)(c->bp-1u),(uint8_t)c->ax);
    c->ax=(uint16_t)((c->ax&0xff00u)|rb(b,c->ss,(uint16_t)(c->bp-1u)));
    if(c->bp>0xfffau){c->ip=0x0d48u;return PR_290B_FIRST_PROFILE;}
    c->sp=c->bp;c->bp=rw(b,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u);
    c->ip=rw(b,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u);c->cs=rw(b,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u);
    if(c->cs!=x->cs||c->ip!=x->ip||c->sp!=x->sp||c->bp!=x->bp||c->ss!=x->ss)return PR_290B_FIRST_TRANSFER;
    return PR_290B_FIRST_DONE;
}
void pr_u68_290b_exit_init(pr_u68_290b_exit*s){if(s){memset(s,0,sizeof *s);pr_u68_290b_print_init(&s->print);}}
static pr_u68_290b_exit_status exit_next(pr_u68_290b_exit*s,pr_u68_290b_exit_diagnostic*d,pr_u68_290b_print_status st){
    d->print_status=st;if(st==PR_U68_290B_PRINT_YIELD){s->phase=PR_U68_290B_EXIT_WAITING;return PR_U68_290B_EXIT_YIELD;}
    s->phase=PR_U68_290B_EXIT_FINISHED;if(st!=PR_U68_290B_PRINT_RETURNED)return PR_U68_290B_EXIT_STOP;
    d->exit_status=pr_u68_post_display_exit(s->bus,PR_BUS_RAM_SIZE,s->cpu,&s->limits,&s->target,&d->exit);
    if(d->exit_status==PR_290B_FIRST_OUTSIDE)return PR_U68_290B_EXIT_LOOP;
    return d->exit_status==PR_290B_FIRST_DONE?PR_U68_290B_EXIT_RETURNED:PR_U68_290B_EXIT_STOP;
}
pr_u68_290b_exit_status pr_u68_290b_exit_start(pr_u68_290b_exit*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,pr_u68_290b_exit_diagnostic*d){
    pr_u68_290b_print_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_U68_290B_EXIT_EMPTY)return PR_U68_290B_EXIT_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;s->limits=*cl;s->target=*x;
    st=pr_u68_290b_print_start(&s->print,b,n,c,e,cl,pl,&d->print);return exit_next(s,d,st);
}
pr_u68_290b_exit_status pr_u68_290b_exit_resume(pr_u68_290b_exit*s,pr_u68_290b_exit_diagnostic*d){pr_u68_290b_print_status st;
    if(!s||!d||s->phase!=PR_U68_290B_EXIT_WAITING)return PR_U68_290B_EXIT_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_u68_290b_print_resume(&s->print,&d->print);
    if(st==PR_U68_290B_PRINT_ARGUMENT)return PR_U68_290B_EXIT_ARGUMENT;
    return exit_next(s,d,st);
}

pr_290b_first_status pr_u1c_post_up(pr_bus*b,size_t n,pr_r0899_cpu*c){
    uint16_t off;
    if(!b||!b->ram||!b->ega||!c||n!=PR_BUS_RAM_SIZE||c->ip!=0x339au||c->bp<2u||c->sp!=(uint16_t)(c->bp-2u)||
       !ordinary(c->cs)||!ordinary(c->ds)||!ordinary(c->ss))return PR_290B_FIRST_ARGUMENT;
    wb(b,c->ss,(uint16_t)(c->bp-1u),(uint8_t)c->ax);
    c->ax=rw(b,c->ds,0x5d92u);c->dx=rw(b,c->ds,0x5d94u);ww(b,c->ds,0x4436u,c->ax);ww(b,c->ds,0x4438u,c->dx);
    c->ax=rb(b,c->ds,0x84dcu);c->flags=(uint16_t)((c->flags&~0x08d5u)|0x44u);
    c->di=rw(b,c->ds,0x49d2u);c->es=rw(b,c->ds,0x49d4u);off=(uint16_t)(c->di+0x01e4u);
    if(off==0xffffu){c->ip=0x33b4u;return PR_290B_FIRST_PROFILE;}
    ww(b,c->es,off,c->ax);c->di=rw(b,c->ds,0x49d6u);c->es=rw(b,c->ds,0x49d8u);c->ip=0x33bdu;return PR_290B_FIRST_DONE;
}
void pr_u1c_up_session_init(pr_u1c_up_session*s){if(s){memset(s,0,sizeof *s);pr_u68_290b_exit_init(&s->exit);}}
static pr_u1c_up_status u1c_next(pr_u1c_up_session*s,pr_u1c_up_diagnostic*d,pr_u68_290b_exit_status st){
    d->exit_status=st;if(st==PR_U68_290B_EXIT_YIELD){s->phase=PR_U1C_UP_WAITING;return PR_U1C_UP_YIELD;}
    s->phase=PR_U1C_UP_FINISHED;if(st==PR_U68_290B_EXIT_LOOP)return PR_U1C_UP_LOOP;
    if(st!=PR_U68_290B_EXIT_RETURNED)return PR_U1C_UP_STOP;
    d->caller_status=pr_u1c_post_up(s->bus,PR_BUS_RAM_SIZE,s->cpu);
    return d->caller_status==PR_290B_FIRST_DONE?PR_U1C_UP_REACHED:PR_U1C_UP_STOP;
}
pr_u1c_up_status pr_u1c_up_start(pr_u1c_up_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,pr_u1c_up_diagnostic*d){
    pr_u68_290b_exit_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_U1C_UP_EMPTY||x->ip!=0x339au||x->bp<2u||
        x->sp!=(uint16_t)(x->bp-2u)||!ordinary(x->cs)||!ordinary(x->ss))return PR_U1C_UP_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_u68_290b_exit_start(&s->exit,b,n,c,e,cl,pl,x,&d->exit);return u1c_next(s,d,st);
}
pr_u1c_up_status pr_u1c_up_resume(pr_u1c_up_session*s,pr_u1c_up_diagnostic*d){pr_u68_290b_exit_status st;
    if(!s||!d||s->phase!=PR_U1C_UP_WAITING)return PR_U1C_UP_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_u68_290b_exit_resume(&s->exit,&d->exit);
    if(st==PR_U68_290B_EXIT_ARGUMENT)return PR_U1C_UP_ARGUMENT;
    return u1c_next(s,d,st);
}

static void u1c_cmp_word_one(pr_r0899_cpu*c,uint16_t a){unsigned r=(a-1u)&0xffffu,p=r&255u;p^=p>>4;p^=p>>2;p^=p>>1;
    c->flags=(uint16_t)((c->flags&~0x08d5u)|(a==0?1u:0u)|((p&1u)?0u:4u)|(((a^1u^r)&0x10u)?0x10u:0u)|
        (r==0?0x40u:0u)|(r&0x8000u?0x80u:0u)|(((a^1u)&(a^r)&0x8000u)?0x800u:0u));}
static pr_290b_first_status u1c_upcase(pr_bus*b,size_t n,pr_r0899_cpu*c,pr_290b_first_diagnostic*d,uint16_t site,int stage){
    uint16_t cs,sp;pr_system_leaf_cpu t;pr_system_leaf_status st;
    c->ax=(uint16_t)((c->ax&0xff00u)|rb(b,c->ss,(uint16_t)(c->bp-1u)));push(b,c,c->ax);c->ip=site;d->stage=stage;
    if(rb(b,c->cs,site)!=0x9au||rw(b,c->cs,(uint16_t)(site+1u))!=0x182au)return PR_290B_FIRST_CODE;
    cs=c->cs;sp=(uint16_t)(c->sp+2u);push(b,c,cs);push(b,c,(uint16_t)(site+5u));c->cs=rw(b,cs,(uint16_t)(site+3u));c->ip=0x182au;
    if(!ordinary(c->cs))return PR_290B_FIRST_PROFILE;
    {FIELDS(TO)}st=pr_system_upcase_182a(b->ram,n,&t);{FIELDS(FROM)}d->status=(int)st;
    if(st!=PR_SYSTEM_LEAF_RETURNED)return PR_290B_FIRST_CALLEE;
    if(c->cs!=cs||c->ip!=(uint16_t)(site+5u)||c->sp!=sp)return PR_290B_FIRST_TRANSFER;
    return PR_290B_FIRST_DONE;
}
pr_290b_first_status pr_u1c_dispatch(pr_bus*b,size_t n,pr_r0899_cpu*c,pr_290b_first_diagnostic*d){
    uint16_t off,v;pr_290b_first_status st;
    if(!b||!b->ram||!b->ega||!c||!d||n!=PR_BUS_RAM_SIZE||c->ip!=0x33bdu||c->bp<8u||c->sp!=(uint16_t)(c->bp-2u)||
       !ordinary(c->cs)||!ordinary(c->ds)||!ordinary(c->ss))return PR_290B_FIRST_ARGUMENT;
    memset(d,0,sizeof *d);d->status=-1;d->formatter_callee=PR_RES_1037_NO_CALLEE;d->formatter_status=-1;
    off=(uint16_t)(c->di+0x0594u);if(off==0xffffu)return PR_290B_FIRST_PROFILE;
    v=rw(b,c->es,off);u1c_cmp_word_one(c,v);
    if(v<=1u){st=u1c_upcase(b,n,c,d,0x33c9u,1);if(st)return st;cmp8(c,(uint8_t)c->ax,0x45u);
        if((uint8_t)c->ax!=0x45u){c->ip=0x344cu;return PR_290B_FIRST_DONE;}}
    v=rb(b,c->ds,0x49ffu);cmp8(c,(uint8_t)v,0);if(v){c->ip=0x344cu;return PR_290B_FIRST_DONE;}
    st=u1c_upcase(b,n,c,d,0x33ddu,2);if(st)return st;cmp8(c,(uint8_t)c->ax,0x45u);
    c->ip=(uint8_t)c->ax==0x45u?0x33e6u:0x33ecu;return PR_290B_FIRST_DONE;
}
void pr_u1c_dispatch_session_init(pr_u1c_dispatch_session*s){if(s){memset(s,0,sizeof *s);pr_u1c_up_session_init(&s->up);}}
static pr_u1c_dispatch_status dispatch_next(pr_u1c_dispatch_session*s,pr_u1c_dispatch_diagnostic*d,pr_u1c_up_status st){
    d->up_status=st;if(st==PR_U1C_UP_YIELD){s->phase=PR_U1C_DISPATCH_WAITING;return PR_U1C_DISPATCH_YIELD;}
    s->phase=PR_U1C_DISPATCH_FINISHED;if(st==PR_U1C_UP_LOOP)return PR_U1C_DISPATCH_LOOP;
    if(st!=PR_U1C_UP_REACHED)return PR_U1C_DISPATCH_STOP;
    d->dispatch_status=pr_u1c_dispatch(s->bus,PR_BUS_RAM_SIZE,s->cpu,&d->dispatch);
    return d->dispatch_status==PR_290B_FIRST_DONE?PR_U1C_DISPATCH_REACHED:PR_U1C_DISPATCH_STOP;
}
pr_u1c_dispatch_status pr_u1c_dispatch_start(pr_u1c_dispatch_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,pr_u1c_dispatch_diagnostic*d){
    pr_u1c_up_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_U1C_DISPATCH_EMPTY||x->bp<8u)return PR_U1C_DISPATCH_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_u1c_up_start(&s->up,b,n,c,e,cl,pl,x,&d->up);return dispatch_next(s,d,st);
}
pr_u1c_dispatch_status pr_u1c_dispatch_resume(pr_u1c_dispatch_session*s,pr_u1c_dispatch_diagnostic*d){pr_u1c_up_status st;
    if(!s||!d||s->phase!=PR_U1C_DISPATCH_WAITING)return PR_U1C_DISPATCH_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_u1c_up_resume(&s->up,&d->up);
    if(st==PR_U1C_UP_ARGUMENT)return PR_U1C_DISPATCH_ARGUMENT;
    return dispatch_next(s,d,st);
}

pr_290b_first_status pr_u1c_script_entry(pr_bus*b,size_t n,pr_r0899_cpu*c){
    uint8_t v;uint16_t rel;
    if(!b||!b->ram||!b->ega||!c||n!=PR_BUS_RAM_SIZE||c->ip!=0x344cu||c->bp<8u||c->sp!=(uint16_t)(c->bp-2u)||
       !ordinary(c->cs)||!ordinary(c->ds)||!ordinary(c->ss))return PR_290B_FIRST_ARGUMENT;
    v=rb(b,c->ds,0x49ffu);cmp8(c,v,0);
    if(!v){push(b,c,rw(b,c->ds,0x49e3u));push(b,c,c->cs);c->ip=0x3458u;rel=rw(b,c->cs,0x3459u);
        if(rb(b,c->cs,0x3458u)!=0xe8u||(uint16_t)(0x345bu+rel)!=0x3171u)return PR_290B_FIRST_CODE;
        push(b,c,0x345bu);c->ip=0x3171u;return PR_290B_FIRST_DONE;}
    v=rb(b,c->ds,0x442fu);cmp8(c,v,0);if(v){c->ip=0x3462u;return PR_290B_FIRST_DONE;}
    v=rb(b,c->ds,0x49ffu);cmp8(c,v,0);c->ip=v?0x34f5u:0x3473u;return PR_290B_FIRST_DONE;
}
void pr_u1c_script_session_init(pr_u1c_script_session*s){if(s){memset(s,0,sizeof *s);pr_u1c_dispatch_session_init(&s->dispatch);}}
static pr_u1c_script_status script_next(pr_u1c_script_session*s,pr_u1c_script_diagnostic*d,pr_u1c_dispatch_status st){
    d->dispatch_status=st;if(st==PR_U1C_DISPATCH_YIELD){s->phase=PR_U1C_SCRIPT_WAITING;return PR_U1C_SCRIPT_YIELD;}
    s->phase=PR_U1C_SCRIPT_FINISHED;if(st==PR_U1C_DISPATCH_LOOP)return PR_U1C_SCRIPT_LOOP;
    if(st!=PR_U1C_DISPATCH_REACHED)return PR_U1C_SCRIPT_STOP;
    if(s->cpu->ip!=0x344cu)return PR_U1C_SCRIPT_OTHER;
    d->entry_status=pr_u1c_script_entry(s->bus,PR_BUS_RAM_SIZE,s->cpu);
    return d->entry_status==PR_290B_FIRST_DONE?PR_U1C_SCRIPT_REACHED:PR_U1C_SCRIPT_STOP;
}
pr_u1c_script_status pr_u1c_script_start(pr_u1c_script_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,pr_u1c_script_diagnostic*d){
    pr_u1c_dispatch_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_U1C_SCRIPT_EMPTY)return PR_U1C_SCRIPT_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_u1c_dispatch_start(&s->dispatch,b,n,c,e,cl,pl,x,&d->dispatch);return script_next(s,d,st);
}
pr_u1c_script_status pr_u1c_script_resume(pr_u1c_script_session*s,pr_u1c_script_diagnostic*d){pr_u1c_dispatch_status st;
    if(!s||!d||s->phase!=PR_U1C_SCRIPT_WAITING)return PR_U1C_SCRIPT_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_u1c_dispatch_resume(&s->dispatch,&d->dispatch);
    if(st==PR_U1C_DISPATCH_ARGUMENT)return PR_U1C_SCRIPT_ARGUMENT;
    return script_next(s,d,st);
}

pr_290b_first_status pr_u1c_runner_prefix(pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_290b_return_target*x){
    uint8_t v;uint16_t rel;
    if(!b||!b->ram||!b->ega||!c||!x||n!=PR_BUS_RAM_SIZE||c->ip!=0x3171u||c->sp<6u||c->sp>0xfffau||
       !ordinary(c->cs)||!ordinary(c->ds)||!ordinary(c->ss))return PR_290B_FIRST_ARGUMENT;
    push(b,c,c->bp);c->bp=c->sp;c->ax=rw(b,c->ss,(uint16_t)(c->bp+6u));ww(b,c->ds,0x49edu,c->ax);wb(b,c->ds,0x442eu,0);
    v=rb(b,c->ds,0x442eu);cmp8(c,v,0);
    if(!v){v=rb(b,c->ds,0x49ffu);cmp8(c,v,0);}
    if(v){
        wb(b,c->ds,0x442eu,0);c->bp=rw(b,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u);
        c->ip=rw(b,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u);c->cs=rw(b,c->ss,c->sp);c->sp=(uint16_t)(c->sp+4u);
        if(c->cs!=x->cs||c->ip!=x->ip||c->sp!=x->sp||c->bp!=x->bp||c->ss!=x->ss)return PR_290B_FIRST_TRANSFER;
        return PR_290B_FIRST_DONE;
    }
    c->ax=(uint16_t)((c->ax&0xff00u)|rb(b,c->ds,0x6f7fu));wb(b,c->ds,0x6f7eu,(uint8_t)c->ax);
    c->ax=rw(b,c->ds,0x49edu);c->di=rw(b,c->ds,0x49deu);c->es=rw(b,c->ds,0x49e0u);c->di=second_add(c,c->di,c->ax);
    c->ax=(uint16_t)((c->ax&0xff00u)|rb(b,c->es,(uint16_t)(c->di+0x6700u)));wb(b,c->ds,0x6f7fu,(uint8_t)c->ax);
    push(b,c,c->cs);c->ip=0x31a5u;rel=rw(b,c->cs,0x31a6u);
    if(rb(b,c->cs,0x31a5u)!=0xe8u||(uint16_t)(0x31a8u+rel)!=0x2f2du)return PR_290B_FIRST_CODE;
    push(b,c,0x31a8u);c->ip=0x2f2du;return PR_290B_FIRST_DONE;
}
void pr_u1c_runner_session_init(pr_u1c_runner_session*s){if(s){memset(s,0,sizeof *s);pr_u1c_script_session_init(&s->script);}}
static pr_u1c_runner_status runner_next(pr_u1c_runner_session*s,pr_u1c_runner_diagnostic*d,pr_u1c_script_status st){
    d->script_status=st;if(st==PR_U1C_SCRIPT_YIELD){s->phase=PR_U1C_RUNNER_WAITING;return PR_U1C_RUNNER_YIELD;}
    s->phase=PR_U1C_RUNNER_FINISHED;if(st==PR_U1C_SCRIPT_LOOP)return PR_U1C_RUNNER_LOOP;
    if(st==PR_U1C_SCRIPT_OTHER)return PR_U1C_RUNNER_OTHER;
    if(st!=PR_U1C_SCRIPT_REACHED)return PR_U1C_RUNNER_STOP;
    if(s->cpu->ip!=0x3171u)return PR_U1C_RUNNER_OTHER;
    d->runner_status=pr_u1c_runner_prefix(s->bus,PR_BUS_RAM_SIZE,s->cpu,&s->target);
    if(d->runner_status!=PR_290B_FIRST_DONE)return PR_U1C_RUNNER_STOP;
    return s->cpu->ip==0x2f2du?PR_U1C_RUNNER_DISPATCH:PR_U1C_RUNNER_RETURNED;
}
pr_u1c_runner_status pr_u1c_runner_start(pr_u1c_runner_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,pr_u1c_runner_diagnostic*d){
    pr_u1c_script_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_U1C_RUNNER_EMPTY||x->bp<14u)return PR_U1C_RUNNER_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;s->target=*x;s->target.ip=0x345bu;
    st=pr_u1c_script_start(&s->script,b,n,c,e,cl,pl,x,&d->script);return runner_next(s,d,st);
}
pr_u1c_runner_status pr_u1c_runner_resume(pr_u1c_runner_session*s,pr_u1c_runner_diagnostic*d){pr_u1c_script_status st;
    if(!s||!d||s->phase!=PR_U1C_RUNNER_WAITING)return PR_U1C_RUNNER_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_u1c_script_resume(&s->script,&d->script);
    if(st==PR_U1C_SCRIPT_ARGUMENT)return PR_U1C_RUNNER_ARGUMENT;
    return runner_next(s,d,st);
}

pr_290b_first_status pr_u1c_ecl_dispatch_prefix(pr_bus*b,size_t n,pr_r0899_cpu*c){
    static const uint16_t sites[10]={0x2f38u,0x2f43u,0x2f4eu,0x2f59u,0x2f70u,0x2f70u,0x2f70u,0x2f70u,0x2f7bu,0x2f86u};
    static const uint16_t targets[10]={0x0001u,0x00f4u,0x0111u,0x0126u,0x01a3u,0x01a3u,0x01a3u,0x01a3u,0x0234u,0x0292u};
    uint8_t op;uint16_t site,rel;
    if(!b||!b->ram||!b->ega||!c||n!=PR_BUS_RAM_SIZE||c->ip!=0x2f2du||c->sp<6u||c->sp>0xfffcu||
       !ordinary(c->cs)||!ordinary(c->ds)||!ordinary(c->ss))return PR_290B_FIRST_ARGUMENT;
    push(b,c,c->bp);c->bp=c->sp;op=rb(b,c->ds,0x6f7fu);c->ax=(uint16_t)((c->ax&0xff00u)|op);
    if(op>9u){cmp8(c,op,9u);c->ip=0x2f8cu;return PR_290B_FIRST_OUTSIDE;}
    cmp8(c,op,op);site=sites[op];push(b,c,c->cs);c->ip=site;rel=rw(b,c->cs,(uint16_t)(site+1u));
    if(rb(b,c->cs,site)!=0xe8u||(uint16_t)(site+3u+rel)!=targets[op])return PR_290B_FIRST_CODE;
    push(b,c,(uint16_t)(site+3u));c->ip=targets[op];return PR_290B_FIRST_DONE;
}
void pr_u1c_ecl_session_init(pr_u1c_ecl_session*s){if(s){memset(s,0,sizeof *s);pr_u1c_runner_session_init(&s->runner);}}
static pr_u1c_ecl_status ecl_next(pr_u1c_ecl_session*s,pr_u1c_ecl_diagnostic*d,pr_u1c_runner_status st){
    d->runner_status=st;if(st==PR_U1C_RUNNER_YIELD){s->phase=PR_U1C_ECL_WAITING;return PR_U1C_ECL_YIELD;}
    s->phase=PR_U1C_ECL_FINISHED;if(st==PR_U1C_RUNNER_LOOP)return PR_U1C_ECL_LOOP;
    if(st==PR_U1C_RUNNER_OTHER||st==PR_U1C_RUNNER_RETURNED)return PR_U1C_ECL_OTHER;
    if(st!=PR_U1C_RUNNER_DISPATCH)return PR_U1C_ECL_STOP;
    d->prefix_status=pr_u1c_ecl_dispatch_prefix(s->bus,PR_BUS_RAM_SIZE,s->cpu);
    if(d->prefix_status==PR_290B_FIRST_OUTSIDE)return PR_U1C_ECL_OTHER;
    return d->prefix_status==PR_290B_FIRST_DONE?PR_U1C_ECL_HANDLER:PR_U1C_ECL_STOP;
}
pr_u1c_ecl_status pr_u1c_ecl_start(pr_u1c_ecl_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,pr_u1c_ecl_diagnostic*d){
    pr_u1c_runner_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_U1C_ECL_EMPTY||x->bp<20u)return PR_U1C_ECL_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_u1c_runner_start(&s->runner,b,n,c,e,cl,pl,x,&d->runner);return ecl_next(s,d,st);
}
pr_u1c_ecl_status pr_u1c_ecl_resume(pr_u1c_ecl_session*s,pr_u1c_ecl_diagnostic*d){pr_u1c_runner_status st;
    if(!s||!d||s->phase!=PR_U1C_ECL_WAITING)return PR_U1C_ECL_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_u1c_runner_resume(&s->runner,&d->runner);
    if(st==PR_U1C_RUNNER_ARGUMENT)return PR_U1C_ECL_ARGUMENT;
    return ecl_next(s,d,st);
}

static uint16_t save_sub4(pr_r0899_cpu*c,uint16_t a){unsigned r=(a-4u)&0xffffu,p=r&255u;p^=p>>4;p^=p>>2;p^=p>>1;
    c->flags=(uint16_t)((c->flags&~0x08d5u)|(a<4u?1u:0u)|((p&1u)?0u:4u)|(((a^4u^r)&0x10u)?0x10u:0u)|
        (r==0?0x40u:0u)|(r&0x8000u?0x80u:0u)|(((a^4u)&(a^r)&0x8000u)?0x800u:0u));return (uint16_t)r;}
pr_290b_first_status pr_u1c_save_loader_entry(pr_bus*b,size_t n,pr_r0899_cpu*c,pr_290b_first_diagnostic*d){
    uint16_t cs,target;
    if(!b||!b->ram||!b->ega||!c||!d||n!=PR_BUS_RAM_SIZE||c->ip!=0x0292u||c->sp<12u||c->sp>0xfffcu||
       !ordinary(c->cs)||!ordinary(c->ds)||!ordinary(c->ss))return PR_290B_FIRST_ARGUMENT;
    memset(d,0,sizeof *d);d->status=-1;d->formatter_callee=PR_RES_1037_NO_CALLEE;d->formatter_status=-1;
    push(b,c,c->bp);c->bp=c->sp;c->sp=save_sub4(c,c->sp);c->ax=(uint16_t)((c->ax&0xff00u)|2u);push(b,c,c->ax);
    c->ip=0x029bu;d->stage=1;if(rb(b,c->cs,c->ip)!=0x9au||rw(b,c->cs,0x029cu)!=0x002au)return PR_290B_FIRST_CODE;
    cs=c->cs;push(b,c,cs);push(b,c,0x02a0u);c->cs=rw(b,cs,0x029eu);c->ip=0x002au;d->stage=2;
    if(!ordinary(c->cs))return PR_290B_FIRST_PROFILE;
    if(rb(b,c->cs,0x002au)!=0xeau||rw(b,c->cs,0x002bu)!=0x0000u)return PR_290B_FIRST_CODE;
    target=rw(b,c->cs,0x002du);c->cs=target;c->ip=0x0000u;d->stage=3;
    if(!ordinary(target))return PR_290B_FIRST_PROFILE;
    return PR_290B_FIRST_DONE;
}
void pr_u1c_save_session_init(pr_u1c_save_session*s){if(s){memset(s,0,sizeof *s);pr_u1c_ecl_session_init(&s->ecl);}}
static pr_u1c_save_status save_next(pr_u1c_save_session*s,pr_u1c_save_diagnostic*d,pr_u1c_ecl_status st){
    d->ecl_status=st;if(st==PR_U1C_ECL_YIELD){s->phase=PR_U1C_SAVE_WAITING;return PR_U1C_SAVE_YIELD;}
    s->phase=PR_U1C_SAVE_FINISHED;if(st==PR_U1C_ECL_LOOP)return PR_U1C_SAVE_LOOP;
    if(st==PR_U1C_ECL_OTHER)return PR_U1C_SAVE_OTHER;
    if(st!=PR_U1C_ECL_HANDLER)return PR_U1C_SAVE_STOP;
    if(s->cpu->ip!=0x0292u)return PR_U1C_SAVE_OTHER;
    d->entry_status=pr_u1c_save_loader_entry(s->bus,PR_BUS_RAM_SIZE,s->cpu,&d->entry);
    return d->entry_status==PR_290B_FIRST_DONE?PR_U1C_SAVE_LOADER:PR_U1C_SAVE_STOP;
}
pr_u1c_save_status pr_u1c_save_start(pr_u1c_save_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,pr_u1c_save_diagnostic*d){
    pr_u1c_ecl_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_U1C_SAVE_EMPTY||x->bp<32u)return PR_U1C_SAVE_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_u1c_ecl_start(&s->ecl,b,n,c,e,cl,pl,x,&d->ecl);return save_next(s,d,st);
}
pr_u1c_save_status pr_u1c_save_resume(pr_u1c_save_session*s,pr_u1c_save_diagnostic*d){pr_u1c_ecl_status st;
    if(!s||!d||s->phase!=PR_U1C_SAVE_WAITING)return PR_U1C_SAVE_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_u1c_ecl_resume(&s->ecl,&d->ecl);
    if(st==PR_U1C_ECL_ARGUMENT)return PR_U1C_SAVE_ARGUMENT;
    return save_next(s,d,st);
}

static uint16_t u25_inc_word(pr_r0899_cpu*c,uint16_t v){uint16_t cf=(uint16_t)(c->flags&1u),r=second_add(c,v,1u);c->flags=(uint16_t)((c->flags&~1u)|cf);return r;}
static void u25_xor_ah(pr_r0899_cpu*c){c->ax&=0x00ffu;c->flags=(uint16_t)((c->flags&~0x08d5u)|0x44u);}
static void u25_operand_iteration(pr_bus*b,pr_r0899_cpu*c){uint16_t off;
    c->ax=rw(b,c->ds,0x49edu);c->ax=u25_inc_word(c,c->ax);c->di=rw(b,c->ds,0x49deu);c->es=rw(b,c->ds,0x49e0u);
    c->di=second_add(c,c->di,c->ax);off=(uint16_t)(c->di+0x6700u);c->dx=(uint16_t)((c->dx&0xff00u)|rb(b,c->es,off));
    c->ax=(uint16_t)((c->ax&0xff00u)|rb(b,c->ss,(uint16_t)(c->bp-1u)));u25_xor_ah(c);c->di=c->ax;wb(b,c->ds,(uint16_t)(c->di+0x7005u),(uint8_t)c->dx);
    c->ax=rw(b,c->ds,0x49edu);c->ax=u25_inc_word(c,c->ax);c->ax=u25_inc_word(c,c->ax);c->di=rw(b,c->ds,0x49deu);c->es=rw(b,c->ds,0x49e0u);
    c->di=second_add(c,c->di,c->ax);off=(uint16_t)(c->di+0x6700u);c->dx=(uint16_t)((c->dx&0xff00u)|rb(b,c->es,off));
    c->ax=(uint16_t)((c->ax&0xff00u)|rb(b,c->ss,(uint16_t)(c->bp-1u)));u25_xor_ah(c);c->di=c->ax;wb(b,c->ds,(uint16_t)(c->di+0x7085u),(uint8_t)c->dx);
    c->ax=rw(b,c->ds,0x49edu);c->ax=u25_inc_word(c,c->ax);c->ax=u25_inc_word(c,c->ax);ww(b,c->ds,0x49edu,c->ax);
    c->ip=0x005fu;
}
pr_290b_first_status pr_u25_operand_prefix(pr_bus*b,size_t n,pr_r0899_cpu*c){
    uint8_t count;
    if(!b||!b->ram||!b->ega||!c||n!=PR_BUS_RAM_SIZE||c->ip!=0x0000u||c->sp<10u||c->sp>0xfffau||
       !ordinary(c->cs)||!ordinary(c->ds)||!ordinary(c->ss))return PR_290B_FIRST_ARGUMENT;
    push(b,c,c->bp);c->bp=c->sp;c->sp=(uint16_t)(c->sp-8u);wb(b,c->ss,(uint16_t)(c->bp-2u),0);
    c->ax=(uint16_t)((c->ax&0xff00u)|rb(b,c->ss,(uint16_t)(c->bp+6u)));wb(b,c->ss,(uint16_t)(c->bp-7u),(uint8_t)c->ax);
    c->ax=(uint16_t)((c->ax&0xff00u)|1u);count=rb(b,c->ss,(uint16_t)(c->bp-7u));cmp8(c,1u,count);
    if(count==0){c->ip=0x012cu;return PR_290B_FIRST_OUTSIDE;}
    wb(b,c->ss,(uint16_t)(c->bp-1u),(uint8_t)c->ax);
    u25_operand_iteration(b,c);return PR_290B_FIRST_DONE;
}
void pr_u25_operand_session_init(pr_u25_operand_session*s){if(s){memset(s,0,sizeof *s);pr_u1c_save_session_init(&s->save);}}
static pr_u25_operand_status operand_next(pr_u25_operand_session*s,pr_u25_operand_diagnostic*d,pr_u1c_save_status st){
    d->save_status=st;if(st==PR_U1C_SAVE_YIELD){s->phase=PR_U25_OPERAND_WAITING;return PR_U25_OPERAND_YIELD;}
    s->phase=PR_U25_OPERAND_FINISHED;if(st==PR_U1C_SAVE_LOOP)return PR_U25_OPERAND_LOOP;
    if(st==PR_U1C_SAVE_OTHER)return PR_U25_OPERAND_OTHER;
    if(st!=PR_U1C_SAVE_LOADER)return PR_U25_OPERAND_STOP;
    d->prefix_status=pr_u25_operand_prefix(s->bus,PR_BUS_RAM_SIZE,s->cpu);
    if(d->prefix_status==PR_290B_FIRST_OUTSIDE)return PR_U25_OPERAND_ZERO;
    return d->prefix_status==PR_290B_FIRST_DONE?PR_U25_OPERAND_REACHED:PR_U25_OPERAND_STOP;
}
pr_u25_operand_status pr_u25_operand_start(pr_u25_operand_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,pr_u25_operand_diagnostic*d){
    pr_u1c_save_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_U25_OPERAND_EMPTY||x->bp<42u)return PR_U25_OPERAND_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_u1c_save_start(&s->save,b,n,c,e,cl,pl,x,&d->save);return operand_next(s,d,st);
}
pr_u25_operand_status pr_u25_operand_resume(pr_u25_operand_session*s,pr_u25_operand_diagnostic*d){pr_u1c_save_status st;
    if(!s||!d||s->phase!=PR_U25_OPERAND_WAITING)return PR_U25_OPERAND_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_u1c_save_resume(&s->save,&d->save);
    if(st==PR_U1C_SAVE_ARGUMENT)return PR_U25_OPERAND_ARGUMENT;
    return operand_next(s,d,st);
}

pr_290b_first_status pr_u25_operand_tag(pr_bus*b,size_t n,pr_r0899_cpu*c){
    uint8_t tag;uint16_t pc,off;
    if(!b||!b->ram||!b->ega||!c||n!=PR_BUS_RAM_SIZE||c->ip!=0x005fu||c->bp<8u||c->bp>0xfff8u||c->sp!=(uint16_t)(c->bp-8u)||
       !ordinary(c->cs)||!ordinary(c->ds)||!ordinary(c->ss))return PR_290B_FIRST_ARGUMENT;
    c->ax=(uint16_t)((c->ax&0xff00u)|rb(b,c->ss,(uint16_t)(c->bp-1u)));u25_xor_ah(c);c->di=c->ax;
    tag=rb(b,c->ds,(uint16_t)(c->di+0x7005u));c->ax=(uint16_t)((c->ax&0xff00u)|tag);
    if(tag>=1u&&tag<=3u){
        cmp8(c,tag,tag);pc=rw(b,c->ds,0x49edu);pc=u25_inc_word(c,pc);ww(b,c->ds,0x49edu,pc);
        c->ax=rw(b,c->ds,0x49edu);c->di=rw(b,c->ds,0x49deu);c->es=rw(b,c->ds,0x49e0u);c->di=second_add(c,c->di,c->ax);
        off=(uint16_t)(c->di+0x6700u);c->dx=(uint16_t)((c->dx&0xff00u)|rb(b,c->es,off));
        c->ax=(uint16_t)((c->ax&0xff00u)|rb(b,c->ss,(uint16_t)(c->bp-1u)));u25_xor_ah(c);c->di=c->ax;
        wb(b,c->ds,(uint16_t)(c->di+0x7045u),(uint8_t)c->dx);c->ip=0x0121u;return PR_290B_FIRST_DONE;
    }
    cmp8(c,tag,0x80u);if(tag==0x80u){c->ip=0x009au;return PR_290B_FIRST_OUTSIDE;}
    cmp8(c,tag,0x81u);if(tag==0x81u){c->ip=0x00d7u;return PR_290B_FIRST_OUTSIDE;}
    c->ip=0x0121u;return PR_290B_FIRST_DONE;
}
void pr_u25_tag_session_init(pr_u25_tag_session*s){if(s){memset(s,0,sizeof *s);pr_u25_operand_session_init(&s->operand);}}
static pr_u25_tag_status tag_next(pr_u25_tag_session*s,pr_u25_tag_diagnostic*d,pr_u25_operand_status st){
    d->operand_status=st;if(st==PR_U25_OPERAND_YIELD){s->phase=PR_U25_TAG_WAITING;return PR_U25_TAG_YIELD;}
    s->phase=PR_U25_TAG_FINISHED;if(st==PR_U25_OPERAND_LOOP)return PR_U25_TAG_LOOP;
    if(st==PR_U25_OPERAND_OTHER||st==PR_U25_OPERAND_ZERO)return PR_U25_TAG_OTHER;
    if(st!=PR_U25_OPERAND_REACHED)return PR_U25_TAG_STOP;
    d->tag_status=pr_u25_operand_tag(s->bus,PR_BUS_RAM_SIZE,s->cpu);
    if(d->tag_status==PR_290B_FIRST_OUTSIDE)return PR_U25_TAG_STRING;
    return d->tag_status==PR_290B_FIRST_DONE?PR_U25_TAG_REACHED:PR_U25_TAG_STOP;
}
pr_u25_tag_status pr_u25_tag_start(pr_u25_tag_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,pr_u25_tag_diagnostic*d){
    pr_u25_operand_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_U25_TAG_EMPTY)return PR_U25_TAG_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_u25_operand_start(&s->operand,b,n,c,e,cl,pl,x,&d->operand);return tag_next(s,d,st);
}
pr_u25_tag_status pr_u25_tag_resume(pr_u25_tag_session*s,pr_u25_tag_diagnostic*d){pr_u25_operand_status st;
    if(!s||!d||s->phase!=PR_U25_TAG_WAITING)return PR_U25_TAG_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_u25_operand_resume(&s->operand,&d->operand);
    if(st==PR_U25_OPERAND_ARGUMENT)return PR_U25_TAG_ARGUMENT;
    return tag_next(s,d,st);
}

static uint8_t u25_inc_byte(pr_r0899_cpu*c,uint8_t a){unsigned r=(a+1u)&255u,p=r,cf=c->flags&1u;p^=p>>4;p^=p>>2;p^=p>>1;
    c->flags=(uint16_t)((c->flags&~0x08d5u)|cf|((p&1u)?0u:4u)|(((a^1u^r)&0x10u)?0x10u:0u)|
        (r==0?0x40u:0u)|(r&0x80u?0x80u:0u)|(a==0x7fu?0x800u:0u));return (uint8_t)r;}
pr_u25_loop_step_status pr_u25_numeric_loop(pr_bus*b,size_t n,pr_r0899_cpu*c,size_t max_iterations,pr_u25_loop_step_diagnostic*d){
    uint8_t index,count;pr_290b_first_status st;
    if(!b||!b->ram||!b->ega||!c||!d||n!=PR_BUS_RAM_SIZE||c->ip!=0x0121u||c->bp<8u||c->bp>0xfff8u||c->sp!=(uint16_t)(c->bp-8u)||
       !ordinary(c->cs)||!ordinary(c->ds)||!ordinary(c->ss))return PR_U25_LOOP_STEP_ARGUMENT;
    memset(d,0,sizeof *d);d->tag_status=PR_290B_FIRST_DONE;
    for(;;){
        index=rb(b,c->ss,(uint16_t)(c->bp-1u));c->ax=(uint16_t)((c->ax&0xff00u)|index);
        count=rb(b,c->ss,(uint16_t)(c->bp-7u));cmp8(c,index,count);
        if(index==count){c->ip=0x012cu;return PR_U25_LOOP_STEP_DONE;}
        c->ip=0x001fu;if(d->iterations==max_iterations)return PR_U25_LOOP_STEP_LIMIT;
        index=rb(b,c->ss,(uint16_t)(c->bp-1u));index=u25_inc_byte(c,index);wb(b,c->ss,(uint16_t)(c->bp-1u),index);++d->iterations;
        u25_operand_iteration(b,c);st=pr_u25_operand_tag(b,n,c);d->tag_status=st;
        if(st==PR_290B_FIRST_OUTSIDE)return PR_U25_LOOP_STEP_STRING;
        if(st!=PR_290B_FIRST_DONE)return PR_U25_LOOP_STEP_STOP;
    }
}
void pr_u25_loop_session_init(pr_u25_loop_session*s){if(s){memset(s,0,sizeof *s);pr_u25_tag_session_init(&s->tag);}}
static pr_u25_loop_status loop_next(pr_u25_loop_session*s,pr_u25_loop_diagnostic*d,pr_u25_tag_status st){
    d->tag_status=st;if(st==PR_U25_TAG_YIELD){s->phase=PR_U25_LOOP_WAITING;return PR_U25_LOOP_YIELD;}
    s->phase=PR_U25_LOOP_FINISHED;if(st==PR_U25_TAG_LOOP)return PR_U25_LOOP_PROMPT;
    if(st==PR_U25_TAG_OTHER)return PR_U25_LOOP_OTHER;
    if(st==PR_U25_TAG_STRING)return PR_U25_LOOP_STRING;
    if(st!=PR_U25_TAG_REACHED)return PR_U25_LOOP_STOP;
    d->loop_status=pr_u25_numeric_loop(s->bus,PR_BUS_RAM_SIZE,s->cpu,s->max_iterations,&d->loop);
    if(d->loop_status==PR_U25_LOOP_STEP_STRING)return PR_U25_LOOP_STRING;
    if(d->loop_status==PR_U25_LOOP_STEP_LIMIT)return PR_U25_LOOP_LIMIT;
    return d->loop_status==PR_U25_LOOP_STEP_DONE?PR_U25_LOOP_REACHED:PR_U25_LOOP_STOP;
}
pr_u25_loop_status pr_u25_loop_start(pr_u25_loop_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_u25_loop_diagnostic*d){
    pr_u25_tag_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_U25_LOOP_EMPTY)return PR_U25_LOOP_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;s->max_iterations=max_iterations;
    st=pr_u25_tag_start(&s->tag,b,n,c,e,cl,pl,x,&d->tag);return loop_next(s,d,st);
}
pr_u25_loop_status pr_u25_loop_resume(pr_u25_loop_session*s,pr_u25_loop_diagnostic*d){pr_u25_tag_status st;
    if(!s||!d||s->phase!=PR_U25_LOOP_WAITING)return PR_U25_LOOP_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_u25_tag_resume(&s->tag,&d->tag);
    if(st==PR_U25_TAG_ARGUMENT)return PR_U25_LOOP_ARGUMENT;
    return loop_next(s,d,st);
}

pr_290b_first_status pr_u25_operand_return(pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_290b_return_target*x){
    uint16_t pc;
    if(!b||!b->ram||!b->ega||!c||!x||n!=PR_BUS_RAM_SIZE||c->ip!=0x012cu||c->bp<8u||c->bp>0xfff8u||c->sp!=(uint16_t)(c->bp-8u)||
       !ordinary(c->cs)||!ordinary(c->ds)||!ordinary(c->ss))return PR_290B_FIRST_ARGUMENT;
    pc=rw(b,c->ds,0x49edu);pc=u25_inc_word(c,pc);ww(b,c->ds,0x49edu,pc);
    c->sp=c->bp;c->bp=rw(b,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u);
    c->ip=rw(b,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u);
    c->cs=rw(b,c->ss,c->sp);c->sp=(uint16_t)(c->sp+4u);
    if(c->cs!=x->cs||c->ip!=x->ip||c->sp!=x->sp||c->bp!=x->bp||c->ss!=x->ss)return PR_290B_FIRST_TRANSFER;
    return PR_290B_FIRST_DONE;
}
void pr_u25_return_session_init(pr_u25_return_session*s){if(s){memset(s,0,sizeof *s);pr_u25_loop_session_init(&s->loop);}}
static pr_u25_return_status return_next(pr_u25_return_session*s,pr_u25_return_diagnostic*d,pr_u25_loop_status st){
    d->loop_status=st;if(st==PR_U25_LOOP_YIELD){s->phase=PR_U25_RETURN_WAITING;return PR_U25_RETURN_YIELD;}
    s->phase=PR_U25_RETURN_FINISHED;
    if(st==PR_U25_LOOP_STRING)return PR_U25_RETURN_STRING;
    if(st==PR_U25_LOOP_LIMIT)return PR_U25_RETURN_LIMIT;
    if(st==PR_U25_LOOP_PROMPT)return PR_U25_RETURN_PROMPT;
    if(st==PR_U25_LOOP_OTHER && !(d->loop.tag_status==PR_U25_TAG_OTHER && d->loop.tag.operand_status==PR_U25_OPERAND_ZERO && s->cpu->ip==0x012cu))return PR_U25_RETURN_OTHER;
    if(st!=PR_U25_LOOP_REACHED && st!=PR_U25_LOOP_OTHER)return PR_U25_RETURN_STOP;
    d->return_status=pr_u25_operand_return(s->bus,PR_BUS_RAM_SIZE,s->cpu,&s->target);
    return d->return_status==PR_290B_FIRST_DONE?PR_U25_RETURN_REACHED:PR_U25_RETURN_STOP;
}
pr_u25_return_status pr_u25_return_start(pr_u25_return_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_u25_return_diagnostic*d){
    pr_u25_loop_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_U25_RETURN_EMPTY||x->bp<42u)return PR_U25_RETURN_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;s->target=(pr_290b_return_target){x->cs,0x02a0u,(uint16_t)(x->bp-0x1au),(uint16_t)(x->bp-0x16u),x->ss};
    st=pr_u25_loop_start(&s->loop,b,n,c,e,cl,pl,x,max_iterations,&d->loop);return return_next(s,d,st);
}
pr_u25_return_status pr_u25_return_resume(pr_u25_return_session*s,pr_u25_return_diagnostic*d){pr_u25_loop_status st;
    if(!s||!d||s->phase!=PR_U25_RETURN_WAITING)return PR_U25_RETURN_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_u25_loop_resume(&s->loop,&d->loop);
    if(st==PR_U25_LOOP_ARGUMENT)return PR_U25_RETURN_ARGUMENT;
    return return_next(s,d,st);
}

pr_290b_first_status pr_u1c_save_word_entry(pr_bus*b,size_t n,pr_r0899_cpu*c,pr_290b_first_diagnostic*d){
    uint16_t cs,target;
    if(!b||!b->ram||!b->ega||!c||!d||n!=PR_BUS_RAM_SIZE||c->ip!=0x02a0u||c->sp<8u||!ordinary(c->cs)||!ordinary(c->ds)||!ordinary(c->ss))return PR_290B_FIRST_ARGUMENT;
    memset(d,0,sizeof *d);d->status=-1;d->formatter_callee=PR_RES_1037_NO_CALLEE;d->formatter_status=-1;
    c->ax=(uint16_t)((c->ax&0xff00u)|rb(b,c->ds,0x7087u));push(b,c,c->ax);
    c->ax=(uint16_t)((c->ax&0xff00u)|rb(b,c->ds,0x7047u));push(b,c,c->ax);
    c->ip=0x02a8u;d->stage=1;if(rb(b,c->cs,c->ip)!=0x9au||rw(b,c->cs,0x02a9u)!=0x004du)return PR_290B_FIRST_CODE;
    cs=c->cs;push(b,c,cs);push(b,c,0x02adu);c->cs=rw(b,cs,0x02abu);c->ip=0x004du;d->stage=2;
    if(!ordinary(c->cs))return PR_290B_FIRST_PROFILE;
    if(rb(b,c->cs,0x004du)!=0xeau||rw(b,c->cs,0x004eu)!=0x06afu)return PR_290B_FIRST_CODE;
    target=rw(b,c->cs,0x0050u);c->cs=target;c->ip=0x06afu;d->stage=3;
    if(!ordinary(target))return PR_290B_FIRST_PROFILE;
    return PR_290B_FIRST_DONE;
}
void pr_save_word_session_init(pr_save_word_session*s){if(s){memset(s,0,sizeof *s);pr_u25_return_session_init(&s->loader);}}
static pr_save_word_status save_word_next(pr_save_word_session*s,pr_save_word_diagnostic*d,pr_u25_return_status st){
    d->loader_status=st;if(st==PR_U25_RETURN_YIELD){s->phase=PR_SAVE_WORD_WAITING;return PR_SAVE_WORD_YIELD;}
    s->phase=PR_SAVE_WORD_FINISHED;if(st==PR_U25_RETURN_STRING)return PR_SAVE_WORD_STRING;
    if(st==PR_U25_RETURN_LIMIT)return PR_SAVE_WORD_LIMIT;
    if(st==PR_U25_RETURN_PROMPT)return PR_SAVE_WORD_PROMPT;
    if(st==PR_U25_RETURN_OTHER)return PR_SAVE_WORD_OTHER;
    if(st!=PR_U25_RETURN_REACHED)return PR_SAVE_WORD_STOP;
    d->entry_status=pr_u1c_save_word_entry(s->bus,PR_BUS_RAM_SIZE,s->cpu,&d->entry);
    return d->entry_status==PR_290B_FIRST_DONE?PR_SAVE_WORD_REACHED:PR_SAVE_WORD_STOP;
}
pr_save_word_status pr_save_word_start(pr_save_word_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_save_word_diagnostic*d){
    pr_u25_return_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_SAVE_WORD_EMPTY)return PR_SAVE_WORD_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_u25_return_start(&s->loader,b,n,c,e,cl,pl,x,max_iterations,&d->loader);return save_word_next(s,d,st);
}
pr_save_word_status pr_save_word_resume(pr_save_word_session*s,pr_save_word_diagnostic*d){pr_u25_return_status st;
    if(!s||!d||s->phase!=PR_SAVE_WORD_WAITING)return PR_SAVE_WORD_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_u25_return_resume(&s->loader,&d->loader);
    if(st==PR_U25_RETURN_ARGUMENT)return PR_SAVE_WORD_ARGUMENT;
    return save_word_next(s,d,st);
}

pr_290b_first_status pr_u25_word_return(pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_290b_return_target*x){
    uint16_t low,high;
    if(!b||!b->ram||!b->ega||!c||!x||n!=PR_BUS_RAM_SIZE||c->ip!=0x06afu||c->sp<4u||c->sp>0xfff8u||!ordinary(c->cs)||!ordinary(c->ds)||!ordinary(c->ss))return PR_290B_FIRST_ARGUMENT;
    push(b,c,c->bp);c->bp=c->sp;c->sp=(uint16_t)(c->sp-2u);
    low=rb(b,c->ss,(uint16_t)(c->bp+8u));c->dx=low;
    high=rb(b,c->ss,(uint16_t)(c->bp+6u));c->cx=(uint16_t)((c->cx&0xff00u)|8u);
    c->ax=second_add(c,(uint16_t)(high<<8),c->dx);
    ww(b,c->ss,(uint16_t)(c->bp-2u),c->ax);c->ax=rw(b,c->ss,(uint16_t)(c->bp-2u));
    c->sp=c->bp;c->bp=rw(b,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u);
    c->ip=rw(b,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u);
    c->cs=rw(b,c->ss,c->sp);c->sp=(uint16_t)(c->sp+6u);
    if(c->cs!=x->cs||c->ip!=x->ip||c->sp!=x->sp||c->bp!=x->bp||c->ss!=x->ss)return PR_290B_FIRST_TRANSFER;
    return PR_290B_FIRST_DONE;
}
void pr_word_return_session_init(pr_word_return_session*s){if(s){memset(s,0,sizeof *s);pr_save_word_session_init(&s->word);}}
static pr_word_return_status word_return_next(pr_word_return_session*s,pr_word_return_diagnostic*d,pr_save_word_status st){
    d->word_status=st;if(st==PR_SAVE_WORD_YIELD){s->phase=PR_WORD_RETURN_WAITING;return PR_WORD_RETURN_YIELD;}
    s->phase=PR_WORD_RETURN_FINISHED;if(st==PR_SAVE_WORD_STRING)return PR_WORD_RETURN_STRING;
    if(st==PR_SAVE_WORD_LIMIT)return PR_WORD_RETURN_LIMIT;
    if(st==PR_SAVE_WORD_PROMPT)return PR_WORD_RETURN_PROMPT;
    if(st==PR_SAVE_WORD_OTHER)return PR_WORD_RETURN_OTHER;
    if(st!=PR_SAVE_WORD_REACHED)return PR_WORD_RETURN_STOP;
    d->return_status=pr_u25_word_return(s->bus,PR_BUS_RAM_SIZE,s->cpu,&s->target);
    return d->return_status==PR_290B_FIRST_DONE?PR_WORD_RETURN_REACHED:PR_WORD_RETURN_STOP;
}
pr_word_return_status pr_word_return_start(pr_word_return_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_word_return_diagnostic*d){
    pr_save_word_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_WORD_RETURN_EMPTY||x->bp<42u)return PR_WORD_RETURN_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;s->target=(pr_290b_return_target){x->cs,0x02adu,(uint16_t)(x->bp-0x1au),(uint16_t)(x->bp-0x16u),x->ss};
    st=pr_save_word_start(&s->word,b,n,c,e,cl,pl,x,max_iterations,&d->word);return word_return_next(s,d,st);
}
pr_word_return_status pr_word_return_resume(pr_word_return_session*s,pr_word_return_diagnostic*d){pr_save_word_status st;
    if(!s||!d||s->phase!=PR_WORD_RETURN_WAITING)return PR_WORD_RETURN_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_save_word_resume(&s->word,&d->word);
    if(st==PR_SAVE_WORD_ARGUMENT)return PR_WORD_RETURN_ARGUMENT;
    return word_return_next(s,d,st);
}

pr_290b_first_status pr_u1c_save_tag_branch(pr_bus*b,size_t n,pr_r0899_cpu*c){
    uint8_t tag;
    if(!b||!b->ram||!b->ega||!c||n!=PR_BUS_RAM_SIZE||c->ip!=0x02adu||c->bp<4u||!ordinary(c->cs)||!ordinary(c->ds)||!ordinary(c->ss))return PR_290B_FIRST_ARGUMENT;
    ww(b,c->ss,(uint16_t)(c->bp-4u),c->ax);tag=rb(b,c->ds,0x7006u);cmp8(c,tag,0x80u);
    c->ip=tag>=0x80u?0x02cfu:0x02b7u;return PR_290B_FIRST_DONE;
}
void pr_save_tag_session_init(pr_save_tag_session*s){if(s){memset(s,0,sizeof *s);pr_word_return_session_init(&s->word);}}
static pr_save_tag_status save_tag_next(pr_save_tag_session*s,pr_save_tag_diagnostic*d,pr_word_return_status st){
    d->word_status=st;if(st==PR_WORD_RETURN_YIELD){s->phase=PR_SAVE_TAG_WAITING;return PR_SAVE_TAG_YIELD;}
    s->phase=PR_SAVE_TAG_FINISHED;if(st==PR_WORD_RETURN_STRING)return PR_SAVE_TAG_LOADER_STRING;
    if(st==PR_WORD_RETURN_LIMIT)return PR_SAVE_TAG_LIMIT;
    if(st==PR_WORD_RETURN_PROMPT)return PR_SAVE_TAG_PROMPT;
    if(st==PR_WORD_RETURN_OTHER)return PR_SAVE_TAG_OTHER;
    if(st!=PR_WORD_RETURN_REACHED)return PR_SAVE_TAG_STOP;
    d->branch_status=pr_u1c_save_tag_branch(s->bus,PR_BUS_RAM_SIZE,s->cpu);
    if(d->branch_status!=PR_290B_FIRST_DONE)return PR_SAVE_TAG_STOP;
    return s->cpu->ip==0x02cfu?PR_SAVE_TAG_STRING:PR_SAVE_TAG_NUMERIC;
}
pr_save_tag_status pr_save_tag_start(pr_save_tag_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_save_tag_diagnostic*d){
    pr_word_return_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_SAVE_TAG_EMPTY)return PR_SAVE_TAG_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_word_return_start(&s->word,b,n,c,e,cl,pl,x,max_iterations,&d->word);return save_tag_next(s,d,st);
}
pr_save_tag_status pr_save_tag_resume(pr_save_tag_session*s,pr_save_tag_diagnostic*d){pr_word_return_status st;
    if(!s||!d||s->phase!=PR_SAVE_TAG_WAITING)return PR_SAVE_TAG_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_word_return_resume(&s->word,&d->word);
    if(st==PR_WORD_RETURN_ARGUMENT)return PR_SAVE_TAG_ARGUMENT;
    return save_tag_next(s,d,st);
}

pr_290b_first_status pr_u1c_save_slot_entry(pr_bus*b,size_t n,pr_r0899_cpu*c,pr_290b_first_diagnostic*d){
    uint16_t cs,target;
    if(!b||!b->ram||!b->ega||!c||!d||n!=PR_BUS_RAM_SIZE||c->ip!=0x02b7u||c->sp<6u||!ordinary(c->cs)||!ordinary(c->ds)||!ordinary(c->ss))return PR_290B_FIRST_ARGUMENT;
    memset(d,0,sizeof *d);d->status=-1;d->formatter_callee=PR_RES_1037_NO_CALLEE;d->formatter_status=-1;
    c->ax=(uint16_t)((c->ax&0xff00u)|1u);push(b,c,c->ax);
    c->ip=0x02bau;d->stage=1;if(rb(b,c->cs,c->ip)!=0x9au||rw(b,c->cs,0x02bbu)!=0x0025u)return PR_290B_FIRST_CODE;
    cs=c->cs;push(b,c,cs);push(b,c,0x02bfu);c->cs=rw(b,cs,0x02bdu);c->ip=0x0025u;d->stage=2;
    if(!ordinary(c->cs))return PR_290B_FIRST_PROFILE;
    if(rb(b,c->cs,0x0025u)!=0xeau||rw(b,c->cs,0x0026u)!=0x0136u)return PR_290B_FIRST_CODE;
    target=rw(b,c->cs,0x0028u);c->cs=target;c->ip=0x0136u;d->stage=3;
    if(!ordinary(target))return PR_290B_FIRST_PROFILE;
    return PR_290B_FIRST_DONE;
}
void pr_save_slot_session_init(pr_save_slot_session*s){if(s){memset(s,0,sizeof *s);pr_save_tag_session_init(&s->branch);}}
static pr_save_slot_status save_slot_next(pr_save_slot_session*s,pr_save_slot_diagnostic*d,pr_save_tag_status st){
    d->branch_status=st;if(st==PR_SAVE_TAG_YIELD){s->phase=PR_SAVE_SLOT_WAITING;return PR_SAVE_SLOT_YIELD;}
    s->phase=PR_SAVE_SLOT_FINISHED;if(st==PR_SAVE_TAG_STRING)return PR_SAVE_SLOT_STRING;
    if(st==PR_SAVE_TAG_LOADER_STRING)return PR_SAVE_SLOT_LOADER_STRING;
    if(st==PR_SAVE_TAG_LIMIT)return PR_SAVE_SLOT_LIMIT;
    if(st==PR_SAVE_TAG_PROMPT)return PR_SAVE_SLOT_PROMPT;
    if(st==PR_SAVE_TAG_OTHER)return PR_SAVE_SLOT_OTHER;
    if(st!=PR_SAVE_TAG_NUMERIC)return PR_SAVE_SLOT_STOP;
    d->entry_status=pr_u1c_save_slot_entry(s->bus,PR_BUS_RAM_SIZE,s->cpu,&d->entry);
    return d->entry_status==PR_290B_FIRST_DONE?PR_SAVE_SLOT_REACHED:PR_SAVE_SLOT_STOP;
}
pr_save_slot_status pr_save_slot_start(pr_save_slot_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_save_slot_diagnostic*d){
    pr_save_tag_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_SAVE_SLOT_EMPTY)return PR_SAVE_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_save_tag_start(&s->branch,b,n,c,e,cl,pl,x,max_iterations,&d->branch);return save_slot_next(s,d,st);
}
pr_save_slot_status pr_save_slot_resume(pr_save_slot_session*s,pr_save_slot_diagnostic*d){pr_save_tag_status st;
    if(!s||!d||s->phase!=PR_SAVE_SLOT_WAITING)return PR_SAVE_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_save_tag_resume(&s->branch,&d->branch);
    if(st==PR_SAVE_TAG_ARGUMENT)return PR_SAVE_SLOT_ARGUMENT;
    return save_slot_next(s,d,st);
}

static uint16_t slot_sub10(pr_r0899_cpu*c,uint16_t a){unsigned r=(a-10u)&0xffffu,p=r&255u;p^=p>>4;p^=p>>2;p^=p>>1;
    c->flags=(uint16_t)((c->flags&~0x08d5u)|(a<10u?1u:0u)|((p&1u)?0u:4u)|(((a^10u^r)&0x10u)?0x10u:0u)|
        (r==0?0x40u:0u)|(r&0x8000u?0x80u:0u)|(((a^10u)&(a^r)&0x8000u)?0x800u:0u));return (uint16_t)r;}
static void slot_index(pr_bus*b,pr_r0899_cpu*c){uint8_t i=rb(b,c->ss,(uint16_t)(c->bp+6u));c->ax=(i&0x80u)?(uint16_t)(0xff00u|i):i;c->di=c->ax;}
pr_290b_first_status pr_u25_slot_prefix(pr_bus*b,size_t n,pr_r0899_cpu*c){
    if(!b||!b->ram||!b->ega||!c||n!=PR_BUS_RAM_SIZE||c->ip!=0x0136u||c->sp<12u||c->sp>0xfffau||!ordinary(c->cs)||!ordinary(c->ds)||!ordinary(c->ss))return PR_290B_FIRST_ARGUMENT;
    push(b,c,c->bp);c->bp=c->sp;c->sp=slot_sub10(c,c->sp);
    slot_index(b,c);c->ax=(uint16_t)((c->ax&0xff00u)|rb(b,c->ds,(uint16_t)(c->di+0x7085u)));wb(b,c->ss,(uint16_t)(c->bp-9u),(uint8_t)c->ax);
    slot_index(b,c);c->ax=(uint16_t)((c->ax&0xff00u)|rb(b,c->ds,(uint16_t)(c->di+0x7045u)));wb(b,c->ss,(uint16_t)(c->bp-10u),(uint8_t)c->ax);
    slot_index(b,c);c->ax=(uint16_t)((c->ax&0xff00u)|rb(b,c->ds,(uint16_t)(c->di+0x7005u)));c->ip=0x0160u;return PR_290B_FIRST_DONE;
}
void pr_slot_prefix_session_init(pr_slot_prefix_session*s){if(s){memset(s,0,sizeof *s);pr_save_slot_session_init(&s->slot);}}
static pr_slot_prefix_status slot_prefix_next(pr_slot_prefix_session*s,pr_slot_prefix_diagnostic*d,pr_save_slot_status st){
    d->slot_status=st;if(st==PR_SAVE_SLOT_YIELD){s->phase=PR_SLOT_PREFIX_WAITING;return PR_SLOT_PREFIX_YIELD;}
    s->phase=PR_SLOT_PREFIX_FINISHED;if(st==PR_SAVE_SLOT_STRING)return PR_SLOT_PREFIX_STRING;
    if(st==PR_SAVE_SLOT_LOADER_STRING)return PR_SLOT_PREFIX_LOADER_STRING;
    if(st==PR_SAVE_SLOT_LIMIT)return PR_SLOT_PREFIX_LIMIT;
    if(st==PR_SAVE_SLOT_PROMPT)return PR_SLOT_PREFIX_PROMPT;
    if(st==PR_SAVE_SLOT_OTHER)return PR_SLOT_PREFIX_OTHER;
    if(st!=PR_SAVE_SLOT_REACHED)return PR_SLOT_PREFIX_STOP;
    d->prefix_status=pr_u25_slot_prefix(s->bus,PR_BUS_RAM_SIZE,s->cpu);
    return d->prefix_status==PR_290B_FIRST_DONE?PR_SLOT_PREFIX_REACHED:PR_SLOT_PREFIX_STOP;
}
pr_slot_prefix_status pr_slot_prefix_start(pr_slot_prefix_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_slot_prefix_diagnostic*d){
    pr_save_slot_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_SLOT_PREFIX_EMPTY||x->bp<44u)return PR_SLOT_PREFIX_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_save_slot_start(&s->slot,b,n,c,e,cl,pl,x,max_iterations,&d->slot);return slot_prefix_next(s,d,st);
}
pr_slot_prefix_status pr_slot_prefix_resume(pr_slot_prefix_session*s,pr_slot_prefix_diagnostic*d){pr_save_slot_status st;
    if(!s||!d||s->phase!=PR_SLOT_PREFIX_WAITING)return PR_SLOT_PREFIX_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_save_slot_resume(&s->slot,&d->slot);
    if(st==PR_SAVE_SLOT_ARGUMENT)return PR_SLOT_PREFIX_ARGUMENT;
    return slot_prefix_next(s,d,st);
}

pr_290b_first_status pr_u25_slot_dispatch(pr_bus*b,size_t n,pr_r0899_cpu*c){
    uint8_t tag;
    if(!b||!b->ram||!b->ega||!c||n!=PR_BUS_RAM_SIZE||c->ip!=0x0160u||c->bp<10u||c->sp!=(uint16_t)(c->bp-10u)||!ordinary(c->cs)||!ordinary(c->ds)||!ordinary(c->ss))return PR_290B_FIRST_ARGUMENT;
    tag=(uint8_t)c->ax;cmp8(c,tag,0u);if(tag==0u){c->ip=0x0164u;return PR_290B_FIRST_DONE;}
    cmp8(c,tag,1u);if(tag==1u){c->ip=0x017au;return PR_290B_FIRST_DONE;}
    cmp8(c,tag,3u);if(tag==3u){c->ip=0x017au;return PR_290B_FIRST_DONE;}
    cmp8(c,tag,0x80u);if(tag==0x80u){c->ip=0x017au;return PR_290B_FIRST_DONE;}
    cmp8(c,tag,2u);if(tag==2u){c->ip=0x019eu;return PR_290B_FIRST_DONE;}
    cmp8(c,tag,0x81u);c->ip=tag==0x81u?0x019eu:0x01adu;return PR_290B_FIRST_DONE;
}
void pr_slot_dispatch_session_init(pr_slot_dispatch_session*s){if(s){memset(s,0,sizeof *s);pr_slot_prefix_session_init(&s->prefix);}}
static pr_slot_dispatch_status slot_dispatch_next(pr_slot_dispatch_session*s,pr_slot_dispatch_diagnostic*d,pr_slot_prefix_status st){
    d->prefix_status=st;if(st==PR_SLOT_PREFIX_YIELD){s->phase=PR_SLOT_DISPATCH_WAITING;return PR_SLOT_DISPATCH_YIELD;}
    s->phase=PR_SLOT_DISPATCH_FINISHED;if(st==PR_SLOT_PREFIX_STRING)return PR_SLOT_DISPATCH_STRING;
    if(st==PR_SLOT_PREFIX_LOADER_STRING)return PR_SLOT_DISPATCH_LOADER_STRING;
    if(st==PR_SLOT_PREFIX_LIMIT)return PR_SLOT_DISPATCH_LIMIT;
    if(st==PR_SLOT_PREFIX_PROMPT)return PR_SLOT_DISPATCH_PROMPT;
    if(st==PR_SLOT_PREFIX_OTHER)return PR_SLOT_DISPATCH_OTHER;
    if(st!=PR_SLOT_PREFIX_REACHED)return PR_SLOT_DISPATCH_STOP;
    d->dispatch_status=pr_u25_slot_dispatch(s->bus,PR_BUS_RAM_SIZE,s->cpu);
    if(d->dispatch_status!=PR_290B_FIRST_DONE)return PR_SLOT_DISPATCH_STOP;
    if(s->cpu->ip==0x0164u)return PR_SLOT_DISPATCH_ZERO;
    if(s->cpu->ip==0x017au)return PR_SLOT_DISPATCH_WORD;
    if(s->cpu->ip==0x019eu)return PR_SLOT_DISPATCH_PAIR;
    return PR_SLOT_DISPATCH_OTHER_TYPE;
}
pr_slot_dispatch_status pr_slot_dispatch_start(pr_slot_dispatch_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_slot_dispatch_diagnostic*d){
    pr_slot_prefix_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_SLOT_DISPATCH_EMPTY)return PR_SLOT_DISPATCH_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_slot_prefix_start(&s->prefix,b,n,c,e,cl,pl,x,max_iterations,&d->prefix);return slot_dispatch_next(s,d,st);
}
pr_slot_dispatch_status pr_slot_dispatch_resume(pr_slot_dispatch_session*s,pr_slot_dispatch_diagnostic*d){pr_slot_prefix_status st;
    if(!s||!d||s->phase!=PR_SLOT_DISPATCH_WAITING)return PR_SLOT_DISPATCH_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_slot_prefix_resume(&s->prefix,&d->prefix);
    if(st==PR_SLOT_PREFIX_ARGUMENT)return PR_SLOT_DISPATCH_ARGUMENT;
    return slot_dispatch_next(s,d,st);
}

pr_290b_first_status pr_u25_slot_pair_entry(pr_bus*b,size_t n,pr_r0899_cpu*c,pr_290b_first_diagnostic*d){
    uint16_t disp;
    if(!b||!b->ram||!b->ega||!c||!d||n!=PR_BUS_RAM_SIZE||c->ip!=0x019eu||c->bp<18u||c->sp!=(uint16_t)(c->bp-10u)||!ordinary(c->cs)||!ordinary(c->ds)||!ordinary(c->ss))return PR_290B_FIRST_ARGUMENT;
    memset(d,0,sizeof *d);d->status=-1;d->formatter_callee=PR_RES_1037_NO_CALLEE;d->formatter_status=-1;
    c->ax=(uint16_t)((c->ax&0xff00u)|rb(b,c->ss,(uint16_t)(c->bp-9u)));push(b,c,c->ax);
    c->ax=(uint16_t)((c->ax&0xff00u)|rb(b,c->ss,(uint16_t)(c->bp-10u)));push(b,c,c->ax);push(b,c,c->cs);
    c->ip=0x01a7u;d->stage=1;if(rb(b,c->cs,0x01a7u)!=0xe8u)return PR_290B_FIRST_CODE;
    disp=rw(b,c->cs,0x01a8u);if((uint16_t)(0x01aau+disp)!=0x06afu)return PR_290B_FIRST_CODE;
    push(b,c,0x01aau);c->ip=0x06afu;d->stage=2;return PR_290B_FIRST_DONE;
}
void pr_slot_pair_session_init(pr_slot_pair_session*s){if(s){memset(s,0,sizeof *s);pr_slot_dispatch_session_init(&s->dispatch);}}
static pr_slot_pair_status slot_pair_next(pr_slot_pair_session*s,pr_slot_pair_diagnostic*d,pr_slot_dispatch_status st){
    d->dispatch_status=st;if(st==PR_SLOT_DISPATCH_YIELD){s->phase=PR_SLOT_PAIR_WAITING;return PR_SLOT_PAIR_YIELD;}
    s->phase=PR_SLOT_PAIR_FINISHED;if(st==PR_SLOT_DISPATCH_ZERO)return PR_SLOT_PAIR_ZERO;
    if(st==PR_SLOT_DISPATCH_WORD)return PR_SLOT_PAIR_WORD;
    if(st==PR_SLOT_DISPATCH_OTHER_TYPE)return PR_SLOT_PAIR_OTHER_TYPE;
    if(st==PR_SLOT_DISPATCH_STRING)return PR_SLOT_PAIR_STRING;
    if(st==PR_SLOT_DISPATCH_LOADER_STRING)return PR_SLOT_PAIR_LOADER_STRING;
    if(st==PR_SLOT_DISPATCH_LIMIT)return PR_SLOT_PAIR_LIMIT;
    if(st==PR_SLOT_DISPATCH_PROMPT)return PR_SLOT_PAIR_PROMPT;
    if(st==PR_SLOT_DISPATCH_OTHER)return PR_SLOT_PAIR_OTHER;
    if(st!=PR_SLOT_DISPATCH_PAIR)return PR_SLOT_PAIR_STOP;
    d->entry_status=pr_u25_slot_pair_entry(s->bus,PR_BUS_RAM_SIZE,s->cpu,&d->entry);
    return d->entry_status==PR_290B_FIRST_DONE?PR_SLOT_PAIR_REACHED:PR_SLOT_PAIR_STOP;
}
pr_slot_pair_status pr_slot_pair_start(pr_slot_pair_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_slot_pair_diagnostic*d){
    pr_slot_dispatch_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_SLOT_PAIR_EMPTY||x->bp<52u)return PR_SLOT_PAIR_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_slot_dispatch_start(&s->dispatch,b,n,c,e,cl,pl,x,max_iterations,&d->dispatch);return slot_pair_next(s,d,st);
}
pr_slot_pair_status pr_slot_pair_resume(pr_slot_pair_session*s,pr_slot_pair_diagnostic*d){pr_slot_dispatch_status st;
    if(!s||!d||s->phase!=PR_SLOT_PAIR_WAITING)return PR_SLOT_PAIR_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_slot_dispatch_resume(&s->dispatch,&d->dispatch);
    if(st==PR_SLOT_DISPATCH_ARGUMENT)return PR_SLOT_PAIR_ARGUMENT;
    return slot_pair_next(s,d,st);
}

void pr_pair_return_session_init(pr_pair_return_session*s){if(s){memset(s,0,sizeof *s);pr_slot_pair_session_init(&s->pair);}}
static pr_pair_return_status pair_return_next(pr_pair_return_session*s,pr_pair_return_diagnostic*d,pr_slot_pair_status st){
    pr_290b_return_target x;
    d->pair_status=st;if(st==PR_SLOT_PAIR_YIELD){s->phase=PR_PAIR_RETURN_WAITING;return PR_PAIR_RETURN_YIELD;}
    s->phase=PR_PAIR_RETURN_FINISHED;if(st==PR_SLOT_PAIR_ZERO)return PR_PAIR_RETURN_ZERO;
    if(st==PR_SLOT_PAIR_WORD)return PR_PAIR_RETURN_WORD;
    if(st==PR_SLOT_PAIR_OTHER_TYPE)return PR_PAIR_RETURN_OTHER_TYPE;
    if(st==PR_SLOT_PAIR_STRING)return PR_PAIR_RETURN_STRING;
    if(st==PR_SLOT_PAIR_LOADER_STRING)return PR_PAIR_RETURN_LOADER_STRING;
    if(st==PR_SLOT_PAIR_LIMIT)return PR_PAIR_RETURN_LIMIT;
    if(st==PR_SLOT_PAIR_PROMPT)return PR_PAIR_RETURN_PROMPT;
    if(st==PR_SLOT_PAIR_OTHER)return PR_PAIR_RETURN_OTHER;
    if(st!=PR_SLOT_PAIR_REACHED)return PR_PAIR_RETURN_STOP;
    x=(pr_290b_return_target){s->cpu->cs,0x01aau,(uint16_t)(s->cpu->bp-10u),s->cpu->bp,s->cpu->ss};
    d->return_status=pr_u25_word_return(s->bus,PR_BUS_RAM_SIZE,s->cpu,&x);
    return d->return_status==PR_290B_FIRST_DONE?PR_PAIR_RETURN_REACHED:PR_PAIR_RETURN_STOP;
}
pr_pair_return_status pr_pair_return_start(pr_pair_return_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_pair_return_diagnostic*d){
    pr_slot_pair_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_PAIR_RETURN_EMPTY||x->bp<56u)return PR_PAIR_RETURN_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_slot_pair_start(&s->pair,b,n,c,e,cl,pl,x,max_iterations,&d->pair);return pair_return_next(s,d,st);
}
pr_pair_return_status pr_pair_return_resume(pr_pair_return_session*s,pr_pair_return_diagnostic*d){pr_slot_pair_status st;
    if(!s||!d||s->phase!=PR_PAIR_RETURN_WAITING)return PR_PAIR_RETURN_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_slot_pair_resume(&s->pair,&d->pair);
    if(st==PR_SLOT_PAIR_ARGUMENT)return PR_PAIR_RETURN_ARGUMENT;
    return pair_return_next(s,d,st);
}

pr_290b_first_status pr_u25_slot_value_return(pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_290b_return_target*x){
    if(!b||!b->ram||!b->ega||!c||!x||n!=PR_BUS_RAM_SIZE||c->ip!=0x01aau||c->bp<10u||c->bp>0xfff8u||c->sp!=(uint16_t)(c->bp-10u)||!ordinary(c->cs)||!ordinary(c->ds)||!ordinary(c->ss))return PR_290B_FIRST_ARGUMENT;
    ww(b,c->ss,(uint16_t)(c->bp-2u),c->ax);c->ax=rw(b,c->ss,(uint16_t)(c->bp-2u));
    c->sp=c->bp;c->bp=rw(b,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u);
    c->ip=rw(b,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u);
    c->cs=rw(b,c->ss,c->sp);c->sp=(uint16_t)(c->sp+4u);
    if(c->cs!=x->cs||c->ip!=x->ip||c->sp!=x->sp||c->bp!=x->bp||c->ss!=x->ss)return PR_290B_FIRST_TRANSFER;
    return PR_290B_FIRST_DONE;
}
void pr_value_return_session_init(pr_value_return_session*s){if(s){memset(s,0,sizeof *s);pr_pair_return_session_init(&s->pair);}}
static pr_value_return_status value_return_next(pr_value_return_session*s,pr_value_return_diagnostic*d,pr_pair_return_status st){
    d->pair_status=st;if(st==PR_PAIR_RETURN_YIELD){s->phase=PR_VALUE_RETURN_WAITING;return PR_VALUE_RETURN_YIELD;}
    s->phase=PR_VALUE_RETURN_FINISHED;if(st==PR_PAIR_RETURN_ZERO)return PR_VALUE_RETURN_ZERO;
    if(st==PR_PAIR_RETURN_WORD)return PR_VALUE_RETURN_WORD;
    if(st==PR_PAIR_RETURN_OTHER_TYPE)return PR_VALUE_RETURN_OTHER_TYPE;
    if(st==PR_PAIR_RETURN_STRING)return PR_VALUE_RETURN_STRING;
    if(st==PR_PAIR_RETURN_LOADER_STRING)return PR_VALUE_RETURN_LOADER_STRING;
    if(st==PR_PAIR_RETURN_LIMIT)return PR_VALUE_RETURN_LIMIT;
    if(st==PR_PAIR_RETURN_PROMPT)return PR_VALUE_RETURN_PROMPT;
    if(st==PR_PAIR_RETURN_OTHER)return PR_VALUE_RETURN_OTHER;
    if(st!=PR_PAIR_RETURN_REACHED)return PR_VALUE_RETURN_STOP;
    d->return_status=pr_u25_slot_value_return(s->bus,PR_BUS_RAM_SIZE,s->cpu,&s->target);
    return d->return_status==PR_290B_FIRST_DONE?PR_VALUE_RETURN_REACHED:PR_VALUE_RETURN_STOP;
}
pr_value_return_status pr_value_return_start(pr_value_return_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_value_return_diagnostic*d){
    pr_pair_return_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_VALUE_RETURN_EMPTY||x->bp<56u)return PR_VALUE_RETURN_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;s->target=(pr_290b_return_target){x->cs,0x02bfu,(uint16_t)(x->bp-0x1au),(uint16_t)(x->bp-0x16u),x->ss};
    st=pr_pair_return_start(&s->pair,b,n,c,e,cl,pl,x,max_iterations,&d->pair);return value_return_next(s,d,st);
}
pr_value_return_status pr_value_return_resume(pr_value_return_session*s,pr_value_return_diagnostic*d){pr_pair_return_status st;
    if(!s||!d||s->phase!=PR_VALUE_RETURN_WAITING)return PR_VALUE_RETURN_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_pair_return_resume(&s->pair,&d->pair);
    if(st==PR_PAIR_RETURN_ARGUMENT)return PR_VALUE_RETURN_ARGUMENT;
    return value_return_next(s,d,st);
}

pr_290b_first_status pr_u1c_save_store_entry(pr_bus*b,size_t n,pr_r0899_cpu*c,pr_290b_first_diagnostic*d){
    uint16_t cs,target,v;
    if(!b||!b->ram||!b->ega||!c||!d||n!=PR_BUS_RAM_SIZE||c->ip!=0x02bfu||c->bp<12u||c->sp!=(uint16_t)(c->bp-4u)||!ordinary(c->cs)||!ordinary(c->ds)||!ordinary(c->ss))return PR_290B_FIRST_ARGUMENT;
    memset(d,0,sizeof *d);d->status=-1;d->formatter_callee=PR_RES_1037_NO_CALLEE;d->formatter_status=-1;
    ww(b,c->ss,(uint16_t)(c->bp-2u),c->ax);v=rw(b,c->ss,(uint16_t)(c->bp-4u));push(b,c,v);
    v=rw(b,c->ss,(uint16_t)(c->bp-2u));push(b,c,v);
    c->ip=0x02c8u;d->stage=1;if(rb(b,c->cs,c->ip)!=0x9au||rw(b,c->cs,0x02c9u)!=0x0052u)return PR_290B_FIRST_CODE;
    cs=c->cs;push(b,c,cs);push(b,c,0x02cdu);c->cs=rw(b,cs,0x02cbu);c->ip=0x0052u;d->stage=2;
    if(!ordinary(c->cs))return PR_290B_FIRST_PROFILE;
    if(rb(b,c->cs,0x0052u)!=0xeau||rw(b,c->cs,0x0053u)!=0x0c77u)return PR_290B_FIRST_CODE;
    target=rw(b,c->cs,0x0055u);c->cs=target;c->ip=0x0c77u;d->stage=3;
    if(!ordinary(target))return PR_290B_FIRST_PROFILE;
    return PR_290B_FIRST_DONE;
}
void pr_save_store_session_init(pr_save_store_session*s){if(s){memset(s,0,sizeof *s);pr_value_return_session_init(&s->value);}}
static pr_save_store_status save_store_next(pr_save_store_session*s,pr_save_store_diagnostic*d,pr_value_return_status st){
    d->value_status=st;if(st==PR_VALUE_RETURN_YIELD){s->phase=PR_SAVE_STORE_WAITING;return PR_SAVE_STORE_YIELD;}
    s->phase=PR_SAVE_STORE_FINISHED;if(st==PR_VALUE_RETURN_ZERO)return PR_SAVE_STORE_ZERO;
    if(st==PR_VALUE_RETURN_WORD)return PR_SAVE_STORE_WORD;
    if(st==PR_VALUE_RETURN_OTHER_TYPE)return PR_SAVE_STORE_OTHER_TYPE;
    if(st==PR_VALUE_RETURN_STRING)return PR_SAVE_STORE_STRING;
    if(st==PR_VALUE_RETURN_LOADER_STRING)return PR_SAVE_STORE_LOADER_STRING;
    if(st==PR_VALUE_RETURN_LIMIT)return PR_SAVE_STORE_LIMIT;
    if(st==PR_VALUE_RETURN_PROMPT)return PR_SAVE_STORE_PROMPT;
    if(st==PR_VALUE_RETURN_OTHER)return PR_SAVE_STORE_OTHER;
    if(st!=PR_VALUE_RETURN_REACHED)return PR_SAVE_STORE_STOP;
    d->entry_status=pr_u1c_save_store_entry(s->bus,PR_BUS_RAM_SIZE,s->cpu,&d->entry);
    return d->entry_status==PR_290B_FIRST_DONE?PR_SAVE_STORE_REACHED:PR_SAVE_STORE_STOP;
}
pr_save_store_status pr_save_store_start(pr_save_store_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_save_store_diagnostic*d){
    pr_value_return_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_SAVE_STORE_EMPTY)return PR_SAVE_STORE_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_value_return_start(&s->value,b,n,c,e,cl,pl,x,max_iterations,&d->value);return save_store_next(s,d,st);
}
pr_save_store_status pr_save_store_resume(pr_save_store_session*s,pr_save_store_diagnostic*d){pr_value_return_status st;
    if(!s||!d||s->phase!=PR_SAVE_STORE_WAITING)return PR_SAVE_STORE_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_value_return_resume(&s->value,&d->value);
    if(st==PR_VALUE_RETURN_ARGUMENT)return PR_SAVE_STORE_ARGUMENT;
    return save_store_next(s,d,st);
}

static uint16_t store_sub2(pr_r0899_cpu*c,uint16_t a){unsigned r=(a-2u)&0xffffu,p=r&255u;p^=p>>4;p^=p>>2;p^=p>>1;
    c->flags=(uint16_t)((c->flags&~0x08d5u)|(a<2u?1u:0u)|((p&1u)?0u:4u)|(((a^2u^r)&0x10u)?0x10u:0u)|
        (r==0?0x40u:0u)|(r&0x8000u?0x80u:0u)|(((a^2u)&(a^r)&0x8000u)?0x800u:0u));return (uint16_t)r;}
pr_290b_first_status pr_u25_store_classifier_entry(pr_bus*b,size_t n,pr_r0899_cpu*c,pr_290b_first_diagnostic*d){
    uint16_t v,disp;
    if(!b||!b->ram||!b->ega||!c||!d||n!=PR_BUS_RAM_SIZE||c->ip!=0x0c77u||c->sp<10u||c->sp>0xfff8u||!ordinary(c->cs)||!ordinary(c->ds)||!ordinary(c->ss))return PR_290B_FIRST_ARGUMENT;
    memset(d,0,sizeof *d);d->status=-1;d->formatter_callee=PR_RES_1037_NO_CALLEE;d->formatter_status=-1;
    push(b,c,c->bp);c->bp=c->sp;c->sp=store_sub2(c,c->sp);v=rw(b,c->ss,(uint16_t)(c->bp+8u));push(b,c,v);push(b,c,c->cs);
    c->ip=0x0c81u;d->stage=1;if(rb(b,c->cs,0x0c81u)!=0xe8u)return PR_290B_FIRST_CODE;
    disp=rw(b,c->cs,0x0c82u);if((uint16_t)(0x0c84u+disp)!=0x06fbu)return PR_290B_FIRST_CODE;
    push(b,c,0x0c84u);c->ip=0x06fbu;d->stage=2;return PR_290B_FIRST_DONE;
}
void pr_store_class_session_init(pr_store_class_session*s){if(s){memset(s,0,sizeof *s);pr_save_store_session_init(&s->store);}}
static pr_store_class_status store_class_next(pr_store_class_session*s,pr_store_class_diagnostic*d,pr_save_store_status st){
    d->store_status=st;if(st==PR_SAVE_STORE_YIELD){s->phase=PR_STORE_CLASS_WAITING;return PR_STORE_CLASS_YIELD;}
    s->phase=PR_STORE_CLASS_FINISHED;if(st==PR_SAVE_STORE_ZERO)return PR_STORE_CLASS_ZERO;
    if(st==PR_SAVE_STORE_WORD)return PR_STORE_CLASS_WORD;
    if(st==PR_SAVE_STORE_OTHER_TYPE)return PR_STORE_CLASS_OTHER_TYPE;
    if(st==PR_SAVE_STORE_STRING)return PR_STORE_CLASS_STRING;
    if(st==PR_SAVE_STORE_LOADER_STRING)return PR_STORE_CLASS_LOADER_STRING;
    if(st==PR_SAVE_STORE_LIMIT)return PR_STORE_CLASS_LIMIT;
    if(st==PR_SAVE_STORE_PROMPT)return PR_STORE_CLASS_PROMPT;
    if(st==PR_SAVE_STORE_OTHER)return PR_STORE_CLASS_OTHER;
    if(st!=PR_SAVE_STORE_REACHED)return PR_STORE_CLASS_STOP;
    d->entry_status=pr_u25_store_classifier_entry(s->bus,PR_BUS_RAM_SIZE,s->cpu,&d->entry);
    return d->entry_status==PR_290B_FIRST_DONE?PR_STORE_CLASS_REACHED:PR_STORE_CLASS_STOP;
}
pr_store_class_status pr_store_class_start(pr_store_class_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_store_class_diagnostic*d){
    pr_save_store_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_STORE_CLASS_EMPTY)return PR_STORE_CLASS_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_save_store_start(&s->store,b,n,c,e,cl,pl,x,max_iterations,&d->store);return store_class_next(s,d,st);
}
pr_store_class_status pr_store_class_resume(pr_store_class_session*s,pr_store_class_diagnostic*d){pr_save_store_status st;
    if(!s||!d||s->phase!=PR_STORE_CLASS_WAITING)return PR_STORE_CLASS_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_save_store_resume(&s->store,&d->store);
    if(st==PR_SAVE_STORE_ARGUMENT)return PR_STORE_CLASS_ARGUMENT;
    return store_class_next(s,d,st);
}

static void class_cmp16(pr_r0899_cpu*c,uint16_t a,uint16_t b){unsigned r=(a-b)&0xffffu,p=r&255u;p^=p>>4;p^=p>>2;p^=p>>1;
    c->flags=(uint16_t)((c->flags&~0x08d5u)|(a<b?1u:0u)|((p&1u)?0u:4u)|(((a^b^r)&0x10u)?0x10u:0u)|
        (r==0?0x40u:0u)|(r&0x8000u?0x80u:0u)|(((a^b)&(a^r)&0x8000u)?0x800u:0u));}
static void class_range(pr_bus*b,pr_r0899_cpu*c,uint16_t lo,uint16_t hi,uint8_t value){uint16_t a=rw(b,c->ss,(uint16_t)(c->bp+6u));class_cmp16(c,a,lo);if(a<lo)return;
    a=rw(b,c->ss,(uint16_t)(c->bp+6u));class_cmp16(c,a,hi);if(a<=hi)wb(b,c->ss,(uint16_t)(c->bp-1u),value);}
pr_290b_first_status pr_u25_classifier_return(pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_290b_return_target*x){
    if(!b||!b->ram||!b->ega||!c||!x||n!=PR_BUS_RAM_SIZE||c->ip!=0x06fbu||c->sp<4u||c->sp>0xfff8u||!ordinary(c->cs)||!ordinary(c->ds)||!ordinary(c->ss))return PR_290B_FIRST_ARGUMENT;
    push(b,c,c->bp);c->bp=c->sp;c->sp=store_sub2(c,c->sp);wb(b,c->ss,(uint16_t)(c->bp-1u),4u);
    class_range(b,c,0x4900u,0x4cffu,0u);class_range(b,c,0x6b00u,0x6effu,1u);
    class_range(b,c,0x9700u,0x98ffu,2u);class_range(b,c,0x9900u,0xb6ffu,3u);
    c->ax=(uint16_t)((c->ax&0xff00u)|rb(b,c->ss,(uint16_t)(c->bp-1u)));
    c->sp=c->bp;c->bp=rw(b,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u);
    c->ip=rw(b,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u);
    c->cs=rw(b,c->ss,c->sp);c->sp=(uint16_t)(c->sp+4u);
    if(c->cs!=x->cs||c->ip!=x->ip||c->sp!=x->sp||c->bp!=x->bp||c->ss!=x->ss)return PR_290B_FIRST_TRANSFER;
    return PR_290B_FIRST_DONE;
}
void pr_class_return_session_init(pr_class_return_session*s){if(s){memset(s,0,sizeof *s);pr_store_class_session_init(&s->store);}}
static pr_class_return_status class_return_next(pr_class_return_session*s,pr_class_return_diagnostic*d,pr_store_class_status st){
    pr_290b_return_target x;
    d->store_status=st;if(st==PR_STORE_CLASS_YIELD){s->phase=PR_CLASS_RETURN_WAITING;return PR_CLASS_RETURN_YIELD;}
    s->phase=PR_CLASS_RETURN_FINISHED;if(st==PR_STORE_CLASS_ZERO)return PR_CLASS_RETURN_ZERO;
    if(st==PR_STORE_CLASS_WORD)return PR_CLASS_RETURN_WORD;
    if(st==PR_STORE_CLASS_OTHER_TYPE)return PR_CLASS_RETURN_OTHER_TYPE;
    if(st==PR_STORE_CLASS_STRING)return PR_CLASS_RETURN_STRING;
    if(st==PR_STORE_CLASS_LOADER_STRING)return PR_CLASS_RETURN_LOADER_STRING;
    if(st==PR_STORE_CLASS_LIMIT)return PR_CLASS_RETURN_LIMIT;
    if(st==PR_STORE_CLASS_PROMPT)return PR_CLASS_RETURN_PROMPT;
    if(st==PR_STORE_CLASS_OTHER)return PR_CLASS_RETURN_OTHER;
    if(st!=PR_STORE_CLASS_REACHED)return PR_CLASS_RETURN_STOP;
    x=(pr_290b_return_target){s->cpu->cs,0x0c84u,(uint16_t)(s->cpu->sp+6u),s->cpu->bp,s->cpu->ss};
    d->return_status=pr_u25_classifier_return(s->bus,PR_BUS_RAM_SIZE,s->cpu,&x);
    return d->return_status==PR_290B_FIRST_DONE?PR_CLASS_RETURN_REACHED:PR_CLASS_RETURN_STOP;
}
pr_class_return_status pr_class_return_start(pr_class_return_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_class_return_diagnostic*d){
    pr_store_class_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_CLASS_RETURN_EMPTY)return PR_CLASS_RETURN_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_store_class_start(&s->store,b,n,c,e,cl,pl,x,max_iterations,&d->store);return class_return_next(s,d,st);
}
pr_class_return_status pr_class_return_resume(pr_class_return_session*s,pr_class_return_diagnostic*d){pr_store_class_status st;
    if(!s||!d||s->phase!=PR_CLASS_RETURN_WAITING)return PR_CLASS_RETURN_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_store_class_resume(&s->store,&d->store);
    if(st==PR_STORE_CLASS_ARGUMENT)return PR_CLASS_RETURN_ARGUMENT;
    return class_return_next(s,d,st);
}

pr_290b_first_status pr_u25_store_class_dispatch(pr_bus*b,size_t n,pr_r0899_cpu*c){
    uint8_t cls;
    if(!b||!b->ram||!b->ega||!c||n!=PR_BUS_RAM_SIZE||c->ip!=0x0c84u||c->bp<2u||c->sp!=(uint16_t)(c->bp-2u)||!ordinary(c->cs)||!ordinary(c->ds)||!ordinary(c->ss))return PR_290B_FIRST_ARGUMENT;
    wb(b,c->ss,(uint16_t)(c->bp-1u),(uint8_t)c->ax);cls=rb(b,c->ss,(uint16_t)(c->bp-1u));c->ax=(uint16_t)((c->ax&0xff00u)|cls);
    cmp8(c,cls,0u);if(cls==0u){c->ip=0x0c8eu;return PR_290B_FIRST_DONE;}
    cmp8(c,cls,1u);if(cls==1u){c->ip=0x0cbbu;return PR_290B_FIRST_DONE;}
    cmp8(c,cls,2u);if(cls==2u){c->ip=0x0cdfu;return PR_290B_FIRST_DONE;}
    cmp8(c,cls,3u);if(cls==3u){c->ip=0x0cf9u;return PR_290B_FIRST_DONE;}
    cmp8(c,cls,4u);c->ip=cls==4u?0x0d14u:0x0e06u;return PR_290B_FIRST_DONE;
}
void pr_store_dispatch_session_init(pr_store_dispatch_session*s){if(s){memset(s,0,sizeof *s);pr_class_return_session_init(&s->classifier);}}
static pr_store_dispatch_status store_dispatch_next(pr_store_dispatch_session*s,pr_store_dispatch_diagnostic*d,pr_class_return_status st){
    d->classifier_status=st;if(st==PR_CLASS_RETURN_YIELD){s->phase=PR_STORE_DISPATCH_WAITING;return PR_STORE_DISPATCH_YIELD;}
    s->phase=PR_STORE_DISPATCH_FINISHED;if(st==PR_CLASS_RETURN_ZERO)return PR_STORE_DISPATCH_ZERO;
    if(st==PR_CLASS_RETURN_WORD)return PR_STORE_DISPATCH_WORD;
    if(st==PR_CLASS_RETURN_OTHER_TYPE)return PR_STORE_DISPATCH_OTHER_TYPE;
    if(st==PR_CLASS_RETURN_STRING)return PR_STORE_DISPATCH_STRING;
    if(st==PR_CLASS_RETURN_LOADER_STRING)return PR_STORE_DISPATCH_LOADER_STRING;
    if(st==PR_CLASS_RETURN_LIMIT)return PR_STORE_DISPATCH_LIMIT;
    if(st==PR_CLASS_RETURN_PROMPT)return PR_STORE_DISPATCH_PROMPT;
    if(st==PR_CLASS_RETURN_OTHER)return PR_STORE_DISPATCH_OTHER;
    if(st!=PR_CLASS_RETURN_REACHED)return PR_STORE_DISPATCH_STOP;
    d->dispatch_status=pr_u25_store_class_dispatch(s->bus,PR_BUS_RAM_SIZE,s->cpu);
    if(d->dispatch_status!=PR_290B_FIRST_DONE)return PR_STORE_DISPATCH_STOP;
    if(s->cpu->ip==0x0c8eu)return PR_STORE_DISPATCH_CLASS0;
    if(s->cpu->ip==0x0cbbu)return PR_STORE_DISPATCH_CLASS1;
    if(s->cpu->ip==0x0cdfu)return PR_STORE_DISPATCH_CLASS2;
    if(s->cpu->ip==0x0cf9u)return PR_STORE_DISPATCH_CLASS3;
    return s->cpu->ip==0x0d14u?PR_STORE_DISPATCH_CLASS4:PR_STORE_DISPATCH_UNMATCHED;
}
pr_store_dispatch_status pr_store_dispatch_start(pr_store_dispatch_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_store_dispatch_diagnostic*d){
    pr_class_return_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_STORE_DISPATCH_EMPTY)return PR_STORE_DISPATCH_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_class_return_start(&s->classifier,b,n,c,e,cl,pl,x,max_iterations,&d->classifier);return store_dispatch_next(s,d,st);
}
pr_store_dispatch_status pr_store_dispatch_resume(pr_store_dispatch_session*s,pr_store_dispatch_diagnostic*d){pr_class_return_status st;
    if(!s||!d||s->phase!=PR_STORE_DISPATCH_WAITING)return PR_STORE_DISPATCH_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_class_return_resume(&s->classifier,&d->classifier);
    if(st==PR_CLASS_RETURN_ARGUMENT)return PR_STORE_DISPATCH_ARGUMENT;
    return store_dispatch_next(s,d,st);
}

pr_290b_first_status pr_u25_store_class4_low(pr_bus*b,size_t n,pr_r0899_cpu*c){
    uint16_t a,target=0;
    if(!b||!b->ram||!b->ega||!c||n!=PR_BUS_RAM_SIZE||c->ip!=0x0d14u||c->bp<2u||c->bp>0xfff6u||c->sp!=(uint16_t)(c->bp-2u)||!ordinary(c->cs)||!ordinary(c->ds)||!ordinary(c->ss))return PR_290B_FIRST_ARGUMENT;
    a=rw(b,c->ss,(uint16_t)(c->bp+8u));class_cmp16(c,a,0xbf68u);if(a>=0xbf68u){c->ip=0x0d5bu;return PR_290B_FIRST_OUTSIDE;}
    c->ax=rw(b,c->ss,(uint16_t)(c->bp+8u));a=c->ax;
    class_cmp16(c,a,0x00fbu);if(a==0x00fbu)goto done;
    class_cmp16(c,a,0x00fcu);if(a==0x00fcu)goto done;
    class_cmp16(c,a,0x00b1u);if(a==0x00b1u)goto done;
    class_cmp16(c,a,0x03deu);if(a==0x03deu){target=0x84ccu;goto store;}
    class_cmp16(c,a,0x00b8u);if(a==0x00b8u){target=0x84ceu;goto store;}
    class_cmp16(c,a,0x00b9u);if(a==0x00b9u){target=0x84d0u;goto store;}
    goto done;
store:c->ax=rw(b,c->ss,(uint16_t)(c->bp+6u));ww(b,c->ds,target,c->ax);
done:c->ip=0x0e06u;return PR_290B_FIRST_DONE;
}
void pr_class4_low_session_init(pr_class4_low_session*s){if(s){memset(s,0,sizeof *s);pr_store_dispatch_session_init(&s->dispatch);}}
static pr_class4_low_status class4_low_next(pr_class4_low_session*s,pr_class4_low_diagnostic*d,pr_store_dispatch_status st){
    d->dispatch_status=st;if(st==PR_STORE_DISPATCH_YIELD){s->phase=PR_CLASS4_LOW_WAITING;return PR_CLASS4_LOW_YIELD;}
    s->phase=PR_CLASS4_LOW_FINISHED;if(st==PR_STORE_DISPATCH_CLASS0)return PR_CLASS4_LOW_CLASS0;
    if(st==PR_STORE_DISPATCH_CLASS1)return PR_CLASS4_LOW_CLASS1;
    if(st==PR_STORE_DISPATCH_CLASS2)return PR_CLASS4_LOW_CLASS2;
    if(st==PR_STORE_DISPATCH_CLASS3)return PR_CLASS4_LOW_CLASS3;
    if(st==PR_STORE_DISPATCH_UNMATCHED)return PR_CLASS4_LOW_UNMATCHED;
    if(st==PR_STORE_DISPATCH_ZERO)return PR_CLASS4_LOW_ZERO;
    if(st==PR_STORE_DISPATCH_WORD)return PR_CLASS4_LOW_WORD;
    if(st==PR_STORE_DISPATCH_OTHER_TYPE)return PR_CLASS4_LOW_OTHER_TYPE;
    if(st==PR_STORE_DISPATCH_STRING)return PR_CLASS4_LOW_STRING;
    if(st==PR_STORE_DISPATCH_LOADER_STRING)return PR_CLASS4_LOW_LOADER_STRING;
    if(st==PR_STORE_DISPATCH_LIMIT)return PR_CLASS4_LOW_LIMIT;
    if(st==PR_STORE_DISPATCH_PROMPT)return PR_CLASS4_LOW_PROMPT;
    if(st==PR_STORE_DISPATCH_OTHER)return PR_CLASS4_LOW_OTHER;
    if(st!=PR_STORE_DISPATCH_CLASS4)return PR_CLASS4_LOW_STOP;
    d->low_status=pr_u25_store_class4_low(s->bus,PR_BUS_RAM_SIZE,s->cpu);
    if(d->low_status==PR_290B_FIRST_OUTSIDE)return PR_CLASS4_LOW_HIGH;
    return d->low_status==PR_290B_FIRST_DONE?PR_CLASS4_LOW_REACHED:PR_CLASS4_LOW_STOP;
}
pr_class4_low_status pr_class4_low_start(pr_class4_low_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_class4_low_diagnostic*d){
    pr_store_dispatch_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_CLASS4_LOW_EMPTY)return PR_CLASS4_LOW_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_store_dispatch_start(&s->dispatch,b,n,c,e,cl,pl,x,max_iterations,&d->dispatch);return class4_low_next(s,d,st);
}
pr_class4_low_status pr_class4_low_resume(pr_class4_low_session*s,pr_class4_low_diagnostic*d){pr_store_dispatch_status st;
    if(!s||!d||s->phase!=PR_CLASS4_LOW_WAITING)return PR_CLASS4_LOW_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_store_dispatch_resume(&s->dispatch,&d->dispatch);
    if(st==PR_STORE_DISPATCH_ARGUMENT)return PR_CLASS4_LOW_ARGUMENT;
    return class4_low_next(s,d,st);
}

pr_290b_first_status pr_u25_store_return(pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_290b_return_target*x){
    if(!b||!b->ram||!b->ega||!c||!x||n!=PR_BUS_RAM_SIZE||c->ip!=0x0e06u||c->bp<2u||c->bp>0xfff6u||c->sp!=(uint16_t)(c->bp-2u)||!ordinary(c->cs)||!ordinary(c->ds)||!ordinary(c->ss))return PR_290B_FIRST_ARGUMENT;
    c->sp=c->bp;c->bp=rw(b,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u);
    c->ip=rw(b,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u);
    c->cs=rw(b,c->ss,c->sp);c->sp=(uint16_t)(c->sp+6u);
    if(c->cs!=x->cs||c->ip!=x->ip||c->sp!=x->sp||c->bp!=x->bp||c->ss!=x->ss)return PR_290B_FIRST_TRANSFER;
    return PR_290B_FIRST_DONE;
}
void pr_store_return_session_init(pr_store_return_session*s){if(s){memset(s,0,sizeof *s);pr_class4_low_session_init(&s->low);}}
static pr_store_return_status store_return_next(pr_store_return_session*s,pr_store_return_diagnostic*d,pr_class4_low_status st){
    d->low_status=st;if(st==PR_CLASS4_LOW_YIELD){s->phase=PR_STORE_RETURN_WAITING;return PR_STORE_RETURN_YIELD;}
    s->phase=PR_STORE_RETURN_FINISHED;if(st==PR_CLASS4_LOW_HIGH)return PR_STORE_RETURN_HIGH;
    if(st==PR_CLASS4_LOW_CLASS0)return PR_STORE_RETURN_CLASS0;
    if(st==PR_CLASS4_LOW_CLASS1)return PR_STORE_RETURN_CLASS1;
    if(st==PR_CLASS4_LOW_CLASS2)return PR_STORE_RETURN_CLASS2;
    if(st==PR_CLASS4_LOW_CLASS3)return PR_STORE_RETURN_CLASS3;
    if(st==PR_CLASS4_LOW_UNMATCHED)return PR_STORE_RETURN_UNMATCHED;
    if(st==PR_CLASS4_LOW_ZERO)return PR_STORE_RETURN_ZERO;
    if(st==PR_CLASS4_LOW_WORD)return PR_STORE_RETURN_WORD;
    if(st==PR_CLASS4_LOW_OTHER_TYPE)return PR_STORE_RETURN_OTHER_TYPE;
    if(st==PR_CLASS4_LOW_STRING)return PR_STORE_RETURN_STRING;
    if(st==PR_CLASS4_LOW_LOADER_STRING)return PR_STORE_RETURN_LOADER_STRING;
    if(st==PR_CLASS4_LOW_LIMIT)return PR_STORE_RETURN_LIMIT;
    if(st==PR_CLASS4_LOW_PROMPT)return PR_STORE_RETURN_PROMPT;
    if(st==PR_CLASS4_LOW_OTHER)return PR_STORE_RETURN_OTHER;
    if(st!=PR_CLASS4_LOW_REACHED)return PR_STORE_RETURN_STOP;
    d->return_status=pr_u25_store_return(s->bus,PR_BUS_RAM_SIZE,s->cpu,&s->target);
    return d->return_status==PR_290B_FIRST_DONE?PR_STORE_RETURN_REACHED:PR_STORE_RETURN_STOP;
}
pr_store_return_status pr_store_return_start(pr_store_return_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_store_return_diagnostic*d){
    pr_class4_low_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_STORE_RETURN_EMPTY||x->bp<56u)return PR_STORE_RETURN_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;s->target=(pr_290b_return_target){x->cs,0x02cdu,(uint16_t)(x->bp-0x1au),(uint16_t)(x->bp-0x16u),x->ss};
    st=pr_class4_low_start(&s->low,b,n,c,e,cl,pl,x,max_iterations,&d->low);return store_return_next(s,d,st);
}
pr_store_return_status pr_store_return_resume(pr_store_return_session*s,pr_store_return_diagnostic*d){pr_class4_low_status st;
    if(!s||!d||s->phase!=PR_STORE_RETURN_WAITING)return PR_STORE_RETURN_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_class4_low_resume(&s->low,&d->low);
    if(st==PR_CLASS4_LOW_ARGUMENT)return PR_STORE_RETURN_ARGUMENT;
    return store_return_next(s,d,st);
}

pr_290b_first_status pr_u1c_save_handler_return(pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_290b_return_target*x){
    if(!b||!b->ram||!b->ega||!c||!x||n!=PR_BUS_RAM_SIZE||c->ip!=0x02cdu||c->bp<4u||c->bp>0xfffau||c->sp!=(uint16_t)(c->bp-4u)||!ordinary(c->cs)||!ordinary(c->ds)||!ordinary(c->ss))return PR_290B_FIRST_ARGUMENT;
    c->sp=c->bp;c->bp=rw(b,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u);
    c->ip=rw(b,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u);
    c->cs=rw(b,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u);
    if(c->cs!=x->cs||c->ip!=x->ip||c->sp!=x->sp||c->bp!=x->bp||c->ss!=x->ss)return PR_290B_FIRST_TRANSFER;
    return PR_290B_FIRST_DONE;
}
void pr_save_handler_session_init(pr_save_handler_session*s){if(s){memset(s,0,sizeof *s);pr_store_return_session_init(&s->store);}}
static pr_save_handler_status save_handler_next(pr_save_handler_session*s,pr_save_handler_diagnostic*d,pr_store_return_status st){
    d->store_status=st;if(st==PR_STORE_RETURN_YIELD){s->phase=PR_SAVE_HANDLER_WAITING;return PR_SAVE_HANDLER_YIELD;}
    s->phase=PR_SAVE_HANDLER_FINISHED;if(st==PR_STORE_RETURN_HIGH)return PR_SAVE_HANDLER_HIGH;
    if(st==PR_STORE_RETURN_CLASS0)return PR_SAVE_HANDLER_CLASS0;
    if(st==PR_STORE_RETURN_CLASS1)return PR_SAVE_HANDLER_CLASS1;
    if(st==PR_STORE_RETURN_CLASS2)return PR_SAVE_HANDLER_CLASS2;
    if(st==PR_STORE_RETURN_CLASS3)return PR_SAVE_HANDLER_CLASS3;
    if(st==PR_STORE_RETURN_UNMATCHED)return PR_SAVE_HANDLER_UNMATCHED;
    if(st==PR_STORE_RETURN_ZERO)return PR_SAVE_HANDLER_ZERO;
    if(st==PR_STORE_RETURN_WORD)return PR_SAVE_HANDLER_WORD;
    if(st==PR_STORE_RETURN_OTHER_TYPE)return PR_SAVE_HANDLER_OTHER_TYPE;
    if(st==PR_STORE_RETURN_STRING)return PR_SAVE_HANDLER_STRING;
    if(st==PR_STORE_RETURN_LOADER_STRING)return PR_SAVE_HANDLER_LOADER_STRING;
    if(st==PR_STORE_RETURN_LIMIT)return PR_SAVE_HANDLER_LIMIT;
    if(st==PR_STORE_RETURN_PROMPT)return PR_SAVE_HANDLER_PROMPT;
    if(st==PR_STORE_RETURN_OTHER)return PR_SAVE_HANDLER_OTHER;
    if(st!=PR_STORE_RETURN_REACHED)return PR_SAVE_HANDLER_STOP;
    d->return_status=pr_u1c_save_handler_return(s->bus,PR_BUS_RAM_SIZE,s->cpu,&s->target);
    return d->return_status==PR_290B_FIRST_DONE?PR_SAVE_HANDLER_REACHED:PR_SAVE_HANDLER_STOP;
}
pr_save_handler_status pr_save_handler_start(pr_save_handler_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_save_handler_diagnostic*d){
    pr_store_return_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_SAVE_HANDLER_EMPTY||x->bp<56u)return PR_SAVE_HANDLER_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;s->target=(pr_290b_return_target){x->cs,0x2f89u,(uint16_t)(x->bp-0x10u),(uint16_t)(x->bp-0x10u),x->ss};
    st=pr_store_return_start(&s->store,b,n,c,e,cl,pl,x,max_iterations,&d->store);return save_handler_next(s,d,st);
}
pr_save_handler_status pr_save_handler_resume(pr_save_handler_session*s,pr_save_handler_diagnostic*d){pr_store_return_status st;
    if(!s||!d||s->phase!=PR_SAVE_HANDLER_WAITING)return PR_SAVE_HANDLER_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_store_return_resume(&s->store,&d->store);
    if(st==PR_STORE_RETURN_ARGUMENT)return PR_SAVE_HANDLER_ARGUMENT;
    return save_handler_next(s,d,st);
}

pr_290b_first_status pr_u1c_save_dispatch_return(pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_290b_return_target*x){
    if(!b||!b->ram||!b->ega||!c||!x||n!=PR_BUS_RAM_SIZE||c->ip!=0x2f89u||c->sp!=c->bp||c->sp>0xfffau||!ordinary(c->cs)||!ordinary(c->ds)||!ordinary(c->ss))return PR_290B_FIRST_ARGUMENT;
    c->bp=rw(b,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u);
    c->ip=rw(b,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u);
    c->cs=rw(b,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u);
    if(c->cs!=x->cs||c->ip!=x->ip||c->sp!=x->sp||c->bp!=x->bp||c->ss!=x->ss)return PR_290B_FIRST_TRANSFER;
    return PR_290B_FIRST_DONE;
}
void pr_dispatch_return_session_init(pr_dispatch_return_session*s){if(s){memset(s,0,sizeof *s);pr_save_handler_session_init(&s->save);}}
static pr_dispatch_return_status dispatch_return_next(pr_dispatch_return_session*s,pr_dispatch_return_diagnostic*d,pr_save_handler_status st){
    d->save_status=st;if(st==PR_SAVE_HANDLER_YIELD){s->phase=PR_DISPATCH_RETURN_WAITING;return PR_DISPATCH_RETURN_YIELD;}
    s->phase=PR_DISPATCH_RETURN_FINISHED;if(st==PR_SAVE_HANDLER_HIGH)return PR_DISPATCH_RETURN_HIGH;
    if(st==PR_SAVE_HANDLER_CLASS0)return PR_DISPATCH_RETURN_CLASS0;
    if(st==PR_SAVE_HANDLER_CLASS1)return PR_DISPATCH_RETURN_CLASS1;
    if(st==PR_SAVE_HANDLER_CLASS2)return PR_DISPATCH_RETURN_CLASS2;
    if(st==PR_SAVE_HANDLER_CLASS3)return PR_DISPATCH_RETURN_CLASS3;
    if(st==PR_SAVE_HANDLER_UNMATCHED)return PR_DISPATCH_RETURN_UNMATCHED;
    if(st==PR_SAVE_HANDLER_ZERO)return PR_DISPATCH_RETURN_ZERO;
    if(st==PR_SAVE_HANDLER_WORD)return PR_DISPATCH_RETURN_WORD;
    if(st==PR_SAVE_HANDLER_OTHER_TYPE)return PR_DISPATCH_RETURN_OTHER_TYPE;
    if(st==PR_SAVE_HANDLER_STRING)return PR_DISPATCH_RETURN_STRING;
    if(st==PR_SAVE_HANDLER_LOADER_STRING)return PR_DISPATCH_RETURN_LOADER_STRING;
    if(st==PR_SAVE_HANDLER_LIMIT)return PR_DISPATCH_RETURN_LIMIT;
    if(st==PR_SAVE_HANDLER_PROMPT)return PR_DISPATCH_RETURN_PROMPT;
    if(st==PR_SAVE_HANDLER_OTHER)return PR_DISPATCH_RETURN_OTHER;
    if(st!=PR_SAVE_HANDLER_REACHED)return PR_DISPATCH_RETURN_STOP;
    d->return_status=pr_u1c_save_dispatch_return(s->bus,PR_BUS_RAM_SIZE,s->cpu,&s->target);
    return d->return_status==PR_290B_FIRST_DONE?PR_DISPATCH_RETURN_REACHED:PR_DISPATCH_RETURN_STOP;
}
pr_dispatch_return_status pr_dispatch_return_start(pr_dispatch_return_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_dispatch_return_diagnostic*d){
    pr_save_handler_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_DISPATCH_RETURN_EMPTY||x->bp<56u)return PR_DISPATCH_RETURN_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;s->target=(pr_290b_return_target){x->cs,0x31a8u,(uint16_t)(x->bp-0x0au),(uint16_t)(x->bp-0x0au),x->ss};
    st=pr_save_handler_start(&s->save,b,n,c,e,cl,pl,x,max_iterations,&d->save);return dispatch_return_next(s,d,st);
}
pr_dispatch_return_status pr_dispatch_return_resume(pr_dispatch_return_session*s,pr_dispatch_return_diagnostic*d){pr_save_handler_status st;
    if(!s||!d||s->phase!=PR_DISPATCH_RETURN_WAITING)return PR_DISPATCH_RETURN_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_save_handler_resume(&s->save,&d->save);
    if(st==PR_SAVE_HANDLER_ARGUMENT)return PR_DISPATCH_RETURN_ARGUMENT;
    return dispatch_return_next(s,d,st);
}

pr_290b_first_status pr_u1c_runner_next(pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_290b_return_target*x){
    uint8_t v;uint16_t rel;
    if(!b||!b->ram||!b->ega||!c||!x||n!=PR_BUS_RAM_SIZE||c->ip!=0x31a8u||c->bp<4u||c->bp>0xfff8u||c->sp!=c->bp||!ordinary(c->cs)||!ordinary(c->ds)||!ordinary(c->ss))return PR_290B_FIRST_ARGUMENT;
    if(rb(b,c->cs,0x31a8u)!=0xebu||rb(b,c->cs,0x31a9u)!=0xd5u)return PR_290B_FIRST_CODE;
    c->ip=0x317fu;v=rb(b,c->ds,0x442eu);cmp8(c,v,0);
    if(!v){v=rb(b,c->ds,0x49ffu);cmp8(c,v,0);}
    if(v){wb(b,c->ds,0x442eu,0);c->bp=rw(b,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u);
        c->ip=rw(b,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u);c->cs=rw(b,c->ss,c->sp);c->sp=(uint16_t)(c->sp+4u);
        if(c->cs!=x->cs||c->ip!=x->ip||c->sp!=x->sp||c->bp!=x->bp||c->ss!=x->ss)return PR_290B_FIRST_TRANSFER;
        return PR_290B_FIRST_DONE;}
    c->ax=(uint16_t)((c->ax&0xff00u)|rb(b,c->ds,0x6f7fu));wb(b,c->ds,0x6f7eu,(uint8_t)c->ax);
    c->ax=rw(b,c->ds,0x49edu);c->di=rw(b,c->ds,0x49deu);c->es=rw(b,c->ds,0x49e0u);c->di=second_add(c,c->di,c->ax);
    c->ax=(uint16_t)((c->ax&0xff00u)|rb(b,c->es,(uint16_t)(c->di+0x6700u)));wb(b,c->ds,0x6f7fu,(uint8_t)c->ax);
    push(b,c,c->cs);c->ip=0x31a5u;rel=rw(b,c->cs,0x31a6u);
    if(rb(b,c->cs,0x31a5u)!=0xe8u||(uint16_t)(0x31a8u+rel)!=0x2f2du)return PR_290B_FIRST_CODE;
    push(b,c,0x31a8u);c->ip=0x2f2du;return PR_290B_FIRST_OUTSIDE;
}
void pr_runner_next_session_init(pr_runner_next_session*s){if(s){memset(s,0,sizeof *s);pr_dispatch_return_session_init(&s->dispatch);}}
static pr_runner_next_status runner_next_after(pr_runner_next_session*s,pr_runner_next_diagnostic*d,pr_dispatch_return_status st){
    d->dispatch_status=st;if(st==PR_DISPATCH_RETURN_YIELD){s->phase=PR_RUNNER_NEXT_WAITING;return PR_RUNNER_NEXT_YIELD;}
    s->phase=PR_RUNNER_NEXT_FINISHED;if(st==PR_DISPATCH_RETURN_HIGH)return PR_RUNNER_NEXT_HIGH;
    if(st==PR_DISPATCH_RETURN_CLASS0)return PR_RUNNER_NEXT_CLASS0;
    if(st==PR_DISPATCH_RETURN_CLASS1)return PR_RUNNER_NEXT_CLASS1;
    if(st==PR_DISPATCH_RETURN_CLASS2)return PR_RUNNER_NEXT_CLASS2;
    if(st==PR_DISPATCH_RETURN_CLASS3)return PR_RUNNER_NEXT_CLASS3;
    if(st==PR_DISPATCH_RETURN_UNMATCHED)return PR_RUNNER_NEXT_UNMATCHED;
    if(st==PR_DISPATCH_RETURN_ZERO)return PR_RUNNER_NEXT_ZERO;
    if(st==PR_DISPATCH_RETURN_WORD)return PR_RUNNER_NEXT_WORD;
    if(st==PR_DISPATCH_RETURN_OTHER_TYPE)return PR_RUNNER_NEXT_OTHER_TYPE;
    if(st==PR_DISPATCH_RETURN_STRING)return PR_RUNNER_NEXT_STRING;
    if(st==PR_DISPATCH_RETURN_LOADER_STRING)return PR_RUNNER_NEXT_LOADER_STRING;
    if(st==PR_DISPATCH_RETURN_LIMIT)return PR_RUNNER_NEXT_LIMIT;
    if(st==PR_DISPATCH_RETURN_PROMPT)return PR_RUNNER_NEXT_PROMPT;
    if(st==PR_DISPATCH_RETURN_OTHER)return PR_RUNNER_NEXT_OTHER;
    if(st!=PR_DISPATCH_RETURN_REACHED)return PR_RUNNER_NEXT_STOP;
    d->next_status=pr_u1c_runner_next(s->bus,PR_BUS_RAM_SIZE,s->cpu,&s->target);
    if(d->next_status==PR_290B_FIRST_OUTSIDE)return PR_RUNNER_NEXT_DISPATCH;
    return d->next_status==PR_290B_FIRST_DONE?PR_RUNNER_NEXT_RETURNED:PR_RUNNER_NEXT_STOP;
}
pr_runner_next_status pr_runner_next_start(pr_runner_next_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_runner_next_diagnostic*d){
    pr_dispatch_return_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_RUNNER_NEXT_EMPTY||x->bp<56u)return PR_RUNNER_NEXT_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;s->target=*x;s->target.ip=0x345bu;
    st=pr_dispatch_return_start(&s->dispatch,b,n,c,e,cl,pl,x,max_iterations,&d->dispatch);return runner_next_after(s,d,st);
}
pr_runner_next_status pr_runner_next_resume(pr_runner_next_session*s,pr_runner_next_diagnostic*d){pr_dispatch_return_status st;
    if(!s||!d||s->phase!=PR_RUNNER_NEXT_WAITING)return PR_RUNNER_NEXT_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_dispatch_return_resume(&s->dispatch,&d->dispatch);
    if(st==PR_DISPATCH_RETURN_ARGUMENT)return PR_RUNNER_NEXT_ARGUMENT;
    return runner_next_after(s,d,st);
}

void pr_next_dispatch_session_init(pr_next_dispatch_session*s){if(s){memset(s,0,sizeof *s);pr_runner_next_session_init(&s->runner);}}
static pr_next_dispatch_status next_dispatch_after(pr_next_dispatch_session*s,pr_next_dispatch_diagnostic*d,pr_runner_next_status st){
    d->runner_status=st;if(st==PR_RUNNER_NEXT_YIELD){s->phase=PR_NEXT_DISPATCH_WAITING;return PR_NEXT_DISPATCH_YIELD;}
    s->phase=PR_NEXT_DISPATCH_FINISHED;if(st==PR_RUNNER_NEXT_RETURNED)return PR_NEXT_DISPATCH_RETURNED;
    if(st==PR_RUNNER_NEXT_HIGH)return PR_NEXT_DISPATCH_HIGH;
    if(st==PR_RUNNER_NEXT_CLASS0)return PR_NEXT_DISPATCH_CLASS0;
    if(st==PR_RUNNER_NEXT_CLASS1)return PR_NEXT_DISPATCH_CLASS1;
    if(st==PR_RUNNER_NEXT_CLASS2)return PR_NEXT_DISPATCH_CLASS2;
    if(st==PR_RUNNER_NEXT_CLASS3)return PR_NEXT_DISPATCH_CLASS3;
    if(st==PR_RUNNER_NEXT_UNMATCHED)return PR_NEXT_DISPATCH_UNMATCHED;
    if(st==PR_RUNNER_NEXT_ZERO)return PR_NEXT_DISPATCH_ZERO;
    if(st==PR_RUNNER_NEXT_WORD)return PR_NEXT_DISPATCH_WORD;
    if(st==PR_RUNNER_NEXT_OTHER_TYPE)return PR_NEXT_DISPATCH_OTHER_TYPE;
    if(st==PR_RUNNER_NEXT_STRING)return PR_NEXT_DISPATCH_STRING;
    if(st==PR_RUNNER_NEXT_LOADER_STRING)return PR_NEXT_DISPATCH_LOADER_STRING;
    if(st==PR_RUNNER_NEXT_LIMIT)return PR_NEXT_DISPATCH_LIMIT;
    if(st==PR_RUNNER_NEXT_PROMPT)return PR_NEXT_DISPATCH_PROMPT;
    if(st==PR_RUNNER_NEXT_OTHER)return PR_NEXT_DISPATCH_OTHER;
    if(st!=PR_RUNNER_NEXT_DISPATCH)return PR_NEXT_DISPATCH_STOP;
    d->dispatch_status=pr_u1c_ecl_dispatch_prefix(s->bus,PR_BUS_RAM_SIZE,s->cpu);
    if(d->dispatch_status==PR_290B_FIRST_OUTSIDE)return PR_NEXT_DISPATCH_LATER;
    return d->dispatch_status==PR_290B_FIRST_DONE?PR_NEXT_DISPATCH_HANDLER:PR_NEXT_DISPATCH_STOP;
}
pr_next_dispatch_status pr_next_dispatch_start(pr_next_dispatch_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_next_dispatch_diagnostic*d){
    pr_runner_next_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_NEXT_DISPATCH_EMPTY||s->runner.phase!=PR_RUNNER_NEXT_EMPTY||x->bp<56u)return PR_NEXT_DISPATCH_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_runner_next_start(&s->runner,b,n,c,e,cl,pl,x,max_iterations,&d->runner);return next_dispatch_after(s,d,st);
}
pr_next_dispatch_status pr_next_dispatch_resume(pr_next_dispatch_session*s,pr_next_dispatch_diagnostic*d){pr_runner_next_status st;
    if(!s||!d||s->phase!=PR_NEXT_DISPATCH_WAITING)return PR_NEXT_DISPATCH_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_runner_next_resume(&s->runner,&d->runner);
    if(st==PR_RUNNER_NEXT_ARGUMENT)return PR_NEXT_DISPATCH_ARGUMENT;
    return next_dispatch_after(s,d,st);
}

void pr_second_save_session_init(pr_second_save_session*s){if(s){memset(s,0,sizeof *s);pr_next_dispatch_session_init(&s->next);}}
static pr_second_save_status second_save_after(pr_second_save_session*s,pr_second_save_diagnostic*d,pr_next_dispatch_status st){
    d->next_status=st;if(st==PR_NEXT_DISPATCH_YIELD){s->phase=PR_SECOND_SAVE_WAITING;return PR_SECOND_SAVE_YIELD;}
    s->phase=PR_SECOND_SAVE_FINISHED;if(st==PR_NEXT_DISPATCH_LATER)return PR_SECOND_SAVE_LATER;
    if(st==PR_NEXT_DISPATCH_RETURNED)return PR_SECOND_SAVE_RETURNED;
    if(st==PR_NEXT_DISPATCH_HIGH)return PR_SECOND_SAVE_HIGH;
    if(st==PR_NEXT_DISPATCH_CLASS0)return PR_SECOND_SAVE_CLASS0;
    if(st==PR_NEXT_DISPATCH_CLASS1)return PR_SECOND_SAVE_CLASS1;
    if(st==PR_NEXT_DISPATCH_CLASS2)return PR_SECOND_SAVE_CLASS2;
    if(st==PR_NEXT_DISPATCH_CLASS3)return PR_SECOND_SAVE_CLASS3;
    if(st==PR_NEXT_DISPATCH_UNMATCHED)return PR_SECOND_SAVE_UNMATCHED;
    if(st==PR_NEXT_DISPATCH_ZERO)return PR_SECOND_SAVE_ZERO;
    if(st==PR_NEXT_DISPATCH_WORD)return PR_SECOND_SAVE_WORD;
    if(st==PR_NEXT_DISPATCH_OTHER_TYPE)return PR_SECOND_SAVE_OTHER_TYPE;
    if(st==PR_NEXT_DISPATCH_STRING)return PR_SECOND_SAVE_STRING;
    if(st==PR_NEXT_DISPATCH_LOADER_STRING)return PR_SECOND_SAVE_LOADER_STRING;
    if(st==PR_NEXT_DISPATCH_LIMIT)return PR_SECOND_SAVE_LIMIT;
    if(st==PR_NEXT_DISPATCH_PROMPT)return PR_SECOND_SAVE_PROMPT;
    if(st==PR_NEXT_DISPATCH_OTHER)return PR_SECOND_SAVE_OTHER;
    if(st!=PR_NEXT_DISPATCH_HANDLER)return PR_SECOND_SAVE_STOP;
    if(s->cpu->ip!=0x0292u)return PR_SECOND_SAVE_OTHER_HANDLER;
    d->entry_status=pr_u1c_save_loader_entry(s->bus,PR_BUS_RAM_SIZE,s->cpu,&d->entry);
    return d->entry_status==PR_290B_FIRST_DONE?PR_SECOND_SAVE_LOADER:PR_SECOND_SAVE_STOP;
}
pr_second_save_status pr_second_save_start(pr_second_save_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_second_save_diagnostic*d){
    pr_next_dispatch_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_SECOND_SAVE_EMPTY||s->next.phase!=PR_NEXT_DISPATCH_EMPTY||s->next.runner.phase!=PR_RUNNER_NEXT_EMPTY||x->bp<56u)return PR_SECOND_SAVE_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_next_dispatch_start(&s->next,b,n,c,e,cl,pl,x,max_iterations,&d->next);return second_save_after(s,d,st);
}
pr_second_save_status pr_second_save_resume(pr_second_save_session*s,pr_second_save_diagnostic*d){pr_next_dispatch_status st;
    if(!s||!d||s->phase!=PR_SECOND_SAVE_WAITING)return PR_SECOND_SAVE_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_next_dispatch_resume(&s->next,&d->next);
    if(st==PR_NEXT_DISPATCH_ARGUMENT)return PR_SECOND_SAVE_ARGUMENT;
    return second_save_after(s,d,st);
}

void pr_second_loader_session_init(pr_second_loader_session*s){if(s){memset(s,0,sizeof *s);pr_second_save_session_init(&s->save);}}
static pr_second_loader_status second_loader_after(pr_second_loader_session*s,pr_second_loader_diagnostic*d,pr_second_save_status st){
    d->save_status=st;if(st==PR_SECOND_SAVE_YIELD){s->phase=PR_SECOND_LOADER_WAITING;return PR_SECOND_LOADER_YIELD;}
    s->phase=PR_SECOND_LOADER_FINISHED;if(st==PR_SECOND_SAVE_OTHER_HANDLER)return PR_SECOND_LOADER_OTHER_HANDLER;
    if(st==PR_SECOND_SAVE_LATER)return PR_SECOND_LOADER_LATER;
    if(st==PR_SECOND_SAVE_RETURNED)return PR_SECOND_LOADER_RETURNED;
    if(st==PR_SECOND_SAVE_HIGH)return PR_SECOND_LOADER_HIGH;
    if(st==PR_SECOND_SAVE_CLASS0)return PR_SECOND_LOADER_CLASS0;
    if(st==PR_SECOND_SAVE_CLASS1)return PR_SECOND_LOADER_CLASS1;
    if(st==PR_SECOND_SAVE_CLASS2)return PR_SECOND_LOADER_CLASS2;
    if(st==PR_SECOND_SAVE_CLASS3)return PR_SECOND_LOADER_CLASS3;
    if(st==PR_SECOND_SAVE_UNMATCHED)return PR_SECOND_LOADER_UNMATCHED;
    if(st==PR_SECOND_SAVE_ZERO)return PR_SECOND_LOADER_ZERO;
    if(st==PR_SECOND_SAVE_WORD)return PR_SECOND_LOADER_WORD;
    if(st==PR_SECOND_SAVE_OTHER_TYPE)return PR_SECOND_LOADER_OTHER_TYPE;
    if(st==PR_SECOND_SAVE_STRING)return PR_SECOND_LOADER_STRING;
    if(st==PR_SECOND_SAVE_LOADER_STRING)return PR_SECOND_LOADER_FIRST_STRING;
    if(st==PR_SECOND_SAVE_LIMIT)return PR_SECOND_LOADER_FIRST_LIMIT;
    if(st==PR_SECOND_SAVE_PROMPT)return PR_SECOND_LOADER_PROMPT;
    if(st==PR_SECOND_SAVE_OTHER)return PR_SECOND_LOADER_OTHER;
    if(st!=PR_SECOND_SAVE_LOADER)return PR_SECOND_LOADER_STOP;
    d->prefix_status=pr_u25_operand_prefix(s->bus,PR_BUS_RAM_SIZE,s->cpu);
    if(d->prefix_status==PR_290B_FIRST_DONE){
        d->tag_status=pr_u25_operand_tag(s->bus,PR_BUS_RAM_SIZE,s->cpu);
        if(d->tag_status==PR_290B_FIRST_OUTSIDE)return PR_SECOND_LOADER_SECOND_STRING;
        if(d->tag_status!=PR_290B_FIRST_DONE)return PR_SECOND_LOADER_STOP;
        d->loop_status=pr_u25_numeric_loop(s->bus,PR_BUS_RAM_SIZE,s->cpu,s->max_iterations,&d->loop);
        if(d->loop_status==PR_U25_LOOP_STEP_STRING)return PR_SECOND_LOADER_SECOND_STRING;
        if(d->loop_status==PR_U25_LOOP_STEP_LIMIT)return PR_SECOND_LOADER_SECOND_LIMIT;
        if(d->loop_status!=PR_U25_LOOP_STEP_DONE)return PR_SECOND_LOADER_STOP;
    }else if(d->prefix_status!=PR_290B_FIRST_OUTSIDE||s->cpu->ip!=0x012cu)return PR_SECOND_LOADER_STOP;
    d->return_status=pr_u25_operand_return(s->bus,PR_BUS_RAM_SIZE,s->cpu,&s->target);
    return d->return_status==PR_290B_FIRST_DONE?PR_SECOND_LOADER_REACHED:PR_SECOND_LOADER_STOP;
}
pr_second_loader_status pr_second_loader_start(pr_second_loader_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_second_loader_diagnostic*d){
    pr_second_save_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_SECOND_LOADER_EMPTY||s->save.phase!=PR_SECOND_SAVE_EMPTY||s->save.next.phase!=PR_NEXT_DISPATCH_EMPTY||s->save.next.runner.phase!=PR_RUNNER_NEXT_EMPTY||x->bp<56u)return PR_SECOND_LOADER_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;s->max_iterations=max_iterations;s->target=(pr_290b_return_target){x->cs,0x02a0u,(uint16_t)(x->bp-0x1au),(uint16_t)(x->bp-0x16u),x->ss};
    st=pr_second_save_start(&s->save,b,n,c,e,cl,pl,x,max_iterations,&d->save);return second_loader_after(s,d,st);
}
pr_second_loader_status pr_second_loader_resume(pr_second_loader_session*s,pr_second_loader_diagnostic*d){pr_second_save_status st;
    if(!s||!d||s->phase!=PR_SECOND_LOADER_WAITING)return PR_SECOND_LOADER_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_second_save_resume(&s->save,&d->save);
    if(st==PR_SECOND_SAVE_ARGUMENT)return PR_SECOND_LOADER_ARGUMENT;
    return second_loader_after(s,d,st);
}

void pr_second_branch_session_init(pr_second_branch_session*s){if(s){memset(s,0,sizeof *s);pr_second_loader_session_init(&s->loader);}}
static pr_second_branch_status second_branch_after(pr_second_branch_session*s,pr_second_branch_diagnostic*d,pr_second_loader_status st){
    pr_290b_return_target x;
    d->loader_status=st;if(st==PR_SECOND_LOADER_YIELD){s->phase=PR_SECOND_BRANCH_WAITING;return PR_SECOND_BRANCH_YIELD;}
    s->phase=PR_SECOND_BRANCH_FINISHED;if(st==PR_SECOND_LOADER_SECOND_STRING)return PR_SECOND_BRANCH_OPERAND_STRING;
    if(st==PR_SECOND_LOADER_SECOND_LIMIT)return PR_SECOND_BRANCH_SECOND_LIMIT;
    if(st==PR_SECOND_LOADER_OTHER_HANDLER)return PR_SECOND_BRANCH_OTHER_HANDLER;
    if(st==PR_SECOND_LOADER_LATER)return PR_SECOND_BRANCH_LATER;
    if(st==PR_SECOND_LOADER_RETURNED)return PR_SECOND_BRANCH_RETURNED;
    if(st==PR_SECOND_LOADER_HIGH)return PR_SECOND_BRANCH_HIGH;
    if(st==PR_SECOND_LOADER_CLASS0)return PR_SECOND_BRANCH_CLASS0;
    if(st==PR_SECOND_LOADER_CLASS1)return PR_SECOND_BRANCH_CLASS1;
    if(st==PR_SECOND_LOADER_CLASS2)return PR_SECOND_BRANCH_CLASS2;
    if(st==PR_SECOND_LOADER_CLASS3)return PR_SECOND_BRANCH_CLASS3;
    if(st==PR_SECOND_LOADER_UNMATCHED)return PR_SECOND_BRANCH_UNMATCHED;
    if(st==PR_SECOND_LOADER_ZERO)return PR_SECOND_BRANCH_ZERO;
    if(st==PR_SECOND_LOADER_WORD)return PR_SECOND_BRANCH_WORD;
    if(st==PR_SECOND_LOADER_OTHER_TYPE)return PR_SECOND_BRANCH_OTHER_TYPE;
    if(st==PR_SECOND_LOADER_STRING)return PR_SECOND_BRANCH_FIRST_SAVE_STRING;
    if(st==PR_SECOND_LOADER_FIRST_STRING)return PR_SECOND_BRANCH_FIRST_OPERAND_STRING;
    if(st==PR_SECOND_LOADER_FIRST_LIMIT)return PR_SECOND_BRANCH_FIRST_LIMIT;
    if(st==PR_SECOND_LOADER_PROMPT)return PR_SECOND_BRANCH_PROMPT;
    if(st==PR_SECOND_LOADER_OTHER)return PR_SECOND_BRANCH_OTHER;
    if(st!=PR_SECOND_LOADER_REACHED)return PR_SECOND_BRANCH_STOP;
    x=(pr_290b_return_target){s->cpu->cs,0x02adu,s->cpu->sp,s->cpu->bp,s->cpu->ss};
    d->entry_status=pr_u1c_save_word_entry(s->bus,PR_BUS_RAM_SIZE,s->cpu,&d->entry);
    if(d->entry_status!=PR_290B_FIRST_DONE)return PR_SECOND_BRANCH_STOP;
    d->word_status=pr_u25_word_return(s->bus,PR_BUS_RAM_SIZE,s->cpu,&x);
    if(d->word_status!=PR_290B_FIRST_DONE)return PR_SECOND_BRANCH_STOP;
    d->branch_status=pr_u1c_save_tag_branch(s->bus,PR_BUS_RAM_SIZE,s->cpu);
    if(d->branch_status!=PR_290B_FIRST_DONE)return PR_SECOND_BRANCH_STOP;
    return s->cpu->ip==0x02cfu?PR_SECOND_BRANCH_SAVE_STRING:PR_SECOND_BRANCH_NUMERIC;
}
pr_second_branch_status pr_second_branch_start(pr_second_branch_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_second_branch_diagnostic*d){
    pr_second_loader_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_SECOND_BRANCH_EMPTY||s->loader.phase!=PR_SECOND_LOADER_EMPTY||s->loader.save.phase!=PR_SECOND_SAVE_EMPTY||s->loader.save.next.phase!=PR_NEXT_DISPATCH_EMPTY||s->loader.save.next.runner.phase!=PR_RUNNER_NEXT_EMPTY||x->bp<56u)return PR_SECOND_BRANCH_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_second_loader_start(&s->loader,b,n,c,e,cl,pl,x,max_iterations,&d->loader);return second_branch_after(s,d,st);
}
pr_second_branch_status pr_second_branch_resume(pr_second_branch_session*s,pr_second_branch_diagnostic*d){pr_second_loader_status st;
    if(!s||!d||s->phase!=PR_SECOND_BRANCH_WAITING)return PR_SECOND_BRANCH_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_second_loader_resume(&s->loader,&d->loader);
    if(st==PR_SECOND_LOADER_ARGUMENT)return PR_SECOND_BRANCH_ARGUMENT;
    return second_branch_after(s,d,st);
}

void pr_second_slot_session_init(pr_second_slot_session*s){if(s){memset(s,0,sizeof *s);pr_second_branch_session_init(&s->branch);}}
static pr_second_slot_status second_slot_after(pr_second_slot_session*s,pr_second_slot_diagnostic*d,pr_second_branch_status st){
    d->branch_status=st;if(st==PR_SECOND_BRANCH_YIELD){s->phase=PR_SECOND_SLOT_WAITING;return PR_SECOND_SLOT_YIELD;}
    s->phase=PR_SECOND_SLOT_FINISHED;if(st==PR_SECOND_BRANCH_SAVE_STRING)return PR_SECOND_SLOT_SAVE_STRING;
    if(st==PR_SECOND_BRANCH_OPERAND_STRING)return PR_SECOND_SLOT_OPERAND_STRING;
    if(st==PR_SECOND_BRANCH_SECOND_LIMIT)return PR_SECOND_SLOT_SECOND_LIMIT;
    if(st==PR_SECOND_BRANCH_OTHER_HANDLER)return PR_SECOND_SLOT_OTHER_HANDLER;
    if(st==PR_SECOND_BRANCH_LATER)return PR_SECOND_SLOT_LATER;
    if(st==PR_SECOND_BRANCH_RETURNED)return PR_SECOND_SLOT_RETURNED;
    if(st==PR_SECOND_BRANCH_HIGH)return PR_SECOND_SLOT_HIGH;
    if(st==PR_SECOND_BRANCH_CLASS0)return PR_SECOND_SLOT_CLASS0;
    if(st==PR_SECOND_BRANCH_CLASS1)return PR_SECOND_SLOT_CLASS1;
    if(st==PR_SECOND_BRANCH_CLASS2)return PR_SECOND_SLOT_CLASS2;
    if(st==PR_SECOND_BRANCH_CLASS3)return PR_SECOND_SLOT_CLASS3;
    if(st==PR_SECOND_BRANCH_UNMATCHED)return PR_SECOND_SLOT_UNMATCHED;
    if(st==PR_SECOND_BRANCH_ZERO)return PR_SECOND_SLOT_FIRST_ZERO;
    if(st==PR_SECOND_BRANCH_WORD)return PR_SECOND_SLOT_FIRST_WORD;
    if(st==PR_SECOND_BRANCH_OTHER_TYPE)return PR_SECOND_SLOT_FIRST_OTHER_TYPE;
    if(st==PR_SECOND_BRANCH_FIRST_SAVE_STRING)return PR_SECOND_SLOT_FIRST_SAVE_STRING;
    if(st==PR_SECOND_BRANCH_FIRST_OPERAND_STRING)return PR_SECOND_SLOT_FIRST_OPERAND_STRING;
    if(st==PR_SECOND_BRANCH_FIRST_LIMIT)return PR_SECOND_SLOT_FIRST_LIMIT;
    if(st==PR_SECOND_BRANCH_PROMPT)return PR_SECOND_SLOT_PROMPT;
    if(st==PR_SECOND_BRANCH_OTHER)return PR_SECOND_SLOT_OTHER;
    if(st!=PR_SECOND_BRANCH_NUMERIC)return PR_SECOND_SLOT_STOP;
    d->entry_status=pr_u1c_save_slot_entry(s->bus,PR_BUS_RAM_SIZE,s->cpu,&d->entry);
    if(d->entry_status!=PR_290B_FIRST_DONE)return PR_SECOND_SLOT_STOP;
    d->prefix_status=pr_u25_slot_prefix(s->bus,PR_BUS_RAM_SIZE,s->cpu);
    if(d->prefix_status!=PR_290B_FIRST_DONE)return PR_SECOND_SLOT_STOP;
    d->dispatch_status=pr_u25_slot_dispatch(s->bus,PR_BUS_RAM_SIZE,s->cpu);
    if(d->dispatch_status!=PR_290B_FIRST_DONE)return PR_SECOND_SLOT_STOP;
    if(s->cpu->ip==0x0164u)return PR_SECOND_SLOT_ZERO;
    if(s->cpu->ip==0x017au)return PR_SECOND_SLOT_WORD;
    if(s->cpu->ip==0x019eu)return PR_SECOND_SLOT_PAIR;
    return PR_SECOND_SLOT_OTHER_TYPE;
}
pr_second_slot_status pr_second_slot_start(pr_second_slot_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_second_slot_diagnostic*d){
    pr_second_branch_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_SECOND_SLOT_EMPTY||s->branch.phase!=PR_SECOND_BRANCH_EMPTY||s->branch.loader.phase!=PR_SECOND_LOADER_EMPTY||s->branch.loader.save.phase!=PR_SECOND_SAVE_EMPTY||s->branch.loader.save.next.phase!=PR_NEXT_DISPATCH_EMPTY||s->branch.loader.save.next.runner.phase!=PR_RUNNER_NEXT_EMPTY||x->bp<56u)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_second_branch_start(&s->branch,b,n,c,e,cl,pl,x,max_iterations,&d->branch);return second_slot_after(s,d,st);
}
pr_second_slot_status pr_second_slot_resume(pr_second_slot_session*s,pr_second_slot_diagnostic*d){pr_second_branch_status st;
    if(!s||!d||s->phase!=PR_SECOND_SLOT_WAITING)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_second_branch_resume(&s->branch,&d->branch);
    if(st==PR_SECOND_BRANCH_ARGUMENT)return PR_SECOND_SLOT_ARGUMENT;
    return second_slot_after(s,d,st);
}

void pr_second_pair_session_init(pr_second_pair_session*s){if(s){memset(s,0,sizeof *s);pr_second_slot_session_init(&s->slot);}}
static pr_second_slot_status second_pair_after(pr_second_pair_session*s,pr_second_pair_diagnostic*d,pr_second_slot_status st){
    d->slot_status=st;if(st==PR_SECOND_SLOT_YIELD){s->phase=PR_SECOND_PAIR_WAITING;return st;}
    s->phase=PR_SECOND_PAIR_FINISHED;if(st!=PR_SECOND_SLOT_PAIR)return st;
    d->entry_status=pr_u25_slot_pair_entry(s->bus,PR_BUS_RAM_SIZE,s->cpu,&d->entry);
    return d->entry_status==PR_290B_FIRST_DONE?PR_SECOND_SLOT_PAIR:PR_SECOND_SLOT_STOP;
}
pr_second_slot_status pr_second_pair_start(pr_second_pair_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_second_pair_diagnostic*d){
    pr_second_slot_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_SECOND_PAIR_EMPTY||s->slot.phase!=PR_SECOND_SLOT_EMPTY||s->slot.branch.phase!=PR_SECOND_BRANCH_EMPTY||s->slot.branch.loader.phase!=PR_SECOND_LOADER_EMPTY||s->slot.branch.loader.save.phase!=PR_SECOND_SAVE_EMPTY||s->slot.branch.loader.save.next.phase!=PR_NEXT_DISPATCH_EMPTY||s->slot.branch.loader.save.next.runner.phase!=PR_RUNNER_NEXT_EMPTY||x->bp<56u)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_second_slot_start(&s->slot,b,n,c,e,cl,pl,x,max_iterations,&d->slot);return second_pair_after(s,d,st);
}
pr_second_slot_status pr_second_pair_resume(pr_second_pair_session*s,pr_second_pair_diagnostic*d){pr_second_slot_status st;
    if(!s||!d||s->phase!=PR_SECOND_PAIR_WAITING)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_second_slot_resume(&s->slot,&d->slot);
    if(st==PR_SECOND_SLOT_ARGUMENT)return st;
    return second_pair_after(s,d,st);
}

void pr_second_pair_return_session_init(pr_second_pair_return_session*s){if(s){memset(s,0,sizeof *s);pr_second_pair_session_init(&s->pair);}}
static pr_second_slot_status second_pair_return_after(pr_second_pair_return_session*s,pr_second_pair_return_diagnostic*d,pr_second_slot_status st){
    pr_290b_return_target x;d->pair_status=st;if(st==PR_SECOND_SLOT_YIELD){s->phase=PR_SECOND_PAIR_RETURN_WAITING;return st;}
    s->phase=PR_SECOND_PAIR_RETURN_FINISHED;if(st!=PR_SECOND_SLOT_PAIR)return st;
    x=(pr_290b_return_target){s->cpu->cs,0x01aau,(uint16_t)(s->cpu->bp-10u),s->cpu->bp,s->cpu->ss};
    d->return_status=pr_u25_word_return(s->bus,PR_BUS_RAM_SIZE,s->cpu,&x);
    return d->return_status==PR_290B_FIRST_DONE?PR_SECOND_SLOT_PAIR:PR_SECOND_SLOT_STOP;
}
pr_second_slot_status pr_second_pair_return_start(pr_second_pair_return_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_second_pair_return_diagnostic*d){
    pr_second_slot_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_SECOND_PAIR_RETURN_EMPTY||s->pair.phase!=PR_SECOND_PAIR_EMPTY||s->pair.slot.phase!=PR_SECOND_SLOT_EMPTY||s->pair.slot.branch.phase!=PR_SECOND_BRANCH_EMPTY||s->pair.slot.branch.loader.phase!=PR_SECOND_LOADER_EMPTY||s->pair.slot.branch.loader.save.phase!=PR_SECOND_SAVE_EMPTY||s->pair.slot.branch.loader.save.next.phase!=PR_NEXT_DISPATCH_EMPTY||s->pair.slot.branch.loader.save.next.runner.phase!=PR_RUNNER_NEXT_EMPTY||x->bp<56u)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_second_pair_start(&s->pair,b,n,c,e,cl,pl,x,max_iterations,&d->pair);return second_pair_return_after(s,d,st);
}
pr_second_slot_status pr_second_pair_return_resume(pr_second_pair_return_session*s,pr_second_pair_return_diagnostic*d){pr_second_slot_status st;
    if(!s||!d||s->phase!=PR_SECOND_PAIR_RETURN_WAITING)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_second_pair_resume(&s->pair,&d->pair);
    if(st==PR_SECOND_SLOT_ARGUMENT)return st;
    return second_pair_return_after(s,d,st);
}

void pr_second_value_session_init(pr_second_value_session*s){if(s){memset(s,0,sizeof *s);pr_second_pair_return_session_init(&s->pair);}}
static pr_second_slot_status second_value_after(pr_second_value_session*s,pr_second_value_diagnostic*d,pr_second_slot_status st){
    d->pair_status=st;if(st==PR_SECOND_SLOT_YIELD){s->phase=PR_SECOND_VALUE_WAITING;return st;}
    s->phase=PR_SECOND_VALUE_FINISHED;if(st!=PR_SECOND_SLOT_PAIR)return st;
    d->return_status=pr_u25_slot_value_return(s->bus,PR_BUS_RAM_SIZE,s->cpu,&s->target);
    return d->return_status==PR_290B_FIRST_DONE?PR_SECOND_SLOT_PAIR:PR_SECOND_SLOT_STOP;
}
pr_second_slot_status pr_second_value_start(pr_second_value_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_second_value_diagnostic*d){
    pr_second_slot_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_SECOND_VALUE_EMPTY||s->pair.phase!=PR_SECOND_PAIR_RETURN_EMPTY||s->pair.pair.phase!=PR_SECOND_PAIR_EMPTY||s->pair.pair.slot.phase!=PR_SECOND_SLOT_EMPTY||s->pair.pair.slot.branch.phase!=PR_SECOND_BRANCH_EMPTY||s->pair.pair.slot.branch.loader.phase!=PR_SECOND_LOADER_EMPTY||s->pair.pair.slot.branch.loader.save.phase!=PR_SECOND_SAVE_EMPTY||s->pair.pair.slot.branch.loader.save.next.phase!=PR_NEXT_DISPATCH_EMPTY||s->pair.pair.slot.branch.loader.save.next.runner.phase!=PR_RUNNER_NEXT_EMPTY||x->bp<56u)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;s->target=(pr_290b_return_target){x->cs,0x02bfu,(uint16_t)(x->bp-0x1au),(uint16_t)(x->bp-0x16u),x->ss};
    st=pr_second_pair_return_start(&s->pair,b,n,c,e,cl,pl,x,max_iterations,&d->pair);return second_value_after(s,d,st);
}
pr_second_slot_status pr_second_value_resume(pr_second_value_session*s,pr_second_value_diagnostic*d){pr_second_slot_status st;
    if(!s||!d||s->phase!=PR_SECOND_VALUE_WAITING)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_second_pair_return_resume(&s->pair,&d->pair);
    if(st==PR_SECOND_SLOT_ARGUMENT)return st;
    return second_value_after(s,d,st);
}

void pr_second_store_session_init(pr_second_store_session*s){if(s){memset(s,0,sizeof *s);pr_second_value_session_init(&s->value);}}
static pr_second_slot_status second_store_after(pr_second_store_session*s,pr_second_store_diagnostic*d,pr_second_slot_status st){
    d->value_status=st;if(st==PR_SECOND_SLOT_YIELD){s->phase=PR_SECOND_STORE_WAITING;return st;}
    s->phase=PR_SECOND_STORE_FINISHED;if(st!=PR_SECOND_SLOT_PAIR)return st;
    d->entry_status=pr_u1c_save_store_entry(s->bus,PR_BUS_RAM_SIZE,s->cpu,&d->entry);
    return d->entry_status==PR_290B_FIRST_DONE?PR_SECOND_SLOT_PAIR:PR_SECOND_SLOT_STOP;
}
pr_second_slot_status pr_second_store_start(pr_second_store_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_second_store_diagnostic*d){
    pr_second_slot_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_SECOND_STORE_EMPTY||s->value.phase!=PR_SECOND_VALUE_EMPTY||s->value.pair.phase!=PR_SECOND_PAIR_RETURN_EMPTY||s->value.pair.pair.phase!=PR_SECOND_PAIR_EMPTY||s->value.pair.pair.slot.phase!=PR_SECOND_SLOT_EMPTY||s->value.pair.pair.slot.branch.phase!=PR_SECOND_BRANCH_EMPTY||s->value.pair.pair.slot.branch.loader.phase!=PR_SECOND_LOADER_EMPTY||s->value.pair.pair.slot.branch.loader.save.phase!=PR_SECOND_SAVE_EMPTY||s->value.pair.pair.slot.branch.loader.save.next.phase!=PR_NEXT_DISPATCH_EMPTY||s->value.pair.pair.slot.branch.loader.save.next.runner.phase!=PR_RUNNER_NEXT_EMPTY||x->bp<56u)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_second_value_start(&s->value,b,n,c,e,cl,pl,x,max_iterations,&d->value);return second_store_after(s,d,st);
}
pr_second_slot_status pr_second_store_resume(pr_second_store_session*s,pr_second_store_diagnostic*d){pr_second_slot_status st;
    if(!s||!d||s->phase!=PR_SECOND_STORE_WAITING)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_second_value_resume(&s->value,&d->value);
    if(st==PR_SECOND_SLOT_ARGUMENT)return st;
    return second_store_after(s,d,st);
}

void pr_second_class_session_init(pr_second_class_session*s){if(s){memset(s,0,sizeof *s);pr_second_store_session_init(&s->store);}}
static pr_second_slot_status second_class_after(pr_second_class_session*s,pr_second_class_diagnostic*d,pr_second_slot_status st){
    d->store_status=st;if(st==PR_SECOND_SLOT_YIELD){s->phase=PR_SECOND_CLASS_WAITING;return st;}
    s->phase=PR_SECOND_CLASS_FINISHED;if(st!=PR_SECOND_SLOT_PAIR)return st;
    d->entry_status=pr_u25_store_classifier_entry(s->bus,PR_BUS_RAM_SIZE,s->cpu,&d->entry);
    return d->entry_status==PR_290B_FIRST_DONE?PR_SECOND_SLOT_PAIR:PR_SECOND_SLOT_STOP;
}
pr_second_slot_status pr_second_class_start(pr_second_class_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_second_class_diagnostic*d){
    pr_second_slot_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_SECOND_CLASS_EMPTY||s->store.phase!=PR_SECOND_STORE_EMPTY||s->store.value.phase!=PR_SECOND_VALUE_EMPTY||s->store.value.pair.phase!=PR_SECOND_PAIR_RETURN_EMPTY||s->store.value.pair.pair.phase!=PR_SECOND_PAIR_EMPTY||s->store.value.pair.pair.slot.phase!=PR_SECOND_SLOT_EMPTY||s->store.value.pair.pair.slot.branch.phase!=PR_SECOND_BRANCH_EMPTY||s->store.value.pair.pair.slot.branch.loader.phase!=PR_SECOND_LOADER_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.phase!=PR_SECOND_SAVE_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.next.phase!=PR_NEXT_DISPATCH_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.next.runner.phase!=PR_RUNNER_NEXT_EMPTY||x->bp<56u)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_second_store_start(&s->store,b,n,c,e,cl,pl,x,max_iterations,&d->store);return second_class_after(s,d,st);
}
pr_second_slot_status pr_second_class_resume(pr_second_class_session*s,pr_second_class_diagnostic*d){pr_second_slot_status st;
    if(!s||!d||s->phase!=PR_SECOND_CLASS_WAITING)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_second_store_resume(&s->store,&d->store);
    if(st==PR_SECOND_SLOT_ARGUMENT)return st;
    return second_class_after(s,d,st);
}

void pr_second_class_return_session_init(pr_second_class_return_session*s){if(s){memset(s,0,sizeof *s);pr_second_class_session_init(&s->classifier);}}
static pr_second_slot_status second_class_return_after(pr_second_class_return_session*s,pr_second_class_return_diagnostic*d,pr_second_slot_status st){
    pr_290b_return_target x;d->classifier_status=st;if(st==PR_SECOND_SLOT_YIELD){s->phase=PR_SECOND_CLASS_RETURN_WAITING;return st;}
    s->phase=PR_SECOND_CLASS_RETURN_FINISHED;if(st!=PR_SECOND_SLOT_PAIR)return st;
    x=(pr_290b_return_target){s->cpu->cs,0x0c84u,(uint16_t)(s->cpu->sp+6u),s->cpu->bp,s->cpu->ss};
    d->return_status=pr_u25_classifier_return(s->bus,PR_BUS_RAM_SIZE,s->cpu,&x);
    return d->return_status==PR_290B_FIRST_DONE?PR_SECOND_SLOT_PAIR:PR_SECOND_SLOT_STOP;
}
pr_second_slot_status pr_second_class_return_start(pr_second_class_return_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_second_class_return_diagnostic*d){
    pr_second_slot_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_SECOND_CLASS_RETURN_EMPTY||s->classifier.phase!=PR_SECOND_CLASS_EMPTY||s->classifier.store.phase!=PR_SECOND_STORE_EMPTY||s->classifier.store.value.phase!=PR_SECOND_VALUE_EMPTY||s->classifier.store.value.pair.phase!=PR_SECOND_PAIR_RETURN_EMPTY||s->classifier.store.value.pair.pair.phase!=PR_SECOND_PAIR_EMPTY||s->classifier.store.value.pair.pair.slot.phase!=PR_SECOND_SLOT_EMPTY||s->classifier.store.value.pair.pair.slot.branch.phase!=PR_SECOND_BRANCH_EMPTY||s->classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_SECOND_LOADER_EMPTY||s->classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_SECOND_SAVE_EMPTY||s->classifier.store.value.pair.pair.slot.branch.loader.save.next.phase!=PR_NEXT_DISPATCH_EMPTY||s->classifier.store.value.pair.pair.slot.branch.loader.save.next.runner.phase!=PR_RUNNER_NEXT_EMPTY||x->bp<56u)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_second_class_start(&s->classifier,b,n,c,e,cl,pl,x,max_iterations,&d->classifier);return second_class_return_after(s,d,st);
}
pr_second_slot_status pr_second_class_return_resume(pr_second_class_return_session*s,pr_second_class_return_diagnostic*d){pr_second_slot_status st;
    if(!s||!d||s->phase!=PR_SECOND_CLASS_RETURN_WAITING)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_second_class_resume(&s->classifier,&d->classifier);
    if(st==PR_SECOND_SLOT_ARGUMENT)return st;
    return second_class_return_after(s,d,st);
}

void pr_second_dispatch_session_init(pr_second_dispatch_session*s){if(s){memset(s,0,sizeof *s);pr_second_class_return_session_init(&s->classifier);}}
static pr_second_dispatch_status second_dispatch_after(pr_second_dispatch_session*s,pr_second_dispatch_diagnostic*d,pr_second_slot_status st){
    d->classifier_status=st;if(st==PR_SECOND_SLOT_YIELD){s->phase=PR_SECOND_DISPATCH_WAITING;return st;}
    s->phase=PR_SECOND_DISPATCH_FINISHED;if(st!=PR_SECOND_SLOT_PAIR)return st;
    d->dispatch_status=pr_u25_store_class_dispatch(s->bus,PR_BUS_RAM_SIZE,s->cpu);
    if(d->dispatch_status!=PR_290B_FIRST_DONE)return PR_SECOND_SLOT_STOP;
    if(s->cpu->ip==0x0c8eu)return PR_SECOND_DISPATCH_CLASS0;
    if(s->cpu->ip==0x0cbbu)return PR_SECOND_DISPATCH_CLASS1;
    if(s->cpu->ip==0x0cdfu)return PR_SECOND_DISPATCH_CLASS2;
    if(s->cpu->ip==0x0cf9u)return PR_SECOND_DISPATCH_CLASS3;
    if(s->cpu->ip==0x0d14u)return PR_SECOND_DISPATCH_CLASS4;
    return PR_SECOND_DISPATCH_UNMATCHED;
}
pr_second_dispatch_status pr_second_dispatch_start(pr_second_dispatch_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_second_dispatch_diagnostic*d){
    pr_second_slot_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_SECOND_DISPATCH_EMPTY||s->classifier.phase!=PR_SECOND_CLASS_RETURN_EMPTY||s->classifier.classifier.phase!=PR_SECOND_CLASS_EMPTY||s->classifier.classifier.store.phase!=PR_SECOND_STORE_EMPTY||s->classifier.classifier.store.value.phase!=PR_SECOND_VALUE_EMPTY||s->classifier.classifier.store.value.pair.phase!=PR_SECOND_PAIR_RETURN_EMPTY||s->classifier.classifier.store.value.pair.pair.phase!=PR_SECOND_PAIR_EMPTY||s->classifier.classifier.store.value.pair.pair.slot.phase!=PR_SECOND_SLOT_EMPTY||s->classifier.classifier.store.value.pair.pair.slot.branch.phase!=PR_SECOND_BRANCH_EMPTY||s->classifier.classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_SECOND_LOADER_EMPTY||s->classifier.classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_SECOND_SAVE_EMPTY||s->classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.phase!=PR_NEXT_DISPATCH_EMPTY||s->classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.runner.phase!=PR_RUNNER_NEXT_EMPTY||x->bp<56u)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_second_class_return_start(&s->classifier,b,n,c,e,cl,pl,x,max_iterations,&d->classifier);return second_dispatch_after(s,d,st);
}
pr_second_dispatch_status pr_second_dispatch_resume(pr_second_dispatch_session*s,pr_second_dispatch_diagnostic*d){pr_second_slot_status st;
    if(!s||!d||s->phase!=PR_SECOND_DISPATCH_WAITING)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_second_class_return_resume(&s->classifier,&d->classifier);
    if(st==PR_SECOND_SLOT_ARGUMENT)return st;
    return second_dispatch_after(s,d,st);
}

void pr_second_class4_session_init(pr_second_class4_session*s){if(s){memset(s,0,sizeof *s);pr_second_dispatch_session_init(&s->dispatch);}}
static pr_second_dispatch_status second_class4_after(pr_second_class4_session*s,pr_second_class4_diagnostic*d,pr_second_dispatch_status st){
    d->dispatch_status=st;if(st==PR_SECOND_SLOT_YIELD){s->phase=PR_SECOND_CLASS4_WAITING;return st;}
    s->phase=PR_SECOND_CLASS4_FINISHED;if(st!=PR_SECOND_DISPATCH_CLASS4)return st;
    d->low_status=pr_u25_store_class4_low(s->bus,PR_BUS_RAM_SIZE,s->cpu);
    if(d->low_status==PR_290B_FIRST_DONE)return PR_SECOND_CLASS4_LOW_REACHED;
    if(d->low_status==PR_290B_FIRST_OUTSIDE)return PR_SECOND_CLASS4_LOW_HIGH;
    return PR_SECOND_SLOT_STOP;
}
pr_second_dispatch_status pr_second_class4_start(pr_second_class4_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_second_class4_diagnostic*d){
    pr_second_dispatch_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_SECOND_CLASS4_EMPTY||s->dispatch.phase!=PR_SECOND_DISPATCH_EMPTY||s->dispatch.classifier.phase!=PR_SECOND_CLASS_RETURN_EMPTY||s->dispatch.classifier.classifier.phase!=PR_SECOND_CLASS_EMPTY||s->dispatch.classifier.classifier.store.phase!=PR_SECOND_STORE_EMPTY||s->dispatch.classifier.classifier.store.value.phase!=PR_SECOND_VALUE_EMPTY||s->dispatch.classifier.classifier.store.value.pair.phase!=PR_SECOND_PAIR_RETURN_EMPTY||s->dispatch.classifier.classifier.store.value.pair.pair.phase!=PR_SECOND_PAIR_EMPTY||s->dispatch.classifier.classifier.store.value.pair.pair.slot.phase!=PR_SECOND_SLOT_EMPTY||s->dispatch.classifier.classifier.store.value.pair.pair.slot.branch.phase!=PR_SECOND_BRANCH_EMPTY||s->dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_SECOND_LOADER_EMPTY||s->dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_SECOND_SAVE_EMPTY||s->dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.phase!=PR_NEXT_DISPATCH_EMPTY||s->dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.runner.phase!=PR_RUNNER_NEXT_EMPTY||x->bp<56u)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_second_dispatch_start(&s->dispatch,b,n,c,e,cl,pl,x,max_iterations,&d->dispatch);return second_class4_after(s,d,st);
}
pr_second_dispatch_status pr_second_class4_resume(pr_second_class4_session*s,pr_second_class4_diagnostic*d){pr_second_dispatch_status st;
    if(!s||!d||s->phase!=PR_SECOND_CLASS4_WAITING)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_second_dispatch_resume(&s->dispatch,&d->dispatch);
    if(st==PR_SECOND_SLOT_ARGUMENT)return st;
    return second_class4_after(s,d,st);
}

void pr_second_store_return_session_init(pr_second_store_return_session*s){if(s){memset(s,0,sizeof *s);pr_second_class4_session_init(&s->low);}}
static pr_second_dispatch_status second_store_return_after(pr_second_store_return_session*s,pr_second_store_return_diagnostic*d,pr_second_dispatch_status st){
    d->low_status=st;if(st==PR_SECOND_SLOT_YIELD){s->phase=PR_SECOND_STORE_RETURN_WAITING;return st;}
    s->phase=PR_SECOND_STORE_RETURN_FINISHED;if(st!=PR_SECOND_CLASS4_LOW_REACHED)return st;
    d->return_status=pr_u25_store_return(s->bus,PR_BUS_RAM_SIZE,s->cpu,&s->target);
    return d->return_status==PR_290B_FIRST_DONE?PR_SECOND_STORE_RETURN_REACHED:PR_SECOND_SLOT_STOP;
}
pr_second_dispatch_status pr_second_store_return_start(pr_second_store_return_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_second_store_return_diagnostic*d){
    pr_second_dispatch_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_SECOND_STORE_RETURN_EMPTY||s->low.phase!=PR_SECOND_CLASS4_EMPTY||s->low.dispatch.phase!=PR_SECOND_DISPATCH_EMPTY||s->low.dispatch.classifier.phase!=PR_SECOND_CLASS_RETURN_EMPTY||s->low.dispatch.classifier.classifier.phase!=PR_SECOND_CLASS_EMPTY||s->low.dispatch.classifier.classifier.store.phase!=PR_SECOND_STORE_EMPTY||s->low.dispatch.classifier.classifier.store.value.phase!=PR_SECOND_VALUE_EMPTY||s->low.dispatch.classifier.classifier.store.value.pair.phase!=PR_SECOND_PAIR_RETURN_EMPTY||s->low.dispatch.classifier.classifier.store.value.pair.pair.phase!=PR_SECOND_PAIR_EMPTY||s->low.dispatch.classifier.classifier.store.value.pair.pair.slot.phase!=PR_SECOND_SLOT_EMPTY||s->low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.phase!=PR_SECOND_BRANCH_EMPTY||s->low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_SECOND_LOADER_EMPTY||s->low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_SECOND_SAVE_EMPTY||s->low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.phase!=PR_NEXT_DISPATCH_EMPTY||s->low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.runner.phase!=PR_RUNNER_NEXT_EMPTY||x->bp<56u)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;s->target=(pr_290b_return_target){x->cs,0x02cdu,(uint16_t)(x->bp-0x1au),(uint16_t)(x->bp-0x16u),x->ss};
    st=pr_second_class4_start(&s->low,b,n,c,e,cl,pl,x,max_iterations,&d->low);return second_store_return_after(s,d,st);
}
pr_second_dispatch_status pr_second_store_return_resume(pr_second_store_return_session*s,pr_second_store_return_diagnostic*d){pr_second_dispatch_status st;
    if(!s||!d||s->phase!=PR_SECOND_STORE_RETURN_WAITING)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_second_class4_resume(&s->low,&d->low);
    if(st==PR_SECOND_SLOT_ARGUMENT)return st;
    return second_store_return_after(s,d,st);
}

void pr_second_handler_session_init(pr_second_handler_session*s){if(s){memset(s,0,sizeof *s);pr_second_store_return_session_init(&s->store);}}
static pr_second_dispatch_status second_handler_after(pr_second_handler_session*s,pr_second_handler_diagnostic*d,pr_second_dispatch_status st){
    d->store_status=st;if(st==PR_SECOND_SLOT_YIELD){s->phase=PR_SECOND_HANDLER_WAITING;return st;}
    s->phase=PR_SECOND_HANDLER_FINISHED;if(st!=PR_SECOND_STORE_RETURN_REACHED)return st;
    d->return_status=pr_u1c_save_handler_return(s->bus,PR_BUS_RAM_SIZE,s->cpu,&s->target);
    return d->return_status==PR_290B_FIRST_DONE?PR_SECOND_HANDLER_RETURN_REACHED:PR_SECOND_SLOT_STOP;
}
pr_second_dispatch_status pr_second_handler_start(pr_second_handler_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_second_handler_diagnostic*d){
    pr_second_dispatch_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_SECOND_HANDLER_EMPTY||s->store.phase!=PR_SECOND_STORE_RETURN_EMPTY||s->store.low.phase!=PR_SECOND_CLASS4_EMPTY||s->store.low.dispatch.phase!=PR_SECOND_DISPATCH_EMPTY||s->store.low.dispatch.classifier.phase!=PR_SECOND_CLASS_RETURN_EMPTY||s->store.low.dispatch.classifier.classifier.phase!=PR_SECOND_CLASS_EMPTY||s->store.low.dispatch.classifier.classifier.store.phase!=PR_SECOND_STORE_EMPTY||s->store.low.dispatch.classifier.classifier.store.value.phase!=PR_SECOND_VALUE_EMPTY||s->store.low.dispatch.classifier.classifier.store.value.pair.phase!=PR_SECOND_PAIR_RETURN_EMPTY||s->store.low.dispatch.classifier.classifier.store.value.pair.pair.phase!=PR_SECOND_PAIR_EMPTY||s->store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.phase!=PR_SECOND_SLOT_EMPTY||s->store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.phase!=PR_SECOND_BRANCH_EMPTY||s->store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_SECOND_LOADER_EMPTY||s->store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_SECOND_SAVE_EMPTY||s->store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.phase!=PR_NEXT_DISPATCH_EMPTY||s->store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.runner.phase!=PR_RUNNER_NEXT_EMPTY||x->bp<56u)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;s->target=(pr_290b_return_target){x->cs,0x2f89u,(uint16_t)(x->bp-0x10u),(uint16_t)(x->bp-0x10u),x->ss};
    st=pr_second_store_return_start(&s->store,b,n,c,e,cl,pl,x,max_iterations,&d->store);return second_handler_after(s,d,st);
}
pr_second_dispatch_status pr_second_handler_resume(pr_second_handler_session*s,pr_second_handler_diagnostic*d){pr_second_dispatch_status st;
    if(!s||!d||s->phase!=PR_SECOND_HANDLER_WAITING)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_second_store_return_resume(&s->store,&d->store);
    if(st==PR_SECOND_SLOT_ARGUMENT)return st;
    return second_handler_after(s,d,st);
}

void pr_second_dispatch_return_session_init(pr_second_dispatch_return_session*s){if(s){memset(s,0,sizeof *s);pr_second_handler_session_init(&s->handler);}}
static pr_second_dispatch_status second_dispatch_return_after(pr_second_dispatch_return_session*s,pr_second_dispatch_return_diagnostic*d,pr_second_dispatch_status st){
    d->handler_status=st;if(st==PR_SECOND_SLOT_YIELD){s->phase=PR_SECOND_DISPATCH_RETURN_WAITING;return st;}
    s->phase=PR_SECOND_DISPATCH_RETURN_FINISHED;if(st!=PR_SECOND_HANDLER_RETURN_REACHED)return st;
    d->return_status=pr_u1c_save_dispatch_return(s->bus,PR_BUS_RAM_SIZE,s->cpu,&s->target);
    return d->return_status==PR_290B_FIRST_DONE?PR_SECOND_DISPATCH_RETURN_REACHED:PR_SECOND_SLOT_STOP;
}
pr_second_dispatch_status pr_second_dispatch_return_start(pr_second_dispatch_return_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_second_dispatch_return_diagnostic*d){
    pr_second_dispatch_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_SECOND_DISPATCH_RETURN_EMPTY||s->handler.phase!=PR_SECOND_HANDLER_EMPTY||s->handler.store.phase!=PR_SECOND_STORE_RETURN_EMPTY||s->handler.store.low.phase!=PR_SECOND_CLASS4_EMPTY||s->handler.store.low.dispatch.phase!=PR_SECOND_DISPATCH_EMPTY||s->handler.store.low.dispatch.classifier.phase!=PR_SECOND_CLASS_RETURN_EMPTY||s->handler.store.low.dispatch.classifier.classifier.phase!=PR_SECOND_CLASS_EMPTY||s->handler.store.low.dispatch.classifier.classifier.store.phase!=PR_SECOND_STORE_EMPTY||s->handler.store.low.dispatch.classifier.classifier.store.value.phase!=PR_SECOND_VALUE_EMPTY||s->handler.store.low.dispatch.classifier.classifier.store.value.pair.phase!=PR_SECOND_PAIR_RETURN_EMPTY||s->handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.phase!=PR_SECOND_PAIR_EMPTY||s->handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.phase!=PR_SECOND_SLOT_EMPTY||s->handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.phase!=PR_SECOND_BRANCH_EMPTY||s->handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_SECOND_LOADER_EMPTY||s->handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_SECOND_SAVE_EMPTY||s->handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.phase!=PR_NEXT_DISPATCH_EMPTY||s->handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.runner.phase!=PR_RUNNER_NEXT_EMPTY||x->bp<56u)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;s->target=(pr_290b_return_target){x->cs,0x31a8u,(uint16_t)(x->bp-0x0au),(uint16_t)(x->bp-0x0au),x->ss};
    st=pr_second_handler_start(&s->handler,b,n,c,e,cl,pl,x,max_iterations,&d->handler);return second_dispatch_return_after(s,d,st);
}
pr_second_dispatch_status pr_second_dispatch_return_resume(pr_second_dispatch_return_session*s,pr_second_dispatch_return_diagnostic*d){pr_second_dispatch_status st;
    if(!s||!d||s->phase!=PR_SECOND_DISPATCH_RETURN_WAITING)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_second_handler_resume(&s->handler,&d->handler);
    if(st==PR_SECOND_SLOT_ARGUMENT)return st;
    return second_dispatch_return_after(s,d,st);
}

void pr_second_runner_session_init(pr_second_runner_session*s){if(s){memset(s,0,sizeof *s);pr_second_dispatch_return_session_init(&s->dispatch);}}
static pr_second_dispatch_status second_runner_after(pr_second_runner_session*s,pr_second_runner_diagnostic*d,pr_second_dispatch_status st){
    d->dispatch_status=st;if(st==PR_SECOND_SLOT_YIELD){s->phase=PR_SECOND_RUNNER_WAITING;return st;}
    s->phase=PR_SECOND_RUNNER_FINISHED;if(st!=PR_SECOND_DISPATCH_RETURN_REACHED)return st;
    d->next_status=pr_u1c_runner_next(s->bus,PR_BUS_RAM_SIZE,s->cpu,&s->target);
    if(d->next_status==PR_290B_FIRST_OUTSIDE)return PR_SECOND_RUNNER_DISPATCH;
    if(d->next_status==PR_290B_FIRST_DONE)return PR_SECOND_RUNNER_RETURNED;
    return PR_SECOND_SLOT_STOP;
}
pr_second_dispatch_status pr_second_runner_start(pr_second_runner_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_second_runner_diagnostic*d){
    pr_second_dispatch_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_SECOND_RUNNER_EMPTY||s->dispatch.phase!=PR_SECOND_DISPATCH_RETURN_EMPTY||s->dispatch.handler.phase!=PR_SECOND_HANDLER_EMPTY||s->dispatch.handler.store.phase!=PR_SECOND_STORE_RETURN_EMPTY||s->dispatch.handler.store.low.phase!=PR_SECOND_CLASS4_EMPTY||s->dispatch.handler.store.low.dispatch.phase!=PR_SECOND_DISPATCH_EMPTY||s->dispatch.handler.store.low.dispatch.classifier.phase!=PR_SECOND_CLASS_RETURN_EMPTY||s->dispatch.handler.store.low.dispatch.classifier.classifier.phase!=PR_SECOND_CLASS_EMPTY||s->dispatch.handler.store.low.dispatch.classifier.classifier.store.phase!=PR_SECOND_STORE_EMPTY||s->dispatch.handler.store.low.dispatch.classifier.classifier.store.value.phase!=PR_SECOND_VALUE_EMPTY||s->dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.phase!=PR_SECOND_PAIR_RETURN_EMPTY||s->dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.phase!=PR_SECOND_PAIR_EMPTY||s->dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.phase!=PR_SECOND_SLOT_EMPTY||s->dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.phase!=PR_SECOND_BRANCH_EMPTY||s->dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_SECOND_LOADER_EMPTY||s->dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_SECOND_SAVE_EMPTY||s->dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.phase!=PR_NEXT_DISPATCH_EMPTY||s->dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.runner.phase!=PR_RUNNER_NEXT_EMPTY||x->bp<56u)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;s->target=*x;s->target.ip=0x345bu;
    st=pr_second_dispatch_return_start(&s->dispatch,b,n,c,e,cl,pl,x,max_iterations,&d->dispatch);return second_runner_after(s,d,st);
}
pr_second_dispatch_status pr_second_runner_resume(pr_second_runner_session*s,pr_second_runner_diagnostic*d){pr_second_dispatch_status st;
    if(!s||!d||s->phase!=PR_SECOND_RUNNER_WAITING)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_second_dispatch_return_resume(&s->dispatch,&d->dispatch);
    if(st==PR_SECOND_SLOT_ARGUMENT)return st;
    return second_runner_after(s,d,st);
}

void pr_third_dispatch_session_init(pr_third_dispatch_session*s){if(s){memset(s,0,sizeof *s);pr_second_runner_session_init(&s->runner);}}
static pr_second_dispatch_status third_dispatch_after(pr_third_dispatch_session*s,pr_third_dispatch_diagnostic*d,pr_second_dispatch_status st){
    d->runner_status=st;if(st==PR_SECOND_SLOT_YIELD){s->phase=PR_THIRD_DISPATCH_WAITING;return st;}
    s->phase=PR_THIRD_DISPATCH_FINISHED;if(st!=PR_SECOND_RUNNER_DISPATCH)return st;
    d->dispatch_status=pr_u1c_ecl_dispatch_prefix(s->bus,PR_BUS_RAM_SIZE,s->cpu);
    if(d->dispatch_status==PR_290B_FIRST_OUTSIDE)return PR_THIRD_DISPATCH_LATER;
    return d->dispatch_status==PR_290B_FIRST_DONE?PR_THIRD_DISPATCH_HANDLER:PR_SECOND_SLOT_STOP;
}
pr_second_dispatch_status pr_third_dispatch_start(pr_third_dispatch_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_third_dispatch_diagnostic*d){
    pr_second_dispatch_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_THIRD_DISPATCH_EMPTY||s->runner.phase!=PR_SECOND_RUNNER_EMPTY||s->runner.dispatch.phase!=PR_SECOND_DISPATCH_RETURN_EMPTY||s->runner.dispatch.handler.phase!=PR_SECOND_HANDLER_EMPTY||s->runner.dispatch.handler.store.phase!=PR_SECOND_STORE_RETURN_EMPTY||s->runner.dispatch.handler.store.low.phase!=PR_SECOND_CLASS4_EMPTY||s->runner.dispatch.handler.store.low.dispatch.phase!=PR_SECOND_DISPATCH_EMPTY||s->runner.dispatch.handler.store.low.dispatch.classifier.phase!=PR_SECOND_CLASS_RETURN_EMPTY||s->runner.dispatch.handler.store.low.dispatch.classifier.classifier.phase!=PR_SECOND_CLASS_EMPTY||s->runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.phase!=PR_SECOND_STORE_EMPTY||s->runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.phase!=PR_SECOND_VALUE_EMPTY||s->runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.phase!=PR_SECOND_PAIR_RETURN_EMPTY||s->runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.phase!=PR_SECOND_PAIR_EMPTY||s->runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.phase!=PR_SECOND_SLOT_EMPTY||s->runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.phase!=PR_SECOND_BRANCH_EMPTY||s->runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_SECOND_LOADER_EMPTY||s->runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_SECOND_SAVE_EMPTY||s->runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.phase!=PR_NEXT_DISPATCH_EMPTY||s->runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.runner.phase!=PR_RUNNER_NEXT_EMPTY||x->bp<56u)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_second_runner_start(&s->runner,b,n,c,e,cl,pl,x,max_iterations,&d->runner);return third_dispatch_after(s,d,st);
}
pr_second_dispatch_status pr_third_dispatch_resume(pr_third_dispatch_session*s,pr_third_dispatch_diagnostic*d){pr_second_dispatch_status st;
    if(!s||!d||s->phase!=PR_THIRD_DISPATCH_WAITING)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_second_runner_resume(&s->runner,&d->runner);
    if(st==PR_SECOND_SLOT_ARGUMENT)return st;
    return third_dispatch_after(s,d,st);
}

void pr_third_save_session_init(pr_third_save_session*s){if(s){memset(s,0,sizeof *s);pr_third_dispatch_session_init(&s->dispatch);}}
static pr_second_dispatch_status third_save_after(pr_third_save_session*s,pr_third_save_diagnostic*d,pr_second_dispatch_status st){
    d->dispatch_status=st;if(st==PR_SECOND_SLOT_YIELD){s->phase=PR_THIRD_SAVE_WAITING;return st;}
    s->phase=PR_THIRD_SAVE_FINISHED;if(st!=PR_THIRD_DISPATCH_HANDLER)return st;
    if(s->cpu->ip!=0x0292u)return PR_THIRD_SAVE_OTHER_HANDLER;
    d->entry_status=pr_u1c_save_loader_entry(s->bus,PR_BUS_RAM_SIZE,s->cpu,&d->entry);
    return d->entry_status==PR_290B_FIRST_DONE?PR_THIRD_SAVE_LOADER:PR_SECOND_SLOT_STOP;
}
pr_second_dispatch_status pr_third_save_start(pr_third_save_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_third_save_diagnostic*d){
    pr_second_dispatch_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_THIRD_SAVE_EMPTY||s->dispatch.phase!=PR_THIRD_DISPATCH_EMPTY||s->dispatch.runner.phase!=PR_SECOND_RUNNER_EMPTY||s->dispatch.runner.dispatch.phase!=PR_SECOND_DISPATCH_RETURN_EMPTY||s->dispatch.runner.dispatch.handler.phase!=PR_SECOND_HANDLER_EMPTY||s->dispatch.runner.dispatch.handler.store.phase!=PR_SECOND_STORE_RETURN_EMPTY||s->dispatch.runner.dispatch.handler.store.low.phase!=PR_SECOND_CLASS4_EMPTY||s->dispatch.runner.dispatch.handler.store.low.dispatch.phase!=PR_SECOND_DISPATCH_EMPTY||s->dispatch.runner.dispatch.handler.store.low.dispatch.classifier.phase!=PR_SECOND_CLASS_RETURN_EMPTY||s->dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.phase!=PR_SECOND_CLASS_EMPTY||s->dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.phase!=PR_SECOND_STORE_EMPTY||s->dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.phase!=PR_SECOND_VALUE_EMPTY||s->dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.phase!=PR_SECOND_PAIR_RETURN_EMPTY||s->dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.phase!=PR_SECOND_PAIR_EMPTY||s->dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.phase!=PR_SECOND_SLOT_EMPTY||s->dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.phase!=PR_SECOND_BRANCH_EMPTY||s->dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_SECOND_LOADER_EMPTY||s->dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_SECOND_SAVE_EMPTY||s->dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.phase!=PR_NEXT_DISPATCH_EMPTY||s->dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.runner.phase!=PR_RUNNER_NEXT_EMPTY||x->bp<56u)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_third_dispatch_start(&s->dispatch,b,n,c,e,cl,pl,x,max_iterations,&d->dispatch);return third_save_after(s,d,st);
}
pr_second_dispatch_status pr_third_save_resume(pr_third_save_session*s,pr_third_save_diagnostic*d){pr_second_dispatch_status st;
    if(!s||!d||s->phase!=PR_THIRD_SAVE_WAITING)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_third_dispatch_resume(&s->dispatch,&d->dispatch);
    if(st==PR_SECOND_SLOT_ARGUMENT)return st;
    return third_save_after(s,d,st);
}

void pr_third_loader_session_init(pr_third_loader_session*s){if(s){memset(s,0,sizeof *s);pr_third_save_session_init(&s->save);}}
static pr_second_dispatch_status third_loader_after(pr_third_loader_session*s,pr_third_loader_diagnostic*d,pr_second_dispatch_status st){
    d->save_status=st;if(st==PR_SECOND_SLOT_YIELD){s->phase=PR_THIRD_LOADER_WAITING;return st;}
    s->phase=PR_THIRD_LOADER_FINISHED;if(st!=PR_THIRD_SAVE_LOADER)return st;
    d->prefix_status=pr_u25_operand_prefix(s->bus,PR_BUS_RAM_SIZE,s->cpu);
    if(d->prefix_status==PR_290B_FIRST_DONE){
        d->tag_status=pr_u25_operand_tag(s->bus,PR_BUS_RAM_SIZE,s->cpu);
        if(d->tag_status==PR_290B_FIRST_OUTSIDE)return PR_THIRD_LOADER_STRING;
        if(d->tag_status!=PR_290B_FIRST_DONE)return PR_SECOND_SLOT_STOP;
        d->loop_status=pr_u25_numeric_loop(s->bus,PR_BUS_RAM_SIZE,s->cpu,s->max_iterations,&d->loop);
        if(d->loop_status==PR_U25_LOOP_STEP_STRING)return PR_THIRD_LOADER_STRING;
        if(d->loop_status==PR_U25_LOOP_STEP_LIMIT)return PR_THIRD_LOADER_LIMIT;
        if(d->loop_status!=PR_U25_LOOP_STEP_DONE)return PR_SECOND_SLOT_STOP;
    }else if(d->prefix_status!=PR_290B_FIRST_OUTSIDE||s->cpu->ip!=0x012cu)return PR_SECOND_SLOT_STOP;
    d->return_status=pr_u25_operand_return(s->bus,PR_BUS_RAM_SIZE,s->cpu,&s->target);
    return d->return_status==PR_290B_FIRST_DONE?PR_THIRD_LOADER_REACHED:PR_SECOND_SLOT_STOP;
}
pr_second_dispatch_status pr_third_loader_start(pr_third_loader_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_third_loader_diagnostic*d){
    pr_second_dispatch_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_THIRD_LOADER_EMPTY||s->save.phase!=PR_THIRD_SAVE_EMPTY||s->save.dispatch.phase!=PR_THIRD_DISPATCH_EMPTY||s->save.dispatch.runner.phase!=PR_SECOND_RUNNER_EMPTY||s->save.dispatch.runner.dispatch.phase!=PR_SECOND_DISPATCH_RETURN_EMPTY||s->save.dispatch.runner.dispatch.handler.phase!=PR_SECOND_HANDLER_EMPTY||s->save.dispatch.runner.dispatch.handler.store.phase!=PR_SECOND_STORE_RETURN_EMPTY||s->save.dispatch.runner.dispatch.handler.store.low.phase!=PR_SECOND_CLASS4_EMPTY||s->save.dispatch.runner.dispatch.handler.store.low.dispatch.phase!=PR_SECOND_DISPATCH_EMPTY||s->save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.phase!=PR_SECOND_CLASS_RETURN_EMPTY||s->save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.phase!=PR_SECOND_CLASS_EMPTY||s->save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.phase!=PR_SECOND_STORE_EMPTY||s->save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.phase!=PR_SECOND_VALUE_EMPTY||s->save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.phase!=PR_SECOND_PAIR_RETURN_EMPTY||s->save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.phase!=PR_SECOND_PAIR_EMPTY||s->save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.phase!=PR_SECOND_SLOT_EMPTY||s->save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.phase!=PR_SECOND_BRANCH_EMPTY||s->save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_SECOND_LOADER_EMPTY||s->save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_SECOND_SAVE_EMPTY||s->save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.phase!=PR_NEXT_DISPATCH_EMPTY||s->save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.runner.phase!=PR_RUNNER_NEXT_EMPTY||x->bp<56u)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;s->max_iterations=max_iterations;s->target=(pr_290b_return_target){x->cs,0x02a0u,(uint16_t)(x->bp-0x1au),(uint16_t)(x->bp-0x16u),x->ss};
    st=pr_third_save_start(&s->save,b,n,c,e,cl,pl,x,max_iterations,&d->save);return third_loader_after(s,d,st);
}
pr_second_dispatch_status pr_third_loader_resume(pr_third_loader_session*s,pr_third_loader_diagnostic*d){pr_second_dispatch_status st;
    if(!s||!d||s->phase!=PR_THIRD_LOADER_WAITING)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_third_save_resume(&s->save,&d->save);
    if(st==PR_SECOND_SLOT_ARGUMENT)return st;
    return third_loader_after(s,d,st);
}

void pr_third_branch_session_init(pr_third_branch_session*s){if(s){memset(s,0,sizeof *s);pr_third_loader_session_init(&s->loader);}}
static pr_second_dispatch_status third_branch_after(pr_third_branch_session*s,pr_third_branch_diagnostic*d,pr_second_dispatch_status st){
    pr_290b_return_target x;d->loader_status=st;if(st==PR_SECOND_SLOT_YIELD){s->phase=PR_THIRD_BRANCH_WAITING;return st;}
    s->phase=PR_THIRD_BRANCH_FINISHED;if(st!=PR_THIRD_LOADER_REACHED)return st;
    x=(pr_290b_return_target){s->cpu->cs,0x02adu,s->cpu->sp,s->cpu->bp,s->cpu->ss};
    d->entry_status=pr_u1c_save_word_entry(s->bus,PR_BUS_RAM_SIZE,s->cpu,&d->entry);
    if(d->entry_status!=PR_290B_FIRST_DONE)return PR_SECOND_SLOT_STOP;
    d->word_status=pr_u25_word_return(s->bus,PR_BUS_RAM_SIZE,s->cpu,&x);
    if(d->word_status!=PR_290B_FIRST_DONE)return PR_SECOND_SLOT_STOP;
    d->branch_status=pr_u1c_save_tag_branch(s->bus,PR_BUS_RAM_SIZE,s->cpu);
    if(d->branch_status!=PR_290B_FIRST_DONE)return PR_SECOND_SLOT_STOP;
    return s->cpu->ip==0x02cfu?PR_THIRD_BRANCH_SAVE_STRING:PR_THIRD_BRANCH_NUMERIC;
}
pr_second_dispatch_status pr_third_branch_start(pr_third_branch_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_third_branch_diagnostic*d){
    pr_second_dispatch_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_THIRD_BRANCH_EMPTY||s->loader.phase!=PR_THIRD_LOADER_EMPTY||s->loader.save.phase!=PR_THIRD_SAVE_EMPTY||s->loader.save.dispatch.phase!=PR_THIRD_DISPATCH_EMPTY||s->loader.save.dispatch.runner.phase!=PR_SECOND_RUNNER_EMPTY||s->loader.save.dispatch.runner.dispatch.phase!=PR_SECOND_DISPATCH_RETURN_EMPTY||s->loader.save.dispatch.runner.dispatch.handler.phase!=PR_SECOND_HANDLER_EMPTY||s->loader.save.dispatch.runner.dispatch.handler.store.phase!=PR_SECOND_STORE_RETURN_EMPTY||s->loader.save.dispatch.runner.dispatch.handler.store.low.phase!=PR_SECOND_CLASS4_EMPTY||s->loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.phase!=PR_SECOND_DISPATCH_EMPTY||s->loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.phase!=PR_SECOND_CLASS_RETURN_EMPTY||s->loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.phase!=PR_SECOND_CLASS_EMPTY||s->loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.phase!=PR_SECOND_STORE_EMPTY||s->loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.phase!=PR_SECOND_VALUE_EMPTY||s->loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.phase!=PR_SECOND_PAIR_RETURN_EMPTY||s->loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.phase!=PR_SECOND_PAIR_EMPTY||s->loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.phase!=PR_SECOND_SLOT_EMPTY||s->loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.phase!=PR_SECOND_BRANCH_EMPTY||s->loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_SECOND_LOADER_EMPTY||s->loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_SECOND_SAVE_EMPTY||s->loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.phase!=PR_NEXT_DISPATCH_EMPTY||s->loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.runner.phase!=PR_RUNNER_NEXT_EMPTY||x->bp<56u)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_third_loader_start(&s->loader,b,n,c,e,cl,pl,x,max_iterations,&d->loader);return third_branch_after(s,d,st);
}
pr_second_dispatch_status pr_third_branch_resume(pr_third_branch_session*s,pr_third_branch_diagnostic*d){pr_second_dispatch_status st;
    if(!s||!d||s->phase!=PR_THIRD_BRANCH_WAITING)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_third_loader_resume(&s->loader,&d->loader);
    if(st==PR_SECOND_SLOT_ARGUMENT)return st;
    return third_branch_after(s,d,st);
}

void pr_third_slot_session_init(pr_third_slot_session*s){if(s){memset(s,0,sizeof *s);pr_third_branch_session_init(&s->branch);}}
static pr_second_dispatch_status third_slot_after(pr_third_slot_session*s,pr_third_slot_diagnostic*d,pr_second_dispatch_status st){
    d->branch_status=st;if(st==PR_SECOND_SLOT_YIELD){s->phase=PR_THIRD_SLOT_WAITING;return st;}
    s->phase=PR_THIRD_SLOT_FINISHED;if(st!=PR_THIRD_BRANCH_NUMERIC)return st;
    d->entry_status=pr_u1c_save_slot_entry(s->bus,PR_BUS_RAM_SIZE,s->cpu,&d->entry);
    if(d->entry_status!=PR_290B_FIRST_DONE)return PR_SECOND_SLOT_STOP;
    d->prefix_status=pr_u25_slot_prefix(s->bus,PR_BUS_RAM_SIZE,s->cpu);
    if(d->prefix_status!=PR_290B_FIRST_DONE)return PR_SECOND_SLOT_STOP;
    d->dispatch_status=pr_u25_slot_dispatch(s->bus,PR_BUS_RAM_SIZE,s->cpu);
    if(d->dispatch_status!=PR_290B_FIRST_DONE)return PR_SECOND_SLOT_STOP;
    if(s->cpu->ip==0x0164u)return PR_THIRD_SLOT_ZERO;
    if(s->cpu->ip==0x017au)return PR_THIRD_SLOT_WORD;
    if(s->cpu->ip==0x019eu)return PR_THIRD_SLOT_PAIR;
    return PR_THIRD_SLOT_OTHER_TYPE;
}
pr_second_dispatch_status pr_third_slot_start(pr_third_slot_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_third_slot_diagnostic*d){
    pr_second_dispatch_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_THIRD_SLOT_EMPTY||s->branch.phase!=PR_THIRD_BRANCH_EMPTY||s->branch.loader.phase!=PR_THIRD_LOADER_EMPTY||s->branch.loader.save.phase!=PR_THIRD_SAVE_EMPTY||s->branch.loader.save.dispatch.phase!=PR_THIRD_DISPATCH_EMPTY||s->branch.loader.save.dispatch.runner.phase!=PR_SECOND_RUNNER_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.phase!=PR_SECOND_DISPATCH_RETURN_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.handler.phase!=PR_SECOND_HANDLER_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.handler.store.phase!=PR_SECOND_STORE_RETURN_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.handler.store.low.phase!=PR_SECOND_CLASS4_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.phase!=PR_SECOND_DISPATCH_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.phase!=PR_SECOND_CLASS_RETURN_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.phase!=PR_SECOND_CLASS_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.phase!=PR_SECOND_STORE_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.phase!=PR_SECOND_VALUE_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.phase!=PR_SECOND_PAIR_RETURN_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.phase!=PR_SECOND_PAIR_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.phase!=PR_SECOND_SLOT_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.phase!=PR_SECOND_BRANCH_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_SECOND_LOADER_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_SECOND_SAVE_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.phase!=PR_NEXT_DISPATCH_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.runner.phase!=PR_RUNNER_NEXT_EMPTY||x->bp<56u)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_third_branch_start(&s->branch,b,n,c,e,cl,pl,x,max_iterations,&d->branch);return third_slot_after(s,d,st);
}
pr_second_dispatch_status pr_third_slot_resume(pr_third_slot_session*s,pr_third_slot_diagnostic*d){pr_second_dispatch_status st;
    if(!s||!d||s->phase!=PR_THIRD_SLOT_WAITING)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_third_branch_resume(&s->branch,&d->branch);
    if(st==PR_SECOND_SLOT_ARGUMENT)return st;
    return third_slot_after(s,d,st);
}

void pr_third_pair_session_init(pr_third_pair_session*s){if(s){memset(s,0,sizeof *s);pr_third_slot_session_init(&s->slot);}}
static pr_second_dispatch_status third_pair_after(pr_third_pair_session*s,pr_third_pair_diagnostic*d,pr_second_dispatch_status st){
    d->slot_status=st;if(st==PR_SECOND_SLOT_YIELD){s->phase=PR_THIRD_PAIR_WAITING;return st;}
    s->phase=PR_THIRD_PAIR_FINISHED;if(st!=PR_THIRD_SLOT_PAIR)return st;
    d->entry_status=pr_u25_slot_pair_entry(s->bus,PR_BUS_RAM_SIZE,s->cpu,&d->entry);
    return d->entry_status==PR_290B_FIRST_DONE?PR_THIRD_SLOT_PAIR:PR_SECOND_SLOT_STOP;
}
pr_second_dispatch_status pr_third_pair_start(pr_third_pair_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_third_pair_diagnostic*d){
    pr_second_dispatch_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_THIRD_PAIR_EMPTY||s->slot.phase!=PR_THIRD_SLOT_EMPTY||s->slot.branch.phase!=PR_THIRD_BRANCH_EMPTY||s->slot.branch.loader.phase!=PR_THIRD_LOADER_EMPTY||s->slot.branch.loader.save.phase!=PR_THIRD_SAVE_EMPTY||s->slot.branch.loader.save.dispatch.phase!=PR_THIRD_DISPATCH_EMPTY||s->slot.branch.loader.save.dispatch.runner.phase!=PR_SECOND_RUNNER_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.phase!=PR_SECOND_DISPATCH_RETURN_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.handler.phase!=PR_SECOND_HANDLER_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.handler.store.phase!=PR_SECOND_STORE_RETURN_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.phase!=PR_SECOND_CLASS4_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.phase!=PR_SECOND_DISPATCH_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.phase!=PR_SECOND_CLASS_RETURN_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.phase!=PR_SECOND_CLASS_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.phase!=PR_SECOND_STORE_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.phase!=PR_SECOND_VALUE_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.phase!=PR_SECOND_PAIR_RETURN_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.phase!=PR_SECOND_PAIR_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.phase!=PR_SECOND_SLOT_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.phase!=PR_SECOND_BRANCH_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_SECOND_LOADER_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_SECOND_SAVE_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.phase!=PR_NEXT_DISPATCH_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.runner.phase!=PR_RUNNER_NEXT_EMPTY||x->bp<56u)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_third_slot_start(&s->slot,b,n,c,e,cl,pl,x,max_iterations,&d->slot);return third_pair_after(s,d,st);
}
pr_second_dispatch_status pr_third_pair_resume(pr_third_pair_session*s,pr_third_pair_diagnostic*d){pr_second_dispatch_status st;
    if(!s||!d||s->phase!=PR_THIRD_PAIR_WAITING)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_third_slot_resume(&s->slot,&d->slot);
    if(st==PR_SECOND_SLOT_ARGUMENT)return st;
    return third_pair_after(s,d,st);
}

void pr_third_pair_return_session_init(pr_third_pair_return_session*s){if(s){memset(s,0,sizeof *s);pr_third_pair_session_init(&s->pair);}}
static pr_second_dispatch_status third_pair_return_after(pr_third_pair_return_session*s,pr_third_pair_return_diagnostic*d,pr_second_dispatch_status st){
    pr_290b_return_target x;d->pair_status=st;if(st==PR_SECOND_SLOT_YIELD){s->phase=PR_THIRD_PAIR_RETURN_WAITING;return st;}
    s->phase=PR_THIRD_PAIR_RETURN_FINISHED;if(st!=PR_THIRD_SLOT_PAIR)return st;
    x=(pr_290b_return_target){s->cpu->cs,0x01aau,(uint16_t)(s->cpu->bp-10u),s->cpu->bp,s->cpu->ss};
    d->return_status=pr_u25_word_return(s->bus,PR_BUS_RAM_SIZE,s->cpu,&x);
    return d->return_status==PR_290B_FIRST_DONE?PR_THIRD_SLOT_PAIR:PR_SECOND_SLOT_STOP;
}
pr_second_dispatch_status pr_third_pair_return_start(pr_third_pair_return_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_third_pair_return_diagnostic*d){
    pr_second_dispatch_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_THIRD_PAIR_RETURN_EMPTY||s->pair.phase!=PR_THIRD_PAIR_EMPTY||s->pair.slot.phase!=PR_THIRD_SLOT_EMPTY||s->pair.slot.branch.phase!=PR_THIRD_BRANCH_EMPTY||s->pair.slot.branch.loader.phase!=PR_THIRD_LOADER_EMPTY||s->pair.slot.branch.loader.save.phase!=PR_THIRD_SAVE_EMPTY||s->pair.slot.branch.loader.save.dispatch.phase!=PR_THIRD_DISPATCH_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.phase!=PR_SECOND_RUNNER_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.phase!=PR_SECOND_DISPATCH_RETURN_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.phase!=PR_SECOND_HANDLER_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.phase!=PR_SECOND_STORE_RETURN_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.phase!=PR_SECOND_CLASS4_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.phase!=PR_SECOND_DISPATCH_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.phase!=PR_SECOND_CLASS_RETURN_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.phase!=PR_SECOND_CLASS_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.phase!=PR_SECOND_STORE_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.phase!=PR_SECOND_VALUE_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.phase!=PR_SECOND_PAIR_RETURN_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.phase!=PR_SECOND_PAIR_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.phase!=PR_SECOND_SLOT_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.phase!=PR_SECOND_BRANCH_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_SECOND_LOADER_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_SECOND_SAVE_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.phase!=PR_NEXT_DISPATCH_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.runner.phase!=PR_RUNNER_NEXT_EMPTY||x->bp<56u)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_third_pair_start(&s->pair,b,n,c,e,cl,pl,x,max_iterations,&d->pair);return third_pair_return_after(s,d,st);
}
pr_second_dispatch_status pr_third_pair_return_resume(pr_third_pair_return_session*s,pr_third_pair_return_diagnostic*d){pr_second_dispatch_status st;
    if(!s||!d||s->phase!=PR_THIRD_PAIR_RETURN_WAITING)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_third_pair_resume(&s->pair,&d->pair);
    if(st==PR_SECOND_SLOT_ARGUMENT)return st;
    return third_pair_return_after(s,d,st);
}

void pr_third_value_session_init(pr_third_value_session*s){if(s){memset(s,0,sizeof *s);pr_third_pair_return_session_init(&s->pair);}}
static pr_second_dispatch_status third_value_after(pr_third_value_session*s,pr_third_value_diagnostic*d,pr_second_dispatch_status st){
    d->pair_status=st;if(st==PR_SECOND_SLOT_YIELD){s->phase=PR_THIRD_VALUE_WAITING;return st;}
    s->phase=PR_THIRD_VALUE_FINISHED;if(st!=PR_THIRD_SLOT_PAIR)return st;
    d->return_status=pr_u25_slot_value_return(s->bus,PR_BUS_RAM_SIZE,s->cpu,&s->target);
    return d->return_status==PR_290B_FIRST_DONE?PR_THIRD_SLOT_PAIR:PR_SECOND_SLOT_STOP;
}
pr_second_dispatch_status pr_third_value_start(pr_third_value_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_third_value_diagnostic*d){
    pr_second_dispatch_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_THIRD_VALUE_EMPTY||s->pair.phase!=PR_THIRD_PAIR_RETURN_EMPTY||s->pair.pair.phase!=PR_THIRD_PAIR_EMPTY||s->pair.pair.slot.phase!=PR_THIRD_SLOT_EMPTY||s->pair.pair.slot.branch.phase!=PR_THIRD_BRANCH_EMPTY||s->pair.pair.slot.branch.loader.phase!=PR_THIRD_LOADER_EMPTY||s->pair.pair.slot.branch.loader.save.phase!=PR_THIRD_SAVE_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.phase!=PR_THIRD_DISPATCH_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.phase!=PR_SECOND_RUNNER_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.phase!=PR_SECOND_DISPATCH_RETURN_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.phase!=PR_SECOND_HANDLER_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.phase!=PR_SECOND_STORE_RETURN_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.phase!=PR_SECOND_CLASS4_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.phase!=PR_SECOND_DISPATCH_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.phase!=PR_SECOND_CLASS_RETURN_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.phase!=PR_SECOND_CLASS_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.phase!=PR_SECOND_STORE_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.phase!=PR_SECOND_VALUE_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.phase!=PR_SECOND_PAIR_RETURN_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.phase!=PR_SECOND_PAIR_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.phase!=PR_SECOND_SLOT_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.phase!=PR_SECOND_BRANCH_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_SECOND_LOADER_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_SECOND_SAVE_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.phase!=PR_NEXT_DISPATCH_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.runner.phase!=PR_RUNNER_NEXT_EMPTY||x->bp<56u)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;s->target=(pr_290b_return_target){x->cs,0x02bfu,(uint16_t)(x->bp-0x1au),(uint16_t)(x->bp-0x16u),x->ss};
    st=pr_third_pair_return_start(&s->pair,b,n,c,e,cl,pl,x,max_iterations,&d->pair);return third_value_after(s,d,st);
}
pr_second_dispatch_status pr_third_value_resume(pr_third_value_session*s,pr_third_value_diagnostic*d){pr_second_dispatch_status st;
    if(!s||!d||s->phase!=PR_THIRD_VALUE_WAITING)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_third_pair_return_resume(&s->pair,&d->pair);
    if(st==PR_SECOND_SLOT_ARGUMENT)return st;
    return third_value_after(s,d,st);
}

void pr_third_store_session_init(pr_third_store_session*s){if(s){memset(s,0,sizeof *s);pr_third_value_session_init(&s->value);}}
static pr_second_dispatch_status third_store_after(pr_third_store_session*s,pr_third_store_diagnostic*d,pr_second_dispatch_status st){
    d->value_status=st;if(st==PR_SECOND_SLOT_YIELD){s->phase=PR_THIRD_STORE_WAITING;return st;}
    s->phase=PR_THIRD_STORE_FINISHED;if(st!=PR_THIRD_SLOT_PAIR)return st;
    d->entry_status=pr_u1c_save_store_entry(s->bus,PR_BUS_RAM_SIZE,s->cpu,&d->entry);
    return d->entry_status==PR_290B_FIRST_DONE?PR_THIRD_SLOT_PAIR:PR_SECOND_SLOT_STOP;
}
pr_second_dispatch_status pr_third_store_start(pr_third_store_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_third_store_diagnostic*d){
    pr_second_dispatch_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_THIRD_STORE_EMPTY||s->value.phase!=PR_THIRD_VALUE_EMPTY||s->value.pair.phase!=PR_THIRD_PAIR_RETURN_EMPTY||s->value.pair.pair.phase!=PR_THIRD_PAIR_EMPTY||s->value.pair.pair.slot.phase!=PR_THIRD_SLOT_EMPTY||s->value.pair.pair.slot.branch.phase!=PR_THIRD_BRANCH_EMPTY||s->value.pair.pair.slot.branch.loader.phase!=PR_THIRD_LOADER_EMPTY||s->value.pair.pair.slot.branch.loader.save.phase!=PR_THIRD_SAVE_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.phase!=PR_THIRD_DISPATCH_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.phase!=PR_SECOND_RUNNER_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.phase!=PR_SECOND_DISPATCH_RETURN_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.phase!=PR_SECOND_HANDLER_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.phase!=PR_SECOND_STORE_RETURN_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.phase!=PR_SECOND_CLASS4_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.phase!=PR_SECOND_DISPATCH_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.phase!=PR_SECOND_CLASS_RETURN_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.phase!=PR_SECOND_CLASS_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.phase!=PR_SECOND_STORE_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.phase!=PR_SECOND_VALUE_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.phase!=PR_SECOND_PAIR_RETURN_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.phase!=PR_SECOND_PAIR_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.phase!=PR_SECOND_SLOT_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.phase!=PR_SECOND_BRANCH_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_SECOND_LOADER_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_SECOND_SAVE_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.phase!=PR_NEXT_DISPATCH_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.runner.phase!=PR_RUNNER_NEXT_EMPTY||x->bp<56u)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_third_value_start(&s->value,b,n,c,e,cl,pl,x,max_iterations,&d->value);return third_store_after(s,d,st);
}
pr_second_dispatch_status pr_third_store_resume(pr_third_store_session*s,pr_third_store_diagnostic*d){pr_second_dispatch_status st;
    if(!s||!d||s->phase!=PR_THIRD_STORE_WAITING)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_third_value_resume(&s->value,&d->value);
    if(st==PR_SECOND_SLOT_ARGUMENT)return st;
    return third_store_after(s,d,st);
}

void pr_third_class_session_init(pr_third_class_session*s){if(s){memset(s,0,sizeof *s);pr_third_store_session_init(&s->store);}}
static pr_second_dispatch_status third_class_after(pr_third_class_session*s,pr_third_class_diagnostic*d,pr_second_dispatch_status st){
    d->store_status=st;if(st==PR_SECOND_SLOT_YIELD){s->phase=PR_THIRD_CLASS_WAITING;return st;}
    s->phase=PR_THIRD_CLASS_FINISHED;if(st!=PR_THIRD_SLOT_PAIR)return st;
    d->entry_status=pr_u25_store_classifier_entry(s->bus,PR_BUS_RAM_SIZE,s->cpu,&d->entry);
    return d->entry_status==PR_290B_FIRST_DONE?PR_THIRD_SLOT_PAIR:PR_SECOND_SLOT_STOP;
}
pr_second_dispatch_status pr_third_class_start(pr_third_class_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_third_class_diagnostic*d){
    pr_second_dispatch_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_THIRD_CLASS_EMPTY||s->store.phase!=PR_THIRD_STORE_EMPTY||s->store.value.phase!=PR_THIRD_VALUE_EMPTY||s->store.value.pair.phase!=PR_THIRD_PAIR_RETURN_EMPTY||s->store.value.pair.pair.phase!=PR_THIRD_PAIR_EMPTY||s->store.value.pair.pair.slot.phase!=PR_THIRD_SLOT_EMPTY||s->store.value.pair.pair.slot.branch.phase!=PR_THIRD_BRANCH_EMPTY||s->store.value.pair.pair.slot.branch.loader.phase!=PR_THIRD_LOADER_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.phase!=PR_THIRD_SAVE_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.phase!=PR_THIRD_DISPATCH_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.phase!=PR_SECOND_RUNNER_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.phase!=PR_SECOND_DISPATCH_RETURN_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.phase!=PR_SECOND_HANDLER_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.phase!=PR_SECOND_STORE_RETURN_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.phase!=PR_SECOND_CLASS4_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.phase!=PR_SECOND_DISPATCH_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.phase!=PR_SECOND_CLASS_RETURN_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.phase!=PR_SECOND_CLASS_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.phase!=PR_SECOND_STORE_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.phase!=PR_SECOND_VALUE_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.phase!=PR_SECOND_PAIR_RETURN_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.phase!=PR_SECOND_PAIR_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.phase!=PR_SECOND_SLOT_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.phase!=PR_SECOND_BRANCH_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_SECOND_LOADER_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_SECOND_SAVE_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.phase!=PR_NEXT_DISPATCH_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.runner.phase!=PR_RUNNER_NEXT_EMPTY||x->bp<56u)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_third_store_start(&s->store,b,n,c,e,cl,pl,x,max_iterations,&d->store);return third_class_after(s,d,st);
}
pr_second_dispatch_status pr_third_class_resume(pr_third_class_session*s,pr_third_class_diagnostic*d){pr_second_dispatch_status st;
    if(!s||!d||s->phase!=PR_THIRD_CLASS_WAITING)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_third_store_resume(&s->store,&d->store);
    if(st==PR_SECOND_SLOT_ARGUMENT)return st;
    return third_class_after(s,d,st);
}

void pr_third_class_return_session_init(pr_third_class_return_session*s){if(s){memset(s,0,sizeof *s);pr_third_class_session_init(&s->classifier);}}
static pr_second_dispatch_status third_class_return_after(pr_third_class_return_session*s,pr_third_class_return_diagnostic*d,pr_second_dispatch_status st){
    pr_290b_return_target x;d->classifier_status=st;if(st==PR_SECOND_SLOT_YIELD){s->phase=PR_THIRD_CLASS_RETURN_WAITING;return st;}
    s->phase=PR_THIRD_CLASS_RETURN_FINISHED;if(st!=PR_THIRD_SLOT_PAIR)return st;
    x=(pr_290b_return_target){s->cpu->cs,0x0c84u,(uint16_t)(s->cpu->sp+6u),s->cpu->bp,s->cpu->ss};
    d->return_status=pr_u25_classifier_return(s->bus,PR_BUS_RAM_SIZE,s->cpu,&x);
    return d->return_status==PR_290B_FIRST_DONE?PR_THIRD_SLOT_PAIR:PR_SECOND_SLOT_STOP;
}
pr_second_dispatch_status pr_third_class_return_start(pr_third_class_return_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_third_class_return_diagnostic*d){
    pr_second_dispatch_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_THIRD_CLASS_RETURN_EMPTY||s->classifier.phase!=PR_THIRD_CLASS_EMPTY||s->classifier.store.phase!=PR_THIRD_STORE_EMPTY||s->classifier.store.value.phase!=PR_THIRD_VALUE_EMPTY||s->classifier.store.value.pair.phase!=PR_THIRD_PAIR_RETURN_EMPTY||s->classifier.store.value.pair.pair.phase!=PR_THIRD_PAIR_EMPTY||s->classifier.store.value.pair.pair.slot.phase!=PR_THIRD_SLOT_EMPTY||s->classifier.store.value.pair.pair.slot.branch.phase!=PR_THIRD_BRANCH_EMPTY||s->classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_THIRD_LOADER_EMPTY||s->classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_THIRD_SAVE_EMPTY||s->classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.phase!=PR_THIRD_DISPATCH_EMPTY||s->classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.phase!=PR_SECOND_RUNNER_EMPTY||s->classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.phase!=PR_SECOND_DISPATCH_RETURN_EMPTY||s->classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.phase!=PR_SECOND_HANDLER_EMPTY||s->classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.phase!=PR_SECOND_STORE_RETURN_EMPTY||s->classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.phase!=PR_SECOND_CLASS4_EMPTY||s->classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.phase!=PR_SECOND_DISPATCH_EMPTY||s->classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.phase!=PR_SECOND_CLASS_RETURN_EMPTY||s->classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.phase!=PR_SECOND_CLASS_EMPTY||s->classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.phase!=PR_SECOND_STORE_EMPTY||s->classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.phase!=PR_SECOND_VALUE_EMPTY||s->classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.phase!=PR_SECOND_PAIR_RETURN_EMPTY||s->classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.phase!=PR_SECOND_PAIR_EMPTY||s->classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.phase!=PR_SECOND_SLOT_EMPTY||s->classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.phase!=PR_SECOND_BRANCH_EMPTY||s->classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_SECOND_LOADER_EMPTY||s->classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_SECOND_SAVE_EMPTY||s->classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.phase!=PR_NEXT_DISPATCH_EMPTY||s->classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.runner.phase!=PR_RUNNER_NEXT_EMPTY||x->bp<56u)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_third_class_start(&s->classifier,b,n,c,e,cl,pl,x,max_iterations,&d->classifier);return third_class_return_after(s,d,st);
}
pr_second_dispatch_status pr_third_class_return_resume(pr_third_class_return_session*s,pr_third_class_return_diagnostic*d){pr_second_dispatch_status st;
    if(!s||!d||s->phase!=PR_THIRD_CLASS_RETURN_WAITING)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_third_class_resume(&s->classifier,&d->classifier);
    if(st==PR_SECOND_SLOT_ARGUMENT)return st;
    return third_class_return_after(s,d,st);
}

void pr_third_store_dispatch_session_init(pr_third_store_dispatch_session*s){if(s){memset(s,0,sizeof *s);pr_third_class_return_session_init(&s->classifier);}}
static pr_second_dispatch_status third_store_dispatch_after(pr_third_store_dispatch_session*s,pr_third_store_dispatch_diagnostic*d,pr_second_dispatch_status st){
    d->classifier_status=st;if(st==PR_SECOND_SLOT_YIELD){s->phase=PR_THIRD_STORE_DISPATCH_WAITING;return st;}
    s->phase=PR_THIRD_STORE_DISPATCH_FINISHED;if(st!=PR_THIRD_SLOT_PAIR)return st;
    d->dispatch_status=pr_u25_store_class_dispatch(s->bus,PR_BUS_RAM_SIZE,s->cpu);
    if(d->dispatch_status!=PR_290B_FIRST_DONE)return PR_SECOND_SLOT_STOP;
    if(s->cpu->ip==0x0c8eu)return PR_THIRD_DISPATCH_CLASS0;
    if(s->cpu->ip==0x0cbbu)return PR_THIRD_DISPATCH_CLASS1;
    if(s->cpu->ip==0x0cdfu)return PR_THIRD_DISPATCH_CLASS2;
    if(s->cpu->ip==0x0cf9u)return PR_THIRD_DISPATCH_CLASS3;
    if(s->cpu->ip==0x0d14u)return PR_THIRD_DISPATCH_CLASS4;
    return PR_THIRD_DISPATCH_UNMATCHED;
}
pr_second_dispatch_status pr_third_store_dispatch_start(pr_third_store_dispatch_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_third_store_dispatch_diagnostic*d){
    pr_second_dispatch_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_THIRD_STORE_DISPATCH_EMPTY||s->classifier.phase!=PR_THIRD_CLASS_RETURN_EMPTY||s->classifier.classifier.phase!=PR_THIRD_CLASS_EMPTY||s->classifier.classifier.store.phase!=PR_THIRD_STORE_EMPTY||s->classifier.classifier.store.value.phase!=PR_THIRD_VALUE_EMPTY||s->classifier.classifier.store.value.pair.phase!=PR_THIRD_PAIR_RETURN_EMPTY||s->classifier.classifier.store.value.pair.pair.phase!=PR_THIRD_PAIR_EMPTY||s->classifier.classifier.store.value.pair.pair.slot.phase!=PR_THIRD_SLOT_EMPTY||s->classifier.classifier.store.value.pair.pair.slot.branch.phase!=PR_THIRD_BRANCH_EMPTY||s->classifier.classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_THIRD_LOADER_EMPTY||s->classifier.classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_THIRD_SAVE_EMPTY||s->classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.phase!=PR_THIRD_DISPATCH_EMPTY||s->classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.phase!=PR_SECOND_RUNNER_EMPTY||s->classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.phase!=PR_SECOND_DISPATCH_RETURN_EMPTY||s->classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.phase!=PR_SECOND_HANDLER_EMPTY||s->classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.phase!=PR_SECOND_STORE_RETURN_EMPTY||s->classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.phase!=PR_SECOND_CLASS4_EMPTY||s->classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.phase!=PR_SECOND_DISPATCH_EMPTY||s->classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.phase!=PR_SECOND_CLASS_RETURN_EMPTY||s->classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.phase!=PR_SECOND_CLASS_EMPTY||s->classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.phase!=PR_SECOND_STORE_EMPTY||s->classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.phase!=PR_SECOND_VALUE_EMPTY||s->classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.phase!=PR_SECOND_PAIR_RETURN_EMPTY||s->classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.phase!=PR_SECOND_PAIR_EMPTY||s->classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.phase!=PR_SECOND_SLOT_EMPTY||s->classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.phase!=PR_SECOND_BRANCH_EMPTY||s->classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_SECOND_LOADER_EMPTY||s->classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_SECOND_SAVE_EMPTY||s->classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.phase!=PR_NEXT_DISPATCH_EMPTY||s->classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.runner.phase!=PR_RUNNER_NEXT_EMPTY||x->bp<56u)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_third_class_return_start(&s->classifier,b,n,c,e,cl,pl,x,max_iterations,&d->classifier);return third_store_dispatch_after(s,d,st);
}
pr_second_dispatch_status pr_third_store_dispatch_resume(pr_third_store_dispatch_session*s,pr_third_store_dispatch_diagnostic*d){pr_second_dispatch_status st;
    if(!s||!d||s->phase!=PR_THIRD_STORE_DISPATCH_WAITING)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_third_class_return_resume(&s->classifier,&d->classifier);
    if(st==PR_SECOND_SLOT_ARGUMENT)return st;
    return third_store_dispatch_after(s,d,st);
}

void pr_third_class4_session_init(pr_third_class4_session*s){if(s){memset(s,0,sizeof *s);pr_third_store_dispatch_session_init(&s->dispatch);}}
static pr_second_dispatch_status third_class4_after(pr_third_class4_session*s,pr_third_class4_diagnostic*d,pr_second_dispatch_status st){
    d->dispatch_status=st;if(st==PR_SECOND_SLOT_YIELD){s->phase=PR_THIRD_CLASS4_WAITING;return st;}
    s->phase=PR_THIRD_CLASS4_FINISHED;if(st!=PR_THIRD_DISPATCH_CLASS4)return st;
    d->low_status=pr_u25_store_class4_low(s->bus,PR_BUS_RAM_SIZE,s->cpu);
    if(d->low_status==PR_290B_FIRST_DONE)return PR_THIRD_CLASS4_LOW_REACHED;
    if(d->low_status==PR_290B_FIRST_OUTSIDE)return PR_THIRD_CLASS4_LOW_HIGH;
    return PR_SECOND_SLOT_STOP;
}
pr_second_dispatch_status pr_third_class4_start(pr_third_class4_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_third_class4_diagnostic*d){
    pr_second_dispatch_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_THIRD_CLASS4_EMPTY||s->dispatch.phase!=PR_THIRD_STORE_DISPATCH_EMPTY||s->dispatch.classifier.phase!=PR_THIRD_CLASS_RETURN_EMPTY||s->dispatch.classifier.classifier.phase!=PR_THIRD_CLASS_EMPTY||s->dispatch.classifier.classifier.store.phase!=PR_THIRD_STORE_EMPTY||s->dispatch.classifier.classifier.store.value.phase!=PR_THIRD_VALUE_EMPTY||s->dispatch.classifier.classifier.store.value.pair.phase!=PR_THIRD_PAIR_RETURN_EMPTY||s->dispatch.classifier.classifier.store.value.pair.pair.phase!=PR_THIRD_PAIR_EMPTY||s->dispatch.classifier.classifier.store.value.pair.pair.slot.phase!=PR_THIRD_SLOT_EMPTY||s->dispatch.classifier.classifier.store.value.pair.pair.slot.branch.phase!=PR_THIRD_BRANCH_EMPTY||s->dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_THIRD_LOADER_EMPTY||s->dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_THIRD_SAVE_EMPTY||s->dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.phase!=PR_THIRD_DISPATCH_EMPTY||s->dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.phase!=PR_SECOND_RUNNER_EMPTY||s->dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.phase!=PR_SECOND_DISPATCH_RETURN_EMPTY||s->dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.phase!=PR_SECOND_HANDLER_EMPTY||s->dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.phase!=PR_SECOND_STORE_RETURN_EMPTY||s->dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.phase!=PR_SECOND_CLASS4_EMPTY||s->dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.phase!=PR_SECOND_DISPATCH_EMPTY||s->dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.phase!=PR_SECOND_CLASS_RETURN_EMPTY||s->dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.phase!=PR_SECOND_CLASS_EMPTY||s->dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.phase!=PR_SECOND_STORE_EMPTY||s->dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.phase!=PR_SECOND_VALUE_EMPTY||s->dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.phase!=PR_SECOND_PAIR_RETURN_EMPTY||s->dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.phase!=PR_SECOND_PAIR_EMPTY||s->dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.phase!=PR_SECOND_SLOT_EMPTY||s->dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.phase!=PR_SECOND_BRANCH_EMPTY||s->dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_SECOND_LOADER_EMPTY||s->dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_SECOND_SAVE_EMPTY||s->dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.phase!=PR_NEXT_DISPATCH_EMPTY||s->dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.runner.phase!=PR_RUNNER_NEXT_EMPTY||x->bp<56u)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_third_store_dispatch_start(&s->dispatch,b,n,c,e,cl,pl,x,max_iterations,&d->dispatch);return third_class4_after(s,d,st);
}
pr_second_dispatch_status pr_third_class4_resume(pr_third_class4_session*s,pr_third_class4_diagnostic*d){pr_second_dispatch_status st;
    if(!s||!d||s->phase!=PR_THIRD_CLASS4_WAITING)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_third_store_dispatch_resume(&s->dispatch,&d->dispatch);
    if(st==PR_SECOND_SLOT_ARGUMENT)return st;
    return third_class4_after(s,d,st);
}

void pr_third_store_return_session_init(pr_third_store_return_session*s){if(s){memset(s,0,sizeof *s);pr_third_class4_session_init(&s->low);}}
static pr_second_dispatch_status third_store_return_after(pr_third_store_return_session*s,pr_third_store_return_diagnostic*d,pr_second_dispatch_status st){
    d->low_status=st;if(st==PR_SECOND_SLOT_YIELD){s->phase=PR_THIRD_STORE_RETURN_WAITING;return st;}
    s->phase=PR_THIRD_STORE_RETURN_FINISHED;if(st!=PR_THIRD_CLASS4_LOW_REACHED)return st;
    d->return_status=pr_u25_store_return(s->bus,PR_BUS_RAM_SIZE,s->cpu,&s->target);
    return d->return_status==PR_290B_FIRST_DONE?PR_THIRD_STORE_RETURN_REACHED:PR_SECOND_SLOT_STOP;
}
pr_second_dispatch_status pr_third_store_return_start(pr_third_store_return_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_third_store_return_diagnostic*d){
    pr_second_dispatch_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_THIRD_STORE_RETURN_EMPTY||s->low.phase!=PR_THIRD_CLASS4_EMPTY||s->low.dispatch.phase!=PR_THIRD_STORE_DISPATCH_EMPTY||s->low.dispatch.classifier.phase!=PR_THIRD_CLASS_RETURN_EMPTY||s->low.dispatch.classifier.classifier.phase!=PR_THIRD_CLASS_EMPTY||s->low.dispatch.classifier.classifier.store.phase!=PR_THIRD_STORE_EMPTY||s->low.dispatch.classifier.classifier.store.value.phase!=PR_THIRD_VALUE_EMPTY||s->low.dispatch.classifier.classifier.store.value.pair.phase!=PR_THIRD_PAIR_RETURN_EMPTY||s->low.dispatch.classifier.classifier.store.value.pair.pair.phase!=PR_THIRD_PAIR_EMPTY||s->low.dispatch.classifier.classifier.store.value.pair.pair.slot.phase!=PR_THIRD_SLOT_EMPTY||s->low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.phase!=PR_THIRD_BRANCH_EMPTY||s->low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_THIRD_LOADER_EMPTY||s->low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_THIRD_SAVE_EMPTY||s->low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.phase!=PR_THIRD_DISPATCH_EMPTY||s->low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.phase!=PR_SECOND_RUNNER_EMPTY||s->low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.phase!=PR_SECOND_DISPATCH_RETURN_EMPTY||s->low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.phase!=PR_SECOND_HANDLER_EMPTY||s->low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.phase!=PR_SECOND_STORE_RETURN_EMPTY||s->low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.phase!=PR_SECOND_CLASS4_EMPTY||s->low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.phase!=PR_SECOND_DISPATCH_EMPTY||s->low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.phase!=PR_SECOND_CLASS_RETURN_EMPTY||s->low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.phase!=PR_SECOND_CLASS_EMPTY||s->low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.phase!=PR_SECOND_STORE_EMPTY||s->low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.phase!=PR_SECOND_VALUE_EMPTY||s->low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.phase!=PR_SECOND_PAIR_RETURN_EMPTY||s->low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.phase!=PR_SECOND_PAIR_EMPTY||s->low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.phase!=PR_SECOND_SLOT_EMPTY||s->low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.phase!=PR_SECOND_BRANCH_EMPTY||s->low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_SECOND_LOADER_EMPTY||s->low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_SECOND_SAVE_EMPTY||s->low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.phase!=PR_NEXT_DISPATCH_EMPTY||s->low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.runner.phase!=PR_RUNNER_NEXT_EMPTY||x->bp<56u)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;s->target=(pr_290b_return_target){x->cs,0x02cdu,(uint16_t)(x->bp-0x1au),(uint16_t)(x->bp-0x16u),x->ss};
    st=pr_third_class4_start(&s->low,b,n,c,e,cl,pl,x,max_iterations,&d->low);return third_store_return_after(s,d,st);
}
pr_second_dispatch_status pr_third_store_return_resume(pr_third_store_return_session*s,pr_third_store_return_diagnostic*d){pr_second_dispatch_status st;
    if(!s||!d||s->phase!=PR_THIRD_STORE_RETURN_WAITING)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_third_class4_resume(&s->low,&d->low);
    if(st==PR_SECOND_SLOT_ARGUMENT)return st;
    return third_store_return_after(s,d,st);
}

void pr_third_handler_session_init(pr_third_handler_session*s){if(s){memset(s,0,sizeof *s);pr_third_store_return_session_init(&s->store);}}
static pr_second_dispatch_status third_handler_after(pr_third_handler_session*s,pr_third_handler_diagnostic*d,pr_second_dispatch_status st){
    d->store_status=st;if(st==PR_SECOND_SLOT_YIELD){s->phase=PR_THIRD_HANDLER_WAITING;return st;}
    s->phase=PR_THIRD_HANDLER_FINISHED;if(st!=PR_THIRD_STORE_RETURN_REACHED)return st;
    d->return_status=pr_u1c_save_handler_return(s->bus,PR_BUS_RAM_SIZE,s->cpu,&s->target);
    return d->return_status==PR_290B_FIRST_DONE?PR_THIRD_HANDLER_RETURN_REACHED:PR_SECOND_SLOT_STOP;
}
pr_second_dispatch_status pr_third_handler_start(pr_third_handler_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_third_handler_diagnostic*d){
    pr_second_dispatch_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_THIRD_HANDLER_EMPTY||s->store.phase!=PR_THIRD_STORE_RETURN_EMPTY||s->store.low.phase!=PR_THIRD_CLASS4_EMPTY||s->store.low.dispatch.phase!=PR_THIRD_STORE_DISPATCH_EMPTY||s->store.low.dispatch.classifier.phase!=PR_THIRD_CLASS_RETURN_EMPTY||s->store.low.dispatch.classifier.classifier.phase!=PR_THIRD_CLASS_EMPTY||s->store.low.dispatch.classifier.classifier.store.phase!=PR_THIRD_STORE_EMPTY||s->store.low.dispatch.classifier.classifier.store.value.phase!=PR_THIRD_VALUE_EMPTY||s->store.low.dispatch.classifier.classifier.store.value.pair.phase!=PR_THIRD_PAIR_RETURN_EMPTY||s->store.low.dispatch.classifier.classifier.store.value.pair.pair.phase!=PR_THIRD_PAIR_EMPTY||s->store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.phase!=PR_THIRD_SLOT_EMPTY||s->store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.phase!=PR_THIRD_BRANCH_EMPTY||s->store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_THIRD_LOADER_EMPTY||s->store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_THIRD_SAVE_EMPTY||s->store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.phase!=PR_THIRD_DISPATCH_EMPTY||s->store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.phase!=PR_SECOND_RUNNER_EMPTY||s->store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.phase!=PR_SECOND_DISPATCH_RETURN_EMPTY||s->store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.phase!=PR_SECOND_HANDLER_EMPTY||s->store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.phase!=PR_SECOND_STORE_RETURN_EMPTY||s->store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.phase!=PR_SECOND_CLASS4_EMPTY||s->store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.phase!=PR_SECOND_DISPATCH_EMPTY||s->store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.phase!=PR_SECOND_CLASS_RETURN_EMPTY||s->store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.phase!=PR_SECOND_CLASS_EMPTY||s->store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.phase!=PR_SECOND_STORE_EMPTY||s->store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.phase!=PR_SECOND_VALUE_EMPTY||s->store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.phase!=PR_SECOND_PAIR_RETURN_EMPTY||s->store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.phase!=PR_SECOND_PAIR_EMPTY||s->store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.phase!=PR_SECOND_SLOT_EMPTY||s->store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.phase!=PR_SECOND_BRANCH_EMPTY||s->store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_SECOND_LOADER_EMPTY||s->store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_SECOND_SAVE_EMPTY||s->store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.phase!=PR_NEXT_DISPATCH_EMPTY||s->store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.runner.phase!=PR_RUNNER_NEXT_EMPTY||x->bp<56u)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;s->target=(pr_290b_return_target){x->cs,0x2f89u,(uint16_t)(x->bp-0x10u),(uint16_t)(x->bp-0x10u),x->ss};
    st=pr_third_store_return_start(&s->store,b,n,c,e,cl,pl,x,max_iterations,&d->store);return third_handler_after(s,d,st);
}
pr_second_dispatch_status pr_third_handler_resume(pr_third_handler_session*s,pr_third_handler_diagnostic*d){pr_second_dispatch_status st;
    if(!s||!d||s->phase!=PR_THIRD_HANDLER_WAITING)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_third_store_return_resume(&s->store,&d->store);
    if(st==PR_SECOND_SLOT_ARGUMENT)return st;
    return third_handler_after(s,d,st);
}

void pr_third_dispatch_return_session_init(pr_third_dispatch_return_session*s){if(s){memset(s,0,sizeof *s);pr_third_handler_session_init(&s->handler);}}
static pr_second_dispatch_status third_dispatch_return_after(pr_third_dispatch_return_session*s,pr_third_dispatch_return_diagnostic*d,pr_second_dispatch_status st){
    d->handler_status=st;if(st==PR_SECOND_SLOT_YIELD){s->phase=PR_THIRD_DISPATCH_RETURN_WAITING;return st;}
    s->phase=PR_THIRD_DISPATCH_RETURN_FINISHED;if(st!=PR_THIRD_HANDLER_RETURN_REACHED)return st;
    d->return_status=pr_u1c_save_dispatch_return(s->bus,PR_BUS_RAM_SIZE,s->cpu,&s->target);
    return d->return_status==PR_290B_FIRST_DONE?PR_THIRD_DISPATCH_RETURN_REACHED:PR_SECOND_SLOT_STOP;
}
pr_second_dispatch_status pr_third_dispatch_return_start(pr_third_dispatch_return_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_third_dispatch_return_diagnostic*d){
    pr_second_dispatch_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_THIRD_DISPATCH_RETURN_EMPTY||s->handler.phase!=PR_THIRD_HANDLER_EMPTY||s->handler.store.phase!=PR_THIRD_STORE_RETURN_EMPTY||s->handler.store.low.phase!=PR_THIRD_CLASS4_EMPTY||s->handler.store.low.dispatch.phase!=PR_THIRD_STORE_DISPATCH_EMPTY||s->handler.store.low.dispatch.classifier.phase!=PR_THIRD_CLASS_RETURN_EMPTY||s->handler.store.low.dispatch.classifier.classifier.phase!=PR_THIRD_CLASS_EMPTY||s->handler.store.low.dispatch.classifier.classifier.store.phase!=PR_THIRD_STORE_EMPTY||s->handler.store.low.dispatch.classifier.classifier.store.value.phase!=PR_THIRD_VALUE_EMPTY||s->handler.store.low.dispatch.classifier.classifier.store.value.pair.phase!=PR_THIRD_PAIR_RETURN_EMPTY||s->handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.phase!=PR_THIRD_PAIR_EMPTY||s->handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.phase!=PR_THIRD_SLOT_EMPTY||s->handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.phase!=PR_THIRD_BRANCH_EMPTY||s->handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_THIRD_LOADER_EMPTY||s->handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_THIRD_SAVE_EMPTY||s->handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.phase!=PR_THIRD_DISPATCH_EMPTY||s->handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.phase!=PR_SECOND_RUNNER_EMPTY||s->handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.phase!=PR_SECOND_DISPATCH_RETURN_EMPTY||s->handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.phase!=PR_SECOND_HANDLER_EMPTY||s->handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.phase!=PR_SECOND_STORE_RETURN_EMPTY||s->handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.phase!=PR_SECOND_CLASS4_EMPTY||s->handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.phase!=PR_SECOND_DISPATCH_EMPTY||s->handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.phase!=PR_SECOND_CLASS_RETURN_EMPTY||s->handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.phase!=PR_SECOND_CLASS_EMPTY||s->handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.phase!=PR_SECOND_STORE_EMPTY||s->handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.phase!=PR_SECOND_VALUE_EMPTY||s->handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.phase!=PR_SECOND_PAIR_RETURN_EMPTY||s->handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.phase!=PR_SECOND_PAIR_EMPTY||s->handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.phase!=PR_SECOND_SLOT_EMPTY||s->handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.phase!=PR_SECOND_BRANCH_EMPTY||s->handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_SECOND_LOADER_EMPTY||s->handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_SECOND_SAVE_EMPTY||s->handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.phase!=PR_NEXT_DISPATCH_EMPTY||s->handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.runner.phase!=PR_RUNNER_NEXT_EMPTY||x->bp<56u)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;s->target=(pr_290b_return_target){x->cs,0x31a8u,(uint16_t)(x->bp-0x0au),(uint16_t)(x->bp-0x0au),x->ss};
    st=pr_third_handler_start(&s->handler,b,n,c,e,cl,pl,x,max_iterations,&d->handler);return third_dispatch_return_after(s,d,st);
}
pr_second_dispatch_status pr_third_dispatch_return_resume(pr_third_dispatch_return_session*s,pr_third_dispatch_return_diagnostic*d){pr_second_dispatch_status st;
    if(!s||!d||s->phase!=PR_THIRD_DISPATCH_RETURN_WAITING)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_third_handler_resume(&s->handler,&d->handler);
    if(st==PR_SECOND_SLOT_ARGUMENT)return st;
    return third_dispatch_return_after(s,d,st);
}

void pr_third_runner_session_init(pr_third_runner_session*s){if(s){memset(s,0,sizeof *s);pr_third_dispatch_return_session_init(&s->dispatch);}}
static pr_second_dispatch_status third_runner_after(pr_third_runner_session*s,pr_third_runner_diagnostic*d,pr_second_dispatch_status st){
    d->dispatch_status=st;if(st==PR_SECOND_SLOT_YIELD){s->phase=PR_THIRD_RUNNER_WAITING;return st;}
    s->phase=PR_THIRD_RUNNER_FINISHED;if(st!=PR_THIRD_DISPATCH_RETURN_REACHED)return st;
    d->next_status=pr_u1c_runner_next(s->bus,PR_BUS_RAM_SIZE,s->cpu,&s->target);
    if(d->next_status==PR_290B_FIRST_OUTSIDE)return PR_THIRD_RUNNER_DISPATCH;
    if(d->next_status==PR_290B_FIRST_DONE)return PR_THIRD_RUNNER_RETURNED;
    return PR_SECOND_SLOT_STOP;
}
pr_second_dispatch_status pr_third_runner_start(pr_third_runner_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_third_runner_diagnostic*d){
    pr_second_dispatch_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_THIRD_RUNNER_EMPTY||s->dispatch.phase!=PR_THIRD_DISPATCH_RETURN_EMPTY||s->dispatch.handler.phase!=PR_THIRD_HANDLER_EMPTY||s->dispatch.handler.store.phase!=PR_THIRD_STORE_RETURN_EMPTY||s->dispatch.handler.store.low.phase!=PR_THIRD_CLASS4_EMPTY||s->dispatch.handler.store.low.dispatch.phase!=PR_THIRD_STORE_DISPATCH_EMPTY||s->dispatch.handler.store.low.dispatch.classifier.phase!=PR_THIRD_CLASS_RETURN_EMPTY||s->dispatch.handler.store.low.dispatch.classifier.classifier.phase!=PR_THIRD_CLASS_EMPTY||s->dispatch.handler.store.low.dispatch.classifier.classifier.store.phase!=PR_THIRD_STORE_EMPTY||s->dispatch.handler.store.low.dispatch.classifier.classifier.store.value.phase!=PR_THIRD_VALUE_EMPTY||s->dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.phase!=PR_THIRD_PAIR_RETURN_EMPTY||s->dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.phase!=PR_THIRD_PAIR_EMPTY||s->dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.phase!=PR_THIRD_SLOT_EMPTY||s->dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.phase!=PR_THIRD_BRANCH_EMPTY||s->dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_THIRD_LOADER_EMPTY||s->dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_THIRD_SAVE_EMPTY||s->dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.phase!=PR_THIRD_DISPATCH_EMPTY||s->dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.phase!=PR_SECOND_RUNNER_EMPTY||s->dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.phase!=PR_SECOND_DISPATCH_RETURN_EMPTY||s->dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.phase!=PR_SECOND_HANDLER_EMPTY||s->dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.phase!=PR_SECOND_STORE_RETURN_EMPTY||s->dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.phase!=PR_SECOND_CLASS4_EMPTY||s->dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.phase!=PR_SECOND_DISPATCH_EMPTY||s->dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.phase!=PR_SECOND_CLASS_RETURN_EMPTY||s->dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.phase!=PR_SECOND_CLASS_EMPTY||s->dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.phase!=PR_SECOND_STORE_EMPTY||s->dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.phase!=PR_SECOND_VALUE_EMPTY||s->dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.phase!=PR_SECOND_PAIR_RETURN_EMPTY||s->dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.phase!=PR_SECOND_PAIR_EMPTY||s->dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.phase!=PR_SECOND_SLOT_EMPTY||s->dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.phase!=PR_SECOND_BRANCH_EMPTY||s->dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_SECOND_LOADER_EMPTY||s->dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_SECOND_SAVE_EMPTY||s->dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.phase!=PR_NEXT_DISPATCH_EMPTY||s->dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.runner.phase!=PR_RUNNER_NEXT_EMPTY||x->bp<56u)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;s->target=*x;s->target.ip=0x345bu;
    st=pr_third_dispatch_return_start(&s->dispatch,b,n,c,e,cl,pl,x,max_iterations,&d->dispatch);return third_runner_after(s,d,st);
}
pr_second_dispatch_status pr_third_runner_resume(pr_third_runner_session*s,pr_third_runner_diagnostic*d){pr_second_dispatch_status st;
    if(!s||!d||s->phase!=PR_THIRD_RUNNER_WAITING)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_third_dispatch_return_resume(&s->dispatch,&d->dispatch);
    if(st==PR_SECOND_SLOT_ARGUMENT)return st;
    return third_runner_after(s,d,st);
}

void pr_fourth_dispatch_session_init(pr_fourth_dispatch_session*s){if(s){memset(s,0,sizeof *s);pr_third_runner_session_init(&s->runner);}}
static pr_second_dispatch_status fourth_dispatch_after(pr_fourth_dispatch_session*s,pr_fourth_dispatch_diagnostic*d,pr_second_dispatch_status st){
    d->runner_status=st;if(st==PR_SECOND_SLOT_YIELD){s->phase=PR_FOURTH_DISPATCH_WAITING;return st;}
    s->phase=PR_FOURTH_DISPATCH_FINISHED;if(st!=PR_THIRD_RUNNER_DISPATCH)return st;
    d->dispatch_status=pr_u1c_ecl_dispatch_prefix(s->bus,PR_BUS_RAM_SIZE,s->cpu);
    if(d->dispatch_status==PR_290B_FIRST_OUTSIDE)return PR_FOURTH_DISPATCH_LATER;
    return d->dispatch_status==PR_290B_FIRST_DONE?PR_FOURTH_DISPATCH_HANDLER:PR_SECOND_SLOT_STOP;
}
pr_second_dispatch_status pr_fourth_dispatch_start(pr_fourth_dispatch_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_fourth_dispatch_diagnostic*d){
    pr_second_dispatch_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_FOURTH_DISPATCH_EMPTY||s->runner.phase!=PR_THIRD_RUNNER_EMPTY||s->runner.dispatch.phase!=PR_THIRD_DISPATCH_RETURN_EMPTY||s->runner.dispatch.handler.phase!=PR_THIRD_HANDLER_EMPTY||s->runner.dispatch.handler.store.phase!=PR_THIRD_STORE_RETURN_EMPTY||s->runner.dispatch.handler.store.low.phase!=PR_THIRD_CLASS4_EMPTY||s->runner.dispatch.handler.store.low.dispatch.phase!=PR_THIRD_STORE_DISPATCH_EMPTY||s->runner.dispatch.handler.store.low.dispatch.classifier.phase!=PR_THIRD_CLASS_RETURN_EMPTY||s->runner.dispatch.handler.store.low.dispatch.classifier.classifier.phase!=PR_THIRD_CLASS_EMPTY||s->runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.phase!=PR_THIRD_STORE_EMPTY||s->runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.phase!=PR_THIRD_VALUE_EMPTY||s->runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.phase!=PR_THIRD_PAIR_RETURN_EMPTY||s->runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.phase!=PR_THIRD_PAIR_EMPTY||s->runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.phase!=PR_THIRD_SLOT_EMPTY||s->runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.phase!=PR_THIRD_BRANCH_EMPTY||s->runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_THIRD_LOADER_EMPTY||s->runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_THIRD_SAVE_EMPTY||s->runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.phase!=PR_THIRD_DISPATCH_EMPTY||s->runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.phase!=PR_SECOND_RUNNER_EMPTY||s->runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.phase!=PR_SECOND_DISPATCH_RETURN_EMPTY||s->runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.phase!=PR_SECOND_HANDLER_EMPTY||s->runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.phase!=PR_SECOND_STORE_RETURN_EMPTY||s->runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.phase!=PR_SECOND_CLASS4_EMPTY||s->runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.phase!=PR_SECOND_DISPATCH_EMPTY||s->runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.phase!=PR_SECOND_CLASS_RETURN_EMPTY||s->runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.phase!=PR_SECOND_CLASS_EMPTY||s->runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.phase!=PR_SECOND_STORE_EMPTY||s->runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.phase!=PR_SECOND_VALUE_EMPTY||s->runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.phase!=PR_SECOND_PAIR_RETURN_EMPTY||s->runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.phase!=PR_SECOND_PAIR_EMPTY||s->runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.phase!=PR_SECOND_SLOT_EMPTY||s->runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.phase!=PR_SECOND_BRANCH_EMPTY||s->runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_SECOND_LOADER_EMPTY||s->runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_SECOND_SAVE_EMPTY||s->runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.phase!=PR_NEXT_DISPATCH_EMPTY||s->runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.runner.phase!=PR_RUNNER_NEXT_EMPTY||x->bp<56u)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_third_runner_start(&s->runner,b,n,c,e,cl,pl,x,max_iterations,&d->runner);return fourth_dispatch_after(s,d,st);
}
pr_second_dispatch_status pr_fourth_dispatch_resume(pr_fourth_dispatch_session*s,pr_fourth_dispatch_diagnostic*d){pr_second_dispatch_status st;
    if(!s||!d||s->phase!=PR_FOURTH_DISPATCH_WAITING)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_third_runner_resume(&s->runner,&d->runner);
    if(st==PR_SECOND_SLOT_ARGUMENT)return st;
    return fourth_dispatch_after(s,d,st);
}

void pr_fourth_save_session_init(pr_fourth_save_session*s){if(s){memset(s,0,sizeof *s);pr_fourth_dispatch_session_init(&s->dispatch);}}
static pr_second_dispatch_status fourth_save_after(pr_fourth_save_session*s,pr_fourth_save_diagnostic*d,pr_second_dispatch_status st){
    d->dispatch_status=st;if(st==PR_SECOND_SLOT_YIELD){s->phase=PR_FOURTH_SAVE_WAITING;return st;}
    s->phase=PR_FOURTH_SAVE_FINISHED;if(st!=PR_FOURTH_DISPATCH_HANDLER)return st;
    if(s->cpu->ip!=0x0292u)return PR_FOURTH_SAVE_OTHER_HANDLER;
    d->entry_status=pr_u1c_save_loader_entry(s->bus,PR_BUS_RAM_SIZE,s->cpu,&d->entry);
    return d->entry_status==PR_290B_FIRST_DONE?PR_FOURTH_SAVE_LOADER:PR_SECOND_SLOT_STOP;
}
pr_second_dispatch_status pr_fourth_save_start(pr_fourth_save_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_fourth_save_diagnostic*d){
    pr_second_dispatch_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_FOURTH_SAVE_EMPTY||s->dispatch.phase!=PR_FOURTH_DISPATCH_EMPTY||s->dispatch.runner.phase!=PR_THIRD_RUNNER_EMPTY||s->dispatch.runner.dispatch.phase!=PR_THIRD_DISPATCH_RETURN_EMPTY||s->dispatch.runner.dispatch.handler.phase!=PR_THIRD_HANDLER_EMPTY||s->dispatch.runner.dispatch.handler.store.phase!=PR_THIRD_STORE_RETURN_EMPTY||s->dispatch.runner.dispatch.handler.store.low.phase!=PR_THIRD_CLASS4_EMPTY||s->dispatch.runner.dispatch.handler.store.low.dispatch.phase!=PR_THIRD_STORE_DISPATCH_EMPTY||s->dispatch.runner.dispatch.handler.store.low.dispatch.classifier.phase!=PR_THIRD_CLASS_RETURN_EMPTY||s->dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.phase!=PR_THIRD_CLASS_EMPTY||s->dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.phase!=PR_THIRD_STORE_EMPTY||s->dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.phase!=PR_THIRD_VALUE_EMPTY||s->dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.phase!=PR_THIRD_PAIR_RETURN_EMPTY||s->dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.phase!=PR_THIRD_PAIR_EMPTY||s->dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.phase!=PR_THIRD_SLOT_EMPTY||s->dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.phase!=PR_THIRD_BRANCH_EMPTY||s->dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_THIRD_LOADER_EMPTY||s->dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_THIRD_SAVE_EMPTY||s->dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.phase!=PR_THIRD_DISPATCH_EMPTY||s->dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.phase!=PR_SECOND_RUNNER_EMPTY||s->dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.phase!=PR_SECOND_DISPATCH_RETURN_EMPTY||s->dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.phase!=PR_SECOND_HANDLER_EMPTY||s->dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.phase!=PR_SECOND_STORE_RETURN_EMPTY||s->dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.phase!=PR_SECOND_CLASS4_EMPTY||s->dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.phase!=PR_SECOND_DISPATCH_EMPTY||s->dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.phase!=PR_SECOND_CLASS_RETURN_EMPTY||s->dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.phase!=PR_SECOND_CLASS_EMPTY||s->dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.phase!=PR_SECOND_STORE_EMPTY||s->dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.phase!=PR_SECOND_VALUE_EMPTY||s->dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.phase!=PR_SECOND_PAIR_RETURN_EMPTY||s->dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.phase!=PR_SECOND_PAIR_EMPTY||s->dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.phase!=PR_SECOND_SLOT_EMPTY||s->dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.phase!=PR_SECOND_BRANCH_EMPTY||s->dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_SECOND_LOADER_EMPTY||s->dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_SECOND_SAVE_EMPTY||s->dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.phase!=PR_NEXT_DISPATCH_EMPTY||s->dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.runner.phase!=PR_RUNNER_NEXT_EMPTY||x->bp<56u)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_fourth_dispatch_start(&s->dispatch,b,n,c,e,cl,pl,x,max_iterations,&d->dispatch);return fourth_save_after(s,d,st);
}
pr_second_dispatch_status pr_fourth_save_resume(pr_fourth_save_session*s,pr_fourth_save_diagnostic*d){pr_second_dispatch_status st;
    if(!s||!d||s->phase!=PR_FOURTH_SAVE_WAITING)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_fourth_dispatch_resume(&s->dispatch,&d->dispatch);
    if(st==PR_SECOND_SLOT_ARGUMENT)return st;
    return fourth_save_after(s,d,st);
}

void pr_fourth_loader_session_init(pr_fourth_loader_session*s){if(s){memset(s,0,sizeof *s);pr_fourth_save_session_init(&s->save);}}
static pr_second_dispatch_status fourth_loader_after(pr_fourth_loader_session*s,pr_fourth_loader_diagnostic*d,pr_second_dispatch_status st){
    d->save_status=st;if(st==PR_SECOND_SLOT_YIELD){s->phase=PR_FOURTH_LOADER_WAITING;return st;}
    s->phase=PR_FOURTH_LOADER_FINISHED;if(st!=PR_FOURTH_SAVE_LOADER)return st;
    d->prefix_status=pr_u25_operand_prefix(s->bus,PR_BUS_RAM_SIZE,s->cpu);
    if(d->prefix_status==PR_290B_FIRST_DONE){
        d->tag_status=pr_u25_operand_tag(s->bus,PR_BUS_RAM_SIZE,s->cpu);
        if(d->tag_status==PR_290B_FIRST_OUTSIDE)return PR_FOURTH_LOADER_STRING;
        if(d->tag_status!=PR_290B_FIRST_DONE)return PR_SECOND_SLOT_STOP;
        d->loop_status=pr_u25_numeric_loop(s->bus,PR_BUS_RAM_SIZE,s->cpu,s->max_iterations,&d->loop);
        if(d->loop_status==PR_U25_LOOP_STEP_STRING)return PR_FOURTH_LOADER_STRING;
        if(d->loop_status==PR_U25_LOOP_STEP_LIMIT)return PR_FOURTH_LOADER_LIMIT;
        if(d->loop_status!=PR_U25_LOOP_STEP_DONE)return PR_SECOND_SLOT_STOP;
    }else if(d->prefix_status!=PR_290B_FIRST_OUTSIDE||s->cpu->ip!=0x012cu)return PR_SECOND_SLOT_STOP;
    d->return_status=pr_u25_operand_return(s->bus,PR_BUS_RAM_SIZE,s->cpu,&s->target);
    return d->return_status==PR_290B_FIRST_DONE?PR_FOURTH_LOADER_REACHED:PR_SECOND_SLOT_STOP;
}
pr_second_dispatch_status pr_fourth_loader_start(pr_fourth_loader_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_fourth_loader_diagnostic*d){
    pr_second_dispatch_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_FOURTH_LOADER_EMPTY||s->save.phase!=PR_FOURTH_SAVE_EMPTY||s->save.dispatch.phase!=PR_FOURTH_DISPATCH_EMPTY||s->save.dispatch.runner.phase!=PR_THIRD_RUNNER_EMPTY||s->save.dispatch.runner.dispatch.phase!=PR_THIRD_DISPATCH_RETURN_EMPTY||s->save.dispatch.runner.dispatch.handler.phase!=PR_THIRD_HANDLER_EMPTY||s->save.dispatch.runner.dispatch.handler.store.phase!=PR_THIRD_STORE_RETURN_EMPTY||s->save.dispatch.runner.dispatch.handler.store.low.phase!=PR_THIRD_CLASS4_EMPTY||s->save.dispatch.runner.dispatch.handler.store.low.dispatch.phase!=PR_THIRD_STORE_DISPATCH_EMPTY||s->save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.phase!=PR_THIRD_CLASS_RETURN_EMPTY||s->save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.phase!=PR_THIRD_CLASS_EMPTY||s->save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.phase!=PR_THIRD_STORE_EMPTY||s->save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.phase!=PR_THIRD_VALUE_EMPTY||s->save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.phase!=PR_THIRD_PAIR_RETURN_EMPTY||s->save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.phase!=PR_THIRD_PAIR_EMPTY||s->save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.phase!=PR_THIRD_SLOT_EMPTY||s->save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.phase!=PR_THIRD_BRANCH_EMPTY||s->save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_THIRD_LOADER_EMPTY||s->save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_THIRD_SAVE_EMPTY||s->save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.phase!=PR_THIRD_DISPATCH_EMPTY||s->save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.phase!=PR_SECOND_RUNNER_EMPTY||s->save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.phase!=PR_SECOND_DISPATCH_RETURN_EMPTY||s->save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.phase!=PR_SECOND_HANDLER_EMPTY||s->save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.phase!=PR_SECOND_STORE_RETURN_EMPTY||s->save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.phase!=PR_SECOND_CLASS4_EMPTY||s->save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.phase!=PR_SECOND_DISPATCH_EMPTY||s->save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.phase!=PR_SECOND_CLASS_RETURN_EMPTY||s->save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.phase!=PR_SECOND_CLASS_EMPTY||s->save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.phase!=PR_SECOND_STORE_EMPTY||s->save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.phase!=PR_SECOND_VALUE_EMPTY||s->save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.phase!=PR_SECOND_PAIR_RETURN_EMPTY||s->save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.phase!=PR_SECOND_PAIR_EMPTY||s->save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.phase!=PR_SECOND_SLOT_EMPTY||s->save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.phase!=PR_SECOND_BRANCH_EMPTY||s->save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_SECOND_LOADER_EMPTY||s->save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_SECOND_SAVE_EMPTY||s->save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.phase!=PR_NEXT_DISPATCH_EMPTY||s->save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.runner.phase!=PR_RUNNER_NEXT_EMPTY||x->bp<56u)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;s->max_iterations=max_iterations;s->target=(pr_290b_return_target){x->cs,0x02a0u,(uint16_t)(x->bp-0x1au),(uint16_t)(x->bp-0x16u),x->ss};
    st=pr_fourth_save_start(&s->save,b,n,c,e,cl,pl,x,max_iterations,&d->save);return fourth_loader_after(s,d,st);
}
pr_second_dispatch_status pr_fourth_loader_resume(pr_fourth_loader_session*s,pr_fourth_loader_diagnostic*d){pr_second_dispatch_status st;
    if(!s||!d||s->phase!=PR_FOURTH_LOADER_WAITING)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_fourth_save_resume(&s->save,&d->save);
    if(st==PR_SECOND_SLOT_ARGUMENT)return st;
    return fourth_loader_after(s,d,st);
}

void pr_fourth_branch_session_init(pr_fourth_branch_session*s){if(s){memset(s,0,sizeof *s);pr_fourth_loader_session_init(&s->loader);}}
static pr_second_dispatch_status fourth_branch_after(pr_fourth_branch_session*s,pr_fourth_branch_diagnostic*d,pr_second_dispatch_status st){
    pr_290b_return_target x;d->loader_status=st;if(st==PR_SECOND_SLOT_YIELD){s->phase=PR_FOURTH_BRANCH_WAITING;return st;}
    s->phase=PR_FOURTH_BRANCH_FINISHED;if(st!=PR_FOURTH_LOADER_REACHED)return st;
    x=(pr_290b_return_target){s->cpu->cs,0x02adu,s->cpu->sp,s->cpu->bp,s->cpu->ss};
    d->entry_status=pr_u1c_save_word_entry(s->bus,PR_BUS_RAM_SIZE,s->cpu,&d->entry);
    if(d->entry_status!=PR_290B_FIRST_DONE)return PR_SECOND_SLOT_STOP;
    d->word_status=pr_u25_word_return(s->bus,PR_BUS_RAM_SIZE,s->cpu,&x);
    if(d->word_status!=PR_290B_FIRST_DONE)return PR_SECOND_SLOT_STOP;
    d->branch_status=pr_u1c_save_tag_branch(s->bus,PR_BUS_RAM_SIZE,s->cpu);
    if(d->branch_status!=PR_290B_FIRST_DONE)return PR_SECOND_SLOT_STOP;
    return s->cpu->ip==0x02cfu?PR_FOURTH_BRANCH_SAVE_STRING:PR_FOURTH_BRANCH_NUMERIC;
}
pr_second_dispatch_status pr_fourth_branch_start(pr_fourth_branch_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_fourth_branch_diagnostic*d){
    pr_second_dispatch_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_FOURTH_BRANCH_EMPTY||s->loader.phase!=PR_FOURTH_LOADER_EMPTY||s->loader.save.phase!=PR_FOURTH_SAVE_EMPTY||s->loader.save.dispatch.phase!=PR_FOURTH_DISPATCH_EMPTY||s->loader.save.dispatch.runner.phase!=PR_THIRD_RUNNER_EMPTY||s->loader.save.dispatch.runner.dispatch.phase!=PR_THIRD_DISPATCH_RETURN_EMPTY||s->loader.save.dispatch.runner.dispatch.handler.phase!=PR_THIRD_HANDLER_EMPTY||s->loader.save.dispatch.runner.dispatch.handler.store.phase!=PR_THIRD_STORE_RETURN_EMPTY||s->loader.save.dispatch.runner.dispatch.handler.store.low.phase!=PR_THIRD_CLASS4_EMPTY||s->loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.phase!=PR_THIRD_STORE_DISPATCH_EMPTY||s->loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.phase!=PR_THIRD_CLASS_RETURN_EMPTY||s->loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.phase!=PR_THIRD_CLASS_EMPTY||s->loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.phase!=PR_THIRD_STORE_EMPTY||s->loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.phase!=PR_THIRD_VALUE_EMPTY||s->loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.phase!=PR_THIRD_PAIR_RETURN_EMPTY||s->loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.phase!=PR_THIRD_PAIR_EMPTY||s->loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.phase!=PR_THIRD_SLOT_EMPTY||s->loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.phase!=PR_THIRD_BRANCH_EMPTY||s->loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_THIRD_LOADER_EMPTY||s->loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_THIRD_SAVE_EMPTY||s->loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.phase!=PR_THIRD_DISPATCH_EMPTY||s->loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.phase!=PR_SECOND_RUNNER_EMPTY||s->loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.phase!=PR_SECOND_DISPATCH_RETURN_EMPTY||s->loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.phase!=PR_SECOND_HANDLER_EMPTY||s->loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.phase!=PR_SECOND_STORE_RETURN_EMPTY||s->loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.phase!=PR_SECOND_CLASS4_EMPTY||s->loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.phase!=PR_SECOND_DISPATCH_EMPTY||s->loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.phase!=PR_SECOND_CLASS_RETURN_EMPTY||s->loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.phase!=PR_SECOND_CLASS_EMPTY||s->loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.phase!=PR_SECOND_STORE_EMPTY||s->loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.phase!=PR_SECOND_VALUE_EMPTY||s->loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.phase!=PR_SECOND_PAIR_RETURN_EMPTY||s->loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.phase!=PR_SECOND_PAIR_EMPTY||s->loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.phase!=PR_SECOND_SLOT_EMPTY||s->loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.phase!=PR_SECOND_BRANCH_EMPTY||s->loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_SECOND_LOADER_EMPTY||s->loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_SECOND_SAVE_EMPTY||s->loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.phase!=PR_NEXT_DISPATCH_EMPTY||s->loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.runner.phase!=PR_RUNNER_NEXT_EMPTY||x->bp<56u)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_fourth_loader_start(&s->loader,b,n,c,e,cl,pl,x,max_iterations,&d->loader);return fourth_branch_after(s,d,st);
}
pr_second_dispatch_status pr_fourth_branch_resume(pr_fourth_branch_session*s,pr_fourth_branch_diagnostic*d){pr_second_dispatch_status st;
    if(!s||!d||s->phase!=PR_FOURTH_BRANCH_WAITING)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_fourth_loader_resume(&s->loader,&d->loader);
    if(st==PR_SECOND_SLOT_ARGUMENT)return st;
    return fourth_branch_after(s,d,st);
}

void pr_fourth_slot_session_init(pr_fourth_slot_session*s){if(s){memset(s,0,sizeof *s);pr_fourth_branch_session_init(&s->branch);}}
static pr_second_dispatch_status fourth_slot_after(pr_fourth_slot_session*s,pr_fourth_slot_diagnostic*d,pr_second_dispatch_status st){
    d->branch_status=st;if(st==PR_SECOND_SLOT_YIELD){s->phase=PR_FOURTH_SLOT_WAITING;return st;}
    s->phase=PR_FOURTH_SLOT_FINISHED;if(st!=PR_FOURTH_BRANCH_NUMERIC)return st;
    d->entry_status=pr_u1c_save_slot_entry(s->bus,PR_BUS_RAM_SIZE,s->cpu,&d->entry);
    if(d->entry_status!=PR_290B_FIRST_DONE)return PR_SECOND_SLOT_STOP;
    d->prefix_status=pr_u25_slot_prefix(s->bus,PR_BUS_RAM_SIZE,s->cpu);
    if(d->prefix_status!=PR_290B_FIRST_DONE)return PR_SECOND_SLOT_STOP;
    d->dispatch_status=pr_u25_slot_dispatch(s->bus,PR_BUS_RAM_SIZE,s->cpu);
    if(d->dispatch_status!=PR_290B_FIRST_DONE)return PR_SECOND_SLOT_STOP;
    if(s->cpu->ip==0x0164u)return PR_FOURTH_SLOT_ZERO;
    if(s->cpu->ip==0x017au)return PR_FOURTH_SLOT_WORD;
    if(s->cpu->ip==0x019eu)return PR_FOURTH_SLOT_PAIR;
    return PR_FOURTH_SLOT_OTHER_TYPE;
}
pr_second_dispatch_status pr_fourth_slot_start(pr_fourth_slot_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_fourth_slot_diagnostic*d){
    pr_second_dispatch_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_FOURTH_SLOT_EMPTY||s->branch.phase!=PR_FOURTH_BRANCH_EMPTY||s->branch.loader.phase!=PR_FOURTH_LOADER_EMPTY||s->branch.loader.save.phase!=PR_FOURTH_SAVE_EMPTY||s->branch.loader.save.dispatch.phase!=PR_FOURTH_DISPATCH_EMPTY||s->branch.loader.save.dispatch.runner.phase!=PR_THIRD_RUNNER_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.phase!=PR_THIRD_DISPATCH_RETURN_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.handler.phase!=PR_THIRD_HANDLER_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.handler.store.phase!=PR_THIRD_STORE_RETURN_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.handler.store.low.phase!=PR_THIRD_CLASS4_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.phase!=PR_THIRD_STORE_DISPATCH_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.phase!=PR_THIRD_CLASS_RETURN_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.phase!=PR_THIRD_CLASS_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.phase!=PR_THIRD_STORE_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.phase!=PR_THIRD_VALUE_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.phase!=PR_THIRD_PAIR_RETURN_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.phase!=PR_THIRD_PAIR_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.phase!=PR_THIRD_SLOT_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.phase!=PR_THIRD_BRANCH_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_THIRD_LOADER_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_THIRD_SAVE_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.phase!=PR_THIRD_DISPATCH_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.phase!=PR_SECOND_RUNNER_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.phase!=PR_SECOND_DISPATCH_RETURN_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.phase!=PR_SECOND_HANDLER_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.phase!=PR_SECOND_STORE_RETURN_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.phase!=PR_SECOND_CLASS4_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.phase!=PR_SECOND_DISPATCH_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.phase!=PR_SECOND_CLASS_RETURN_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.phase!=PR_SECOND_CLASS_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.phase!=PR_SECOND_STORE_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.phase!=PR_SECOND_VALUE_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.phase!=PR_SECOND_PAIR_RETURN_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.phase!=PR_SECOND_PAIR_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.phase!=PR_SECOND_SLOT_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.phase!=PR_SECOND_BRANCH_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_SECOND_LOADER_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_SECOND_SAVE_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.phase!=PR_NEXT_DISPATCH_EMPTY||s->branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.runner.phase!=PR_RUNNER_NEXT_EMPTY||x->bp<56u)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_fourth_branch_start(&s->branch,b,n,c,e,cl,pl,x,max_iterations,&d->branch);return fourth_slot_after(s,d,st);
}
pr_second_dispatch_status pr_fourth_slot_resume(pr_fourth_slot_session*s,pr_fourth_slot_diagnostic*d){pr_second_dispatch_status st;
    if(!s||!d||s->phase!=PR_FOURTH_SLOT_WAITING)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_fourth_branch_resume(&s->branch,&d->branch);
    if(st==PR_SECOND_SLOT_ARGUMENT)return st;
    return fourth_slot_after(s,d,st);
}

void pr_fourth_pair_session_init(pr_fourth_pair_session*s){if(s){memset(s,0,sizeof *s);pr_fourth_slot_session_init(&s->slot);}}
static pr_second_dispatch_status fourth_pair_after(pr_fourth_pair_session*s,pr_fourth_pair_diagnostic*d,pr_second_dispatch_status st){
    d->slot_status=st;if(st==PR_SECOND_SLOT_YIELD){s->phase=PR_FOURTH_PAIR_WAITING;return st;}
    s->phase=PR_FOURTH_PAIR_FINISHED;if(st!=PR_FOURTH_SLOT_PAIR)return st;
    d->entry_status=pr_u25_slot_pair_entry(s->bus,PR_BUS_RAM_SIZE,s->cpu,&d->entry);
    return d->entry_status==PR_290B_FIRST_DONE?PR_FOURTH_SLOT_PAIR:PR_SECOND_SLOT_STOP;
}
pr_second_dispatch_status pr_fourth_pair_start(pr_fourth_pair_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_fourth_pair_diagnostic*d){
    pr_second_dispatch_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_FOURTH_PAIR_EMPTY||s->slot.phase!=PR_FOURTH_SLOT_EMPTY||s->slot.branch.phase!=PR_FOURTH_BRANCH_EMPTY||s->slot.branch.loader.phase!=PR_FOURTH_LOADER_EMPTY||s->slot.branch.loader.save.phase!=PR_FOURTH_SAVE_EMPTY||s->slot.branch.loader.save.dispatch.phase!=PR_FOURTH_DISPATCH_EMPTY||s->slot.branch.loader.save.dispatch.runner.phase!=PR_THIRD_RUNNER_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.phase!=PR_THIRD_DISPATCH_RETURN_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.handler.phase!=PR_THIRD_HANDLER_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.handler.store.phase!=PR_THIRD_STORE_RETURN_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.phase!=PR_THIRD_CLASS4_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.phase!=PR_THIRD_STORE_DISPATCH_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.phase!=PR_THIRD_CLASS_RETURN_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.phase!=PR_THIRD_CLASS_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.phase!=PR_THIRD_STORE_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.phase!=PR_THIRD_VALUE_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.phase!=PR_THIRD_PAIR_RETURN_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.phase!=PR_THIRD_PAIR_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.phase!=PR_THIRD_SLOT_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.phase!=PR_THIRD_BRANCH_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_THIRD_LOADER_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_THIRD_SAVE_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.phase!=PR_THIRD_DISPATCH_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.phase!=PR_SECOND_RUNNER_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.phase!=PR_SECOND_DISPATCH_RETURN_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.phase!=PR_SECOND_HANDLER_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.phase!=PR_SECOND_STORE_RETURN_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.phase!=PR_SECOND_CLASS4_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.phase!=PR_SECOND_DISPATCH_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.phase!=PR_SECOND_CLASS_RETURN_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.phase!=PR_SECOND_CLASS_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.phase!=PR_SECOND_STORE_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.phase!=PR_SECOND_VALUE_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.phase!=PR_SECOND_PAIR_RETURN_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.phase!=PR_SECOND_PAIR_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.phase!=PR_SECOND_SLOT_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.phase!=PR_SECOND_BRANCH_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_SECOND_LOADER_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_SECOND_SAVE_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.phase!=PR_NEXT_DISPATCH_EMPTY||s->slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.runner.phase!=PR_RUNNER_NEXT_EMPTY||x->bp<56u)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_fourth_slot_start(&s->slot,b,n,c,e,cl,pl,x,max_iterations,&d->slot);return fourth_pair_after(s,d,st);
}
pr_second_dispatch_status pr_fourth_pair_resume(pr_fourth_pair_session*s,pr_fourth_pair_diagnostic*d){pr_second_dispatch_status st;
    if(!s||!d||s->phase!=PR_FOURTH_PAIR_WAITING)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_fourth_slot_resume(&s->slot,&d->slot);
    if(st==PR_SECOND_SLOT_ARGUMENT)return st;
    return fourth_pair_after(s,d,st);
}

void pr_fourth_pair_return_session_init(pr_fourth_pair_return_session*s){if(s){memset(s,0,sizeof *s);pr_fourth_pair_session_init(&s->pair);}}
static pr_second_dispatch_status fourth_pair_return_after(pr_fourth_pair_return_session*s,pr_fourth_pair_return_diagnostic*d,pr_second_dispatch_status st){
    pr_290b_return_target x;d->pair_status=st;if(st==PR_SECOND_SLOT_YIELD){s->phase=PR_FOURTH_PAIR_RETURN_WAITING;return st;}
    s->phase=PR_FOURTH_PAIR_RETURN_FINISHED;if(st!=PR_FOURTH_SLOT_PAIR)return st;
    x=(pr_290b_return_target){s->cpu->cs,0x01aau,(uint16_t)(s->cpu->bp-10u),s->cpu->bp,s->cpu->ss};
    d->return_status=pr_u25_word_return(s->bus,PR_BUS_RAM_SIZE,s->cpu,&x);
    return d->return_status==PR_290B_FIRST_DONE?PR_FOURTH_SLOT_PAIR:PR_SECOND_SLOT_STOP;
}
pr_second_dispatch_status pr_fourth_pair_return_start(pr_fourth_pair_return_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_fourth_pair_return_diagnostic*d){
    pr_second_dispatch_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_FOURTH_PAIR_RETURN_EMPTY||s->pair.phase!=PR_FOURTH_PAIR_EMPTY||s->pair.slot.phase!=PR_FOURTH_SLOT_EMPTY||s->pair.slot.branch.phase!=PR_FOURTH_BRANCH_EMPTY||s->pair.slot.branch.loader.phase!=PR_FOURTH_LOADER_EMPTY||s->pair.slot.branch.loader.save.phase!=PR_FOURTH_SAVE_EMPTY||s->pair.slot.branch.loader.save.dispatch.phase!=PR_FOURTH_DISPATCH_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.phase!=PR_THIRD_RUNNER_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.phase!=PR_THIRD_DISPATCH_RETURN_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.phase!=PR_THIRD_HANDLER_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.phase!=PR_THIRD_STORE_RETURN_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.phase!=PR_THIRD_CLASS4_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.phase!=PR_THIRD_STORE_DISPATCH_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.phase!=PR_THIRD_CLASS_RETURN_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.phase!=PR_THIRD_CLASS_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.phase!=PR_THIRD_STORE_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.phase!=PR_THIRD_VALUE_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.phase!=PR_THIRD_PAIR_RETURN_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.phase!=PR_THIRD_PAIR_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.phase!=PR_THIRD_SLOT_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.phase!=PR_THIRD_BRANCH_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_THIRD_LOADER_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_THIRD_SAVE_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.phase!=PR_THIRD_DISPATCH_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.phase!=PR_SECOND_RUNNER_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.phase!=PR_SECOND_DISPATCH_RETURN_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.phase!=PR_SECOND_HANDLER_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.phase!=PR_SECOND_STORE_RETURN_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.phase!=PR_SECOND_CLASS4_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.phase!=PR_SECOND_DISPATCH_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.phase!=PR_SECOND_CLASS_RETURN_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.phase!=PR_SECOND_CLASS_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.phase!=PR_SECOND_STORE_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.phase!=PR_SECOND_VALUE_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.phase!=PR_SECOND_PAIR_RETURN_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.phase!=PR_SECOND_PAIR_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.phase!=PR_SECOND_SLOT_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.phase!=PR_SECOND_BRANCH_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_SECOND_LOADER_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_SECOND_SAVE_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.phase!=PR_NEXT_DISPATCH_EMPTY||s->pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.runner.phase!=PR_RUNNER_NEXT_EMPTY||x->bp<56u)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_fourth_pair_start(&s->pair,b,n,c,e,cl,pl,x,max_iterations,&d->pair);return fourth_pair_return_after(s,d,st);
}
pr_second_dispatch_status pr_fourth_pair_return_resume(pr_fourth_pair_return_session*s,pr_fourth_pair_return_diagnostic*d){pr_second_dispatch_status st;
    if(!s||!d||s->phase!=PR_FOURTH_PAIR_RETURN_WAITING)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_fourth_pair_resume(&s->pair,&d->pair);
    if(st==PR_SECOND_SLOT_ARGUMENT)return st;
    return fourth_pair_return_after(s,d,st);
}

void pr_fourth_value_session_init(pr_fourth_value_session*s){if(s){memset(s,0,sizeof *s);pr_fourth_pair_return_session_init(&s->pair);}}
static pr_second_dispatch_status fourth_value_after(pr_fourth_value_session*s,pr_fourth_value_diagnostic*d,pr_second_dispatch_status st){
    d->pair_status=st;if(st==PR_SECOND_SLOT_YIELD){s->phase=PR_FOURTH_VALUE_WAITING;return st;}
    s->phase=PR_FOURTH_VALUE_FINISHED;if(st!=PR_FOURTH_SLOT_PAIR)return st;
    d->return_status=pr_u25_slot_value_return(s->bus,PR_BUS_RAM_SIZE,s->cpu,&s->target);
    return d->return_status==PR_290B_FIRST_DONE?PR_FOURTH_SLOT_PAIR:PR_SECOND_SLOT_STOP;
}
pr_second_dispatch_status pr_fourth_value_start(pr_fourth_value_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_fourth_value_diagnostic*d){
    pr_second_dispatch_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_FOURTH_VALUE_EMPTY||s->pair.phase!=PR_FOURTH_PAIR_RETURN_EMPTY||s->pair.pair.phase!=PR_FOURTH_PAIR_EMPTY||s->pair.pair.slot.phase!=PR_FOURTH_SLOT_EMPTY||s->pair.pair.slot.branch.phase!=PR_FOURTH_BRANCH_EMPTY||s->pair.pair.slot.branch.loader.phase!=PR_FOURTH_LOADER_EMPTY||s->pair.pair.slot.branch.loader.save.phase!=PR_FOURTH_SAVE_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.phase!=PR_FOURTH_DISPATCH_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.phase!=PR_THIRD_RUNNER_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.phase!=PR_THIRD_DISPATCH_RETURN_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.phase!=PR_THIRD_HANDLER_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.phase!=PR_THIRD_STORE_RETURN_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.phase!=PR_THIRD_CLASS4_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.phase!=PR_THIRD_STORE_DISPATCH_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.phase!=PR_THIRD_CLASS_RETURN_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.phase!=PR_THIRD_CLASS_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.phase!=PR_THIRD_STORE_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.phase!=PR_THIRD_VALUE_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.phase!=PR_THIRD_PAIR_RETURN_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.phase!=PR_THIRD_PAIR_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.phase!=PR_THIRD_SLOT_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.phase!=PR_THIRD_BRANCH_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_THIRD_LOADER_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_THIRD_SAVE_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.phase!=PR_THIRD_DISPATCH_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.phase!=PR_SECOND_RUNNER_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.phase!=PR_SECOND_DISPATCH_RETURN_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.phase!=PR_SECOND_HANDLER_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.phase!=PR_SECOND_STORE_RETURN_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.phase!=PR_SECOND_CLASS4_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.phase!=PR_SECOND_DISPATCH_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.phase!=PR_SECOND_CLASS_RETURN_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.phase!=PR_SECOND_CLASS_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.phase!=PR_SECOND_STORE_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.phase!=PR_SECOND_VALUE_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.phase!=PR_SECOND_PAIR_RETURN_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.phase!=PR_SECOND_PAIR_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.phase!=PR_SECOND_SLOT_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.phase!=PR_SECOND_BRANCH_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_SECOND_LOADER_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_SECOND_SAVE_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.phase!=PR_NEXT_DISPATCH_EMPTY||s->pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.runner.phase!=PR_RUNNER_NEXT_EMPTY||x->bp<56u)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;s->target=(pr_290b_return_target){x->cs,0x02bfu,(uint16_t)(x->bp-0x1au),(uint16_t)(x->bp-0x16u),x->ss};
    st=pr_fourth_pair_return_start(&s->pair,b,n,c,e,cl,pl,x,max_iterations,&d->pair);return fourth_value_after(s,d,st);
}
pr_second_dispatch_status pr_fourth_value_resume(pr_fourth_value_session*s,pr_fourth_value_diagnostic*d){pr_second_dispatch_status st;
    if(!s||!d||s->phase!=PR_FOURTH_VALUE_WAITING)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_fourth_pair_return_resume(&s->pair,&d->pair);
    if(st==PR_SECOND_SLOT_ARGUMENT)return st;
    return fourth_value_after(s,d,st);
}

void pr_fourth_store_session_init(pr_fourth_store_session*s){if(s){memset(s,0,sizeof *s);pr_fourth_value_session_init(&s->value);}}
static pr_second_dispatch_status fourth_store_after(pr_fourth_store_session*s,pr_fourth_store_diagnostic*d,pr_second_dispatch_status st){
    d->value_status=st;if(st==PR_SECOND_SLOT_YIELD){s->phase=PR_FOURTH_STORE_WAITING;return st;}
    s->phase=PR_FOURTH_STORE_FINISHED;if(st!=PR_FOURTH_SLOT_PAIR)return st;
    d->entry_status=pr_u1c_save_store_entry(s->bus,PR_BUS_RAM_SIZE,s->cpu,&d->entry);
    return d->entry_status==PR_290B_FIRST_DONE?PR_FOURTH_SLOT_PAIR:PR_SECOND_SLOT_STOP;
}
pr_second_dispatch_status pr_fourth_store_start(pr_fourth_store_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_fourth_store_diagnostic*d){
    pr_second_dispatch_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_FOURTH_STORE_EMPTY||s->value.phase!=PR_FOURTH_VALUE_EMPTY||s->value.pair.phase!=PR_FOURTH_PAIR_RETURN_EMPTY||s->value.pair.pair.phase!=PR_FOURTH_PAIR_EMPTY||s->value.pair.pair.slot.phase!=PR_FOURTH_SLOT_EMPTY||s->value.pair.pair.slot.branch.phase!=PR_FOURTH_BRANCH_EMPTY||s->value.pair.pair.slot.branch.loader.phase!=PR_FOURTH_LOADER_EMPTY||s->value.pair.pair.slot.branch.loader.save.phase!=PR_FOURTH_SAVE_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.phase!=PR_FOURTH_DISPATCH_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.phase!=PR_THIRD_RUNNER_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.phase!=PR_THIRD_DISPATCH_RETURN_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.phase!=PR_THIRD_HANDLER_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.phase!=PR_THIRD_STORE_RETURN_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.phase!=PR_THIRD_CLASS4_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.phase!=PR_THIRD_STORE_DISPATCH_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.phase!=PR_THIRD_CLASS_RETURN_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.phase!=PR_THIRD_CLASS_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.phase!=PR_THIRD_STORE_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.phase!=PR_THIRD_VALUE_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.phase!=PR_THIRD_PAIR_RETURN_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.phase!=PR_THIRD_PAIR_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.phase!=PR_THIRD_SLOT_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.phase!=PR_THIRD_BRANCH_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_THIRD_LOADER_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_THIRD_SAVE_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.phase!=PR_THIRD_DISPATCH_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.phase!=PR_SECOND_RUNNER_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.phase!=PR_SECOND_DISPATCH_RETURN_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.phase!=PR_SECOND_HANDLER_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.phase!=PR_SECOND_STORE_RETURN_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.phase!=PR_SECOND_CLASS4_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.phase!=PR_SECOND_DISPATCH_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.phase!=PR_SECOND_CLASS_RETURN_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.phase!=PR_SECOND_CLASS_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.phase!=PR_SECOND_STORE_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.phase!=PR_SECOND_VALUE_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.phase!=PR_SECOND_PAIR_RETURN_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.phase!=PR_SECOND_PAIR_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.phase!=PR_SECOND_SLOT_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.phase!=PR_SECOND_BRANCH_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_SECOND_LOADER_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_SECOND_SAVE_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.phase!=PR_NEXT_DISPATCH_EMPTY||s->value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.runner.phase!=PR_RUNNER_NEXT_EMPTY||x->bp<56u)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_fourth_value_start(&s->value,b,n,c,e,cl,pl,x,max_iterations,&d->value);return fourth_store_after(s,d,st);
}
pr_second_dispatch_status pr_fourth_store_resume(pr_fourth_store_session*s,pr_fourth_store_diagnostic*d){pr_second_dispatch_status st;
    if(!s||!d||s->phase!=PR_FOURTH_STORE_WAITING)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_fourth_value_resume(&s->value,&d->value);
    if(st==PR_SECOND_SLOT_ARGUMENT)return st;
    return fourth_store_after(s,d,st);
}

void pr_fourth_class_session_init(pr_fourth_class_session*s){if(s){memset(s,0,sizeof *s);pr_fourth_store_session_init(&s->store);}}
static pr_second_dispatch_status fourth_class_after(pr_fourth_class_session*s,pr_fourth_class_diagnostic*d,pr_second_dispatch_status st){
    d->store_status=st;if(st==PR_SECOND_SLOT_YIELD){s->phase=PR_FOURTH_CLASS_WAITING;return st;}
    s->phase=PR_FOURTH_CLASS_FINISHED;if(st!=PR_FOURTH_SLOT_PAIR)return st;
    d->entry_status=pr_u25_store_classifier_entry(s->bus,PR_BUS_RAM_SIZE,s->cpu,&d->entry);
    return d->entry_status==PR_290B_FIRST_DONE?PR_FOURTH_SLOT_PAIR:PR_SECOND_SLOT_STOP;
}
pr_second_dispatch_status pr_fourth_class_start(pr_fourth_class_session*s,pr_bus*b,size_t n,pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,const pr_290b_clear_limits*cl,const pr_290b_print_limits*pl,const pr_290b_return_target*x,size_t max_iterations,pr_fourth_class_diagnostic*d){
    pr_second_dispatch_status st;if(!s||!b||!c||!e||!cl||!pl||!x||!d||s->phase!=PR_FOURTH_CLASS_EMPTY||s->store.phase!=PR_FOURTH_STORE_EMPTY||s->store.value.phase!=PR_FOURTH_VALUE_EMPTY||s->store.value.pair.phase!=PR_FOURTH_PAIR_RETURN_EMPTY||s->store.value.pair.pair.phase!=PR_FOURTH_PAIR_EMPTY||s->store.value.pair.pair.slot.phase!=PR_FOURTH_SLOT_EMPTY||s->store.value.pair.pair.slot.branch.phase!=PR_FOURTH_BRANCH_EMPTY||s->store.value.pair.pair.slot.branch.loader.phase!=PR_FOURTH_LOADER_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.phase!=PR_FOURTH_SAVE_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.phase!=PR_FOURTH_DISPATCH_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.phase!=PR_THIRD_RUNNER_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.phase!=PR_THIRD_DISPATCH_RETURN_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.phase!=PR_THIRD_HANDLER_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.phase!=PR_THIRD_STORE_RETURN_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.phase!=PR_THIRD_CLASS4_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.phase!=PR_THIRD_STORE_DISPATCH_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.phase!=PR_THIRD_CLASS_RETURN_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.phase!=PR_THIRD_CLASS_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.phase!=PR_THIRD_STORE_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.phase!=PR_THIRD_VALUE_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.phase!=PR_THIRD_PAIR_RETURN_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.phase!=PR_THIRD_PAIR_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.phase!=PR_THIRD_SLOT_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.phase!=PR_THIRD_BRANCH_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_THIRD_LOADER_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_THIRD_SAVE_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.phase!=PR_THIRD_DISPATCH_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.phase!=PR_SECOND_RUNNER_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.phase!=PR_SECOND_DISPATCH_RETURN_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.phase!=PR_SECOND_HANDLER_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.phase!=PR_SECOND_STORE_RETURN_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.phase!=PR_SECOND_CLASS4_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.phase!=PR_SECOND_DISPATCH_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.phase!=PR_SECOND_CLASS_RETURN_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.phase!=PR_SECOND_CLASS_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.phase!=PR_SECOND_STORE_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.phase!=PR_SECOND_VALUE_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.phase!=PR_SECOND_PAIR_RETURN_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.phase!=PR_SECOND_PAIR_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.phase!=PR_SECOND_SLOT_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.phase!=PR_SECOND_BRANCH_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.phase!=PR_SECOND_LOADER_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.phase!=PR_SECOND_SAVE_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.phase!=PR_NEXT_DISPATCH_EMPTY||s->store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.dispatch.runner.dispatch.handler.store.low.dispatch.classifier.classifier.store.value.pair.pair.slot.branch.loader.save.next.runner.phase!=PR_RUNNER_NEXT_EMPTY||x->bp<56u)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);s->bus=b;s->cpu=c;st=pr_fourth_store_start(&s->store,b,n,c,e,cl,pl,x,max_iterations,&d->store);return fourth_class_after(s,d,st);
}
pr_second_dispatch_status pr_fourth_class_resume(pr_fourth_class_session*s,pr_fourth_class_diagnostic*d){pr_second_dispatch_status st;
    if(!s||!d||s->phase!=PR_FOURTH_CLASS_WAITING)return PR_SECOND_SLOT_ARGUMENT;
    memset(d,0,sizeof *d);st=pr_fourth_store_resume(&s->store,&d->store);
    if(st==PR_SECOND_SLOT_ARGUMENT)return st;
    return fourth_class_after(s,d,st);
}
