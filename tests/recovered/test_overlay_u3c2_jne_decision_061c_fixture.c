#include "poolrad/recovered/overlay_u3c2_jne_decision_061c_fixture.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    static const uint16_t cases[] = {0,0x40,0xFFFF,0xFFBF,0x5555,0xAAAA,0x41,1};
    static const uint16_t literal_stops[] = {0x0676,0x061E,0x061E,0x0676,0x061E,0x0676,0x061E,0x0676};
    uint8_t *ram=malloc(PR_U6C_3F31_RAM_SIZE),*before=malloc(PR_U6C_3F31_RAM_SIZE);
    unsigned flags,i,taken=0,zf_classes[2]={0};
    size_t j;
    assert(ram && before);
    for(j=0;j<PR_U6C_3F31_RAM_SIZE;++j)
        ram[j]=(uint8_t)((j*73u)^(j>>8u)^(j>>16u));
    memcpy(before,ram,PR_U6C_3F31_RAM_SIZE);
    for(flags=0;flags<65536u;++flags) {
        pr_u6c_3f31_cpu cpu={(uint16_t)flags,(uint16_t)~flags,0xFFFF,0,0x8000,0x7FFF,
                            (uint16_t)(flags*17u),(uint16_t)(flags*31u),0x1404,0x061c,
                            0x1000,0x3000,0xFFFF,(uint16_t)flags},want=cpu;
        unsigned zf=(flags/64u)%2u;
        want.ip=zf==0u?0x0676:0x061E;
        assert(pr_u3c2_jne_decision_061c_fixture(ram,PR_U6C_3F31_RAM_SIZE,&cpu)==PR_U3C2_061C_OK);
        assert(memcmp(&cpu,&want,sizeof(cpu))==0);
        ++zf_classes[zf];if(cpu.ip==0x0676)++taken;
    }
    assert(taken==32768u);
    for(i=0;i<2;++i)assert(zf_classes[i]==32768u);
    assert(memcmp(ram,before,PR_U6C_3F31_RAM_SIZE)==0);
    for(i=0;i<sizeof(cases)/sizeof(cases[0]);++i) {
        pr_u6c_3f31_cpu cpu={0},want;
        cpu.ip=0x061c;cpu.flags=cases[i];cpu.ss=0xFFFF;cpu.ds=0x3000;cpu.es=0x4000;cpu.cs=0x1404;
        cpu.sp=(uint16_t)(65535u-i);cpu.bp=(uint16_t)(i*8191u);want=cpu;
        want.ip=literal_stops[i];
        for(j=0;j<PR_U6C_3F31_RAM_SIZE;++j)
            ram[j]=(uint8_t)((j*73u)^(j>>8u)^(j>>16u)^(i*31u));
        memcpy(before,ram,PR_U6C_3F31_RAM_SIZE);
        assert(pr_u3c2_jne_decision_061c_fixture(ram,PR_U6C_3F31_RAM_SIZE,&cpu)==PR_U3C2_061C_OK);
        assert(memcmp(&cpu,&want,sizeof(cpu))==0);assert(memcmp(ram,before,PR_U6C_3F31_RAM_SIZE)==0);
    }
    {
        pr_u6c_3f31_cpu cpu={0},want;
        cpu.ip=0x061c;cpu.flags=0xFFFF;want=cpu;
        assert(pr_u3c2_jne_decision_061c_fixture(NULL,PR_U6C_3F31_RAM_SIZE,&cpu)==PR_U3C2_061C_ARGUMENT);
        assert(pr_u3c2_jne_decision_061c_fixture(ram,PR_U6C_3F31_RAM_SIZE-1u,&cpu)==PR_U3C2_061C_ARGUMENT);
        assert(pr_u3c2_jne_decision_061c_fixture(ram,PR_U6C_3F31_RAM_SIZE,NULL)==PR_U3C2_061C_ARGUMENT);
        assert(memcmp(&cpu,&want,sizeof(cpu))==0);
        cpu.ip=0x061E;want=cpu;
        assert(pr_u3c2_jne_decision_061c_fixture(ram,PR_U6C_3F31_RAM_SIZE,&cpu)==PR_U3C2_061C_ARGUMENT);
        assert(memcmp(&cpu,&want,sizeof(cpu))==0);assert(memcmp(ram,before,PR_U6C_3F31_RAM_SIZE)==0);
    }
    free(before);free(ram);return 0;
}
