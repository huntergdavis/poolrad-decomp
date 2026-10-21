/* Focused synthetic test for the raw-CPU SYSTEM Seek 0AF8:1563 (+ composed 14B6) (docs/re/SYSTEM_SEEK_1563_CPU.md). No game bytes. */
#include "poolrad/recovered/system_seek_1563_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "seek line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
static size_t p(unsigned s, unsigned o) { return ((size_t)s * 16u + (o & 65535u)) & 1048575u; }
static void put(uint8_t *m, unsigned s, unsigned o, unsigned v) { size_t a = p(s, o); m[a] = (uint8_t)v; m[(a + 1u) & 1048575u] = (uint8_t)(v >> 8); }
static unsigned get(const uint8_t *m, unsigned s, unsigned o) { size_t a = p(s, o); return m[a] | (unsigned)m[(a + 1u) & 1048575u] << 8; }

enum { SS = 0x2000, DS = 0x5000, F = 0x3000 };
static int calls, fail; static pr_seek_cpu at_int;
static int dos(void *ctx, const pr_seek_cpu *before, pr_seek_cpu *after)
{
    (void)ctx;
    ++calls; at_int = *before; *after = *before; after->ip = 0x158Au;
    if (fail) { after->ax = 6; after->flags = (uint16_t)(before->flags | 1u); }
    else { after->ax = before->dx; after->dx = before->cx; after->flags = (uint16_t)(before->flags & ~1u); }
    return 0;
}
static pr_seek_cpu setup(uint8_t *ram, unsigned magic, unsigned recsize, uint32_t n)
{
    pr_seek_cpu c = {0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, 0x8000, 0x6000, 0x1563, DS, 0x1111, SS, 0x0002};
    memset(ram, 0x5A, PR_SEEK_RAM_SIZE);
    put(ram, DS, 0x4426, 0x7777);
    put(ram, F, 0x100, 9); put(ram, F, 0x102, magic); put(ram, F, 0x104, recsize);
    put(ram, SS, 0x8000, 0x7000); put(ram, SS, 0x8002, 0x9001);
    put(ram, SS, 0x8004, n & 0xFFFFu); put(ram, SS, 0x8006, n >> 16); put(ram, SS, 0x8008, 0x0100); put(ram, SS, 0x800A, F);
    calls = fail = 0;
    return c;
}

int main(void)
{
    uint8_t *ram = malloc(PR_SEEK_RAM_SIZE);
    pr_seek_cpu c, before;
    CHECK(ram != NULL);

    /* Seek(f, 10) with recsize 128: position 1280 = 0000:0500h; LSEEK AX=4200h, BX = handle 9, CX:DX = 0000:0500. */
    c = setup(ram, 0xD7B3, 128, 10);
    CHECK(pr_system_seek_1563(ram, PR_SEEK_RAM_SIZE, &c, dos, NULL) == PR_SEEK_RETURNED && calls == 1);
    CHECK(at_int.ip == 0x1588 && at_int.ax == 0x4200 && at_int.bx == 9 && at_int.cx == 0 && at_int.dx == 0x0500);
    CHECK(c.cs == 0x9001 && c.ip == 0x7000 && c.sp == 0x800C && c.bp == 0x7777 && get(ram, DS, 0x4426) == 0x7777);
    /* 32-bit truncation: n = 01000000h * 0200h = 2^33 -> 0; n = 00230001h * 2000h: high 23h*2000h = 46000h keeps 6000h,
     * low 1*2000h = 2000h: CX:DX = 6000:2000. */
    c = setup(ram, 0xD7B3, 0x0200, 0x01000000u);
    CHECK(pr_system_seek_1563(ram, PR_SEEK_RAM_SIZE, &c, dos, NULL) == PR_SEEK_RETURNED && at_int.cx == 0 && at_int.dx == 0);
    c = setup(ram, 0xD7B3, 0x2000, 0x00230001u);
    CHECK(pr_system_seek_1563(ram, PR_SEEK_RAM_SIZE, &c, dos, NULL) == PR_SEEK_RETURNED && at_int.cx == 0x6000 && at_int.dx == 0x2000);
    /* The carry from the low product's high word into CX: n = 0001FFFFh * 2: high 1*2 = 2, low FFFFh*2 = 1:FFFE -> CX = 3. */
    c = setup(ram, 0xD7B3, 2, 0x0001FFFFu);
    CHECK(pr_system_seek_1563(ram, PR_SEEK_RAM_SIZE, &c, dos, NULL) == PR_SEEK_RETURNED && at_int.cx == 3 && at_int.dx == 0xFFFE);
    /* A DOS error: InOutRes := AX (6). */
    c = setup(ram, 0xD7B3, 128, 1); fail = 1;
    CHECK(pr_system_seek_1563(ram, PR_SEEK_RAM_SIZE, &c, dos, NULL) == PR_SEEK_RETURNED && get(ram, DS, 0x4426) == 6);
    /* Not open (D7B0h): the helper sets InOutRes := 103 and there is no LSEEK; its RET word 156Ch stays below the frame. */
    c = setup(ram, 0xD7B0, 128, 1);
    CHECK(pr_system_seek_1563(ram, PR_SEEK_RAM_SIZE, &c, dos, NULL) == PR_SEEK_RETURNED && calls == 0 && get(ram, DS, 0x4426) == 103);
    CHECK(get(ram, SS, 0x7FFC) == 0x156C && get(ram, SS, 0x7FFE) == 0x7777 && (c.flags & 0x40u) == 0);

    /* TRANSFER: SS = DS, entry SP 442Ah puts the check's return slot on InOutRes; not open stores 103 there: RET to 0067h. */
    c = setup(ram, 0xD7B0, 128, 1); c.ss = DS; c.sp = 0x442A;
    CHECK(pr_system_seek_1563(ram, PR_SEEK_RAM_SIZE, &c, dos, NULL) == PR_SEEK_TRANSFER && c.ip == 0x0067 && c.sp == 0x4428 && calls == 0);

    /* ARGUMENT: rejected before any change. */
    c = setup(ram, 0xD7B3, 128, 1); before = c;
    CHECK(pr_system_seek_1563(ram, PR_SEEK_RAM_SIZE, &c, NULL, NULL) == PR_SEEK_ARGUMENT);
    CHECK(pr_system_seek_1563(ram, PR_SEEK_RAM_SIZE - 1u, &c, dos, NULL) == PR_SEEK_ARGUMENT);
    c.sp = 0x0005; CHECK(pr_system_seek_1563(ram, PR_SEEK_RAM_SIZE, &c, dos, NULL) == PR_SEEK_ARGUMENT);
    c.sp = 0xFFF5; CHECK(pr_system_seek_1563(ram, PR_SEEK_RAM_SIZE, &c, dos, NULL) == PR_SEEK_ARGUMENT);
    c.sp = 0x8000; c.ip = 0x1564; CHECK(pr_system_seek_1563(ram, PR_SEEK_RAM_SIZE, &c, dos, NULL) == PR_SEEK_ARGUMENT);
    c.ip = 0x1563; CHECK(memcmp(&c, &before, sizeof c) == 0 && calls == 0);

    free(ram);
    printf("seek 1563: %u semantic checks passed\n", checks);
    return 0;
}
