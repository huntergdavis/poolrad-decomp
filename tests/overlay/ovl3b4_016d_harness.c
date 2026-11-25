/* Vector driver for the unit 0x3B4 +0x016D (view refresh) translation. Reads cases from stdin and, for each, runs the C translation with
 * hooks that log their calls, apply the case's DS writes at entry, answer far reads from the case's byte-addressed far table, and
 * return the case's values. The near case maps return the value stated in docs/re/U3B4_CASE_MAPS.md (AL), which the Unicorn oracle
 * checks against the original bytes.
 *
 * Input (one case per block):
 *   case
 *   entry_ah <n>
 *   ds <off> <byte>             sparse initial DS bytes (all others 0)
 *   far <seg> <off> <word>      far words, written low byte first in line order (all others 0)
 *   ret34 <word>  ret25 <word>  ret39 <word>
 *   eff <34|25|39|cd|z> <off> <byte>   DS byte written at that callee's entry, in order
 *   end
 * Output per case: the call lines in call order, `status <n>`, `ds <off> <byte>` for each DS byte that differs from its initial value,
 * then `endcase`. */
#include "poolrad/recovered/overlay_u3b4_view_refresh_016d.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_EFF 8
#define MAX_FAR_BYTES 1024
#define NCALLEES 5

typedef struct { uint16_t off; uint8_t val; } eff_entry;
typedef struct { uint16_t seg, off; uint8_t val; } far_byte;

static uint8_t ds[PR_U3B4_016D_DS_SIZE];
static uint8_t ds_init[PR_U3B4_016D_DS_SIZE];
static eff_entry eff[NCALLEES][MAX_EFF];
static size_t eff_n[NCALLEES];
static far_byte far_tab[MAX_FAR_BYTES];
static size_t far_n;
static uint16_t ret34, ret25, ret39;

static void apply_eff(int which)
{
    size_t k;
    for (k = 0; k < eff_n[which]; k++) {
        ds[eff[which][k].off] = eff[which][k].val;
    }
}

static uint16_t hook_34(void *ctx, uint16_t w1, uint16_t w2)
{
    (void)ctx;
    printf("call 34 %u %u\n", (unsigned)w1, (unsigned)w2);
    apply_eff(0);
    return ret34;
}

static uint16_t hook_25(void *ctx, uint16_t w1, uint16_t w2, uint16_t w3, uint16_t w4)
{
    (void)ctx;
    printf("call 25 %u %u %u %u\n", (unsigned)w1, (unsigned)w2, (unsigned)w3, (unsigned)w4);
    apply_eff(1);
    return ret25;
}

static uint16_t hook_39(void *ctx, uint16_t w1, uint16_t w2, uint16_t w3)
{
    (void)ctx;
    printf("call 39 %u %u %u\n", (unsigned)w1, (unsigned)w2, (unsigned)w3);
    apply_eff(2);
    return ret39;
}

/* docs/re/U3B4_CASE_MAPS.md: the AL each near case map returns for argument byte b. */
static uint8_t map_00c7(uint16_t arg)
{
    uint8_t b = (uint8_t)(arg & 0xFFu);
    if (b == 0x08u || b == 0x41u) return 0x00u;
    if (b == 0xE8u) return 0x0Fu;
    return 0x0Fu;
}

static uint8_t map_00fb(uint16_t arg)
{
    uint8_t b = (uint8_t)(arg & 0xFFu);
    if (b == 0x08u) return 0x08u;
    if (b == 0x41u || b == 0xE8u) return 0x07u;
    return 0x07u;
}

static uint8_t map_012f(uint16_t arg)
{
    uint8_t b = (uint8_t)(arg & 0xFFu);
    if (b == 0x0Bu) return 0x08u;
    if (b == 0x09u) return 0x06u;
    if (b == 0xDBu) return 0x06u;
    if (b == 0x06u) return 0x01u;
    return 0x06u;
}

static uint8_t hook_00c7(void *ctx, uint16_t arg)
{
    (void)ctx;
    printf("near 00c7 %u\n", (unsigned)arg);
    return map_00c7(arg);
}

static uint8_t hook_00fb(void *ctx, uint16_t arg)
{
    (void)ctx;
    printf("near 00fb %u\n", (unsigned)arg);
    return map_00fb(arg);
}

static uint8_t hook_012f(void *ctx, uint16_t arg)
{
    (void)ctx;
    printf("near 012f %u\n", (unsigned)arg);
    return map_012f(arg);
}

static void hook_cd(void *ctx, uint16_t w1, uint16_t w2)
{
    (void)ctx;
    printf("call cd %u %u\n", (unsigned)w1, (unsigned)w2);
    apply_eff(3);
}

static void hook_z(void *ctx)
{
    (void)ctx;
    printf("near 0000\n");
    apply_eff(4);
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

static int hook_far(void *ctx, uint16_t seg, uint16_t off, uint16_t *out)
{
    (void)ctx;
    *out = (uint16_t)(far_byte_at(seg, off) | ((uint16_t)far_byte_at(seg, (uint16_t)(off + 1u)) << 8));
    return 0;
}

static int which_callee(const char *tok)
{
    if (strcmp(tok, "34") == 0) return 0;
    if (strcmp(tok, "25") == 0) return 1;
    if (strcmp(tok, "39") == 0) return 2;
    if (strcmp(tok, "cd") == 0) return 3;
    if (strcmp(tok, "z") == 0) return 4;
    return -1;
}

static int run_case(unsigned long entry_ah)
{
    pr_u3b4_016d_env env;
    pr_u3b4_016d_status st;
    size_t k;

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

    st = pr_u3b4_view_refresh_016d(&env, (uint8_t)entry_ah);
    printf("status %d\n", (int)st);
    for (k = 0; k < sizeof ds; k++) {
        if (ds[k] != ds_init[k]) {
            printf("ds %u %u\n", (unsigned)k, (unsigned)ds[k]);
        }
    }
    printf("endcase\n");
    return 0;
}

int main(void)
{
    char line[256];
    char tok[4][64];
    int in_case = 0;
    unsigned long entry_ah = 0;

    memset(ds, 0, sizeof ds);
    memset(ds_init, 0, sizeof ds_init);
    ret34 = ret25 = ret39 = 0;
    far_n = 0;
    while (fgets(line, sizeof line, stdin) != NULL) {
        int n = sscanf(line, "%63s %63s %63s %63s", tok[0], tok[1], tok[2], tok[3]);
        if (n < 1) {
            continue;
        }
        if (strcmp(tok[0], "case") == 0) {
            memset(ds, 0, sizeof ds);
            memset(ds_init, 0, sizeof ds_init);
            eff_n[0] = eff_n[1] = eff_n[2] = eff_n[3] = eff_n[4] = 0;
            far_n = 0;
            ret34 = ret25 = ret39 = 0;
            entry_ah = 0;
            in_case = 1;
        } else if (strcmp(tok[0], "entry_ah") == 0 && n == 2) {
            entry_ah = strtoul(tok[1], NULL, 0);
        } else if (strcmp(tok[0], "ds") == 0 && n == 3) {
            unsigned long off = strtoul(tok[1], NULL, 0), val = strtoul(tok[2], NULL, 0);
            if (off >= sizeof ds) return 1;
            ds[off] = (uint8_t)val;
            ds_init[off] = (uint8_t)val;
        } else if (strcmp(tok[0], "far") == 0 && n == 4) {
            uint16_t seg = (uint16_t)strtoul(tok[1], NULL, 0);
            uint16_t off = (uint16_t)strtoul(tok[2], NULL, 0);
            uint16_t word = (uint16_t)strtoul(tok[3], NULL, 0);
            if (far_n + 2 > MAX_FAR_BYTES) return 1;
            far_tab[far_n].seg = seg; far_tab[far_n].off = off; far_tab[far_n].val = (uint8_t)(word & 0xFFu); far_n++;
            far_tab[far_n].seg = seg; far_tab[far_n].off = (uint16_t)(off + 1u); far_tab[far_n].val = (uint8_t)(word >> 8); far_n++;
        } else if (strcmp(tok[0], "ret34") == 0 && n == 2) {
            ret34 = (uint16_t)strtoul(tok[1], NULL, 0);
        } else if (strcmp(tok[0], "ret25") == 0 && n == 2) {
            ret25 = (uint16_t)strtoul(tok[1], NULL, 0);
        } else if (strcmp(tok[0], "ret39") == 0 && n == 2) {
            ret39 = (uint16_t)strtoul(tok[1], NULL, 0);
        } else if (strcmp(tok[0], "eff") == 0 && n == 4) {
            int which = which_callee(tok[1]);
            unsigned long off;
            if (which < 0 || eff_n[which] >= MAX_EFF) return 1;
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
