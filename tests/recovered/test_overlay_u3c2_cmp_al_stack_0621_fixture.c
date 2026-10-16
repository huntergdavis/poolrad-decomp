#include "poolrad/recovered/overlay_u3c2_cmp_al_stack_0621_fixture.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static size_t address(uint16_t ss,uint16_t bp)
{
    return ((size_t)ss*16u+((unsigned)bp+65534u)%65536u)%1048576u;
}
/* Independent subtraction reference: signed range, nibble borrow, bit count. */
static uint16_t reference(unsigned a,unsigned b,uint16_t old)
{
    unsigned r=(a+256u-b)%256u,ones=0,i;
    int sa=a<128u?(int)a:(int)a-256,sb=b<128u?(int)b:(int)b-256;
    uint16_t flags=(uint16_t)(old & 0xF72Au);
    for(i=0;i<8u;++i)ones+=(r>>i)&1u;
    if(a<b)flags|=1u;
    if(ones%2u==0)flags|=4u;
    if(a%16u<b%16u)flags|=0x10u;
    if(r==0)flags|=0x40u;
    if(r>=128u)flags|=0x80u;
    if(sa-sb < -128 || sa-sb > 127)flags|=0x800u;
    return flags;
}
static void check(uint8_t *ram,pr_u6c_3f31_cpu *cpu,unsigned b)
{
    pr_u6c_3f31_cpu want=*cpu;
    want.flags=reference((unsigned)cpu->ax%256u,b,cpu->flags);want.ip=0x0624;
    assert(pr_u3c2_cmp_al_stack_0621_fixture(ram,PR_U6C_3F31_RAM_SIZE,cpu,PR_U3C2_0621_AF_CMP_DEFINED)==PR_U3C2_0621_OK);
    assert(memcmp(cpu,&want,sizeof(want))==0);
    assert(ram[address(cpu->ss,cpu->bp)]==(uint8_t)b);
}
int main(void)
{
    uint8_t *ram=malloc(PR_U6C_3F31_RAM_SIZE),*before=malloc(PR_U6C_3F31_RAM_SIZE);
    unsigned a,b,v,i;uint32_t seed=0x06212736u;size_t at;
    assert(ram&&before);
    for(i=0;i<PR_U6C_3F31_RAM_SIZE;++i){seed^=seed<<13u;seed^=seed>>17u;seed^=seed<<5u;ram[i]=(uint8_t)seed;}
    memcpy(before,ram,PR_U6C_3F31_RAM_SIZE);
    /* All operand pairs; each expected RAM update is only test setup. */
    at=address(0x2000,0x1234);
    for(a=0;a<256u;++a)for(b=0;b<256u;++b){
        pr_u6c_3f31_cpu cpu={(uint16_t)(0xAB00u+a),1,2,3,4,5,0x1234,0xFFFE,
                            0x1404,0x0621,0x3000,0x4000,0x2000,0xA55A};
        ram[at]=(uint8_t)b;check(ram,&cpu,b);
    }
    before[at]=ram[at];assert(memcmp(ram,before,PR_U6C_3F31_RAM_SIZE)==0);
    for(v=0;v<65536u;++v){
        pr_u6c_3f31_cpu cpu={0xAB80,1,2,3,4,5,0x1234,0xFFFE,
                            0x1404,0x0621,0x3000,0x4000,0x2000,(uint16_t)v};
        ram[at]=1;check(ram,&cpu,1);
    }
    before[at]=ram[at];assert(memcmp(ram,before,PR_U6C_3F31_RAM_SIZE)==0);
    /* Every BP, SS, AX and FLAGS value through coupled odd permutations. */
    for(v=0;v<65536u;++v){
        pr_u6c_3f31_cpu cpu={(uint16_t)(v^0xABCDu),1,2,3,4,5,(uint16_t)v,0xFFFE,
                            0x1404,0x0621,0x3000,0x4000,(uint16_t)(v*13u),(uint16_t)(v*17u)};
        b=ram[address(cpu.ss,cpu.bp)];check(ram,&cpu,b);
    }
    assert(memcmp(ram,before,PR_U6C_3F31_RAM_SIZE)==0);
    {
        static const struct { uint16_t ss,bp;size_t at; } cases[]={
            {0,1,65535u},{0,2,0u},{0x2000,1,196607u},
            {0xFFFF,17,1048575u},{0xFFFF,18,0u},{0xFFFF,0,65518u},
            {0x2000,3,131073u},{0x1404,0x0623,83553u}
        };
        for(i=0;i<8u;++i){
            pr_u6c_3f31_cpu cpu={(uint16_t)(0xAB00u+i*31u),1,2,3,4,5,cases[i].bp,0xFFFE,
                                0x1404,0x0621,0x3000,0x4000,cases[i].ss,0xFFFF};
            memset(ram,0xA5,PR_U6C_3F31_RAM_SIZE);ram[cases[i].at]=(uint8_t)(i*23u);
            memcpy(before,ram,PR_U6C_3F31_RAM_SIZE);check(ram,&cpu,i*23u);
            assert(memcmp(ram,before,PR_U6C_3F31_RAM_SIZE)==0);
        }
    }
    {
        static const int policies[]={0,2,255,-1};
        pr_u6c_3f31_cpu cpu={0},want;cpu.ip=0x0621;cpu.flags=0xFFFF;want=cpu;
        for(i=0;i<4u;++i){
            assert(pr_u3c2_cmp_al_stack_0621_fixture(ram,PR_U6C_3F31_RAM_SIZE,&cpu,(pr_u3c2_0621_af_policy)policies[i])==PR_U3C2_0621_ARGUMENT);
            assert(memcmp(&cpu,&want,sizeof(cpu))==0);assert(memcmp(ram,before,PR_U6C_3F31_RAM_SIZE)==0);
        }
        assert(pr_u3c2_cmp_al_stack_0621_fixture(NULL,PR_U6C_3F31_RAM_SIZE,&cpu,PR_U3C2_0621_AF_CMP_DEFINED)==PR_U3C2_0621_ARGUMENT);
        assert(pr_u3c2_cmp_al_stack_0621_fixture(ram,PR_U6C_3F31_RAM_SIZE-1u,&cpu,PR_U3C2_0621_AF_CMP_DEFINED)==PR_U3C2_0621_ARGUMENT);
        assert(pr_u3c2_cmp_al_stack_0621_fixture(ram,PR_U6C_3F31_RAM_SIZE,NULL,PR_U3C2_0621_AF_CMP_DEFINED)==PR_U3C2_0621_ARGUMENT);
        assert(memcmp(&cpu,&want,sizeof(cpu))==0);
        cpu.ip=0x0624;want=cpu;
        assert(pr_u3c2_cmp_al_stack_0621_fixture(ram,PR_U6C_3F31_RAM_SIZE,&cpu,PR_U3C2_0621_AF_CMP_DEFINED)==PR_U3C2_0621_ARGUMENT);
        assert(memcmp(&cpu,&want,sizeof(cpu))==0);assert(memcmp(ram,before,PR_U6C_3F31_RAM_SIZE)==0);
    }
    free(before);free(ram);return 0;
}
