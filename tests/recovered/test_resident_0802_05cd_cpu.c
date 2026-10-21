/* Focused synthetic test for the raw-CPU resident 0802:05CD (docs/re/RES_0802_05CD_CPU.md). No game bytes: the far-call
 * operands of 05CD, of the composed 025D and of 02EB are written at their sites, and the CS:05CB literal is synthetic. */
#include "poolrad/recovered/resident_0802_05cd_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "05cd line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
static size_t p(unsigned s, unsigned o) { return ((size_t)s * 16u + (o & 65535u)) & 1048575u; }
static void put(uint8_t *m, unsigned s, unsigned o, unsigned v) { size_t a = p(s, o); m[a] = (uint8_t)v; m[(a + 1u) & 1048575u] = (uint8_t)(v >> 8); }
static unsigned get(const uint8_t *m, unsigned s, unsigned o) { size_t a = p(s, o); return m[a] | (unsigned)m[(a + 1u) & 1048575u] << 8; }

enum { CS = 0x1802, SYS = 0x1AF8, DS = 0x1C7C, SS = 0x7000, N = 0x5800 };
static int present, opens, closes, reads, seeks, read_fails;
static unsigned next_handle, header_in = 0x1234, seek_to;
static int dos_open(void *ctx, const pr_reset_cpu *b, pr_reset_cpu *a)
{
    (void)ctx;
    ++opens; *a = *b; a->ip = (uint16_t)(b->ip + 2u);
    if (present) { a->ax = (uint16_t)next_handle++; a->flags = (uint16_t)(b->flags & ~1u); }
    else { a->ax = 2; a->flags = (uint16_t)(b->flags | 1u); }
    return 0;
}
static int dos_close(void *ctx, const pr_system_close_cpu *b, pr_system_close_cpu *a)
{
    (void)ctx;
    ++closes; --next_handle; *a = *b; a->ip = (uint16_t)(b->ip + 2u); a->ax = (uint16_t)((b->ax & 0xFF00u) | 1u);
    a->flags = (uint16_t)(b->flags & ~1u);
    return 0;
}
static int dos_read(void *ctx, const pr_blockread_cpu *b, pr_blockread_cpu *a, uint8_t *buffer, size_t length)
{
    (void)ctx;
    ++reads; *a = *b; a->ip = (uint16_t)(b->ip + 2u);
    if (read_fails) { a->ax = 5; a->flags = (uint16_t)(b->flags | 1u); return 0; }
    if (length >= 2u) { buffer[0] = (uint8_t)header_in; buffer[1] = (uint8_t)(header_in >> 8); }
    a->ax = 2; a->flags = (uint16_t)(b->flags & ~1u);
    return 0;
}
static int dos_seek(void *ctx, const pr_seek_cpu *b, pr_seek_cpu *a)
{
    (void)ctx;
    ++seeks; *a = *b; a->ip = (uint16_t)(b->ip + 2u); seek_to = (unsigned)b->cx << 16 | b->dx;
    a->ax = b->dx; a->dx = b->cx; a->flags = (uint16_t)(b->flags & ~1u);
    return 0;
}
static void lcall(uint8_t *m, unsigned site, unsigned off) { m[p(CS, site)] = 0x9A; put(m, CS, site + 1u, off); put(m, CS, site + 3u, SYS); }
static void pstr(uint8_t *m, unsigned s, unsigned o, const char *t) { size_t n = strlen(t); m[p(s, o)] = (uint8_t)n; memcpy(m + p(s, o + 1u), t, n); }
static pr_r0802_05cd_cpu setup(uint8_t *m, unsigned flag, int is_present)
{
    static const unsigned sites[][2] = {
        {0x05E2, 0x0B25}, {0x05F5, 0x0B25}, {0x0605, 0x0B0B}, {0x060F, 0x0B98}, {0x062C, 0x0B0B}, {0x063B, 0x0C3B}, {0x0640, 0x0B98},
        {0x064A, 0x0B98}, {0x0670, 0x0B0B}, {0x067A, 0x0B98}, {0x067F, 0x13E2}, {0x068D, 0x1410}, {0x0692, 0x04A9}, {0x06AA, 0x14FB},
        {0x06AF, 0x04A9}, {0x06CA, 0x1563}, {0x06CF, 0x04A9},
        {0x0271, 0x0B25}, {0x0284, 0x0B25}, {0x02C9, 0x13E2}, {0x02D7, 0x1410},                          /* 025D */
        {0x0300, 0x0B25}, {0x0310, 0x13E2}, {0x031F, 0x1410}, {0x032A, 0x1491}, {0x032F, 0x04A2}};       /* 02EB */
    pr_r0802_05cd_cpu c = {0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, 0xF000, CS, 0x05CD, DS, 0x1111, SS, 0x0002};
    unsigned k;
    memset(m, 0x5A, PR_R0802_05CD_RAM_SIZE);
    for (k = 0; k < sizeof sites / sizeof sites[0]; ++k) lcall(m, sites[k][0], sites[k][1]);
    m[p(CS, 0x05CB)] = 1; m[p(CS, 0x05CC)] = ':';                       /* the literal: a synthetic one-character string */
    put(m, DS, 0x4426, 0); m[p(DS, 0x442C)] = 2;
    m[p(DS, 0x5376)] = 1; pstr(m, DS, 0x2614 + 0x1E, "INSERT DISK "); m[p(DS, 0x547D)] = 'B'; m[p(DS, 0x557C)] = (uint8_t)flag;
    for (k = 0; k < 0x80; ++k) { m[p(N, 0x200 + k)] = 0; m[p(N, 0x300 + k)] = 0; }
    pstr(m, N, 0x10, "ECL1"); pstr(m, N, 0x60, ".DAX"); put(m, N, 0x400, 0xA5A5);
    {   unsigned frame[] = {0x7000, 0x9001, 0x0300, N, 0x0200, N, 0x0400, N, 0x0060, N, 0x0010, N};
        for (k = 0; k < 12; ++k) put(m, SS, 0xF000 + 2u * k, frame[k]); }
    present = is_present; opens = closes = reads = seeks = read_fails = 0; next_handle = 5; header_in = 0x1234; seek_to = 0xFFFFFFFFu;
    return c;
}

int main(void)
{
    uint8_t *m = malloc(PR_R0802_05CD_RAM_SIZE);
    pr_r0802_05cd_env env = {{dos_open, NULL, dos_close, NULL}, dos_read, NULL, dos_seek, NULL};
    pr_r0802_05cd_cpu c, before;
    pr_r0802_05cd_diagnostic d;
    CHECK(m != NULL);

    /* Present, flag 1: 025D opens f (FileExists open + close, then Reset), then Assign + Reset g, read the 2-byte header from g,
     * header += 2, Seek(f, header): AL = 1, both records open, RETF 14h. */
    c = setup(m, 1, 1);
    CHECK(pr_r0802_05cd(m, PR_R0802_05CD_RAM_SIZE, &c, &env, &d) == PR_R0802_05CD_RETURNED);
    CHECK((c.ax & 0xFF) == 1 && opens == 3 && closes == 1 && reads == 1 && seeks == 1);
    CHECK(get(m, N, 0x400) == 0x1236 && seek_to == 0x1236);
    CHECK(get(m, N, 0x202) == 0xD7B3 && get(m, N, 0x200) == 5 && get(m, N, 0x302) == 0xD7B3 && get(m, N, 0x300) == 6);
    CHECK(get(m, N, 0x204) == 1 && get(m, N, 0x304) == 1);
    CHECK(c.ip == 0x7000 && c.cs == 0x9001 && c.sp == 0xF018 && c.bp == 0x7777);
    /* The header word wraps at 16 bits: FFFEh + 2 = 0. */
    c = setup(m, 1, 1); header_in = 0xFFFE;
    CHECK(pr_r0802_05cd(m, PR_R0802_05CD_RAM_SIZE, &c, &env, &d) == PR_R0802_05CD_RETURNED && get(m, N, 0x400) == 0 && seek_to == 0);
    /* Absent, flag 1: AL = 0; header, g and the services untouched. */
    c = setup(m, 1, 0);
    CHECK(pr_r0802_05cd(m, PR_R0802_05CD_RAM_SIZE, &c, &env, &d) == PR_R0802_05CD_RETURNED && (c.ax & 0xFF) == 0);
    CHECK(get(m, N, 0x400) == 0xA5A5 && get(m, N, 0x302) == 0 && reads == 0 && seeks == 0 && c.sp == 0xF018);
    /* Absent, flag 0: the prompt inside 025D. Its message is table[1] + the disk char + the CS:05CB literal. */
    c = setup(m, 0, 0);
    CHECK(pr_r0802_05cd(m, PR_R0802_05CD_RAM_SIZE, &c, &env, &d) == PR_R0802_05CD_PROMPT && c.ip == 0x0193 && d.site == 0x02AA);
    CHECK(get(m, SS, c.sp) == 0x02AD && get(m, SS, c.sp + 2) == CS);
    CHECK(m[p(SS, get(m, SS, c.sp + 4))] == 14 && memcmp(m + p(SS, get(m, SS, c.sp + 4) + 1u), "INSERT DISK B:", 14) == 0);
    /* The prompt returns (RETF 6) with the disk in: 025D finds the file and returns to 065Ch; this body finishes. */
    c.ip = (uint16_t)get(m, SS, c.sp); c.cs = (uint16_t)get(m, SS, c.sp + 2); c.sp = (uint16_t)(c.sp + 4u + 6u); present = 1;
    CHECK(pr_r0802_05cd(m, PR_R0802_05CD_RAM_SIZE, &c, &env, &d) == PR_R0802_05CD_RETURNED && (c.ax & 0xFF) == 1);
    CHECK(get(m, N, 0x400) == 0x1236 && c.sp == 0xF018 && c.ip == 0x7000);
    /* A failed read: IOCheck after BlockRead is terminal: RUNERROR at 0AF8:00E2 with AX = InOutRes; no Seek. */
    c = setup(m, 1, 1); read_fails = 1;
    CHECK(pr_r0802_05cd(m, PR_R0802_05CD_RAM_SIZE, &c, &env, &d) == PR_R0802_05CD_RUNERROR);
    CHECK(c.cs == SYS && c.ip == 0x00E2 && c.ax == 5 && seeks == 0 && get(m, N, 0x400) == 0xA5A5);

    /* ARGUMENT: rejected before any change. */
    c = setup(m, 1, 1); before = c;
    { pr_r0802_05cd_env bad = env; bad.read = NULL; CHECK(pr_r0802_05cd(m, PR_R0802_05CD_RAM_SIZE, &c, &bad, &d) == PR_R0802_05CD_ARGUMENT); }
    { pr_r0802_05cd_env bad = env; bad.seek = NULL; CHECK(pr_r0802_05cd(m, PR_R0802_05CD_RAM_SIZE, &c, &bad, &d) == PR_R0802_05CD_ARGUMENT); }
    CHECK(pr_r0802_05cd(m, PR_R0802_05CD_RAM_SIZE - 1u, &c, &env, &d) == PR_R0802_05CD_ARGUMENT);
    c.sp = 0x0669; CHECK(pr_r0802_05cd(m, PR_R0802_05CD_RAM_SIZE, &c, &env, &d) == PR_R0802_05CD_ARGUMENT);
    c.sp = 0xFFE9; CHECK(pr_r0802_05cd(m, PR_R0802_05CD_RAM_SIZE, &c, &env, &d) == PR_R0802_05CD_ARGUMENT);
    c.sp = 0xF000; c.ip = 0x025D; CHECK(pr_r0802_05cd(m, PR_R0802_05CD_RAM_SIZE, &c, &env, &d) == PR_R0802_05CD_ARGUMENT);
    c.ip = 0x05CD; CHECK(memcmp(&c, &before, sizeof c) == 0 && opens == 0);

    free(m);
    printf("0802:05CD: %u semantic checks passed\n", checks);
    return 0;
}
