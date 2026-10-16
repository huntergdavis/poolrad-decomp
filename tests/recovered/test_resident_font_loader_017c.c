#include "poolrad/recovered/resident_font_loader_017c.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

_Static_assert(sizeof(pr_font017c_cpu) == 28u, "14 CPU words");
typedef struct {
    uint8_t *expected;
    uint16_t entry_sp, ss, cs, bias, size;
    unsigned count, fail_at, bad_return_at;
} fixture;

static size_t addr(uint16_t seg, unsigned off)
{
    return ((size_t)seg * 16u + off % 65536u) % PR_FONT017C_RAM_SIZE;
}
static uint16_t get(const uint8_t *ram, uint16_t seg, unsigned off)
{
    size_t p = addr(seg, off);
    return (uint16_t)(ram[p] + 256u * ram[(p + 1u) % PR_FONT017C_RAM_SIZE]);
}
static void put(uint8_t *ram, uint16_t seg, unsigned off, uint16_t value)
{
    size_t p = addr(seg, off);
    ram[p] = (uint8_t)value;
    ram[(p + 1u) % PR_FONT017C_RAM_SIZE] = (uint8_t)(value >> 8);
}
static void both(uint8_t *ram, fixture *f, unsigned off, uint16_t value)
{
    put(ram, f->ss, off, value);
    put(f->expected, f->ss, off, value);
}
static unsigned relative(const fixture *f, unsigned minus)
{
    return (unsigned)f->entry_sp + 65536u - minus;
}
static int callee(void *context, const pr_font017c_call *call,
                  uint8_t *ram, size_t bytes, pr_font017c_cpu *cpu)
{
    static const uint16_t sites[] = {0x18Cu,0x19Bu,0x1ADu,0x1C8u,0x1D6u};
    static const uint16_t targets[] = {0xB0Bu,0xB0Bu,0x716u,0x1685u,0x254u};
    static const unsigned stack_delta[] = {32u,36u,42u,34u,30u};
    static const unsigned cleanup[] = {4u,4u,18u,10u,6u};
    fixture *f = context;
    unsigned n = ++f->count;
    uint16_t segment = (uint16_t)(f->bias + (n == 3u ? 0x802u : 0xAF8u));
    assert(bytes == PR_FONT017C_RAM_SIZE);
    assert(n <= 5u && call->ordinal == n);
    assert(call->call_ip == sites[n-1u] && call->target_offset == targets[n-1u]);
    assert(call->target_segment == segment);
    assert(call->return_cs == f->cs && call->return_ip == sites[n-1u]+5u);
    assert(cpu->cs == segment && cpu->ip == targets[n-1u]);
    assert(cpu->sp == (uint16_t)relative(f,stack_delta[n-1u]));
    assert(cpu->bp == (uint16_t)relative(f,2u) && cpu->ss == f->ss);
    assert(get(ram,cpu->ss,cpu->sp) == call->return_ip);
    assert(get(ram,cpu->ss,(unsigned)cpu->sp+2u) == f->cs);
    if (n == 1u) {
        uint16_t flags;
        switch (f->entry_sp) {
        case 0u: case 0xFFFEu: flags=0x0282u; break;
        case 1u: case 0xFFFFu: flags=0x0286u; break;
        case 6u: flags=0x0283u; break;
        case 7u: flags=0x0287u; break;
        default: flags=0x0202u; break;
        }
        assert(cpu->flags==flags);
    }
    if (n <= 2u) {
        unsigned destination = relative(f,n == 1u ? 14u : 19u);
        assert(get(ram,f->ss,(unsigned)cpu->sp+4u) == (n == 1u ? 0x171u : 0x177u));
        assert(get(ram,f->ss,(unsigned)cpu->sp+6u) == f->cs);
        assert(get(ram,f->ss,(unsigned)cpu->sp+8u) == (uint16_t)destination);
        assert(get(ram,f->ss,(unsigned)cpu->sp+10u) == f->ss);
        /* Synthetic strings only; no original game literals. */
        for (unsigned i=0u; i<(n == 1u ? 6u : 5u); ++i) {
            size_t p=addr(f->ss,destination+i);
            ram[p]=f->expected[p]=(uint8_t)(0x30u+n*9u+i);
        }
    } else if (n == 3u) {
        const uint16_t args[9]={(uint16_t)relative(f,8u),f->ss,
            (uint16_t)relative(f,4u),f->ss,0x12C9u,
            (uint16_t)relative(f,19u),f->ss,
            (uint16_t)relative(f,14u),f->ss};
        for (unsigned i=0u;i<9u;++i)
            assert(get(ram,f->ss,(unsigned)cpu->sp+4u+2u*i)==args[i]);
        both(ram,f,relative(f,4u),f->size);
        /* DAX's far-pointer output is four contiguous physical bytes. */
        size_t p=addr(f->ss,relative(f,8u));
        const uint8_t data[4]={0x56u,0x34u,0x00u,0x25u};
        for (unsigned i=0u;i<4u;++i)
            ram[(p+i)%PR_FONT017C_RAM_SIZE]=f->expected[(p+i)%PR_FONT017C_RAM_SIZE]=data[i];
    } else if (n == 4u) {
        const uint16_t args[5]={f->size,0x789Au,0x4500u,0x3456u,0x2500u};
        for (unsigned i=0u;i<5u;++i)
            assert(get(ram,f->ss,(unsigned)cpu->sp+4u+2u*i)==args[i]);
        assert(cpu->flags==(f->size==1u || f->size==2u ? 0x0202u :
                            f->size>=0x8000u ? 0x0286u : 0x0206u));
        /* The next call must reread all three locals after Move returns. */
        both(ram,f,relative(f,4u),5u);
        both(ram,f,relative(f,8u),0x1111u);
        both(ram,f,relative(f,6u),0x2222u);
        size_t p=addr(0x4500u,0x789Au);
        ram[p]=f->expected[p]=0xA6u;
    } else {
        const uint16_t args[3]={5u,0x1111u,0x2222u};
        for (unsigned i=0u;i<3u;++i)
            assert(get(ram,f->ss,(unsigned)cpu->sp+4u+2u*i)==args[i]);
    }
    cpu->ax = n == 2u ? 0x12ABu : (uint16_t)(0x1100u+n);
    cpu->bx=(uint16_t)(0x2200u+n); cpu->cx=(uint16_t)(0x3300u+n);
    cpu->dx=(uint16_t)(0x4400u+n); cpu->si=(uint16_t)(0x5500u+n);
    cpu->di=(uint16_t)(0x6600u+n); cpu->es=(uint16_t)(0x7700u+n);
    cpu->flags=n==3u ? 0x0AD7u : 0x5AA5u;
    if (f->fail_at==n) return 1;
    cpu->ip=get(ram,cpu->ss,cpu->sp);
    cpu->sp=(uint16_t)(cpu->sp+2u);
    cpu->cs=get(ram,cpu->ss,cpu->sp);
    cpu->sp=(uint16_t)(cpu->sp+2u+cleanup[n-1u]);
    if (f->bad_return_at==n) cpu->ip ^= 1u;
    return 0;
}

static void run_case(uint16_t ss,uint16_t sp,uint16_t bias,uint16_t loaded_size,
                     unsigned fail,unsigned bad_return)
{
    int nonempty=loaded_size!=0u;
    uint8_t *ram=malloc(PR_FONT017C_RAM_SIZE);
    uint8_t *expected=malloc(PR_FONT017C_RAM_SIZE);
    pr_font017c_cpu cpu={
        0xA55Au,0x1357u,0x2468u,0x3579u,0x468Au,0x579Bu,0xBEEFu,sp,
        (uint16_t)(0x709u+bias),0x17Cu,0x6000u,0x8000u,ss,0x0AD7u
    };
    pr_font017c_cpu want=cpu;
    fixture f={expected,sp,ss,cpu.cs,bias,loaded_size,0u,fail,bad_return};
    uint32_t seed=0x017C2026u;
    assert(ram && expected);
    for (size_t i=0u;i<PR_FONT017C_RAM_SIZE;++i) {
        seed=seed*1664525u+1013904223u;ram[i]=(uint8_t)(seed>>24);
    }
    put(ram,ss,sp,0x1234u);put(ram,ss,(unsigned)sp+2u,0xABCDu);
    put(ram,cpu.ds,0x5E20u,0x789Au);put(ram,cpu.ds,0x5E22u,0x4500u);
    memcpy(expected,ram,PR_FONT017C_RAM_SIZE);
    pr_font017c_status status=pr_resident_font_loader_017c(
        ram,PR_FONT017C_RAM_SIZE,&cpu,bias,callee,&f);
    if (fail || bad_return) {
        assert(status==(fail?PR_FONT017C_CALLEE:PR_FONT017C_RETURN));
        assert(f.count==(fail?fail:bad_return));
        assert(get(ram,ss,relative(&f,2u))==0xBEEFu);
    } else {
        static const unsigned deltas[]={2u,22u,24u,26u,28u,30u,32u,34u,36u,38u,40u,42u};
        uint16_t zero_words[]={0xBEEFu,ss,(uint16_t)relative(&f,14u),ss,
            (uint16_t)relative(&f,19u),0x12C9u,ss,(uint16_t)relative(&f,4u),
            ss,(uint16_t)relative(&f,8u),f.cs,0x1B2u};
        uint16_t filled_words[]={0xBEEFu,0x2222u,0x1111u,5u,f.cs,0x1DBu,
            f.cs,0x1CDu,ss,(uint16_t)relative(&f,8u),f.cs,0x1B2u};
        uint16_t *words=nonempty?filled_words:zero_words;
        unsigned n=nonempty?5u:3u;
        assert(status==PR_FONT017C_OK && f.count==n);
        for (size_t i=0u;i<sizeof(deltas)/sizeof(deltas[0]);++i)
            put(expected,ss,relative(&f,deltas[i]),words[i]);
        want.ax=(uint16_t)(0x1100u+n);want.bx=(uint16_t)(0x2200u+n);
        want.cx=(uint16_t)(0x3300u+n);want.dx=(uint16_t)(0x4400u+n);
        want.si=(uint16_t)(0x5500u+n);want.di=(uint16_t)(0x6600u+n);
        want.es=(uint16_t)(0x7700u+n);want.flags=nonempty?0x5AA5u:0x0246u;
        want.sp=(uint16_t)(sp+4u);want.ip=0x1234u;want.cs=0xABCDu;
        assert(memcmp(&cpu,&want,sizeof(cpu))==0);
        assert(memcmp(ram,expected,PR_FONT017C_RAM_SIZE)==0);
    }
    free(expected);free(ram);
}
static void guards(void)
{
    uint8_t *ram=calloc(PR_FONT017C_RAM_SIZE,1u);
    uint8_t *expected=calloc(PR_FONT017C_RAM_SIZE,1u);
    pr_font017c_cpu cpu={0},before;
    assert(ram && expected);cpu.ip=0x17Cu;before=cpu;
    assert(pr_resident_font_loader_017c(NULL,PR_FONT017C_RAM_SIZE,&cpu,0u,callee,NULL)==PR_FONT017C_ARGUMENT);
    assert(memcmp(&cpu,&before,sizeof(cpu))==0);
    assert(pr_resident_font_loader_017c(ram,PR_FONT017C_RAM_SIZE,NULL,0u,callee,NULL)==PR_FONT017C_ARGUMENT);
    assert(memcmp(&cpu,&before,sizeof(cpu))==0);
    assert(pr_resident_font_loader_017c(ram,PR_FONT017C_RAM_SIZE,&cpu,0u,NULL,NULL)==PR_FONT017C_ARGUMENT);
    assert(pr_resident_font_loader_017c(ram,PR_FONT017C_RAM_SIZE-1u,&cpu,0u,callee,NULL)==PR_FONT017C_ARGUMENT);
    assert(memcmp(&cpu,&before,sizeof(cpu))==0);
    assert(memcmp(&cpu,&before,sizeof(cpu))==0);
    cpu.ip=0u;before=cpu;
    assert(pr_resident_font_loader_017c(ram,PR_FONT017C_RAM_SIZE,&cpu,0u,callee,NULL)==PR_FONT017C_ARGUMENT);
    assert(memcmp(&cpu,&before,sizeof(cpu))==0);
    assert(memcmp(&cpu,&before,sizeof(cpu))==0);
    assert(memcmp(ram,expected,PR_FONT017C_RAM_SIZE)==0);
    free(expected);free(ram);
}
int main(void)
{
    const uint16_t stacks[]={0u,1u,6u,7u,0x8000u,0xFFFEu,0xFFFFu};
    const uint16_t segments[]={0u,0x7000u,0xFFFFu};
    const uint16_t biases[]={0u,0x824u,0xFFF0u};
    unsigned cases=0u;
    guards();
    for (size_t s=0u;s<3u;++s)
        for (size_t p=0u;p<7u;++p)
            for (size_t b=0u;b<3u;++b)
                for (int nonempty=0;nonempty<2;++nonempty) {
                    run_case(segments[s],stacks[p],biases[b],(uint16_t)(nonempty?3u:0u),0u,0u);
                    ++cases;
                }
    const uint16_t extra_sizes[]={1u,2u,0x7FFFu,0x8000u,0xFFFFu};
    for (size_t i=0u;i<5u;++i) {
        run_case(0x7000u,0x8000u,0x824u,extra_sizes[i],0u,0u);
        ++cases;
    }
    for (unsigned n=1u;n<=5u;++n) {
        run_case(0x7000u,0x8000u,0x824u,3u,n,0u);
        run_case(0x7000u,0x8000u,0x824u,3u,0u,n);
    }
    printf("RES0709:017C synthetic PASS: %u full-RAM/all14 cases; 10 callback stops; 5 guards\n",cases);
    return 0;
}
