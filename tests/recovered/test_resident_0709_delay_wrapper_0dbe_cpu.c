/* Focused synthetic test for the raw-CPU CRT Delay 0A40:029C and the raw-CPU resident 0709:0DBE composing it
 * (docs/re/RES_0709_DELAY_0DBE_CPU.md). No game bytes beyond the wrapper's LCALL encoding. */
#include "poolrad/recovered/resident_0709_delay_wrapper_0dbe_cpu.h"
#include "poolrad/recovered/resident_a40_delay_029c_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "delay 0dbe line %d: %s\n", __LINE__, #x);            \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { RAM = 1u << 20, DS0 = 0x1000, SS0 = 0x2000, SP0 = 0x8000 };

static uint8_t *ram;
static unsigned checks;

static size_t at(uint16_t segment, uint16_t offset) { return (((size_t)segment << 4) + offset) & (RAM - 1u); }
static void put16(uint16_t segment, uint16_t offset, uint16_t value)
{
    ram[at(segment, offset)] = (uint8_t)value;
    ram[at(segment, (uint16_t)(offset + 1u))] = (uint8_t)(value >> 8);
}
static uint16_t get16(uint16_t segment, uint16_t offset) { return (uint16_t)(ram[at(segment, offset)] | (ram[at(segment, (uint16_t)(offset + 1u))] << 8)); }

/* BIOS byte 0040:0000 = 5Ah, DS:[84FCh] = the per-millisecond count, DS:[49E2h] = the wrapper's factor. */
static void world(uint16_t count, uint8_t factor)
{
    memset(ram, 0xA5, RAM);
    ram[0x400] = 0x5A;
    put16(DS0, 0x84FCu, count);
    ram[at(DS0, 0x49E2u)] = factor;
    ram[at(0x0709u, 0x0DCCu)] = 0x9A;
    put16(0x0709u, 0x0DCDu, 0x029Cu);
    put16(0x0709u, 0x0DCFu, 0x0A40u);
}

static pr_a40_delay_cpu delay_frame(uint16_t ss, uint16_t sp, uint16_t ms)
{
    pr_a40_delay_cpu cpu = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu, 0x7777u, sp, 0x0A40u, 0x029Cu, DS0, 0x1111u, ss, 0x7202u};
    put16(ss, sp, 0x7000u); put16(ss, (uint16_t)(sp + 2u), 0x7001u); put16(ss, (uint16_t)(sp + 4u), ms);
    return cpu;
}

static pr_r0709_delay_cpu wrapper_frame(void)
{
    pr_r0709_delay_cpu cpu = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu, 0x7777u, SP0, 0x0709u, 0x0DBEu, DS0, 0x1111u, SS0, 0x7202u};
    put16(SS0, SP0, 0x7000u); put16(SS0, SP0 + 2u, 0x7001u);
    return cpu;
}

int main(void)
{
    pr_a40_delay_cpu delay;
    pr_r0709_delay_cpu cpu;
    int callee = 99;
    ram = malloc(RAM);
    if (ram == NULL)
        return 1;

    /* Delay(2) with three passes per millisecond: 23 loop-backs; DEC DX's flags with the equal CMP's CF. */
    world(3, 0);
    delay = delay_frame(SS0, SP0, 2);
    CHECK(pr_a40_delay_029c_cpu(ram, RAM, &delay, 23) == PR_A40_DELAY_CPU_RETURNED);
    CHECK(delay.cs == 0x7001u && delay.ip == 0x7000u && delay.sp == SP0 + 6u && delay.flags == 0x7246u);
    CHECK(delay.ax == 0x005Au && delay.bx == 0 && delay.cx == 0 && delay.dx == 0 && delay.es == 0x0040u && delay.di == 0);
    CHECK(get16(SS0, SP0 - 2u) == 0x02B7u && delay.bp == 0x7777u && delay.si == 0x9ABCu);

    /* Cap 22: the 23rd loop-back is the last pass's third JNE 02C0. */
    world(3, 0);
    delay = delay_frame(SS0, SP0, 2);
    CHECK(pr_a40_delay_029c_cpu(ram, RAM, &delay, 22) == PR_A40_DELAY_CPU_LIMIT && delay.ip == 0x02C0u);
    CHECK(delay.bx == 1 && delay.cx == 1 && delay.dx == 1 && delay.sp == SP0 - 2u);

    /* Delay(0): OR DX,DX returns at once; BX = the entry SP. */
    world(3, 0);
    delay = delay_frame(SS0, SP0, 0);
    CHECK(pr_a40_delay_029c_cpu(ram, RAM, &delay, 0) == PR_A40_DELAY_CPU_RETURNED);
    CHECK(delay.bx == SP0 && delay.dx == 0 && delay.ax == 0x1234u && delay.es == 0x1111u && delay.flags == 0x7246u && delay.sp == SP0 + 6u);

    /* The CALL's return word lands on 0040:0000 (SS:SP = 0000:0402): CMP 5Ah,B7h ends LOOPE after one pass with CF. */
    world(3, 0);
    delay = delay_frame(0x0000u, 0x0402u, 1);
    CHECK(pr_a40_delay_029c_cpu(ram, RAM, &delay, 100) == PR_A40_DELAY_CPU_RETURNED);
    CHECK(delay.cx == 2 && delay.ax == 0x005Au && ram[0x400] == 0xB7 && ram[0x401] == 0x02 && delay.flags == 0x7247u);

    /* 0709:0DBE with factor 1 and one pass per millisecond: Delay(225), 899 loop-backs. */
    world(1, 1);
    cpu = wrapper_frame();
    CHECK(pr_r0709_delay_wrapper_0dbe_cpu(ram, RAM, &cpu, 899, &callee) == PR_R0709_DELAY_CPU_OK && callee == 0);
    CHECK(cpu.cs == 0x7001u && cpu.ip == 0x7000u && cpu.sp == SP0 + 4u && cpu.bp == 0x7777u && cpu.flags == 0x7246u);
    CHECK(cpu.ax == 0x005Au && cpu.bx == 0 && cpu.cx == 0 && cpu.dx == 0 && cpu.es == 0x0040u && cpu.di == 0);
    CHECK(get16(SS0, SP0 - 2u) == 0x7777u && get16(SS0, SP0 - 4u) == 225u && get16(SS0, SP0 - 6u) == 0x0709u && get16(SS0, SP0 - 8u) == 0x0DD1u);

    /* One loop-back fewer: the delay's LIMIT is the wrapper's CALLEE, stopped inside A40. */
    world(1, 1);
    cpu = wrapper_frame();
    CHECK(pr_r0709_delay_wrapper_0dbe_cpu(ram, RAM, &cpu, 898, &callee) == PR_R0709_DELAY_CPU_CALLEE);
    CHECK(callee == (int)PR_A40_DELAY_CPU_LIMIT && cpu.cs == 0x0A40u && cpu.ip == 0x02C0u);

    /* Factor 0: Delay(0); AX = 0, BX = the delay's entry SP, OR DX,DX's flags. */
    world(1, 0);
    cpu = wrapper_frame();
    CHECK(pr_r0709_delay_wrapper_0dbe_cpu(ram, RAM, &cpu, 0, &callee) == PR_R0709_DELAY_CPU_OK && callee == 0);
    CHECK(cpu.ax == 0 && cpu.dx == 0 && cpu.bx == SP0 - 8u && cpu.cx == 0x5678u && cpu.flags == 0x7246u && cpu.sp == SP0 + 4u);

    /* CODE at +0DCC with 2 * 225 pushed; MUL's flags as Unicorn sets them (PF from C2h is odd). */
    world(1, 2);
    ram[at(0x0709u, 0x0DCDu)] = 0x9D;
    cpu = wrapper_frame();
    CHECK(pr_r0709_delay_wrapper_0dbe_cpu(ram, RAM, &cpu, 64, &callee) == PR_R0709_DELAY_CPU_CODE && callee == -1);
    CHECK(cpu.ip == 0x0DCCu && cpu.sp == SP0 - 4u && get16(SS0, cpu.sp) == 450u && cpu.dx == 0 && cpu.flags == 0x7202u);

    /* Guards change nothing. */
    world(1, 1);
    cpu = wrapper_frame();
    cpu.ip = 0x0DBFu;
    CHECK(pr_r0709_delay_wrapper_0dbe_cpu(ram, RAM, &cpu, 64, &callee) == PR_R0709_DELAY_CPU_ARGUMENT && callee == -1 && cpu.sp == SP0);
    CHECK(pr_r0709_delay_wrapper_0dbe_cpu(NULL, RAM, &cpu, 64, NULL) == PR_R0709_DELAY_CPU_ARGUMENT);
    delay = delay_frame(SS0, SP0, 1);
    delay.ip = 0x029Du;
    CHECK(pr_a40_delay_029c_cpu(ram, RAM, &delay, 64) == PR_A40_DELAY_CPU_ARGUMENT && delay.sp == SP0);
    CHECK(pr_a40_delay_029c_cpu(ram, RAM - 1u, &delay, 64) == PR_A40_DELAY_CPU_ARGUMENT);

    free(ram);
    printf("delay 029C/0DBE: %u checks passed\n", checks);
    return 0;
}
