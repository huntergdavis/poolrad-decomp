/* Vector driver for the unit 0x1c +0x02E0 translation. Reads cases from stdin and, for each case, runs the C translation with
 * hooks that log their arguments, apply the case's per-callee DS writes, and answer far reads from the case's sparse table. Used
 * by tests/overlay/ovl1c_02e0_oracle.py, which runs the same cases through the original bytes under Unicorn and compares output.
 *
 * Input (one case per block, numbers in C strtoul base 0 form):
 *   case
 *   entry_ah <n>
 *   ds <off> <byte>            sparse initial DS bytes (all others 0)
 *   far <seg> <off> <word>     sparse far words (all others 0)
 *   ret_a <word> / ret_b <word>
 *   eff <a|b|c|d> <off> <byte> DS byte written at that callee's entry, in order
 *   end
 * Output per case: the call lines in call order, then `status <n>`, then `ds <off> <byte>` for every DS byte that differs from
 * its initial value, ascending, then `endcase`. */
#include "poolrad/recovered/overlay_u1c_load_character_02e0.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LIST 256
#define MAX_FAR_BYTES 1024
#define MAX_EFF 8

/* Far memory is byte-addressed, as in the original: a `far seg off word` line writes two bytes (low first), in line order, and a
 * later byte at the same address replaces an earlier one. Unlisted addresses read as zero. */
typedef struct { uint16_t seg, off; uint8_t val; } far_byte;
typedef struct { uint16_t off; uint8_t val; } eff_entry;

static uint8_t ds[PR_U1C_LC02E0_DS_SIZE];
static uint8_t ds_init[PR_U1C_LC02E0_DS_SIZE];
static far_byte far_tab[MAX_FAR_BYTES];
static size_t far_n;
static uint16_t ret_a, ret_b;
static eff_entry eff[4][MAX_EFF];
static size_t eff_n[4];

static void apply_eff(int which)
{
    size_t k;
    for (k = 0; k < eff_n[which]; k++) {
        ds[eff[which][k].off] = eff[which][k].val;
    }
}

static uint16_t hook_2b_2a(void *ctx, uint16_t arg)
{
    (void)ctx;
    printf("call a %u\n", (unsigned)arg);
    apply_eff(0);
    return ret_a;
}

static uint16_t hook_2b_25(void *ctx, uint16_t arg)
{
    (void)ctx;
    printf("call b %u\n", (unsigned)arg);
    apply_eff(1);
    return ret_b;
}

static void hook_78_2a(void *ctx, uint16_t first, uint16_t second)
{
    (void)ctx;
    printf("call c %u %u\n", (unsigned)first, (unsigned)second);
    apply_eff(2);
}

static void hook_ba_767(void *ctx, uint16_t off, uint16_t seg)
{
    (void)ctx;
    printf("call d %u %u\n", (unsigned)off, (unsigned)seg);
    apply_eff(3);
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

static int hook_far_read16(void *ctx, uint16_t seg, uint16_t off, uint16_t *out)
{
    (void)ctx;
    *out = (uint16_t)(far_byte_at(seg, off) | ((uint16_t)far_byte_at(seg, (uint16_t)(off + 1u)) << 8));
    return 0;
}

static int fail(const char *msg)
{
    fprintf(stderr, "harness: %s\n", msg);
    return 1;
}

static int run_case(uint8_t entry_ah)
{
    pr_u1c_lc02e0_env env;
    pr_u1c_lc02e0_status st;
    size_t k;

    env.ds = ds;
    env.ds_size = sizeof ds;
    env.hooks.call_2b_2a = hook_2b_2a;
    env.hooks.call_2b_25 = hook_2b_25;
    env.hooks.call_78_2a = hook_78_2a;
    env.hooks.call_ba_767 = hook_ba_767;
    env.hooks.far_read16 = hook_far_read16;
    env.ctx = NULL;

    st = pr_u1c_load_character_02e0(&env, entry_ah);
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
    far_n = 0;
    eff_n[0] = eff_n[1] = eff_n[2] = eff_n[3] = 0;
    ret_a = ret_b = 0;

    while (fgets(line, sizeof line, stdin) != NULL) {
        int n = sscanf(line, "%63s %63s %63s %63s", tok[0], tok[1], tok[2], tok[3]);
        if (n < 1) {
            continue;
        }
        if (strcmp(tok[0], "case") == 0) {
            memset(ds, 0, sizeof ds);
            far_n = 0;
            eff_n[0] = eff_n[1] = eff_n[2] = eff_n[3] = 0;
            ret_a = ret_b = 0;
            entry_ah = 0;
            in_case = 1;
        } else if (strcmp(tok[0], "entry_ah") == 0 && n == 2) {
            entry_ah = strtoul(tok[1], NULL, 0);
        } else if (strcmp(tok[0], "ds") == 0 && n == 3) {
            unsigned long off = strtoul(tok[1], NULL, 0), val = strtoul(tok[2], NULL, 0);
            if (off >= sizeof ds) return fail("ds offset out of range");
            ds[off] = (uint8_t)val;
            ds_init[off] = (uint8_t)val;
        } else if (strcmp(tok[0], "far") == 0 && n == 4) {
            uint16_t seg = (uint16_t)strtoul(tok[1], NULL, 0);
            uint16_t off = (uint16_t)strtoul(tok[2], NULL, 0);
            uint16_t word = (uint16_t)strtoul(tok[3], NULL, 0);
            if (far_n + 2 > MAX_FAR_BYTES) return fail("too many far bytes");
            far_tab[far_n].seg = seg;
            far_tab[far_n].off = off;
            far_tab[far_n].val = (uint8_t)(word & 0xFFu);
            far_n++;
            far_tab[far_n].seg = seg;
            far_tab[far_n].off = (uint16_t)(off + 1u);
            far_tab[far_n].val = (uint8_t)(word >> 8);
            far_n++;
        } else if (strcmp(tok[0], "ret_a") == 0 && n == 2) {
            ret_a = (uint16_t)strtoul(tok[1], NULL, 0);
        } else if (strcmp(tok[0], "ret_b") == 0 && n == 2) {
            ret_b = (uint16_t)strtoul(tok[1], NULL, 0);
        } else if (strcmp(tok[0], "eff") == 0 && n == 4) {
            int which = tok[1][0] == 'a' ? 0 : tok[1][0] == 'b' ? 1 : tok[1][0] == 'c' ? 2 : tok[1][0] == 'd' ? 3 : -1;
            if (which < 0) return fail("eff callee must be a, b, c or d");
            if (eff_n[which] >= MAX_EFF) return fail("too many eff entries");
            {
                unsigned long off = strtoul(tok[2], NULL, 0);
                if (off >= sizeof ds) return fail("eff offset out of range");
                eff[which][eff_n[which]].off = (uint16_t)off;
                eff[which][eff_n[which]].val = (uint8_t)strtoul(tok[3], NULL, 0);
                eff_n[which]++;
            }
        } else if (strcmp(tok[0], "end") == 0 && in_case) {
            /* The DS image starts from the case's sparse values, so the initial copy is the image before the run. */
            if (run_case((uint8_t)entry_ah) != 0) return 1;
            memset(ds_init, 0, sizeof ds_init);
            memset(ds, 0, sizeof ds);
            in_case = 0;
        } else {
            return fail("unrecognised line");
        }
    }
    return in_case ? fail("case without end") : 0;
}
