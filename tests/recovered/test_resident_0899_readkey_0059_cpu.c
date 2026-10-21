/* Focused synthetic test for the raw-CPU resident 0899:0059 ReadKey-with-hotkeys (docs/re/RES_0899_READKEY_0059_CPU.md).
 * No game bytes: the eight segment words, the CS set and a CS literal of this test's own ("HELLO", standing where the game
 * keeps its literal) are placed; the model reads all of them live. */
#include "poolrad/recovered/resident_0899_readkey_0059_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "readkey 0059 line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
static size_t p(unsigned s, unsigned o) { return ((size_t)s * 16u + (o & 65535u)) & 1048575u; }
static void put(uint8_t *m, unsigned s, unsigned o, unsigned v) { size_t a = p(s, o); m[a] = (uint8_t)v; m[(a + 1u) & 1048575u] = (uint8_t)(v >> 8); }

enum { BODY = 0x1899, DRV = 0x18AE, DOS = 0x1A32, SYS = 0x1AF8, CRT = 0x1A40, DS = 0x1C7C, SS = 0x7000, HSEG = 0x6000, PSP = 0x6800 };
static uint16_t script[8]; static unsigned nkeys, taken;
static int keyboard(void *ctx, const pr_crt_cpu *before, pr_crt_cpu *after)
{
    (void)ctx;
    *after = *before; after->ip = (uint16_t)(before->ip + 2u);
    if ((before->ax >> 8) == 0x01u) {
        if (taken < nkeys) { after->ax = script[taken]; after->flags = (uint16_t)(before->flags & ~0x0040u); }
        else after->flags = (uint16_t)(before->flags | 0x0040u);
        return 0;
    }
    if ((before->ax >> 8) == 0x00u && taken < nkeys) { after->ax = script[taken++]; return 0; }
    return 1;
}
static int dos_vector(void *ctx, uint8_t *ram, pr_r08ae_driver_cpu *c, uint16_t *flags_out) { (void)ctx; (void)ram; (void)c; (void)flags_out; return 1; }
static int port_out(void *ctx, uint16_t port, uint8_t value) { (void)ctx; (void)port; (void)value; return 1; }
static int port_in(void *ctx, uint16_t port, uint8_t *value) { (void)ctx; (void)port; (void)value; return 1; }
static int gettime(void *ctx, const pr_dos_gettime_cpu *before, pr_dos_gettime_cpu *after) { (void)ctx; (void)before; (void)after; return 1; }
static int video(void *ctx, uint8_t *ram, const pr_dos_intr_cpu *before, pr_dos_intr_cpu *after)
{
    size_t a = p(before->ss, before->sp);
    (void)ctx;
    *after = *before;
    after->ip = (uint16_t)(ram[a] | ram[a + 1u] << 8); after->cs = (uint16_t)(ram[a + 2u] | ram[a + 3u] << 8);
    after->flags = (uint16_t)(ram[a + 4u] | ram[a + 5u] << 8); after->sp = (uint16_t)(before->sp + 6u);
    return 0;
}
static pr_r0899_cpu setup(uint8_t *ram, const uint16_t *keys, unsigned n, const char *tail)
{
    static const uint16_t sites[8][2] = {{0x0063, CRT}, {0x0082, DRV}, {0x0097, SYS}, {0x00A4, DRV}, {0x00B3, SYS}, {0x00BD, SYS},
                                         {0x00E7, CRT}, {0x00F0, CRT}};
    pr_r0899_cpu c = {0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, 0xF000, BODY, 0x0059, DS, 0x1111, SS, 0x0002};
    unsigned i; size_t len = strlen(tail);
    memset(ram, 0x5A, PR_R0899_KEY_RAM_SIZE);
    for (i = 0; i < 8u; ++i) put(ram, BODY, sites[i][0], sites[i][1]);
    put(ram, BODY, 0x0010, DRV); put(ram, BODY, 0x0028, DOS); put(ram, BODY, 0x002F, SYS);   /* QUIT's three */
    memset(ram + p(BODY, 0x0033), 0, 32); ram[p(BODY, 0x0033)] = 0x03;                       /* the set {0,1} */
    memcpy(ram + p(BODY, 0x0053), "\x05" "HELLO", 6);                                          /* this test's literal */
    put(ram, 0, 0x40, 0x0000); put(ram, 0, 0x42, HSEG);
    put(ram, DS, 0x4422, PSP); ram[p(PSP, 0x80)] = (uint8_t)len; memcpy(ram + p(PSP, 0x81), tail, len); ram[p(PSP, 0x81) + len] = 0x0D;
    ram[p(DS, 0x8501)] = 0; ram[p(DS, 0x8502)] = 0; ram[p(DS, 0x49F7)] = 0;
    ram[p(DS, 0x5372)] = 1; ram[p(DS, 0x5373)] = 0; put(ram, DS, 0x5374, 0); put(ram, DS, 0x2612, 1);
    put(ram, SS, 0xF000, 0x7000); put(ram, SS, 0xF002, 0x9001);
    for (i = 0; i < n; ++i) script[i] = keys[i];
    nkeys = n; taken = 0;
    return c;
}

int main(void)
{
    uint8_t *ram = malloc(PR_R0899_KEY_RAM_SIZE);
    pr_r0899_key_env env = {{{NULL, dos_vector, port_out, port_in, gettime, 1000u}, video, NULL}, keyboard, NULL, 8u};
    pr_r0899_key_diagnostic d;
    pr_r0899_cpu c, before;
    static const uint16_t a[] = {0x1E61}, ax_[] = {0x1E61, 0x2D78}, axa[] = {0x1E61, 0x2D78, 0x1E61}, s_[] = {0x1F13},
                          d4[] = {0x2004}, c3[] = {0x2E03};
    CHECK(ram != NULL);

    /* 'a', ParamStr(1) "foo" <> "HELLO": no hotkey; the drain finds nothing (KeyPressed AL 0); AL = 61h returned.
     * The result byte [BP-1] and the key [BP-2] sit just below the saved BP; the final flags are OR AL,AL's on AL = 0. */
    c = setup(ram, a, 1, " foo");
    CHECK(pr_r0899_readkey_0059_cpu(ram, PR_R0899_KEY_RAM_SIZE, &c, &env, &d) == PR_R0899_KEY_RETURN);
    CHECK((c.ax & 0xFF) == 0x61 && c.cs == 0x9001 && c.ip == 0x7000 && c.sp == 0xF004 && c.bp == 0x7777);
    CHECK(ram[p(SS, 0xEFFD)] == 0x61 && ram[p(SS, 0xEFFC)] == 0x61 && c.flags == 0x0046);
    CHECK(d.readkeys == 1 && d.keypressed == 1);
    /* ParamStr(1) landed in the 256-byte frame string at [BP-102h] = SS:EEFC: length 3, "foo". */
    CHECK(ram[p(SS, 0xEEFC)] == 3 && memcmp(ram + p(SS, 0xEEFD), "foo", 3) == 0);

    /* Drain: 'a' then 'x' pending: KeyPressed, ReadKey 'x', KeyPressed none; AL = 78h. With drain_limit 1 and a third key: LIMIT. */
    c = setup(ram, ax_, 2, "");
    CHECK(pr_r0899_readkey_0059_cpu(ram, PR_R0899_KEY_RAM_SIZE, &c, &env, &d) == PR_R0899_KEY_RETURN && (c.ax & 0xFF) == 0x78);
    env.drain_limit = 1u; c = setup(ram, axa, 3, "");
    CHECK(pr_r0899_readkey_0059_cpu(ram, PR_R0899_KEY_RAM_SIZE, &c, &env, &d) == PR_R0899_KEY_LIMIT && c.ip == 0x00E4 && taken == 2);
    env.drain_limit = 8u;

    /* Ctrl-S with sound on ([5372] = 1): [5373] := 1, 00DC(1) with [5374] = 0 runs only 181A, then [5372] := 2. */
    c = setup(ram, s_, 1, "");
    CHECK(pr_r0899_readkey_0059_cpu(ram, PR_R0899_KEY_RAM_SIZE, &c, &env, &d) == PR_R0899_KEY_RETURN && (c.ax & 0xFF) == 0x13);
    CHECK(ram[p(DS, 0x5372)] == 2 && ram[p(DS, 0x5373)] == 1 && d.driver.calls == 1);
    /* Ctrl-S muted with [5373] = 5 (not in {0,1}): nothing changes and no driver call. */
    c = setup(ram, s_, 1, ""); ram[p(DS, 0x5372)] = 2; ram[p(DS, 0x5373)] = 5;
    CHECK(pr_r0899_readkey_0059_cpu(ram, PR_R0899_KEY_RAM_SIZE, &c, &env, &d) == PR_R0899_KEY_RETURN);
    CHECK(ram[p(DS, 0x5372)] == 2 && d.driver.calls == 0);
    /* Ctrl-S muted with [5373] = 1: [5372] := 1 and 017B runs; its first callee 137C asks DOS, which refuses: DRIVER. */
    c = setup(ram, s_, 1, ""); ram[p(DS, 0x5372)] = 2; ram[p(DS, 0x5373)] = 1;
    CHECK(pr_r0899_readkey_0059_cpu(ram, PR_R0899_KEY_RAM_SIZE, &c, &env, &d) == PR_R0899_KEY_DRIVER);
    CHECK(d.callee == 0x00A1 && d.driver.member == 0x137C && ram[p(DS, 0x5372)] == 1);

    /* The literal matches ParamStr(1): key 04 toggles [49F7] 0 -> 1; key 03 calls QUIT, which reaches Halt: EXIT. */
    c = setup(ram, d4, 1, " HELLO");
    CHECK(pr_r0899_readkey_0059_cpu(ram, PR_R0899_KEY_RAM_SIZE, &c, &env, &d) == PR_R0899_KEY_RETURN && ram[p(DS, 0x49F7)] == 1);
    c = setup(ram, d4, 1, "HELLO"); ram[p(DS, 0x49F7)] = 7;
    CHECK(pr_r0899_readkey_0059_cpu(ram, PR_R0899_KEY_RAM_SIZE, &c, &env, &d) == PR_R0899_KEY_RETURN && ram[p(DS, 0x49F7)] == 0);
    c = setup(ram, c3, 1, " HELLO"); ram[p(DS, 0x5373)] = 2;   /* QUIT restores [5372] := 2: 00DC returns at once */
    CHECK(pr_r0899_readkey_0059_cpu(ram, PR_R0899_KEY_RAM_SIZE, &c, &env, &d) == PR_R0899_KEY_EXIT);
    CHECK(c.cs == SYS && c.ip == 0x00E9 && c.ax == 0 && ram[p(DS, 0x5372)] == 2 && d.callee == 0x00DB);
    /* The same key without the literal: no QUIT; the key is returned. */
    c = setup(ram, c3, 1, " hello");
    CHECK(pr_r0899_readkey_0059_cpu(ram, PR_R0899_KEY_RAM_SIZE, &c, &env, &d) == PR_R0899_KEY_RETURN && (c.ax & 0xFF) == 0x03);

    /* A pending Ctrl-Break: the first ReadKey stops at 0A40:014B, EXIT class. */
    c = setup(ram, a, 1, ""); ram[p(DS, 0x8502)] = 1;
    CHECK(pr_r0899_readkey_0059_cpu(ram, PR_R0899_KEY_RAM_SIZE, &c, &env, &d) == PR_R0899_KEY_BREAK && c.cs == CRT && c.ip == 0x014B);

    /* ARGUMENT: rejected before any change. */
    c = setup(ram, a, 1, ""); before = c;
    CHECK(pr_r0899_readkey_0059_cpu(NULL, PR_R0899_KEY_RAM_SIZE, &c, &env, &d) == PR_R0899_KEY_ARGUMENT);
    c.sp = 0x0111; CHECK(pr_r0899_readkey_0059_cpu(ram, PR_R0899_KEY_RAM_SIZE, &c, &env, &d) == PR_R0899_KEY_ARGUMENT);
    c.sp = 0xF000; c.ip = 0x005A; CHECK(pr_r0899_readkey_0059_cpu(ram, PR_R0899_KEY_RAM_SIZE, &c, &env, &d) == PR_R0899_KEY_ARGUMENT);
    c.ip = 0x0059; CHECK(memcmp(&c, &before, sizeof c) == 0 && taken == 0);

    free(ram);
    printf("readkey 0059: %u semantic checks passed\n", checks);
    return 0;
}
