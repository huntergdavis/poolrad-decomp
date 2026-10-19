#include "poolrad/recovered/overlay_u6c_record_release_0000_cpu.h"
#include "poolrad/recovered/system_freemem_0254_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
#define CHECK(x) do { ++checks; if(!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1); } } while(0)
static unsigned at(unsigned s,unsigned o) { return ((s<<4)+(o&65535u))&0xFFFFFu; }
static void wr(uint8_t *r,unsigned s,unsigned o,unsigned v) { unsigned p=at(s,o);r[p]=(uint8_t)v;r[(p+1)&0xFFFFF]=(uint8_t)(v>>8); }
static unsigned rd(const uint8_t *r,unsigned s,unsigned o) { unsigned p=at(s,o);return r[p]|((unsigned)r[(p+1)&0xFFFFF]<<8); }
static void ptr(uint8_t *r,unsigned s,unsigned o,unsigned seg,unsigned off) { wr(r,s,o,off);wr(r,s,o+2,seg); }
static pr_u6c_0000_cpu setup(uint8_t *r,unsigned ss,unsigned sp)
{
    static const unsigned sites[]={0x2F,0x76,0xBE,0xCF};
    pr_u6c_0000_cpu c={0x1234,0x2345,0x3456,0x4567,0x5678,0x6789,0x789A,0,0x5000,0,0x1800,0x8888,0,0xED7};
    memset(r,0xA5,PR_U6C_0000_RAM_SIZE);c.ss=(uint16_t)ss;c.sp=(uint16_t)sp;
    ptr(r,c.ds,0x4400,0x3000,0);ptr(r,c.ds,0x4404,0x3800,0);ptr(r,c.ds,0x4408,0x4000,0);ptr(r,c.ds,0x440C,0x3800,0);
    ptr(r,0x3100,0x108,0,0);ptr(r,0x3100,0xC8,0,0);ptr(r,0x3100,0x7F,0,0);
    wr(r,ss,sp,0x7000);wr(r,ss,sp+2,0x9001);ptr(r,ss,sp+4,0x3100,0);
    for(unsigned i=0;i<4;++i) { r[at(c.cs,sites[i])]=0x9A;wr(r,c.cs,sites[i]+1,0x254);wr(r,c.cs,sites[i]+3,0xAF8); }
    return c;
}
static void block(const uint8_t *r,unsigned s,unsigned next,unsigned paragraphs,unsigned granule)
{
    CHECK(rd(r,s,0)==0);CHECK(rd(r,s,2)==next);CHECK(rd(r,s,4)==granule);CHECK(rd(r,s,6)==paragraphs);
}
int main(void)
{
    uint8_t *r=malloc(PR_U6C_0000_RAM_SIZE),*saved=malloc(PR_U6C_0000_RAM_SIZE);
    pr_u6c_0000_cpu c,before;pr_u6c_0000_limits l={64,64};pr_u6c_0000_diagnostic d;
    CHECK(r && saved);
    c=setup(r,0x2800,0x8000);
    CHECK(pr_u6c_record_release_0000_cpu(r,PR_U6C_0000_RAM_SIZE,&c,&l,&d)==PR_U6C_0000_OK);
    CHECK(c.cs==0x9001 && c.ip==0x7000 && c.sp==0x8008 && c.bp==0x789A && c.ax==0);
    CHECK((c.flags&0x8D5)==0x44);CHECK(d.call_site==0xCF && d.freemem_status==0);
    CHECK(rd(r,0x2800,0x8004)==0 && rd(r,0x2800,0x8006)==0);
    CHECK(rd(r,0x1800,0x440C)==0 && rd(r,0x1800,0x440E)==0x3100);
    block(r,0x3100,0x3800,0x12,0);

    /* All combinations of optional sidecar and both lists. Check the resulting
       ascending free chain, rounded sizes and untouched source link fields. */
    for(unsigned side=0;side<2;++side)for(unsigned a=0;a<7;++a)for(unsigned b=0;b<7;++b) {
        unsigned nodes[16],sizes[16],n=0;
        c=setup(r,0x2800,0x8000);nodes[n]=0x3100;sizes[n++]=0x120;
        if(side) { ptr(r,0x3100,0x108,0x3200,0);nodes[n]=0x3200;sizes[n++]=0x18; }
        if(a)ptr(r,0x3100,0xC8,0x3300,0);
        for(unsigned i=0;i<a;++i) { unsigned seg=0x3300+i*8;ptr(r,seg,0x2A,i+1<a?seg+8:0,0);nodes[n]=seg;sizes[n++]=0x40; }
        if(b)ptr(r,0x3100,0x7F,0x3500,0);
        for(unsigned i=0;i<b;++i) { unsigned seg=0x3500+i*4;ptr(r,seg,5,i+1<b?seg+4:0,0);nodes[n]=seg;sizes[n++]=0x10; }
        CHECK(pr_u6c_record_release_0000_cpu(r,PR_U6C_0000_RAM_SIZE,&c,&l,&d)==PR_U6C_0000_OK);
        for(unsigned i=0;i<n;++i)block(r,nodes[i],i+1<n?nodes[i+1]:0x3800,sizes[i]>>4,sizes[i]&15);
        CHECK(rd(r,0x3100,0x10A)==(side?0x3200:0));
        CHECK(rd(r,0x3100,0xCA)==(a?0x3300:0));
        CHECK(rd(r,0x3100,0x81)==(b?0x3500:0));
        CHECK(rd(r,0x2800,0x8004)==0 && rd(r,0x2800,0x8006)==0);
    }
    /* A cap is a stop after the real free, with the next pointer already read.
       FreeMem overwrites next-at-5 in a 9-byte node; caching that link first matters. */
    c=setup(r,0x2800,0x8000);ptr(r,0x3100,0x7F,0x3500,0);ptr(r,0x3500,5,0x3504,0);ptr(r,0x3504,5,0,0);l.max_list_loopbacks=0;
    CHECK(pr_u6c_record_release_0000_cpu(r,PR_U6C_0000_RAM_SIZE,&c,&l,&d)==PR_U6C_0000_LIMIT);
    CHECK(c.cs==0x5000 && c.ip==0x8F && d.call_site==0xBE && d.freemem_status==0);
    CHECK(rd(r,c.ss,c.bp-0xE)==0x3504 && rd(r,c.ss,c.bp-0x10)==0);
    block(r,0x3500,0x3800,1,0);CHECK(rd(r,0x3504,0)==0xA5A5);
    CHECK(rd(r,0x2800,0x8006)==0x3100);l.max_list_loopbacks=64;

    /* Freeing the sidecar rewrites the original stack argument. The final
       free must reread it, rather than use the preserved record pointer. */
    c=setup(r,0x3000,0x2000);ptr(r,0x3100,0x108,0x3200,0);
    CHECK(pr_u6c_record_release_0000_cpu(r,PR_U6C_0000_RAM_SIZE,&c,&l,&d)==PR_U6C_0000_CALLEE);
    CHECK(d.call_site==0xCF && d.freemem_status==PR_SYSTEM_FREEMEM_RUNERROR);
    CHECK(c.cs==0xAF8 && c.ip==0xE2 && c.ax==204);
    CHECK(rd(r,0x3000,0x2004)==8 && rd(r,0x3000,0x2006)==1);
    CHECK(rd(r,0x3100,4)==0xA5A5);

    for(unsigned sp=0x1020;sp<=0x1028;sp+=8) {
        c=setup(r,0x3000,sp);
        CHECK(pr_u6c_record_release_0000_cpu(r,PR_U6C_0000_RAM_SIZE,&c,&l,&d)==(sp==0x1020?PR_U6C_0000_TRANSFER:PR_U6C_0000_CALLEE));
        CHECK(d.freemem_status==(sp==0x1020?0:PR_SYSTEM_FREEMEM_TRANSFER));
    }
    c=setup(r,0x2800,0x8000);r[at(c.cs,0xCF)]=0x90;
    CHECK(pr_u6c_record_release_0000_cpu(r,PR_U6C_0000_RAM_SIZE,&c,&l,&d)==PR_U6C_0000_CODE);
    CHECK(c.ip==0xCF && c.sp==0x7FE4 && d.freemem_status==-1);
    CHECK(rd(r,0x1800,0x440E)==0x3800);

    for(unsigned trial=0;trial<6;++trial) {
        c=setup(r,0x2800,0x8000);if(trial==0)c.ip=1;if(trial==1)c.sp=0x3F;if(trial==2)c.sp=0xFFF9;
        before=c;memcpy(saved,r,PR_U6C_0000_RAM_SIZE);
        CHECK(pr_u6c_record_release_0000_cpu(trial==3?NULL:r,trial==4?1:PR_U6C_0000_RAM_SIZE,&c,trial==5?NULL:&l,&d)==PR_U6C_0000_ARGUMENT);
        CHECK(memcmp(&before,&c,sizeof c)==0);CHECK(memcmp(saved,r,PR_U6C_0000_RAM_SIZE)==0);
    }
    c=setup(r,0x2800,0x8000);
    CHECK(pr_u6c_record_release_0000_cpu(r,PR_U6C_0000_RAM_SIZE,&c,&l,NULL)==PR_U6C_0000_OK);
    free(saved);free(r);printf("%u semantic checks\n",checks);return 0;
}
