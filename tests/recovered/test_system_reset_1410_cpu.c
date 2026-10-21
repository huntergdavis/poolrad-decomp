/* Focused synthetic test for the raw-CPU SYSTEM Reset 0AF8:1410 (docs/re/SYSTEM_RESET_1410_CPU.md). No game bytes. */
#include "poolrad/recovered/system_reset_1410_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "reset line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
static size_t p(unsigned s, unsigned o) { return ((size_t)s * 16u + (o & 65535u)) & 1048575u; }
static void put(uint8_t *m, unsigned s, unsigned o, unsigned v) { size_t a = p(s, o); m[a] = (uint8_t)v; m[(a + 1u) & 1048575u] = (uint8_t)(v >> 8); }
static unsigned get(const uint8_t *m, unsigned s, unsigned o) { size_t a = p(s, o); return m[a] | (unsigned)m[(a + 1u) & 1048575u] << 8; }

enum { SS = 0x2000, DS = 0x5000, F = 0x3000 };
static int calls, refuse, fail; static pr_reset_cpu at_int;
static int dos(void *ctx, const pr_reset_cpu *before, pr_reset_cpu *after)
{
    (void)ctx;
    ++calls; at_int = *before;
    if (refuse) return 1;
    *after = *before; after->ip = 0x1458u;
    if (fail) { after->ax = 2; after->flags = (uint16_t)(before->flags | 1u); }
    else { after->ax = 5; after->flags = (uint16_t)(before->flags & ~1u); }
    return 0;
}
static pr_reset_cpu setup(uint8_t *ram, unsigned magic, const char *name)
{
    pr_reset_cpu c = {0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, 0x8000, 0x6000, 0x1410, DS, 0x1111, SS, 0x0002};
    memset(ram, 0x5A, PR_RESET_RAM_SIZE);
    ram[p(DS, 0x442C)] = 2; put(ram, DS, 0x4426, 0x7777);                  /* FileMode 2, InOutRes untouched marker */
    put(ram, F, 0x100, 7); put(ram, F, 0x102, magic); put(ram, F, 0x104, 0x1111);
    strcpy((char *)ram + p(F, 0x130), name);
    put(ram, SS, 0x8000, 0x7000); put(ram, SS, 0x8002, 0x9001);
    put(ram, SS, 0x8004, 128); put(ram, SS, 0x8006, 0x0100); put(ram, SS, 0x8008, F);
    calls = refuse = fail = 0;
    return c;
}

int main(void)
{
    uint8_t *ram = malloc(PR_RESET_RAM_SIZE);
    pr_reset_cpu c, before;
    CHECK(ram != NULL);

    /* Closed file with a name: INT 21h AH=3Dh, AL = FileMode 2, DS:DX = f+30h; handle 5 -> f = (5, D7B3h, 128). */
    c = setup(ram, 0xD7B0, "GAME.DAT");
    CHECK(pr_system_reset_1410(ram, PR_RESET_RAM_SIZE, &c, dos, NULL) == PR_RESET_RETURNED && calls == 1);
    CHECK(at_int.ip == 0x1456 && at_int.ax == 0x3D02 && at_int.ds == F && at_int.dx == 0x0130 && at_int.cx == 0);
    CHECK(get(ram, F, 0x100) == 5 && get(ram, F, 0x102) == 0xD7B3 && get(ram, F, 0x104) == 128 && get(ram, DS, 0x4426) == 0x7777);
    CHECK(c.cs == 0x9001 && c.ip == 0x7000 && c.sp == 0x800A && c.bp == 0x7777 && c.ds == DS && c.ax == 128);
    /* Empty name: NO INT; XCHG DX,AX makes the handle 0 (standard input); DX keeps 3D02h. */
    c = setup(ram, 0xD7B0, "");
    CHECK(pr_system_reset_1410(ram, PR_RESET_RAM_SIZE, &c, dos, NULL) == PR_RESET_RETURNED && calls == 0);
    CHECK(get(ram, F, 0x100) == 0 && get(ram, F, 0x102) == 0xD7B3 && c.dx == 0x3D02);
    /* DOS error: InOutRes := 2, FileRec untouched. */
    c = setup(ram, 0xD7B0, "NONE"); fail = 1;
    CHECK(pr_system_reset_1410(ram, PR_RESET_RAM_SIZE, &c, dos, NULL) == PR_RESET_RETURNED);
    CHECK(get(ram, DS, 0x4426) == 2 && get(ram, F, 0x102) == 0xD7B0 && get(ram, F, 0x100) == 7);
    /* Not a FileRec magic: InOutRes := 102, no INT. */
    c = setup(ram, 0x1234, "X");
    CHECK(pr_system_reset_1410(ram, PR_RESET_RAM_SIZE, &c, dos, NULL) == PR_RESET_RETURNED && calls == 0 && get(ram, DS, 0x4426) == 102);

    /* Already open: the nested Close is a named stop with AX, DX, ES, DI, CS and the return 1445h pushed (frame BP = 7FFEh). */
    c = setup(ram, 0xD7B3, "GAME.DAT");
    CHECK(pr_system_reset_1410(ram, PR_RESET_RAM_SIZE, &c, dos, NULL) == PR_RESET_CLOSE && calls == 0);
    CHECK(c.ip == 0x1491 && c.cs == 0x6000 && c.sp == 0x7FF2 && c.bp == 0x7FFE);
    CHECK(get(ram, SS, 0x7FF2) == 0x1445 && get(ram, SS, 0x7FF4) == 0x6000 && get(ram, SS, 0x7FF6) == 0x0100 &&
          get(ram, SS, 0x7FF8) == F && get(ram, SS, 0x7FFA) == 0 && get(ram, SS, 0x7FFC) == 0x3D02);
    /* Resume at 1445h after a Close that returned (RETF 4 popped ES:DI; Close set the magic to D7B0h): POP DX, POP AX, open. */
    put(ram, F, 0x102, 0xD7B0); c.ip = 0x1445; c.sp = 0x7FFA;
    CHECK(pr_system_reset_1410(ram, PR_RESET_RAM_SIZE, &c, dos, NULL) == PR_RESET_RETURNED && calls == 1 && at_int.ax == 0x3D02);
    CHECK(get(ram, F, 0x100) == 5 && get(ram, F, 0x102) == 0xD7B3 && c.sp == 0x800A && c.ip == 0x7000);

    /* SERVICE: the DOS service declines; stopped at the INT. */
    c = setup(ram, 0xD7B0, "A"); refuse = 1;
    CHECK(pr_system_reset_1410(ram, PR_RESET_RAM_SIZE, &c, dos, NULL) == PR_RESET_SERVICE && c.ip == 0x1456);

    /* ARGUMENT: rejected before any change. */
    c = setup(ram, 0xD7B0, "A"); before = c;
    CHECK(pr_system_reset_1410(ram, PR_RESET_RAM_SIZE, &c, NULL, NULL) == PR_RESET_ARGUMENT);
    CHECK(pr_system_reset_1410(ram, PR_RESET_RAM_SIZE - 1u, &c, dos, NULL) == PR_RESET_ARGUMENT);
    c.sp = 0x000D; CHECK(pr_system_reset_1410(ram, PR_RESET_RAM_SIZE, &c, dos, NULL) == PR_RESET_ARGUMENT);
    c.sp = 0xFFF7; CHECK(pr_system_reset_1410(ram, PR_RESET_RAM_SIZE, &c, dos, NULL) == PR_RESET_ARGUMENT);
    c.sp = 0x8000; c.ip = 0x1419; CHECK(pr_system_reset_1410(ram, PR_RESET_RAM_SIZE, &c, dos, NULL) == PR_RESET_ARGUMENT);
    c.ip = 0x1410; CHECK(memcmp(&c, &before, sizeof c) == 0 && calls == 0);

    free(ram);
    printf("reset 1410: %u semantic checks passed\n", checks);
    return 0;
}
