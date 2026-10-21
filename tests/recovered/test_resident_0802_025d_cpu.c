/* Focused synthetic test for the raw-CPU resident 0802:025D (docs/re/RES_0802_025D_CPU.md). No game bytes: the far-call
 * operands of 025D and of the composed 02EB are written at their sites. */
#include "poolrad/recovered/resident_0802_025d_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "025d line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
static size_t p(unsigned s, unsigned o) { return ((size_t)s * 16u + (o & 65535u)) & 1048575u; }
static void put(uint8_t *m, unsigned s, unsigned o, unsigned v) { size_t a = p(s, o); m[a] = (uint8_t)v; m[(a + 1u) & 1048575u] = (uint8_t)(v >> 8); }
static unsigned get(const uint8_t *m, unsigned s, unsigned o) { size_t a = p(s, o); return m[a] | (unsigned)m[(a + 1u) & 1048575u] << 8; }

enum { CS = 0x1802, SYS = 0x1AF8, DS = 0x1C7C, SS = 0x7000, N = 0x5800 };
static int present, opens, closes;
static int dos_open(void *ctx, const pr_reset_cpu *b, pr_reset_cpu *a)
{
    (void)ctx;
    ++opens; *a = *b; a->ip = (uint16_t)(b->ip + 2u);
    if (present) { a->ax = 6; a->flags = (uint16_t)(b->flags & ~1u); }
    else { a->ax = 2; a->flags = (uint16_t)(b->flags | 1u); }
    return 0;
}
static int dos_close(void *ctx, const pr_system_close_cpu *b, pr_system_close_cpu *a)
{
    (void)ctx;
    ++closes; *a = *b; a->ip = (uint16_t)(b->ip + 2u); a->ax = (uint16_t)((b->ax & 0xFF00u) | 1u); a->flags = (uint16_t)(b->flags & ~1u);
    return 0;
}
static void lcall(uint8_t *m, unsigned site, unsigned off) { m[p(CS, site)] = 0x9A; put(m, CS, site + 1u, off); put(m, CS, site + 3u, SYS); }
static void pstr(uint8_t *m, unsigned s, unsigned o, const char *t) { size_t n = strlen(t); m[p(s, o)] = (uint8_t)n; memcpy(m + p(s, o + 1u), t, n); }
static pr_r0802_025d_cpu setup(uint8_t *m, unsigned flag, int is_present)
{
    pr_r0802_025d_cpu c = {0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, 0xF000, CS, 0x025D, DS, 0x1111, SS, 0x0002};
    unsigned k;
    memset(m, 0x5A, PR_R0802_025D_RAM_SIZE);
    lcall(m, 0x0271, 0x0B25); lcall(m, 0x0284, 0x0B25); lcall(m, 0x02C9, 0x13E2); lcall(m, 0x02D7, 0x1410);
    lcall(m, 0x0300, 0x0B25); lcall(m, 0x0310, 0x13E2); lcall(m, 0x031F, 0x1410); lcall(m, 0x032A, 0x1491); lcall(m, 0x032F, 0x04A2);
    put(m, DS, 0x4426, 0); m[p(DS, 0x442C)] = 2;
    for (k = 0; k < 0x80; ++k) m[p(N, 0x200 + k)] = 0;
    pstr(m, N, 0x10, "ECL1.DAX"); pstr(m, N, 0x80, "INSERT DISK");
    {   unsigned frame[] = {0x7000, 0x9001, 0x0200, N, flag, 0x0080, N, 0x0010, N};
        for (k = 0; k < 9; ++k) put(m, SS, 0xF000 + 2u * k, frame[k]); }
    present = is_present; opens = closes = 0;
    return c;
}

int main(void)
{
    uint8_t *m = malloc(PR_R0802_025D_RAM_SIZE);
    pr_r0802_02eb_env env = {dos_open, NULL, dos_close, NULL};
    pr_r0802_025d_cpu c, before;
    pr_r0802_025d_diagnostic d;
    CHECK(m != NULL);

    /* Present, flag 1: FileExists (open + close), then Assign + Reset(f, 1): AL = 1, f open with recsize 1; RETF 0Eh. */
    c = setup(m, 1, 1);
    CHECK(pr_r0802_025d(m, PR_R0802_025D_RAM_SIZE, &c, &env, &d) == PR_R0802_025D_RETURNED && opens == 2 && closes == 1);
    CHECK((c.ax & 0xFF) == 1 && get(m, N, 0x202) == 0xD7B3 && get(m, N, 0x200) == 6 && get(m, N, 0x204) == 1);
    CHECK(c.ip == 0x7000 && c.cs == 0x9001 && c.sp == 0xF012 && c.bp == 0x7777);
    /* Absent, flag 1: no prompt, no Reset, f untouched; AL = 0. */
    c = setup(m, 1, 0);
    CHECK(pr_r0802_025d(m, PR_R0802_025D_RAM_SIZE, &c, &env, &d) == PR_R0802_025D_RETURNED && opens == 1 && (c.ax & 0xFF) == 0);
    CHECK(get(m, N, 0x202) == 0 && get(m, N, 0x200) == 0);
    /* Absent, flag 0: PROMPT(message [BP-53h], attribute 0Eh), CS and 02ADh pushed. */
    c = setup(m, 0, 0);
    CHECK(pr_r0802_025d(m, PR_R0802_025D_RAM_SIZE, &c, &env, &d) == PR_R0802_025D_PROMPT && c.ip == 0x0193 && d.site == 0x02AA);
    CHECK(get(m, SS, c.sp) == 0x02AD && get(m, SS, c.sp + 2) == CS && get(m, SS, c.sp + 4) == (uint16_t)(c.bp - 0x53u));
    CHECK((get(m, SS, c.sp + 8) & 0xFF) == 0x0E && m[p(SS, c.bp - 0x53u)] == 11 && memcmp(m + p(SS, c.bp - 0x52u), "INSERT DISK", 11) == 0);
    /* The prompt returns (RETF 6) and the disk is now in: the retry finds the file -> Assign + Reset -> AL = 1. */
    c.ip = (uint16_t)get(m, SS, c.sp); c.cs = (uint16_t)get(m, SS, c.sp + 2); c.sp = (uint16_t)(c.sp + 4u + 6u); present = 1;
    CHECK(pr_r0802_025d(m, PR_R0802_025D_RAM_SIZE, &c, &env, &d) == PR_R0802_025D_RETURNED && (c.ax & 0xFF) == 1 && opens == 3);
    CHECK(get(m, N, 0x202) == 0xD7B3 && c.sp == 0xF012);
    /* Still absent after the prompt: the loop prompts again. */
    c = setup(m, 0, 0);
    CHECK(pr_r0802_025d(m, PR_R0802_025D_RAM_SIZE, &c, &env, &d) == PR_R0802_025D_PROMPT);
    c.ip = (uint16_t)get(m, SS, c.sp); c.cs = (uint16_t)get(m, SS, c.sp + 2); c.sp = (uint16_t)(c.sp + 4u + 6u);
    CHECK(pr_r0802_025d(m, PR_R0802_025D_RAM_SIZE, &c, &env, &d) == PR_R0802_025D_PROMPT && opens == 2);
    /* Flag 0 with the file present: no prompt. */
    c = setup(m, 0, 1);
    CHECK(pr_r0802_025d(m, PR_R0802_025D_RAM_SIZE, &c, &env, &d) == PR_R0802_025D_RETURNED && (c.ax & 0xFF) == 1);

    /* ARGUMENT: rejected before any change. */
    c = setup(m, 1, 1); before = c;
    CHECK(pr_r0802_025d(m, PR_R0802_025D_RAM_SIZE, &c, NULL, &d) == PR_R0802_025D_ARGUMENT);
    CHECK(pr_r0802_025d(m, PR_R0802_025D_RAM_SIZE - 1u, &c, &env, &d) == PR_R0802_025D_ARGUMENT);
    c.sp = 0x01FF; CHECK(pr_r0802_025d(m, PR_R0802_025D_RAM_SIZE, &c, &env, &d) == PR_R0802_025D_ARGUMENT);
    c.sp = 0xFFEF; CHECK(pr_r0802_025d(m, PR_R0802_025D_RAM_SIZE, &c, &env, &d) == PR_R0802_025D_ARGUMENT);
    c.sp = 0xF000; c.ip = 0x0289; CHECK(pr_r0802_025d(m, PR_R0802_025D_RAM_SIZE, &c, &env, &d) == PR_R0802_025D_ARGUMENT);
    c.ip = 0x025D; CHECK(memcmp(&c, &before, sizeof c) == 0 && opens == 0);

    free(m);
    printf("0802:025D: %u semantic checks passed\n", checks);
    return 0;
}
