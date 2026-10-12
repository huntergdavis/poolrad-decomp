/* Synthetic test for the unit 0x3ce entry 4 model. Constants, names, slot table and all hooks are fabricated. Checked: each of the four
 * paths and its exact call order, the 'T' (case-insensitive) +0x40 on p3 and the dropped last character, p3 + 0x80 for the second load,
 * the Copy edge cases (short names), the string cap, slot-indexed destinations, hook NULL/failure stops, and the argument guards. */
#include "poolrad/recovered/overlay_u3ce_entry4_01c8.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

static const uint8_t K0[] = { 5, 'p','r','e','f','x' }, K1[] = { 5, 'o','t','h','e','r' }, K2[] = { 6, 's','i','x','c','h','r' }, K3[] = { 4, 'f','o','u','r' };
static pr_u3ce_slot slots[256];
static pr_u3ce_e4_env env;

typedef struct { int log[8]; unsigned n; uint8_t names[8][260]; uint8_t p3[8]; unsigned which[8]; uint16_t tab[8]; pr_far16 obj[8]; int fail_kind, fail_val; uint8_t strv; } Rec;
static Rec rec;

static int h_load(void *c, const pr_u3ce_e4_env *e, const uint8_t *nm, uint8_t p3, unsigned which, pr_far16 *dest)
{
    Rec *r = c; (void)e;
    r->log[r->n] = 1; memcpy(r->names[r->n], nm, 1u + nm[0]); r->p3[r->n] = p3; r->which[r->n] = which; r->n++;
    dest->off = (uint16_t)(0x1000 + which); dest->seg = (uint16_t)(0x2000 + p3);
    return r->fail_kind == 1 ? r->fail_val : 0;
}
static int h_pal(void *c, const pr_u3ce_e4_env *e, uint16_t table, pr_far16 obj)
{
    Rec *r = c; (void)e;
    r->log[r->n] = 2; r->tab[r->n] = table; r->obj[r->n] = obj; r->n++;
    return r->fail_kind == 2 ? r->fail_val : 0;
}
static int h_str(void *c, const pr_u3ce_e4_env *e, uint32_t v, uint8_t out[2])
{
    Rec *r = c; (void)e; (void)v;
    r->log[r->n++] = 3; out[0] = 1; out[1] = r->strv;
    return r->fail_kind == 3 ? r->fail_val : 0;
}
static int h_flush(void *c, const pr_u3ce_e4_env *e) { Rec *r = c; (void)e; r->log[r->n++] = 4; return r->fail_kind == 4 ? r->fail_val : 0; }

static void setup(void)
{
    memset(slots, 0, sizeof slots); memset(&rec, 0, sizeof rec); rec.strv = '7';
    memset(&env, 0, sizeof env);
    env.c1b0 = K0; env.c1b6 = K1; env.c1bc = K2; env.c1c3 = K3; env.b5376 = 7; env.slots = slots;
    env.hooks.load_464_0111 = h_load; env.hooks.palette_464_1e5c = h_pal; env.hooks.str_af8_1366 = h_str; env.hooks.flush_899_0104 = h_flush; env.ctx = &rec;
}
static void ps(uint8_t *d, const char *s) { size_t n = strlen(s); d[0] = (uint8_t)n; memcpy(d + 1, s, n); }

int main(void)
{
    uint8_t nm[260];
    pr_u3ce_e4_report rep;

    /* path A (first constant): last char not T: dropped, p3 unchanged; two loads (p3, p3+0x80) then the flush */
    setup(); ps(nm, "prefxA");
    CHECK(pr_u3ce_entry4(&env, nm, 0x10, 3, &rep) == PR_U3CE_E4_OK && rep.path == PR_U3CE_E4_PATH_A && rep.t_suffix == 0);
    CHECK(rec.n == 3 && rec.log[0] == 1 && rec.log[1] == 1 && rec.log[2] == 4);
    CHECK(memcmp(rec.names[0], "\x05prefx", 6) == 0 && memcmp(rec.names[1], "\x05prefx", 6) == 0);
    CHECK(rec.p3[0] == 0x10 && rec.p3[1] == 0x90 && rec.which[0] == 0 && rec.which[1] == 1);
    CHECK(slots[3].first.off == 0x1000 && slots[3].second.off == 0x1001 && slots[2].first.off == 0 && slots[4].first.off == 0);   /* destinations indexed by slot */

    /* path A (second constant), lowercase 't' and uppercase 'T': p3 += 0x40 with byte wrap */
    setup(); ps(nm, "otherT");
    CHECK(pr_u3ce_entry4(&env, nm, 0xF0, 0, &rep) == PR_U3CE_E4_OK && rep.path == PR_U3CE_E4_PATH_A && rep.t_suffix == 1 && rep.p3_used == 0x30);
    CHECK(rec.p3[0] == 0x30 && rec.p3[1] == 0xB0);
    setup(); ps(nm, "othert");
    CHECK(pr_u3ce_entry4(&env, nm, 1, 0, &rep) == PR_U3CE_E4_OK && rep.t_suffix == 1 && rec.p3[0] == 0x41);
    /* a longer name starting with the constant: only the last character is dropped */
    setup(); ps(nm, "prefxlongt");
    CHECK(pr_u3ce_entry4(&env, nm, 0, 0, &rep) == PR_U3CE_E4_OK && memcmp(rec.names[0], "\x09prefxlong", 10) == 0);

    /* path B6 (6-char constant): two loads, no palette, p3 unchanged */
    setup(); ps(nm, "sixchr");
    CHECK(pr_u3ce_entry4(&env, nm, 0x20, 5, &rep) == PR_U3CE_E4_OK && rep.path == PR_U3CE_E4_PATH_B6 && rec.n == 3 && rec.log[0] == 1 && rec.log[1] == 1 && rec.log[2] == 4 && rec.p3[0] == 0x20);

    /* path B4: both loads first, then both palettes (table 0xCF2) reading the slot words the loads wrote, then the flush */
    setup(); ps(nm, "four");
    CHECK(pr_u3ce_entry4(&env, nm, 0x02, 9, &rep) == PR_U3CE_E4_OK && rep.path == PR_U3CE_E4_PATH_B4);
    CHECK(rec.n == 5 && rec.log[0] == 1 && rec.log[1] == 1 && rec.log[2] == 2 && rec.log[3] == 2 && rec.log[4] == 4);
    CHECK(rec.tab[2] == 0xCF2 && rec.tab[3] == 0xCF2 && rec.obj[2].off == 0x1000 && rec.obj[2].seg == 0x2002 && rec.obj[3].off == 0x1001 && rec.obj[3].seg == 0x2082);

    /* path C: Str suffix (1 char), load, palette (0xCE2), load, palette, flush; the name is s + suffix */
    setup(); ps(nm, "zed");
    CHECK(pr_u3ce_entry4(&env, nm, 0x05, 1, &rep) == PR_U3CE_E4_OK && rep.path == PR_U3CE_E4_PATH_C);
    CHECK(rec.n == 6 && rec.log[0] == 3 && rec.log[1] == 1 && rec.log[2] == 2 && rec.log[3] == 1 && rec.log[4] == 2 && rec.log[5] == 4);
    CHECK(memcmp(rec.names[1], "\x04zed7", 5) == 0 && rec.tab[2] == 0xCE2 && rec.tab[4] == 0xCE2 && rec.p3[1] == 5 && rec.p3[3] == 0x85);
    /* short names and the empty string fall to path C, not A (Copy returns the shorter string) */
    setup(); ps(nm, "pref");
    CHECK(pr_u3ce_entry4(&env, nm, 0, 0, &rep) == PR_U3CE_E4_OK && rep.path == PR_U3CE_E4_PATH_C);
    setup(); nm[0] = 0;
    CHECK(pr_u3ce_entry4(&env, nm, 0, 0, &rep) == PR_U3CE_E4_OK && rep.path == PR_U3CE_E4_PATH_C && memcmp(rec.names[1], "\x01" "7", 2) == 0);
    /* a 255-character name: the suffix append is capped at 255 */
    setup(); memset(nm + 1, 'q', 255); nm[0] = 255;
    CHECK(pr_u3ce_entry4(&env, nm, 0, 0, &rep) == PR_U3CE_E4_OK && rec.names[1][0] == 255 && rec.names[1][255] == 'q');

    /* stops: failure keeps earlier effects; NULL hook reports the call; the flush is not reached */
    setup(); ps(nm, "four"); rec.fail_kind = 2; rec.fail_val = -4;
    CHECK(pr_u3ce_entry4(&env, nm, 0, 0, &rep) == PR_U3CE_E4_CALLBACK && rep.callback_status == -4 && rep.failed_call == PR_U3CE_E4_CALL_PALETTE && rec.n == 3 && slots[0].second.off == 0x1001);
    setup(); ps(nm, "zed"); env.hooks.str_af8_1366 = NULL;
    CHECK(pr_u3ce_entry4(&env, nm, 0, 0, &rep) == PR_U3CE_E4_HOOK && rep.failed_call == PR_U3CE_E4_CALL_STR && rec.n == 0);
    setup(); ps(nm, "sixchr"); env.hooks.flush_899_0104 = NULL;
    CHECK(pr_u3ce_entry4(&env, nm, 0, 0, &rep) == PR_U3CE_E4_HOOK && rep.failed_call == PR_U3CE_E4_CALL_FLUSH && rec.n == 2);
    setup(); ps(nm, "sixchr"); env.hooks.load_464_0111 = NULL;
    CHECK(pr_u3ce_entry4(&env, nm, 0, 0, &rep) == PR_U3CE_E4_HOOK && rep.failed_call == PR_U3CE_E4_CALL_LOAD && rep.n_calls == 0);

    /* argument guards; NULL report accepted */
    setup(); ps(nm, "sixchr");
    CHECK(pr_u3ce_entry4(NULL, nm, 0, 0, &rep) == PR_U3CE_E4_ARGUMENT && pr_u3ce_entry4(&env, NULL, 0, 0, &rep) == PR_U3CE_E4_ARGUMENT);
    env.c1c3 = NULL; CHECK(pr_u3ce_entry4(&env, nm, 0, 0, &rep) == PR_U3CE_E4_ARGUMENT && rec.n == 0);
    env.c1c3 = K3; env.slots = NULL; CHECK(pr_u3ce_entry4(&env, nm, 0, 0, NULL) == PR_U3CE_E4_ARGUMENT);
    env.slots = slots; CHECK(pr_u3ce_entry4(&env, nm, 0, 0, NULL) == PR_U3CE_E4_OK);
    puts("overlay_u3ce_entry4_01c8: ok");
    return 0;
}
