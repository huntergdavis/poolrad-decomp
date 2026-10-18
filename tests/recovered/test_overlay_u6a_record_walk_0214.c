#include "poolrad/recovered/overlay_u6a_record_walk_0214.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "u6A +0214 line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
#define RAM (1u << 20)

static size_t at(uint16_t seg, uint16_t off) { return (((size_t)seg << 4) + off) & (RAM - 1u); }
static void w16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t v)
{
    ram[at(seg, off)] = (uint8_t)v;
    ram[at(seg, (uint16_t)(off + 1u))] = (uint8_t)(v >> 8);
}

/* Synthetic loaded-state metadata only: the +019C far-call site (9A 003E 0096 at +01C3) and the stub
 * at 0096:003E. Records sit at 5000:0100 + 200h*k; each has 21 item bytes at +17, the byte +2C, an empty
 * node list at +C8 and a far link at +104. */
static pr_u6a_0214_cpu enter(uint8_t *ram, unsigned count, int cyclic, int head)
{
    pr_u6a_0214_cpu cpu = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu,
                           0x7777u, 0x8000u, 0x3000u, 0x0214u, 0x1000u, 0x1111u, 0x2000u, 0x0ED7u};
    unsigned k, i;
    memset(ram, 0xA5, RAM);
    ram[at(0x3000u, 0x01C3u)] = 0x9Au; w16(ram, 0x3000u, 0x01C4u, 0x003Eu); w16(ram, 0x3000u, 0x01C6u, 0x0096u);
    ram[at(0x0096u, 0x003Eu)] = 0xEAu; w16(ram, 0x0096u, 0x003Fu, 0x323Du); w16(ram, 0x0096u, 0x0041u, 0x4000u);
    for (k = 0; k < count; ++k) {
        uint16_t off = (uint16_t)(0x0100u + 0x200u * k), next = (uint16_t)(off + 0x200u);
        for (i = 0; i < 21u; ++i)
            ram[at(0x5000u, (uint16_t)(off + 0x17u + i))] = (uint8_t)(i & 1u ? 0x80u | (k * 21u + i) : (k * 21u + i) & 0x7Fu);
        ram[at(0x5000u, (uint16_t)(off + 0x2Cu))] = 0x99u;
        w16(ram, 0x5000u, (uint16_t)(off + 0xC8u), 0u); w16(ram, 0x5000u, (uint16_t)(off + 0xCAu), 0u);
        if (k + 1u < count) { w16(ram, 0x5000u, (uint16_t)(off + 0x104u), next); w16(ram, 0x5000u, (uint16_t)(off + 0x106u), 0x5000u); }
        else if (cyclic) { w16(ram, 0x5000u, (uint16_t)(off + 0x104u), 0x0100u); w16(ram, 0x5000u, (uint16_t)(off + 0x106u), 0x5000u); }
        else { w16(ram, 0x5000u, (uint16_t)(off + 0x104u), 0u); w16(ram, 0x5000u, (uint16_t)(off + 0x106u), 0u); }
    }
    if (count == 0u || head < 0) { w16(ram, 0x1000u, 0x5D96u, 0u); w16(ram, 0x1000u, 0x5D98u, 0u); }
    else { w16(ram, 0x1000u, 0x5D96u, (uint16_t)(0x0100u + 0x200u * (unsigned)head)); w16(ram, 0x1000u, 0x5D98u, 0x5000u); }
    w16(ram, 0x2000u, 0x8000u, 0x7000u); w16(ram, 0x2000u, 0x8002u, 0x7001u);
    return cpu;
}

static int cleared(const uint8_t *ram, unsigned k)
{
    uint16_t off = (uint16_t)(0x0100u + 0x200u * k);
    unsigned i;
    for (i = 0; i < 21u; ++i)
        if (ram[at(0x5000u, (uint16_t)(off + 0x17u + i))] >= 0x80u) return 0;
    return ram[at(0x5000u, (uint16_t)(off + 0x2Cu))] == 0u;
}

int main(void)
{
    uint8_t *ram = malloc(RAM);
    pr_u6a_0214_cpu cpu;
    int callee = 0;
    CHECK(ram != NULL);

    /* Three linked records: +0159 clears every record's high item bytes and +2C; the walk ends on the
     * null link with OR's flags (ZF and PF) and AX = 0. */
    cpu = enter(ram, 3u, 0, 0);
    CHECK(pr_u6a_record_walk_0214(ram, RAM, &cpu, 16u, &callee) == PR_U6A_0214_OK);
    CHECK(cleared(ram, 0) && cleared(ram, 1) && cleared(ram, 2));
    CHECK(ram[at(0x5000u, 0x0100u + 0x17u)] == 0x00u && ram[at(0x5000u, 0x0300u + 0x18u)] == 0x00u);
    CHECK(ram[at(0x5000u, 0x0100u + 0x19u)] == 2u && ram[at(0x5000u, 0x0300u + 0x17u)] == (21u & 0x7Fu));   /* low bytes keep */
    CHECK(cpu.ax == 0u && (cpu.flags & 0x08D5u) == 0x0044u);
    CHECK(cpu.ip == 0x7000u && cpu.cs == 0x7001u && cpu.sp == 0x8004u && cpu.bp == 0x7777u);

    /* A list headed at the second record leaves the first record untouched. */
    cpu = enter(ram, 3u, 0, 1);
    CHECK(pr_u6a_record_walk_0214(ram, RAM, &cpu, 16u, &callee) == PR_U6A_0214_OK);
    CHECK(!cleared(ram, 0) && cleared(ram, 1) && cleared(ram, 2));

    /* An empty list: nothing changes, AX = 0 with the null pointer's flags. */
    cpu = enter(ram, 2u, 0, -1);
    CHECK(pr_u6a_record_walk_0214(ram, RAM, &cpu, 16u, &callee) == PR_U6A_0214_OK);
    CHECK(!cleared(ram, 0) && !cleared(ram, 1) && cpu.ax == 0u && (cpu.flags & 0x08D5u) == 0x0044u);

    /* A cyclic record list never returns in DOS: the cap stops the walk at the loop head. */
    cpu = enter(ram, 2u, 1, 0);
    CHECK(pr_u6a_record_walk_0214(ram, RAM, &cpu, 8u, &callee) == PR_U6A_0214_LIMIT);
    CHECK(cpu.ip == 0x0227u && cpu.cs == 0x3000u && cleared(ram, 0) && cleared(ram, 1));

    {
        pr_u6a_0214_cpu before = enter(ram, 1u, 0, 0), after = before;
        CHECK(pr_u6a_record_walk_0214(NULL, RAM, &after, 8u, NULL) == PR_U6A_0214_ARGUMENT);
        CHECK(pr_u6a_record_walk_0214(ram, RAM - 1u, &after, 8u, NULL) == PR_U6A_0214_ARGUMENT);
        CHECK(pr_u6a_record_walk_0214(ram, RAM, NULL, 8u, NULL) == PR_U6A_0214_ARGUMENT);
        CHECK(memcmp(&before, &after, sizeof before) == 0);
    }
    free(ram);
    printf("u6A +0214: list, partial, empty, cyclic-cap and guard cases passed\n");
    return 0;
}
