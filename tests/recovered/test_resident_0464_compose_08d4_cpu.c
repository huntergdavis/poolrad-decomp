/* Focused synthetic test for the raw-CPU resident 0464:08D4 tile compositor (docs/re/RES_0464_COMPOSE_08D4_CPU.md).
 * No game bytes: only the two relocated far-call segment words, synthetic objects and metadata. */
#include "poolrad/recovered/resident_0464_compose_08d4_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "0464 08d4 line %d: %s\n", __LINE__, #x);             \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { RAM = 1u << 20, CS0 = 0x8800, DS0 = 0x5000, SS0 = 0x2000, SP0 = 0x8000, SRC = 0x3000, DST = 0x4000, SYS = 0x6000 };

static uint8_t *ram;
static unsigned checks;

static size_t at(uint16_t seg, uint16_t off) { return (((size_t)seg << 4) + off) & (RAM - 1u); }
static void put16(uint16_t seg, uint16_t off, uint16_t v) { ram[at(seg, off)] = (uint8_t)v; ram[at(seg, (uint16_t)(off + 1u))] = (uint8_t)(v >> 8); }
static uint16_t get16(uint16_t seg, uint16_t off) { return (uint16_t)(ram[at(seg, off)] | (ram[at(seg, (uint16_t)(off + 1u))] << 8)); }

/* Source tile set at SRC:0100 (rows, width 1, tile size 4, no aux), destination object at DST:0200 (width 4). */
static pr_r0464_compose_cpu frame(uint8_t mode, uint16_t rows, uint16_t flag)
{
    static const uint16_t words[10] = {0x7000u, 0x9001u, 0x0200u, DST, 0x0100u, SRC, 0, 0, 0, 1};
    pr_r0464_compose_cpu c = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu, 0x7777u, SP0, CS0, 0x08D4u, DS0, 0x1111u, SS0, 0x0002u};
    unsigned k;
    memset(ram, 0x5A, RAM);
    put16(CS0, 0x0B33u, SYS); put16(CS0, 0x0D2Du, SYS);
    for (k = 0; k < 10; ++k)
        put16(SS0, (uint16_t)(SP0 + 2u * k), k == 6 ? flag : words[k]);
    ram[at(DS0, 0x4A1Eu)] = mode;
    for (k = 0; k < 0xA8u; ++k) {
        ram[at(DS0, (uint16_t)(0x4A28u + k))] = 0;
        put16(DS0, (uint16_t)(0x4AD0u + 2u * k), 0xEEEEu);
        put16(DS0, (uint16_t)(0x4C20u + 2u * k), 0xFFFFu);
        put16(DS0, (uint16_t)(0x4D70u + 2u * k), 0x0000u);
    }
    put16(SRC, 0x0100u, rows); put16(SRC, 0x0102u, 1u); put16(SRC, 0x0111u, 4u); put16(SRC, 0x0113u, 0u); put16(SRC, 0x0115u, 0u);
    for (k = 0; k < 16u; ++k)
        ram[at(SRC, (uint16_t)(0x0117u + k))] = (uint8_t)(0xA0u + k);
    put16(DST, 0x0202u, 4u);
    return c;
}

int main(void)
{
    pr_bus bus;
    pr_r0464_compose_cpu c;
    pr_r0464_compose_cpu_diagnostic d;
    ram = malloc(RAM);
    if (ram == NULL)
        return 1;
    memset(&bus, 0, sizeof bus);
    bus.ram = ram;

    /* Mode 0, Move path (flag 0), one row at (x 1, y 0): the row is marked dirty, the extents become 2 and 3, the two
     * tile bytes land at destination +17h+2, and RETF 10h pops the far return and eight words. */
    c = frame(0, 1, 0);
    CHECK(pr_r0464_compose_08d4_cpu(&bus, RAM, &c, 1000, &d) == PR_R0464_COMPOSE_CPU_OK && d.move_calls == 1 && d.move_status == 0);
    CHECK(ram[at(DS0, 0x4A28u)] == 1 && get16(DS0, 0x4AD0u) == 2u && get16(DS0, 0x4C20u) == 2u && get16(DS0, 0x4D70u) == 3u);
    CHECK(ram[at(DST, 0x0219u)] == 0xA0 && ram[at(DST, 0x021Au)] == 0xA1 && ram[at(DST, 0x0218u)] == 0x5A && ram[at(DST, 0x021Bu)] == 0x5A);
    CHECK(ram[at(DS0, 0x4A29u)] == 0 && c.cs == 0x9001u && c.ip == 0x7000u && c.sp == SP0 + 20u && c.bp == 0x7777u && c.ds == DS0);

    /* An unknown video state (3) returns with no metadata or destination change. */
    c = frame(3, 1, 0);
    CHECK(pr_r0464_compose_08d4_cpu(&bus, RAM, &c, 1000, &d) == PR_R0464_COMPOSE_CPU_OK && d.move_calls == 0);
    CHECK(ram[at(DS0, 0x4A28u)] == 0 && get16(DS0, 0x4C20u) == 0xFFFFu && ram[at(DST, 0x0219u)] == 0x5A && c.sp == SP0 + 20u);

    /* Zero rows means 65,536 iterations: a budget of 2 stops with LIMIT before the third loop-back. */
    c = frame(0, 0, 0);
    CHECK(pr_r0464_compose_08d4_cpu(&bus, RAM, &c, 2, &d) == PR_R0464_COMPOSE_CPU_LIMIT && d.move_calls == 3);

    /* Guards change nothing. */
    c = frame(0, 1, 0);
    c.ip = 0x08D5u;
    CHECK(pr_r0464_compose_08d4_cpu(&bus, RAM, &c, 1000, &d) == PR_R0464_COMPOSE_CPU_ARGUMENT && c.sp == SP0);
    CHECK(pr_r0464_compose_08d4_cpu(NULL, RAM, &c, 1000, &d) == PR_R0464_COMPOSE_CPU_ARGUMENT);
    c.ip = 0x08D4u;
    CHECK(pr_r0464_compose_08d4_cpu(&bus, RAM - 1u, &c, 1000, &d) == PR_R0464_COMPOSE_CPU_ARGUMENT);

    free(ram);
    printf("0464 08d4: %u checks passed\n", checks);
    return 0;
}
