#include "poolrad/recovered/overlay_u3c2_entry3_02cc.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

_Static_assert(sizeof(pr_u3c2_02cc_cpu)==28u,"14 CPU words");

static size_t address(uint16_t seg,unsigned off)
{
    return ((size_t)seg*16u+off%65536u)%PR_U3C2_02CC_RAM_SIZE;
}
static uint8_t byte(const uint8_t *ram,uint16_t seg,unsigned off)
{
    return ram[address(seg,off)];
}
static uint16_t word(const uint8_t *ram,uint16_t seg,unsigned off)
{
    size_t p=address(seg,off);
    return (uint16_t)(ram[p]+256u*ram[(p+1u)%PR_U3C2_02CC_RAM_SIZE]);
}
static void set_byte(uint8_t *ram,uint16_t seg,unsigned off,uint8_t value)
{
    ram[address(seg,off)]=value;
}
static void set_word(uint8_t *ram,uint16_t seg,unsigned off,uint16_t value)
{
    size_t p=address(seg,off);
    ram[p]=(uint8_t)value;
    ram[(p+1u)%PR_U3C2_02CC_RAM_SIZE]=(uint8_t)(value/256u);
}
static void far_pointer(uint8_t *ram,uint16_t seg,unsigned off,
                        uint16_t ptr_off,uint16_t ptr_seg)
{
    size_t p=address(seg,off);
    const uint8_t values[4]={(uint8_t)ptr_off,(uint8_t)(ptr_off/256u),
                            (uint8_t)ptr_seg,(uint8_t)(ptr_seg/256u)};
    for (unsigned i=0u;i<4u;++i)
        ram[(p+i)%PR_U3C2_02CC_RAM_SIZE]=values[i];
}
static int signed8(uint8_t value)
{
    return value<128u?(int)value:(int)value-256;
}
static int signed16(uint16_t value)
{
    return value<32768u?(int)value:(int)value-65536;
}
static uint16_t final_flags(uint16_t incoming,uint16_t left,uint16_t right)
{
    unsigned wide=(unsigned)left+right,ones=0u;
    uint16_t result=(uint16_t)wide;
    int signed_sum=signed16(left)+signed16(right);
    for (unsigned bit=0u;bit<8u;++bit) ones+=(result/(1u<<bit))%2u;
    return (uint16_t)((incoming&0xF72Au) |
        (wide>=65536u?1u:0u) | (ones%2u==0u?4u:0u) |
        ((left%16u+right%16u)>=16u?16u:0u) | (result==0u?64u:0u) |
        (result>=32768u?128u:0u) |
        (signed_sum < -32768 || signed_sum > 32767?2048u:0u));
}

/* Record and stack are disjoint in this mathematical specification.
 * The record may overlap its DS lookup table. Expected memory is updated
 * before the lookup, so that table alias is visible without a cached table. */
static void specification(uint8_t *ram,pr_u3c2_02cc_cpu *cpu,
                          uint16_t rec_seg,uint16_t rec)
{
    uint16_t original_sp=cpu->sp;
    unsigned bp=(unsigned)original_sp+65534u;
    int y_major=signed16(word(ram,rec_seg,rec+0xAu))>
                signed16(word(ram,rec_seg,rec+0xCu));
    int moved=word(ram,rec_seg,rec+(y_major?0x14u:0x16u))!=
              word(ram,rec_seg,rec+(y_major?4u:6u));
    int a=0,b=0;
    set_word(ram,cpu->ss,bp,cpu->bp);
    set_word(ram,cpu->ss,bp+65529u,rec);
    set_word(ram,cpu->ss,bp+65531u,rec_seg);
    if (moved) {
        int both=signed16(word(ram,rec_seg,rec+8u))>=0;
        a=(y_major||both)?signed8(byte(ram,rec_seg,rec+0x12u)):0;
        b=(!y_major||both)?signed8(byte(ram,rec_seg,rec+0x13u)):0;
        set_word(ram,rec_seg,rec+0x14u,
                 (uint16_t)(word(ram,rec_seg,rec+0x14u)+a));
        set_word(ram,rec_seg,rec+0x16u,
                 (uint16_t)(word(ram,rec_seg,rec+0x16u)+b));
        set_word(ram,rec_seg,rec+8u,
                 (uint16_t)(word(ram,rec_seg,rec+8u)+
                            word(ram,rec_seg,rec+(both?0xEu:0x10u))));
        set_byte(ram,rec_seg,rec+0x18u,
                 (uint8_t)(byte(ram,rec_seg,rec+0x18u)+(both?3u:2u)));
    }
    set_byte(ram,cpu->ss,bp+65534u,(uint8_t)a);
    set_byte(ram,cpu->ss,bp+65533u,(uint8_t)b);
    set_byte(ram,cpu->ss,bp+65535u,(uint8_t)moved);
    uint16_t x=(uint16_t)(a+1),y=(uint16_t)(b+1);
    unsigned lookup=(unsigned)((int)0x25C0+3*(b+1)+(a+1));
    set_byte(ram,rec_seg,rec+0x19u,byte(ram,cpu->ds,lookup));
    cpu->ax=(uint16_t)((y&0xFF00u)|(uint16_t)moved);
    cpu->dx=x;cpu->si=y;cpu->di=rec;cpu->es=rec_seg;
    cpu->flags=final_flags(cpu->flags,(uint16_t)(3u*y),x);
    cpu->sp=(uint16_t)(original_sp+8u);
    cpu->ip=word(ram,cpu->ss,original_sp);
    cpu->cs=word(ram,cpu->ss,(unsigned)original_sp+2u);
}

static pr_u3c2_02cc_cpu initial(uint16_t ss,uint16_t sp,uint16_t flags,uint16_t ds)
{
    pr_u3c2_02cc_cpu cpu={0xABCDu,0x1357u,0x2468u,0x3579u,0x468Au,
        0x579Bu,0x68ACu,sp,0xF000u,0x02CCu,ds,0x8000u,ss,flags};
    return cpu;
}
static void setup(uint8_t *ram,const pr_u3c2_02cc_cpu *cpu,
                  uint16_t rec_seg,uint16_t rec,unsigned path,uint8_t a,uint8_t b)
{
    int y_major=path<3u,stopped=path%3u==0u,negative=path%3u==2u;
    set_word(ram,cpu->ss,cpu->sp,0x1234u);
    set_word(ram,cpu->ss,(unsigned)cpu->sp+2u,0xD000u);
    far_pointer(ram,cpu->ss,(unsigned)cpu->sp+4u,rec,rec_seg);
    /* At SP 7/8, the stored local pointer's segment word is not the
     * physical segment half subsequently read by LES. Seed its shadow. */
    if (cpu->sp==7u || cpu->sp==8u) {
        size_t p=address(cpu->ss,(unsigned)cpu->sp+65527u);
        ram[(p+2u)%PR_U3C2_02CC_RAM_SIZE]=(uint8_t)rec_seg;
        ram[(p+3u)%PR_U3C2_02CC_RAM_SIZE]=(uint8_t)(rec_seg/256u);
    }
    for (unsigned i=0u;i<26u;++i) set_byte(ram,rec_seg,rec+i,(uint8_t)(0x91u+i*7u));
    set_word(ram,rec_seg,rec+0xAu,y_major?5u:2u);
    set_word(ram,rec_seg,rec+0xCu,3u);
    set_word(ram,rec_seg,rec+4u,0x8001u);
    set_word(ram,rec_seg,rec+6u,0x7FFFu);
    set_word(ram,rec_seg,rec+0x14u,y_major&&stopped?0x8001u:0xFFFFu);
    set_word(ram,rec_seg,rec+0x16u,!y_major&&stopped?0x7FFFu:0x8000u);
    set_word(ram,rec_seg,rec+8u,negative?0xFFFFu:0u);
    set_word(ram,rec_seg,rec+0xEu,0x7FFFu);
    set_word(ram,rec_seg,rec+0x10u,0x8000u);
    set_byte(ram,rec_seg,rec+0x12u,a);set_byte(ram,rec_seg,rec+0x13u,b);
    set_byte(ram,rec_seg,rec+0x18u,0xFFu);
}

static void compare(uint8_t *ram,uint8_t *expected,uint16_t ss,uint16_t sp,
                    uint16_t rec,unsigned path,uint8_t a,uint8_t b,
                    uint16_t flags,int table_alias,int full_ram)
{
    uint16_t ds=table_alias?0x5000u:0x6000u;
    pr_u3c2_02cc_cpu cpu=initial(ss,sp,flags,ds),want=cpu;
    setup(ram,&cpu,0x5000u,rec,path,a,b);
    if (full_ram) memcpy(expected,ram,PR_U3C2_02CC_RAM_SIZE);
    else setup(expected,&want,0x5000u,rec,path,a,b);
    specification(expected,&want,0x5000u,rec);
    assert(pr_u3c2_entry3_02cc(ram,PR_U3C2_02CC_RAM_SIZE,&cpu)==PR_U3C2_02CC_OK);
    assert(memcmp(&cpu,&want,sizeof(cpu))==0);
    for (unsigned i=0u;i<26u;++i)
        assert(byte(ram,0x5000u,rec+i)==byte(expected,0x5000u,rec+i));
    if (full_ram) assert(memcmp(ram,expected,PR_U3C2_02CC_RAM_SIZE)==0);
}
/* Literal alias cases are independent of the disjoint-record mathematics. */
static void alias_case(uint8_t *ram,uint8_t *expected,int redirects)
{
    pr_u3c2_02cc_cpu cpu=initial(0x7000u,0x8000u,0x0AD7u,0x6000u),want=cpu;
    uint16_t rec=redirects?0x7FE0u:0x7FE4u;
    setup(ram,&cpu,0x7000u,rec,redirects?2u:0u,2u,3u);
    if (redirects) {
        set_byte(ram,0x7000u,0x81F2u,0xFEu);
        set_word(ram,0x7000u,0x81F4u,1u);
        set_byte(ram,0x6000u,0x25C6u,0xA5u);
    } else {
        set_word(ram,0x7000u,rec+4u,0x007Fu);
        set_byte(ram,0x6000u,0x25C4u,0xA5u);
    }
    memcpy(expected,ram,PR_U3C2_02CC_RAM_SIZE);
    set_word(expected,0x7000u,0x7FFEu,0x68ACu);
    set_word(expected,0x7000u,0x7FF7u,rec);
    set_word(expected,0x7000u,0x7FF9u,0x7000u);
    set_byte(expected,0x7000u,0x7FFCu,redirects?2u:0u);
    set_byte(expected,0x7000u,0x7FFBu,0u);
    set_byte(expected,0x7000u,0x7FFDu,redirects?1u:0xA5u);
    if (redirects) {
        set_word(expected,0x7000u,0x7FE8u,0x7FFFu);
        set_byte(expected,0x7000u,0x7FF8u,0x81u);
        set_word(expected,0x7000u,0x81F4u,0xFFFFu);
        set_byte(expected,0x7000u,0x81F9u,0xA5u);
    }
    want.ax=redirects?1u:0xA5u;want.dx=redirects?3u:1u;
    want.si=1u;want.di=redirects?0x81E0u:rec;want.es=0x7000u;
    want.flags=redirects?0x0206u:0x0202u;
    want.sp=0x8008u;want.ip=0x1234u;want.cs=0xD000u;
    assert(pr_u3c2_entry3_02cc(ram,PR_U3C2_02CC_RAM_SIZE,&cpu)==PR_U3C2_02CC_OK);
    assert(memcmp(&cpu,&want,sizeof(cpu))==0);
    assert(memcmp(ram,expected,PR_U3C2_02CC_RAM_SIZE)==0);
}
static void signed_comparison_case(uint8_t *ram,uint8_t *expected,
                                   uint16_t left,uint16_t right)
{
    pr_u3c2_02cc_cpu cpu=initial(0x7000u,0x8000u,0x0AD7u,0x6000u),want=cpu;
    setup(ram,&cpu,0x5000u,0x2200u,2u,0x80u,0x7Fu);
    set_word(ram,0x5000u,0x220Au,left);
    set_word(ram,0x5000u,0x220Cu,right);
    memcpy(expected,ram,PR_U3C2_02CC_RAM_SIZE);
    specification(expected,&want,0x5000u,0x2200u);
    assert(pr_u3c2_entry3_02cc(ram,PR_U3C2_02CC_RAM_SIZE,&cpu)==PR_U3C2_02CC_OK);
    assert(memcmp(&cpu,&want,sizeof(cpu))==0);
    assert(memcmp(ram,expected,PR_U3C2_02CC_RAM_SIZE)==0);
}
static void guards(uint8_t *ram,uint8_t *expected)
{
    pr_u3c2_02cc_cpu cpu=initial(0x7000u,0x8000u,0xFFFFu,0x6000u),before=cpu;
    memcpy(expected,ram,PR_U3C2_02CC_RAM_SIZE);
    assert(pr_u3c2_entry3_02cc(NULL,PR_U3C2_02CC_RAM_SIZE,&cpu)==PR_U3C2_02CC_ARGUMENT);
    assert(memcmp(&cpu,&before,sizeof(cpu))==0);
    assert(pr_u3c2_entry3_02cc(ram,PR_U3C2_02CC_RAM_SIZE,NULL)==PR_U3C2_02CC_ARGUMENT);
    assert(pr_u3c2_entry3_02cc(ram,0u,&cpu)==PR_U3C2_02CC_ARGUMENT);
    assert(memcmp(&cpu,&before,sizeof(cpu))==0);
    assert(pr_u3c2_entry3_02cc(ram,PR_U3C2_02CC_RAM_SIZE-1u,&cpu)==PR_U3C2_02CC_ARGUMENT);
    assert(memcmp(&cpu,&before,sizeof(cpu))==0);
    cpu.ip=0u;before=cpu;
    assert(pr_u3c2_entry3_02cc(ram,PR_U3C2_02CC_RAM_SIZE,&cpu)==PR_U3C2_02CC_ARGUMENT);
    assert(memcmp(&cpu,&before,sizeof(cpu))==0);
    assert(memcmp(ram,expected,PR_U3C2_02CC_RAM_SIZE)==0);
}
int main(void)
{
    const uint16_t stacks[]={0u,1u,7u,8u,0x8000u,0xFFFAu,0xFFFBu,0xFFFEu,0xFFFFu};
    const uint16_t segments[]={0u,0x7000u,0xFFFFu};
    const uint8_t pairs[][2]={{0u,0u},{1u,1u},{2u,3u},{0x7Fu,0x80u},
        {0x80u,0xFFu},{0xFFu,0x7Fu},{0x80u,0x80u},{0xFFu,0xFFu}};
    uint8_t *ram=malloc(PR_U3C2_02CC_RAM_SIZE);
    uint8_t *expected=malloc(PR_U3C2_02CC_RAM_SIZE);
    uint32_t seed=0x02CC2026u;
    unsigned bulk=0u,edges=0u;
    assert(ram && expected);
    for (size_t i=0u;i<PR_U3C2_02CC_RAM_SIZE;++i) {
        seed=seed*1664525u+1013904223u;ram[i]=(uint8_t)(seed>>24);
    }
    memcpy(expected,ram,PR_U3C2_02CC_RAM_SIZE);
    guards(ram,expected);
    /* Literal anchors keep independent carry/AF/parity/sign expectations. */
    assert(final_flags(0x0AD7u,3u,1u)==0x0202u);
    assert(final_flags(0x0AD7u,0xFFFDu,1u)==0x0282u);
    assert(final_flags(0x0AD7u,0xFFFFu,1u)==0x0257u);
    for (unsigned path=0u;path<6u;++path) {
        for (unsigned pair=0u;pair<65536u;++pair) {
            compare(ram,expected,0x7000u,0x8000u,0x2200u,path,
                    (uint8_t)pair,(uint8_t)(pair/256u),
                    (uint16_t)(pair^0x0AD7u),0,0);
            ++bulk;
        }
        assert(memcmp(ram,expected,PR_U3C2_02CC_RAM_SIZE)==0);
    }
    for (unsigned s=0u;s<3u;++s)
        for (unsigned p=0u;p<9u;++p)
            for (unsigned layout=0u;layout<3u;++layout)
                for (unsigned v=0u;v<8u;++v)
                    for (unsigned path=0u;path<6u;++path) {
                        uint16_t rec=layout==0u?0x2200u:layout==1u?0xFFF0u:0x25C0u;
                        compare(ram,expected,segments[s],stacks[p],rec,path,
                                pairs[v][0],pairs[v][1],0x0AD7u,layout==2u,1);
                        ++edges;
                    }
    const uint16_t word_pairs[6][2]={{0x7FFFu,0x8000u},{0xFFFFu,0u},
        {0x8000u,0x7FFFu},{0x8000u,0xFFFFu},{0u,0xFFFFu},{0x8000u,0x8000u}};
    for (unsigned i=0u;i<6u;++i) {
        signed_comparison_case(ram,expected,word_pairs[i][0],word_pairs[i][1]);
        ++edges;
    }
    alias_case(ram,expected,0);alias_case(ram,expected,1);edges+=2u;
    free(expected);free(ram);
    printf("U3C2 +02CC synthetic PASS: %u all14 CPU cases; %u full-RAM edges; 5 guards\n",
           bulk+edges,edges);
    return 0;
}
