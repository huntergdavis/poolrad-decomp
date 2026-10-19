#include "poolrad/recovered/overlay_u3b0_entry6_1140.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "poolrad/recovered/system_getmem_023f_cpu.h"

typedef struct {
    unsigned calls;
    unsigned abort_at;
} fixture;

static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (PR_U3B0_1140_RAM_SIZE - 1u);
}

static void put16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t p = physical(seg, off);
    ram[p] = (uint8_t)value;
    ram[(p + 1u) & (PR_U3B0_1140_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static uint16_t get16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t p = physical(seg, off);
    return (uint16_t)(ram[p] |
        ((uint16_t)ram[(p + 1u) & (PR_U3B0_1140_RAM_SIZE - 1u)] << 8));
}

static uint16_t node_off(unsigned i)
{
    return (uint16_t)(0x4000u + i * 0x40u);
}

static uint16_t node_seg(unsigned i)
{
    return (uint16_t)(0x2200u + i * 0x100u);
}

static int alloc(void *context, uint8_t *ram, size_t ram_size,
                 pr_u3b0_1140_cpu *cpu)
{
    fixture *f = context;
    unsigned i = f->calls;
    unsigned j;
    assert(ram_size == PR_U3B0_1140_RAM_SIZE);
    assert(cpu->ip == (i == 0u ? 0x114Au : 0x11A8u));
    assert(get16(ram, cpu->ss, cpu->sp) == 0x002Eu);
    ++f->calls;
    if (f->calls == f->abort_at) return 0;
    for (j = 0u; j < 0x2Eu; ++j)
        ram[physical(node_seg(i), (uint16_t)(node_off(i) + j))] =
            (uint8_t)(0xB0u + i + j);
    cpu->ax = node_off(i);
    cpu->dx = node_seg(i);
    cpu->bx = (uint16_t)(0xB000u + i);
    cpu->flags = 0x0A53u;
    return 1;
}

static void run(uint16_t count, unsigned abort_at, int alias_return)
{
    uint8_t *ram = calloc(PR_U3B0_1140_RAM_SIZE, 1u);
    pr_u3b0_1140_cpu cpu = {0};
    fixture f = {0u, abort_at};
    uint16_t output_seg, output_off;
    unsigned i;
    assert(ram != NULL);
    cpu.ax = 0xA5CDu; cpu.bp = 0x7777u; cpu.sp = 0x8000u;
    cpu.cs = 0xF000u; cpu.ip = 0x1140u;
    cpu.ds = 0x3000u; cpu.ss = 0xE000u;
    output_seg = alias_return ? cpu.ss : cpu.ds;
    output_off = alias_return ? cpu.sp : 0x7000u;
    put16(ram, cpu.ss, cpu.sp, 0x0712u);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 2u), 0xD000u);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 4u), output_off);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 6u), output_seg);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 8u), count);
    if (abort_at != 0u) {
        assert(pr_u3b0_entry6_1140(ram, PR_U3B0_1140_RAM_SIZE,
                                    &cpu, alloc, &f) == PR_U3B0_1140_ALLOC_ABORT);
        assert(f.calls == abort_at);
        assert(cpu.ip == (abort_at == 1u ? 0x114Au : 0x11A8u));
    } else {
        unsigned nodes = count >= 2u && count < 0x8000u ? count : 1u;
        assert(pr_u3b0_entry6_1140(ram, PR_U3B0_1140_RAM_SIZE,
                                    &cpu, alloc, &f) == PR_U3B0_1140_OK);
        assert(f.calls == nodes && cpu.sp == 0x800Au && cpu.bp == 0x7777u);
        assert(cpu.ip == (alias_return ? node_off(0u) : 0x0712u));
        assert(cpu.cs == (alias_return ? node_seg(0u) : 0xD000u));
        if (!alias_return) {
            assert(get16(ram, output_seg, output_off) == node_off(0u));
            assert(get16(ram, output_seg, (uint16_t)(output_off + 2u)) == node_seg(0u));
        }
        for (i = 0u; i < nodes; ++i) {
            assert(ram[physical(node_seg(i), node_off(i))] == 0u);
            assert(ram[physical(node_seg(i), (uint16_t)(node_off(i) + 0x29u))] == 0u);
            assert(get16(ram, node_seg(i), (uint16_t)(node_off(i) + 0x2Au)) ==
                   (i + 1u < nodes ? node_off(i + 1u) : 0u));
            assert(get16(ram, node_seg(i), (uint16_t)(node_off(i) + 0x2Cu)) ==
                   (i + 1u < nodes ? node_seg(i + 1u) : 0u));
            assert(ram[physical(node_seg(i), (uint16_t)(node_off(i) + 1u))] ==
                   (uint8_t)(0xB1u + i));
        }
    }
    free(ram);
}

/* Synthetic heap and caller fixtures; no original media. */
static unsigned long raw_checks;
#define CHECK(x) do { ++raw_checks; assert(x); } while (0)

static void raw_world(uint8_t *ram, pr_u3b0_1140_cpu *cpu, uint16_t count)
{
    memset(ram, 0xA5, PR_U3B0_1140_RAM_SIZE);
    *cpu = (pr_u3b0_1140_cpu){0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,
                             0x7777,0x8000,0x8800,0x1140,0x1800,0x7890,0x2800,0x0602};
    put16(ram,cpu->ss,0x8000,0x7000); put16(ram,cpu->ss,0x8002,0x9001);
    put16(ram,cpu->ss,0x8004,0x7000); put16(ram,cpu->ss,0x8006,cpu->ds);
    put16(ram,cpu->ss,0x8008,count);
    put16(ram,cpu->ds,0x4404,0); put16(ram,cpu->ds,0x4406,0x3400);
    put16(ram,cpu->ds,0x4408,0); put16(ram,cpu->ds,0x440A,0x5000);
    put16(ram,cpu->ds,0x440C,0); put16(ram,cpu->ds,0x440E,0x3400);
    put16(ram,cpu->ds,0x4414,0x00A9); put16(ram,cpu->ds,0x4416,0x0AF8);
    ram[physical(cpu->cs,0x114A)]=ram[physical(cpu->cs,0x11A8)]=0x9A;
    put16(ram,cpu->cs,0x114B,0x023F); put16(ram,cpu->cs,0x114D,0x0AF8);
    put16(ram,cpu->cs,0x11A9,0x023F); put16(ram,cpu->cs,0x11AB,0x0AF8);
}

static void raw_semantics(uint8_t *ram)
{
    static const uint16_t counts[]={0,1,2,3,4,7,16,31,64,127,0x8000,0x8001,0xFFFE,0xFFFF};
    static const uint16_t flags[]={2,0x0602,0x08D7,0x0047,0x0403,0x0213};
    pr_u3b0_1140_cpu cpu;
    unsigned n,f,j,k;
    int diagnostic;
    for (n=0;n<sizeof(counts)/sizeof(*counts);++n) {
        unsigned nodes=(counts[n]>=2 && counts[n]<0x8000)?counts[n]:1;
        for(f=0;f<sizeof(flags)/sizeof(*flags);++f) {
            raw_world(ram,&cpu,counts[n]);cpu.flags=flags[f];
            CHECK(pr_u3b0_entry6_1140_cpu(ram,PR_U3B0_1140_RAM_SIZE,&cpu,128,64,&diagnostic)==PR_U3B0_1140_CPU_OK);
            CHECK(diagnostic==0 && cpu.sp==0x800A && cpu.bp==0x7777 && cpu.cs==0x9001 && cpu.ip==0x7000);
            CHECK(get16(ram,cpu.ds,0x7000)==0 && get16(ram,cpu.ds,0x7002)==0x3400);
            CHECK(get16(ram,cpu.ds,0x4404)==0 && get16(ram,cpu.ds,0x4406)==0x3400+3*nodes);
            for(j=0;j<nodes;++j) {
                uint16_t seg=(uint16_t)(0x3400+3*j);
                CHECK(ram[physical(seg,0)]==0 && ram[physical(seg,0x29)]==0);
                CHECK(get16(ram,seg,0x2A)==0);
                CHECK(get16(ram,seg,0x2C)==(j+1<nodes?(uint16_t)(seg+3):0));
                for(k=1;k<0x29;++k) CHECK(ram[physical(seg,(uint16_t)k)]==0xA5);
                CHECK(get16(ram,seg,0x2E)==0xA5A5); /* allocator rounds to 48, caller touches 46 */
            }
        }
    }
}

static void raw_stops(uint8_t *ram)
{
    pr_u3b0_1140_cpu cpu, saved;
    int diagnostic;
    unsigned n;
    uint8_t *copy=malloc(PR_U3B0_1140_RAM_SIZE);
    CHECK(copy!=NULL);
    raw_world(ram,&cpu,3);saved=cpu;memcpy(copy,ram,PR_U3B0_1140_RAM_SIZE);
    CHECK(pr_u3b0_entry6_1140_cpu(NULL,PR_U3B0_1140_RAM_SIZE,&cpu,4,4,&diagnostic)==PR_U3B0_1140_CPU_ARGUMENT);
    CHECK(diagnostic==-1 && memcmp(&cpu,&saved,sizeof cpu)==0);
    CHECK(pr_u3b0_entry6_1140_cpu(ram,PR_U3B0_1140_RAM_SIZE-1,&cpu,4,4,NULL)==PR_U3B0_1140_CPU_ARGUMENT);
    CHECK(memcmp(copy,ram,PR_U3B0_1140_RAM_SIZE)==0 && memcmp(&cpu,&saved,sizeof cpu)==0);
    for(n=0;n<4;++n) {
        raw_world(ram,&cpu,7);
        CHECK(pr_u3b0_entry6_1140_cpu(ram,PR_U3B0_1140_RAM_SIZE,&cpu,n,64,&diagnostic)==PR_U3B0_1140_CPU_LIMIT);
        CHECK(cpu.ip==(n?0x11A8:0x114A) && cpu.cs==0x8800 && cpu.sp==0x7FF4);
        CHECK(diagnostic==(n?0:-1) && get16(ram,cpu.ss,cpu.sp)==0x2E);
        CHECK(get16(ram,cpu.ds,0x4406)==0x3400+3*n);
    }
    for(n=0;n<2;++n) {
        raw_world(ram,&cpu,3);
        ram[physical(cpu.cs,n?0x11A8:0x114A)]=0x90;
        CHECK(pr_u3b0_entry6_1140_cpu(ram,PR_U3B0_1140_RAM_SIZE,&cpu,n,64,&diagnostic)==PR_U3B0_1140_CPU_CODE);
        CHECK(cpu.ip==(n?0x11A8:0x114A) && cpu.sp==0x7FF4 && diagnostic==(n?0:-1));
    }
    for(n=0;n<3;++n) {
        raw_world(ram,&cpu,4);
        put16(ram,cpu.ds,0x440A,(uint16_t)(0x3400+3*n));
        CHECK(pr_u3b0_entry6_1140_cpu(ram,PR_U3B0_1140_RAM_SIZE,&cpu,8,64,&diagnostic)==PR_U3B0_1140_CPU_CALLEE);
        CHECK(diagnostic==PR_SYSTEM_GETMEM_RUNERROR && cpu.cs==0x0AF8 && cpu.ip==0x00E2 && cpu.ax==203);
        CHECK(get16(ram,cpu.ss,cpu.sp)==(n?0x11AD:0x114F));
        CHECK(get16(ram,cpu.ds,0x4406)==0x3400+3*n);
    }
    for(n=0;n<2;++n) {
        raw_world(ram,&cpu,3);
        put16(ram,cpu.ds,0x4414,0x1234);
        if(n) put16(ram,cpu.ds,0x440A,0x3400);
        CHECK(pr_u3b0_entry6_1140_cpu(ram,PR_U3B0_1140_RAM_SIZE,&cpu,8,64,&diagnostic)==PR_U3B0_1140_CPU_CALLEE);
        CHECK(diagnostic==PR_SYSTEM_GETMEM_HEAP_ERROR && cpu.cs==0x0AF8 && cpu.ip==(n?0x03C3:0x03B7));
    }
    raw_world(ram,&cpu,3);
    put16(ram,cpu.ds,0x440E,0x3010);
    put16(ram,0x3010,0,0);put16(ram,0x3010,2,0x3010);
    put16(ram,0x3010,4,0);put16(ram,0x3010,6,0);
    CHECK(pr_u3b0_entry6_1140_cpu(ram,PR_U3B0_1140_RAM_SIZE,&cpu,8,3,&diagnostic)==PR_U3B0_1140_CPU_CALLEE);
    CHECK(diagnostic==PR_SYSTEM_GETMEM_LIMIT && cpu.ip==0x0319 && cpu.cs==0x0AF8);
    free(copy);
}

static void raw_tests(void)
{
    uint8_t *ram=malloc(PR_U3B0_1140_RAM_SIZE);
    CHECK(ram!=NULL);
    raw_semantics(ram);raw_stops(ram);free(ram);
    printf("U3B0 +1140 raw: %lu semantic/diagnostic checks passed\n",raw_checks);
}

int main(void)
{
    raw_tests();
    run(0u, 0u, 0);
    run(1u, 0u, 0);
    run(2u, 0u, 0);
    run(3u, 0u, 0);
    run(0xFFFFu, 0u, 0);
    run(2u, 0u, 1);
    run(2u, 1u, 0);
    run(2u, 2u, 0);
    run(3u, 3u, 0);
    return 0;
}
