#include "poolrad/recovered/overlay_u68_roll_gate_0294.h"
#include "dice_chain_fixture.h"
#include <stdio.h>
#include <stdlib.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
enum { CS=0x3000, DS=0x1000, SS=0x2000, MAP=0x6000, NODE=0x5000 };
static uint8_t ram[DICE_RAM], snapshot[DICE_RAM], expected_map[0x400];
static const uint16_t sites[]={0x300,0x319,0x33D,0x35A,0x373,0x38C,0x3C2,0x3E0,0x3FD,0x425,0x442,0x45B};
typedef struct { uint32_t seed; unsigned marks,rolls; } expected_state;

static uint8_t roll(void *ctx,uint8_t sides,uint8_t count)
{
    expected_state *e=ctx;
    ++e->rolls;
    return dice_roll(&e->seed,count,sides);
}

static void mark(void *ctx,uint8_t x,uint8_t y,uint8_t selector)
{
    expected_state *e=ctx;
    CHECK(x==(e->marks ? 4 : 3) && y==4 && selector==(e->marks ? 6 : 2));
    ++e->marks;
    expected_map[0x300+16u*y+x]=(uint8_t)((expected_map[0x300+16u*y+x] & ~(3u<<selector)) | (1u<<selector));
}

static pr_u68_roll_gate_cpu frame(unsigned field,unsigned kind,unsigned value,uint32_t seed)
{
    pr_u68_roll_gate_cpu c={0x1234,0x2345,0x3456,0x4567,0x5678,0x6789,0x789A,0x8000,CS,0x294,DS,0x2222,SS,0x0603};
    memset(ram,0xA5,sizeof ram);
    for (unsigned i=0;i<sizeof sites/sizeof sites[0];++i)
        dice_site(ram,CS,sites[i],0x48,0xDE3);
    dice_system(ram,DS,seed);
    ram[dice_physical(CS,0x2D4)]=0x9A;
    dice_put16(ram,CS,0x2D5,0x2F);dice_put16(ram,CS,0x2D7,0x3D5);
    ram[dice_physical(0x3D5,0x2F)]=0xEA;
    dice_put16(ram,0x3D5,0x30,0x350);dice_put16(ram,0x3D5,0x32,0x7000);
    for (unsigned i=0;i<2;++i) {
        uint16_t s=i ? 0x4CF : 0x491;
        ram[dice_physical(CS,s)]=0xE8;
        dice_put16(ram,CS,(uint16_t)(s+1),(uint16_t)(0x109-s-3));
    }
    dice_put16(ram,DS,0x5D96,0);dice_put16(ram,DS,0x5D98,NODE);
    ram[dice_physical(NODE,0x10)]=(uint8_t)kind;
    ram[dice_physical(NODE,0x16)]=(uint8_t)value;
    dice_put16(ram,NODE,0x104,0);dice_put16(ram,NODE,0x106,0);
    ram[dice_physical(DS,0x6AAD)]=3;ram[dice_physical(DS,0x6AAE)]=4;ram[dice_physical(DS,0x6AAF)]=2;
    ram[dice_physical(DS,0x84DC)]=0;ram[dice_physical(DS,0x6DDE)]=0xA5;
    ram[dice_physical(DS,0x2738)]=1;ram[dice_physical(DS,0x2741)]=0;
    dice_put16(ram,DS,0x6A5C,0x100);dice_put16(ram,DS,0x6A5E,MAP);
    ram[dice_physical(MAP,0x443)]=(uint8_t)((0xA5u & ~12u) | field*4u);
    dice_put16(ram,SS,0x8000,0x7000);dice_put16(ram,SS,0x8002,0x9200);
    return c;
}

static void semantics(void)
{
    static const unsigned values[]={0,0x32,0x33,0x5A,0x5B,0x63,0x64,0x65,0xFF};
    uint8_t directions[265]={0};
    directions[2]=1;
    for (unsigned field=1;field<=3;field+=2)
        for (unsigned kind=0;kind<256;++kind)
            for (unsigned v=0;v<sizeof values/sizeof values[0];++v)
                for (unsigned high=0;high<8;++high) {
                    uint32_t seed=(((uint32_t)high<<16)-1u)*UINT32_C(0xD94FA8CD);
                    pr_u68_roll_gate_cpu c=frame(field,kind,values[v],seed);
                    pr_u68_roll_gate_cpu_report r;
                    pr_u68_roll_gate_state st={0};
                    pr_u68_roll_gate_report legacy;
                    pr_u68_segment_view view={NODE,ram+dice_physical(NODE,0),65536};
                    pr_u68_far visited[1];
                    uint8_t want=0;
                    expected_state e={seed,0,0};
                    memcpy(expected_map,ram+dice_physical(MAP,0x100),sizeof expected_map);
                    st.head.segment=NODE;st.views=&view;st.view_count=1;
                    st.visited=visited;st.visited_capacity=1;
                    st.party_x=3;st.party_y=4;st.facing=2;st.ds_6dde=0xA5;
                    st.direction_table=directions;st.direction_table_size=sizeof directions;
                    st.field.map_id=0;st.field.buffer=expected_map;
                    st.hooks.roll_9e_entry8=roll;st.hooks.mark_0109=mark;st.hooks.ctx=&e;
                    CHECK(pr_u68_roll_gate(&st,&want,&legacy)==PR_U68_ROLL_OK);
                    CHECK(pr_u68_roll_gate_0294_cpu(ram,DICE_RAM,&c,16,256,&r)==PR_U68_ROLL_CPU_OK);
                    CHECK((uint8_t)c.ax==want && ram[dice_physical(DS,0x6DDE)]==st.ds_6dde);
                    CHECK(dice_seed(ram,DS)==e.seed);
                    CHECK(!memcmp(expected_map,ram+dice_physical(MAP,0x100),sizeof expected_map));
                    CHECK(c.cs==0x9200 && c.ip==0x7000 && c.sp==0x8004 && c.bp==0x789A);
                    CHECK(c.si==0x5678 && c.ds==DS && c.ss==SS);
                    CHECK(r.status==0 && r.callee==(want ? PR_U68_ROLL_CPU_MAP : e.rolls ? PR_U68_ROLL_CPU_DICE : PR_U68_ROLL_CPU_FIELD));
                }
}

static void boundaries(void)
{
    pr_u68_roll_gate_cpu c=frame(3,0x19,0,123),before=c;
    pr_u68_roll_gate_cpu_report r;
    memcpy(snapshot,ram,sizeof ram);
    CHECK(pr_u68_roll_gate_0294_cpu(NULL,DICE_RAM,&c,0,0,&r)==PR_U68_ROLL_CPU_ARGUMENT);
    CHECK(pr_u68_roll_gate_0294_cpu(ram,DICE_RAM,NULL,0,0,&r)==PR_U68_ROLL_CPU_ARGUMENT);
    CHECK(pr_u68_roll_gate_0294_cpu(ram,DICE_RAM-1,&c,0,0,&r)==PR_U68_ROLL_CPU_ARGUMENT);
    CHECK(!memcmp(&c,&before,sizeof c) && !memcmp(snapshot,ram,sizeof ram));
    CHECK(r.callee==PR_U68_ROLL_CPU_NONE && r.status==-1);
    for (unsigned i=0;i<2;++i) {
        c=before;c.sp=i ? 0xFFFD : 0x003F;
        CHECK(pr_u68_roll_gate_0294_cpu(ram,DICE_RAM,&c,0,0,NULL)==PR_U68_ROLL_CPU_ARGUMENT);
    }
    c=before;c.ip++;
    CHECK(pr_u68_roll_gate_0294_cpu(ram,DICE_RAM,&c,0,0,NULL)==PR_U68_ROLL_CPU_ARGUMENT);
    c=before;ram[dice_physical(CS,0x2D4)]=0x90;
    CHECK(pr_u68_roll_gate_0294_cpu(ram,DICE_RAM,&c,0,0,&r)==PR_U68_ROLL_CPU_CODE);
    CHECK(c.ip==0x2D4 && c.sp==0x7FEE && c.bp==0x7FFE && r.callee==PR_U68_ROLL_CPU_FIELD && r.status==-1);
    CHECK(dice_seed(ram,DS)==123);
    c=frame(3,0x19,0,123);ram[dice_physical(CS,0x491)]=0x90;
    CHECK(pr_u68_roll_gate_0294_cpu(ram,DICE_RAM,&c,16,256,&r)==PR_U68_ROLL_CPU_CODE);
    CHECK(c.ip==0x491 && c.sp==0x7FEC && r.callee==PR_U68_ROLL_CPU_MAP && r.status==-1);
    CHECK(ram[dice_physical(MAP,0x443)]==0xAD); /* First mark has not run. */
    c=frame(3,0,0,123);dice_put16(ram,NODE,0x106,NODE);
    CHECK(pr_u68_roll_gate_0294_cpu(ram,DICE_RAM,&c,0,256,&r)==PR_U68_ROLL_CPU_LIMIT);
    CHECK(c.cs==CS && c.ip==0x2AB && c.sp==0x7FF4 && r.callee==PR_U68_ROLL_CPU_FIELD && r.status==0);
    CHECK(ram[dice_physical(DS,0x6DDE)]==0 && dice_seed(ram,DS)==123);
    c=frame(3,0x12,95,123);
    CHECK(pr_u68_roll_gate_0294_cpu(ram,DICE_RAM,&c,16,0,&r)==PR_U68_ROLL_CPU_CALLEE);
    CHECK(c.cs==DICE_CS && c.ip==0xE0E && c.sp==0x7FE4 && c.bp==0x7FEA);
    CHECK(r.callee==PR_U68_ROLL_CPU_DICE && r.status==11 && !(c.flags&0x10));
    CHECK(dice_seed(ram,DS)==123);
    c=frame(3,0x19,0,123);dice_put16(ram,DS,0x5D98,0);
    CHECK(pr_u68_roll_gate_0294_cpu(ram,DICE_RAM,&c,0,0,&r)==PR_U68_ROLL_CPU_OK);
    CHECK((uint8_t)c.ax==0 && r.callee==PR_U68_ROLL_CPU_NONE && r.status==-1);
}

int main(void)
{
    boundaries();
    semantics();
    printf("U68 0294 raw upgrade: %u checks passed\n",checks);
    return 0;
}
