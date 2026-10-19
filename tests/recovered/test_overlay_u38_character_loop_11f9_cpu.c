/* Synthetic records/strings and call binding; no original instruction body. */
#include "poolrad/recovered/overlay_u38_character_loop_11f9_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define RAM PR_U38_11F9_RAM_SIZE
#define CHECK(x) do { if(!(x)){fprintf(stderr,"U38 11F9 line %d: %s\n",__LINE__,#x);exit(1);}++checks; } while(0)
enum {CS=0x3000,DS=0x1000,SS=0x2000,SP=0x8000,A=0x4000,B=0x5000,P=0x6000,ACTIVE=0x7000};
static unsigned checks;
static uint8_t *ram;
static size_t at(unsigned s,unsigned o){return (((size_t)s<<4)+(o&65535u))&(RAM-1u);}
static void w8(unsigned s,unsigned o,unsigned v){ram[at(s,o)]=(uint8_t)v;}
static void w16(unsigned s,unsigned o,unsigned v){size_t p=at(s,o);ram[p]=(uint8_t)v;ram[(p+1)&(RAM-1u)]=(uint8_t)(v>>8);}
static unsigned r16(unsigned s,unsigned o){size_t p=at(s,o);return ram[p]|((unsigned)ram[(p+1)&(RAM-1u)]<<8);}
static void text(unsigned s,unsigned o,const char *v){size_t n=strlen(v);w8(s,o,(unsigned)n);for(size_t j=0;j<n;++j)w8(s,o+1+(unsigned)j,(unsigned char)v[j]);}
static void frame(unsigned ss,unsigned sp){
    static const unsigned v[]={0x7000,0x9001,0x1111,0x2222,0x3333,0x100,A};
    for(unsigned j=0;j<7;++j)w16(ss,sp+2*j,v[j]);
}
static pr_u38_11f9_cpu world(unsigned marker,unsigned kind,unsigned matches,unsigned count){
    pr_u38_11f9_cpu c={0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,SP,CS,0x11F9,DS,0x1111,SS,0x7603};
    memset(ram,0xA5,RAM);frame(SS,SP);
    w8(CS,0x1250,0x9A);w16(CS,0x1251,0xC10);w16(CS,0x1253,0xAF8);
    w16(A,0x208,0x200);w16(A,0x20A,B);w16(B,0x20A,0x300);w16(B,0x20C,P);
    w16(DS,0x5D92,0x100);w16(DS,0x5D94,ACTIVE);w8(ACTIVE,0x19F,marker);w8(ACTIVE,0x16C,kind);
    text(P,0x300,"ABC");
    for(unsigned j=0;j<8;++j)text(DS,0x402+16*j,(matches&(1u<<j))?"ABC":"ABD");
    w8(DS,0x6822,count);return c;
}
static void returned(const pr_u38_11f9_cpu *c){
    CHECK(c->cs==0x9001 && c->ip==0x7000 && c->sp==SP+14 && c->bp==0x7777 && c->ds==DS && c->ss==SS);
    CHECK(r16(DS,0x6826)==0x300 && r16(DS,0x6828)==P);
}
int main(void){
    pr_u38_11f9_cpu c,before;int status=99;
    ram=malloc(RAM);CHECK(ram!=NULL);
    for(unsigned mask=0;mask<256;++mask){
        unsigned matches=0,count=(mask*17)&255;
        for(unsigned bit=0;bit<8;++bit)matches+=(mask>>bit)&1u;
        c=world(1,mask&1?0x82:2,mask,count);
        CHECK(pr_u38_character_loop_11f9_cpu(ram,RAM,&c,7,&status)==PR_U38_11F9_OK && status==0);
        CHECK(ram[at(DS,0x6822)]==(uint8_t)(count-4*matches));
        CHECK(c.ax==0x0303 && c.bx==SP-16 && c.dx==DS && c.cx==0 && c.es==DS && c.si==0x304 && c.di==0x476);
        CHECK(c.flags==0x7246 && ram[at(SS,SP-3)]==8);returned(&c);
    }
    for(unsigned value=0;value<256;++value){
        c=world(value,2,255,0x31);before=c;
        CHECK(pr_u38_character_loop_11f9_cpu(ram,RAM,&c,7,&status)==PR_U38_11F9_OK);returned(&c);
        CHECK(ram[at(DS,0x6822)]==(value==1?0x11:0x31));
        if(value!=1){
            CHECK(status==-1 && c.es==ACTIVE && c.di==0x100 && c.ax==0x300 && c.dx==P);
            CHECK(c.bx==before.bx && c.cx==before.cx && c.si==before.si && (c.flags&0x400));
        }
        c=world(1,value,255,0x31);
        CHECK(pr_u38_character_loop_11f9_cpu(ram,RAM,&c,7,&status)==PR_U38_11F9_OK);returned(&c);
        if((value&127)!=2){
            CHECK(status==-1 && c.ax==(0x300u|(value&127u)) && c.es==ACTIVE && c.di==0x100 && c.dx==P);
            CHECK(ram[at(DS,0x6822)]==0x31 && (c.flags&0x400));
        }else CHECK(status==0 && ram[at(DS,0x6822)]==0x11 && !(c.flags&0x400));
    }
    for(unsigned cap=0;cap<8;++cap){
        c=world(1,2,255,0);
        CHECK(pr_u38_character_loop_11f9_cpu(ram,RAM,&c,cap,&status)==(cap==7?PR_U38_11F9_OK:PR_U38_11F9_LIMIT));
        CHECK(ram[at(DS,0x6822)]==(uint8_t)(0u-4u*(cap+1u)) && status==0 && ram[at(SS,SP-3)]==cap+1);
        if(cap<7)CHECK(c.ip==0x1236 && c.cs==CS && c.sp==SP-4 && c.bp==SP-2);
        else returned(&c);
    }
    /* The counter is also the decremented result byte: a match changes it
     * before CMP [BP-1],8. Empty strings in all 256 table slots match. */
    for(unsigned cap=16;cap<=167;cap+=151){
        unsigned counter=1,calls=0;
        for(;;){counter=(counter-4)&255;++calls;if(counter==8||calls>cap)break;counter=(counter+1)&255;}
        c=world(1,2,255,0);c.ss=DS;c.sp=0x6825;frame(DS,c.sp);
        w8(P,0x300,0);for(unsigned i=0;i<256;++i)w8(DS,0x3F2+16*i,0);
        CHECK(pr_u38_character_loop_11f9_cpu(ram,RAM,&c,cap,&status)==(cap==167?PR_U38_11F9_OK:PR_U38_11F9_LIMIT));
        CHECK(ram[at(DS,0x6822)]==counter && status==0);
        if(cap==167)CHECK(calls==168 && counter==8);
        else CHECK(c.ip==0x1236 && calls==17 && counter==205);
    }
    c=world(1,2,255,9);w8(CS,0x1250,0x90);
    CHECK(pr_u38_character_loop_11f9_cpu(ram,RAM,&c,7,&status)==PR_U38_11F9_CODE && status==-1);
    CHECK(c.ip==0x1250 && c.sp==SP-12 && c.bp==SP-2 && c.di==0x402 && c.ax==1);
    CHECK(r16(SS,c.sp)==0x402 && r16(SS,c.sp+2)==DS && r16(SS,c.sp+4)==0x300 && r16(SS,c.sp+6)==P && ram[at(DS,0x6822)]==9);
    c=world(0,2,255,9);w8(CS,0x1250,0x90);
    CHECK(pr_u38_character_loop_11f9_cpu(ram,RAM,&c,0,&status)==PR_U38_11F9_OK && status==-1);
    c=world(1,2,0,9);before=c;
    CHECK(pr_u38_character_loop_11f9_cpu(NULL,RAM,&c,7,&status)==PR_U38_11F9_ARGUMENT && status==-1 && memcmp(&c,&before,sizeof c)==0);
    CHECK(pr_u38_character_loop_11f9_cpu(ram,RAM,NULL,7,NULL)==PR_U38_11F9_ARGUMENT);
    CHECK(pr_u38_character_loop_11f9_cpu(ram,RAM-1,&c,7,NULL)==PR_U38_11F9_ARGUMENT);
    c.sp=15;CHECK(pr_u38_character_loop_11f9_cpu(ram,RAM,&c,7,NULL)==PR_U38_11F9_ARGUMENT && c.sp==15);
    c.sp=0xFFF3;CHECK(pr_u38_character_loop_11f9_cpu(ram,RAM,&c,7,NULL)==PR_U38_11F9_ARGUMENT && c.sp==0xFFF3);
    c=before;c.ip=0;CHECK(pr_u38_character_loop_11f9_cpu(ram,RAM,&c,7,NULL)==PR_U38_11F9_ARGUMENT);
    CHECK(r16(DS,0x6826)==0xA5A5);
    c=before;CHECK(pr_u38_character_loop_11f9_cpu(ram,RAM,&c,7,NULL)==PR_U38_11F9_OK);
    free(ram);printf("PASS: U38 11F9 %u semantic checks\n",checks);return 0;
}
