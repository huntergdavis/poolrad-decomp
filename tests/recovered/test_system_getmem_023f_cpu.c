/* Focused synthetic test for raw-CPU SYSTEM GetMem 0AF8:023F (docs/re/SYSTEM_GETMEM_023F.md). No game bytes: the model
 * needs no code bytes; synthetic Turbo Pascal heap variables and free blocks are placed in RAM and every result is
 * predicted by hand. */
#include "poolrad/recovered/system_getmem_023f_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM PR_SYSTEM_GETMEM_RAM_SIZE
#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "getmem line %d: %s\n", __LINE__, #x);                \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { CS = 0x0AF8, DS = 0x1800, SS = 0x2800, SP0 = 0x8000, HEAP = 0x3000, RET_IP = 0x7000, RET_CS = 0x9001, BP0 = 0x7777,
       FLAGS0 = 0x0ED7, KEPT = 0x0602 };

typedef struct { unsigned seg, off, paragraphs, granule; } block;

static uint8_t *ram;
static unsigned checks;

static size_t at(unsigned seg, unsigned off) { return (((size_t)seg << 4) + (off & 0xFFFFu)) & (RAM - 1u); }
static void w8(unsigned seg, unsigned off, unsigned v) { ram[at(seg, off)] = (uint8_t)v; }
static void w16(unsigned seg, unsigned off, unsigned v) { w8(seg, off, v & 0xFFu); w8(seg, off + 1u, v >> 8); }
static unsigned r16(unsigned seg, unsigned off) { return ram[at(seg, off)] | (unsigned)ram[at(seg, off + 1u)] << 8; }

/* Blocks chained from FreeList to HeapPtr = top_seg:top_off; HeapEnd = end_seg:end_off; the default HeapError. */
static pr_system_getmem_cpu world(unsigned size, const block *list, unsigned count, unsigned top_seg, unsigned top_off, unsigned end_seg,
                                  unsigned end_off)
{
    pr_system_getmem_cpu cpu = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu, BP0, SP0, CS, 0x023Fu, DS, 0x4444u, SS, FLAGS0};
    unsigned i;
    memset(ram, 0xA5, RAM);
    w16(DS, 0x4404u, top_off); w16(DS, 0x4406u, top_seg); w16(DS, 0x4408u, end_off); w16(DS, 0x440Au, end_seg);
    w16(DS, 0x4414u, 0x00A9u); w16(DS, 0x4416u, CS);
    w16(DS, 0x440Cu, count ? list[0].off : top_off); w16(DS, 0x440Eu, count ? list[0].seg : top_seg);
    for (i = 0u; i < count; ++i) {
        w16(list[i].seg, list[i].off, i + 1u < count ? list[i + 1u].off : top_off);
        w16(list[i].seg, list[i].off + 2u, i + 1u < count ? list[i + 1u].seg : top_seg);
        w16(list[i].seg, list[i].off + 4u, list[i].granule); w16(list[i].seg, list[i].off + 6u, list[i].paragraphs);
    }
    w16(SS, SP0, RET_IP); w16(SS, SP0 + 2u, RET_CS); w16(SS, SP0 + 4u, size);
    return cpu;
}

static void returned(const pr_system_getmem_cpu *cpu, unsigned seg, unsigned off)
{
    CHECK(cpu->cs == RET_CS && cpu->ip == RET_IP && cpu->sp == SP0 + 6u && cpu->bp == BP0 && cpu->dx == seg && cpu->ax == off);
    CHECK((cpu->flags & 0x0001u) == 0u && (cpu->flags & 0x0700u) == (FLAGS0 & 0x0700u));
}

static void allocations(void)
{
    static const block two[2] = {{HEAP + 0x10u, 0u, 2u, 0u}, {HEAP + 0x20u, 8u, 4u, 8u}};
    pr_system_getmem_cpu cpu;

    cpu = world(0u, two, 2u, HEAP + 0x400u, 0u, HEAP + 0x1000u, 0u);     /* size 0: nil, XOR's flags */
    CHECK(pr_system_getmem_023f(ram, RAM, &cpu, 64u) == PR_SYSTEM_GETMEM_RETURNED);
    returned(&cpu, 0u, 0u);
    CHECK(cpu.flags == (KEPT | 0x0044u) && r16(DS, 0x440Cu) == 0u && r16(DS, 0x440Eu) == HEAP + 0x10u);

    cpu = world(32u, two, 2u, HEAP + 0x400u, 0u, HEAP + 0x1000u, 0u);    /* 32 bytes = 2 paragraphs: block 1 exactly */
    CHECK(pr_system_getmem_023f(ram, RAM, &cpu, 64u) == PR_SYSTEM_GETMEM_RETURNED);
    returned(&cpu, HEAP + 0x10u, 0u);
    CHECK(r16(DS, 0x440Cu) == 8u && r16(DS, 0x440Eu) == HEAP + 0x20u && r16(DS, 0x8754u) == 32u);
    CHECK((cpu.flags & 0x08D5u) == 0x0044u);                              /* the equal CMP, CF cleared */

    cpu = world(72u, two, 2u, HEAP + 0x400u, 0u, HEAP + 0x1000u, 0u);    /* 72 = 4 paragraphs + 8: block 2 exactly */
    CHECK(pr_system_getmem_023f(ram, RAM, &cpu, 64u) == PR_SYSTEM_GETMEM_RETURNED);
    returned(&cpu, HEAP + 0x20u, 8u);
    CHECK(r16(HEAP + 0x10u, 0u) == 0u && r16(HEAP + 0x10u, 2u) == HEAP + 0x400u);

    cpu = world(17u, two, 2u, HEAP + 0x400u, 0u, HEAP + 0x1000u, 0u);    /* 17 -> 24 bytes from block 1 (32): 8 remain */
    CHECK(pr_system_getmem_023f(ram, RAM, &cpu, 64u) == PR_SYSTEM_GETMEM_RETURNED);
    returned(&cpu, HEAP + 0x10u, 0u);
    CHECK(r16(DS, 0x440Cu) == 8u && r16(DS, 0x440Eu) == HEAP + 0x11u);   /* the remainder at 3011:0008 */
    CHECK(r16(HEAP + 0x11u, 8u) == 8u && r16(HEAP + 0x11u, 10u) == HEAP + 0x20u && r16(HEAP + 0x11u, 12u) == 8u && r16(HEAP + 0x11u, 14u) == 0u);
    CHECK((cpu.flags & 0x08D5u) == 0x0000u);                              /* AND AL,0Fh giving 8: one bit, PF clear */

    cpu = world(200u, two, 2u, HEAP + 0x400u, 8u, HEAP + 0x1000u, 0u);   /* nothing fits: the top grows by 200 bytes */
    CHECK(pr_system_getmem_023f(ram, RAM, &cpu, 64u) == PR_SYSTEM_GETMEM_RETURNED);
    returned(&cpu, HEAP + 0x400u, 8u);
    CHECK(r16(DS, 0x4404u) == 0u && r16(DS, 0x4406u) == HEAP + 0x40Du);  /* 3400:0008 + 200 -> 340D:0000 */
    CHECK(r16(HEAP + 0x20u, 8u) == 0u && r16(HEAP + 0x20u, 10u) == HEAP + 0x40Du);
    CHECK((cpu.flags & 0x08D5u) == 0x0044u && r16(SS, SP0 - 4u - 4u - 2u) == 0u); /* HeapError's XOR; its argument 0 */

    cpu = world(0xFFFFu, NULL, 0u, HEAP + 0x400u, 0u, HEAP + 0x1500u, 0u); /* 65535 -> 1000h paragraphs (the 17th bit) */
    CHECK(pr_system_getmem_023f(ram, RAM, &cpu, 64u) == PR_SYSTEM_GETMEM_RETURNED);
    returned(&cpu, HEAP + 0x400u, 0u);
    CHECK(r16(DS, 0x4406u) == HEAP + 0x1400u && r16(DS, 0x4404u) == 0u && r16(DS, 0x440Eu) == HEAP + 0x1400u);

    cpu = world(1u, NULL, 0u, HEAP + 0x400u, 8u, HEAP + 0x1000u, 0u);     /* 1 -> 8 bytes: 3400:0008 -> 3401:0000 */
    CHECK(pr_system_getmem_023f(ram, RAM, &cpu, 64u) == PR_SYSTEM_GETMEM_RETURNED);
    returned(&cpu, HEAP + 0x400u, 8u);
    CHECK(r16(DS, 0x4406u) == HEAP + 0x401u && r16(DS, 0x4404u) == 0u);
}

static void stops(void)
{
    static const block self[1] = {{HEAP + 0x10u, 0u, 1u, 0u}};
    pr_system_getmem_cpu cpu;

    cpu = world(0x200u, NULL, 0u, HEAP + 0x400u, 0u, HEAP + 0x410u, 0u); /* the top would pass HeapEnd: RunError 203 */
    CHECK(pr_system_getmem_023f(ram, RAM, &cpu, 64u) == PR_SYSTEM_GETMEM_RUNERROR);
    CHECK(cpu.ax == 0x00CBu && cpu.cs == CS && cpu.ip == 0x00E2u && cpu.sp == SP0 && cpu.bp == BP0);
    CHECK(cpu.flags == (KEPT | 0x0095u) && r16(DS, 0x8754u) == 0x200u && r16(DS, 0x4406u) == HEAP + 0x400u);

    cpu = world(64u, NULL, 0u, HEAP + 0x400u, 0u, HEAP + 0x1000u, 0u);   /* another HeapError: stop at its call */
    w16(DS, 0x4414u, 0x1234u); w16(DS, 0x4416u, 0x5678u);
    CHECK(pr_system_getmem_023f(ram, RAM, &cpu, 64u) == PR_SYSTEM_GETMEM_HEAP_ERROR);
    CHECK(cpu.ip == 0x03B7u && cpu.sp == SP0 - 2u - 2u - 4u - 2u && r16(DS, 0x4406u) == HEAP + 0x404u);
    cpu = world(0x200u, NULL, 0u, HEAP + 0x400u, 0u, HEAP + 0x410u, 0u);
    w16(DS, 0x4416u, CS + 1u);
    CHECK(pr_system_getmem_023f(ram, RAM, &cpu, 64u) == PR_SYSTEM_GETMEM_HEAP_ERROR);
    CHECK(cpu.ip == 0x03C3u && cpu.sp == SP0 - 2u - 2u - 2u && r16(SS, cpu.sp) == 0x200u);

    cpu = world(0x400u, self, 1u, HEAP + 0x400u, 0u, HEAP + 0x1000u, 0u); /* a self-linked block walks forever */
    w16(HEAP + 0x10u, 0u, 0u); w16(HEAP + 0x10u, 2u, HEAP + 0x10u);
    CHECK(pr_system_getmem_023f(ram, RAM, &cpu, 5u) == PR_SYSTEM_GETMEM_LIMIT);
    CHECK(cpu.ip == 0x0319u && cpu.es == HEAP + 0x10u && cpu.di == 0u && cpu.dx == 0x40u && cpu.ax == 0u);
}

static void arguments(void)
{
    pr_system_getmem_cpu cpu, before;
    cpu = world(16u, NULL, 0u, HEAP + 0x400u, 0u, HEAP + 0x1000u, 0u);
    before = cpu;
    CHECK(pr_system_getmem_023f(NULL, RAM, &cpu, 64u) == PR_SYSTEM_GETMEM_ARGUMENT);
    CHECK(pr_system_getmem_023f(ram, RAM, NULL, 64u) == PR_SYSTEM_GETMEM_ARGUMENT);
    CHECK(pr_system_getmem_023f(ram, RAM - 1u, &cpu, 64u) == PR_SYSTEM_GETMEM_ARGUMENT);
    cpu.ip = 0x0240u;
    CHECK(pr_system_getmem_023f(ram, RAM, &cpu, 64u) == PR_SYSTEM_GETMEM_ARGUMENT);
    cpu.ip = before.ip;
    CHECK(memcmp(&cpu, &before, sizeof cpu) == 0 && r16(SS, SP0 - 2u) == 0xA5A5u);
}

int main(void)
{
    ram = malloc(RAM);
    CHECK(ram != NULL);
    allocations();
    stops();
    arguments();
    free(ram);
    printf("getmem: %u checks passed\n", checks);
    return 0;
}
