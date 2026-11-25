/* Vector driver for the unit 0x1c +0x2B33 (WHO) translation. Reads cases from stdin and, for each, runs the C translation with hooks
 * that print their arguments in pushed order. Used by tests/overlay/ovl1c_2b33_oracle.py, which runs the same cases through the
 * original bytes under Unicorn and compares the output.
 *
 * Input (one case per block):
 *   case
 *   entry_ah <n>
 *   entry_sp <n>
 *   ds_seg <n>
 *   ss_seg <n>
 *   ret_a <word>
 *   ret_c <word>
 *   end
 * Output per case: the call lines in call order (`call a p0`, `call d p0 p1 p2 p3`, `call c p0..p4`, `call e p0 p1 p2`), then
 * `status <n>`, then `endcase`. */
#include "poolrad/recovered/overlay_u1c_who_2b33.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint16_t ret_a, ret_c;

static uint16_t hook_a(void *ctx, uint16_t arg)
{
    (void)ctx;
    printf("call a %u\n", (unsigned)arg);
    return ret_a;
}

static void hook_d(void *ctx, uint16_t p0, uint16_t p1, uint16_t p2, uint16_t p3)
{
    (void)ctx;
    printf("call d %u %u %u %u\n", (unsigned)p0, (unsigned)p1, (unsigned)p2, (unsigned)p3);
}

static uint16_t hook_c(void *ctx, uint16_t ds_seg, uint16_t src_off, uint16_t ss_seg, uint16_t dst_off, uint16_t max)
{
    (void)ctx;
    printf("call c %u %u %u %u %u\n", (unsigned)ds_seg, (unsigned)src_off, (unsigned)ss_seg, (unsigned)dst_off, (unsigned)max);
    return ret_c;
}

static void hook_e(void *ctx, uint16_t word, uint16_t ptr_seg, uint16_t ptr_off)
{
    (void)ctx;
    printf("call e %u %u %u\n", (unsigned)word, (unsigned)ptr_seg, (unsigned)ptr_off);
}

static int run_case(unsigned long entry_ah, unsigned long entry_sp, unsigned long ds_seg, unsigned long ss_seg)
{
    pr_u1c_who_env env;
    pr_u1c_who_status st;

    env.ds_seg = (uint16_t)ds_seg;
    env.ss_seg = (uint16_t)ss_seg;
    env.entry_sp = (uint16_t)entry_sp;
    env.hooks.call_2b_2a = hook_a;
    env.hooks.call_709_0dd3 = hook_d;
    env.hooks.call_af8_0b25 = hook_c;
    env.hooks.call_ba_2c48 = hook_e;
    env.ctx = NULL;
    st = pr_u1c_who_2b33(&env, (uint8_t)entry_ah);
    printf("status %d\n", (int)st);
    printf("endcase\n");
    return 0;
}

int main(void)
{
    char line[256];
    char tok[2][64];
    int in_case = 0;
    unsigned long entry_ah = 0, entry_sp = 0, ds_seg = 0, ss_seg = 0;

    ret_a = ret_c = 0;
    while (fgets(line, sizeof line, stdin) != NULL) {
        int n = sscanf(line, "%63s %63s", tok[0], tok[1]);
        if (n < 1) {
            continue;
        }
        if (strcmp(tok[0], "case") == 0) {
            entry_ah = entry_sp = ds_seg = ss_seg = 0;
            ret_a = ret_c = 0;
            in_case = 1;
        } else if (strcmp(tok[0], "entry_ah") == 0 && n == 2) {
            entry_ah = strtoul(tok[1], NULL, 0);
        } else if (strcmp(tok[0], "entry_sp") == 0 && n == 2) {
            entry_sp = strtoul(tok[1], NULL, 0);
        } else if (strcmp(tok[0], "ds_seg") == 0 && n == 2) {
            ds_seg = strtoul(tok[1], NULL, 0);
        } else if (strcmp(tok[0], "ss_seg") == 0 && n == 2) {
            ss_seg = strtoul(tok[1], NULL, 0);
        } else if (strcmp(tok[0], "ret_a") == 0 && n == 2) {
            ret_a = (uint16_t)strtoul(tok[1], NULL, 0);
        } else if (strcmp(tok[0], "ret_c") == 0 && n == 2) {
            ret_c = (uint16_t)strtoul(tok[1], NULL, 0);
        } else if (strcmp(tok[0], "end") == 0 && in_case) {
            if (run_case(entry_ah, entry_sp, ds_seg, ss_seg) != 0) return 1;
            in_case = 0;
        } else {
            fprintf(stderr, "harness: unrecognised line\n");
            return 1;
        }
    }
    return in_case ? 1 : 0;
}
