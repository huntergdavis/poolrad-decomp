/* Focused behaviour check for the unit 0x1c +0x02E0 (LOAD CHARACTER) translation. Callees and far memory are fabricated here;
 * expected values are written out by hand from the file's instructions. Checked: argument guards; the AH carried into each call
 * (entry AH, the first callee's returned AH, the flag-path AH from [0x4436]); count masking and the unsigned index bound; the
 * chain walk, its null stops and the no-store rule; the flag clear; call order and the read of [0x5D92]/[0x5D94] after the
 * third call; the flag-path skip; and a far-read failure stopping the routine. The byte-level comparison against the original
 * bytes is the separate oracle in tests/overlay/ovl1c_02e0_oracle.py. */
#include "poolrad/recovered/overlay_u1c_load_character_02e0.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

#define MAX_FAR 16

static uint8_t ds[PR_U1C_LC02E0_DS_SIZE];
static struct { uint16_t seg, off, word; } far_tab[MAX_FAR];
static size_t far_n;
static uint16_t ret_a, ret_b;
static int fail_far_after;   /* far reads succeed this many times, then fail */
static int far_reads;

/* call log */
static uint16_t log_a_arg, log_b_arg, log_c_first, log_c_second, log_d_off, log_d_seg;
static int n_a, n_b, n_c, n_d;
static int c_writes_5d92;    /* when set, the third callee writes [0x5D92] = 0x1234 (to test read order) */

static uint16_t hook_a(void *ctx, uint16_t arg)
{
    (void)ctx;
    log_a_arg = arg;
    n_a++;
    return ret_a;
}

static uint16_t hook_b(void *ctx, uint16_t arg)
{
    (void)ctx;
    log_b_arg = arg;
    n_b++;
    return ret_b;
}

static void hook_c(void *ctx, uint16_t first, uint16_t second)
{
    (void)ctx;
    log_c_first = first;
    log_c_second = second;
    n_c++;
    if (c_writes_5d92) {
        ds[0x5D92] = 0x34;
        ds[0x5D93] = 0x12;
    }
}

static void hook_d(void *ctx, uint16_t off, uint16_t seg)
{
    (void)ctx;
    log_d_off = off;
    log_d_seg = seg;
    n_d++;
}

static int hook_far(void *ctx, uint16_t seg, uint16_t off, uint16_t *out)
{
    size_t k;
    (void)ctx;
    far_reads++;
    if (fail_far_after >= 0 && far_reads > fail_far_after) {
        return 1;
    }
    *out = 0;
    for (k = 0; k < far_n; k++) {
        if (far_tab[k].seg == seg && far_tab[k].off == off) {
            *out = far_tab[k].word;
        }
    }
    return 0;
}

static void put16(uint16_t off, uint16_t v)
{
    ds[off] = (uint8_t)(v & 0xFF);
    ds[(uint16_t)(off + 1u)] = (uint8_t)(v >> 8);
}

static uint16_t get16(uint16_t off)
{
    return (uint16_t)(ds[off] | (ds[(uint16_t)(off + 1u)] << 8));
}

static void reset(void)
{
    memset(ds, 0, sizeof ds);
    far_n = 0;
    ret_a = ret_b = 0;
    fail_far_after = -1;
    far_reads = 0;
    log_a_arg = log_b_arg = log_c_first = log_c_second = log_d_off = log_d_seg = 0;
    n_a = n_b = n_c = n_d = 0;
    c_writes_5d92 = 0;
}

static void add_far(uint16_t seg, uint16_t off, uint16_t word)
{
    far_tab[far_n].seg = seg;
    far_tab[far_n].off = off;
    far_tab[far_n].word = word;
    far_n++;
}

static pr_u1c_lc02e0_env make_env(void)
{
    pr_u1c_lc02e0_env env;
    env.ds = ds;
    env.ds_size = sizeof ds;
    env.hooks.call_2b_2a = hook_a;
    env.hooks.call_2b_25 = hook_b;
    env.hooks.call_78_2a = hook_c;
    env.hooks.call_ba_767 = hook_d;
    env.hooks.far_read16 = hook_far;
    env.ctx = NULL;
    return env;
}

static int test_arguments(void)
{
    pr_u1c_lc02e0_env env;
    pr_u1c_lc02e0_env bad;

    reset();
    env = make_env();
    CHECK(pr_u1c_load_character_02e0(NULL, 0) == PR_U1C_LC02E0_ARGUMENT);
    bad = env;
    bad.ds = NULL;
    CHECK(pr_u1c_load_character_02e0(&bad, 0) == PR_U1C_LC02E0_ARGUMENT);
    bad = env;
    bad.ds_size = PR_U1C_LC02E0_DS_SIZE - 1u;
    CHECK(pr_u1c_load_character_02e0(&bad, 0) == PR_U1C_LC02E0_ARGUMENT);
    bad = env;
    bad.hooks.far_read16 = NULL;
    CHECK(pr_u1c_load_character_02e0(&bad, 0) == PR_U1C_LC02E0_ARGUMENT);
    CHECK(n_a == 0 && n_b == 0 && ds[0x4430] == 0);   /* rejected before any effect */
    return 0;
}

static int test_ah_and_null_walk(void)
{
    pr_u1c_lc02e0_env env;

    /* Entry AH 0x12, first callee returns 0x3456: the second argument carries AH 0x34. Count 5 with hi bit clear, chain of two
     * nodes then null: the walk stops on the null (no store), so [0x5D92] keeps its prior value; no flag path, no calls c or d. */
    reset();
    env = make_env();
    ret_a = 0x3456;
    ret_b = 0x0005;
    put16(0x5D96, 0x0200);
    put16(0x5D98, 0x3000);
    put16(0x5D92, 0xAAAA);
    put16(0x5D94, 0xBBBB);
    add_far(0x3000, 0x0200 + 0x104, 0x0400);
    add_far(0x3000, 0x0200 + 0x106, 0x3000);
    add_far(0x3000, 0x0400 + 0x104, 0x0000);
    add_far(0x3000, 0x0400 + 0x106, 0x0000);
    CHECK(pr_u1c_load_character_02e0(&env, 0x12) == PR_U1C_LC02E0_OK);
    CHECK(n_a == 1 && log_a_arg == 0x1201);
    CHECK(n_b == 1 && log_b_arg == 0x3401);
    CHECK(get16(0x5D92) == 0xAAAA && get16(0x5D94) == 0xBBBB);
    CHECK(n_c == 0 && n_d == 0);
    CHECK(ds[0x4430] == 1);
    return 0;
}

static int test_flag_clear_and_order(void)
{
    pr_u1c_lc02e0_env env;

    /* Head = (0x4000, 0x0200), count 0 with hi bit set: out := head; [0x84D2], [0x84D3] nonzero; [0x4436]:[0x4438] equal to the
     * out pair, so [0x4430] := 0. Third call: AH = high byte of [0x4436] = 0x02, so first = 0x0200, second = 0x0201. The
     * third callee writes [0x5D92] = 0x1234 before the fourth call, which must see it. */
    reset();
    env = make_env();
    ret_a = 0x0000;
    ret_b = 0x0080;
    put16(0x5D96, 0x0200);
    put16(0x5D98, 0x4000);
    put16(0x4436, 0x0200);
    put16(0x4438, 0x4000);
    ds[0x84D2] = 1;
    ds[0x84D3] = 1;
    c_writes_5d92 = 1;
    CHECK(pr_u1c_load_character_02e0(&env, 0x00) == PR_U1C_LC02E0_OK);
    CHECK(ds[0x4430] == 0);
    CHECK(n_c == 1 && log_c_first == 0x0200 && log_c_second == 0x0201);
    CHECK(n_d == 1 && log_d_off == 0x1234 && log_d_seg == 0x4000);
    CHECK(ds[0x84D2] == 0 && ds[0x84D3] == 0);
    return 0;
}

static int test_flag_skip_and_mismatch(void)
{
    pr_u1c_lc02e0_env env;

    /* Same head, but [0x4438] differs from the out segment: the flag stays 1, the calls still run because the branch
     * conditions hold. Then the hi bit clear: no calls, no clears. */
    reset();
    env = make_env();
    ret_b = 0x0080;
    put16(0x5D96, 0x0200);
    put16(0x5D98, 0x4000);
    put16(0x4436, 0x0200);
    put16(0x4438, 0x5000);
    ds[0x84D2] = 1;
    ds[0x84D3] = 1;
    CHECK(pr_u1c_load_character_02e0(&env, 0x00) == PR_U1C_LC02E0_OK);
    CHECK(ds[0x4430] == 1);
    CHECK(n_c == 1 && n_d == 1);

    reset();
    env = make_env();
    ret_b = 0x0000;
    put16(0x5D96, 0x0200);
    put16(0x5D98, 0x4000);
    ds[0x84D2] = 1;
    ds[0x84D3] = 1;
    CHECK(pr_u1c_load_character_02e0(&env, 0x00) == PR_U1C_LC02E0_OK);
    CHECK(n_c == 0 && n_d == 0);
    CHECK(ds[0x84D2] == 1 && ds[0x84D3] == 1);
    return 0;
}

static int test_far_failure(void)
{
    pr_u1c_lc02e0_env env;

    /* The first walk step's second read (the segment word) fails: the routine stops, no store, no calls c or d. */
    reset();
    env = make_env();
    ret_b = 0x0082;
    put16(0x5D96, 0x0200);
    put16(0x5D98, 0x4000);
    put16(0x5D92, 0xAAAA);
    put16(0x5D94, 0xBBBB);
    ds[0x84D2] = 1;
    ds[0x84D3] = 1;
    add_far(0x4000, 0x0200 + 0x104, 0x0400);
    add_far(0x4000, 0x0200 + 0x106, 0x4000);
    fail_far_after = 1;
    CHECK(pr_u1c_load_character_02e0(&env, 0x00) == PR_U1C_LC02E0_FAR_READ);
    CHECK(n_c == 0 && n_d == 0);
    CHECK(get16(0x5D92) == 0xAAAA && get16(0x5D94) == 0xBBBB);
    CHECK(ds[0x4430] == 1);
    return 0;
}

int main(void)
{
    if (test_arguments() != 0) return 1;
    if (test_ah_and_null_walk() != 0) return 1;
    if (test_flag_clear_and_order() != 0) return 1;
    if (test_flag_skip_and_mismatch() != 0) return 1;
    if (test_far_failure() != 0) return 1;
    printf("PASS overlay_u1c_load_character_02e0 focused checks\n");
    return 0;
}
