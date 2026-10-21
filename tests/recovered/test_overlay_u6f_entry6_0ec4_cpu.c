/* Focused synthetic test for the raw-CPU unit 0x6F +0EC4 record loader (docs/re/OVL_006F_0EC4_CPU.md). No game bytes: the call words, the name
 * constants and the SYSTEM power table are this test's. The block loader's call is made a CODE stop, so what is observable is everything before
 * it: the Str digit, the three-part file name, the extension and the argument frame handed to 0802:0716. The full closure runs in the audit. */
#include "poolrad/recovered/overlay_u6f_entry6_0ec4_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "0EC4 line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
#define RAM PR_BUS_RAM_SIZE
static size_t p(unsigned s, unsigned o) { return ((size_t)s * 16u + (o & 65535u)) & 1048575u; }
static void put(uint8_t *m, unsigned s, unsigned o, unsigned v) { size_t a = p(s, o); m[a] = (uint8_t)v; m[(a + 1u) & 1048575u] = (uint8_t)(v >> 8); }
static unsigned get(const uint8_t *m, unsigned s, unsigned o) { size_t a = p(s, o); return (unsigned)(m[a] | (m[(a + 1u) & 1048575u] << 8)); }

enum { CS = 0x8800, SYS = 0x1AF8, LD = 0x1802, DS = 0x1C7C, SS = 0x2000, SP0 = 0xF000, REC = 0x7800 };
static pr_u6f_0ec4_env env;
static pr_ega ega;

static void far_at(uint8_t *m, unsigned site, unsigned entry, unsigned seg)
{
    m[p(CS, site)] = 0x9A; put(m, CS, site + 1u, entry); put(m, CS, site + 3u, seg);
}
static void pascal(uint8_t *m, unsigned off, const char *s)
{
    unsigned i, n = (unsigned)strlen(s);
    m[p(CS, off)] = (uint8_t)n;
    for (i = 0; i < n; ++i) m[p(CS, off + 1u + i)] = (uint8_t)s[i];
}
static pr_u6f_0ec4_cpu setup(uint8_t *m, unsigned digit)
{
    static const uint32_t powers[10] = {1000000000u, 100000000u, 10000000u, 1000000u, 100000u, 10000u, 1000u, 100u, 10u, 1u};
    pr_u6f_0ec4_cpu c = {0x1234, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x789A, SP0, CS, 0x0EC4, DS, 0x8888, SS, 0x0202};
    unsigned i;
    memset(m, 0x5A, RAM);
    for (i = 0; i < 10u; ++i) { put(m, SYS, 0x12A6u + 4u * i, powers[i] & 0xFFFFu); put(m, SYS, 0x12A8u + 4u * i, powers[i] >> 16); }
    far_at(m, 0x0EE0, 0x1366, SYS); far_at(m, 0x0EF0, 0x0B0B, SYS); far_at(m, 0x0EFA, 0x0B98, SYS); far_at(m, 0x0F04, 0x0B98, SYS);
    far_at(m, 0x0F14, 0x0B0B, SYS); far_at(m, 0x0F27, 0x0716, LD);
    pascal(m, 0x0E98, "MON"); pascal(m, 0x0E9C, "CHA"); pascal(m, 0x0EA0, ".dax");
    m[p(DS, 0x5376)] = (uint8_t)digit;
    put(m, SS, SP0, 0x7000); put(m, SS, SP0 + 2, 0x9001); put(m, SS, SP0 + 4, 0x0100); put(m, SS, SP0 + 6, REC); put(m, SS, SP0 + 8, 0xAB07);
    return c;
}
static void pstr(const uint8_t *m, unsigned s, unsigned o, const char *want)
{
    unsigned n = (unsigned)strlen(want), i;
    CHECK(m[p(s, o)] == n);
    for (i = 0; i < n; ++i) CHECK(m[p(s, o + 1u + i)] == (uint8_t)want[i]);
}

int main(void)
{
    static uint8_t ram[RAM], copy[RAM];
    pr_bus bus = {ram, &ega, NULL, NULL, NULL};
    pr_u6f_0ec4_diagnostic d;
    pr_u6f_0ec4_cpu c, before;
    unsigned bp, digit, bad;
    pr_ega_reset(&ega);
    memset(&env, 0, sizeof env);
    /* The block loader's call is not the original (offset 0717h): CODE with IP at the call and no return frame pushed. */
    for (digit = 0; digit < 256u; digit += 51u) {
        char name[16], digits[8];
        c = setup(ram, digit); put(ram, CS, 0x0F28, 0x0717);
        CHECK(pr_u6f_entry6_0ec4_cpu(&bus, RAM, &c, &env, &d) == PR_U6F_0EC4_CODE);
        CHECK(c.ip == 0x0F27 && c.cs == CS && d.site == 0x0F27 && d.callee == PR_U6F_0EC4_LOAD && d.calls == 5u);
        bp = (SP0 - 2u) & 0xFFFFu;
        CHECK(c.bp == bp && c.sp == ((SP0 - 0x11Cu - 0x12u) & 0xFFFFu));
        snprintf(digits, sizeof digits, "%u", digit);
        digits[1] = 0;                                     /* Str's result string holds one character (M = 1): the first digit */
        snprintf(name, sizeof name, "MON%sCHA", digits);
        pstr(ram, SS, bp - 0x12u, digits);
        pstr(ram, SS, bp - 0x114u, name);
        pstr(ram, SS, bp - 0x119u, ".dax");
        /* the frame 0716 would receive: P, U, the id word, the extension and the name (the two far pointers left by the string loads) */
        CHECK(get(ram, SS, c.sp + 0) == ((bp - 6u) & 0xFFFFu) && get(ram, SS, c.sp + 2) == SS);
        CHECK(get(ram, SS, c.sp + 4) == ((bp - 2u) & 0xFFFFu) && get(ram, SS, c.sp + 6) == SS);
        CHECK((get(ram, SS, c.sp + 8) & 0xFFu) == 0x07u);
        CHECK(get(ram, SS, c.sp + 10) == ((bp - 0x119u) & 0xFFFFu) && get(ram, SS, c.sp + 12) == SS);
        CHECK(get(ram, SS, c.sp + 14) == ((bp - 0x114u) & 0xFFFFu) && get(ram, SS, c.sp + 16) == SS);
        CHECK((c.flags & 0x08D5u) == 0x0044u);                                             /* the last writer: StrLoad's XOR CH,CH */
        CHECK(c.dx == DS && c.cx == 0u && c.cs == CS);                                      /* StrLoad leaves DX := DS and CX = 0 */
    }
    /* A call whose opcode is not 9Ah stops before its pushes. */
    c = setup(ram, 3); ram[p(CS, 0x0EE0)] = 0x90;
    CHECK(pr_u6f_entry6_0ec4_cpu(&bus, RAM, &c, &env, &d) == PR_U6F_0EC4_CODE && c.ip == 0x0EE0 && d.calls == 0u && d.callee == PR_U6F_0EC4_STR);
    c = setup(ram, 3); put(ram, CS, 0x0EF1, 0x0B0C);
    CHECK(pr_u6f_entry6_0ec4_cpu(&bus, RAM, &c, &env, &d) == PR_U6F_0EC4_CODE && c.ip == 0x0EF0 && d.calls == 1u && d.callee == PR_U6F_0EC4_STRLOAD);
    /* Host rejections are atomic. */
    for (bad = 0; bad < 4u; ++bad) {
        c = setup(ram, 3);
        if (bad == 0) c.ip = 0x0EC5;
        if (bad == 1) c.sp = 0x09F3;
        if (bad == 2) c.sp = 0xFFF5;
        if (bad == 3) c.sp = 0;
        before = c; memcpy(copy, ram, RAM);
        CHECK(pr_u6f_entry6_0ec4_cpu(&bus, RAM, &c, &env, &d) == PR_U6F_0EC4_ARGUMENT);
        CHECK(memcmp(&c, &before, sizeof c) == 0 && memcmp(copy, ram, RAM) == 0);
    }
    c = setup(ram, 3);
    CHECK(pr_u6f_entry6_0ec4_cpu(NULL, RAM, &c, &env, &d) == PR_U6F_0EC4_ARGUMENT);
    CHECK(pr_u6f_entry6_0ec4_cpu(&bus, RAM - 1u, &c, &env, &d) == PR_U6F_0EC4_ARGUMENT);
    CHECK(pr_u6f_entry6_0ec4_cpu(&bus, RAM, NULL, &env, &d) == PR_U6F_0EC4_ARGUMENT);
    CHECK(pr_u6f_entry6_0ec4_cpu(&bus, RAM, &c, NULL, &d) == PR_U6F_0EC4_ARGUMENT);
    printf("%u semantic checks passed\n", checks);
    return 0;
}
