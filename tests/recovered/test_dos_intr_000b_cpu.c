/* Focused synthetic test for the raw-CPU Dos.Intr 0A32:000B (docs/re/DOS_INTR_000B_CPU.md). No game bytes. */
#include "poolrad/recovered/dos_intr_000b_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "intr line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
static size_t p(unsigned s, unsigned o) { return ((size_t)s * 16u + (o & 65535u)) & 1048575u; }
static void put(uint8_t *m, unsigned s, unsigned o, unsigned v) { size_t a = p(s, o); m[a] = (uint8_t)v; m[(a + 1u) & 1048575u] = (uint8_t)(v >> 8); }
static unsigned get(const uint8_t *m, unsigned s, unsigned o) { size_t a = p(s, o); return m[a] | (unsigned)m[(a + 1u) & 1048575u] << 8; }

enum { SS = 0x2000, DATA = 0x3000, BODY = 0x6000, HSEG = 0x7800, HOFF = 0x0040 };
static int mode;  /* 0 IRET-style handler; 1 refuse; 2 wrong SP */
static pr_dos_intr_cpu seen;
static unsigned frame[3];  /* the fake INT frame at handler entry */
static int handler(void *ctx, uint8_t *ram, const pr_dos_intr_cpu *before, pr_dos_intr_cpu *after)
{
    (void)ctx;
    seen = *before;
    frame[0] = get(ram, before->ss, before->sp); frame[1] = get(ram, before->ss, before->sp + 2u);
    frame[2] = get(ram, before->ss, before->sp + 4u);
    if (mode == 1) return 1;
    *after = *before;
    after->ax = 0xAAAA; after->si = 0x5151; after->bp = 0xB0B0; after->ds = 0x4444;
    after->ip = (uint16_t)get(ram, before->ss, before->sp);              /* IRET */
    after->cs = (uint16_t)get(ram, before->ss, before->sp + 2u);
    after->flags = (uint16_t)get(ram, before->ss, before->sp + 4u);
    after->sp = (uint16_t)(before->sp + (mode == 2 ? 4u : 6u));
    return 0;
}
static pr_dos_intr_cpu setup(uint8_t *ram, unsigned intno, uint16_t sp)
{
    pr_dos_intr_cpu c = {0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, sp, BODY, 0x000B, 0x5000, 0x1111, SS, 0x0603};
    const unsigned regs[10] = {0x0A0A, 0x0B0B, 0x0C0C, 0x0D0D, 0x0E0E, 0x0F0F, 0x1010, 0x2121, 0x3131, 0xFFFF};
    unsigned i;
    memset(ram, 0x5A, PR_DOS_INTR_RAM_SIZE);
    for (i = 0; i < 10u; ++i) put(ram, DATA, 0x0100u + 2u * i, regs[i]);
    put(ram, 0, intno * 4u, HOFF); put(ram, 0, intno * 4u + 2u, HSEG);
    put(ram, SS, sp, 0x7000); put(ram, SS, sp + 2u, 0x9001);             /* caller's far return */
    put(ram, SS, sp + 4u, 0x0100); put(ram, SS, sp + 6u, DATA);         /* Regs */
    put(ram, SS, sp + 8u, 0xA500u | intno);                              /* IntNo: only the low byte is read */
    mode = 0;
    return c;
}

int main(void)
{
    uint8_t *ram = malloc(PR_DOS_INTR_RAM_SIZE);
    pr_dos_intr_cpu c, before;
    CHECK(ram != NULL);

    c = setup(ram, 0x10, 0x8000);
    CHECK(pr_dos_intr_000b(ram, PR_DOS_INTR_RAM_SIZE, &c, handler, NULL) == PR_DOS_INTR_RETURNED);
    /* Handler entry: the vector at 0000:0040h; registers from Regs; SP 8000h - 0Ah below the fake INT frame
     * (0045h, CS, entry flags); flags of SHL BX,1 on 20h (ZF 0, PF 0, AF 1), DF and IF cleared: 0603h -> 0012h. */
    CHECK(seen.cs == HSEG && seen.ip == HOFF && seen.sp == 0x7FF6 && seen.ss == SS);
    CHECK(seen.ax == 0x0A0A && seen.bx == 0x0B0B && seen.cx == 0x0C0C && seen.dx == 0x0D0D && seen.bp == 0x0E0E);
    CHECK(seen.si == 0x0F0F && seen.di == 0x1010 && seen.ds == 0x2121 && seen.es == 0x3131);
    CHECK(seen.flags == 0x0012);
    CHECK(frame[0] == 0x0045 && frame[1] == BODY && frame[2] == 0x0603);
    /* After return the words below the entry SP hold 0045h's pushes (BP DI ES FLAGS of the handler) over the frame,
     * then the saved DS and BP. */
    CHECK(get(ram, SS, 0x7FF4) == 0xB0B0 && get(ram, SS, 0x7FF6) == 0x1010 && get(ram, SS, 0x7FF8) == 0x3131);
    CHECK(get(ram, SS, 0x7FFA) == 0x0603 && get(ram, SS, 0x7FFC) == 0x5000 && get(ram, SS, 0x7FFE) == 0x7777);
    /* Regs gets the handler's AX BX CX DX BP SI DI DS ES and the flags it returned (the IRET frame's 0603h). */
    CHECK(get(ram, DATA, 0x100) == 0xAAAA && get(ram, DATA, 0x102) == 0x0B0B && get(ram, DATA, 0x104) == 0x0C0C);
    CHECK(get(ram, DATA, 0x106) == 0x0D0D && get(ram, DATA, 0x108) == 0xB0B0 && get(ram, DATA, 0x10A) == 0x5151);
    CHECK(get(ram, DATA, 0x10C) == 0x1010 && get(ram, DATA, 0x10E) == 0x4444 && get(ram, DATA, 0x110) == 0x3131);
    CHECK(get(ram, DATA, 0x112) == 0x0603);
    /* Return: AX holds the stored flags, DI past the record, ES the Regs segment, DS and BP the caller's,
     * RETF 6 to 9001:7000 with SP 800Ah, CLD applied to the handler's flags. */
    CHECK(c.cs == 0x9001 && c.ip == 0x7000 && c.sp == 0x800A && c.bp == 0x7777 && c.ds == 0x5000);
    CHECK(c.ax == 0x0603 && c.bx == 0x0B0B && c.si == 0x5151 && c.di == 0x0114 && c.es == DATA && c.flags == 0x0203);

    /* IntNo 0: SHL of 0 sets ZF and PF; AF is still 1. */
    c = setup(ram, 0x00, 0x8000);
    CHECK(pr_dos_intr_000b(ram, PR_DOS_INTR_RAM_SIZE, &c, handler, NULL) == PR_DOS_INTR_RETURNED && seen.flags == 0x0056);

    /* Refusal and a non-continuation (wrong SP): SERVICE, stopped at handler entry, Regs untouched. */
    c = setup(ram, 0x61, 0x8000); mode = 1;
    CHECK(pr_dos_intr_000b(ram, PR_DOS_INTR_RAM_SIZE, &c, handler, NULL) == PR_DOS_INTR_SERVICE);
    CHECK(c.cs == HSEG && c.ip == HOFF && c.sp == 0x7FF6 && get(ram, DATA, 0x100) == 0x0A0A);
    c = setup(ram, 0x61, 0x8000); mode = 2;
    CHECK(pr_dos_intr_000b(ram, PR_DOS_INTR_RAM_SIZE, &c, handler, NULL) == PR_DOS_INTR_SERVICE && c.ip == HOFF);

    /* ARGUMENT: rejected before any change, including SP outside [0014h,FFF6h]. */
    c = setup(ram, 0x61, 0x8000); before = c;
    CHECK(pr_dos_intr_000b(NULL, PR_DOS_INTR_RAM_SIZE, &c, handler, NULL) == PR_DOS_INTR_ARGUMENT);
    CHECK(pr_dos_intr_000b(ram, PR_DOS_INTR_RAM_SIZE - 1u, &c, handler, NULL) == PR_DOS_INTR_ARGUMENT);
    CHECK(pr_dos_intr_000b(ram, PR_DOS_INTR_RAM_SIZE, &c, NULL, NULL) == PR_DOS_INTR_ARGUMENT);
    c.sp = 0x0013; CHECK(pr_dos_intr_000b(ram, PR_DOS_INTR_RAM_SIZE, &c, handler, NULL) == PR_DOS_INTR_ARGUMENT);
    c.sp = 0xFFF7; CHECK(pr_dos_intr_000b(ram, PR_DOS_INTR_RAM_SIZE, &c, handler, NULL) == PR_DOS_INTR_ARGUMENT);
    c.sp = 0x8000; c.ip = 0x0000; CHECK(pr_dos_intr_000b(ram, PR_DOS_INTR_RAM_SIZE, &c, handler, NULL) == PR_DOS_INTR_ARGUMENT);
    c.ip = 0x000B; CHECK(memcmp(&c, &before, sizeof c) == 0 && get(ram, SS, 0x7FFE) == 0x5A5A);

    free(ram);
    printf("intr 000B: %u semantic checks passed\n", checks);
    return 0;
}
