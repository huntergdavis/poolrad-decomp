#include "poolrad/recovered/overlay_u6a_link_high_clear_019c.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "u6A +019C line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
#define RAM PR_U6A_019C_RAM_SIZE

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

static pr_u6a_019c_cpu entry(void)
{
    pr_u6a_019c_cpu cpu = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu,
                           0x7777u, 0x8000u, 0x3000u, 0x019Cu, 0x1000u, 0x1111u,
                           0x2000u, 0x7202u};
    return cpu;
}

/* Record at 5000:0100; nodes at 6000:0000 and 6000:0040. */
static void frame(uint8_t *ram, pr_u6a_019c_cpu *cpu, uint16_t head_off, uint16_t head_seg)
{
    memset(ram, 0xA5, RAM);
    put16(ram, cpu->ss, cpu->sp, 0x7000u);
    put16(ram, cpu->ss, (uint16_t)(cpu->sp + 2u), 0x7001u);
    put16(ram, cpu->ss, (uint16_t)(cpu->sp + 4u), 0x0100u);
    put16(ram, cpu->ss, (uint16_t)(cpu->sp + 6u), 0x5000u);
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

static void check_return(const pr_u6a_019c_cpu *cpu)
{
    CHECK(cpu->ax == 0u && cpu->dx == 0u);
    CHECK(cpu->bx == 0x3456u && cpu->si == 0x9ABCu);
    CHECK(cpu->bp == 0x7777u && cpu->sp == 0x8008u);
    CHECK(cpu->ip == 0x7000u && cpu->cs == 0x7001u);
    CHECK(cpu->ds == 0x1000u && cpu->ss == 0x2000u);
    CHECK(cpu->flags == 0x7246u); /* ZF and PF from OR AX,AX; AF represented as zero */
}

int main(void)
{
    static const uint8_t first[3] = {0x81u, 0x7Fu, 0xFFu};
    static const uint8_t second[3] = {0x80u, 0x90u, 0xA0u};
    static const uint8_t third[3] = {0xFEu, 0x01u, 0x80u};
    uint8_t *ram = malloc(RAM);
    pr_u6a_019c_cpu cpu;
    CHECK(ram != NULL);

    /* Three nodes: predicate true, false, true. */
    cpu = entry();
    frame(ram, &cpu, 0x0000u, 0x6000u);
    node(ram, 0x0000u, 5u, 12u, first, 0x0040u, 0x6000u);
    node(ram, 0x0040u, 6u, 3u, second, 0x0080u, 0x6000u);
    node(ram, 0x0080u, 7u, 11u, third, 0u, 0u);
    CHECK(pr_u6a_link_high_clear_019c(ram, RAM, &cpu) == PR_U6A_019C_OK);
    CHECK(ram[physical(0x6000u, 0x3Cu)] == 0x01u && ram[physical(0x6000u, 0x3Du)] == 0x7Fu &&
          ram[physical(0x6000u, 0x3Eu)] == 0x7Fu);
    CHECK(memcmp(ram + physical(0x6000u, 0x7Cu), second, 3u) == 0);
    CHECK(ram[physical(0x6000u, 0xBCu)] == 0x7Eu && ram[physical(0x6000u, 0xBDu)] == 0x01u &&
          ram[physical(0x6000u, 0xBEu)] == 0x00u);
    CHECK(ram[physical(0x6000u, 0x3Bu)] == 0xA5u && ram[physical(0x6000u, 0x3Fu)] == 0xA5u);
    check_return(&cpu);
    CHECK(cpu.di == 0x0080u && cpu.es == 0x6000u); /* last LES before the null link */
    CHECK(cpu.cx == 0x5604u);                          /* CL := 4 inside the predicate */
    CHECK(ram[physical(0x2000u, 0x7FFDu)] == 3u);      /* final index local [BP-1] */

    /* Empty list: no call, ES:DI is the record pointer, CX untouched. */
    cpu = entry();
    frame(ram, &cpu, 0u, 0u);
    CHECK(pr_u6a_link_high_clear_019c(ram, RAM, &cpu) == PR_U6A_019C_OK);
    check_return(&cpu);
    CHECK(cpu.di == 0x0100u && cpu.es == 0x5000u && cpu.cx == 0x5678u);
    CHECK(ram[physical(0x2000u, 0x7FFDu)] == 0xA5u);

    /* Only the offset word nonzero still walks; predicate false leaves bytes. */
    cpu = entry();
    frame(ram, &cpu, 0x0040u, 0u);
    ram[physical(0u, 0x6Eu)] = 9u;
    ram[physical(0x1000u, 0x5582u + 16u * 9u)] = 14u;
    memcpy(ram + physical(0u, 0x7Cu), first, 3u);
    put16(ram, 0u, 0x6Au, 0u);
    put16(ram, 0u, 0x6Cu, 0u);
    CHECK(pr_u6a_link_high_clear_019c(ram, RAM, &cpu) == PR_U6A_019C_OK);
    CHECK(memcmp(ram + physical(0u, 0x7Cu), first, 3u) == 0);
    check_return(&cpu);

    /* Host guards leave the state untouched. */
    {
        pr_u6a_019c_cpu before = entry(), after = before;
        CHECK(pr_u6a_link_high_clear_019c(NULL, RAM, &after) == PR_U6A_019C_ARGUMENT);
        CHECK(pr_u6a_link_high_clear_019c(ram, RAM - 1u, &after) == PR_U6A_019C_ARGUMENT);
        CHECK(pr_u6a_link_high_clear_019c(ram, RAM, NULL) == PR_U6A_019C_ARGUMENT);
        CHECK(memcmp(&before, &after, sizeof before) == 0);
    }
    free(ram);
    printf("u6A +019C: three-node, empty, offset-only and guard cases passed\n");
    return 0;
}
