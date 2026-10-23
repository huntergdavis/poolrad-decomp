#include "loaded_stub_fixture.h"
/* U7F +1010: the SYSTEM segment of each FillChar LCALL is the loader-relocated word in the caller's code, not a constant 0AF8h.
 * FillChar is wrapped at link time (-Wl,--wrap) so the test records the CS it is entered with and the pushed return IP, which names
 * the call site; each site gets its own word, so a model that read the wrong word, or a constant, fails. All other callees are the
 * real models. */
#include "poolrad/recovered/overlay_u7f_caller_1010.h"
#include "poolrad/recovered/system_runtime_leaves_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
#define CHECK(x) do { ++checks;if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1);} }while(0)
static size_t physical(unsigned s,unsigned o){return ((size_t)s*16u+(o&65535u))&1048575u;}
static void put(uint8_t *m,unsigned s,unsigned o,unsigned v)
{size_t a=physical(s,o);m[a]=(uint8_t)v;m[(a+1u)&1048575u]=(uint8_t)(v>>8u);}
static unsigned get(const uint8_t *m,unsigned s,unsigned o)
{size_t a=physical(s,o);return m[a]+256u*m[(a+1u)&1048575u];}
pr_system_leaf_status __real_pr_system_fillchar_16a8(uint8_t *ram,size_t size,pr_system_leaf_cpu *cpu);
static unsigned seen_return_ip[8],seen_cs[8],seen;
pr_system_leaf_status __wrap_pr_system_fillchar_16a8(uint8_t *ram,size_t size,pr_system_leaf_cpu *cpu)
{
    if(seen<8u){seen_return_ip[seen]=get(ram,cpu->ss,cpu->sp);seen_cs[seen]=cpu->cs;++seen;}
    return __real_pr_system_fillchar_16a8(ram,size,cpu);
}
static pr_u7f_1010_cpu setup(uint8_t *m,unsigned type,unsigned w1,unsigned w2,unsigned w3)
{
    pr_u7f_1010_cpu c={0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,
        0x7777,0xF000,0x3000,0x1010,0x5000,0x1111,0x2000,0x602};
    unsigned i;memset(m,0xA5,PR_U7F_1010_RAM_SIZE);
    test_loaded_stub(m, c.cs, 0x10C7u, (uint16_t)(0x4000u + 0x100u), 0x0048u, 0x4000u, 0x0DE3u);
    test_loaded_stub(m, c.cs, 0x1596u, (uint16_t)(0x4000u + 0x100u), 0x0048u, 0x4000u, 0x0DE3u);
    test_loaded_stub(m, c.cs, 0x1611u, (uint16_t)(0x4000u + 0x100u), 0x0048u, 0x4000u, 0x0DE3u);
    test_loaded_stub(m, c.cs, 0x1630u, (uint16_t)(0x4000u + 0x100u), 0x0048u, 0x4000u, 0x0DE3u);
    test_loaded_stub(m, c.cs, 0x1649u, (uint16_t)(0x4000u + 0x100u), 0x0048u, 0x4000u, 0x0DE3u);
    test_loaded_stub(m, c.cs, 0x1662u, (uint16_t)(0x4000u + 0x100u), 0x0048u, 0x4000u, 0x0DE3u);
    test_loaded_stub(m, c.cs, 0x167Eu, (uint16_t)(0x4000u + 0x100u), 0x0048u, 0x4000u, 0x0DE3u);
    test_loaded_stub(m, c.cs, 0x1692u, (uint16_t)(0x4000u + 0x100u), 0x0048u, 0x4000u, 0x0DE3u);
    test_loaded_stub(m, c.cs, 0x16ABu, (uint16_t)(0x4000u + 0x100u), 0x0048u, 0x4000u, 0x0DE3u);
    test_loaded_stub(m, c.cs, 0x170Fu, (uint16_t)(0x4000u + 0x100u), 0x0048u, 0x4000u, 0x0DE3u);
    test_loaded_stub(m, c.cs, 0x174Bu, (uint16_t)(0x4000u + 0x100u), 0x0048u, 0x4000u, 0x0DE3u);
    test_loaded_stub(m, c.cs, 0x17A1u, (uint16_t)(0x4000u + 0x100u), 0x0048u, 0x4000u, 0x0DE3u);
    test_loaded_stub(m, c.cs, 0x17E6u, (uint16_t)(0x4000u + 0x100u), 0x0048u, 0x4000u, 0x0DE3u);
    put(m,c.ss,c.sp,0x9000);put(m,c.ss,c.sp+2u,0xA000);
    put(m,c.ss,c.sp+4u,0x8000);put(m,c.ss,c.sp+6u,0x5000);put(m,c.ss,c.sp+8u,type);
    for(i=0;i<512u;++i)put(m,c.ds,0x970u+2u*i,i*317u+19u);
    put(m,c.ds,0x4428,0);put(m,c.ds,0x442A,0);
    put(m,0xAF8,0x1245,0x8405);
    put(m,c.cs,0x1025,w1);put(m,c.cs,0x1047,w2);put(m,c.cs,0x108B,w3);
    m[physical(c.cs,0xFDE)]=0x9A;put(m,c.cs,0xFDF,0x48);put(m,c.cs,0xFE1,0x4100);
    m[physical(0x4100,0x48)]=0xEA;put(m,0x4100,0x49,0xDE3);put(m,0x4100,0x4B,0x4000);
    return c;
}
int main(void)
{
    static const unsigned sets[][3]={{0x0AF8,0x0AF8,0x0AF8},{0x1234,0x2345,0x3456},{0x0001,0xFFF0,0x10F8}};
    uint8_t *m=malloc(PR_U7F_1010_RAM_SIZE);
    unsigned covered=0,s,t,k;
    CHECK(m!=NULL);
    for(s=0;s<3u;++s)for(t=0;t<256u;++t){
        pr_u7f_1010_cpu c=setup(m,t,sets[s][0],sets[s][1],sets[s][2]);
        seen=0;
        (void)pr_u7f_caller_1010(m,PR_U7F_1010_RAM_SIZE,&c,0x4000,1024,256);
        for(k=0;k<seen;++k){
            unsigned site=seen_return_ip[k]==0x1027u?0u:seen_return_ip[k]==0x1049u?1u:seen_return_ip[k]==0x108Du?2u:3u;
            CHECK(site<3u);
            CHECK(seen_cs[k]==sets[s][site]);            /* entered at that site's own relocated word */
            covered|=1u<<site;
        }
    }
    CHECK(covered==7u);                                  /* all three FillChar LCALLs were reached */
    free(m);printf("%u live-segment checks passed\n",checks);return 0;
}
