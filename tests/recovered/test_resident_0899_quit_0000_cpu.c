/* Focused synthetic test for the raw-CPU resident 0899:0000 QUIT (docs/re/RES_0899_QUIT_0000_CPU.md).
 * No game bytes: only the three far-call segment words, the INT 10h vector and the game fields are placed. */
#include "poolrad/recovered/resident_0899_quit_0000_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "quit 0000 line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
static size_t p(unsigned s, unsigned o) { return ((size_t)s * 16u + (o & 65535u)) & 1048575u; }
static void put(uint8_t *m, unsigned s, unsigned o, unsigned v) { size_t a = p(s, o); m[a] = (uint8_t)v; m[(a + 1u) & 1048575u] = (uint8_t)(v >> 8); }
static unsigned get(const uint8_t *m, unsigned s, unsigned o) { size_t a = p(s, o); return m[a] | (unsigned)m[(a + 1u) & 1048575u] << 8; }

enum { BODY = 0x1899, DRV = 0x18AE, DOS = 0x1A32, SYS = 0x1AF8, DS = 0x1C7C, SS = 0x7000, HSEG = 0x6000 };
static int refuse_dos, refuse_video, video_calls;
static pr_dos_intr_cpu at_video;
static int dos_vector(void *ctx, uint8_t *ram, pr_r08ae_driver_cpu *c, uint16_t *flags_out)
{ (void)ctx; (void)ram; (void)c; (void)flags_out; return 1; }  /* every DOS call refused */
static int port_out(void *ctx, uint16_t port, uint8_t value) { (void)ctx; (void)port; (void)value; return 1; }
static int port_in(void *ctx, uint16_t port, uint8_t *value) { (void)ctx; (void)port; (void)value; return 1; }
static int gettime(void *ctx, const pr_dos_gettime_cpu *before, pr_dos_gettime_cpu *after) { (void)ctx; (void)before; (void)after; return 1; }
/* An IRET-returning INT 10h handler that changes nothing else. */
static int video(void *ctx, uint8_t *ram, const pr_dos_intr_cpu *before, pr_dos_intr_cpu *after)
{
    (void)ctx;
    ++video_calls; at_video = *before;
    if (refuse_video) return 1;
    *after = *before;
    after->ip = (uint16_t)get(ram, before->ss, before->sp);
    after->cs = (uint16_t)get(ram, before->ss, before->sp + 2u);
    after->flags = (uint16_t)get(ram, before->ss, before->sp + 4u);
    after->sp = (uint16_t)(before->sp + 6u);
    return 0;
}
static pr_r0899_cpu setup(uint8_t *ram, unsigned saved, unsigned w, unsigned mode)
{
    pr_r0899_cpu c = {0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, 0xF000, BODY, 0x0000, DS, 0x1111, SS, 0x0002};
    memset(ram, 0x5A, PR_R0899_QUIT_RAM_SIZE);
    put(ram, BODY, 0x0010, DRV); put(ram, BODY, 0x0028, DOS); put(ram, BODY, 0x002F, SYS);   /* relocated far-call segments */
    put(ram, 0, 0x40, 0x0000); put(ram, 0, 0x42, HSEG);                                       /* IVT[10h] */
    ram[p(DS, 0x5372)] = 0xEE; ram[p(DS, 0x5373)] = (uint8_t)saved;
    put(ram, DS, 0x5374, w); put(ram, DS, 0x2612, 0x0001); ram[p(DS, 0x4A1C)] = (uint8_t)mode;
    refuse_dos = 0; refuse_video = 0; video_calls = 0;
    return c;
}

int main(void)
{
    uint8_t *ram = malloc(PR_R0899_QUIT_RAM_SIZE);
    pr_r0899_quit_env env = {{NULL, dos_vector, port_out, port_in, gettime, 1000u}, video, NULL};
    pr_r0899_quit_diagnostic d;
    pr_r0899_cpu c, before;
    CHECK(ram != NULL);

    /* Saved sound state 2 (muted): [5372] := 2, so 00DC skips every callee and RETF 2 returns. Then Regs.AX at DS:4A02 =
     * (AH 0, AL mode 0Dh), Intr(10h) reaches the handler at 6000:0000 with AX 000Dh, and Halt is called: EXIT. */
    c = setup(ram, 2, 1, 0x0D);
    CHECK(pr_r0899_quit_0000_cpu(ram, PR_R0899_QUIT_RAM_SIZE, &c, &env, &d) == PR_R0899_QUIT_EXIT);
    CHECK(ram[p(DS, 0x5372)] == 2 && d.driver.calls == 0 && d.intr_status == PR_DOS_INTR_RETURNED);
    CHECK(get(ram, DS, 0x4A02) == 0x000D && video_calls == 1 && at_video.cs == HSEG && at_video.ip == 0 && at_video.ax == 0x000D);
    /* EXIT: AX = 0 (the exit code), XOR AX,AX flags (0002h -> 0046h), CS:IP = SYS:00E9, the Halt return 0031 and the body CS
     * pushed below the saved BP (SP F000h - 6). BP is the body's frame pointer (F000h - 2); it never returns to restore it. */
    CHECK(c.ax == 0 && c.flags == 0x0046 && c.cs == SYS && c.ip == 0x00E9 && c.sp == 0xEFFA);
    CHECK(get(ram, SS, 0xEFFA) == 0x0031 && get(ram, SS, 0xEFFC) == BODY && get(ram, SS, 0xEFFE) == 0x7777 && c.bp == 0xEFFE);
    /* Intr's ten STOSWs leave DI past the 20-byte Regs record (4A02h + 14h) and ES = the Regs segment; DS is the game's. */
    CHECK(c.di == 0x4A16 && c.es == DS && c.ds == DS);

    /* Sound on (saved 0) with [5374] non-zero: 00DC's first callee is 137C, whose INT 21h AH=35h the environment
     * refuses: a DRIVER stop naming 137C and its ENV_DOS status; nothing after the driver call runs. */
    c = setup(ram, 0, 1, 0x03); refuse_dos = 1;
    CHECK(pr_r0899_quit_0000_cpu(ram, PR_R0899_QUIT_RAM_SIZE, &c, &env, &d) == PR_R0899_QUIT_DRIVER);
    CHECK(d.driver.member == 0x137C && d.driver.status == 2 && d.driver.calls == 1 && video_calls == 0);
    CHECK(ram[p(DS, 0x5372)] == 0 && ram[p(DS, 0x4A02)] == 0x5A);

    /* The INT 10h handler refuses: an INTR stop with Intr's SERVICE status, at the handler entry. */
    c = setup(ram, 2, 0, 0x03); refuse_video = 1;
    CHECK(pr_r0899_quit_0000_cpu(ram, PR_R0899_QUIT_RAM_SIZE, &c, &env, &d) == PR_R0899_QUIT_INTR);
    CHECK(d.intr_status == PR_DOS_INTR_SERVICE && c.cs == HSEG && c.ip == 0 && ram[p(DS, 0x4A02)] == 0x03);

    /* ARGUMENT: rejected before any change. */
    c = setup(ram, 2, 0, 0x03); before = c;
    CHECK(pr_r0899_quit_0000_cpu(NULL, PR_R0899_QUIT_RAM_SIZE, &c, &env, &d) == PR_R0899_QUIT_ARGUMENT);
    CHECK(pr_r0899_quit_0000_cpu(ram, PR_R0899_QUIT_RAM_SIZE - 1u, &c, &env, &d) == PR_R0899_QUIT_ARGUMENT);
    c.sp = 0x000B; CHECK(pr_r0899_quit_0000_cpu(ram, PR_R0899_QUIT_RAM_SIZE, &c, &env, &d) == PR_R0899_QUIT_ARGUMENT);
    c.sp = 0xFFFF; CHECK(pr_r0899_quit_0000_cpu(ram, PR_R0899_QUIT_RAM_SIZE, &c, &env, &d) == PR_R0899_QUIT_ARGUMENT);
    c.sp = 0xF000; c.ip = 0x0001; CHECK(pr_r0899_quit_0000_cpu(ram, PR_R0899_QUIT_RAM_SIZE, &c, &env, &d) == PR_R0899_QUIT_ARGUMENT);
    c.ip = 0x0000; CHECK(memcmp(&c, &before, sizeof c) == 0 && ram[p(DS, 0x5372)] == 0xEE);

    free(ram);
    printf("quit 0000: %u semantic checks passed\n", checks);
    return 0;
}
