/* Focused synthetic test for the raw-CPU resident 0802:0716 block loader (docs/re/RES_0802_0716_CPU.md). No game bytes:
 * the far-call operands are written at their sites, the archive is in memory, Str's table is the ten powers of ten. */
#include "poolrad/recovered/resident_0802_0716_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "0716 line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
static size_t p(unsigned s, unsigned o) { return ((size_t)s * 16u + (o & 65535u)) & 1048575u; }
static void put(uint8_t *m, unsigned s, unsigned o, unsigned v) { size_t a = p(s, o); m[a] = (uint8_t)v; m[(a + 1u) & 1048575u] = (uint8_t)(v >> 8); }
static unsigned get(const uint8_t *m, unsigned s, unsigned o) { size_t a = p(s, o); return m[a] | (unsigned)m[(a + 1u) & 1048575u] << 8; }
static void pstr(uint8_t *m, unsigned s, unsigned o, const char *t) { size_t n = strlen(t); m[p(s, o)] = (uint8_t)n; memcpy(m + p(s, o + 1u), t, n); }

enum { CS = 0x1802, SYS = 0x1AF8, QUITSEG = 0x1899, DS = 0x1C7C, SS = 0x7000, OUT = 0x5800, HEAP = 0x3000 };
/* Archive: header length 18 (two rows), then the rows, then the packed data. */
static const uint8_t ARCH[] = { 18, 0,  7, 0,0,0,0, 6,0, 4,0,   9, 4,0,0,0, 3,0, 0x95,0x8C,
                                0xAA, 0xBB, 0xCC, 0xDD,  0xEE, 0xFF };
/* A one-row archive whose packed size (9000h) exceeds the 8CA0h limit. */
static const uint8_t BIG[] = { 9, 0,  5, 0,0,0,0, 6,0, 0x00,0x90 };
static const uint8_t *arch = ARCH; static unsigned arch_len = sizeof ARCH;
static unsigned pos[16]; static int fail_read, reads;
static int dos_read(void *ctx, const pr_blockread_cpu *b, pr_blockread_cpu *a, uint8_t *buf, size_t n)
{
    unsigned k, h = b->bx, want = b->cx, have;
    (void)ctx; (void)n;
    *a = *b; a->ip = (uint16_t)(b->ip + 2u);
    if (++reads == fail_read) { a->ax = 5; a->flags = (uint16_t)(b->flags | 1u); return 0; }
    have = pos[h] < arch_len ? arch_len - pos[h] : 0u;
    if (want > have) want = have;
    for (k = 0; k < want; ++k) buf[k] = arch[pos[h] + k];
    pos[h] += want; a->ax = (uint16_t)want; a->flags = (uint16_t)(b->flags & ~1u);
    return 0;
}
static int dos_lseek(void *ctx, const void *bv, void *av)
{
    const pr_seek_cpu *b = bv; pr_seek_cpu *a = av;
    unsigned h = b->bx, al = b->ax & 0xFFu; long off = (long)(((unsigned long)b->cx << 16) | b->dx), base;
    (void)ctx;
    base = al == 0 ? 0 : al == 1 ? (long)pos[h] : (long)arch_len;
    pos[h] = (unsigned)(base + off);
    *a = *b; a->ip = (uint16_t)(b->ip + 2u); a->ax = (uint16_t)pos[h]; a->dx = (uint16_t)(pos[h] >> 16); a->flags = (uint16_t)(b->flags & ~1u);
    return 0;
}
static int seek_svc(void *ctx, const pr_seek_cpu *b, pr_seek_cpu *a) { return dos_lseek(ctx, b, a); }
static int filepos_svc(void *ctx, const pr_filepos_cpu *b, pr_filepos_cpu *a) { return dos_lseek(ctx, b, a); }

static void lcall(uint8_t *m, unsigned site, unsigned seg, unsigned off)
{
    m[p(CS, site)] = 0x9A; put(m, CS, site + 1u, off); put(m, CS, site + 3u, seg);
}
static pr_r0802_0716_cpu setup(uint8_t *m, unsigned id)
{
    static const unsigned sites[][2] = {{0x072B,0x0B25},{0x073E,0x0B25},{0x074E,0x0B0B},{0x0758,0x0B98},{0x07A7,0x14FB},{0x07AC,0x04A9},
        {0x07C5,0x14FB},{0x07CA,0x04A9},{0x07E3,0x14FB},{0x07E8,0x04A9},{0x0801,0x14FB},{0x0806,0x04A9},{0x0826,0x1366},{0x0839,0x0B0B},
        {0x0844,0x0B98},{0x0860,0x1593},{0x0865,0x04A9},{0x08C5,0x023F},{0x08E8,0x1563},{0x08ED,0x04A9},{0x0906,0x14FB},{0x090B,0x04A9}};
    static const unsigned long pow10[] = {1000000000ul, 100000000ul, 10000000ul, 1000000ul, 100000ul, 10000ul, 1000ul, 100ul, 10ul, 1ul};
    unsigned k;
    pr_r0802_0716_cpu c = {0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, 0xF000, CS, 0x0716, DS, 0x1111, SS, 0x0002};
    memset(m, 0x5A, PR_R0802_0716_RAM_SIZE);
    for (k = 0; k < sizeof sites / sizeof sites[0]; ++k) lcall(m, sites[k][0], SYS, sites[k][1]);
    lcall(m, 0x084D, QUITSEG, 0x0000);
    for (k = 0; k < 10; ++k) { put(m, SYS, 0x12A6 + 4u * k, (unsigned)(pow10[k] & 0xFFFFu)); put(m, SYS, 0x12A8 + 4u * k, (unsigned)(pow10[k] >> 16)); }
    pstr(m, CS, 0x070C, "TOO BIG ");
    put(m, DS, 0x4426, 0);
    pstr(m, DS, 0x547C, "ECL");
    put(m, DS, 0x4404, 0); put(m, DS, 0x4406, HEAP); put(m, DS, 0x4408, 0); put(m, DS, 0x440A, HEAP + 0x1000);
    put(m, DS, 0x440C, 0); put(m, DS, 0x440E, HEAP); put(m, DS, 0x4414, 0x00A9); put(m, DS, 0x4416, SYS);
    pstr(m, OUT, 0x0000, "1"); pstr(m, OUT, 0x0040, ".DAX");
    put(m, OUT, 0x0080, 0xEEEE); put(m, OUT, 0x0084, 0xEEEE); put(m, OUT, 0x0086, 0xEEEE);
    {   unsigned frame[] = {0x7000, 0x9001, 0x0084, OUT, 0x0080, OUT, id, 0x0040, OUT, 0x0000, OUT};
        for (k = 0; k < 11; ++k) put(m, SS, 0xF000 + 2u * k, frame[k]); }
    memset(pos, 0, sizeof pos); fail_read = 0; reads = 0; arch = ARCH; arch_len = sizeof ARCH;
    return c;
}
/* The effect of the not-modelled callee at a near stop: its RETF n (args popped), plus the given AX and RAM effect. */
static void callee_returns(pr_r0802_0716_cpu *c, const uint8_t *m, unsigned args)
{
    c->ip = (uint16_t)get(m, c->ss, c->sp); c->cs = (uint16_t)get(m, c->ss, (uint16_t)(c->sp + 2u));
    c->sp = (uint16_t)(c->sp + 4u + args);
}
/* 05CD's effect as the test states it: FileRecs [BP-133h] (handle 5) and [BP-0B3h] (handle 6), open, recsize 1; the header
 * length variable := 18 + 2; handle 5 at the first row, handle 6 at the data. */
static void opened(uint8_t *m, pr_r0802_0716_cpu *c)
{
    unsigned bp = c->bp, header = arch[0] | (unsigned)arch[1] << 8;
    put(m, SS, bp - 0x133u, 5); put(m, SS, bp - 0x131u, 0xD7B3); put(m, SS, bp - 0x12Fu, 1);
    put(m, SS, bp - 0x0B3u, 6); put(m, SS, bp - 0x0B1u, 0xD7B3); put(m, SS, bp - 0x0AFu, 1);
    put(m, SS, bp - 0x135u, header + 2u); pos[5] = 2; pos[6] = header + 2u;
    callee_returns(c, m, 0x14); c->ax = 1;
}

int main(void)
{
    uint8_t *m = malloc(PR_R0802_0716_RAM_SIZE);
    pr_r0802_0716_env env = {dos_read, NULL, filepos_svc, NULL, seek_svc, NULL, 1000};
    pr_r0802_0716_cpu c, before;
    pr_r0802_0716_diagnostic d;
    CHECK(m != NULL);

    /* Entry -> OPEN: name and ext copied (max 20), path := DS:547C + name, then 05CD's five far arguments; CS and 0778h pushed. */
    c = setup(m, 7);
    CHECK(pr_r0802_0716(m, PR_R0802_0716_RAM_SIZE, &c, &env, &d) == PR_R0802_0716_OPEN && d.site == 0x0775);
    CHECK(c.ip == 0x05CD && c.cs == CS && c.bp == 0xEFFE && c.sp == 0xEFFE - 0x23E - 20 - 4);
    CHECK(get(m, SS, c.sp) == 0x0778 && get(m, SS, c.sp + 2) == CS);
    CHECK(get(m, SS, c.sp + 4) == 0xEFFE - 0x133 && get(m, SS, c.sp + 8) == 0xEFFE - 0x0B3 && get(m, SS, c.sp + 12) == 0xEFFE - 0x135);
    CHECK(get(m, SS, c.sp + 16) == 0xEFFE - 0x2A && get(m, SS, c.sp + 20) == 0xEFFE - 0x23D);
    CHECK(m[p(SS, 0xEFFE - 0x23D)] == 4 && memcmp(m + p(SS, 0xEFFE - 0x23C), "ECL1", 4) == 0);
    CHECK(m[p(SS, 0xEFFE - 0x2A)] == 4 && memcmp(m + p(SS, 0xEFFE - 0x29), ".DAX", 4) == 0);

    /* Resume at 0778h with AL = 0 (not opened): U^ := 0, P^ := nil, RETF 12h. */
    before = c; callee_returns(&c, m, 0x14); c.ax = 0;
    CHECK(pr_r0802_0716(m, PR_R0802_0716_RAM_SIZE, &c, &env, &d) == PR_R0802_0716_RETURNED);
    CHECK(get(m, OUT, 0x80) == 0 && get(m, OUT, 0x84) == 0 && get(m, OUT, 0x86) == 0 && c.sp == 0xF016 && c.ip == 0x7000 && c.bp == 0x7777);

    /* Opened; id 7 is the first row: four BlockReads (1, 4, 2, 2 bytes) then ALLOC(DS:557E, packed 4). */
    c = before; opened(m, &c);
    CHECK(pr_r0802_0716(m, PR_R0802_0716_RAM_SIZE, &c, &env, &d) == PR_R0802_0716_ALLOC && c.ip == 0x0020 && reads == 4);
    CHECK(get(m, OUT, 0x80) == 6 && get(m, SS, c.bp - 0x2C) == 4 && m[p(SS, c.bp - 0x31)] == 7);
    CHECK(get(m, SS, c.sp) == 0x08BF && get(m, SS, c.sp + 4) == 4 && get(m, SS, c.sp + 6) == 0x557E && get(m, SS, c.sp + 8) == DS);
    /* 0020's effect: DS:557E := a packed buffer (2000:0000). Resume: P^ := GetMem(6); Seek(handle 6, 20 + 0); BlockRead 4 bytes
     * into the buffer; then CLOSE (both FileRecs). */
    put(m, DS, 0x557E, 0x0000); put(m, DS, 0x5580, 0x2000); callee_returns(&c, m, 6);
    CHECK(pr_r0802_0716(m, PR_R0802_0716_RAM_SIZE, &c, &env, &d) == PR_R0802_0716_CLOSE && c.ip == 0x06E7 && d.site == 0x091D);
    CHECK(get(m, OUT, 0x86) == HEAP && get(m, OUT, 0x84) == 0 && get(m, 0x2000, 0) == 0xBBAA && get(m, 0x2000, 2) == 0xDDCC);
    CHECK(pos[6] == 24 && get(m, DS, 0x4426) == 0);
    /* Resume at 0920h -> DECODE(DS:557E, P, &packed, U); 0938h -> FREE(DS:557E, packed); 0944h -> RETF 12h. */
    callee_returns(&c, m, 8);
    CHECK(pr_r0802_0716(m, PR_R0802_0716_RAM_SIZE, &c, &env, &d) == PR_R0802_0716_DECODE && c.ip == 0x050F);
    CHECK(get(m, SS, c.sp + 4) == 0x0080 && get(m, SS, c.sp + 8) == (uint16_t)(c.bp - 0x2Cu) && get(m, SS, c.sp + 12) == 0x0084);
    callee_returns(&c, m, 0x10);
    CHECK(pr_r0802_0716(m, PR_R0802_0716_RAM_SIZE, &c, &env, &d) == PR_R0802_0716_FREE && c.ip == 0x0000 && get(m, SS, c.sp + 4) == 4);
    callee_returns(&c, m, 6);
    CHECK(pr_r0802_0716(m, PR_R0802_0716_RAM_SIZE, &c, &env, &d) == PR_R0802_0716_RETURNED && c.sp == 0xF016 && c.cs == 0x9001);

    /* id 9 is the second row: FilePos(handle 5) = 11 < 20 loops; its packed 8C95h is just under the 8CA0h limit -> ALLOC. */
    c = setup(m, 9);
    CHECK(pr_r0802_0716(m, PR_R0802_0716_RAM_SIZE, &c, &env, &d) == PR_R0802_0716_OPEN);
    opened(m, &c);
    CHECK(pr_r0802_0716(m, PR_R0802_0716_RAM_SIZE, &c, &env, &d) == PR_R0802_0716_ALLOC && reads == 8 && get(m, SS, c.bp - 0x2C) == 0x8C95);
    /* An absent id: both rows read, FilePos reaches 20 = the header end, U^ := 0, P^ := nil, CLOSE. */
    c = setup(m, 3);
    CHECK(pr_r0802_0716(m, PR_R0802_0716_RAM_SIZE, &c, &env, &d) == PR_R0802_0716_OPEN);
    opened(m, &c);
    CHECK(pr_r0802_0716(m, PR_R0802_0716_RAM_SIZE, &c, &env, &d) == PR_R0802_0716_CLOSE && d.site == 0x08AD && reads == 8);
    CHECK(get(m, OUT, 0x80) == 0 && get(m, OUT, 0x84) == 0 && get(m, OUT, 0x86) == 0);
    callee_returns(&c, m, 8);
    CHECK(pr_r0802_0716(m, PR_R0802_0716_RAM_SIZE, &c, &env, &d) == PR_R0802_0716_RETURNED && c.ip == 0x7000);

    /* Packed 9000h > 8CA0h: Str(36864) into [BP-13Dh], message := CS:070C + it, PROMPT(message, attribute 0Eh); then QUIT. */
    c = setup(m, 3); arch = BIG; arch_len = sizeof BIG;
    CHECK(pr_r0802_0716(m, PR_R0802_0716_RAM_SIZE, &c, &env, &d) == PR_R0802_0716_OPEN);
    opened(m, &c);
    CHECK(pr_r0802_0716(m, PR_R0802_0716_RAM_SIZE, &c, &env, &d) == PR_R0802_0716_PROMPT && c.ip == 0x0193 && d.site == 0x084A);
    CHECK(m[p(SS, c.bp - 0x13D)] == 5 && memcmp(m + p(SS, c.bp - 0x13C), "36864", 5) == 0);
    CHECK(m[p(SS, c.bp - 0x23D)] == 13 && memcmp(m + p(SS, c.bp - 0x23C), "TOO BIG 36864", 13) == 0);
    CHECK(get(m, SS, c.sp) == 0x084D && get(m, SS, c.sp + 4) == (uint16_t)(c.bp - 0x23Du) && (get(m, SS, c.sp + 8) & 0xFF) == 0x0E);
    callee_returns(&c, m, 6);
    CHECK(pr_r0802_0716(m, PR_R0802_0716_RAM_SIZE, &c, &env, &d) == PR_R0802_0716_QUIT && c.cs == QUITSEG && c.ip == 0x0000);
    CHECK(get(m, SS, c.sp) == 0x0852 && get(m, SS, c.sp + 2) == CS);

    /* A failing read: IOCheck sends InOutRes to RunError (TERMINAL, at 0AF8:00E2 with AX = 5). */
    c = setup(m, 7);
    CHECK(pr_r0802_0716(m, PR_R0802_0716_RAM_SIZE, &c, &env, &d) == PR_R0802_0716_OPEN);
    opened(m, &c); fail_read = 2;
    CHECK(pr_r0802_0716(m, PR_R0802_0716_RAM_SIZE, &c, &env, &d) == PR_R0802_0716_RUNERROR && c.ip == 0x00E2 && c.cs == SYS && c.ax == 5);

    /* ARGUMENT: rejected before any change. */
    c = setup(m, 7); before = c;
    CHECK(pr_r0802_0716(m, PR_R0802_0716_RAM_SIZE, &c, NULL, &d) == PR_R0802_0716_ARGUMENT);
    CHECK(pr_r0802_0716(m, PR_R0802_0716_RAM_SIZE - 1u, &c, &env, &d) == PR_R0802_0716_ARGUMENT);
    c.sp = 0x02FF; CHECK(pr_r0802_0716(m, PR_R0802_0716_RAM_SIZE, &c, &env, &d) == PR_R0802_0716_ARGUMENT);
    c.sp = 0xFFEB; CHECK(pr_r0802_0716(m, PR_R0802_0716_RAM_SIZE, &c, &env, &d) == PR_R0802_0716_ARGUMENT);
    c.sp = 0xF000; c.ip = 0x0779; CHECK(pr_r0802_0716(m, PR_R0802_0716_RAM_SIZE, &c, &env, &d) == PR_R0802_0716_ARGUMENT);
    c.ip = 0x0716; CHECK(memcmp(&c, &before, sizeof c) == 0);

    free(m);
    printf("0802:0716: %u semantic checks passed\n", checks);
    return 0;
}
