/* Focused synthetic test for raw-CPU U9E +0E52, entry 10, composing SYSTEM GetMem (docs/re/U9E_LIST_APPEND_0E52_CPU.md).
 * No game bytes beyond the five LCALL bytes at +0E5C: a synthetic heap, record and lists, every result predicted by hand. */
#include "poolrad/recovered/overlay_u9e_list_append_0e52_cpu.h"
#include "poolrad/recovered/system_getmem_023f_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM PR_U9E_0E52_RAM_SIZE
#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "u9e 0e52 line %d: %s\n", __LINE__, #x);              \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { CS0 = 0x8800, DS0 = 0x1800, SS0 = 0x2800, SP0 = 0x6000, BP0 = 0x7777, HEAP = 0x3000, REC_SEG = 0x4100, REC_OFF = 0x0020,
       RET_IP = 0x7000, RET_CS = 0x9001, FLAGS0 = 0x0602, A = 0xBEEF, B = 0x3355, C = 0x2233, D = 0x1144 };

static uint8_t *ram;
static unsigned checks;

static size_t at(unsigned seg, unsigned off) { return (((size_t)seg << 4) + (off & 0xFFFFu)) & (RAM - 1u); }
static unsigned get8(unsigned seg, unsigned off) { return ram[at(seg, off)]; }
static unsigned get16(unsigned seg, unsigned off) { return ram[at(seg, off)] | ((unsigned)ram[at(seg, off + 1u)] << 8); }
static void put16(unsigned seg, unsigned off, unsigned value)
{
    ram[at(seg, off)] = (uint8_t)value;
    ram[at(seg, off + 1u)] = (uint8_t)(value >> 8);
}

/* A fresh world: the GetMem LCALL at +0E5C, no free blocks (FreeList = HeapPtr = 3000:0000), HeapEnd end:0000, the
 * default HeapError, a nil head at the record's +7Fh, and the far-call frame d, c, a, b, record at SS:SP. */
static pr_u9e_0e52_cpu world(unsigned ss, unsigned sp, unsigned end, unsigned record_off)
{
    static const uint8_t lcall[5] = {0x9A, 0x3F, 0x02, 0xF8, 0x0A};
    pr_u9e_0e52_cpu cpu;
    memset(ram, 0xA5, RAM);
    memcpy(ram + at(CS0, 0x0E5C), lcall, sizeof lcall);
    put16(DS0, 0x4400, 0); put16(DS0, 0x4402, HEAP);
    put16(DS0, 0x4404, 0); put16(DS0, 0x4406, HEAP);
    put16(DS0, 0x4408, 0); put16(DS0, 0x440A, end);
    put16(DS0, 0x440C, 0); put16(DS0, 0x440E, HEAP);
    put16(DS0, 0x4414, 0x00A9); put16(DS0, 0x4416, 0x0AF8);
    put16(REC_SEG, record_off + 0x7Fu, 0); put16(REC_SEG, record_off + 0x81u, 0);
    put16(ss, sp, RET_IP); put16(ss, sp + 2u, RET_CS);
    put16(ss, sp + 4u, D); put16(ss, sp + 6u, C); put16(ss, sp + 8u, A); put16(ss, sp + 10u, B);
    put16(ss, sp + 12u, record_off); put16(ss, sp + 14u, REC_SEG);
    memset(&cpu, 0, sizeof cpu);
    cpu.ax = 0x1111; cpu.bx = 0x2222; cpu.cx = 0x3333; cpu.dx = 0x4444; cpu.si = 0x5555; cpu.di = 0x6666; cpu.bp = BP0;
    cpu.sp = (uint16_t)sp; cpu.cs = CS0; cpu.ip = 0x0E52; cpu.ds = DS0; cpu.es = 0x9999; cpu.ss = (uint16_t)ss; cpu.flags = FLAGS0;
    return cpu;
}

/* The head := nodes[0]; each node's link (+5/+7) := the next, the last's := nil or, when cyclic, nodes[0]. */
static void chain(const unsigned nodes[][2], size_t count, int cyclic)
{
    size_t i;
    put16(REC_SEG, REC_OFF + 0x7Fu, nodes[0][1]); put16(REC_SEG, REC_OFF + 0x81u, nodes[0][0]);
    for (i = 0; i < count; ++i) {
        unsigned seg = i + 1 < count ? nodes[i + 1][0] : cyclic ? nodes[0][0] : 0u;
        unsigned off = i + 1 < count ? nodes[i + 1][1] : cyclic ? nodes[0][1] : 0u;
        put16(nodes[i][0], nodes[i][1] + 5u, off); put16(nodes[i][0], nodes[i][1] + 7u, seg);
    }
}

/* GetMem(9) took 16 bytes at the heap top 3000:0000; the node holds b, a, c, d and a nil link; the routine returned. */
static void appended(const pr_u9e_0e52_cpu *cpu, unsigned sp)
{
    CHECK(get16(DS0, 0x4404) == 0 && get16(DS0, 0x4406) == HEAP + 1u && get16(DS0, 0x440C) == 0 && get16(DS0, 0x440E) == HEAP + 1u);
    CHECK(get8(HEAP, 0) == (B & 0xFFu) && get16(HEAP, 1) == A && get8(HEAP, 3) == (C & 0xFFu) && get8(HEAP, 4) == (D & 0xFFu));
    CHECK(get16(HEAP, 5) == 0 && get16(HEAP, 7) == 0);
    CHECK(cpu->ip == RET_IP && cpu->cs == RET_CS && cpu->sp == (uint16_t)(sp + 16u) && cpu->bp == BP0);
    CHECK(cpu->ax == ((A & 0xFF00u) | (C & 0xFFu)) && cpu->dx == HEAP && cpu->es == HEAP && cpu->di == 0);
    CHECK(cpu->ds == DS0 && cpu->ss == SS0 && cpu->flags == ((FLAGS0 & ~0x08D5u) | 0x0044u));
}

int main(void)
{
    static const unsigned three[3][2] = {{0x5100, 0x0000}, {0x5200, 0x0008}, {0x5300, 0x0004}};
    static const unsigned two[2][2] = {{0x5100, 0x0000}, {0x5200, 0x0008}};
    pr_u9e_0e52_cpu cpu, before;
    int getmem = 99;
    ram = malloc(RAM);
    if (ram == NULL)
        return 1;

    /* An empty list: the head := the node; [BP-4]/[BP-2] hold it and [BP-8]/[BP-6] stay untouched. */
    cpu = world(SS0, SP0, HEAP + 0x100u, REC_OFF);
    CHECK(pr_u9e_list_append_0e52_cpu(ram, RAM, &cpu, 64, 64, &getmem) == PR_U9E_0E52_OK && getmem == PR_SYSTEM_GETMEM_RETURNED);
    appended(&cpu, SP0);
    CHECK(get16(REC_SEG, REC_OFF + 0x7Fu) == 0 && get16(REC_SEG, REC_OFF + 0x81u) == HEAP);
    CHECK(get16(SS0, SP0 - 2u) == BP0 && get16(SS0, SP0 - 6u) == 0 && get16(SS0, SP0 - 4u) == HEAP);
    CHECK(get16(SS0, SP0 - 10u) == 0xA5A5u && get16(SS0, SP0 - 8u) == 0xA5A5u);
    CHECK(get16(SS0, SP0 - 12u) == 9 && get16(SS0, SP0 - 16u) == 0x0E61u && get16(SS0, SP0 - 14u) == CS0);

    /* Three nodes: the walk ends at the third, whose link := the node; the head and the other links stay. */
    cpu = world(SS0, SP0, HEAP + 0x100u, REC_OFF);
    chain(three, 3, 0);
    CHECK(pr_u9e_list_append_0e52_cpu(ram, RAM, &cpu, 64, 64, &getmem) == PR_U9E_0E52_OK);
    appended(&cpu, SP0);
    CHECK(get16(REC_SEG, REC_OFF + 0x7Fu) == 0x0000 && get16(REC_SEG, REC_OFF + 0x81u) == 0x5100);
    CHECK(get16(0x5100, 5) == 0x0008 && get16(0x5100, 7) == 0x5200 && get16(0x5200, 13) == 0x0004 && get16(0x5200, 15) == 0x5300);
    CHECK(get16(0x5300, 9) == 0x0000 && get16(0x5300, 11) == HEAP);
    CHECK(get16(SS0, SP0 - 10u) == 0x0004 && get16(SS0, SP0 - 8u) == 0x5300);

    /* A two-node cycle with max_links 3: after three links the cursor is the second node and its link is not nil. */
    cpu = world(SS0, SP0, HEAP + 0x100u, REC_OFF);
    chain(two, 2, 1);
    CHECK(pr_u9e_list_append_0e52_cpu(ram, RAM, &cpu, 64, 3, &getmem) == PR_U9E_0E52_LIMIT && getmem == PR_SYSTEM_GETMEM_RETURNED);
    CHECK(cpu.ip == 0x0EA8 && cpu.cs == CS0 && cpu.es == 0x5200 && cpu.di == 0x0008 && cpu.ax == 0x5100);
    CHECK(cpu.sp == SP0 - 10u && cpu.bp == SP0 - 2u && cpu.flags == ((FLAGS0 & ~0x08D5u) | 0x0004u));
    CHECK(get16(SS0, SP0 - 10u) == 0x0008 && get16(SS0, SP0 - 8u) == 0x5200 && get16(HEAP, 5) == 0xA5A5u);

    /* A full heap (HeapEnd = HeapPtr): the default HeapError fails and GetMem stops at RunError 203. */
    cpu = world(SS0, SP0, HEAP, REC_OFF);
    CHECK(pr_u9e_list_append_0e52_cpu(ram, RAM, &cpu, 64, 64, &getmem) == PR_U9E_0E52_CALLEE && getmem == PR_SYSTEM_GETMEM_RUNERROR);
    CHECK(cpu.cs == 0x0AF8 && cpu.ip == 0x00E2 && cpu.ax == 203);
    CHECK(get16(REC_SEG, REC_OFF + 0x81u) == 0 && get16(DS0, 0x4406) == HEAP);

    /* CODE: the LCALL opcode changed; stopped at +0E5C with the size pushed and SUB SP,8's flags (5FFE - 8: only PF). */
    cpu = world(SS0, SP0, HEAP + 0x100u, REC_OFF);
    ram[at(CS0, 0x0E5C)] = 0xEA;
    CHECK(pr_u9e_list_append_0e52_cpu(ram, RAM, &cpu, 64, 64, &getmem) == PR_U9E_0E52_CODE && getmem == -1);
    CHECK(cpu.ip == 0x0E5C && cpu.cs == CS0 && cpu.sp == SP0 - 12u && cpu.bp == SP0 - 2u && cpu.ax == 9);
    CHECK(get16(SS0, SP0 - 12u) == 9 && cpu.flags == ((FLAGS0 & ~0x08D5u) | 0x0004u));

    /* CODE with SP = 6: BP = 4 and SUB SP,8 borrows to FFFCh (CF, PF, AF, SF); the size lands at SS:FFFAh. */
    cpu = world(0x1000, 0x0006, HEAP + 0x100u, REC_OFF);
    ram[at(CS0, 0x0E5D)] = 0x40;
    CHECK(pr_u9e_list_append_0e52_cpu(ram, RAM, &cpu, 64, 64, &getmem) == PR_U9E_0E52_CODE);
    CHECK(cpu.bp == 0x0004 && cpu.sp == 0xFFFA && get16(0x1000, 0xFFFA) == 9 && cpu.flags == ((FLAGS0 & ~0x08D5u) | 0x0095u));

    /* A record at offset FFC0h: the head words wrap to +003Fh/+0041h. */
    cpu = world(SS0, SP0, HEAP + 0x100u, 0xFFC0);
    CHECK(pr_u9e_list_append_0e52_cpu(ram, RAM, &cpu, 64, 64, &getmem) == PR_U9E_0E52_OK);
    CHECK(get16(REC_SEG, 0x003F) == 0 && get16(REC_SEG, 0x0041) == HEAP);
    appended(&cpu, SP0);

    /* Guards: nothing changes. */
    cpu = world(SS0, SP0, HEAP + 0x100u, REC_OFF);
    cpu.ip = 0x0E53;
    before = cpu;
    getmem = 99;
    CHECK(pr_u9e_list_append_0e52_cpu(ram, RAM, &cpu, 64, 64, &getmem) == PR_U9E_0E52_ARGUMENT && getmem == -1);
    CHECK(memcmp(&cpu, &before, sizeof cpu) == 0 && get16(SS0, SP0 - 2u) == 0xA5A5u);
    cpu.ip = 0x0E52;
    CHECK(pr_u9e_list_append_0e52_cpu(NULL, RAM, &cpu, 64, 64, NULL) == PR_U9E_0E52_ARGUMENT);
    CHECK(pr_u9e_list_append_0e52_cpu(ram, RAM - 1u, &cpu, 64, 64, NULL) == PR_U9E_0E52_ARGUMENT);
    CHECK(pr_u9e_list_append_0e52_cpu(ram, RAM, NULL, 64, 64, NULL) == PR_U9E_0E52_ARGUMENT);

    free(ram);
    printf("u9e +0e52: %u checks passed\n", checks);
    return 0;
}
