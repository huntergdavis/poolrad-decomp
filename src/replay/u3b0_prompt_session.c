#include "poolrad/replay/u3b0_prompt_session.h"
#include <string.h>
void pr_u3b0_prompt_session_init(pr_u3b0_prompt_session*s){if(s)memset(s,0,sizeof *s);}
static int same_bus(const pr_bus*a,const pr_bus*b){return a->ram==b->ram&&a->ega==b->ega&&a->ports==b->ports&&a->on_vram_write==b->on_vram_write&&a->ctx==b->ctx;}
static pr_u3b0_session_status result(pr_u3b0_prompt_session*s,pr_u3b0_wait_status w,const pr_u3b0_wait_diagnostic*d){
    if(w==PR_U3B0_WAIT_PASS && d->status==PR_U3B0_PASS_RETURN){s->phase=PR_U3B0_SESSION_FINISHED;return PR_U3B0_SESSION_RETURN;}
    /* A no-key poll leaves site 0566; any key read advances it to 0572/0580. */
    if(w==PR_U3B0_WAIT_PASS && d->status==PR_U3B0_PASS_LOOP && d->pass.stage==PR_U3B0_PASS_STAGE_RETURN &&
       d->pass.poll.site==0x0566u && d->pass.poll.crt_status==PR_CRT_RETURNED && s->cpu->ip==0x048eu){
        s->snapshot=*s->cpu;s->phase=PR_U3B0_SESSION_WAITING;return PR_U3B0_SESSION_YIELD;
    }
    s->phase=PR_U3B0_SESSION_FINISHED;return PR_U3B0_SESSION_STOP;
}
pr_u3b0_session_status pr_u3b0_prompt_session_start(pr_u3b0_prompt_session*s,pr_bus*b,size_t n,
    pr_r0899_cpu*c,const pr_u3b0_clock_draw_env*e,pr_u3b0_session_diagnostic*d){
    if(!s||!b||!c||!e||!d||s->phase!=PR_U3B0_SESSION_EMPTY)return PR_U3B0_SESSION_ARGUMENT;
    memset(d,0,sizeof *d);d->from_entry=1;s->bus=b;s->binding=*b;s->cpu=c;s->env=e;
    d->entry_status=pr_u3b0_entry_03bd(b,n,c,e,&d->entry);
    if(d->entry_status!=PR_U3B0_ENTRY_REACHED||d->entry.phase!=3||
       d->entry.clock_draw_status!=PR_U3B0_CLOCK_DRAW_REACHED||d->entry.clock_draw.phase!=3||
       d->entry.clock_draw.draw_status!=PR_U3B0_DRAW_WAIT_REACHED){s->phase=PR_U3B0_SESSION_FINISHED;return PR_U3B0_SESSION_STOP;}
    return result(s,d->entry.clock_draw.draw.wait_status,&d->entry.clock_draw.draw.wait);
}
pr_u3b0_session_status pr_u3b0_prompt_session_resume(pr_u3b0_prompt_session*s,pr_u3b0_session_diagnostic*d){
    if(!s||!d||s->phase!=PR_U3B0_SESSION_WAITING||!s->bus||!s->cpu||!s->env||
       !same_bus(s->bus,&s->binding)||memcmp(s->cpu,&s->snapshot,sizeof s->snapshot)!=0)return PR_U3B0_SESSION_ARGUMENT;
    memset(d,0,sizeof *d);d->wait_status=pr_u3b0_wait_048e(s->bus,PR_BUS_RAM_SIZE,s->cpu,&s->env->draw.pass,&d->wait);
    return result(s,d->wait_status,&d->wait);
}
