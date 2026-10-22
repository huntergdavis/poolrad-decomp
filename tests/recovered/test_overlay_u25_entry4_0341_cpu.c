/* Focused synthetic test for the raw-CPU unit 0x25 +0341 screen-block loader (docs/re/OVL_0025_0341_CPU.md). No game bytes: the call words, the stub and the buffer are this
 * test's. The row-clear callee (unit 0x3B0 +06FB) has no code here, so it stops with CODE at its own call: what is observable is the FillChar of the buffer, the clear of
 * DS:[557C], the stub check and the frames. The loop, the printer, the loader and the retry LIMIT run in the audit against the original bytes. */
#include "poolrad/recovered/overlay_u25_entry4_0341_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "0341 line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
#define RAM PR_BUS_RAM_SIZE
static size_t p(unsigned s, unsigned o) { return ((size_t)s * 16u + (o & 65535u)) & 1048575u; }
static void put(uint8_t *m, unsigned s, unsigned o, unsigned v) { size_t a = p(s, o); m[a] = (uint8_t)v; m[(a + 1u) & 1048575u] = (uint8_t)(v >> 8); }
static unsigned get(const uint8_t *m, unsigned s, unsigned o) { size_t a = p(s, o); return (unsigned)(m[a] | (m[(a + 1u) & 1048575u] << 8)); }

enum { W = 0x8800, SYS = 0x1AF8, STUB = 0x13C5, U3B = 0x9000, DS = 0x1C7C, SS = 0x2000, SP0 = 0xF000, BUFS = 0x7A00 };
static pr_u25_0341_env env;
static pr_ega ega;

static void far_at(uint8_t *m, unsigned site, unsigned entry, unsigned seg) { m[p(W, site)] = 0x9A; put(m, W, site + 1u, entry); put(m, W, site + 3u, seg); }

static pr_u25_0341_cpu setup(uint8_t *m)
{
    pr_u25_0341_cpu c = {0x1234, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x789A, SP0, W, 0x0341, DS, 0x8888, SS, 0x0202};
    unsigned i;
    memset(m, 0x5A, RAM);
    far_at(m, 0x0355, 0x16A8, SYS); far_at(m, 0x035F, 0x002F, STUB);
    m[p(STUB, 0x2F)] = 0xEA; put(m, STUB, 0x30, 0x06FB); put(m, STUB, 0x32, U3B);          /* the declared trampoline */
    put(m, DS, 0x49DE, 0x0000); put(m, DS, 0x49E0, BUFS); m[p(DS, 0x557C)] = 0x77;
    for (i = 0; i < 0x1E40u; ++i) m[p(BUFS, i)] = 0xCC;
    put(m, SS, SP0, 0x7000); put(m, SS, SP0 + 2, 0x9001); put(m, SS, SP0 + 4, 0xAB07);
    return c;
}

int main(void)
{
    static uint8_t ram[RAM], copy[RAM];
    pr_bus bus = {ram, &ega, NULL, NULL, NULL};
    pr_u25_0341_diagnostic d;
    pr_u25_0341_cpu c, before;
    unsigned i, bad, bp = (SP0 - 2u) & 0xFFFFu;
    pr_ega_reset(&ega);
    memset(&env, 0, sizeof env); env.heap_steps = 1000; env.retries = 3;
    /* FillChar clears exactly 1E00h bytes of the buffer, DS:[557C] becomes 0, the stub is accepted, and 06FB (no code) stops with CODE inside the row clear. */
    c = setup(ram);
    CHECK(pr_u25_entry4_0341_cpu(&bus, RAM, &c, &env, &d) == PR_U25_0341_CODE);
    CHECK(d.callee == PR_U25_0341_ROWCLEAR && d.callee_status == PR_U3B0_06FB_CODE && d.calls == 1u && d.site == 0x035Fu);
    for (i = 0; i < 0x1E00u; ++i) CHECK(ram[p(BUFS, i)] == 0u);
    for (i = 0x1E00u; i < 0x1E40u; ++i) CHECK(ram[p(BUFS, i)] == 0xCCu);                       /* not one byte more */
    CHECK(ram[p(DS, 0x557C)] == 0u);
    /* the row clear was entered with the far frame: IP 0364 and the wrapper's CS, below BP and the 112h locals */
    CHECK(get(ram, SS, (bp - 0x112u - 4u) & 0xFFFFu) == 0x0364u && get(ram, SS, (bp - 0x112u - 2u) & 0xFFFFu) == W);
    /* FillChar's argument words: value 0 (low byte), count 1E00h, the buffer's offset and segment */
    c = setup(ram); ram[p(W, 0x0355)] = 0x90;
    CHECK(pr_u25_entry4_0341_cpu(&bus, RAM, &c, &env, &d) == PR_U25_0341_CODE && c.ip == 0x0355u && d.calls == 0u && d.callee == PR_U25_0341_FILL);
    CHECK(get(ram, SS, c.sp + 2u) == 0x1E00u && get(ram, SS, c.sp + 4u) == 0x0000u && get(ram, SS, c.sp + 6u) == BUFS && (get(ram, SS, c.sp) & 0xFFu) == 0u);
    CHECK(ram[p(BUFS, 0)] == 0xCCu && ram[p(DS, 0x557C)] == 0x77u);                              /* nothing was cleared yet */
    /* the stub entry is not a far JMP, not to 06FB, or its call is not the original: CODE at the call after FillChar ran */
    c = setup(ram); ram[p(STUB, 0x2F)] = 0x90;
    CHECK(pr_u25_entry4_0341_cpu(&bus, RAM, &c, &env, &d) == PR_U25_0341_CODE && c.ip == 0x035Fu && d.calls == 1u && d.callee == PR_U25_0341_ROWCLEAR && ram[p(BUFS, 0)] == 0u);
    c = setup(ram); put(ram, STUB, 0x30, 0x06FC);
    CHECK(pr_u25_entry4_0341_cpu(&bus, RAM, &c, &env, &d) == PR_U25_0341_CODE && c.ip == 0x035Fu && d.calls == 1u);
    c = setup(ram); put(ram, W, 0x0360, 0x0030);
    CHECK(pr_u25_entry4_0341_cpu(&bus, RAM, &c, &env, &d) == PR_U25_0341_CODE && c.ip == 0x035Fu);
    /* the stub segment word is read live */
    c = setup(ram); put(ram, W, 0x0362, STUB + 1u);
    CHECK(pr_u25_entry4_0341_cpu(&bus, RAM, &c, &env, &d) == PR_U25_0341_CODE && c.ip == 0x035Fu);
    /* the buffer pointer is read live from DS:[49DE]: a different buffer is the one cleared */
    c = setup(ram); put(ram, DS, 0x49DE, 0x0010); put(ram, DS, 0x49E0, BUFS + 0x100u);
    for (i = 0; i < 0x1F00u; ++i) ram[p(BUFS + 0x100u, 0x10u + i)] = 0xEE;
    CHECK(pr_u25_entry4_0341_cpu(&bus, RAM, &c, &env, &d) == PR_U25_0341_CODE);
    for (i = 0; i < 0x1E00u; ++i) CHECK(ram[p(BUFS + 0x100u, 0x10u + i)] == 0u);
    CHECK(ram[p(BUFS + 0x100u, 0x10u + 0x1E00u)] == 0xEEu && ram[p(BUFS, 0)] == 0xCCu);
    /* Host rejections are atomic. */
    for (bad = 0; bad < 4u; ++bad) {
        c = setup(ram);
        if (bad == 0) c.ip = 0x0342;
        if (bad == 1) c.sp = 0x09EB;
        if (bad == 2) c.sp = 0xFFFB;
        if (bad == 3) c.sp = 0;
        before = c; memcpy(copy, ram, RAM);
        CHECK(pr_u25_entry4_0341_cpu(&bus, RAM, &c, &env, &d) == PR_U25_0341_ARGUMENT);
        CHECK(memcmp(&c, &before, sizeof c) == 0 && memcmp(copy, ram, RAM) == 0);
    }
    c = setup(ram);
    CHECK(pr_u25_entry4_0341_cpu(NULL, RAM, &c, &env, &d) == PR_U25_0341_ARGUMENT);
    CHECK(pr_u25_entry4_0341_cpu(&bus, RAM - 1u, &c, &env, &d) == PR_U25_0341_ARGUMENT);
    CHECK(pr_u25_entry4_0341_cpu(&bus, RAM, NULL, &env, &d) == PR_U25_0341_ARGUMENT);
    CHECK(pr_u25_entry4_0341_cpu(&bus, RAM, &c, NULL, &d) == PR_U25_0341_ARGUMENT);
    printf("%u semantic checks passed\n", checks);
    return 0;
}
