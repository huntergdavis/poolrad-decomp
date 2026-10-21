/* Focused synthetic test for the raw-CPU Crt.ReadKey 0A40:030F + 0143 (docs/re/CRT_READKEY_030F_CPU.md). No game bytes. */
#include "poolrad/recovered/crt_keyboard_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "readkey line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
static size_t p(unsigned s, unsigned o) { return ((size_t)s * 16u + (o & 65535u)) & 1048575u; }
static void put(uint8_t *m, unsigned s, unsigned o, unsigned v) { size_t a = p(s, o); m[a] = (uint8_t)v; m[(a + 1u) & 1048575u] = (uint8_t)(v >> 8); }
static unsigned get(const uint8_t *m, unsigned s, unsigned o) { size_t a = p(s, o); return m[a] | (unsigned)m[(a + 1u) & 1048575u] << 8; }

enum { SS = 0x2000, DS = 0x5000 };
static int mode, calls;  /* mode 0 answer, 1 decline, 2 wrong IP */
static uint16_t key;
static pr_crt_cpu at_int;
static int service(void *ctx, const pr_crt_cpu *before, pr_crt_cpu *after)
{
    (void)ctx;
    ++calls; at_int = *before;
    if (mode == 1) return 1;
    *after = *before; after->ax = key; after->ip = mode == 2 ? 0x0320u : 0x031Fu;
    return 0;
}
static pr_crt_cpu setup(uint8_t *ram, unsigned buffered, unsigned brk, uint16_t k)
{
    pr_crt_cpu c = {0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, 0x8000, 0x6000, 0x030F, DS, 0x1111, SS, 0x0002};
    memset(ram, 0x5A, PR_CRT_KEYBOARD_RAM_SIZE);
    ram[p(DS, 0x8501)] = (uint8_t)buffered; ram[p(DS, 0x8502)] = (uint8_t)brk;
    put(ram, SS, 0x8000, 0x7000); put(ram, SS, 0x8002, 0x9001);
    key = k; mode = 0; calls = 0;
    return c;
}
static pr_crt_status run(uint8_t *ram, pr_crt_cpu *c) { return pr_crt_readkey_030f(ram, PR_CRT_KEYBOARD_RAM_SIZE, c, service, NULL); }

int main(void)
{
    uint8_t *ram = malloc(PR_CRT_KEYBOARD_RAM_SIZE);
    pr_crt_cpu c, before;
    CHECK(ram != NULL);

    /* Buffered scan code 3Bh: AL := 3Bh, the byte cleared, no BIOS call; final flags from CMP [8502],0 with
     * [8502] = 0 (ZF PF set): 0046h. The CALL 0143 return word 0330h stays below the popped frame. */
    c = setup(ram, 0x3B, 0, 0);
    CHECK(run(ram, &c) == PR_CRT_RETURNED && calls == 0);
    CHECK(c.ax == 0x123B && ram[p(DS, 0x8501)] == 0 && c.flags == 0x0046);
    CHECK(c.cs == 0x9001 && c.ip == 0x7000 && c.sp == 0x8004 && get(ram, SS, 0x7FFE) == 0x0330);

    /* Ordinary key 'a' (1E61h): INT 16h at 031Dh with AH 00h and the XOR AH,AH flags (0046h); AX as the BIOS gave it. */
    c = setup(ram, 0, 0, 0x1E61);
    CHECK(run(ram, &c) == PR_CRT_RETURNED && calls == 1);
    CHECK(at_int.ip == 0x031D && (at_int.ax >> 8) == 0 && at_int.flags == 0x0046);
    CHECK(c.ax == 0x1E61 && ram[p(DS, 0x8501)] == 0);
    /* Extended key F1 (3B00h): AL 0 returned, the scan code buffered for the next call. */
    c = setup(ram, 0, 0, 0x3B00);
    CHECK(run(ram, &c) == PR_CRT_RETURNED && c.ax == 0x3B00 && ram[p(DS, 0x8501)] == 0x3B);
    /* AL and AH both 0: AL := 3 (the synthesised Ctrl-C), nothing buffered. */
    c = setup(ram, 0, 0, 0x0000);
    CHECK(run(ram, &c) == PR_CRT_RETURNED && c.ax == 0x0003 && ram[p(DS, 0x8501)] == 0);

    /* BREAK (EXIT class): [8502] = 1 stops at 0A40:014B after CALL 0143, the return word pushed; CMP 1,0 flags 0002h. */
    c = setup(ram, 0, 1, 0x1E61);
    CHECK(run(ram, &c) == PR_CRT_BREAK);
    CHECK(c.cs == 0x6000 && c.ip == 0x014B && c.sp == 0x7FFE && get(ram, SS, 0x7FFE) == 0x0330 && c.flags == 0x0002);
    CHECK(c.ax == 0x1E61);

    /* SERVICE: declined, or a non-continuation; stopped at the INT. */
    c = setup(ram, 0, 0, 0x1E61); mode = 1;
    CHECK(run(ram, &c) == PR_CRT_SERVICE && c.ip == 0x031D);
    c = setup(ram, 0, 0, 0x1E61); mode = 2;
    CHECK(run(ram, &c) == PR_CRT_SERVICE && c.ip == 0x031D);

    /* ARGUMENT: rejected before any change, including SP outside [0002h,FFFCh]. */
    c = setup(ram, 0x3B, 0, 0); before = c;
    CHECK(pr_crt_readkey_030f(NULL, PR_CRT_KEYBOARD_RAM_SIZE, &c, service, NULL) == PR_CRT_ARGUMENT);
    CHECK(pr_crt_readkey_030f(ram, PR_CRT_KEYBOARD_RAM_SIZE - 1u, &c, service, NULL) == PR_CRT_ARGUMENT);
    CHECK(pr_crt_readkey_030f(ram, PR_CRT_KEYBOARD_RAM_SIZE, &c, NULL, NULL) == PR_CRT_ARGUMENT);
    c.sp = 0x0001; CHECK(run(ram, &c) == PR_CRT_ARGUMENT);
    c.sp = 0xFFFD; CHECK(run(ram, &c) == PR_CRT_ARGUMENT);
    c.sp = 0x8000; c.ip = 0x0310; CHECK(run(ram, &c) == PR_CRT_ARGUMENT);
    c.ip = 0x030F; CHECK(memcmp(&c, &before, sizeof c) == 0 && ram[p(DS, 0x8501)] == 0x3B);

    free(ram);
    printf("readkey 030F: %u semantic checks passed\n", checks);
    return 0;
}
