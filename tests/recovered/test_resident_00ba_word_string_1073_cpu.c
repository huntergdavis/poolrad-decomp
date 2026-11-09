#include "poolrad/recovered/resident_00ba_word_string_1073_cpu.h"
#include "poolrad/recovered/system_str_1366_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1);} }while(0)
static size_t at(unsigned s,unsigned o){return (((size_t)s<<4)+(o&65535u))&1048575u;}
static void put(uint8_t*m,unsigned s,unsigned o,unsigned v){size_t a=at(s,o);m[a]=(uint8_t)v;m[(a+1u)&1048575u]=(uint8_t)(v>>8);}
static void world(uint8_t*m,unsigned value,unsigned destseg){uint32_t power=1000000000u;unsigned i;
    memset(m,0xa5,PR_BUS_RAM_SIZE);for(i=0;i<10;i++,power/=10){put(m,0xaf8,0x12a6+4*i,power);put(m,0xaf8,0x12a8+4*i,power>>16);}
    m[at(0xba,0x108e)]=0x9a;put(m,0xba,0x108f,0x1366);put(m,0xba,0x1091,0xaf8);
    m[at(0xba,0x10a2)]=0x9a;put(m,0xba,0x10a3,0x0b25);put(m,0xba,0x10a5,0xaf8);
    put(m,0x2800,0x4000,0x7000);put(m,0x2800,0x4002,0x9001);put(m,0x2800,0x4004,value);
    put(m,0x2800,0x4006,0x0300);put(m,0x2800,0x4008,destseg);
}
static pr_res_1073_cpu cpu(void){pr_res_1073_cpu c={0x1111,0x2222,0x3333,0x4444,0x5555,0x6666,0x7777,0x4000,0xba,0x1073,0x1357,0x9999,0x2800,0x602};return c;}
int main(void){
    uint8_t *m=malloc(PR_BUS_RAM_SIZE),*saved=malloc(PR_BUS_RAM_SIZE);pr_ega *ega=calloc(1,sizeof *ega),*old=malloc(sizeof *old);
    pr_bus b={m,ega,NULL,NULL,NULL};pr_res_1073_cpu c,before;pr_res_1073_callee callee;int sub;unsigned i;
    const unsigned values[]={0,1,255,256,32768,65535};const char*texts[]={"0","1","255","256","32768","65535"};
    CHECK(m&&saved&&ega&&old);pr_ega_reset(ega);
    for(i=0;i<sizeof values/sizeof *values;i++){
        world(m,values[i],0x5000);c=cpu();
        CHECK(pr_res_00ba_1073_cpu(&b,PR_BUS_RAM_SIZE,&c,&callee,&sub)==PR_RES_1073_OK);
        CHECK(callee==PR_RES_1037_STRSTORE && sub==0 && c.cs==0x9001 && c.ip==0x7000 && c.sp==0x4006 && c.bp==0x7777);
        CHECK(m[at(0x5000,0x300)]==strlen(texts[i]) && memcmp(m+at(0x5000,0x301),texts[i],strlen(texts[i]))==0);
    }
    world(m,0,0x5000);c=cpu();c.ss=0xffff;
    put(m,c.ss,0x4000,0x7000);put(m,c.ss,0x4002,0x9001);put(m,c.ss,0x4004,65535);put(m,c.ss,0x4006,0x0300);put(m,c.ss,0x4008,0x5000);
    CHECK(pr_res_00ba_1073_cpu(&b,PR_BUS_RAM_SIZE,&c,&callee,&sub)==PR_RES_1073_OK && m[at(0x5000,0x300)]==5 && memcmp(m+at(0x5000,0x301),"65535",5)==0);
    world(m,0xabcd,0x5000);c=cpu();m[at(0xba,0x108e)]=0x90;
    CHECK(pr_res_00ba_1073_cpu(&b,PR_BUS_RAM_SIZE,&c,&callee,&sub)==PR_RES_1073_CODE && c.ip==0x108e && callee==PR_RES_1037_NO_CALLEE);
    CHECK(m[at(0x2800,0x3efa)]==0xcd && m[at(0x2800,0x3efb)]==0xab);
    world(m,256,0x2800);put(m,0x2800,0x4006,0x3eff);c=cpu();
    CHECK(pr_res_00ba_1073_cpu(&b,PR_BUS_RAM_SIZE,&c,&callee,&sub)==PR_RES_1073_OK);
    CHECK(m[at(0x2800,0x3eff)]==3 && m[at(0x2800,0x3f00)]==3 && m[at(0x2800,0x3f01)]==3 && m[at(0x2800,0x3f02)]==3);
    world(m,256,0x5000);c=cpu();m[at(0xba,0x10a2)]=0x90;
    CHECK(pr_res_00ba_1073_cpu(&b,PR_BUS_RAM_SIZE,&c,&callee,&sub)==PR_RES_1073_CODE && c.ip==0x10a2 && callee==PR_RES_1037_STR && m[at(0x2800,0x3efe)]==3);
    world(m,7,0x5000);c=cpu();put(m,0xaf8,0x12ca,2);
    CHECK(pr_res_00ba_1073_cpu(&b,PR_BUS_RAM_SIZE,&c,&callee,&sub)==PR_RES_1073_CALLEE && c.ip==0x1366 && callee==PR_RES_1037_STR && sub==PR_SYSTEM_LEAF_ARGUMENT);
    world(m,256,0xa000);c=cpu();memcpy(old,ega,sizeof *ega);
    CHECK(pr_res_00ba_1073_cpu(&b,PR_BUS_RAM_SIZE,&c,&callee,&sub)==PR_RES_1073_PROFILE && c.ip==0x0b25 && callee==PR_RES_1037_STR && memcmp(old,ega,sizeof *ega)==0);
    world(m,7,0x5000);c=cpu();put(m,0xba,0x10a5,0xa000);memcpy(old,ega,sizeof *ega);
    CHECK(pr_res_00ba_1073_cpu(&b,PR_BUS_RAM_SIZE,&c,&callee,&sub)==PR_RES_1073_PROFILE && c.cs==0xa000 && c.ip==0x0b25 && callee==PR_RES_1037_STR && memcmp(old,ega,sizeof *ega)==0);
    world(m,7,0x5000);c=cpu();put(m,0xba,0x1091,0xa000);memcpy(old,ega,sizeof *ega);
    CHECK(pr_res_00ba_1073_cpu(&b,PR_BUS_RAM_SIZE,&c,&callee,&sub)==PR_RES_1073_PROFILE && c.cs==0xa000 && c.ip==0x1366 && memcmp(old,ega,sizeof *ega)==0);
    world(m,7,0x5000);c=cpu();c.ss=0x9000;before=c;memcpy(saved,m,PR_BUS_RAM_SIZE);memcpy(old,ega,sizeof *ega);
    CHECK(pr_res_00ba_1073_cpu(&b,PR_BUS_RAM_SIZE,&c,&callee,&sub)==PR_RES_1073_ARGUMENT && memcmp(&c,&before,sizeof c)==0 && memcmp(saved,m,PR_BUS_RAM_SIZE)==0 && memcmp(old,ega,sizeof *ega)==0);
    free(old);free(ega);free(saved);free(m);puts("1073 word formatter semantic checks passed");return 0;
}
