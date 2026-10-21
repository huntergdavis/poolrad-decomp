/* Focused synthetic test for the raw-CPU SYSTEM FilePos 0AF8:1593 + 15DA (docs/re/SYSTEM_FILEPOS_1593_CPU.md). No game bytes. */
#include "poolrad/recovered/system_filepos_1593_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "filepos line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
static size_t p(unsigned s, unsigned o) { return ((size_t)s * 16u + (o & 65535u)) & 1048575u; }
static void put(uint8_t *m, unsigned s, unsigned o, unsigned v) { size_t a = p(s, o); m[a] = (uint8_t)v; m[(a + 1u) & 1048575u] = (uint8_t)(v >> 8); }
static unsigned get(const uint8_t *m, unsigned s, unsigned o) { size_t a = p(s, o); return m[a] | (unsigned)m[(a + 1u) & 1048575u] << 8; }

enum { SS = 0x2000, DS = 0x5000, F = 0x3000 };
/* Each LSEEK answers DX:AX := pos[k], carry err[k]; refuse declines call `refuse` (1-based). */
static int calls, refuse; static uint32_t pos[3]; static int err[3]; static pr_filepos_cpu at[3];
static int dos(void *ctx, const pr_filepos_cpu *before, pr_filepos_cpu *after)
{
    int k = calls++;
    (void)ctx;
    at[k] = *before; *after = *before; after->ip = (uint16_t)(before->ip + 2u);
    if (refuse == calls) return 1;
    after->ax = (uint16_t)pos[k]; after->dx = (uint16_t)(pos[k] >> 16);
    after->flags = (uint16_t)(err[k] ? before->flags | 1u : before->flags & ~1u);
    return 0;
}
static pr_filepos_cpu setup(uint8_t *ram, unsigned magic, unsigned recsize, uint32_t cur, uint32_t end)
{
    pr_filepos_cpu c = {0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, 0x8000, 0x6000, 0x1593, DS, 0x1111, SS, 0x0002};
    memset(ram, 0x5A, PR_FILEPOS_RAM_SIZE);
    put(ram, DS, 0x4426, 0x7777);
    put(ram, F, 0x100, 9); put(ram, F, 0x102, magic); put(ram, F, 0x104, recsize);
    put(ram, SS, 0x8000, 0x7000); put(ram, SS, 0x8002, 0x9001); put(ram, SS, 0x8004, 0x0100); put(ram, SS, 0x8006, F);
    calls = refuse = 0; pos[0] = cur; pos[1] = end; pos[2] = cur; err[0] = err[1] = err[2] = 0;
    return c;
}

int main(void)
{
    uint8_t *ram = malloc(PR_FILEPOS_RAM_SIZE);
    pr_filepos_cpu c, before;
    CHECK(ram != NULL);

    /* recsize 128, current 12000h, end 30000h: THREE LSEEKs in order 4201h (CX:DX 0), 4202h (CX:DX 0), 4200h (CX:DX = the
     * current 0001:2000); then LongDiv: 12000h / 128 = 240h. */
    c = setup(ram, 0xD7B3, 128, 0x12000u, 0x30000u);
    CHECK(pr_system_filepos_1593(ram, PR_FILEPOS_RAM_SIZE, &c, dos, NULL) == PR_FILEPOS_RETURNED && calls == 3);
    CHECK(at[0].ip == 0x15F2 && at[0].ax == 0x4201 && at[0].bx == 9 && at[0].cx == 0 && at[0].dx == 0 && at[0].sp == 0x7FFE);
    CHECK(at[1].ip == 0x1600 && at[1].ax == 0x4202 && at[1].bx == 9 && at[1].cx == 0 && at[1].dx == 0 && at[1].sp == 0x7FFA);
    CHECK(at[2].ip == 0x160E && at[2].ax == 0x4200 && at[2].bx == 9 && at[2].cx == 0x0001 && at[2].dx == 0x2000 && at[2].sp == 0x7FFA);
    CHECK(c.dx == 0 && c.ax == 0x0240 && c.cs == 0x9001 && c.ip == 0x7000 && c.sp == 0x8008 && get(ram, DS, 0x4426) == 0x7777);
    /* recsize 1: no division; DX:AX is the restore's answer; CX = 1, BX = the end's high word. */
    c = setup(ram, 0xD7B3, 1, 0x12345u, 0x30000u);
    CHECK(pr_system_filepos_1593(ram, PR_FILEPOS_RAM_SIZE, &c, dos, NULL) == PR_FILEPOS_RETURNED);
    CHECK(c.dx == 0x0001 && c.ax == 0x2345 && c.cx == 1 && c.bx == 0x0003 && (c.flags & 0x40u) != 0);
    /* LongDiv is signed: 80000000h / 2 = C0000000h. */
    c = setup(ram, 0xD7B3, 2, 0x80000000u, 0x80000000u);
    CHECK(pr_system_filepos_1593(ram, PR_FILEPOS_RAM_SIZE, &c, dos, NULL) == PR_FILEPOS_RETURNED && c.dx == 0xC000 && c.ax == 0);
    /* No carry check between the LSEEKs: a failing 4201h (AX = 6) still feeds the restore's CX:DX = 0000:0006. */
    c = setup(ram, 0xD7B3, 128, 6u, 0x30000u); err[0] = 1; pos[2] = 0x100u;
    CHECK(pr_system_filepos_1593(ram, PR_FILEPOS_RAM_SIZE, &c, dos, NULL) == PR_FILEPOS_RETURNED && calls == 3);
    CHECK(at[2].cx == 0 && at[2].dx == 6 && c.ax == 2 && c.dx == 0);
    /* The restore's carry: JB 15A7 skips the division; DX:AX is the restore's answer; InOutRes untouched. */
    c = setup(ram, 0xD7B3, 128, 0x12000u, 0x30000u); err[2] = 1; pos[2] = 6u;
    CHECK(pr_system_filepos_1593(ram, PR_FILEPOS_RAM_SIZE, &c, dos, NULL) == PR_FILEPOS_RETURNED && c.ax == 6 && c.dx == 0);
    CHECK((c.flags & 1u) != 0 && get(ram, DS, 0x4426) == 0x7777);
    /* Not open: InOutRes := 103, DX:AX := 0, CF set, no LSEEK. */
    c = setup(ram, 0xD7B0, 128, 1u, 1u);
    CHECK(pr_system_filepos_1593(ram, PR_FILEPOS_RAM_SIZE, &c, dos, NULL) == PR_FILEPOS_RETURNED && calls == 0);
    CHECK(get(ram, DS, 0x4426) == 103 && c.ax == 0 && c.dx == 0 && (c.flags & 1u) != 0 && c.sp == 0x8008);

    /* DIVIDE (EXIT-class): recsize 0 -> LongDiv divisor 0 -> runtime error 200 at CS:00E2. */
    c = setup(ram, 0xD7B3, 0, 0x12000u, 0x30000u);
    CHECK(pr_system_filepos_1593(ram, PR_FILEPOS_RAM_SIZE, &c, dos, NULL) == PR_FILEPOS_DIVIDE && calls == 3);
    CHECK(c.ip == 0x00E2 && c.ax == 0x00C8);
    /* TRANSFER: SS = DS, entry SP 4428h: the helper's 103 store lands on its return slot; RET to 0067h. */
    c = setup(ram, 0xD7B0, 128, 1u, 1u); c.ss = DS; c.sp = 0x4428;
    CHECK(pr_system_filepos_1593(ram, PR_FILEPOS_RAM_SIZE, &c, dos, NULL) == PR_FILEPOS_TRANSFER && c.ip == 0x0067 && c.sp == 0x4428);
    /* SERVICE: the second LSEEK declined; stopped at its INT. */
    c = setup(ram, 0xD7B3, 128, 1u, 1u); refuse = 2;
    CHECK(pr_system_filepos_1593(ram, PR_FILEPOS_RAM_SIZE, &c, dos, NULL) == PR_FILEPOS_SERVICE && c.ip == 0x1600 && calls == 2);

    /* ARGUMENT: rejected before any change. */
    c = setup(ram, 0xD7B3, 128, 1u, 1u); before = c;
    CHECK(pr_system_filepos_1593(ram, PR_FILEPOS_RAM_SIZE, &c, NULL, NULL) == PR_FILEPOS_ARGUMENT);
    CHECK(pr_system_filepos_1593(ram, PR_FILEPOS_RAM_SIZE - 1u, &c, dos, NULL) == PR_FILEPOS_ARGUMENT);
    c.sp = 0x0009; CHECK(pr_system_filepos_1593(ram, PR_FILEPOS_RAM_SIZE, &c, dos, NULL) == PR_FILEPOS_ARGUMENT);
    c.sp = 0xFFF9; CHECK(pr_system_filepos_1593(ram, PR_FILEPOS_RAM_SIZE, &c, dos, NULL) == PR_FILEPOS_ARGUMENT);
    c.sp = 0x8000; c.ip = 0x15AA; CHECK(pr_system_filepos_1593(ram, PR_FILEPOS_RAM_SIZE, &c, dos, NULL) == PR_FILEPOS_ARGUMENT);
    c.ip = 0x1593; CHECK(memcmp(&c, &before, sizeof c) == 0 && calls == 0);

    free(ram);
    printf("filepos 1593: %u semantic checks passed\n", checks);
    return 0;
}
