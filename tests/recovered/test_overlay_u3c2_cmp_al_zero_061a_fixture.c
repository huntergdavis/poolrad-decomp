#include "poolrad/recovered/overlay_u3c2_cmp_al_zero_061a_fixture.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

/* Independent immediate-zero reference: bit count and literal flag masks. */
static uint16_t reference(unsigned al,uint16_t old)
{
    unsigned i,ones=0;
    uint16_t flags=(uint16_t)(old & 0xF72Au);
    for(i=0;i<8u;++i)ones+=(al>>i)&1u;
    if(ones%2u==0u)flags|=4u;
    if(al==0u)flags|=0x40u;
    if(al>=128u)flags|=0x80u;
    return flags; /* CF/AF/OF remain zero for subtraction by zero. */
}
static void check(uint8_t *ram,pr_u6c_3f31_cpu *cpu)
{
    pr_u6c_3f31_cpu want=*cpu;
    want.flags=reference((unsigned)cpu->ax%256u,cpu->flags);want.ip=0x061C;
    assert(pr_u3c2_cmp_al_zero_061a_fixture(ram,PR_U6C_3F31_RAM_SIZE,cpu,
           PR_U3C2_061A_AF_CMP_DEFINED)==PR_U3C2_061A_OK);
    assert(memcmp(cpu,&want,sizeof(want))==0);
}
int main(void)
{
    static const uint16_t flags_cases[]={0,0xFFFF,0xAAAA,0x5555,0x10,0x40,0x800,0x8D5};
    static const uint8_t edge_al[]={0,1,0x7F,0x80,0xFF,0x55,0xAA,0xFE};
    uint8_t *ram=malloc(PR_U6C_3F31_RAM_SIZE),*before=malloc(PR_U6C_3F31_RAM_SIZE);
    unsigned a,i,v;uint32_t seed=0x061A2736u;
    assert(ram&&before);
    for(i=0;i<PR_U6C_3F31_RAM_SIZE;++i){seed^=seed<<13u;seed^=seed>>17u;seed^=seed<<5u;ram[i]=(uint8_t)seed;}
    memcpy(before,ram,PR_U6C_3F31_RAM_SIZE);
    for(a=0;a<256u;++a)for(i=0;i<8u;++i){
        pr_u6c_3f31_cpu cpu={(uint16_t)(0xAB00u+a),0x2345,0x3456,0x4567,0x5678,0x6789,
                            (uint16_t)(a*257u),0xFFFE,0x1404,0x061A,0x3000,0x4000,
                            (uint16_t)~a,flags_cases[i]};
        check(ram,&cpu);
    }
    assert(memcmp(ram,before,PR_U6C_3F31_RAM_SIZE)==0);
    for(v=0;v<65536u;++v){
        pr_u6c_3f31_cpu cpu={(uint16_t)((v*73u)^0xABCDu),(uint16_t)v,(uint16_t)~v,
                            0x4567,0x5678,0x6789,(uint16_t)(v*13u),(uint16_t)(v*31u),
                            0x1404,0x061A,0x3000,0x4000,(uint16_t)(v*17u),(uint16_t)v};
        check(ram,&cpu);
    }
    assert(memcmp(ram,before,PR_U6C_3F31_RAM_SIZE)==0);
    for(i=0;i<8u;++i){
        pr_u6c_3f31_cpu cpu={(uint16_t)(0xCD00u+edge_al[i]),1,2,3,4,5,
                            (uint16_t)(65535u-i),(uint16_t)(i*8191u),0x1404,0x061A,
                            0x3000,0x4000,(uint16_t)(i%2u?0xFFFF:0),flags_cases[i]};
        memset(ram,(int)(i*31u),PR_U6C_3F31_RAM_SIZE);
        memcpy(before,ram,PR_U6C_3F31_RAM_SIZE);check(ram,&cpu);
        assert(memcmp(ram,before,PR_U6C_3F31_RAM_SIZE)==0);
    }
    {
        static const int policies[]={0,2,255,-1};
        pr_u6c_3f31_cpu cpu={0},want;cpu.ip=0x061A;cpu.ax=0xAB00;cpu.flags=0xFFFF;want=cpu;
        for(i=0;i<4u;++i){
            assert(pr_u3c2_cmp_al_zero_061a_fixture(ram,PR_U6C_3F31_RAM_SIZE,&cpu,
                   (pr_u3c2_061a_af_policy)policies[i])==PR_U3C2_061A_ARGUMENT);
            assert(memcmp(&cpu,&want,sizeof(cpu))==0);
            assert(memcmp(ram,before,PR_U6C_3F31_RAM_SIZE)==0);
        }
        assert(pr_u3c2_cmp_al_zero_061a_fixture(NULL,PR_U6C_3F31_RAM_SIZE,&cpu,PR_U3C2_061A_AF_CMP_DEFINED)==PR_U3C2_061A_ARGUMENT);
        assert(pr_u3c2_cmp_al_zero_061a_fixture(ram,PR_U6C_3F31_RAM_SIZE-1u,&cpu,PR_U3C2_061A_AF_CMP_DEFINED)==PR_U3C2_061A_ARGUMENT);
        assert(pr_u3c2_cmp_al_zero_061a_fixture(ram,PR_U6C_3F31_RAM_SIZE,NULL,PR_U3C2_061A_AF_CMP_DEFINED)==PR_U3C2_061A_ARGUMENT);
        assert(memcmp(&cpu,&want,sizeof(cpu))==0);
        cpu.ip=0x061C;want=cpu;
        assert(pr_u3c2_cmp_al_zero_061a_fixture(ram,PR_U6C_3F31_RAM_SIZE,&cpu,PR_U3C2_061A_AF_CMP_DEFINED)==PR_U3C2_061A_ARGUMENT);
        assert(memcmp(&cpu,&want,sizeof(cpu))==0);
        assert(memcmp(ram,before,PR_U6C_3F31_RAM_SIZE)==0);
    }
    free(before);free(ram);return 0;
}
