/* Vector driver for the unit 0x1c +0x2B7E ADD NPC model. Reads cases from stdin and, for each, runs the C model with
 * callbacks that log their calls, return the case's slot words, and answer nothing else. Used by
 * tests/overlay/ovl1c_add_npc_2b7e_oracle.py, which runs the same cases through the original bytes under Unicorn.
 *
 * Input (one case per block):
 *   case
 *   slot <1|2> <word>          the word the slot value callback returns for that slot (default 0)
 *   rec <seg> <off>            the far record pointer held at DS:0x5D92 (off) and DS:0x5D94 (seg)
 *   recdata <hex>              the 0x11D record bytes as hex (default zero)
 *   end
 * Output per case: the call lines in call order (`load 2`, `slot 1 <word>`, `npc <word>`, `r0bb8 <seg> <off>`,
 * `r0767 <seg> <off>`), `status <n>`, `rec <idx> <val>` for each record byte that differs from its input, then
 * `endcase`. */
#include "poolrad/recovered/overlay_u1c_add_npc_2b7e.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint16_t slot_word[3];
static uint8_t rec_in[PR_U1C_ADD_NPC_RECORD_SIZE];
static uint8_t rec_now[PR_U1C_ADD_NPC_RECORD_SIZE];

static pr_rec_ecl_loader_status hook_load(void *ctx, uint8_t count)
{
    (void)ctx;
    printf("load %u\n", (unsigned)count);
    return PR_REC_ECL_LOADER_OK;
}

static pr_rec_ecl_window_status hook_slot(void *ctx, uint8_t slot, uint16_t *value)
{
    (void)ctx;
    *value = slot_word[slot];
    printf("slot %u %u\n", (unsigned)slot, (unsigned)*value);
    return PR_REC_ECL_WINDOW_OK;
}

static void hook_npc(void *ctx, uint16_t word)
{
    (void)ctx;
    printf("npc %u\n", (unsigned)word);
}

static void hook_0bb8(void *ctx, uint16_t seg, uint16_t off)
{
    (void)ctx;
    printf("r0bb8 %u %u\n", (unsigned)seg, (unsigned)off);
}

static void hook_0767(void *ctx, uint16_t seg, uint16_t off)
{
    (void)ctx;
    printf("r0767 %u %u\n", (unsigned)seg, (unsigned)off);
}

static int hex_nibble(int c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static int run_case(uint16_t seg, uint16_t off)
{
    pr_u1c_add_npc_hooks h;
    pr_u1c_add_npc_record r;
    pr_u1c_add_npc_report rep;
    pr_u1c_add_npc_status st;
    size_t i;

    h.context = NULL;
    h.load_operands = hook_load;
    h.slot_value = hook_slot;
    h.add_npc = hook_npc;
    h.resident_0bb8 = hook_0bb8;
    h.resident_0767 = hook_0767;
    memcpy(rec_now, rec_in, sizeof rec_now);
    r.record = rec_now;
    r.record_size = sizeof rec_now;
    r.record_off = off;
    r.record_seg = seg;
    st = pr_u1c_op_add_npc(&h, &r, &rep);
    printf("status %d\n", (int)st);
    for (i = 0; i < sizeof rec_now; i++) {
        if (rec_now[i] != rec_in[i]) {
            printf("rec %u %u\n", (unsigned)i, (unsigned)rec_now[i]);
        }
    }
    printf("endcase\n");
    return 0;
}

int main(void)
{
    char line[4096];
    int in_case = 0;
    uint16_t seg = 0, off = 0;

    memset(slot_word, 0, sizeof slot_word);
    memset(rec_in, 0, sizeof rec_in);
    while (fgets(line, sizeof line, stdin) != NULL) {
        char tok[3][64];
        int n = sscanf(line, "%63s %63s %63s", tok[0], tok[1], tok[2]);
        if (n < 1) {
            continue;
        }
        if (strcmp(tok[0], "case") == 0) {
            memset(slot_word, 0, sizeof slot_word);
            memset(rec_in, 0, sizeof rec_in);
            seg = off = 0;
            in_case = 1;
        } else if (strcmp(tok[0], "slot") == 0 && n == 3) {
            unsigned long s = strtoul(tok[1], NULL, 0), w = strtoul(tok[2], NULL, 0);
            if (s != 1u && s != 2u) return 1;
            slot_word[s] = (uint16_t)w;
        } else if (strcmp(tok[0], "rec") == 0 && n == 3) {
            seg = (uint16_t)strtoul(tok[1], NULL, 0);
            off = (uint16_t)strtoul(tok[2], NULL, 0);
        } else if (strcmp(tok[0], "recdata") == 0) {
            char *p = strstr(line, "recdata") + 7;
            size_t k = 0;
            while (*p && k < sizeof rec_in) {
                int hi, lo;
                while (*p == ' ' || *p == '\t') p++;
                if (*p == '\n' || *p == '\0') break;
                hi = hex_nibble(p[0]);
                lo = hex_nibble(p[1]);
                if (hi < 0 || lo < 0) return 1;
                rec_in[k++] = (uint8_t)(hi * 16 + lo);
                p += 2;
            }
            if (k != sizeof rec_in) return 1;
        } else if (strcmp(tok[0], "end") == 0 && in_case) {
            if (run_case(seg, off) != 0) return 1;
            in_case = 0;
        } else {
            fprintf(stderr, "harness: unrecognised line\n");
            return 1;
        }
    }
    return in_case ? 1 : 0;
}
