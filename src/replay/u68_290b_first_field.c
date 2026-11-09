#include "poolrad/replay/u1c_up_dispatch.h"
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
