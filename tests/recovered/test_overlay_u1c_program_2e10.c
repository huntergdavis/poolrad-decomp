/* Focused behaviour check for the unit 0x1c +0x2E10 (PROGRAM) translation. Callees are fabricated hooks that record their order and
 * may write the DS image; expected values are written out by hand from the file's instructions. Checked: argument guards; the
 * flag path (the copy into [0x5D92]/[0x5D94], the clear of [0x4430], and the AH it leaves in the first word) and the non-flag path
 * (AH from the entry); the second word's AH from the first callee's return; the selector (the low byte of the second return) for
 * 0, 8, 9 and an unlisted value; the [0x49ED] save and restore around the first near call; and the call order. The byte-level
 * comparison against the original bytes is the separate oracle in tests/overlay/ovl1c_2e10_oracle.py. */
#include "poolrad/recovered/overlay_u1c_program_2e10.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

static uint8_t ds[PR_U1C_PROGRAM_DS_SIZE];
static uint16_t ret_a, ret_b;
static uint16_t a_arg, b_arg;
static int order[8], n_order;
static uint8_t near_2dd3_write;          /* when set, the first near callee overwrites [0x49ED] */

static uint16_t hook_a(void *ctx, uint16_t arg)
{
    (void)ctx;
    a_arg = arg;
    order[n_order++] = 1;
    return ret_a;
}

static uint16_t hook_b(void *ctx, uint16_t arg)
{
    (void)ctx;
    b_arg = arg;
    order[n_order++] = 2;
    return ret_b;
}

static void hook_78(void *ctx)
{
    (void)ctx;
    order[n_order++] = 3;
}

static void hook_82(void *ctx)
{
    (void)ctx;
    order[n_order++] = 4;
}

static void hook_ba(void *ctx)
{
    (void)ctx;
    order[n_order++] = 5;
}

static void hook_2dd3(void *ctx)
{
    (void)ctx;
    order[n_order++] = 6;
    if (near_2dd3_write) {
        ds[0x49ED] = 0xAA;
        ds[0x49EE] = 0xBB;
    }
}

static void hook_0001(void *ctx)
{
    (void)ctx;
    order[n_order++] = 7;
}

static void reset(void)
{
    memset(ds, 0, sizeof ds);
    ret_a = ret_b = 0;
    a_arg = b_arg = 0;
    memset(order, 0, sizeof order);
    n_order = 0;
    near_2dd3_write = 0;
}

static pr_u1c_program_env make_env(void)
{
    pr_u1c_program_env env;
    env.ds = ds;
    env.ds_size = sizeof ds;
    env.hooks.call_2b_2a = hook_a;
    env.hooks.call_2b_25 = hook_b;
    env.hooks.call_78_25 = hook_78;
    env.hooks.call_82_25 = hook_82;
    env.hooks.call_ba_27d9 = hook_ba;
    env.hooks.call_near_2dd3 = hook_2dd3;
    env.hooks.call_near_0001 = hook_0001;
    env.ctx = NULL;
    return env;
}

static uint16_t get16(uint16_t off)
{
    return (uint16_t)(ds[off] | (ds[(uint16_t)(off + 1u)] << 8));
}

static void put16(uint16_t off, uint16_t v)
{
    ds[off] = (uint8_t)(v & 0xFF);
    ds[(uint16_t)(off + 1u)] = (uint8_t)(v >> 8);
}

static int test_guards(void)
{
    pr_u1c_program_env env, bad;

    reset();
    env = make_env();
    CHECK(pr_u1c_program_2e10(NULL, 0) == PR_U1C_PROGRAM_ARGUMENT);
    bad = env;
    bad.ds = NULL;
    CHECK(pr_u1c_program_2e10(&bad, 0) == PR_U1C_PROGRAM_ARGUMENT);
    bad = env;
    bad.ds_size = PR_U1C_PROGRAM_DS_SIZE - 1u;
    CHECK(pr_u1c_program_2e10(&bad, 0) == PR_U1C_PROGRAM_ARGUMENT);
    bad = env;
    bad.hooks.call_near_0001 = NULL;
    CHECK(pr_u1c_program_2e10(&bad, 0) == PR_U1C_PROGRAM_ARGUMENT);
    CHECK(n_order == 0);
    return 0;
}

static int test_flag_path_and_ah(void)
{
    pr_u1c_program_env env = make_env();

    /* Flag set: AX := [0x4436] = 0x1234 (AH 0x12), so the first word is 0x1201 whatever the entry AH is. The copy stores
     * [0x5D92] = 0x1234 and [0x5D94] = [0x4438] = 0x5678, and the flag is cleared. The first callee returns 0x3C00, so the
     * second word is 0x3C01. The selector is 5: no further call. */
    reset();
    env = make_env();
    ds[0x4430] = 1;
    put16(0x4436, 0x1234);
    put16(0x4438, 0x5678);
    ret_a = 0x3C00;
    ret_b = 0x0005;
    CHECK(pr_u1c_program_2e10(&env, 0x9A) == PR_U1C_PROGRAM_OK);
    CHECK(a_arg == 0x1201);
    CHECK(b_arg == 0x3C01);
    CHECK(get16(0x5D92) == 0x1234 && get16(0x5D94) == 0x5678);
    CHECK(ds[0x4430] == 0);
    CHECK(n_order == 2 && order[0] == 1 && order[1] == 2);
    return 0;
}

static int test_nonflag_path_and_ah(void)
{
    pr_u1c_program_env env = make_env();

    /* Flag clear: the first word takes AH from the entry (0x9A), so it is 0x9A01. Nothing is copied to [0x5D92]. */
    reset();
    env = make_env();
    put16(0x4436, 0x1234);
    put16(0x5D92, 0xAAAA);
    ret_a = 0x0000;
    ret_b = 0x0005;
    CHECK(pr_u1c_program_2e10(&env, 0x9A) == PR_U1C_PROGRAM_OK);
    CHECK(a_arg == 0x9A01);
    CHECK(b_arg == 0x0001);
    CHECK(get16(0x5D92) == 0xAAAA);
    CHECK(n_order == 2);
    return 0;
}

static int test_selector(void)
{
    pr_u1c_program_env env = make_env();

    /* Selector 0: 78:25 then BA:27D9. The selector is the low byte of the second return (0x0100 has low byte 0). */
    reset();
    env = make_env();
    ret_b = 0x0100;
    CHECK(pr_u1c_program_2e10(&env, 0x00) == PR_U1C_PROGRAM_OK);
    CHECK(n_order == 4 && order[2] == 3 && order[3] == 5);

    /* Selector 8: 82:25 only. */
    reset();
    env = make_env();
    ret_b = 0x0008;
    CHECK(pr_u1c_program_2e10(&env, 0x00) == PR_U1C_PROGRAM_OK);
    CHECK(n_order == 3 && order[2] == 4);

    /* Selector 9 with a high byte set: the near pair, in order. */
    reset();
    env = make_env();
    ret_b = 0x7F09;
    CHECK(pr_u1c_program_2e10(&env, 0x00) == PR_U1C_PROGRAM_OK);
    CHECK(n_order == 4 && order[2] == 6 && order[3] == 7);

    /* An unlisted selector (5, and 0x10): no further call. */
    reset();
    env = make_env();
    ret_b = 0x0010;
    CHECK(pr_u1c_program_2e10(&env, 0x00) == PR_U1C_PROGRAM_OK);
    CHECK(n_order == 2);
    return 0;
}

static int test_saved_pc_restore(void)
{
    pr_u1c_program_env env = make_env();

    /* The near callee at +2DD3 overwrites [0x49ED] with 0xBBAA; the routine restores the value it saved before the call. */
    reset();
    env = make_env();
    put16(0x49ED, 0x1357);
    near_2dd3_write = 1;
    ret_b = 0x0009;
    CHECK(pr_u1c_program_2e10(&env, 0x00) == PR_U1C_PROGRAM_OK);
    CHECK(get16(0x49ED) == 0x1357);
    CHECK(n_order == 4 && order[2] == 6 && order[3] == 7);
    return 0;
}

int main(void)
{
    if (test_guards() != 0) return 1;
    if (test_flag_path_and_ah() != 0) return 1;
    if (test_nonflag_path_and_ah() != 0) return 1;
    if (test_selector() != 0) return 1;
    if (test_saved_pc_restore() != 0) return 1;
    printf("PASS overlay_u1c_program_2e10 focused checks\n");
    return 0;
}
