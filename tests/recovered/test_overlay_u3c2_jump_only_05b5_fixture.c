#include "poolrad/recovered/overlay_u3c2_jump_only_05b5_fixture.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static void check(pr_u6c_3f31_cpu *cpu,uint8_t *ram)
{
    pr_u6c_3f31_cpu want=*cpu;
    want.ip=0x0900;
    assert(pr_u3c2_jump_only_05b5_fixture(ram,PR_U6C_3F31_RAM_SIZE,cpu,
           PR_U3C2_05B5_AF_PRESERVE_SUPPLIED)==PR_U3C2_05B5_OK);
    assert(memcmp(cpu,&want,sizeof(want))==0);
}
int main(void)
{
    uint8_t *ram=malloc(PR_U6C_3F31_RAM_SIZE),*before=malloc(PR_U6C_3F31_RAM_SIZE);
    unsigned v,i,af_counts[2]={0};
    assert(ram&&before);memset(ram,0xA5,PR_U6C_3F31_RAM_SIZE);memcpy(before,ram,PR_U6C_3F31_RAM_SIZE);
    for(v=0;v<65536u;++v) {
        pr_u6c_3f31_cpu cpu={(uint16_t)~v,0x2345,0x3456,0x4567,0x5678,0x6789,
                            (uint16_t)v,(uint16_t)(v*17u),0x1404,0x05B5,
                            0x1000,0x3000,0xFFFF,(uint16_t)v};
        check(&cpu,ram);++af_counts[(cpu.flags/16u)%2u];
    }
    assert(af_counts[0]==32768u && af_counts[1]==32768u);
    for(v=0;v<65536u;++v) {
        pr_u6c_3f31_cpu cpu={0};cpu.ip=0x05B5;cpu.ax=(uint16_t)v;cpu.flags=(uint16_t)(v*17u);
        check(&cpu,ram);
    }
    assert(memcmp(ram,before,PR_U6C_3F31_RAM_SIZE)==0);
    for(i=0;i<8u;++i) {
        pr_u6c_3f31_cpu cpu={0};cpu.ip=0x05B5;cpu.ax=(uint16_t)(i*8191u);cpu.flags=(uint16_t)(i*9361u);
        cpu.ss=cpu.ds=cpu.cs=0xFFFF;cpu.sp=cpu.bp=(uint16_t)(65535u-i);
        memset(ram,(int)(i*31u),PR_U6C_3F31_RAM_SIZE);memcpy(before,ram,PR_U6C_3F31_RAM_SIZE);
        check(&cpu,ram);assert(memcmp(ram,before,PR_U6C_3F31_RAM_SIZE)==0);
    }
    {
        static const int policies[]={0,2,255,-1};
        pr_u6c_3f31_cpu cpu={0},want;cpu.ip=0x05B5;cpu.flags=0xFFFF;want=cpu;
        for(i=0;i<4u;++i) {
            assert(pr_u3c2_jump_only_05b5_fixture(ram,PR_U6C_3F31_RAM_SIZE,&cpu,
                   (pr_u3c2_05b5_af_policy)policies[i])==PR_U3C2_05B5_ARGUMENT);
            assert(memcmp(&cpu,&want,sizeof(cpu))==0);assert(memcmp(ram,before,PR_U6C_3F31_RAM_SIZE)==0);
        }
        assert(pr_u3c2_jump_only_05b5_fixture(NULL,PR_U6C_3F31_RAM_SIZE,&cpu,PR_U3C2_05B5_AF_PRESERVE_SUPPLIED)==PR_U3C2_05B5_ARGUMENT);
        assert(pr_u3c2_jump_only_05b5_fixture(ram,PR_U6C_3F31_RAM_SIZE-1u,&cpu,PR_U3C2_05B5_AF_PRESERVE_SUPPLIED)==PR_U3C2_05B5_ARGUMENT);
        assert(pr_u3c2_jump_only_05b5_fixture(ram,PR_U6C_3F31_RAM_SIZE,NULL,PR_U3C2_05B5_AF_PRESERVE_SUPPLIED)==PR_U3C2_05B5_ARGUMENT);
        assert(memcmp(&cpu,&want,sizeof(cpu))==0);
        cpu.ip=0x0900;want=cpu;
        assert(pr_u3c2_jump_only_05b5_fixture(ram,PR_U6C_3F31_RAM_SIZE,&cpu,PR_U3C2_05B5_AF_PRESERVE_SUPPLIED)==PR_U3C2_05B5_ARGUMENT);
        assert(memcmp(&cpu,&want,sizeof(cpu))==0);assert(memcmp(ram,before,PR_U6C_3F31_RAM_SIZE)==0);
    }
    free(before);free(ram);return 0;
}
