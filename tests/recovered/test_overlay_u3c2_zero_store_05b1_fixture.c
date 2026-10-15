#include "poolrad/recovered/overlay_u3c2_zero_store_05b1_fixture.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static size_t address(uint16_t ss,uint16_t bp)
{
    return ((size_t)ss*16u+((unsigned)bp+65535u)%65536u)%1048576u;
}
static void run(uint8_t *ram,uint8_t *want_ram,uint16_t ss,uint16_t bp,
                uint16_t ax,unsigned initial,uint16_t flags)
{
    pr_u6c_3f31_cpu cpu={ax,0x2345,0x3456,0x4567,0x5678,0x6789,bp,0xFFFE,
                        0x1404,0x05B1,ss,ss,ss,flags},want=cpu;
    memset(ram,(flags&1u)?0xA5:0x5A,PR_U6C_3F31_RAM_SIZE);
    ram[address(ss,bp)]=(uint8_t)initial;
    memcpy(want_ram,ram,PR_U6C_3F31_RAM_SIZE);
    want_ram[address(ss,bp)]=0;want.ip=0x05B5;
    assert(pr_u3c2_zero_store_05b1_fixture(ram,PR_U6C_3F31_RAM_SIZE,&cpu,
           PR_U3C2_05B1_AF_PRESERVE_SUPPLIED)==PR_U3C2_05B1_OK);
    assert(memcmp(&cpu,&want,sizeof(cpu))==0);
    assert(memcmp(ram,want_ram,PR_U6C_3F31_RAM_SIZE)==0);
}
int main(void)
{
    static const uint16_t ss_cases[]={0,0x2000,0xFFFF};
    static const uint16_t bp_cases[]={0,1,2,3,15,16,17,18,0xFFFE,0xFFFF};
    static const uint16_t alias_bp[]={0xFFFF,0,0x5679,0x678A,0x1235,0x2346,0x3457,0x4568};
    uint8_t *ram=malloc(PR_U6C_3F31_RAM_SIZE),*want_ram=malloc(PR_U6C_3F31_RAM_SIZE);
    unsigned v,f,i,j,cases=0;
    assert(ram&&want_ram);
    for(v=0;v<256u;++v)for(f=0;f<2u;++f) {
        run(ram,want_ram,0x2000,0x8000,f?0xABCD:0x1234,v,f?0xFFFF:0);++cases;
    }
    for(i=0;i<3u;++i)for(j=0;j<10u;++j) {
        run(ram,want_ram,ss_cases[i],bp_cases[j],(uint16_t)(0xAB00u+i*10u+j),(i+j)%2u?255u:127u,0x5555);++cases;
    }
    /* Destination aliases supplied data/stack addresses, with no code fetch. */
    for(i=0;i<8u;++i) {
        run(ram,want_ram,0x2000,alias_bp[i],(uint16_t)(0x1200u+i*31u),i*31u+1u,0xAAAA);++cases;
    }
    assert(cases==550u);
    /* Full incoming FLAGS preservation, with a fixed byte destination. */
    memset(ram,0xA5,PR_U6C_3F31_RAM_SIZE);memcpy(want_ram,ram,PR_U6C_3F31_RAM_SIZE);
    want_ram[address(0,2)]=0;
    for(v=0;v<65536u;++v) {
        pr_u6c_3f31_cpu cpu={0xAB42,0x2345,0x3456,0x4567,0x5678,0x6789,2,0xFFFE,
                            0x1404,0x05B1,0,0x3000,0,(uint16_t)v},want=cpu;
        want.ip=0x05B5;
        assert(pr_u3c2_zero_store_05b1_fixture(ram,PR_U6C_3F31_RAM_SIZE,&cpu,
               PR_U3C2_05B1_AF_PRESERVE_SUPPLIED)==PR_U3C2_05B1_OK);
        assert(memcmp(&cpu,&want,sizeof(cpu))==0);assert(ram[address(0,2)]==0);
    }
    assert(memcmp(ram,want_ram,PR_U6C_3F31_RAM_SIZE)==0);
    /* Every BP, with independent arithmetic addressing and coupled AX/SS/FLAGS. */
    for(v=0;v<PR_U6C_3F31_RAM_SIZE;++v)ram[v]=(uint8_t)((v*73u+(v>>8u)*19u+(v>>16u)*7u)%256u);
    memcpy(want_ram,ram,PR_U6C_3F31_RAM_SIZE);
    for(v=0;v<65536u;++v) {
        pr_u6c_3f31_cpu cpu={(uint16_t)(v*31u),0x2345,0x3456,0x4567,0x5678,0x6789,
                            (uint16_t)v,0xFFFE,0x1404,0x05B1,0x1000,0x3000,
                            (uint16_t)(v*17u),(uint16_t)(v*257u)},want=cpu;
        size_t at=address(cpu.ss,cpu.bp);want_ram[at]=0;want.ip=0x05B5;
        assert(pr_u3c2_zero_store_05b1_fixture(ram,PR_U6C_3F31_RAM_SIZE,&cpu,
               PR_U3C2_05B1_AF_PRESERVE_SUPPLIED)==PR_U3C2_05B1_OK);
        assert(memcmp(&cpu,&want,sizeof(cpu))==0);assert(ram[at]==want_ram[at]);
    }
    assert(memcmp(ram,want_ram,PR_U6C_3F31_RAM_SIZE)==0);
    {
        static const int policies[]={0,2,255,-1};
        pr_u6c_3f31_cpu cpu={0},want;cpu.ip=0x05B1;cpu.ax=0xAB42;cpu.flags=0xFFFF;want=cpu;
        for(i=0;i<4u;++i) {
            assert(pr_u3c2_zero_store_05b1_fixture(ram,PR_U6C_3F31_RAM_SIZE,&cpu,
                   (pr_u3c2_05b1_af_policy)policies[i])==PR_U3C2_05B1_ARGUMENT);
            assert(memcmp(&cpu,&want,sizeof(cpu))==0);assert(memcmp(ram,want_ram,PR_U6C_3F31_RAM_SIZE)==0);
        }
        assert(pr_u3c2_zero_store_05b1_fixture(NULL,PR_U6C_3F31_RAM_SIZE,&cpu,PR_U3C2_05B1_AF_PRESERVE_SUPPLIED)==PR_U3C2_05B1_ARGUMENT);
        assert(pr_u3c2_zero_store_05b1_fixture(ram,PR_U6C_3F31_RAM_SIZE-1u,&cpu,PR_U3C2_05B1_AF_PRESERVE_SUPPLIED)==PR_U3C2_05B1_ARGUMENT);
        assert(pr_u3c2_zero_store_05b1_fixture(ram,PR_U6C_3F31_RAM_SIZE,NULL,PR_U3C2_05B1_AF_PRESERVE_SUPPLIED)==PR_U3C2_05B1_ARGUMENT);
        assert(memcmp(&cpu,&want,sizeof(cpu))==0);
        cpu.ip=0x05B5;want=cpu;
        assert(pr_u3c2_zero_store_05b1_fixture(ram,PR_U6C_3F31_RAM_SIZE,&cpu,PR_U3C2_05B1_AF_PRESERVE_SUPPLIED)==PR_U3C2_05B1_ARGUMENT);
        assert(memcmp(&cpu,&want,sizeof(cpu))==0);assert(memcmp(ram,want_ram,PR_U6C_3F31_RAM_SIZE)==0);
    }
    free(want_ram);free(ram);return 0;
}
