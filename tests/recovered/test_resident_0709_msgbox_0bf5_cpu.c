/* Focused synthetic test for the raw-CPU resident 0709:0BF5 message-line prompt (docs/re/RES_0709_MSGBOX_0BF5_CPU.md). No game bytes: the relocated
 * segment words, near-call displacements, the callees' code words and ReadKey's own words and literal are this test's. */
#include "poolrad/recovered/resident_0709_msgbox_0bf5_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks, outs, writes;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "0BF5 line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
#define RAM PR_BUS_RAM_SIZE
static size_t p(unsigned s, unsigned o) { return ((size_t)s * 16u + (o & 65535u)) & 1048575u; }
static void put(uint8_t *m, unsigned s, unsigned o, unsigned v) { size_t a = p(s, o); m[a] = (uint8_t)v; m[(a + 1u) & 1048575u] = (uint8_t)(v >> 8); }

enum { CS = 0x8800, RK = 0x1899, DRV = 0x18AE, DOS = 0x1A32, SYS = 0x1AF8, CRT = 0x1A40, DS = 0x1C7C, SS = 0x2000, FONT = 0x7400, PSP = 0x6800, MSG = 0x4800, SP0 = 0xF000 };
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
static int port_out_k(void *ctx, uint16_t port, uint8_t value) { (void)ctx; (void)port; (void)value; return 1; }
static int port_in_k(void *ctx, uint16_t port, uint8_t *value) { (void)ctx; (void)port; (void)value; return 1; }
static int gettime(void *ctx, const pr_dos_gettime_cpu *before, pr_dos_gettime_cpu *after) { (void)ctx; (void)before; (void)after; return 1; }
static int intr_refuse(void *ctx, uint8_t *ram, const pr_dos_intr_cpu *before, pr_dos_intr_cpu *after) { (void)ctx; (void)ram; (void)before; (void)after; return 1; }
static int out(void *ctx, uint16_t port, uint16_t value, unsigned size, uint16_t cs, uint16_t ip)
{ (void)ctx; (void)port; (void)value; (void)size; (void)cs; (void)ip; ++outs; return 0; }
static void written(void *ctx, uint16_t offset, uint8_t value) { (void)ctx; (void)offset; (void)value; ++writes; }

static pr_r0709_0bf5_cpu setup(pr_bus *b, const char *text, unsigned n, unsigned color, const uint16_t *keys, unsigned nk)
{
    static const uint16_t sites[8][2] = {{0x0063, CRT}, {0x0082, DRV}, {0x0097, SYS}, {0x00A4, DRV}, {0x00B3, SYS}, {0x00BD, SYS}, {0x00E7, CRT}, {0x00F0, CRT}};
    pr_r0709_0bf5_cpu c = {0x1234, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x789A, SP0, CS, 0x0BF5, DS, 0x8888, SS, 0x0202};
    uint8_t *m = b->ram;
    unsigned i;
    memset(m, 0x5A, RAM); pr_ega_reset(b->ega); memset(b->ega->plane, 0xA5, sizeof b->ega->plane);
    put(m, SS, SP0, 0x7000); put(m, SS, SP0 + 2, 0x9001); put(m, SS, SP0 + 4, 0x0100); put(m, SS, SP0 + 6, MSG); put(m, SS, SP0 + 8, 0xAB00 | color);
    m[p(MSG, 0x0100)] = (uint8_t)n;
    for (i = 0; i < n; ++i) m[p(MSG, 0x0101 + i)] = (uint8_t)text[i % strlen(text)];
    /* display data (the glyph writer's) */
    m[p(DS, 0x4A1E)] = 1; put(m, DS, 0x4A16, 0xA000); put(m, DS, 0x4A18, 0xA200); put(m, DS, 0x5E20, 0x1000); put(m, DS, 0x5E22, FONT);
    for (i = 0; i < 4u; ++i) m[p(DS, 0x25C9 + i)] = (uint8_t)(1u << i);
    for (i = 0; i < 256u; ++i) m[p(DS, 0x25D5 + i)] = (uint8_t)(i % 4u);
    for (i = 0; i < 4u; ++i) put(m, DS, 0x25CD + 2u * i, i * 0x5555u);
    for (i = 0; i < 8u; ++i) put(m, DS, 0x25E5 + 2u * i, 0xC000u >> (2u * i));
    for (i = 0; i < 2048u; ++i) m[p(FONT, 0x1000 + i)] = (uint8_t)((i * 37u + (i >> 3) * 13u) ^ 0xA6u);
    /* 0709 code words: this body's five, 05EB's near call to 01DF, 0626's StrStore call and near call, 01DF's UpCase call */
    put(m, CS, 0x0C0A, 0x0B25); put(m, CS, 0x0C0C, SYS); put(m, CS, 0x0C1C, (0x05EB - 0x0C1E) & 0xFFFFu); put(m, CS, 0x0C2F, (0x0626 - 0x0C31) & 0xFFFFu);
    put(m, CS, 0x0C32, 0x0059); put(m, CS, 0x0C34, RK);
    m[p(CS, 0x061F)] = 0xE8; put(m, CS, 0x0620, 0xFBBD);
    m[p(CS, 0x063C)] = 0x9A; put(m, CS, 0x063D, 0x0B25); put(m, CS, 0x063F, SYS);
    m[p(CS, 0x0691)] = 0xE8; put(m, CS, 0x0692, 0xFB4B);
    m[p(CS, 0x0207)] = 0x9A; put(m, CS, 0x0208, 0x182A); put(m, CS, 0x020A, SYS);
    /* ReadKey's code words, set and literal (the model reads all of them live) */
    for (i = 0; i < 8u; ++i) put(m, RK, sites[i][0], sites[i][1]);
    put(m, RK, 0x0010, DRV); put(m, RK, 0x0028, DOS); put(m, RK, 0x002F, SYS);
    memset(m + p(RK, 0x0033), 0, 32); m[p(RK, 0x0033)] = 0x03;
    memcpy(m + p(RK, 0x0053), "\x05" "HELLO", 6);
    put(m, DS, 0x4422, PSP); m[p(PSP, 0x80)] = 0; m[p(PSP, 0x81)] = 0x0D;
    m[p(DS, 0x8501)] = 0; m[p(DS, 0x8502)] = 0; m[p(DS, 0x49F7)] = 0;
    m[p(DS, 0x5372)] = 1; m[p(DS, 0x5373)] = 0; put(m, DS, 0x5374, 0); put(m, DS, 0x2612, 1);
    for (i = 0; i < nk; ++i) script[i] = keys[i];
    nkeys = nk; taken = 0; outs = writes = 0;
    return c;
}
static pr_r0709_0bf5_env env_(unsigned limit, size_t space, size_t printer, size_t glyph)
{
    pr_r0709_0bf5_env e = {{{{NULL, dos_vector, port_out_k, port_in_k, gettime, 1000u}, intr_refuse, NULL}, keyboard, NULL, limit}, space, printer, glyph};
    return e;
}

int main(void)
{
    uint8_t *ram = malloc(RAM), *saved = malloc(RAM);
    pr_ega *ega = malloc(sizeof *ega);
    pr_port_env ports = {NULL, out, NULL};
    pr_bus bus = {ram, ega, &ports, written, NULL};
    pr_r0709_0bf5_env env = env_(8u, 65536u, 65536u, 65536u);
    pr_r0709_0bf5_diagnostic d;
    pr_r0709_0bf5_cpu c, before;
    static const uint16_t a[] = {0x1E61}, axa[] = {0x1E61, 0x2D78, 0x1E61};
    unsigned k, plane;
    CHECK(ram && saved && ega);

    /* "Hi", colour 0Eh, key 'a': StrStore copies the message, 05EB clears the row, 0626 prints it, ReadKey waits, the key is stored in [BP-2Ah]. */
    c = setup(&bus, "Hi", 2, 0x0E, a, 1);
    CHECK(pr_r0709_msgbox_0bf5(&bus, RAM, &c, &env, &d) == PR_R0709_0BF5_RETURNED && d.key_status == PR_R0899_KEY_RETURN);
    CHECK(c.cs == 0x9001 && c.ip == 0x7000 && c.sp == SP0 + 10 && c.bp == 0x789A && taken == 1);
    CHECK(ram[p(SS, SP0 - 2 - 0x2A)] == 0x61 && ram[p(SS, SP0 - 2 - 0x29)] == 2 && ram[p(SS, SP0 - 2 - 0x28)] == 'H');   /* the key; the Pascal copy */
    CHECK(d.space.callee == 0 && d.space.status == 0 && d.printer.callee == 1 && d.printer.status == 0 && d.callee_status == -1 && d.site == 0);
    CHECK(outs > 0 && writes > 0 && ega->seq[2] == 15);
    for (plane = 0; plane < 4u; ++plane) CHECK(ega->plane[plane][0x283] != 0xA5 || ega->plane[plane][0x284] != 0xA5 || writes > 0);
    /* A message longer than 40 is truncated by StrStore to 40 (the length byte of the copy). */
    c = setup(&bus, "0123456789", 60, 0x0E, a, 1);
    CHECK(pr_r0709_msgbox_0bf5(&bus, RAM, &c, &env, &d) == PR_R0709_0BF5_RETURNED && ram[p(SS, SP0 - 2 - 0x29)] == 40);
    /* The empty message draws only the row clear. */
    c = setup(&bus, "x", 0, 0x0E, a, 1);
    CHECK(pr_r0709_msgbox_0bf5(&bus, RAM, &c, &env, &d) == PR_R0709_0BF5_RETURNED && ram[p(SS, SP0 - 2 - 0x29)] == 0);
    /* Type-ahead: ReadKey drains, keeping the last key; the prompt stores it. */
    c = setup(&bus, "Hi", 2, 0x0E, axa, 3);
    CHECK(pr_r0709_msgbox_0bf5(&bus, RAM, &c, &env, &d) == PR_R0709_0BF5_RETURNED && taken == 3 && ram[p(SS, SP0 - 2 - 0x2A)] == 0x61);
    /* 05EB stops (glyph budget 0): SPACE with 05EB's own status; neither 0626 nor ReadKey runs. */
    env = env_(8u, 0u, 65536u, 65536u); c = setup(&bus, "Hi", 2, 0x0E, a, 1);
    CHECK(pr_r0709_msgbox_0bf5(&bus, RAM, &c, &env, &d) == PR_R0709_0BF5_SPACE && d.site == 0x0C1B && d.callee_status == PR_R0709_SPACE_CPU_CALLEE && taken == 0);
    /* 0626 stops (outer budget 0): PRINTER with 0626's status; ReadKey never runs. */
    env = env_(8u, 65536u, 0u, 65536u); c = setup(&bus, "Hi", 2, 0x0E, a, 1);
    CHECK(pr_r0709_msgbox_0bf5(&bus, RAM, &c, &env, &d) == PR_R0709_0BF5_PRINTER && d.site == 0x0C2E && d.callee_status == PR_R0709_PRINTER_CPU_LIMIT && taken == 0);
    /* ReadKey stops (drain limit 1 with three keys pending): KEY, naming ReadKey's status. */
    env = env_(1u, 65536u, 65536u, 65536u); c = setup(&bus, "Hi", 2, 0x0E, axa, 3);
    CHECK(pr_r0709_msgbox_0bf5(&bus, RAM, &c, &env, &d) == PR_R0709_0BF5_KEY && d.key_status == PR_R0899_KEY_LIMIT && d.site == 0x0C31);
    env = env_(8u, 65536u, 65536u, 65536u);
    /* A near call that does not reach 05EB or 0626 is CALLEE, before the callee runs. */
    c = setup(&bus, "Hi", 2, 0x0E, a, 1); put(ram, CS, 0x0C1C, 0x1234);
    CHECK(pr_r0709_msgbox_0bf5(&bus, RAM, &c, &env, &d) == PR_R0709_0BF5_CALLEE && d.site == 0x0C1B && outs == 0 && writes == 0);
    c = setup(&bus, "Hi", 2, 0x0E, a, 1); put(ram, CS, 0x0C2F, 0x1234);
    CHECK(pr_r0709_msgbox_0bf5(&bus, RAM, &c, &env, &d) == PR_R0709_0BF5_CALLEE && d.site == 0x0C2E && taken == 0);
    /* ARGUMENT: wrong IP, entry SP outside [016A,FFF6], bad host objects; nothing changes. */
    for (k = 0; k < 7; ++k) {
        c = setup(&bus, "Hi", 2, 0x0E, a, 1);
        if (k == 0) c.ip = 0x0BF6;
        if (k == 1) c.sp = 0x0169;
        if (k == 2) c.sp = 0xFFF7;
        before = c; memcpy(saved, ram, RAM);
        CHECK(pr_r0709_msgbox_0bf5(k == 3 ? NULL : &bus, k == 4 ? 1 : RAM, k == 5 ? NULL : &c, k == 6 ? NULL : &env, NULL) == PR_R0709_0BF5_ARGUMENT);
        if (k != 5) CHECK(memcmp(&before, &c, sizeof c) == 0);
        CHECK(memcmp(saved, ram, RAM) == 0 && outs == 0 && writes == 0 && taken == 0);
    }
    free(saved); free(ram); free(ega);
    printf("%u semantic checks\n", checks);
    return 0;
}
