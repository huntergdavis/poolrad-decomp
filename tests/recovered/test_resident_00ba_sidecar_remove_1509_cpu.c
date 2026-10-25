/* Focused synthetic test for the raw-CPU resident 00BA:1509 record removal (docs/re/RES_00BA_1509_CPU.md). No game bytes: the five far-call words, the
 * message literal and the heap are this test's. The prompt paths (a null or absent record) are covered by the Unicorn and native differentials; here the
 * prompt's code is absent, so a null record stops at the prompt with CALLEE after StrLoad has copied the message. */
#include "poolrad/recovered/resident_00ba_sidecar_remove_1509_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "1509 line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
#define RAM PR_BUS_RAM_SIZE
static uint8_t *m;
static size_t p(unsigned s, unsigned o) { return ((size_t)s * 16u + (o & 65535u)) & 1048575u; }
static void put(unsigned s, unsigned o, unsigned v) { size_t a = p(s, o); m[a] = (uint8_t)v; m[(a + 1u) & 1048575u] = (uint8_t)(v >> 8); }
static unsigned get(unsigned s, unsigned o) { size_t a = p(s, o); return m[a] | ((unsigned)m[(a + 1u) & 1048575u] << 8); }

enum { CS = 0x6000, SYS = 0x1AF8, VID = 0x1709, DS = 0x1C7C, SS = 0x2000, SP0 = 0xF000, CHAR = 0x5000, ORG = 0x3000, TOP = 0x3400 };
static unsigned node(unsigned k) { return ORG + 1u + 4u * k; }
static void lcall(unsigned site, unsigned off, unsigned seg) { m[p(CS, site)] = 0x9A; put(CS, site + 1u, off); put(CS, site + 3u, seg); }

/* a list of n records (segments node(order[i]), offset 0) linked through +2Ah/+2Ch from the character's +0C8h; the frame names the record (seg, off) */
static pr_res_1509_cpu setup(const unsigned *order, unsigned n, unsigned rseg, unsigned roff)
{
    pr_res_1509_cpu c = {0x1234, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x789A, SP0, CS, 0x1509, DS, 0x8888, SS, 0x0202};
    unsigned i;
    memset(m, 0x5A, RAM);
    lcall(0x1524, 0x0B0B, SYS); lcall(0x1529, 0x0BF5, VID); lcall(0x15B3, 0x0B0B, SYS); lcall(0x15B8, 0x0BF5, VID); lcall(0x15EC, 0x0254, SYS);
    m[p(CS, 0x14CE)] = 3; memcpy(m + p(CS, 0x14CF), "Nul", 3); m[p(CS, 0x14E2)] = 2; memcpy(m + p(CS, 0x14E3), "No", 2);
    put(DS, 0x4400, 0); put(DS, 0x4402, ORG); put(DS, 0x4404, 0); put(DS, 0x4406, TOP); put(DS, 0x4408, 0); put(DS, 0x440A, TOP + 0x80);
    put(DS, 0x440C, 0); put(DS, 0x440E, TOP); put(DS, 0x4410, 0); put(DS, 0x4412, 0);
    for (i = 0; i < n; ++i) { put(node(order[i]), 0x2A, 0); put(node(order[i]), 0x2C, i + 1u < n ? node(order[i + 1u]) : 0u); }
    put(CHAR, 0x0100 + 0xC8, 0); put(CHAR, 0x0100 + 0xCA, n ? node(order[0]) : 0u);
    put(SS, SP0, 0x7000); put(SS, SP0 + 2u, 0x9001); put(SS, SP0 + 4u, roff); put(SS, SP0 + 6u, rseg); put(SS, SP0 + 8u, 0x0100); put(SS, SP0 + 10u, CHAR);
    return c;
}

int main(void)
{
    static pr_ega ega; static uint8_t before[RAM];
    pr_bus bus; pr_res_1509_env env; pr_res_1509_diagnostic d; pr_res_1509_cpu c, saved; size_t left;
    const unsigned three[3] = {0, 1, 2}, five[5] = {3, 0, 4, 1, 2};
    m = malloc(RAM); CHECK(m != NULL);
    memset(&bus, 0, sizeof bus); bus.ram = m; bus.ega = &ega; pr_ega_reset(&ega);
    memset(&env, 0, sizeof env); env.freemem_steps = 64;
    /* removing the head: the head := its link, its own link := 0:0, FreeMem puts the 40h-byte block on the free list; RETF 8 back to the caller */
    c = setup(three, 3, node(0), 0); left = 1000;
    CHECK(pr_res_00ba_sidecar_remove_1509_cpu(&bus, RAM, &c, &left, &env, &d) == PR_RES_1509_RETURNED);
    CHECK(c.cs == 0x9001 && c.ip == 0x7000 && c.sp == SP0 + 12u && c.bp == 0x789A && d.callee == 3);
    CHECK(get(CHAR, 0x01CA) == node(1) && get(node(0), 0x2C) == 0 && get(node(0), 0x2A) == 0 && get(node(1), 0x2C) == node(2));
    CHECK(get(DS, 0x440E) == node(0) && get(node(0), 2) == TOP && get(node(0), 4) == 0 && get(node(0), 6) == 4);
    /* removing from the middle of five (order 3,0,4,1,2): the predecessor (node 4) links past it to node 2 */
    c = setup(five, 5, node(1), 0); left = 1000;
    CHECK(pr_res_00ba_sidecar_remove_1509_cpu(&bus, RAM, &c, &left, &env, &d) == PR_RES_1509_RETURNED);
    CHECK(get(CHAR, 0x01CA) == node(3) && get(node(4), 0x2C) == node(2) && get(node(1), 0x2C) == 0 && get(DS, 0x440E) == node(1));
    /* the block just under HeapPtr: FreeMem lowers HeapPtr instead of listing it */
    c = setup(three, 3, node(2), 0); put(DS, 0x4406, node(2) + 4u); put(DS, 0x440E, node(2) + 4u); left = 1000;
    CHECK(pr_res_00ba_sidecar_remove_1509_cpu(&bus, RAM, &c, &left, &env, &d) == PR_RES_1509_RETURNED);
    CHECK(get(DS, 0x4406) == node(2) && get(node(1), 0x2C) == 0);
    /* an invalid block (offset 4): unlinked, then FreeMem's RunError 204 at SYSTEM:00E2 */
    c = setup(three, 0, 0, 0); put(CHAR, 0x01C8, 4); put(CHAR, 0x01CA, node(0)); put(node(0), 0x2E, 0); put(node(0), 0x30, 0); put(SS, SP0 + 4u, 4); put(SS, SP0 + 6u, node(0));
    left = 1000; CHECK(pr_res_00ba_sidecar_remove_1509_cpu(&bus, RAM, &c, &left, &env, &d) == PR_RES_1509_RUNERROR);
    CHECK(c.cs == SYS && c.ip == 0x00E2 && c.ax == 204 && d.callee == 3 && get(CHAR, 0x01CA) == 0);
    /* a null record: StrLoad copies the message at CS:14CE to [BP-18h]; the prompt (no code here) stops, reported as CALLEE with its status */
    c = setup(three, 3, 0, 0); left = 1000;
    CHECK(pr_res_00ba_sidecar_remove_1509_cpu(&bus, RAM, &c, &left, &env, &d) == PR_RES_1509_CALLEE);
    CHECK(d.callee == 2 && d.site == 0x1529 && m[p(SS, SP0 - 2u - 0x18u)] == 3 && !memcmp(m + p(SS, SP0 - 2u - 0x17u), "Nul", 3));
    /* an absent record: the walk reaches null and loads the second message at [BP-2Bh] */
    c = setup(three, 3, node(7), 0); left = 1000;
    CHECK(pr_res_00ba_sidecar_remove_1509_cpu(&bus, RAM, &c, &left, &env, &d) == PR_RES_1509_CALLEE);
    CHECK(d.callee == 2 && d.site == 0x15B8 && m[p(SS, SP0 - 2u - 0x2Bu)] == 2 && !memcmp(m + p(SS, SP0 - 2u - 0x2Au), "No", 2));
    /* a cyclic list never ends: the budget stops it inside the walk with the budget spent */
    c = setup(three, 3, node(7), 0); put(node(2), 0x2C, node(0)); left = 500;
    CHECK(pr_res_00ba_sidecar_remove_1509_cpu(&bus, RAM, &c, &left, &env, &d) == PR_RES_1509_LIMIT && left == 0 && c.ip >= 0x1531 && c.ip < 0x15B3);
    /* a call site that is not the original: CODE at the call, before its frame */
    c = setup(three, 3, node(0), 0); put(CS, 0x15ED, 0x0255); left = 1000;
    CHECK(pr_res_00ba_sidecar_remove_1509_cpu(&bus, RAM, &c, &left, &env, &d) == PR_RES_1509_CODE && c.ip == 0x15EC && d.site == 0x15EC);
    /* budget 0 changes nothing */
    c = setup(three, 3, node(0), 0); saved = c; memcpy(before, m, RAM); left = 0;
    CHECK(pr_res_00ba_sidecar_remove_1509_cpu(&bus, RAM, &c, &left, &env, &d) == PR_RES_1509_LIMIT && !memcmp(&c, &saved, sizeof c) && !memcmp(m, before, RAM));
    /* argument rejection is atomic */
    c = setup(three, 3, node(0), 0); memcpy(before, m, RAM); left = 100;
    c.ip = 0x150A; saved = c; CHECK(pr_res_00ba_sidecar_remove_1509_cpu(&bus, RAM, &c, &left, &env, &d) == PR_RES_1509_ARGUMENT);
    c.ip = 0x1509; c.sp = 0x01FF; saved = c; CHECK(pr_res_00ba_sidecar_remove_1509_cpu(&bus, RAM, &c, &left, &env, &d) == PR_RES_1509_ARGUMENT);
    c.sp = 0xFFF3; saved = c; CHECK(pr_res_00ba_sidecar_remove_1509_cpu(&bus, RAM, &c, &left, &env, &d) == PR_RES_1509_ARGUMENT);
    CHECK(!memcmp(&c, &saved, sizeof c) && !memcmp(m, before, RAM) && left == 100); c.sp = SP0;
    CHECK(pr_res_00ba_sidecar_remove_1509_cpu(NULL, RAM, &c, &left, &env, &d) == PR_RES_1509_ARGUMENT);
    CHECK(pr_res_00ba_sidecar_remove_1509_cpu(&bus, RAM - 1u, &c, &left, &env, &d) == PR_RES_1509_ARGUMENT);
    CHECK(pr_res_00ba_sidecar_remove_1509_cpu(&bus, RAM, NULL, &left, &env, &d) == PR_RES_1509_ARGUMENT);
    CHECK(pr_res_00ba_sidecar_remove_1509_cpu(&bus, RAM, &c, NULL, &env, &d) == PR_RES_1509_ARGUMENT);
    CHECK(pr_res_00ba_sidecar_remove_1509_cpu(&bus, RAM, &c, &left, NULL, &d) == PR_RES_1509_ARGUMENT);
    free(m); printf("RES_00BA_1509 synthetic, budget and guard cases passed (%u checks)\n", checks); return 0;
}
