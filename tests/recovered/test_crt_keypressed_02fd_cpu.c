/* Focused synthetic test for the raw-CPU Crt.KeyPressed 0A40:02FD (docs/re/CRT_KEYPRESSED_02FD_CPU.md). No game bytes. */
#include "poolrad/recovered/crt_keyboard_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "keypressed line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
static size_t p(unsigned s, unsigned o) { return ((size_t)s * 16u + (o & 65535u)) & 1048575u; }
static void put(uint8_t *m, unsigned s, unsigned o, unsigned v) { size_t a = p(s, o); m[a] = (uint8_t)v; m[(a + 1u) & 1048575u] = (uint8_t)(v >> 8); }

enum { SS = 0x2000, DS = 0x5000 };
static int mode, calls;  /* mode 0 no key, 1 key pending, 2 decline, 3 wrong IP */
static pr_crt_cpu at_int;
static int service(void *ctx, const pr_crt_cpu *before, pr_crt_cpu *after)
{
    (void)ctx;
    ++calls; at_int = *before;
    if (mode == 2) return 1;
    *after = *before;
    after->ax = 0x1E61;                                                   /* 'a', scan 1Eh */
    after->flags = (uint16_t)((before->flags & ~0x0040u) | (mode == 1 ? 0u : 0x0040u));
    after->ip = mode == 3 ? 0x0307u : 0x0308u;
    return 0;
}
static pr_crt_cpu setup(uint8_t *ram, unsigned buffered, uint16_t flags)
{
    pr_crt_cpu c = {0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, 0x8000, 0x6000, 0x02FD, DS, 0x1111, SS, 0};
    memset(ram, 0x5A, PR_CRT_KEYBOARD_RAM_SIZE);
    ram[p(DS, 0x8501)] = (uint8_t)buffered;
    put(ram, SS, 0x8000, 0x7000); put(ram, SS, 0x8002, 0x9001);
    c.flags = flags; mode = 0; calls = 0;
    return c;
}

int main(void)
{
    uint8_t *ram = malloc(PR_CRT_KEYBOARD_RAM_SIZE);
    pr_crt_cpu c, before;
    CHECK(ram != NULL);

    /* Buffered extended key 3Bh: AL := 1, no BIOS call; CMP 3Bh,0 leaves ZF SF PF clear (3Bh has five bits set). */
    c = setup(ram, 0x3B, 0x08D7);
    CHECK(pr_crt_keypressed_02fd(ram, PR_CRT_KEYBOARD_RAM_SIZE, &c, service, NULL) == PR_CRT_RETURNED);
    CHECK(calls == 0 && c.ax == 0x1201 && c.flags == 0x0002);
    CHECK(c.cs == 0x9001 && c.ip == 0x7000 && c.sp == 0x8004);
    /* Buffered 80h: SF set, PF clear (one bit). */
    c = setup(ram, 0x80, 0x0002);
    CHECK(pr_crt_keypressed_02fd(ram, PR_CRT_KEYBOARD_RAM_SIZE, &c, service, NULL) == PR_CRT_RETURNED && c.flags == 0x0082);

    /* Nothing buffered, BIOS reports a key: the INT sees AH=01h at 0306h; AL := 1, AH as the BIOS left it,
     * flags from CMP 0,0 (ZF PF set: 0046h) with ZF then cleared by the BIOS: 0006h. */
    c = setup(ram, 0x00, 0x0002); mode = 1;
    CHECK(pr_crt_keypressed_02fd(ram, PR_CRT_KEYBOARD_RAM_SIZE, &c, service, NULL) == PR_CRT_RETURNED);
    CHECK(calls == 1 && at_int.ip == 0x0306 && (at_int.ax >> 8) == 0x01 && at_int.flags == 0x0046);
    CHECK(c.ax == 0x1E01 && c.flags == 0x0006 && c.ip == 0x7000 && c.sp == 0x8004);
    /* Nothing buffered, no key: AL := 0, ZF still set. */
    c = setup(ram, 0x00, 0x0002);
    CHECK(pr_crt_keypressed_02fd(ram, PR_CRT_KEYBOARD_RAM_SIZE, &c, service, NULL) == PR_CRT_RETURNED);
    CHECK(c.ax == 0x1E00 && c.flags == 0x0046);

    /* SERVICE: declined, or a result that is not the continuation; stopped at the INT. */
    c = setup(ram, 0x00, 0x0002); mode = 2;
    CHECK(pr_crt_keypressed_02fd(ram, PR_CRT_KEYBOARD_RAM_SIZE, &c, service, NULL) == PR_CRT_SERVICE && c.ip == 0x0306);
    c = setup(ram, 0x00, 0x0002); mode = 3;
    CHECK(pr_crt_keypressed_02fd(ram, PR_CRT_KEYBOARD_RAM_SIZE, &c, service, NULL) == PR_CRT_SERVICE && c.ip == 0x0306);

    /* ARGUMENT: rejected before any change. */
    c = setup(ram, 0x00, 0x0002); before = c;
    CHECK(pr_crt_keypressed_02fd(NULL, PR_CRT_KEYBOARD_RAM_SIZE, &c, service, NULL) == PR_CRT_ARGUMENT);
    CHECK(pr_crt_keypressed_02fd(ram, PR_CRT_KEYBOARD_RAM_SIZE - 1u, &c, service, NULL) == PR_CRT_ARGUMENT);
    CHECK(pr_crt_keypressed_02fd(ram, PR_CRT_KEYBOARD_RAM_SIZE, &c, NULL, NULL) == PR_CRT_ARGUMENT);
    c.sp = 0xFFFD; CHECK(pr_crt_keypressed_02fd(ram, PR_CRT_KEYBOARD_RAM_SIZE, &c, service, NULL) == PR_CRT_ARGUMENT);
    c.sp = 0x8000; c.ip = 0x030F; CHECK(pr_crt_keypressed_02fd(ram, PR_CRT_KEYBOARD_RAM_SIZE, &c, service, NULL) == PR_CRT_ARGUMENT);
    c.ip = 0x02FD; CHECK(memcmp(&c, &before, sizeof c) == 0 && calls == 0);

    free(ram);
    printf("keypressed 02FD: %u semantic checks passed\n", checks);
    return 0;
}
