#include "poolrad/replay/u3b0_prompt_pass.h"
#include <string.h>
#define MASK (PR_BUS_RAM_SIZE-1u)
static size_t a(uint16_t s,uint16_t o){return (((size_t)s<<4u)+o)&MASK;}
static uint16_t w(const uint8_t*m,uint16_t s,uint16_t o){size_t p=a(s,o);return (uint16_t)(m[p]|((uint16_t)m[(p+1u)&MASK]<<8u));}
static int ordinary(uint16_t s){return s<=0x9000u||s>=0xb000u;}
static int aperture(size_t p){return p>=0xa0000u && p<0xb0000u;}
/* The return model writes the window before rereading the flag pointer. If
 * those locations alias, decline rather than guess the post-write pointer. */
static int return_safe(const uint8_t*m,const pr_r0899_cpu*c){
    uint16_t off,seg,fo,fs;size_t p,q,ptr;unsigned i;
    if(c->bp<0x63u||c->bp>0xffe3u||!ordinary(c->ss)||!ordinary(c->ds))return PR_U3B0_PASS_PROFILE_FRAME;
    if(m[a(c->ss,(uint16_t)(c->bp-0x63u))]==0u)return 0;
    off=w(m,c->ds,0x49d2u);seg=w(m,c->ds,0x49d4u);p=a(seg,(uint16_t)(off+0x03feu));q=(p+1u)&MASK;
    if(aperture(p)||aperture(q))return PR_U3B0_PASS_PROFILE_RETURN_APERTURE;
    ptr=a(c->ss,(uint16_t)(c->bp+6u));
    for(i=0;i<4u;++i)if(((ptr+i)&MASK)==p||((ptr+i)&MASK)==q)return PR_U3B0_PASS_PROFILE_FLAG_POINTER_ALIAS;
    fo=w(m,c->ss,(uint16_t)(c->bp+6u));fs=w(m,c->ss,(uint16_t)(c->bp+8u));
    return aperture(a(fs,fo))?PR_U3B0_PASS_PROFILE_RETURN_APERTURE:0;
}
pr_u3b0_pass_status pr_u3b0_prompt_pass(pr_bus*b,size_t n,pr_r0899_cpu*c,
    const pr_u3b0_pass_env*e,pr_u3b0_pass_diagnostic*d){
    pr_u3b0_poll_status ps;pr_u3b0_character_status cs;pr_u3b0_selection_status ss;
    pr_u3b0_mapped_status ms;pr_u3b0_text_status ts;pr_u3b0_return_status rs;int profile;
    if(!b||!b->ram||!b->ega||!c||!e||!d||!e->poll.key_env||!e->poll.key_env->keyboard||
       n!=PR_BUS_RAM_SIZE||c->ip!=0x0566u||c->bp<0x90u||c->bp>0xffe3u||
       c->sp<0x0116u||c->sp>0xfffcu)return PR_U3B0_PASS_ARGUMENT;
    memset(d,0,sizeof *d);
    if(!ordinary(c->cs)||!ordinary(c->ds)||!ordinary(c->ss)||!ordinary(e->poll.crt_segment)||
       !ordinary(e->poll.key_segment)||!ordinary(e->mapped.system_segment)) {
        d->status=PR_U3B0_PASS_PROFILE_SEGMENT;return PR_U3B0_PASS_PROFILE;
    }
    if(!ordinary(w(b->ram,c->ss,(uint16_t)(c->bp+8u)))) {
        d->status=PR_U3B0_PASS_PROFILE_FLAG_SEGMENT;return PR_U3B0_PASS_PROFILE;
    }
    d->stage=PR_U3B0_PASS_STAGE_POLL;ps=pr_u3b0_poll_0566(b->ram,n,c,&e->poll,&d->poll);d->status=(int)ps;
    if(ps==PR_U3B0_POLL_NON_EXTENDED){
        d->stage=PR_U3B0_PASS_STAGE_CHARACTER;cs=pr_u3b0_character_059c(b->ram,n,c);d->status=(int)cs;
        if(cs==PR_U3B0_CHARACTER_OTHER){
            d->stage=PR_U3B0_PASS_STAGE_SELECTION;ss=pr_u3b0_selection_05c4(b,n,c,&e->selection,&d->selection);d->status=(int)ss;
            if(ss==PR_U3B0_SELECTION_OTHER){
                d->stage=PR_U3B0_PASS_STAGE_MAPPED;ms=pr_u3b0_mapped_060c(b,n,c,&e->mapped,&d->mapped);d->status=(int)ms;
                if(ms==PR_U3B0_MAPPED_TEXT_SEARCH){
                    d->stage=PR_U3B0_PASS_STAGE_TEXT;ts=pr_u3b0_text_0634(b,n,c,&e->text,&d->text);d->status=(int)ts;
                    if(ts!=PR_U3B0_TEXT_BOUNDARY)return PR_U3B0_PASS_STOP;
                    d->stage=PR_U3B0_PASS_STAGE_MAPPED;ms=pr_u3b0_mapped_060c(b,n,c,&e->mapped,&d->mapped);d->status=(int)ms;
                }
                if(ms!=PR_U3B0_MAPPED_BOUNDARY)return PR_U3B0_PASS_STOP;
            }else if(ss!=PR_U3B0_SELECTION_DRAWN)return PR_U3B0_PASS_STOP;
        }else if(cs!=PR_U3B0_CHARACTER_ESCAPE&&cs!=PR_U3B0_CHARACTER_ENTER_ENABLED&&cs!=PR_U3B0_CHARACTER_ENTER_DISABLED)return PR_U3B0_PASS_STOP;
    }else if(ps!=PR_U3B0_POLL_NO_EVENT&&ps!=PR_U3B0_POLL_EXTENDED&&ps!=PR_U3B0_POLL_EXTENDED_DISABLED)return PR_U3B0_PASS_STOP;
    d->stage=PR_U3B0_PASS_STAGE_RETURN_GUARD;
    profile=return_safe(b->ram,c);if(profile){d->status=profile;return PR_U3B0_PASS_PROFILE;}
    d->stage=PR_U3B0_PASS_STAGE_RETURN;rs=pr_u3b0_return_06cf(b->ram,n,c,&e->target);d->status=(int)rs;
    if(rs==PR_U3B0_RETURN_DONE)return PR_U3B0_PASS_RETURN;
    if(rs==PR_U3B0_RETURN_LOOP)return PR_U3B0_PASS_LOOP;
    return PR_U3B0_PASS_STOP;
}
