/* Focused behaviour check for the unit 0x3B4 +0x016D (view refresh) translation. Callees and the far record are fabricated; expected values
 * are written out by hand from the file's instructions. Checked: argument guards; the flag path (the record word +0x1CC, [0x49FA]); the
 * full-register argument words of the first far call (entry AH) and of the second (the first return's AH); the low and high record
 * branches on [0x6AB1]; the case-map calls when [0x84E5] is set and the AL they store; the four-word second call; the [0x6AAC] clear;
 * the three-word third call; the coordinate path for flags 2 to 4, with its signed clamps to 0..0x27 and 0..0x1F and the CBW of each
 * byte; the no-call flags; and a far read failure that stops the routine. The byte-level comparison against the original bytes is the
 * separate oracle in tests/overlay/ovl3b4_016d_oracle.py. */
#include "poolrad/recovered/overlay_u3b4_view_refresh_016d.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

#define MAX_FAR 32

static uint8_t ds[PR_U3B4_016D_DS_SIZE];
static struct { uint16_t seg, off, word; } far_tab[MAX_FAR];
static size_t far_n;
static int fail_far_after;     /* far reads succeed this many times, then fail (-1: never) */
static int far_reads;

static uint16_t ret34, ret25, ret39;
static uint16_t w34[2], w25[4], w39[3], wcd[2];
static int n34, n25, n39, n00c7, n00fb, n012f, ncd, nz;
static uint16_t arg00c7, arg00fb, arg012f;
static int order[8], n_order;
static uint8_t map_result[3];   /* what the three case-map hooks return */

static uint16_t hook_34(void *ctx, uint16_t w1, uint16_t w2)
{
    (void)ctx;
    w34[0] = w1; w34[1] = w2; n34++; order[n_order++] = 1;
    return ret34;
}

static uint16_t hook_25(void *ctx, uint16_t w1, uint16_t w2, uint16_t w3, uint16_t w4)
{
    (void)ctx;
    w25[0] = w1; w25[1] = w2; w25[2] = w3; w25[3] = w4; n25++; order[n_order++] = 2;
    return ret25;
}

static uint16_t hook_39(void *ctx, uint16_t w1, uint16_t w2, uint16_t w3)
{
    (void)ctx;
    w39[0] = w1; w39[1] = w2; w39[2] = w3; n39++; order[n_order++] = 3;
    return ret39;
}

static uint8_t hook_00c7(void *ctx, uint16_t arg)
{
    (void)ctx;
    arg00c7 = arg; n00c7++; order[n_order++] = 4;
    return map_result[0];
}

static uint8_t hook_00fb(void *ctx, uint16_t arg)
{
    (void)ctx;
    arg00fb = arg; n00fb++; order[n_order++] = 5;
    return map_result[1];
}

static uint8_t hook_012f(void *ctx, uint16_t arg)
{
    (void)ctx;
    arg012f = arg; n012f++; order[n_order++] = 6;
    return map_result[2];
}

static void hook_cd(void *ctx, uint16_t w1, uint16_t w2)
{
    (void)ctx;
    wcd[0] = w1; wcd[1] = w2; ncd++; order[n_order++] = 7;
}

static void hook_z(void *ctx)
{
    (void)ctx;
    nz++; order[n_order++] = 8;
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

static void add_far(uint16_t seg, uint16_t off, uint16_t word)
{
    far_tab[far_n].seg = seg; far_tab[far_n].off = off; far_tab[far_n].word = word; far_n++;
}

/* A record at seg 0x4000, offset 0x0100: DS:0x49D2 holds the offset and DS:0x49D4 the segment. Fields are written by the caller. */
static void set_record_pointer(void)
{
    put16(0x49D2u, 0x0100u);
    put16(0x49D4u, 0x4000u);
}

static void reset(void)
{
    memset(ds, 0, sizeof ds);
    far_n = 0;
    fail_far_after = -1;
    far_reads = 0;
    ret34 = ret25 = ret39 = 0;
    memset(w34, 0, sizeof w34); memset(w25, 0, sizeof w25); memset(w39, 0, sizeof w39); memset(wcd, 0, sizeof wcd);
    n34 = n25 = n39 = n00c7 = n00fb = n012f = ncd = nz = 0;
    arg00c7 = arg00fb = arg012f = 0;
    memset(order, 0, sizeof order);
    n_order = 0;
    map_result[0] = map_result[1] = map_result[2] = 0;
}

static pr_u3b4_016d_env make_env(void)
{
    pr_u3b4_016d_env env;
    env.ds = ds;
    env.ds_size = sizeof ds;
    env.hooks.call_3d5_34 = hook_34;
    env.hooks.call_3d5_25 = hook_25;
    env.hooks.call_3d5_39 = hook_39;
    env.hooks.near_00c7 = hook_00c7;
    env.hooks.near_00fb = hook_00fb;
    env.hooks.near_012f = hook_012f;
    env.hooks.call_3cd_25 = hook_cd;
    env.hooks.near_0000 = hook_z;
    env.hooks.far_read16 = hook_far;
    env.ctx = NULL;
    return env;
}

static int test_guards(void)
{
    pr_u3b4_016d_env env, bad;

    reset();
    env = make_env();
    CHECK(pr_u3b4_view_refresh_016d(NULL, 0) == PR_U3B4_016D_ARGUMENT);
    bad = env;
    bad.ds = NULL;
    CHECK(pr_u3b4_view_refresh_016d(&bad, 0) == PR_U3B4_016D_ARGUMENT);
    bad = env;
    bad.ds_size = PR_U3B4_016D_DS_SIZE - 1u;
    CHECK(pr_u3b4_view_refresh_016d(&bad, 0) == PR_U3B4_016D_ARGUMENT);
    bad = env;
    bad.hooks.near_012f = NULL;
    CHECK(pr_u3b4_view_refresh_016d(&bad, 0) == PR_U3B4_016D_ARGUMENT);
    CHECK(n_order == 0);
    return 0;
}

static int test_flag1_low_branch_and_words(void)
{
    pr_u3b4_016d_env env;

    /* Flag 1 from the record word +0x1CC. Entry AH 0x55. Words: (0x55,0x11) = 0x5511, (0x55,0x22) = 0x5522. ret34 = 0x7A3C: [0x6AB1] = 0x3C
     * (not above 0x7F), so the record word at +0x1FA = 0x0100 gives DS[0x0100+0xC26] = DS[0x0D26] = 0x9E: AL := 0x9E, AH stays 0x7A.
     * [0x84E5] = 0: no case maps. Second call words: (7A,9E)=0x7A9E, then (7A,[6A61]=0x01)=0x7A01, (7A,[6A62]=0x02)=0x7A02,
     * (7A,[6A63]=0x03)=0x7A03. ret25 = 0 (AH 0); [0x1F6] = 0, so [0x6AAC] keeps its value. Third call words, AH 0:
     * 0x0011, 0x0022, 0x0033. Then the routine returns. */
    reset();
    env = make_env();
    set_record_pointer();
    add_far(0x4000u, 0x0100u + 0x1CCu, 1u);
    add_far(0x4000u, 0x0100u + 0x1FAu, 0x0100u);
    ds[0x0D26u] = 0x9E;
    ds[0x6AADu] = 0x11; ds[0x6AAEu] = 0x22; ds[0x6AAFu] = 0x33;
    ds[0x6A61u] = 0x01; ds[0x6A62u] = 0x02; ds[0x6A63u] = 0x03;
    ds[0x6AACu] = 0xEE;
    ret34 = 0x7A3Cu;
    ret25 = 0x0000u;
    CHECK(pr_u3b4_view_refresh_016d(&env, 0x55) == PR_U3B4_016D_OK);
    CHECK(n34 == 1 && w34[0] == 0x5511 && w34[1] == 0x5522);
    CHECK(ds[0x6AB1u] == 0x3C);
    CHECK(ds[0x6A60u] == 0x9E);
    CHECK(n25 == 1 && w25[0] == 0x7A9E && w25[1] == 0x7A01 && w25[2] == 0x7A02 && w25[3] == 0x7A03);
    CHECK(ds[0x6AACu] == 0xEE);
    CHECK(n39 == 1 && w39[0] == 0x0011 && w39[1] == 0x0022 && w39[2] == 0x0033);
    CHECK(n00c7 == 0 && n00fb == 0 && n012f == 0 && ncd == 0 && nz == 0);
    CHECK(n_order == 3 && order[0] == 1 && order[1] == 2 && order[2] == 3);
    return 0;
}

static int test_flag1_high_branch_and_clear(void)
{
    pr_u3b4_016d_env env;

    /* [0x6AB1] = 0x90 (above 0x7F): the record word +0x1FC = 0x0200 gives DS[0x0E26] = 0x5C. The record word +0x1F6 is nonzero, so
     * [0x6AAC] := 0. Flag from the record word +0x1CC = 1. */
    reset();
    env = make_env();
    set_record_pointer();
    add_far(0x4000u, 0x0100u + 0x1CCu, 1u);
    add_far(0x4000u, 0x0100u + 0x1FCu, 0x0200u);
    add_far(0x4000u, 0x0100u + 0x1F6u, 1u);
    ds[0x0E26u] = 0x5C;
    ds[0x6AACu] = 0xEE;
    ret34 = 0x0090u;
    CHECK(pr_u3b4_view_refresh_016d(&env, 0x00) == PR_U3B4_016D_OK);
    CHECK(ds[0x6AB1u] == 0x90 && ds[0x6A60u] == 0x5C);
    CHECK(ds[0x6AACu] == 0x00);
    return 0;
}

static int test_case_maps_when_flag84e5(void)
{
    pr_u3b4_016d_env env;

    /* [0x84E5] set: the three case maps take (AH:6F69) = 0x7A41, (AH:6F69) = 0x7A41 and (AH:6F68) = 0x7A07 (AH from ret34 = 0x7A3C).
     * They return AL and keep AH: 0x0F, 0x07, 0x08 stored at [0x6A61], [0x6A62], [0x6A63]. The second call then carries (7A,0x0F). */
    reset();
    env = make_env();
    set_record_pointer();
    add_far(0x4000u, 0x0100u + 0x1CCu, 1u);
    add_far(0x4000u, 0x0100u + 0x1FAu, 0x0100u);
    ds[0x0D26u] = 0x10;
    ds[0x84E5u] = 1;
    ds[0x6F69u] = 0x41;
    ds[0x6F68u] = 0x07;
    ret34 = 0x7A3Cu;
    map_result[0] = 0x0F; map_result[1] = 0x07; map_result[2] = 0x08;
    CHECK(pr_u3b4_view_refresh_016d(&env, 0x00) == PR_U3B4_016D_OK);
    CHECK(n00c7 == 1 && arg00c7 == 0x7A41);
    CHECK(n00fb == 1 && arg00fb == 0x7A41);
    CHECK(n012f == 1 && arg012f == 0x7A07);
    CHECK(ds[0x6A61u] == 0x0F && ds[0x6A62u] == 0x07 && ds[0x6A63u] == 0x08);
    CHECK(w25[0] == 0x7A10 && w25[1] == 0x7A0F && w25[2] == 0x7A07 && w25[3] == 0x7A08);
    CHECK(n_order == 6 && order[0] == 1 && order[1] == 4 && order[2] == 5 && order[3] == 6 && order[4] == 2 && order[5] == 3);
    return 0;
}

static int test_coordinate_path_clamps(void)
{
    pr_u3b4_016d_env env;

    /* Flag 2 (from [0x49FA], record word +0x1CC = 0). Table DS[0x3C76+2] = 0x10; record +0x186 = 0x0020: 0x10 + 0x20 - 2 = 0x2E, clamped
     * to 0x27. Record +0x188 = 0x0010: 0x10 - 2 = 0x0E, within 0..0x1F. Words: CBW(0x27) = 0x0027, CBW(0x0E) = 0x000E. Then the near
     * call to +0000. */
    reset();
    env = make_env();
    set_record_pointer();
    ds[0x49FAu] = 2;
    ds[0x3C78u] = 0x10;
    add_far(0x4000u, 0x0100u + 0x186u, 0x0020u);
    add_far(0x4000u, 0x0100u + 0x188u, 0x0010u);
    CHECK(pr_u3b4_view_refresh_016d(&env, 0x00) == PR_U3B4_016D_OK);
    CHECK(ncd == 1 && wcd[0] == 0x0027 && wcd[1] == 0x000E);
    CHECK(nz == 1);
    CHECK(n34 == 0 && n25 == 0 && n39 == 0);
    CHECK(n_order == 2 && order[0] == 7 && order[1] == 8);

    /* Flag 4 with negative results: table 0, record +0x186 = 0 gives 0 - 2 = 0xFE, which is -2 as a signed byte, so it clamps to 0.
     * Record +0x188 = 0x0000 also gives -2 and clamps to 0. Words are 0 and 0. */
    reset();
    env = make_env();
    set_record_pointer();
    ds[0x49FAu] = 4;
    ds[0x3C7Au] = 0x00;
    add_far(0x4000u, 0x0100u + 0x186u, 0x0000u);
    add_far(0x4000u, 0x0100u + 0x188u, 0x0000u);
    CHECK(pr_u3b4_view_refresh_016d(&env, 0x00) == PR_U3B4_016D_OK);
    CHECK(ncd == 1 && wcd[0] == 0x0000 && wcd[1] == 0x0000);
    return 0;
}

static int test_no_call_flags(void)
{
    pr_u3b4_016d_env env;

    /* Flags 0 and 5 make no call. */
    reset();
    env = make_env();
    set_record_pointer();
    ds[0x49FAu] = 0;
    CHECK(pr_u3b4_view_refresh_016d(&env, 0x00) == PR_U3B4_016D_OK);
    CHECK(n_order == 0);
    reset();
    env = make_env();
    set_record_pointer();
    ds[0x49FAu] = 5;
    CHECK(pr_u3b4_view_refresh_016d(&env, 0x00) == PR_U3B4_016D_OK);
    CHECK(n_order == 0);
    return 0;
}

static int test_far_failure(void)
{
    pr_u3b4_016d_env env;

    /* Reads: +0x1CC (1), +0x1FA after the first call (2), +0x1F6 after the second call (3). The third read fails, so the third call is
     * never made. */
    reset();
    env = make_env();
    set_record_pointer();
    add_far(0x4000u, 0x0100u + 0x1CCu, 1u);
    add_far(0x4000u, 0x0100u + 0x1FAu, 0x0100u);
    fail_far_after = 2;
    CHECK(pr_u3b4_view_refresh_016d(&env, 0x00) == PR_U3B4_016D_FAR_READ);
    CHECK(n34 == 1 && n25 == 1 && n39 == 0);
    return 0;
}

int main(void)
{
    if (test_guards() != 0) return 1;
    if (test_flag1_low_branch_and_words() != 0) return 1;
    if (test_flag1_high_branch_and_clear() != 0) return 1;
    if (test_case_maps_when_flag84e5() != 0) return 1;
    if (test_coordinate_path_clamps() != 0) return 1;
    if (test_no_call_flags() != 0) return 1;
    if (test_far_failure() != 0) return 1;
    printf("PASS overlay_u3b4_view_refresh_016d focused checks\n");
    return 0;
}
