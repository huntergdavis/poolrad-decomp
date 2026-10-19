/* Synthetic constants/records/bindings only; no original game strings. */
#include "poolrad/recovered/overlay_u38_character_compare_1276_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define RAM PR_U38_1276_RAM_SIZE
#define CHECK(x) do{if(!(x)){fprintf(stderr,"U38 1276 line %d: %s\n",__LINE__,#x);exit(1);}++checks;}while(0)
enum{CS=0x3000,DS=0x1000,SS=0x2000,SP=0x8000,P=0x6000};
static uint8_t *ram;static unsigned checks;
static size_t at(unsigned s,unsigned o){return (((size_t)s<<4)+(o&65535u))&(RAM-1u);}
static void w8(unsigned s,unsigned o,unsigned v){ram[at(s,o)]=(uint8_t)v;}
static void w16(unsigned s,unsigned o,unsigned v){size_t p=at(s,o);ram[p]=(uint8_t)v;ram[(p+1)&(RAM-1u)]=(uint8_t)(v>>8);}
static unsigned r16(unsigned s,unsigned o){size_t p=at(s,o);return ram[p]|((unsigned)ram[(p+1)&(RAM-1u)]<<8);}
static void string(unsigned s,unsigned o,const char *v){size_t n=strlen(v);w8(s,o,(unsigned)n);for(size_t j=0;j<n;++j)w8(s,o+1+(unsigned)j,(unsigned char)v[j]);}
static void frame(unsigned ss,unsigned sp){
    static const unsigned words[]={0x7000,0x9001,0x1111,0x2222,0x3333,0x4444,0x5555};
    for(unsigned i=0;i<7;++i)w16(ss,sp+2*i,words[i]);
}
static pr_u38_1276_cpu world(unsigned marker,unsigned mode,unsigned count){
    pr_u38_1276_cpu c={0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,SP,CS,0x1276,DS,0x1111,SS,0x7603};
    memset(ram,0xA5,RAM);frame(SS,SP);
    for(unsigned i=0;i<2;++i){unsigned site=i?0x12A2:0x1290;w8(CS,site,0x9A);w16(CS,site+1,0xC10);w16(CS,site+3,i?0x7500:0x7400);}
    string(CS,0x1268,"AAAAAAA");string(CS,0x1270,"BBBBB");
    w16(DS,0x5D92,0x300);w16(DS,0x5D94,P);
    string(P,0x300,mode==0?"AAAAAAA":mode==1?"BBBBB":mode==2?"CCCCCCC":"");
    w8(P,0x39F,marker);w8(DS,0x6822,count);return c;
}
static unsigned arithmetic_flags(unsigned left,unsigned right){
    unsigned value=(left-right)&255,result=0,pop=0;
    for(unsigned i=0;i<8;++i)pop+=(value>>i)&1u;
    if(left<right)result|=1;
    if(!(pop&1))result|=4;
    if((left&15)<(right&15))result|=16;
    if(value==0)result|=64;
    if(value>=128)result|=128;
    int signed_left=left>=128?(int)left-256:(int)left;
    int signed_right=right>=128?(int)right-256:(int)right;
    int difference=signed_left-signed_right;
    if(difference < -128 || difference > 127)result|=0x800;
    return result;
}
static void returned(const pr_u38_1276_cpu *c){CHECK(c->cs==0x9001 && c->ip==0x7000 && c->sp==SP+14 && c->bp==0x7777 && c->ds==DS && c->ss==SS);}
int main(void){
    pr_u38_1276_cpu c,before;int callee=99;
    ram=malloc(RAM);CHECK(ram!=NULL);
    for(unsigned count=0;count<256;++count)for(unsigned mode=0;mode<4;++mode){
        c=world(1,mode,count);
        CHECK(pr_u38_character_compare_1276_cpu(ram,RAM,&c,&callee)==PR_U38_1276_OK && callee==0);returned(&c);
        CHECK(ram[at(DS,0x6822)]==(mode<2?(uint8_t)(count-4):(uint8_t)count));
        CHECK(c.dx==DS && c.es==CS && c.bx==SP-14 && !(c.flags&0x400));
        if(mode<2){
            unsigned length=mode?5:7;
            CHECK(c.ax==(length|(length<<8)) && c.si==0x301+length && c.di==(mode?0x1276:0x1270) && c.cx==0);
            CHECK(c.flags==(0x7202u|arithmetic_flags(count,4)));
            CHECK(r16(SS,SP-14)==(mode?0x12A7:0x1295));
        }else if(mode==2)CHECK(c.ax==0x0507 && c.cx==4 && c.flags==0x7202);
        else CHECK(c.ax==0x0500 && c.cx==0x5600 && c.flags==(0x7202u|arithmetic_flags(0,5)));
    }
    for(unsigned marker=0;marker<256;++marker){
        c=world(marker,1,0x31);before=c;
        CHECK(pr_u38_character_compare_1276_cpu(ram,RAM,&c,&callee)==PR_U38_1276_OK);returned(&c);
        if(marker!=1){
            CHECK(callee==-1 && ram[at(DS,0x6822)]==0x31 && c.flags==(0x7602u|arithmetic_flags(marker,1)));
            CHECK(c.ax==before.ax && c.bx==before.bx && c.cx==before.cx && c.dx==before.dx && c.si==before.si && c.es==P && c.di==0x300);
        }else CHECK(callee==0 && ram[at(DS,0x6822)]==0x2D);
    }
    for(unsigned which=0;which<2;++which){
        unsigned site=which?0x12A2:0x1290,constant=which?0x1270:0x1268;
        c=world(1,1,8);w8(CS,site,0x90);
        CHECK(pr_u38_character_compare_1276_cpu(ram,RAM,&c,&callee)==PR_U38_1276_CODE && c.ip==site && c.sp==SP-10 && callee==(which?0:-1));
        CHECK(r16(SS,c.sp)==constant && r16(SS,c.sp+2)==CS && r16(SS,c.sp+4)==0x300 && r16(SS,c.sp+6)==P && ram[at(DS,0x6822)]==8);
    }
    c=world(0,1,8);w8(CS,0x1290,0x90);
    CHECK(pr_u38_character_compare_1276_cpu(ram,RAM,&c,&callee)==PR_U38_1276_OK && callee==-1);
    c=world(1,0,8);w8(CS,0x12A2,0x90);
    CHECK(pr_u38_character_compare_1276_cpu(ram,RAM,&c,&callee)==PR_U38_1276_OK && ram[at(DS,0x6822)]==4);
    /* First call's pushed return overwrites DS:5D92/5D94. The second
     * comparison must reload that pointer rather than reuse the old name. */
    c=world(1,1,8);c.ss=DS;c.sp=0x5DA0;frame(c.ss,c.sp);
    CHECK(pr_u38_character_compare_1276_cpu(ram,RAM,&c,&callee)==PR_U38_1276_OK && callee==0);
    CHECK(ram[at(DS,0x6822)]==8 && r16(DS,0x5D92)==0x12A7 && r16(DS,0x5D94)==CS);
    c=world(1,0,8);c.ss=DS;c.sp=0x6822;frame(c.ss,c.sp);
    CHECK(pr_u38_character_compare_1276_cpu(ram,RAM,&c,&callee)==PR_U38_1276_OK && c.ip==0x70FC && c.cs==0x9001 && c.sp==0x6830);
    c=world(1,0,8);c.ss=DS;c.sp=0x6824;frame(c.ss,c.sp);
    CHECK(pr_u38_character_compare_1276_cpu(ram,RAM,&c,&callee)==PR_U38_1276_OK && c.bp==0x7773);
    c=world(1,0,8);before=c;
    CHECK(pr_u38_character_compare_1276_cpu(NULL,RAM,&c,&callee)==PR_U38_1276_ARGUMENT && callee==-1 && memcmp(&c,&before,sizeof c)==0);
    CHECK(pr_u38_character_compare_1276_cpu(ram,RAM,NULL,NULL)==PR_U38_1276_ARGUMENT);
    CHECK(pr_u38_character_compare_1276_cpu(ram,RAM-1,&c,NULL)==PR_U38_1276_ARGUMENT);
    c.sp=13;CHECK(pr_u38_character_compare_1276_cpu(ram,RAM,&c,NULL)==PR_U38_1276_ARGUMENT && c.sp==13);
    c.sp=0xFFF3;CHECK(pr_u38_character_compare_1276_cpu(ram,RAM,&c,NULL)==PR_U38_1276_ARGUMENT && c.sp==0xFFF3);
    c=before;c.ip=0;CHECK(pr_u38_character_compare_1276_cpu(ram,RAM,&c,NULL)==PR_U38_1276_ARGUMENT);
    CHECK(r16(SS,SP-2)==0xA5A5);
    c=before;CHECK(pr_u38_character_compare_1276_cpu(ram,RAM,&c,NULL)==PR_U38_1276_OK);
    free(ram);printf("PASS: U38 1276 %u semantic checks\n",checks);return 0;
}
