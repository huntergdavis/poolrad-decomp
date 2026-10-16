#include "poolrad/recovered/overlay_u3c2_je_decision_05fe_fixture.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    static const uint16_t cases[] = {0,0x40,0xFFFF,0xFFBF,0x5555,0xAAAA,0x41,1};
    uint8_t *ram=malloc(PR_U6C_3F31_RAM_SIZE),*before=malloc(PR_U6C_3F31_RAM_SIZE);
    unsigned flags,i,taken=0,zf_classes[2]={0};
    assert(ram && before);
    memset(ram,0xA5,PR_U6C_3F31_RAM_SIZE);
    memcpy(before,ram,PR_U6C_3F31_RAM_SIZE);
    for(flags=0;flags<65536u;++flags) {
        pr_u6c_3f31_cpu cpu={(uint16_t)flags,(uint16_t)~flags,0xFFFF,0,0x8000,0x7FFF,
                            (uint16_t)(flags*17u),(uint16_t)(flags*31u),0x1404,0x05FE,
                            0x1000,0x3000,0xFFFF,(uint16_t)flags},want=cpu;
        unsigned zf=(flags/64u)%2u;
        want.ip=zf!=0u?0x0610:0x0600;
        assert(pr_u3c2_je_decision_05fe_fixture(ram,PR_U6C_3F31_RAM_SIZE,&cpu)==PR_U3C2_05FE_OK);
        assert(memcmp(&cpu,&want,sizeof(cpu))==0);
        ++zf_classes[zf];if(cpu.ip==0x0610)++taken;
    }
    assert(taken==32768u);
    for(i=0;i<2;++i)assert(zf_classes[i]==32768u);
    assert(memcmp(ram,before,PR_U6C_3F31_RAM_SIZE)==0);
    for(i=0;i<sizeof(cases)/sizeof(cases[0]);++i) {
        pr_u6c_3f31_cpu cpu={0},want;
        cpu.ip=0x05FE;cpu.flags=cases[i];cpu.ss=0xFFFF;cpu.ds=0x3000;cpu.es=0x4000;cpu.cs=0x1404;
        cpu.sp=(uint16_t)(65535u-i);cpu.bp=(uint16_t)(i*8191u);want=cpu;
        want.ip=(cases[i]/64u)%2u!=0u?0x0610:0x0600;
        memset(ram,(int)(i*31u),PR_U6C_3F31_RAM_SIZE);memcpy(before,ram,PR_U6C_3F31_RAM_SIZE);
        assert(pr_u3c2_je_decision_05fe_fixture(ram,PR_U6C_3F31_RAM_SIZE,&cpu)==PR_U3C2_05FE_OK);
        assert(memcmp(&cpu,&want,sizeof(cpu))==0);assert(memcmp(ram,before,PR_U6C_3F31_RAM_SIZE)==0);
    }
    {
        pr_u6c_3f31_cpu cpu={0},want;
        cpu.ip=0x05FE;cpu.flags=0xFFFF;want=cpu;
        assert(pr_u3c2_je_decision_05fe_fixture(NULL,PR_U6C_3F31_RAM_SIZE,&cpu)==PR_U3C2_05FE_ARGUMENT);
        assert(pr_u3c2_je_decision_05fe_fixture(ram,PR_U6C_3F31_RAM_SIZE-1u,&cpu)==PR_U3C2_05FE_ARGUMENT);
        assert(pr_u3c2_je_decision_05fe_fixture(ram,PR_U6C_3F31_RAM_SIZE,NULL)==PR_U3C2_05FE_ARGUMENT);
        assert(memcmp(&cpu,&want,sizeof(cpu))==0);
        cpu.ip=0x0600;want=cpu;
        assert(pr_u3c2_je_decision_05fe_fixture(ram,PR_U6C_3F31_RAM_SIZE,&cpu)==PR_U3C2_05FE_ARGUMENT);
        assert(memcmp(&cpu,&want,sizeof(cpu))==0);assert(memcmp(ram,before,PR_U6C_3F31_RAM_SIZE)==0);
    }
    free(before);free(ram);return 0;
}
