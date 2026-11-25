/* Vector driver for the unit 0x1c +0x2E10 (PROGRAM) translation. Reads cases from stdin and, for each, runs the C translation with hooks
 * that log their calls, apply the case's per-callee DS writes at entry, and return the case's AX values. Used by
 * tests/overlay/ovl1c_2e10_oracle.py, which runs the same cases through the original bytes under Unicorn and compares the output.
 *
 * Input (one case per block):
 *   case
 *   entry_ah <n>
 *   ds <off> <byte>            sparse initial DS bytes (all others 0)
 *   ret_a <word> / ret_b <word>
 *   eff <a|b|c|d|e|f|g> <off> <byte>   DS byte written at that callee's entry, in order (a 0x2B:0x2A, b 0x2B:0x25, c 0x78:0x25,
 *                                      d 0x82:0x25, e 0xBA:0x27D9, f near 0x2DD3, g near 0x0001)
 *   end
 * Output per case: the call lines in call order, `status <n>`, `ds <off> <byte>` for every DS byte that differs from its initial
 * value (ascending, offsets below PR_U1C_PROGRAM_DS_SIZE), then `endcase`. */
#include "poolrad/recovered/overlay_u1c_program_2e10.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_EFF 8
#define NCALLEES 7

typedef struct { uint16_t off; uint8_t val; } eff_entry;

static uint8_t ds[PR_U1C_PROGRAM_DS_SIZE];
static uint8_t ds_init[PR_U1C_PROGRAM_DS_SIZE];
static eff_entry eff[NCALLEES][MAX_EFF];
static size_t eff_n[NCALLEES];
static uint16_t ret_a, ret_b;

static void apply_eff(int which)
{
    size_t k;
    for (k = 0; k < eff_n[which]; k++) {
        ds[eff[which][k].off] = eff[which][k].val;
    }
}

static uint16_t hook_a(void *ctx, uint16_t arg)
{
    (void)ctx;
    printf("call a %u\n", (unsigned)arg);
    apply_eff(0);
    return ret_a;
}

static uint16_t hook_b(void *ctx, uint16_t arg)
{
    (void)ctx;
    printf("call b %u\n", (unsigned)arg);
    apply_eff(1);
    return ret_b;
}

static void hook_78(void *ctx)
{
    (void)ctx;
    printf("call 78 25\n");
    apply_eff(2);
}

static void hook_82(void *ctx)
{
    (void)ctx;
    printf("call 82 25\n");
    apply_eff(3);
}

static void hook_ba(void *ctx)
{
    (void)ctx;
    printf("call ba 27d9\n");
    apply_eff(4);
}

static void hook_2dd3(void *ctx)
{
    (void)ctx;
    printf("call near 2dd3\n");
    apply_eff(5);
}

static void hook_0001(void *ctx)
{
    (void)ctx;
    printf("call near 0001\n");
    apply_eff(6);
}

static int run_case(unsigned long entry_ah)
{
    pr_u1c_program_env env;
    pr_u1c_program_status st;
    size_t k;

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

    st = pr_u1c_program_2e10(&env, (uint8_t)entry_ah);
    printf("status %d\n", (int)st);
    for (k = 0; k < sizeof ds; k++) {
        if (ds[k] != ds_init[k]) {
            printf("ds %u %u\n", (unsigned)k, (unsigned)ds[k]);
        }
    }
    printf("endcase\n");
    return 0;
}

static int which_callee(const char *tok)
{
    if (strcmp(tok, "a") == 0) return 0;
    if (strcmp(tok, "b") == 0) return 1;
    if (strcmp(tok, "c") == 0) return 2;
    if (strcmp(tok, "d") == 0) return 3;
    if (strcmp(tok, "e") == 0) return 4;
    if (strcmp(tok, "f") == 0) return 5;
    if (strcmp(tok, "g") == 0) return 6;
    return -1;
}

int main(void)
{
    char line[256];
    char tok[4][64];
    int in_case = 0;
    unsigned long entry_ah = 0;

    memset(ds, 0, sizeof ds);
    memset(ds_init, 0, sizeof ds_init);
    ret_a = ret_b = 0;
    while (fgets(line, sizeof line, stdin) != NULL) {
        int n = sscanf(line, "%63s %63s %63s %63s", tok[0], tok[1], tok[2], tok[3]);
        if (n < 1) {
            continue;
        }
        if (strcmp(tok[0], "case") == 0) {
            memset(ds, 0, sizeof ds);
            memset(ds_init, 0, sizeof ds_init);
            eff_n[0] = eff_n[1] = eff_n[2] = eff_n[3] = eff_n[4] = eff_n[5] = eff_n[6] = 0;
            ret_a = ret_b = 0;
            entry_ah = 0;
            in_case = 1;
        } else if (strcmp(tok[0], "entry_ah") == 0 && n == 2) {
            entry_ah = strtoul(tok[1], NULL, 0);
        } else if (strcmp(tok[0], "ds") == 0 && n == 3) {
            unsigned long off = strtoul(tok[1], NULL, 0), val = strtoul(tok[2], NULL, 0);
            if (off >= sizeof ds) return 1;
            ds[off] = (uint8_t)val;
            ds_init[off] = (uint8_t)val;
        } else if (strcmp(tok[0], "ret_a") == 0 && n == 2) {
            ret_a = (uint16_t)strtoul(tok[1], NULL, 0);
        } else if (strcmp(tok[0], "ret_b") == 0 && n == 2) {
            ret_b = (uint16_t)strtoul(tok[1], NULL, 0);
        } else if (strcmp(tok[0], "eff") == 0 && n == 4) {
            int which = which_callee(tok[1]);
            unsigned long off;
            if (which < 0) return 1;
            if (eff_n[which] >= MAX_EFF) return 1;
            off = strtoul(tok[2], NULL, 0);
            if (off >= sizeof ds) return 1;
            eff[which][eff_n[which]].off = (uint16_t)off;
            eff[which][eff_n[which]].val = (uint8_t)strtoul(tok[3], NULL, 0);
            eff_n[which]++;
        } else if (strcmp(tok[0], "end") == 0 && in_case) {
            if (run_case(entry_ah) != 0) return 1;
            in_case = 0;
        } else {
            fprintf(stderr, "harness: unrecognised line\n");
            return 1;
        }
    }
    return in_case ? 1 : 0;
}
