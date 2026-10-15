#include "poolrad/recovered/overlay_u3c2_add_ax_dx_05eb_fixture.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static uint16_t expected_flags(uint16_t old,uint16_t a,uint16_t b)
{
    uint32_t sum=(uint32_t)a+b;
    unsigned result=(unsigned)(sum%65536u),bits=0,x=result%256u;
    int32_t sa=a<32768u?(int32_t)a:(int32_t)a-65536;
    int32_t sb=b<32768u?(int32_t)b:(int32_t)b-65536;
    int32_t signed_sum=sa+sb;
    uint16_t flags=(uint16_t)(old&~0x08D5u);
    while(x){bits+=x%2u;x/=2u;}
    if(sum>=65536u)flags|=1u;
    if(bits%2u==0u)flags|=4u;
    if((a%16u)+(b%16u)>=16u)flags|=16u;
    if(result==0u)flags|=64u;
    if(result>=32768u)flags|=128u;
    if(signed_sum < -32768 || signed_sum > 32767)flags|=2048u;
    return flags;
}
static void check(uint8_t *ram,uint16_t a,uint16_t b,uint16_t flags)
{
    pr_u6c_3f31_cpu cpu={a,0x2345,0x3456,b,0x5678,0x6789,
                        0xFFFF,0xFFFE,0x1404,0x05EB,0xFFFF,0x3000,0xFFFF,flags},want=cpu;
    want.ax=(uint16_t)(((uint32_t)a+b)%65536u);
    want.flags=expected_flags(flags,a,b);want.ip=0x05ED;
    assert(pr_u3c2_add_ax_dx_05eb_fixture(ram,PR_U6C_3F31_RAM_SIZE,&cpu,
           PR_U3C2_05EB_AF_ADD_DEFINED)==PR_U3C2_05EB_OK);
    assert(memcmp(&cpu,&want,sizeof(cpu))==0);
}
int main(void)
{
    static const uint16_t dx_edges[]={0,1,0x000F,0x007F,0x7FFF,0x8000,0xFFF0,0xFFFF};
    static const uint16_t cbw_ax[]={0,1,0x007F,0xFF80,0xFF81,0xFFF0,0xFFFE,0xFFFF};
    uint8_t *ram=malloc(PR_U6C_3F31_RAM_SIZE),*before=malloc(PR_U6C_3F31_RAM_SIZE);
    unsigned v,i,wide=0,cbw_dx=0,cbw_all=0,flags_count=0;
    assert(ram&&before);memset(ram,0xA5,PR_U6C_3F31_RAM_SIZE);memcpy(before,ram,PR_U6C_3F31_RAM_SIZE);
    /* Wider independent arithmetic states; no claim that CBW produced each AX. */
    for(v=0;v<65536u;++v)for(i=0;i<8u;++i) {
        check(ram,(uint16_t)v,dx_edges[i],(uint16_t)(v*17u+i*9361u));++wide;
    }
    /* Eight CBW-shaped AX values against every DX word. */
    for(v=0;v<65536u;++v)for(i=0;i<8u;++i) {
        check(ram,cbw_ax[i],(uint16_t)v,(uint16_t)(v*31u+i));++cbw_dx;
    }
    /* Every possible CBW-shaped AX, independently supplied, against DX edges. */
    for(v=0;v<256u;++v)for(i=0;i<8u;++i) {
        uint16_t a=(uint16_t)(v<128u?v:v+65280u);
        check(ram,a,dx_edges[i],(uint16_t)(v*257u+i));++cbw_all;
    }
    for(v=0;v<65536u;++v) {
        check(ram,0xFFFF,1,(uint16_t)v);++flags_count;
    }
    assert(wide==524288u && cbw_dx==524288u && cbw_all==2048u && flags_count==65536u);
    assert(memcmp(ram,before,PR_U6C_3F31_RAM_SIZE)==0);
    for(i=0;i<8u;++i) {
        memset(ram,(int)(i*31u),PR_U6C_3F31_RAM_SIZE);memcpy(before,ram,PR_U6C_3F31_RAM_SIZE);
        check(ram,dx_edges[i],dx_edges[7u-i],(uint16_t)(i*9361u));
        assert(memcmp(ram,before,PR_U6C_3F31_RAM_SIZE)==0);
    }
    {
        static const int policies[]={0,2,255,-1};
        pr_u6c_3f31_cpu cpu={0},want;cpu.ip=0x05EB;cpu.ax=0xFFFF;cpu.dx=1;cpu.flags=0xFFFF;want=cpu;
        for(i=0;i<4u;++i) {
            assert(pr_u3c2_add_ax_dx_05eb_fixture(ram,PR_U6C_3F31_RAM_SIZE,&cpu,
                   (pr_u3c2_05eb_af_policy)policies[i])==PR_U3C2_05EB_ARGUMENT);
            assert(memcmp(&cpu,&want,sizeof(cpu))==0);assert(memcmp(ram,before,PR_U6C_3F31_RAM_SIZE)==0);
        }
        assert(pr_u3c2_add_ax_dx_05eb_fixture(NULL,PR_U6C_3F31_RAM_SIZE,&cpu,PR_U3C2_05EB_AF_ADD_DEFINED)==PR_U3C2_05EB_ARGUMENT);
        assert(pr_u3c2_add_ax_dx_05eb_fixture(ram,PR_U6C_3F31_RAM_SIZE-1u,&cpu,PR_U3C2_05EB_AF_ADD_DEFINED)==PR_U3C2_05EB_ARGUMENT);
        assert(pr_u3c2_add_ax_dx_05eb_fixture(ram,PR_U6C_3F31_RAM_SIZE,NULL,PR_U3C2_05EB_AF_ADD_DEFINED)==PR_U3C2_05EB_ARGUMENT);
        assert(memcmp(&cpu,&want,sizeof(cpu))==0);
        cpu.ip=0x05ED;want=cpu;
        assert(pr_u3c2_add_ax_dx_05eb_fixture(ram,PR_U6C_3F31_RAM_SIZE,&cpu,PR_U3C2_05EB_AF_ADD_DEFINED)==PR_U3C2_05EB_ARGUMENT);
        assert(memcmp(&cpu,&want,sizeof(cpu))==0);assert(memcmp(ram,before,PR_U6C_3F31_RAM_SIZE)==0);
    }
    free(before);free(ram);return 0;
}
