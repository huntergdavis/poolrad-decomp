#include "poolrad/recovered/overlay_u3c2_stack_load_0608_fixture.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static void run(uint8_t *ram,uint8_t *before,uint16_t ss,uint16_t bp,unsigned value,uint16_t flags)
{
    pr_u6c_3f31_cpu cpu={0x1234,0x2345,0x3456,0x4567,0x5678,0x6789,bp,0xFFFE,
                        0x1404,0x0608,0x3000,0x4000,ss,flags},want=cpu;
    size_t at=((size_t)ss*16u+((unsigned)bp+65533u)%65536u)%1048576u;
    memset(ram,0xA5,PR_U6C_3F31_RAM_SIZE);ram[at]=(uint8_t)value;
    ram[(at+1u)%1048576u]=(uint8_t)~value;memcpy(before,ram,PR_U6C_3F31_RAM_SIZE);
    want.ax=(uint16_t)(0x1200u+value);want.ip=0x060B;
    assert(pr_u3c2_stack_load_0608_fixture(ram,PR_U6C_3F31_RAM_SIZE,&cpu,PR_U3C2_0608_AF_PRESERVE_SUPPLIED)==PR_U3C2_0608_OK);
    assert(memcmp(&cpu,&want,sizeof(cpu))==0);assert(memcmp(ram,before,PR_U6C_3F31_RAM_SIZE)==0);
}
int main(void)
{
    static const uint16_t indexes[]={0,1,2,3,4,0xFFFF,0xFFFE,18,19,20};
    static const uint16_t segments[]={0,0x2000,0xFFFF};
    uint8_t *ram=malloc(PR_U6C_3F31_RAM_SIZE),*before=malloc(PR_U6C_3F31_RAM_SIZE);
    unsigned v,f,i,j,cases=0;uint32_t seed=0x06082736u;
    assert(ram&&before);
    for(v=0;v<256u;++v)for(f=0;f<2u;++f){run(ram,before,0x2000,0x1234,v,f?0xFFFF:0);++cases;}
    for(i=0;i<3u;++i)for(j=0;j<10u;++j){run(ram,before,segments[i],indexes[j],(i+j)%2u?255u:127u,0x5555);++cases;}
    for(i=0;i<8u;++i){run(ram,before,0x1404,(uint16_t)(0x0608u+3u+i),i%2u?255u:128u,0xAAAA);++cases;}
    assert(cases==550u);
    {
        static const struct { uint16_t ss,bp;size_t at; } literal[]={
            {0,0,65533u},{0,2,65535u},{0,3,0u},{0x2000,2,196607u},
            {0xFFFF,0,65517u},{0xFFFF,18,1048575u},
            {0xFFFF,19,0u},{0xFFFF,0xFFFF,65516u}
        };
        for(i=0;i<8u;++i){
            pr_u6c_3f31_cpu cpu={0xABCD,1,2,3,4,5,literal[i].bp,0xFFFE,
                                0x1404,0x0608,0x3000,0x4000,literal[i].ss,0xA55A},want=cpu;
            memset(ram,0xA5,PR_U6C_3F31_RAM_SIZE);ram[literal[i].at]=0x39;
            memcpy(before,ram,PR_U6C_3F31_RAM_SIZE);want.ax=0xAB39;want.ip=0x060B;
            assert(pr_u3c2_stack_load_0608_fixture(ram,PR_U6C_3F31_RAM_SIZE,&cpu,PR_U3C2_0608_AF_PRESERVE_SUPPLIED)==PR_U3C2_0608_OK);
            assert(memcmp(&cpu,&want,sizeof(cpu))==0);
            assert(memcmp(ram,before,PR_U6C_3F31_RAM_SIZE)==0);
        }
    }
    /* Nonperiodic background detects incorrect 64KiB offset wrap. */
    for(i=0;i<PR_U6C_3F31_RAM_SIZE;++i){seed^=seed<<13u;seed^=seed>>17u;seed^=seed<<5u;ram[i]=(uint8_t)seed;}
    memcpy(before,ram,PR_U6C_3F31_RAM_SIZE);
    for(v=0;v<65536u;++v){
        pr_u6c_3f31_cpu cpu={0},want;size_t at;
        cpu.ip=0x0608;cpu.bp=(uint16_t)v;cpu.ss=(uint16_t)(v*13u);cpu.ax=(uint16_t)(v^0xABCDu);cpu.flags=(uint16_t)(v*17u);want=cpu;
        at=((size_t)cpu.ss*16u+(v+65533u)%65536u)%1048576u;
        want.ax=(uint16_t)((cpu.ax/256u)*256u+ram[at]);want.ip=0x060B;
        assert(pr_u3c2_stack_load_0608_fixture(ram,PR_U6C_3F31_RAM_SIZE,&cpu,PR_U3C2_0608_AF_PRESERVE_SUPPLIED)==PR_U3C2_0608_OK);
        assert(memcmp(&cpu,&want,sizeof(cpu))==0);
    }
    assert(memcmp(ram,before,PR_U6C_3F31_RAM_SIZE)==0);
    {
        static const int policies[]={0,2,255,-1};
        pr_u6c_3f31_cpu cpu={0},want;cpu.ip=0x0608;cpu.flags=0xFFFF;want=cpu;
        for(i=0;i<4u;++i){
            assert(pr_u3c2_stack_load_0608_fixture(ram,PR_U6C_3F31_RAM_SIZE,&cpu,(pr_u3c2_0608_af_policy)policies[i])==PR_U3C2_0608_ARGUMENT);
            assert(memcmp(&cpu,&want,sizeof(cpu))==0);assert(memcmp(ram,before,PR_U6C_3F31_RAM_SIZE)==0);
        }
        assert(pr_u3c2_stack_load_0608_fixture(NULL,PR_U6C_3F31_RAM_SIZE,&cpu,PR_U3C2_0608_AF_PRESERVE_SUPPLIED)==PR_U3C2_0608_ARGUMENT);
        assert(pr_u3c2_stack_load_0608_fixture(ram,PR_U6C_3F31_RAM_SIZE-1u,&cpu,PR_U3C2_0608_AF_PRESERVE_SUPPLIED)==PR_U3C2_0608_ARGUMENT);
        assert(pr_u3c2_stack_load_0608_fixture(ram,PR_U6C_3F31_RAM_SIZE,NULL,PR_U3C2_0608_AF_PRESERVE_SUPPLIED)==PR_U3C2_0608_ARGUMENT);
        assert(memcmp(&cpu,&want,sizeof(cpu))==0);
        cpu.ip=0x060B;want=cpu;
        assert(pr_u3c2_stack_load_0608_fixture(ram,PR_U6C_3F31_RAM_SIZE,&cpu,PR_U3C2_0608_AF_PRESERVE_SUPPLIED)==PR_U3C2_0608_ARGUMENT);
        assert(memcmp(&cpu,&want,sizeof(cpu))==0);assert(memcmp(ram,before,PR_U6C_3F31_RAM_SIZE)==0);
    }
    free(before);free(ram);return 0;
}
