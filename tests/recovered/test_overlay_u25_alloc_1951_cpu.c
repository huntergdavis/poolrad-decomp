/* Focused synthetic test for raw-CPU U25 +1951, composing SYSTEM GetMem and U25 +06AF (docs/re/U25_ALLOC_1951_CPU.md).
 * No game bytes: only the two GetMem LCALL encodings are placed in the code segment; a synthetic Turbo Pascal heap, list
 * head and tables are placed in DS, and every result is predicted by hand. */
#include "poolrad/recovered/overlay_u25_alloc_1951_cpu.h"
#include "poolrad/recovered/system_getmem_023f_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM PR_U25_1951_RAM_SIZE
#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "u25 1951 line %d: %s\n", __LINE__, #x);              \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { CS = 0x8800, SYS = 0x0AF8, DS = 0x1800, SS = 0x2800, SP0 = 0x8000, BP1 = SP0 - 2, HEAP = 0x3000, RET_IP = 0x7000,
       RET_CS = 0x9001, BP0 = 0x7777, FLAGS0 = 0x0ED7, WORD0 = 0x4D2Bu };

static uint8_t *ram;
static unsigned checks;

static size_t at(unsigned seg, unsigned off) { return (((size_t)seg << 4) + (off & 0xFFFFu)) & (RAM - 1u); }
static void w8(unsigned seg, unsigned off, unsigned v) { ram[at(seg, off)] = (uint8_t)v; }
static void w16(unsigned seg, unsigned off, unsigned v) { w8(seg, off, v & 0xFFu); w8(seg, off + 1u, v >> 8); }
static unsigned r16(unsigned seg, unsigned off) { return ram[at(seg, off)] | (unsigned)ram[at(seg, off + 1u)] << 8; }

/* An empty free list with HeapPtr = 3400:0008 and HeapEnd = end_seg:0; the list head; tables; the frame with `index`. */
static pr_u25_1951_cpu world(unsigned head_seg, unsigned head_off, unsigned end_seg, unsigned index)
{
    pr_u25_1951_cpu cpu = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu, BP0, SP0, CS, 0x1951u, DS, 0x4444u, SS, FLAGS0};
    unsigned i;
    memset(ram, 0xA5, RAM);
    for (i = 0u; i < 2u; ++i) {
        unsigned site = i ? 0x198Fu : 0x1961u;
        w8(CS, site, 0x9Au); w16(CS, site + 1u, 0x023Fu); w16(CS, site + 3u, SYS);
    }
    w16(DS, 0x4404u, 8u); w16(DS, 0x4406u, HEAP + 0x400u); w16(DS, 0x4408u, 0u); w16(DS, 0x440Au, end_seg);
    w16(DS, 0x440Cu, 8u); w16(DS, 0x440Eu, HEAP + 0x400u); w16(DS, 0x4414u, 0x00A9u); w16(DS, 0x4416u, SYS);
    w16(DS, 0x6F70u, head_off); w16(DS, 0x6F72u, head_seg); w16(DS, 0x49EDu, WORD0);
    for (i = 0u; i < 256u; ++i) { w8(DS, 0x7045u + i, i ^ 0x5Au); }
    for (i = 0u; i < 256u; ++i) { w8(DS, 0x7085u + i, (i * 3u) & 0xFFu); }
    w16(SS, SP0, RET_IP); w16(SS, SP0 + 2u, RET_CS); w16(SS, SP0 + 4u, 0x5A00u | index);
    return cpu;
}

/* The table pair the routine reads last: 7085h (pushed first) and 7045h; +06AF returns (7045h byte << 8) + 7085h byte. */
static unsigned expected_word(unsigned index)
{
    return ((unsigned)ram[at(DS, 0x7045u + index)] << 8) + ram[at(DS, 0x7085u + index)];
}

static void pushes(void)
{
    pr_u25_1951_cpu cpu;
    pr_u25_1951_callee callee;
    int status;
    unsigned word;

    cpu = world(0u, 0u, HEAP + 0x1000u, 0x21u);                          /* an empty list: the first node, link nil */
    word = expected_word(0x21u);
    CHECK(pr_u25_alloc_1951_cpu(ram, RAM, &cpu, 64u, &callee, &status) == PR_U25_1951_OK);
    CHECK(callee == PR_U25_1951_WORD_06AF && status == 0);
    CHECK(cpu.cs == RET_CS && cpu.ip == RET_IP && cpu.sp == SP0 + 6u && cpu.bp == BP0 && cpu.ax == word && r16(DS, 0x49EDu) == word);
    CHECK(r16(DS, 0x6F70u) == 8u && r16(DS, 0x6F72u) == HEAP + 0x400u);  /* the node is the old HeapPtr 3400:0008 */
    CHECK(r16(HEAP + 0x400u, 8u) == WORD0 && r16(HEAP + 0x400u, 10u) == 0u && r16(HEAP + 0x400u, 12u) == 0u);
    CHECK(r16(DS, 0x4404u) == 0u && r16(DS, 0x4406u) == HEAP + 0x401u);  /* 6 bytes round to 8: 3401:0000 */
    CHECK(r16(DS, 0x6F74u) == 0xA5A5u);                                   /* the old-head scratch is untouched */

    cpu = world(0x5000u, 0x0123u, HEAP + 0x1000u, 0xFFu);                /* push onto 5000:0123 */
    word = expected_word(0xFFu);
    CHECK(pr_u25_alloc_1951_cpu(ram, RAM, &cpu, 64u, NULL, NULL) == PR_U25_1951_OK && cpu.ax == word);
    CHECK(r16(DS, 0x6F74u) == 0x0123u && r16(DS, 0x6F76u) == 0x5000u);
    CHECK(r16(HEAP + 0x400u, 8u) == WORD0 && r16(HEAP + 0x400u, 10u) == 0x0123u && r16(HEAP + 0x400u, 12u) == 0x5000u);
    CHECK(r16(DS, 0x6F70u) == 8u && r16(DS, 0x6F72u) == HEAP + 0x400u && r16(DS, 0x49EDu) == word);
}

static void stops(void)
{
    pr_u25_1951_cpu cpu;
    pr_u25_1951_callee callee;
    int status;

    cpu = world(0u, 0u, HEAP + 0x400u, 1u);                              /* HeapEnd at HeapPtr: GetMem's RunError 203 */
    CHECK(pr_u25_alloc_1951_cpu(ram, RAM, &cpu, 64u, &callee, &status) == PR_U25_1951_CALLEE);
    CHECK(callee == PR_U25_1951_GETMEM && status == (int)PR_SYSTEM_GETMEM_RUNERROR);
    CHECK(cpu.cs == SYS && cpu.ip == 0x00E2u && cpu.ax == 0x00CBu && cpu.sp == BP1 - 2u - 4u && cpu.bp == BP1);
    CHECK(r16(SS, cpu.sp) == 0x1966u && r16(SS, cpu.sp + 2u) == CS && r16(SS, cpu.sp + 4u) == 6u && r16(DS, 0x6F70u) == 0u);

    cpu = world(0x5000u, 0x0123u, HEAP + 0x1000u, 1u);                   /* a custom HeapError stops at its call */
    w16(DS, 0x4414u, 0x1234u);
    CHECK(pr_u25_alloc_1951_cpu(ram, RAM, &cpu, 64u, &callee, &status) == PR_U25_1951_CALLEE);
    CHECK(callee == PR_U25_1951_GETMEM && status == (int)PR_SYSTEM_GETMEM_HEAP_ERROR && cpu.ip == 0x03B7u);
    CHECK(r16(DS, 0x6F74u) == 0x0123u);

    cpu = world(0u, 0u, HEAP + 0x1000u, 1u);                             /* a rewritten site */
    w16(CS, 0x1962u, 0x0240u);
    CHECK(pr_u25_alloc_1951_cpu(ram, RAM, &cpu, 64u, &callee, &status) == PR_U25_1951_CODE);
    CHECK(callee == PR_U25_1951_NO_CALLEE && status == -1 && cpu.ip == 0x1961u && cpu.cs == CS && cpu.sp == BP1 - 2u);
    CHECK(r16(SS, cpu.sp) == 6u && (cpu.flags & 0x08D5u) == 0x0044u);     /* the size pushed; OR's flags of a nil head */
}

static void arguments(void)
{
    pr_u25_1951_cpu cpu, before;
    pr_u25_1951_callee callee = PR_U25_1951_GETMEM;
    int status = 99;
    cpu = world(0u, 0u, HEAP + 0x1000u, 1u);
    before = cpu;
    CHECK(pr_u25_alloc_1951_cpu(NULL, RAM, &cpu, 64u, &callee, &status) == PR_U25_1951_ARGUMENT);
    CHECK(callee == PR_U25_1951_NO_CALLEE && status == -1);
    CHECK(pr_u25_alloc_1951_cpu(ram, RAM, NULL, 64u, NULL, NULL) == PR_U25_1951_ARGUMENT);
    CHECK(pr_u25_alloc_1951_cpu(ram, RAM + 1u, &cpu, 64u, NULL, NULL) == PR_U25_1951_ARGUMENT);
    cpu.ip = 0x1952u;
    CHECK(pr_u25_alloc_1951_cpu(ram, RAM, &cpu, 64u, NULL, NULL) == PR_U25_1951_ARGUMENT);
    cpu.ip = before.ip;
    CHECK(memcmp(&cpu, &before, sizeof cpu) == 0 && r16(SS, SP0 - 2u) == 0xA5A5u);
}

int main(void)
{
    ram = malloc(RAM);
    CHECK(ram != NULL);
    pushes();
    stops();
    arguments();
    free(ram);
    printf("u25 +1951 cpu: %u checks passed\n", checks);
    return 0;
}
