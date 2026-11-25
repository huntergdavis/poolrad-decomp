/* Vector driver for the unit 0x3B4 +0x0000 translation. Reads cases from stdin and, for each, runs the C translation with hooks that log
 * their calls, apply the case's DS writes at entry, and answer far reads from the case's byte-addressed far table. Used by
 * tests/overlay/ovl3b4_0000_oracle.py, which runs the same cases through the original bytes under Unicorn and compares the output.
 *
 * Input (one case per block):
 *   case
 *   ds <off> <byte>             sparse initial DS bytes (all others 0)
 *   far <seg> <off> <word>      far words, written low byte first in line order (all others 0)
 *   eff <39|1b52> <off> <byte>  DS byte written at that callee's entry, in order
 *   end
 * Output per case: the call lines in call order (`call 39 w1 w2 w3 w4 w5`, `call 1b52`), `status <n>`, `ds <off> <byte>` for each DS byte that
 * differs from its initial value, then `endcase`. */
#include "poolrad/recovered/overlay_u3b4_entry2_0000.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_EFF 8
#define MAX_FAR_BYTES 1024
#define NCALLEES 2

typedef struct { uint16_t off; uint8_t val; } eff_entry;
typedef struct { uint16_t seg, off; uint8_t val; } far_byte;

static uint8_t ds[PR_U3B4_ENTRY2_DS_SIZE];
static uint8_t ds_init[PR_U3B4_ENTRY2_DS_SIZE];
static eff_entry eff[NCALLEES][MAX_EFF];
static size_t eff_n[NCALLEES];
static far_byte far_tab[MAX_FAR_BYTES];
static size_t far_n;

static void apply_eff(int which)
{
    size_t k;
    for (k = 0; k < eff_n[which]; k++) {
        ds[eff[which][k].off] = eff[which][k].val;
    }
}

static void hook_39(void *ctx, uint16_t w1, uint16_t w2, uint16_t w3, uint16_t w4, uint16_t w5)
{
    (void)ctx;
    printf("call 39 %u %u %u %u %u\n", (unsigned)w1, (unsigned)w2, (unsigned)w3, (unsigned)w4, (unsigned)w5);
    apply_eff(0);
}

static void hook_1b52(void *ctx)
{
    (void)ctx;
    printf("call 1b52\n");
    apply_eff(1);
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

static int hook_far8(void *ctx, uint16_t seg, uint16_t off, uint8_t *out)
{
    (void)ctx;
    *out = far_byte_at(seg, off);
    return 0;
}

static int which_callee(const char *tok)
{
    if (strcmp(tok, "39") == 0) return 0;
    if (strcmp(tok, "1b52") == 0) return 1;
    return -1;
}

static int run_case(void)
{
    pr_u3b4_entry2_env env;
    pr_u3b4_entry2_status st;
    size_t k;

    env.ds = ds;
    env.ds_size = sizeof ds;
    env.hooks.call_3e9_39 = hook_39;
    env.hooks.call_464_1b52 = hook_1b52;
    env.hooks.far_read16 = hook_far;
    env.hooks.far_read8 = hook_far8;
    env.ctx = NULL;

    st = pr_u3b4_entry2_0000(&env);
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

    memset(ds, 0, sizeof ds);
    memset(ds_init, 0, sizeof ds_init);
    far_n = 0;
    while (fgets(line, sizeof line, stdin) != NULL) {
        int n = sscanf(line, "%63s %63s %63s %63s", tok[0], tok[1], tok[2], tok[3]);
        if (n < 1) {
            continue;
        }
        if (strcmp(tok[0], "case") == 0) {
            memset(ds, 0, sizeof ds);
            memset(ds_init, 0, sizeof ds_init);
            eff_n[0] = eff_n[1] = 0;
            far_n = 0;
            in_case = 1;
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
            if (run_case() != 0) return 1;
            in_case = 0;
        } else {
            fprintf(stderr, "harness: unrecognised line\n");
            return 1;
        }
    }
    return in_case ? 1 : 0;
}
