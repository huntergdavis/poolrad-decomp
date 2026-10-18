/* Focused synthetic test for the raw-CPU SYSTEM set expansion 0AF8:0D12 and resident 0899:0104
 * (docs/re/RES_0709_0768_PURE_CALLEES_CPU.md). No game bytes. */
#include "poolrad/recovered/resident_0899_idle_0104_cpu.h"
#include "poolrad/recovered/system_set_expand_0d12_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "0768 callees line %d: %s\n", __LINE__, #x);          \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { RAM = 1u << 20, DS0 = 0x1000, SS0 = 0x2000, SP0 = 0x8000, SRC = 0x5000, DST = 0x6000 };

static uint8_t *ram;
static unsigned checks;

static size_t at(uint16_t segment, uint16_t offset) { return (((size_t)segment << 4) + offset) & (RAM - 1u); }
static void put16(uint16_t segment, uint16_t offset, uint16_t value)
{
    ram[at(segment, offset)] = (uint8_t)value;
    ram[at(segment, (uint16_t)(offset + 1u))] = (uint8_t)(value >> 8);
}

static pr_system_leaf_cpu set_frame(uint16_t count, uint16_t flags)
{
    pr_system_leaf_cpu cpu = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu, 0x7777u, SP0, 0x0AF8u, 0x0D12u, DS0, 0x1111u, SS0, flags};
    static const uint16_t words[7] = {0x7000u, 0x7001u, 0, 0x0100u, SRC, 0x0200u, DST};
    unsigned i;
    memset(ram, 0xA5, RAM);
    for (i = 0; i < 7; ++i)
        put16(SS0, (uint16_t)(SP0 + 2u * i), i == 2 ? count : words[i]);
    for (i = 0; i < 0x40u; ++i)
        ram[at(SRC, (uint16_t)(0x0100u + i))] = (uint8_t)(0x40u + i);
    return cpu;
}

int main(void)
{
    pr_system_leaf_cpu set;
    pr_r0899_idle_cpu idle = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu, 0x7777u, SP0, 0x0899u, 0x0104u, DS0, 0x1111u, SS0, 0x7202u};
    unsigned i, zeros = 0;
    ram = malloc(RAM);
    if (ram == NULL)
        return 1;

    /* BH = 2 leading zeros, BL = 3 bytes, then 20h-3-2 = 27 zeros; DF cleared; SUB 1Dh,2 = 1Bh sets only PF. */
    set = set_frame(0x0203u, 0x7602u);
    CHECK(pr_system_set_expand_0d12(ram, RAM, &set) == PR_SYSTEM_LEAF_RETURNED);
    CHECK(ram[at(DST, 0x0200u)] == 0 && ram[at(DST, 0x0201u)] == 0 && ram[at(DST, 0x0202u)] == 0x40 && ram[at(DST, 0x0204u)] == 0x42);
    for (i = 5; i < 32; ++i)
        zeros += ram[at(DST, (uint16_t)(0x0200u + i))] == 0;
    CHECK(zeros == 27 && ram[at(DST, 0x0220u)] == 0xA5);
    CHECK(set.flags == 0x7206u && set.ax == 0x1200u && set.bx == 0x0203u && set.cx == 0 && set.dx == DS0 && set.ds == DS0);
    CHECK(set.si == 0x0103u && set.di == 0x0220u && set.es == DST && set.cs == 0x7001u && set.ip == 0x7000u && set.sp == SP0 + 10u);

    /* BL = 11h and BH = 21h: CL = 20h - 32h = EEh, so 21h + 11h + EEh = 288 bytes are written. */
    set = set_frame(0x2111u, 0x7202u);
    CHECK(pr_system_set_expand_0d12(ram, RAM, &set) == PR_SYSTEM_LEAF_RETURNED);
    CHECK(set.di == 0x0200u + 288u && ram[at(DST, 0x0200u + 287u)] == 0 && ram[at(DST, 0x0200u + 288u)] == 0xA5 && ram[at(DST, 0x0221u)] == 0x40);

    /* 0899:0104: a frame, SUB SP,2 (7FFEh - 2: only PF), and the return. */
    memset(ram, 0xA5, RAM);
    put16(SS0, SP0, 0x7000u); put16(SS0, SP0 + 2u, 0x7001u);
    CHECK(pr_r0899_idle_0104_cpu(ram, RAM, &idle) == PR_R0899_IDLE_OK && idle.flags == 0x7206u);
    CHECK(idle.cs == 0x7001u && idle.ip == 0x7000u && idle.sp == SP0 + 4u && idle.bp == 0x7777u && ram[at(SS0, SP0 - 2u)] == 0x77);

    /* With SP 2 the frame sits at SS:0000 and SUB 0,2 borrows: CF, AF and SF. */
    idle.sp = 2u; idle.ip = 0x0104u; idle.flags = 0x7202u; idle.bp = 0x7777u;
    put16(SS0, 2u, 0x7000u); put16(SS0, 4u, 0x7001u);
    CHECK(pr_r0899_idle_0104_cpu(ram, RAM, &idle) == PR_R0899_IDLE_OK && idle.flags == 0x7293u && idle.sp == 6u);

    /* Guards change nothing. */
    idle.ip = 0x0105u;
    CHECK(pr_r0899_idle_0104_cpu(ram, RAM, &idle) == PR_R0899_IDLE_ARGUMENT && idle.sp == 6u);
    CHECK(pr_r0899_idle_0104_cpu(NULL, RAM, &idle) == PR_R0899_IDLE_ARGUMENT);
    CHECK(pr_system_set_expand_0d12(ram, RAM - 1u, &set) == PR_SYSTEM_LEAF_ARGUMENT);

    free(ram);
    printf("0768 pure callees: %u checks passed\n", checks);
    return 0;
}
