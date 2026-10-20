#include "poolrad/recovered/overlay_u6c_record_unlink_3291_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
#define CHECK(x) do { ++checks; if(!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1); } } while(0)
#define CS 0x5800u
#define DS 0x1800u
#define SS 0x2800u
#define SP 0x8000u
#define STUB 0x6000u
#define U3CE 0x4000u
#define RES 0x5000u
#define SYS 0xAF8u
static unsigned at(unsigned s,unsigned o) { return ((s<<4)+(o&65535u))&0xFFFFFu; }
static void wr(uint8_t *r,unsigned s,unsigned o,unsigned v) { unsigned p=at(s,o);r[p]=(uint8_t)v;r[(p+1)&0xFFFFF]=(uint8_t)(v>>8); }
static unsigned rd(const uint8_t *r,unsigned s,unsigned o) { unsigned p=at(s,o);return r[p]|((unsigned)r[(p+1)&0xFFFFF]<<8); }
static void ptr(uint8_t *r,unsigned s,unsigned o,unsigned seg,unsigned off) { wr(r,s,o,off);wr(r,s,o+2,seg); }
static void call9a(uint8_t *r,unsigned s,unsigned site,unsigned off,unsigned seg) { r[at(s,site)]=0x9A;wr(r,s,site+1,off);wr(r,s,site+3,seg); }
/* Records A,B,C at 3100/3120/3140 (next at +104h, slot byte at +0BFh), the
 * heap empty up to HeapPtr 3800, window record at 3A00 with +67Ch = 1234h. */
static pr_u6c_3291_cpu setup(uint8_t *r,unsigned b6,unsigned b8,unsigned head,unsigned active)
{
    pr_u6c_3291_cpu c={0x1234,0x2345,0x3456,0x4567,0x5678,0x6789,0x789A,SP,CS,0x3291,DS,0x8888,SS,0xED7};
    static const unsigned sites0[]={0x2F,0x76,0xBE,0xCF},sites95[]={0x25F0,0x2605,0x261B};
    memset(r,0xA5,PR_U6C_3291_RAM_SIZE);
    ptr(r,DS,0x4400,0x3000,0);ptr(r,DS,0x4404,0x3800,0);ptr(r,DS,0x4408,0x4000,0);ptr(r,DS,0x440C,0x3800,0);
    for(unsigned i=0;i<3;++i) {
        unsigned seg=0x3100+0x20*i;
        ptr(r,seg,0x104,i<2?seg+0x20:0,0);r[at(seg,0xBF)]=(uint8_t)(7+i);
        ptr(r,seg,0x108,0,0);ptr(r,seg,0xC8,0,0);ptr(r,seg,0x7F,0,0);
    }
    for(unsigned i=0;i<256;++i) { ptr(r,DS,(0x5E5Au)+8u*i,0,0);ptr(r,DS,(0x5E5Eu)+8u*i,0,0); }
    ptr(r,DS,0x5D96,head,0);ptr(r,DS,0x5D92,active,0);ptr(r,DS,0x49D6,0x3A00,0);wr(r,0x3A00,0x67C,0x1234);
    wr(r,SS,SP,0x7000);wr(r,SS,SP+2,0x9001);wr(r,SS,SP+4,0xAB00|b6);wr(r,SS,SP+6,0xCD00|b8);
    for(unsigned i=0;i<4;++i)call9a(r,CS,sites0[i],0x254,SYS);
    for(unsigned i=0;i<3;++i)call9a(r,RES,sites95[i],0x254,SYS);
    call9a(r,CS,0x3321,0x2F,STUB);call9a(r,CS,0x337B,0x2F,STUB);
    for(unsigned i=0;i<2;++i) { unsigned site=i?0x33B2:0x334A;r[at(CS,site-1)]=0x0E;r[at(CS,site)]=0xE8;wr(r,CS,site+1,(0u-(site+3))&0xFFFF); }
    call9a(r,U3CE,0x17A,0x2595,RES);call9a(r,U3CE,0x1A5,0x2595,RES);
    r[at(STUB,0x2F)]=0xEA;wr(r,STUB,0x30,0x14E);wr(r,STUB,0x32,U3CE);
    return c;
}
static void block(const uint8_t *r,unsigned s,unsigned next,unsigned paragraphs,unsigned granule)
{
    CHECK(rd(r,s,0)==0);CHECK(rd(r,s,2)==next);CHECK(rd(r,s,4)==granule);CHECK(rd(r,s,6)==paragraphs);
}
static void returned(const pr_u6c_3291_cpu *c)
{
    CHECK(c->cs==0x9001 && c->ip==0x7000 && c->sp==SP+8 && c->bp==0x789A);
}
int main(void)
{
    uint8_t *r=malloc(PR_U6C_3291_RAM_SIZE),*saved=malloc(PR_U6C_3291_RAM_SIZE);
    pr_u6c_3291_cpu c,before;pr_u6c_3291_limits l={64,64,64};pr_u6c_3291_diagnostic d;
    CHECK(r && saved);
#define RUN(cpu) pr_u6c_record_unlink_3291_cpu(r,PR_U6C_3291_RAM_SIZE,&(cpu),RES,SYS,&l,&d)

    /* Nil head: return at once with the flags of the zero OR; nothing else touched. */
    c=setup(r,1,1,0,0x3120);memcpy(saved,r,PR_U6C_3291_RAM_SIZE);
    CHECK(RUN(c)==PR_U6C_3291_OK);returned(&c);
    CHECK((c.flags&0x8D5)==0x44 && c.ax==0);
    CHECK(d.callee==PR_U6C_3291_NO_CALLEE && d.callee_status==-1 && d.pair_calls==0 && d.record_calls==0 && d.call_site==0);
    CHECK(rd(r,0x3A00,0x67C)==0x1234 && rd(r,DS,0x5D92)==0 && rd(r,DS,0x5D94)==0x3120);

    /* Head is the active record, single node: head and active become nil; the
     * record (11Dh -> 12h paragraphs) is freed; B8 nonzero skips the DEC. */
    c=setup(r,0,1,0x3100,0x3100);ptr(r,0x3100,0x104,0,0);
    CHECK(RUN(c)==PR_U6C_3291_OK);returned(&c);
    CHECK(rd(r,DS,0x5D96)==0 && rd(r,DS,0x5D98)==0 && rd(r,DS,0x5D92)==0 && rd(r,DS,0x5D94)==0);
    CHECK(rd(r,DS,0x440C)==0 && rd(r,DS,0x440E)==0x3100);block(r,0x3100,0x3800,0x12,0);
    CHECK(rd(r,0x3A00,0x67C)==0x1234 && (c.flags&0x8D5)==0); /* last flags: CMP B8=1,0 */
    CHECK(d.callee==PR_U6C_3291_RECORD_RELEASE && d.callee_status==0 && d.record_calls==1 && d.pair_calls==0 && d.call_site==0x33B2);

    /* Head is the active record with a successor: both move to it; B8 zero decrements. */
    c=setup(r,0,0,0x3100,0x3100);
    CHECK(RUN(c)==PR_U6C_3291_OK);returned(&c);
    CHECK(rd(r,DS,0x5D96)==0 && rd(r,DS,0x5D98)==0x3120 && rd(r,DS,0x5D92)==0 && rd(r,DS,0x5D94)==0x3120);
    block(r,0x3100,0x3800,0x12,0);CHECK(rd(r,0x3A00,0x67C)==0x1233);

    /* Walk: A -> B -> C with active B. A.next := C, B freed, active := A. */
    c=setup(r,0,0,0x3100,0x3120);
    CHECK(RUN(c)==PR_U6C_3291_OK);returned(&c);
    CHECK(rd(r,0x3100,0x104)==0 && rd(r,0x3100,0x106)==0x3140);
    CHECK(rd(r,DS,0x5D96)==0 && rd(r,DS,0x5D98)==0x3100 && rd(r,DS,0x5D92)==0 && rd(r,DS,0x5D94)==0x3100);
    block(r,0x3120,0x3800,0x12,0);CHECK(rd(r,0x3A00,0x67C)==0x1233);
    CHECK(d.call_site==0x334A && (c.flags&0x8D5)==0x04); /* last flags: DEC 1234h->1233h leaves PF only */

    /* Active not in the list: the walk reaches the nil link and returns, freeing nothing. */
    c=setup(r,1,0,0x3100,0x3180);memcpy(saved,r,PR_U6C_3291_RAM_SIZE);
    CHECK(RUN(c)==PR_U6C_3291_OK);returned(&c);
    CHECK(rd(r,DS,0x440E)==0x3800 && rd(r,0x3A00,0x67C)==0x1234 && d.callee==PR_U6C_3291_NO_CALLEE);
    CHECK(rd(r,0x3100,0x104)==0 && rd(r,0x3100,0x106)==0x3120);

    /* B6 nonzero calls through the loaded stub with the active record's slot
     * byte (B in the walk case is 8): both table entries nil, so +014E returns. */
    c=setup(r,1,0,0x3100,0x3120);
    CHECK(RUN(c)==PR_U6C_3291_OK);returned(&c);
    CHECK(d.pair_calls==1 && d.record_calls==1 && d.pair.calls==0 && d.pair.resident_status==-1 && d.callee==PR_U6C_3291_RECORD_RELEASE);
    CHECK(rd(r,0x3A00,0x67C)==0x1233);
    c=setup(r,1,1,0x3100,0x3100);
    CHECK(RUN(c)==PR_U6C_3291_OK);returned(&c);CHECK(d.pair_calls==1 && d.record_calls==1);

    /* A cyclic list is a LIMIT at the loop head after the earlier effects. */
    c=setup(r,0,0,0x3100,0x3180);ptr(r,0x3120,0x104,0x3100,0);l.max_walk_loopbacks=2;
    CHECK(RUN(c)==PR_U6C_3291_LIMIT);CHECK(c.ip==0x3291+0x34 && c.cs==CS && c.sp==SP-6);
    l.max_walk_loopbacks=64;

    /* Stub that is not a loaded EA jump: CODE at the stub after the call's pushes. */
    c=setup(r,1,0,0x3100,0x3120);r[at(STUB,0x2F)]=0xCD;
    CHECK(RUN(c)==PR_U6C_3291_CODE);CHECK(c.cs==STUB && c.ip==0x2F && c.sp==SP-6-2-4);
    CHECK(rd(r,SS,c.sp)==0x3326 && rd(r,SS,c.sp+2)==CS && d.call_site==0x3321 && d.pair_calls==0);
    /* Malformed call opcode/operand: stopped at the site, nothing pushed. */
    c=setup(r,1,0,0x3100,0x3120);r[at(CS,0x3321)]=0x90;
    CHECK(RUN(c)==PR_U6C_3291_CODE);CHECK(c.ip==0x3321 && c.sp==SP-6-2);
    c=setup(r,0,0,0x3100,0x3120);r[at(CS,0x334A)]=0xE9;
    CHECK(RUN(c)==PR_U6C_3291_CODE);CHECK(c.ip==0x3349 && d.call_site==0x334A && d.record_calls==0);

    /* An unfreeable record (odd offset) stops +0000 with its RunError; the caller reports CALLEE. */
    c=setup(r,0,0,0x3100,0x3120);ptr(r,DS,0x5D92,0x3120,1);ptr(r,0x3100,0x104,0x3120,1);
    CHECK(RUN(c)==PR_U6C_3291_CALLEE);CHECK(d.callee==PR_U6C_3291_RECORD_RELEASE && d.callee_status==(int)PR_U6C_0000_CALLEE);
    CHECK(c.cs==SYS && rd(r,0x3A00,0x67C)==0x1234);

    for(unsigned trial=0;trial<6;++trial) {
        c=setup(r,1,1,0x3100,0x3120);if(trial==0)c.ip=1;if(trial==1)c.sp=0x8B;if(trial==2)c.sp=0xFFF9;
        before=c;memcpy(saved,r,PR_U6C_3291_RAM_SIZE);
        CHECK(pr_u6c_record_unlink_3291_cpu(trial==3?NULL:r,trial==4?1:PR_U6C_3291_RAM_SIZE,&c,RES,SYS,trial==5?NULL:&l,&d)==PR_U6C_3291_ARGUMENT);
        CHECK(memcmp(&before,&c,sizeof c)==0);CHECK(memcmp(saved,r,PR_U6C_3291_RAM_SIZE)==0);
    }
    c=setup(r,0,0,0x3100,0x3120);
    CHECK(pr_u6c_record_unlink_3291_cpu(r,PR_U6C_3291_RAM_SIZE,&c,RES,SYS,&l,NULL)==PR_U6C_3291_OK);
    free(saved);free(r);printf("%u semantic checks\n",checks);return 0;
}
