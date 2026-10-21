/* Focused synthetic test for the raw-CPU resident 0802:0193 status-line prompt (docs/re/RES_0802_0193_CPU.md). No game bytes: the relocated
 * segment words, the near-call displacements and ReadKey's own code words and literal are this test's. */
#include "poolrad/recovered/resident_0802_0193_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "0193 line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
#define RAM PR_R0802_0193_RAM_SIZE
static size_t p(unsigned s, unsigned o) { return ((size_t)s * 16u + (o & 65535u)) & 1048575u; }
static void put(uint8_t *m, unsigned s, unsigned o, unsigned v) { size_t a = p(s, o); m[a] = (uint8_t)v; m[(a + 1u) & 1048575u] = (uint8_t)(v >> 8); }
static unsigned get(const uint8_t *m, unsigned s, unsigned o) { size_t a = p(s, o); return m[a] | (unsigned)m[(a + 1u) & 1048575u] << 8; }

/* CS = the 0802 code segment of this test; RK = the 0899 segment holding ReadKey's code words. */
enum { CS = 0x5800, RK = 0x1899, DRV = 0x18AE, DOS = 0x1A32, SYS = 0x1AF8, CRT = 0x1A40, DS = 0x1C7C, SS = 0x7000, HSEG = 0x6000, PSP = 0x6800, MSG = 0x4800, SP0 = 0xF000 };
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
static struct { unsigned ax, bx, cx, dx; } calls[400];
static unsigned ncalls;
static int refuse_at;
static int video(void *ctx, uint8_t *ram, const pr_dos_intr_cpu *before, pr_dos_intr_cpu *after)
{
    (void)ctx;
    if (ncalls < 400) { calls[ncalls].ax = before->ax; calls[ncalls].bx = before->bx; calls[ncalls].cx = before->cx; calls[ncalls].dx = before->dx; }
    ++ncalls;
    if (refuse_at && (int)ncalls == refuse_at) return 1;
    *after = *before;
    after->ip = (uint16_t)get(ram, before->ss, before->sp); after->cs = (uint16_t)get(ram, before->ss, before->sp + 2u);
    after->flags = (uint16_t)get(ram, before->ss, before->sp + 4u); after->sp = (uint16_t)(before->sp + 6u);
    return 0;
}
static pr_r0802_0193_cpu setup(uint8_t *ram, unsigned ega, const char *text, unsigned n, unsigned attr, const uint16_t *keys, unsigned nk)
{
    static const uint16_t sites[8][2] = {{0x0063, CRT}, {0x0082, DRV}, {0x0097, SYS}, {0x00A4, DRV}, {0x00B3, SYS}, {0x00BD, SYS}, {0x00E7, CRT}, {0x00F0, CRT}};
    pr_r0802_0193_cpu c = {0x1234, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x789A, SP0, CS, 0x0193, DS, 0x8888, SS, 0x0202};
    unsigned i;
    memset(ram, 0x5A, RAM);
    put(ram, SS, SP0, 0x7000); put(ram, SS, SP0 + 2, 0x9001); put(ram, SS, SP0 + 4, 0x0100); put(ram, SS, SP0 + 6, MSG); put(ram, SS, SP0 + 8, 0xAB00 | attr);
    ram[p(MSG, 0x0100)] = (uint8_t)n;
    for (i = 0; i < n; ++i) ram[p(MSG, 0x0101 + i)] = (uint8_t)text[i % strlen(text)];
    /* display data */
    ram[p(DS, 0x4A1A)] = 7; ram[p(DS, 0x4A1B)] = 3; ram[p(DS, 0x4A1E)] = (uint8_t)ega;
    for (i = 0; i < 256u; ++i) ram[p(DS, 0x25D5 + i)] = (uint8_t)(0xA0 + i);
    put(ram, 0, 0x40, 0x0000); put(ram, 0, 0x42, HSEG);
    /* 0802 code words: 0040's four Intr segments, 0105's StrStore call and near call, 0175's near call, 0193's own five */
    put(ram, CS, 0x65, DOS); put(ram, CS, 0xA8, DOS); put(ram, CS, 0xD3, DOS); put(ram, CS, 0xFF, DOS);
    put(ram, CS, 0x11C, 0x0B25); put(ram, CS, 0x11E, SYS); put(ram, CS, 0x160, (0x0040 - 0x0162) & 0xFFFFu); put(ram, CS, 0x18D, (0x0040 - 0x018F) & 0xFFFFu);
    put(ram, CS, 0x1A8, 0x0B25); put(ram, CS, 0x1AA, SYS); put(ram, CS, 0x1BA, (0x0175 - 0x01BC) & 0xFFFFu); put(ram, CS, 0x1CD, (0x0105 - 0x01CF) & 0xFFFFu);
    put(ram, CS, 0x1D0, 0x0059); put(ram, CS, 0x1D2, RK); put(ram, CS, 0x1E5, (0x0175 - 0x01E7) & 0xFFFFu);
    /* ReadKey's code words, set and literal (the model reads all of them live) */
    for (i = 0; i < 8u; ++i) put(ram, RK, sites[i][0], sites[i][1]);
    put(ram, RK, 0x0010, DRV); put(ram, RK, 0x0028, DOS); put(ram, RK, 0x002F, SYS);
    memset(ram + p(RK, 0x0033), 0, 32); ram[p(RK, 0x0033)] = 0x03;
    memcpy(ram + p(RK, 0x0053), "\x05" "HELLO", 6);
    put(ram, DS, 0x4422, PSP); ram[p(PSP, 0x80)] = 0; ram[p(PSP, 0x81)] = 0x0D;
    ram[p(DS, 0x8501)] = 0; ram[p(DS, 0x8502)] = 0; ram[p(DS, 0x49F7)] = 0;
    ram[p(DS, 0x5372)] = 1; ram[p(DS, 0x5373)] = 0; put(ram, DS, 0x5374, 0); put(ram, DS, 0x2612, 1);
    for (i = 0; i < nk; ++i) script[i] = keys[i];
    nkeys = nk; taken = 0; ncalls = 0; refuse_at = 0;
    return c;
}
static pr_r0802_0193_env env_(unsigned limit)
{
    pr_r0802_0193_env e = {{video, NULL}, {{{NULL, dos_vector, port_out, port_in, gettime, 1000u}, video, NULL}, keyboard, NULL, limit}};
    return e;
}

int main(void)
{
    uint8_t *ram = malloc(RAM), *saved = malloc(RAM);
    pr_r0802_0193_env env = env_(8u);
    pr_r0802_0193_diagnostic d;
    pr_r0802_0193_cpu c, before;
    static const uint16_t a[] = {0x1E61}, axa[] = {0x1E61, 0x2D78, 0x1E61};
    unsigned k;
    CHECK(ram && saved);

    /* "Insert disk" (11), attribute 0Eh, DS:4A1E = 0, key 'a': blank row 24 (one 0175 = 2 INT 10h calls), the message (11 characters x 2), ReadKey, blank again. */
    c = setup(ram, 0, "Insert disk", 11, 0x0E, a, 1);
    CHECK(pr_r0802_0193(ram, RAM, &c, &env, &d) == PR_R0802_0193_RETURNED && d.key_status == PR_R0899_KEY_RETURN);
    CHECK(ncalls == 2 + 22 + 2 && taken == 1);
    CHECK((calls[0].ax >> 8) == 2 && calls[0].dx == ((0x18u << 8) | 0u) && (calls[1].ax & 0xFF) == 0xDB && calls[1].cx == 0x28 && (calls[1].bx & 0xFF) == 0xA0);   /* blank: DBh, attribute 0 -> map[0] */
    CHECK((calls[3].ax & 0xFF) == 'I' && (calls[3].bx & 0xFF) == 0xAE && calls[3].cx == 1 && calls[3].dx == ((0x18u << 8) | 0u));                                        /* the message, attribute 0Eh -> map[0Eh] */
    CHECK((calls[5].ax & 0xFF) == 'n' && calls[4].dx == ((0x18u << 8) | 1u));                                                                                          /* the next column */
    CHECK((calls[25].ax & 0xFF) == 0xDB && calls[24].dx == ((0x18u << 8) | 0u) && calls[25].cx == 0x28);                                                                 /* the second blank */
    CHECK(c.cs == 0x9001 && c.ip == 0x7000 && c.sp == SP0 + 10 && c.bp == 0x789A);
    CHECK(ram[p(SS, SP0 - 2 - 0x2A)] == 0x61);                                                                                                                           /* [BP-2Ah] = the key */
    CHECK(ram[p(SS, SP0 - 2 - 0x29)] == 11);                                                                                                                             /* the Pascal copy */
    /* A message longer than 40 is truncated by StrStore to 40: 40 characters x 2. */
    c = setup(ram, 0, "0123456789", 60, 0x0E, a, 1);
    CHECK(pr_r0802_0193(ram, RAM, &c, &env, &d) == PR_R0802_0193_RETURNED && ncalls == 2 + 80 + 2 && ram[p(SS, SP0 - 2 - 0x29)] == 40);
    /* An empty message, and DS:4A1E = 1 (four INT 10h calls per character and per blank). */
    c = setup(ram, 0, "x", 0, 0x0E, a, 1);
    CHECK(pr_r0802_0193(ram, RAM, &c, &env, &d) == PR_R0802_0193_RETURNED && ncalls == 4);
    c = setup(ram, 1, "Hi", 2, 0x0E, a, 1);
    CHECK(pr_r0802_0193(ram, RAM, &c, &env, &d) == PR_R0802_0193_RETURNED && ncalls == 4 + 8 + 4);
    /* Type-ahead: ReadKey drains, keeping the last key; the prompt stores it. */
    c = setup(ram, 0, "Hi", 2, 0x0E, axa, 3);
    CHECK(pr_r0802_0193(ram, RAM, &c, &env, &d) == PR_R0802_0193_RETURNED && taken == 3 && ram[p(SS, SP0 - 2 - 0x2A)] == 0x61);
    /* ReadKey stops (drain limit 1 with three keys pending): KEY, naming ReadKey's status; the second blank never runs. */
    env = env_(1u); c = setup(ram, 0, "Hi", 2, 0x0E, axa, 3);
    CHECK(pr_r0802_0193(ram, RAM, &c, &env, &d) == PR_R0802_0193_KEY && d.key_status == PR_R0899_KEY_LIMIT && ncalls == 2 + 4);
    env = env_(8u);
    /* A refusing INT 10h handler during the first blank: DISPLAY with the 0175 diagnostic. */
    c = setup(ram, 0, "Hi", 2, 0x0E, a, 1); refuse_at = 2;
    CHECK(pr_r0802_0193(ram, RAM, &c, &env, &d) == PR_R0802_0193_DISPLAY && d.site == 0x01B9 && d.fill.display.intr_status == PR_DOS_INTR_SERVICE && taken == 0);
    /* ARGUMENT: wrong IP, entry SP outside [0190,FFF0], bad host objects; nothing changes. */
    for (k = 0; k < 7; ++k) {
        pr_r0802_0193_env bad = env_(8u);
        bad.display.video = NULL;
        c = setup(ram, 0, "Hi", 2, 0x0E, a, 1);
        if (k == 0) c.ip = 0x0194;
        if (k == 1) c.sp = 0x018F;
        if (k == 2) c.sp = 0xFFF1;
        before = c; memcpy(saved, ram, RAM);
        CHECK(pr_r0802_0193(k == 3 ? NULL : ram, k == 4 ? 1 : RAM, k == 5 ? NULL : &c, k == 6 ? &bad : &env, NULL) == PR_R0802_0193_ARGUMENT);
        if (k != 5) CHECK(memcmp(&before, &c, sizeof c) == 0);
        CHECK(memcmp(saved, ram, RAM) == 0 && ncalls == 0 && taken == 0);
    }
    free(saved); free(ram);
    printf("%u semantic checks\n", checks);
    return 0;
}
