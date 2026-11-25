/* Focused behaviour check for the unit 0x1c +0x2B33 (WHO) translation. Callees are fabricated hooks; the expected words are written
 * out by hand from the file's instructions. Checked: argument guards and the frame-wrap rejection; the call order (A, D, C, E); the
 * caller's entry AH on the first word; the first callee's returned AH on the four display words; the StrStore words (DS segment,
 * 0x70C8, SS segment, frame address entry_sp-0x102, 0x00FF); and the StrStore's returned AH on the final word. The byte-level
 * comparison against the original bytes is the separate oracle in tests/overlay/ovl1c_2b33_oracle.py. */
#include "poolrad/recovered/overlay_u1c_who_2b33.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

static int seq;                       /* call order: a=1, d=2, c=3, e=4 */
static int n_a, n_d, n_c, n_e;
static int order[8], n_order;
static uint16_t a_arg;
static uint16_t d_w[4];
static uint16_t c_w[5];
static uint16_t e_w[3];
static uint16_t ret_a, ret_c;

static uint16_t hook_a(void *ctx, uint16_t arg)
{
    (void)ctx;
    a_arg = arg;
    n_a++;
    order[n_order++] = 1;
    return ret_a;
}

static void hook_d(void *ctx, uint16_t p0, uint16_t p1, uint16_t p2, uint16_t p3)
{
    (void)ctx;
    d_w[0] = p0; d_w[1] = p1; d_w[2] = p2; d_w[3] = p3;
    n_d++;
    order[n_order++] = 2;
}

static uint16_t hook_c(void *ctx, uint16_t ds_seg, uint16_t src_off, uint16_t ss_seg, uint16_t dst_off, uint16_t max)
{
    (void)ctx;
    c_w[0] = ds_seg; c_w[1] = src_off; c_w[2] = ss_seg; c_w[3] = dst_off; c_w[4] = max;
    n_c++;
    order[n_order++] = 3;
    return ret_c;
}

static void hook_e(void *ctx, uint16_t word, uint16_t ptr_seg, uint16_t ptr_off)
{
    (void)ctx;
    e_w[0] = word; e_w[1] = ptr_seg; e_w[2] = ptr_off;
    n_e++;
    order[n_order++] = 4;
}

static void reset(void)
{
    seq = 0;
    n_a = n_d = n_c = n_e = 0;
    n_order = 0;
    memset(order, 0, sizeof order);
    memset(d_w, 0, sizeof d_w);
    memset(c_w, 0, sizeof c_w);
    memset(e_w, 0, sizeof e_w);
    a_arg = 0;
    ret_a = ret_c = 0;
}

static pr_u1c_who_env make_env(void)
{
    pr_u1c_who_env env;
    env.ds_seg = 0x1234;
    env.ss_seg = 0x2345;
    env.entry_sp = 0x0500;
    env.hooks.call_2b_2a = hook_a;
    env.hooks.call_709_0dd3 = hook_d;
    env.hooks.call_af8_0b25 = hook_c;
    env.hooks.call_ba_2c48 = hook_e;
    env.ctx = NULL;
    return env;
}

static int test_guards(void)
{
    pr_u1c_who_env env, bad;

    reset();
    env = make_env();
    CHECK(pr_u1c_who_2b33(NULL, 0) == PR_U1C_WHO_ARGUMENT);
    bad = env;
    bad.hooks.call_ba_2c48 = NULL;
    CHECK(pr_u1c_who_2b33(&bad, 0) == PR_U1C_WHO_ARGUMENT);
    bad = env;
    bad.entry_sp = 0x0101;                           /* BP-0x100 would be below zero */
    CHECK(pr_u1c_who_2b33(&bad, 0) == PR_U1C_WHO_ARGUMENT);
    CHECK(n_a == 0 && n_d == 0 && n_c == 0 && n_e == 0);   /* rejected before any call */

    /* entry_sp = 0x0102 is the smallest accepted value: the frame starts at offset 0. */
    env.entry_sp = 0x0102;
    CHECK(pr_u1c_who_2b33(&env, 0) == PR_U1C_WHO_OK);
    CHECK(c_w[3] == 0x0000);
    return 0;
}

static int test_words_and_order(void)
{
    pr_u1c_who_env env = make_env();

    /* Entry AH 0x12: the first word is 0x1201. The first callee returns 0x3456, so the display words carry AH 0x34.
     * The StrStore returns 0x7800, so the final word is 0x7800. Frame address: 0x0500 - 0x102 = 0x03FE. */
    reset();
    ret_a = 0x3456;
    ret_c = 0x7800;
    CHECK(pr_u1c_who_2b33(&env, 0x12) == PR_U1C_WHO_OK);
    CHECK(n_a == 1 && a_arg == 0x1201);
    CHECK(n_d == 1);
    CHECK(d_w[0] == 0x3401 && d_w[1] == 0x3411 && d_w[2] == 0x3426 && d_w[3] == 0x3416);
    CHECK(n_c == 1);
    CHECK(c_w[0] == 0x1234 && c_w[1] == 0x70C8 && c_w[2] == 0x2345 && c_w[3] == 0x03FE && c_w[4] == 0x00FF);
    CHECK(n_e == 1);
    CHECK(e_w[0] == 0x7800 && e_w[1] == 0x1234 && e_w[2] == 0x5D92);
    CHECK(n_order == 4 && order[0] == 1 && order[1] == 2 && order[2] == 3 && order[3] == 4);
    return 0;
}

static int test_high_byte_carry(void)
{
    pr_u1c_who_env env = make_env();

    /* Entry AH 0x00, first return 0xFF80: the display words carry 0xFF, the StrStore's return 0x0001 gives 0x0000 in the final word. */
    reset();
    ret_a = 0xFF80;
    ret_c = 0x0001;
    CHECK(pr_u1c_who_2b33(&env, 0x00) == PR_U1C_WHO_OK);
    CHECK(a_arg == 0x0001);
    CHECK(d_w[0] == 0xFF01 && d_w[1] == 0xFF11 && d_w[2] == 0xFF26 && d_w[3] == 0xFF16);
    CHECK(e_w[0] == 0x0000);
    return 0;
}

int main(void)
{
    if (test_guards() != 0) return 1;
    if (test_words_and_order() != 0) return 1;
    if (test_high_byte_carry() != 0) return 1;
    printf("PASS overlay_u1c_who_2b33 focused checks\n");
    return 0;
}
