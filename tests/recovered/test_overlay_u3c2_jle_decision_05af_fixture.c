#include "poolrad/recovered/overlay_u3c2_jle_decision_05af_fixture.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    static const uint16_t cases[] = {0,0x80,0x800,0x880,0xFFFF,0xF77F,0x5555,0xAAAA};
    uint8_t *ram=malloc(PR_U6C_3F31_RAM_SIZE),*before=malloc(PR_U6C_3F31_RAM_SIZE);
    unsigned flags,i,taken=0,quadrants[8]={0};
    assert(ram && before);
    memset(ram,0xA5,PR_U6C_3F31_RAM_SIZE);
    memcpy(before,ram,PR_U6C_3F31_RAM_SIZE);
    for(flags=0;flags<65536u;++flags) {
        pr_u6c_3f31_cpu cpu={(uint16_t)flags,(uint16_t)~flags,0xFFFF,0,0x8000,0x7FFF,
                            (uint16_t)(flags*17u),(uint16_t)(flags*31u),0x1404,0x05AF,
                            0x1000,0x3000,0xFFFF,(uint16_t)flags},want=cpu;
        unsigned sf=(flags/128u)%2u,of=(flags/2048u)%2u;
        want.ip=sf!=of || (flags/64u)%2u!=0u?0x05B8:0x05B1;
        assert(pr_u3c2_jle_decision_05af_fixture(ram,PR_U6C_3F31_RAM_SIZE,&cpu)==PR_U3C2_05AF_OK);
        assert(memcmp(&cpu,&want,sizeof(cpu))==0);
        ++quadrants[sf+2u*of+4u*((flags/64u)%2u)];if(cpu.ip==0x05B8)++taken;
    }
    assert(taken==49152u);
    for(i=0;i<8;++i)assert(quadrants[i]==8192u);
    assert(memcmp(ram,before,PR_U6C_3F31_RAM_SIZE)==0);
    for(i=0;i<sizeof(cases)/sizeof(cases[0]);++i) {
        pr_u6c_3f31_cpu cpu={0},want;
        cpu.ip=0x05AF;cpu.flags=cases[i];cpu.ss=cpu.ds=cpu.cs=0xFFFF;
        cpu.sp=(uint16_t)(65535u-i);cpu.bp=(uint16_t)(i*8191u);want=cpu;
        want.ip=((cases[i]/128u)%2u)!=((cases[i]/2048u)%2u) || (cases[i]/64u)%2u!=0u?0x05B8:0x05B1;
        memset(ram,(int)(i*31u),PR_U6C_3F31_RAM_SIZE);memcpy(before,ram,PR_U6C_3F31_RAM_SIZE);
        assert(pr_u3c2_jle_decision_05af_fixture(ram,PR_U6C_3F31_RAM_SIZE,&cpu)==PR_U3C2_05AF_OK);
        assert(memcmp(&cpu,&want,sizeof(cpu))==0);assert(memcmp(ram,before,PR_U6C_3F31_RAM_SIZE)==0);
    }
    {
        pr_u6c_3f31_cpu cpu={0},want;
        cpu.ip=0x05AF;cpu.flags=0xFFFF;want=cpu;
        assert(pr_u3c2_jle_decision_05af_fixture(NULL,PR_U6C_3F31_RAM_SIZE,&cpu)==PR_U3C2_05AF_ARGUMENT);
        assert(pr_u3c2_jle_decision_05af_fixture(ram,PR_U6C_3F31_RAM_SIZE-1u,&cpu)==PR_U3C2_05AF_ARGUMENT);
        assert(pr_u3c2_jle_decision_05af_fixture(ram,PR_U6C_3F31_RAM_SIZE,NULL)==PR_U3C2_05AF_ARGUMENT);
        assert(memcmp(&cpu,&want,sizeof(cpu))==0);
        cpu.ip=0x05B1;want=cpu;
        assert(pr_u3c2_jle_decision_05af_fixture(ram,PR_U6C_3F31_RAM_SIZE,&cpu)==PR_U3C2_05AF_ARGUMENT);
        assert(memcmp(&cpu,&want,sizeof(cpu))==0);assert(memcmp(ram,before,PR_U6C_3F31_RAM_SIZE)==0);
    }
    free(before);free(ram);return 0;
}
