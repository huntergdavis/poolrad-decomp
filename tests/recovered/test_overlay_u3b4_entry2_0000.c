/* Focused behaviour check for the unit 0x3B4 +0x0000 translation. Callees and the far record are fabricated; expected values are written
 * out by hand from the file's instructions. Checked: argument guards; the table lookup and the record add with its carry into AH, which is
 * the high byte of every pushed word; the signed clamps of the first byte (a value of 0x80 or more is negative and is kept) and the 0x2A and
 * 0x2B bands; the second byte's bands (below 2, 2 to 0x21, 0x22, 0x23 and above), each taken from its own fresh read of the record; the read
 * sequence of the record (the word at +0x186, then the word at +0x188 at each LES, and the byte at +0x188 when the first word is below 2); the
 * remainder and quotient words from DS:0x6DDC; the update of DS:0x6DDC modulo 6; the order of the two calls; and a read failure that stops the
 * routine before any call. Regression: a callback that retargets DS:0x49D2 after the first +0x188 read; the later reads must use the new
 * pointer, so w2 follows the retargeted record (2 then 0x23 gives 4; 0x22 then 0x23 gives 4). The byte-read path is retargeted the same way.
 * The byte-level comparison against the original bytes is the separate oracle in tests/overlay/ovl3b4_0000_oracle.py. */
#include "poolrad/recovered/overlay_u3b4_entry2_0000.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

#define MAX_FAR 32
#define MAX_READS 16

static uint8_t ds[PR_U3B4_ENTRY2_DS_SIZE];
static struct { uint16_t seg, off; uint8_t val; } far_tab[MAX_FAR];
static size_t far_n;
static int fail_at_read;       /* the read with this index (0-based) fails; -1: never */

/* the read log: kind 'w' (word) or 'b' (byte), the offset read, and the value returned */
static struct { char kind; uint16_t off; uint16_t val; } reads[MAX_READS];
static int n_reads;

/* the retarget hook: on the first read at trigger_off, DS:0x49D2 is set to retarget_to (after the value is returned) */
static int trigger_on;
static uint16_t trigger_off, retarget_to;
static int triggered;

static uint16_t w39[5];
static int n39, n1b52, n_order;
static int order[4];

static void hook_39(void *ctx, uint16_t w1, uint16_t w2, uint16_t w3, uint16_t w4, uint16_t w5)
{
    (void)ctx;
    w39[0] = w1; w39[1] = w2; w39[2] = w3; w39[3] = w4; w39[4] = w5;
    n39++;
    order[n_order++] = 1;
}

static void hook_1b52(void *ctx)
{
    (void)ctx;
    n1b52++;
    order[n_order++] = 2;
}

static uint8_t far_byte_at(uint16_t seg, uint16_t off)
{
    size_t k = far_n;
    while (k > 0) {
        k--;
        if (far_tab[k].seg == seg && far_tab[k].off == off) {
            return far_tab[k].val;
        }
    }
    return 0;
}

static void maybe_retarget(uint16_t off)
{
    if (trigger_on && !triggered && off == trigger_off) {
        ds[0x49D2u] = (uint8_t)(retarget_to & 0xFF);
        ds[0x49D3u] = (uint8_t)(retarget_to >> 8);
        triggered = 1;
    }
}

static int hook_far16(void *ctx, uint16_t seg, uint16_t off, uint16_t *out)
{
    int idx = n_reads;
    uint16_t v;
    (void)ctx;
    if (n_reads >= MAX_READS) {
        return 1;
    }
    if (fail_at_read == idx) {
        return 1;
    }
    v = (uint16_t)(far_byte_at(seg, off) | ((uint16_t)far_byte_at(seg, (uint16_t)(off + 1u)) << 8));
    reads[n_reads].kind = 'w';
    reads[n_reads].off = off;
    reads[n_reads].val = v;
    n_reads++;
    *out = v;
    maybe_retarget(off);
    return 0;
}

static int hook_far8(void *ctx, uint16_t seg, uint16_t off, uint8_t *out)
{
    int idx = n_reads;
    uint8_t v;
    (void)ctx;
    if (n_reads >= MAX_READS) {
        return 1;
    }
    if (fail_at_read == idx) {
        return 1;
    }
    v = far_byte_at(seg, off);
    reads[n_reads].kind = 'b';
    reads[n_reads].off = off;
    reads[n_reads].val = v;
    n_reads++;
    *out = v;
    maybe_retarget(off);
    return 0;
}

static void put16(uint16_t off, uint16_t v)
{
    ds[off] = (uint8_t)(v & 0xFF);
    ds[(uint16_t)(off + 1u)] = (uint8_t)(v >> 8);
}

static void add_far(uint16_t off, uint8_t val)
{
    far_tab[far_n].seg = 0x4000; far_tab[far_n].off = off; far_tab[far_n].val = val; far_n++;
}

static void add_far_word(uint16_t off, uint16_t word)
{
    add_far(off, (uint8_t)(word & 0xFF));
    add_far((uint16_t)(off + 1u), (uint8_t)(word >> 8));
}

static void reset(void)
{
    memset(ds, 0, sizeof ds);
    far_n = 0;
    fail_at_read = -1;
    n_reads = 0;
    memset(reads, 0, sizeof reads);
    trigger_on = 0;
    trigger_off = 0;
    retarget_to = 0;
    triggered = 0;
    memset(w39, 0, sizeof w39);
    n39 = n1b52 = n_order = 0;
    memset(order, 0, sizeof order);
    /* the record pointer: offset 0x0100, segment 0x4000 */
    put16(0x49D2u, 0x0100u);
    put16(0x49D4u, 0x4000u);
}

static pr_u3b4_entry2_env make_env(void)
{
    pr_u3b4_entry2_env env;
    env.ds = ds;
    env.ds_size = sizeof ds;
    env.hooks.call_3e9_39 = hook_39;
    env.hooks.call_464_1b52 = hook_1b52;
    env.hooks.far_read16 = hook_far16;
    env.hooks.far_read8 = hook_far8;
    env.ctx = NULL;
    return env;
}

static int test_guards(void)
{
    pr_u3b4_entry2_env env, bad;

    reset();
    env = make_env();
    CHECK(pr_u3b4_entry2_0000(NULL) == PR_U3B4_ENTRY2_ARGUMENT);
    bad = env;
    bad.ds = NULL;
    CHECK(pr_u3b4_entry2_0000(&bad) == PR_U3B4_ENTRY2_ARGUMENT);
    bad = env;
    bad.ds_size = PR_U3B4_ENTRY2_DS_SIZE - 1u;
    CHECK(pr_u3b4_entry2_0000(&bad) == PR_U3B4_ENTRY2_ARGUMENT);
    bad = env;
    bad.hooks.call_464_1b52 = NULL;
    CHECK(pr_u3b4_entry2_0000(&bad) == PR_U3B4_ENTRY2_ARGUMENT);
    bad = env;
    bad.hooks.far_read8 = NULL;
    CHECK(pr_u3b4_entry2_0000(&bad) == PR_U3B4_ENTRY2_ARGUMENT);
    CHECK(n39 == 0 && n1b52 == 0 && n_reads == 0);
    return 0;
}

static int test_basic_words_and_update(void)
{
    pr_u3b4_entry2_env env;

    /* [0x49FA] = 0: the table byte at DS:0x3C76 is 0x10. Record +0x186 = 0x0005: AX = 0x10 + 5 = 0x15, a signed value from 2 up to 0x29,
     * so [bp-1] = 2. Record +0x188 = 0 (below 2): the byte read gives [bp-2] = 0. H = 0. Words: 0x0002, 0x0000, (0x00):[0x6AAF] = 0x0007,
     * remainder of [0x6DDC] = 0x0B by 2 = 1, and (0x0B >> 1) + 9 = 14. Then [0x6DDC] := (0x0B + 1) mod 6 = 0, and the resident call.
     * Reads: word +0x186, word +0x188, byte +0x188. */
    reset();
    env = make_env();
    ds[0x3C76u] = 0x10;
    add_far_word(0x0100u + 0x186u, 0x0005u);
    add_far_word(0x0100u + 0x188u, 0x0000u);
    ds[0x6AAFu] = 0x07;
    ds[0x6DDCu] = 0x0B;
    CHECK(pr_u3b4_entry2_0000(&env) == PR_U3B4_ENTRY2_OK);
    CHECK(n39 == 1 && n1b52 == 1);
    CHECK(w39[0] == 0x0002 && w39[1] == 0x0000 && w39[2] == 0x0007 && w39[3] == 0x0001 && w39[4] == 0x000E);
    CHECK(ds[0x6DDCu] == 0x00);
    CHECK(n_order == 2 && order[0] == 1 && order[1] == 2);
    CHECK(n_reads == 3);
    CHECK(reads[0].kind == 'w' && reads[0].off == 0x0286u);
    CHECK(reads[1].kind == 'w' && reads[1].off == 0x0288u);
    CHECK(reads[2].kind == 'b' && reads[2].off == 0x0288u);
    return 0;
}

static int test_ah_carry(void)
{
    pr_u3b4_entry2_env env;

    /* [0x49FA] = 0, table 0x10, record +0x186 = 0x0FF0: AX = 0x1000, so H = 0x10 and [bp-1] = 0x00 (kept: signed 0 is below 2).
     * Record +0x188 = 0x0023: the first word is 0x23 (2 or more), the second read is 0x23 (0x22 or more), the third is 0x23 (0x23 or more):
     * [bp-2] = 4. Words: 0x1000, 0x1004, (0x10):[0x6AAF] = 0x10FF, and [0x6DDC] = 0 gives remainder 0 and (0 >> 1) + 9 = 9. */
    reset();
    env = make_env();
    ds[0x3C76u] = 0x10;
    add_far_word(0x0100u + 0x186u, 0x0FF0u);
    add_far_word(0x0100u + 0x188u, 0x0023u);
    ds[0x6AAFu] = 0xFF;
    ds[0x6DDCu] = 0x00;
    CHECK(pr_u3b4_entry2_0000(&env) == PR_U3B4_ENTRY2_OK);
    CHECK(w39[0] == 0x1000 && w39[1] == 0x1004 && w39[2] == 0x10FF && w39[3] == 0x0000 && w39[4] == 0x0009);
    CHECK(ds[0x6DDCu] == 0x01);
    CHECK(n_reads == 4 && reads[1].off == 0x0288u && reads[2].off == 0x0288u && reads[3].off == 0x0288u);
    return 0;
}

static int test_signed_clamp_negative(void)
{
    pr_u3b4_entry2_env env;

    /* Table 0 and record +0x186 = 0x0080: AX = 0x0080, so [bp-1] = 0x80, which is -128 as a signed byte. It is below 2 and is kept, so the
     * first word has low byte 0x80 and H = 0. Record +0x188 = 0x0001: below 2, so the byte read gives [bp-2] = 1. */
    reset();
    env = make_env();
    add_far_word(0x0100u + 0x186u, 0x0080u);
    add_far_word(0x0100u + 0x188u, 0x0001u);
    CHECK(pr_u3b4_entry2_0000(&env) == PR_U3B4_ENTRY2_OK);
    CHECK(w39[0] == 0x0080 && w39[1] == 0x0001);
    return 0;
}

static int test_bands(void)
{
    pr_u3b4_entry2_env env;

    /* First byte: 0x2A becomes 3, 0x2B becomes 4, 0x29 becomes 2. Second byte: the word 0x22 gives 3 (its third read is 0x22, below 0x23),
     * the word 0x21 gives 2. */
    reset();
    env = make_env();
    add_far_word(0x0100u + 0x186u, 0x002Au);
    add_far_word(0x0100u + 0x188u, 0x0022u);
    CHECK(pr_u3b4_entry2_0000(&env) == PR_U3B4_ENTRY2_OK);
    CHECK(w39[0] == 0x0003 && w39[1] == 0x0003);

    reset();
    env = make_env();
    add_far_word(0x0100u + 0x186u, 0x002Bu);
    add_far_word(0x0100u + 0x188u, 0x0021u);
    CHECK(pr_u3b4_entry2_0000(&env) == PR_U3B4_ENTRY2_OK);
    CHECK(w39[0] == 0x0004 && w39[1] == 0x0002);
    CHECK(n_reads == 3);

    reset();
    env = make_env();
    add_far_word(0x0100u + 0x186u, 0x0029u);
    CHECK(pr_u3b4_entry2_0000(&env) == PR_U3B4_ENTRY2_OK);
    CHECK(w39[0] == 0x0002);
    return 0;
}

static int test_regression_word_retarget_2_then_23(void)
{
    pr_u3b4_entry2_env env;

    /* Regression. Table 0, record +0x186 = 0: b1 = 0, and H = 0. The first +0x188 read (at the record at 0x0100) returns 2 and retargets
     * DS:0x49D2 to 0x0200. The second read (a fresh LES) is at 0x0200 + 0x188 = 0x0388 and returns 0x23, the third (the top-band reread) is
     * also 0x0388 and returns 0x23, so b2 = 4. A cached first read would give 2 and w2 = 2. The expected word is 0x0004. */
    reset();
    env = make_env();
    add_far_word(0x0100u + 0x186u, 0x0000u);
    add_far_word(0x0100u + 0x188u, 0x0002u);
    add_far_word(0x0200u + 0x188u, 0x0023u);
    trigger_on = 1;
    trigger_off = 0x0100u + 0x188u;
    retarget_to = 0x0200u;
    CHECK(pr_u3b4_entry2_0000(&env) == PR_U3B4_ENTRY2_OK);
    CHECK(n_reads == 4);
    CHECK(reads[0].off == 0x0286u && reads[0].kind == 'w');
    CHECK(reads[1].off == 0x0288u && reads[1].kind == 'w' && reads[1].val == 0x0002u);
    CHECK(reads[2].off == 0x0388u && reads[2].kind == 'w' && reads[2].val == 0x0023u);
    CHECK(reads[3].off == 0x0388u && reads[3].kind == 'w' && reads[3].val == 0x0023u);
    CHECK(w39[1] == 0x0004);
    CHECK(ds[0x49D2u] == 0x00 && ds[0x49D3u] == 0x02);
    return 0;
}

static int test_regression_word_retarget_22_then_23(void)
{
    pr_u3b4_entry2_env env;

    /* Regression. The first +0x188 read returns 0x22 and retargets to 0x0200. The second read at 0x0388 returns 0x23, which is 0x22 or more,
     * so a third fresh read at 0x0388 returns 0x23, so b2 = 4. A cached value would give 3 and w2 = 3. The expected word is 0x0004. */
    reset();
    env = make_env();
    add_far_word(0x0100u + 0x186u, 0x0000u);
    add_far_word(0x0100u + 0x188u, 0x0022u);
    add_far_word(0x0200u + 0x188u, 0x0023u);
    trigger_on = 1;
    trigger_off = 0x0100u + 0x188u;
    retarget_to = 0x0200u;
    CHECK(pr_u3b4_entry2_0000(&env) == PR_U3B4_ENTRY2_OK);
    CHECK(n_reads == 4);
    CHECK(reads[2].off == 0x0388u && reads[2].val == 0x0023u);
    CHECK(reads[3].off == 0x0388u && reads[3].val == 0x0023u);
    CHECK(w39[1] == 0x0004);
    return 0;
}

static int test_regression_byte_retarget(void)
{
    pr_u3b4_entry2_env env;

    /* Regression, the byte path. The first +0x188 word read returns 1 (below 2) and retargets to 0x0200. The byte read is then at
     * 0x0200 + 0x188 = 0x0388 and returns 0x07, so b2 = 7 and w2 = 7. A cached first byte would give 1. */
    reset();
    env = make_env();
    add_far_word(0x0100u + 0x186u, 0x0000u);
    add_far_word(0x0100u + 0x188u, 0x0001u);
    add_far(0x0200u + 0x188u, 0x07u);
    trigger_on = 1;
    trigger_off = 0x0100u + 0x188u;
    retarget_to = 0x0200u;
    CHECK(pr_u3b4_entry2_0000(&env) == PR_U3B4_ENTRY2_OK);
    CHECK(n_reads == 3);
    CHECK(reads[2].kind == 'b' && reads[2].off == 0x0388u && reads[2].val == 0x07u);
    CHECK(w39[1] == 0x0007);
    return 0;
}

static int test_no_call_on_read_failure(void)
{
    pr_u3b4_entry2_env env;

    /* The second record read fails: no call is made and no DS byte is updated. */
    reset();
    env = make_env();
    add_far_word(0x0100u + 0x186u, 0x0005u);
    add_far_word(0x0100u + 0x188u, 0x0000u);
    ds[0x6DDCu] = 0x0B;
    fail_at_read = 1;
    CHECK(pr_u3b4_entry2_0000(&env) == PR_U3B4_ENTRY2_FAR_READ);
    CHECK(n39 == 0 && n1b52 == 0);
    CHECK(ds[0x6DDCu] == 0x0B);
    return 0;
}

int main(void)
{
    if (test_guards() != 0) return 1;
    if (test_basic_words_and_update() != 0) return 1;
    if (test_ah_carry() != 0) return 1;
    if (test_signed_clamp_negative() != 0) return 1;
    if (test_bands() != 0) return 1;
    if (test_regression_word_retarget_2_then_23() != 0) return 1;
    if (test_regression_word_retarget_22_then_23() != 0) return 1;
    if (test_regression_byte_retarget() != 0) return 1;
    if (test_no_call_on_read_failure() != 0) return 1;
    printf("PASS overlay_u3b4_entry2_0000 focused checks\n");
    return 0;
}
