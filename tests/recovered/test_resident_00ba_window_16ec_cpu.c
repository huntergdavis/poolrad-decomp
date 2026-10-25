#include "poolrad/recovered/resident_00ba_window_16ec_cpu.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; assert(x); } while (0)
static size_t at(unsigned seg, unsigned off) { return ((seg << 4u) + (off & 65535u)) & (PR_BUS_RAM_SIZE - 1u); }
static void word(uint8_t *m, unsigned seg, unsigned off, unsigned v)
{ m[at(seg, off)] = (uint8_t)v; m[at(seg, off + 1u)] = (uint8_t)(v >> 8u); }
static unsigned get(const uint8_t *m, unsigned seg, unsigned off)
{ return m[at(seg, off)] | ((unsigned)m[at(seg, off + 1u)] << 8u); }
static pr_res_16ec_cpu setup(pr_bus *bus)
{
    pr_res_16ec_cpu c = {0x1234,0x2345,0x3456,0x4567,0x5678,0x6789,0x7777,
                        0x9000,0x1000,0x16EC,0x2000,0x1111,0x3000,0x0203};
    memset(bus->ram, 0, PR_BUS_RAM_SIZE);
    word(bus->ram,c.ss,c.sp,0xBEEF); word(bus->ram,c.ss,c.sp+2u,0x7000);
    word(bus->ram,c.ss,c.sp+8u,0x1234); word(bus->ram,c.ss,c.sp+10u,0x4500);
    return c;
}
int main(void)
{
    uint8_t *ram = malloc(PR_BUS_RAM_SIZE), *before = malloc(PR_BUS_RAM_SIZE);
    pr_bus bus = {0}; pr_res_16ec_env env = {0}; pr_res_16ec_diagnostic d;
    pr_res_16ec_cpu c, saved; size_t budget;
    CHECK(ram && before); bus.ram = ram;
    c = setup(&bus); saved = c; memcpy(before,ram,PR_BUS_RAM_SIZE); budget = 12;
    CHECK(pr_res_00ba_window_16ec_cpu(NULL,PR_BUS_RAM_SIZE,&c,&budget,&env,&d)==PR_RES_16EC_ARGUMENT);
    CHECK(pr_res_00ba_window_16ec_cpu(&bus,PR_BUS_RAM_SIZE-1,&c,&budget,&env,&d)==PR_RES_16EC_ARGUMENT);
    CHECK(pr_res_00ba_window_16ec_cpu(&bus,PR_BUS_RAM_SIZE,&c,NULL,&env,&d)==PR_RES_16EC_ARGUMENT);
    CHECK(pr_res_00ba_window_16ec_cpu(&bus,PR_BUS_RAM_SIZE,&c,&budget,NULL,&d)==PR_RES_16EC_ARGUMENT);
    CHECK(!memcmp(&c,&saved,sizeof c) && budget==12 && !memcmp(ram,before,PR_BUS_RAM_SIZE));
    for (unsigned sp=0; sp<2; ++sp) {
        c = setup(&bus); c.sp = sp ? 0xFFF1 : 0x07FF; saved=c; budget=12;
        CHECK(pr_res_00ba_window_16ec_cpu(&bus,PR_BUS_RAM_SIZE,&c,&budget,&env,&d)==PR_RES_16EC_ARGUMENT);
        CHECK(!memcmp(&c,&saved,sizeof c) && budget==12);
    }
    c=setup(&bus); saved=c; memcpy(before,ram,PR_BUS_RAM_SIZE); budget=0;
    CHECK(pr_res_00ba_window_16ec_cpu(&bus,PR_BUS_RAM_SIZE,&c,&budget,&env,&d)==PR_RES_16EC_LIMIT);
    CHECK(!memcmp(&c,&saved,sizeof c) && !memcmp(ram,before,PR_BUS_RAM_SIZE));
    c=setup(&bus); budget=3;
    CHECK(pr_res_00ba_window_16ec_cpu(&bus,PR_BUS_RAM_SIZE,&c,&budget,&env,&d)==PR_RES_16EC_LIMIT);
    CHECK(c.ip==0x16F3 && c.bp==0x8FFE && c.sp==0x8EFC && c.flags==0x0286 && budget==0);
    CHECK(get(ram,c.ss,0x8FFE)==0x7777 && get(ram,c.ss,0x9000)==0xBEEF);
    c=setup(&bus); budget=11;
    CHECK(pr_res_00ba_window_16ec_cpu(&bus,PR_BUS_RAM_SIZE,&c,&budget,&env,&d)==PR_RES_16EC_LIMIT);
    CHECK(c.ip==0x1702 && c.sp==0x8EF2 && budget==0 && get(ram,c.ss,c.sp)==255);
    c=setup(&bus); budget=12;
    CHECK(pr_res_00ba_window_16ec_cpu(&bus,PR_BUS_RAM_SIZE,&c,&budget,&env,&d)==PR_RES_16EC_CODE);
    CHECK(c.ip==0x1702 && c.sp==0x8EF2 && budget==1 && d.site==0x1702);
    c=setup(&bus); budget=12; ram[at(c.cs,0x1702)]=0x9A;
    word(ram,c.cs,0x1703,0x0B26); word(ram,c.cs,0x1705,0x0AF8);
    CHECK(pr_res_00ba_window_16ec_cpu(&bus,PR_BUS_RAM_SIZE,&c,&budget,&env,&d)==PR_RES_16EC_CODE);
    CHECK(c.ip==0x1702 && budget==1);
    c=setup(&bus); budget=12; ram[at(c.cs,0x1702)]=0x9A;
    word(ram,c.cs,0x1703,0x0B25); word(ram,c.cs,0x1705,0x0AF8);
    ram[at(0x4500,0x1234)]=2; ram[at(0x4500,0x1235)]='A'; ram[at(0x4500,0x1236)]='B';
    CHECK(pr_res_00ba_window_16ec_cpu(&bus,PR_BUS_RAM_SIZE,&c,&budget,&env,&d)==PR_RES_16EC_LIMIT);
    CHECK(c.ip==0x1707 && c.cs==0x1000 && c.sp==0x8EFC && budget==0);
    CHECK(ram[at(c.ss,0x8EFE)]==2 && ram[at(c.ss,0x8EFF)]=='A' && ram[at(c.ss,0x8F00)]=='B');
    free(before); free(ram); printf("16EC boundary checks: %u\n",checks); return 0;
}
