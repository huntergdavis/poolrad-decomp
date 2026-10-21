/* Focused synthetic test for the raw-CPU SYSTEM BlockRead 0AF8:14FB (docs/re/SYSTEM_BLOCKREAD_14FB_CPU.md). No game bytes. */
#include "poolrad/recovered/system_blockread_14fb_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "blockread line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
static size_t p(unsigned s, unsigned o) { return ((size_t)s * 16u + (o & 65535u)) & 1048575u; }
static void put(uint8_t *m, unsigned s, unsigned o, unsigned v) { size_t a = p(s, o); m[a] = (uint8_t)v; m[(a + 1u) & 1048575u] = (uint8_t)(v >> 8); }
static unsigned get(const uint8_t *m, unsigned s, unsigned o) { size_t a = p(s, o); return m[a] | (unsigned)m[(a + 1u) & 1048575u] << 8; }

enum { SS = 0x2000, DS = 0x5000, F = 0x3000, BUF = 0x4000, RES = 0x4800 };
static int calls, refuse, fail; static unsigned got, wrote; static size_t length; static pr_blockread_cpu at_int;
static int dos(void *ctx, const pr_blockread_cpu *before, pr_blockread_cpu *after, uint8_t *buffer, size_t n)
{
    unsigned k;
    (void)ctx;
    ++calls; at_int = *before; length = n; *after = *before; after->ip = 0x152Bu;
    for (k = 0; k < wrote && k < n; ++k)
        buffer[k] = (uint8_t)(0xA0u + k);
    if (refuse) return 1;
    if (fail) { after->ax = 5; after->flags = (uint16_t)(before->flags | 1u); }
    else { after->ax = (uint16_t)got; after->flags = (uint16_t)(before->flags & ~1u); }
    return 0;
}
static pr_blockread_cpu setup(uint8_t *ram, unsigned magic, unsigned recsize, unsigned count, int nil)
{
    pr_blockread_cpu c = {0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, 0x8000, 0x6000, 0x14FB, DS, 0x1111, SS, 0x0002};
    memset(ram, 0x5A, PR_BLOCKREAD_RAM_SIZE);
    put(ram, DS, 0x4426, 0x7777); put(ram, RES, 0x10, 0xEEEE);
    put(ram, F, 0x100, 9); put(ram, F, 0x102, magic); put(ram, F, 0x104, recsize);
    put(ram, SS, 0x8000, 0x7000); put(ram, SS, 0x8002, 0x9001);
    put(ram, SS, 0x8004, nil ? 0 : 0x10); put(ram, SS, 0x8006, nil ? 0 : RES); put(ram, SS, 0x8008, count);
    put(ram, SS, 0x800A, 0x0020); put(ram, SS, 0x800C, BUF); put(ram, SS, 0x800E, 0x0100); put(ram, SS, 0x8010, F);
    calls = refuse = fail = 0; got = wrote = 0;
    return c;
}

int main(void)
{
    uint8_t *ram = malloc(PR_BLOCKREAD_RAM_SIZE);
    pr_blockread_cpu c, before;
    CHECK(ram != NULL);

    /* BlockRead(f, buf, 4, @r), recsize 128: INT 21h AH=3Fh, BX = handle 9, CX = 512, DS:DX = buf; 512 read -> r = 4. */
    c = setup(ram, 0xD7B3, 128, 4, 0); got = 512; wrote = 3;
    CHECK(pr_system_blockread_14fb(ram, PR_BLOCKREAD_RAM_SIZE, &c, dos, NULL) == PR_BLOCKREAD_RETURNED && calls == 1 && length == 512);
    CHECK(at_int.ip == 0x1529 && (at_int.ax >> 8) == 0x3F && at_int.bx == 9 && at_int.cx == 512 && at_int.ds == BUF && at_int.dx == 0x20);
    CHECK(ram[p(BUF, 0x20)] == 0xA0 && ram[p(BUF, 0x22)] == 0xA2 && ram[p(BUF, 0x23)] == 0x5A);      /* the permitted write lands */
    CHECK(get(ram, RES, 0x10) == 4 && get(ram, DS, 0x4426) == 0x7777);
    CHECK(c.cs == 0x9001 && c.ip == 0x7000 && c.sp == 0x8012 && c.bp == 0x7777 && c.ds == DS && c.cx == 0x64 && c.dx == (RES | 0x10));
    /* Short read, 300 bytes: r = 300 DIV 128 = 2 (remainder 44 in DX before the result store's MOV DX,ES); InOutRes untouched. */
    c = setup(ram, 0xD7B3, 128, 4, 0); got = 300;
    CHECK(pr_system_blockread_14fb(ram, PR_BLOCKREAD_RAM_SIZE, &c, dos, NULL) == PR_BLOCKREAD_RETURNED && get(ram, RES, 0x10) == 2);
    CHECK(get(ram, DS, 0x4426) == 0x7777);
    /* Nil result: a short read sets InOutRes := CX = 100; a full read leaves it. */
    c = setup(ram, 0xD7B3, 128, 4, 1); got = 300;
    CHECK(pr_system_blockread_14fb(ram, PR_BLOCKREAD_RAM_SIZE, &c, dos, NULL) == PR_BLOCKREAD_RETURNED && get(ram, DS, 0x4426) == 100);
    CHECK(c.ax == 2 && get(ram, RES, 0x10) == 0xEEEE);
    c = setup(ram, 0xD7B3, 128, 4, 1); got = 512;
    CHECK(pr_system_blockread_14fb(ram, PR_BLOCKREAD_RAM_SIZE, &c, dos, NULL) == PR_BLOCKREAD_RETURNED && get(ram, DS, 0x4426) == 0x7777);
    /* The LOW WORD of count * recsize: 200h * 100h = 20000h -> CX = 0; the read returns 0, r = 0. */
    c = setup(ram, 0xD7B3, 0x100, 0x200, 0); got = 0;
    CHECK(pr_system_blockread_14fb(ram, PR_BLOCKREAD_RAM_SIZE, &c, dos, NULL) == PR_BLOCKREAD_RETURNED && at_int.cx == 0 && length == 0);
    CHECK(get(ram, RES, 0x10) == 0);
    /* 3 * 6000h = 12000h -> CX = 2000h (the high word 1 dropped). */
    c = setup(ram, 0xD7B3, 0x6000, 3, 0); got = 0x2000;
    CHECK(pr_system_blockread_14fb(ram, PR_BLOCKREAD_RAM_SIZE, &c, dos, NULL) == PR_BLOCKREAD_RETURNED && at_int.cx == 0x2000);
    CHECK(get(ram, RES, 0x10) == 0);
    /* count = 0: no INT, r = 0. */
    c = setup(ram, 0xD7B3, 128, 0, 0);
    CHECK(pr_system_blockread_14fb(ram, PR_BLOCKREAD_RAM_SIZE, &c, dos, NULL) == PR_BLOCKREAD_RETURNED && calls == 0 && get(ram, RES, 0x10) == 0);
    /* A DOS error: InOutRes := 5, r := 0; the bytes the failed read wrote stay (the permission covers the error too). */
    c = setup(ram, 0xD7B3, 128, 4, 0); fail = 1; wrote = 2;
    CHECK(pr_system_blockread_14fb(ram, PR_BLOCKREAD_RAM_SIZE, &c, dos, NULL) == PR_BLOCKREAD_RETURNED && get(ram, DS, 0x4426) == 5);
    CHECK(get(ram, RES, 0x10) == 0 && ram[p(BUF, 0x21)] == 0xA1 && c.ax == 0);
    /* Not open (D7B0h): InOutRes := 103, no INT, r := 0. */
    c = setup(ram, 0xD7B0, 128, 4, 0);
    CHECK(pr_system_blockread_14fb(ram, PR_BLOCKREAD_RAM_SIZE, &c, dos, NULL) == PR_BLOCKREAD_RETURNED && calls == 0);
    CHECK(get(ram, DS, 0x4426) == 103 && get(ram, RES, 0x10) == 0 && get(ram, SS, 0x7FFC) == 0x1510);

    /* DIVIDE (EXIT-class): recsize 0, count 3: the read of CX = 0 bytes runs, then the DIV at 1531h faults; DX = 0. */
    c = setup(ram, 0xD7B3, 0, 3, 0); got = 0;
    CHECK(pr_system_blockread_14fb(ram, PR_BLOCKREAD_RAM_SIZE, &c, dos, NULL) == PR_BLOCKREAD_DIVIDE && calls == 1 && at_int.cx == 0);
    CHECK(c.ip == 0x1531 && c.dx == 0 && c.ax == 0 && c.sp == 0x7FFE && c.ds == DS && c.cx == 0x64 && get(ram, RES, 0x10) == 0xEEEE);
    /* recsize 0 with count 0 takes no DIV. */
    c = setup(ram, 0xD7B3, 0, 0, 0);
    CHECK(pr_system_blockread_14fb(ram, PR_BLOCKREAD_RAM_SIZE, &c, dos, NULL) == PR_BLOCKREAD_RETURNED && get(ram, RES, 0x10) == 0);

    /* TRANSFER: SS = DS, entry SP 442Ah puts the check's return slot on InOutRes; not open stores 103 there: RET to 0067h. */
    c = setup(ram, 0xD7B0, 128, 4, 0); c.ss = DS; c.sp = 0x442A;
    CHECK(pr_system_blockread_14fb(ram, PR_BLOCKREAD_RAM_SIZE, &c, dos, NULL) == PR_BLOCKREAD_TRANSFER && c.ip == 0x0067 && c.sp == 0x4428);

    /* SERVICE: declined, or a success claiming more than CX bytes; stopped at the INT, the buffer unchanged. */
    c = setup(ram, 0xD7B3, 128, 4, 0); refuse = 1; wrote = 1;
    CHECK(pr_system_blockread_14fb(ram, PR_BLOCKREAD_RAM_SIZE, &c, dos, NULL) == PR_BLOCKREAD_SERVICE && c.ip == 0x1529 && ram[p(BUF, 0x20)] == 0x5A);
    c = setup(ram, 0xD7B3, 128, 4, 0); got = 513;
    CHECK(pr_system_blockread_14fb(ram, PR_BLOCKREAD_RAM_SIZE, &c, dos, NULL) == PR_BLOCKREAD_SERVICE && c.ip == 0x1529);

    /* ARGUMENT: rejected before any change. */
    c = setup(ram, 0xD7B3, 128, 4, 0); before = c;
    CHECK(pr_system_blockread_14fb(ram, PR_BLOCKREAD_RAM_SIZE, &c, NULL, NULL) == PR_BLOCKREAD_ARGUMENT);
    CHECK(pr_system_blockread_14fb(ram, PR_BLOCKREAD_RAM_SIZE - 1u, &c, dos, NULL) == PR_BLOCKREAD_ARGUMENT);
    c.sp = 0x0005; CHECK(pr_system_blockread_14fb(ram, PR_BLOCKREAD_RAM_SIZE, &c, dos, NULL) == PR_BLOCKREAD_ARGUMENT);
    c.sp = 0xFFEF; CHECK(pr_system_blockread_14fb(ram, PR_BLOCKREAD_RAM_SIZE, &c, dos, NULL) == PR_BLOCKREAD_ARGUMENT);
    c.sp = 0x8000; c.ip = 0x1502; CHECK(pr_system_blockread_14fb(ram, PR_BLOCKREAD_RAM_SIZE, &c, dos, NULL) == PR_BLOCKREAD_ARGUMENT);
    c.ip = 0x14FB; CHECK(memcmp(&c, &before, sizeof c) == 0 && calls == 0);

    free(ram);
    printf("blockread 14fb: %u semantic checks passed\n", checks);
    return 0;
}
