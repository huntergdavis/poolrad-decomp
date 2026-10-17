#include "poolrad/recovered/overlay_u6a_record_weight_0000.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "u6A +0000 line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
#define RAM PR_U6A_0000_RAM_SIZE

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & (RAM - 1u);
}

static void put16(uint8_t *ram, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & (RAM - 1u)] = (uint8_t)(value >> 8);
}

static pr_u6a_0000_cpu entry(void)
{
    pr_u6a_0000_cpu cpu = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu,
                           0x7777u, 0x8000u, 0x3000u, 0x0000u, 0x1000u, 0x1111u,
                           0x2000u, 0x7202u};
    return cpu;
}

/* Record at 5000:0100 with low item bytes; table byte for index i is i. */
static void frame(uint8_t *ram, pr_u6a_0000_cpu *cpu, uint16_t head_off, uint16_t head_seg)
{
    unsigned i;
    memset(ram, 0xA5, RAM);
    put16(ram, cpu->ss, cpu->sp, 0x7000u);
    put16(ram, cpu->ss, (uint16_t)(cpu->sp + 2u), 0x7001u);
    put16(ram, cpu->ss, (uint16_t)(cpu->sp + 4u), 0x0100u);
    put16(ram, cpu->ss, (uint16_t)(cpu->sp + 6u), 0x5000u);
    for (i = 0u; i < 0x15u; ++i)
        ram[physical(0x5000u, (uint16_t)(0x0117u + i))] = (uint8_t)(0x10u + i);
    for (i = 0u; i < 128u; ++i)
        ram[physical(0x1000u, (uint16_t)(0x31FBu + 16u * i))] = (uint8_t)i;
    put16(ram, 0x5000u, 0x01C8u, head_off);
    put16(ram, 0x5000u, 0x01CAu, head_seg);
}

static void node(uint8_t *ram, uint16_t offset, uint8_t index, uint8_t table_value,
                 const uint8_t bytes[3], uint16_t next_off, uint16_t next_seg)
{
    ram[physical(0x6000u, (uint16_t)(offset + 0x2Eu))] = index;
    ram[physical(0x1000u, (uint16_t)(0x5582u + 16u * index))] = table_value;
    memcpy(ram + physical(0x6000u, (uint16_t)(offset + 0x3Cu)), bytes, 3u);
    put16(ram, 0x6000u, (uint16_t)(offset + 0x2Au), next_off);
    put16(ram, 0x6000u, (uint16_t)(offset + 0x2Cu), next_seg);
}

static void check_return(const pr_u6a_0000_cpu *cpu, uint16_t result, uint16_t bx, uint16_t cx)
{
    CHECK(cpu->ax == result && cpu->bx == bx && cpu->cx == cx && cpu->dx == 0u);
    CHECK(cpu->si == 0x9ABCu && cpu->di == 0x0100u && cpu->es == 0x5000u);
    CHECK(cpu->bp == 0x7777u && cpu->sp == 0x8008u);
    CHECK(cpu->ip == 0x7000u && cpu->cs == 0x7001u);
    CHECK(cpu->ds == 0x1000u && cpu->ss == 0x2000u);
}

int main(void)
{
    static const uint8_t mixed[3] = {0x81u, 0x7Fu, 0x80u | 0x7Au};
    uint8_t *ram = malloc(RAM);
    pr_u6a_0000_cpu cpu;
    CHECK(ram != NULL);

    /* Item 0 looks up 3, item 5 looks up 2; one true node adds 1 and 122. */
    cpu = entry();
    frame(ram, &cpu, 0x0000u, 0x6000u);
    ram[physical(0x5000u, 0x0117u)] = 0x83u;
    ram[physical(0x5000u, 0x011Cu)] = 0x82u;
    node(ram, 0x0000u, 5u, 12u, mixed, 0x0040u, 0x6000u);
    node(ram, 0x0040u, 6u, 3u, mixed, 0u, 0u);
    CHECK(pr_u6a_record_weight_0000(ram, RAM, &cpu) == PR_U6A_0000_OK);
    /* sum1 = 5, max1 = 3, sum2 = 123, max2 = 122, tier 6 -> 15*123 + 15*5 + 360. */
    check_return(&cpu, 1845u + 75u + 360u, 1845u, 75u);
    CHECK(ram[physical(0x5000u, 0x012Cu)] == 6u);
    CHECK(cpu.flags == 0x7206u); /* ADD 1920+1845: PF set, no carry, no sign */
    CHECK(memcmp(ram + physical(0x6000u, 0x3Cu), mixed, 3u) == 0); /* nodes are not modified */

    /* Nothing above 7F and an empty list: tier 0 and a zero result. */
    cpu = entry();
    frame(ram, &cpu, 0u, 0u);
    CHECK(pr_u6a_record_weight_0000(ram, RAM, &cpu) == PR_U6A_0000_OK);
    check_return(&cpu, 0u, 0u, 0u);
    CHECK(ram[physical(0x5000u, 0x012Cu)] == 0u);
    CHECK(cpu.flags == 0x7246u);

    /* Twenty-one lookups of 100 wrap the 8-bit sum to 52; tier 6 from the maximum. */
    cpu = entry();
    frame(ram, &cpu, 0u, 0u);
    memset(ram + physical(0x5000u, 0x0117u), 0x80u | 100u, 0x15u);
    CHECK(pr_u6a_record_weight_0000(ram, RAM, &cpu) == PR_U6A_0000_OK);
    check_return(&cpu, 780u + 360u, 0u, 780u);
    CHECK(ram[physical(0x5000u, 0x012Cu)] == 6u);

    /* Lookups of 1 and 2 only: tier 4 from a nonzero sum with maxima at most 2. */
    cpu = entry();
    frame(ram, &cpu, 0x0040u, 0x6000u);
    {
        static const uint8_t low[3] = {0x81u, 0x82u, 0x00u};
        node(ram, 0x0040u, 9u, 13u, low, 0u, 0u);
    }
    CHECK(pr_u6a_record_weight_0000(ram, RAM, &cpu) == PR_U6A_0000_OK);
    check_return(&cpu, 45u + 240u, 45u, 0u);
    CHECK(ram[physical(0x5000u, 0x012Cu)] == 4u);

    /* Host guards leave the state untouched. */
    {
        pr_u6a_0000_cpu before = entry(), after = before;
        CHECK(pr_u6a_record_weight_0000(NULL, RAM, &after) == PR_U6A_0000_ARGUMENT);
        CHECK(pr_u6a_record_weight_0000(ram, RAM - 1u, &after) == PR_U6A_0000_ARGUMENT);
        CHECK(pr_u6a_record_weight_0000(ram, RAM, NULL) == PR_U6A_0000_ARGUMENT);
        CHECK(memcmp(&before, &after, sizeof before) == 0);
    }
    free(ram);
    printf("u6A +0000: mixed, empty, sum-wrap, tier-4 and guard cases passed\n");
    return 0;
}
