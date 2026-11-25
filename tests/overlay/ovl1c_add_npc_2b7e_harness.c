/* Vector driver for the unit 0x1c +0x2B7E ADD NPC model (live active-record resolution). Reads cases from stdin and, for
 * each, runs the C model. Three far records (indices 0..2) are host buffers at declared segment:offset pairs; one active
 * pointer names one of them. The add-NPC callback and the 0xBA:0x0BB8 resident may retarget the active pointer, as the
 * owner's insertion and resident routines can. Used by tests/overlay/ovl1c_add_npc_2b7e_oracle.py, which runs the same
 * cases through the original bytes under Unicorn and compares the output.
 *
 * Input (one case per block):
 *   case
 *   slot <1|2> <word>             the word the slot value callback returns for that slot (default 0)
 *   rec <idx> <seg> <off>         declare record idx (0..2) at segment:offset; undeclared records resolve to nothing
 *   recdata <idx> <hex>           the 0x11D record bytes of record idx as hex (default zero)
 *   active <idx>                  the active pointer at entry (default 0)
 *   npc_retarget <idx|-1>         the record the add-NPC callback selects (default -1: none)
 *   r0bb8_retarget <idx|-1>       the record the 0xBA:0x0BB8 callback selects (default -1: none)
 *   end
 * Output per case: the call lines in call order (`load 2`, `slot 1 <word>`, `npc <word>`, `r0bb8 <seg> <off>`,
 * `r0767 <seg> <off>`), `status <n>`, `rec <idx> <byte> <val>` for each record byte that differs from its input, then
 * `endcase`. */
#include "poolrad/recovered/overlay_u1c_add_npc_2b7e.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NREC 3

typedef struct {
    int declared;
    uint16_t seg, off;
    uint8_t in[PR_U1C_ADD_NPC_RECORD_SIZE];
    uint8_t now[PR_U1C_ADD_NPC_RECORD_SIZE];
} rec_t;

static rec_t rec[NREC];
static int active;
static int npc_retarget;
static int r0bb8_retarget;
static uint16_t slot_word[3];

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
    if (npc_retarget >= 0)
        active = npc_retarget;
}

static void hook_0bb8(void *ctx, uint16_t seg, uint16_t off)
{
    (void)ctx;
    printf("r0bb8 %u %u\n", (unsigned)seg, (unsigned)off);
    if (r0bb8_retarget >= 0)
        active = r0bb8_retarget;
}

static void hook_0767(void *ctx, uint16_t seg, uint16_t off)
{
    (void)ctx;
    printf("r0767 %u %u\n", (unsigned)seg, (unsigned)off);
}

static int hook_read(void *ctx, uint16_t *seg, uint16_t *off)
{
    (void)ctx;
    if (active < 0 || active >= NREC || !rec[active].declared)
        return 1;
    *seg = rec[active].seg;
    *off = rec[active].off;
    return 0;
}

static uint8_t *hook_bytes(void *ctx, uint16_t seg, uint16_t off, size_t size)
{
    int i;
    (void)ctx;
    if (size > PR_U1C_ADD_NPC_RECORD_SIZE)
        return NULL;
    for (i = 0; i < NREC; i++) {
        if (rec[i].declared && rec[i].seg == seg && rec[i].off == off)
            return rec[i].now;
    }
    return NULL;
}

static int hex_nibble(int c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static int run_case(void)
{
    pr_u1c_add_npc_hooks h;
    pr_u1c_add_npc_report rep;
    pr_u1c_add_npc_status st;
    int i;
    size_t k;

    h.context = NULL;
    h.load_operands = hook_load;
    h.slot_value = hook_slot;
    h.add_npc = hook_npc;
    h.read_active = hook_read;
    h.record_bytes = hook_bytes;
    h.resident_0bb8 = hook_0bb8;
    h.resident_0767 = hook_0767;
    for (i = 0; i < NREC; i++)
        memcpy(rec[i].now, rec[i].in, sizeof rec[i].now);
    st = pr_u1c_op_add_npc(&h, &rep);
    printf("status %d\n", (int)st);
    for (i = 0; i < NREC; i++) {
        for (k = 0; k < sizeof rec[i].now; k++) {
            if (rec[i].now[k] != rec[i].in[k]) {
                printf("rec %d %u %u\n", i, (unsigned)k, (unsigned)rec[i].now[k]);
            }
        }
    }
    printf("endcase\n");
    return 0;
}

int main(void)
{
    char line[4096];
    int in_case = 0;

    memset(rec, 0, sizeof rec);
    memset(slot_word, 0, sizeof slot_word);
    active = 0;
    npc_retarget = -1;
    r0bb8_retarget = -1;
    while (fgets(line, sizeof line, stdin) != NULL) {
        char tok[4][64];
        int n = sscanf(line, "%63s %63s %63s %63s", tok[0], tok[1], tok[2], tok[3]);
        if (n < 1) {
            continue;
        }
        if (strcmp(tok[0], "case") == 0) {
            memset(rec, 0, sizeof rec);
            memset(slot_word, 0, sizeof slot_word);
            active = 0;
            npc_retarget = -1;
            r0bb8_retarget = -1;
            in_case = 1;
        } else if (strcmp(tok[0], "slot") == 0 && n == 3) {
            unsigned long s = strtoul(tok[1], NULL, 0), w = strtoul(tok[2], NULL, 0);
            if (s != 1u && s != 2u) return 1;
            slot_word[s] = (uint16_t)w;
        } else if (strcmp(tok[0], "rec") == 0 && n == 4) {
            unsigned long idx = strtoul(tok[1], NULL, 0);
            if (idx >= NREC) return 1;
            rec[idx].declared = 1;
            rec[idx].seg = (uint16_t)strtoul(tok[2], NULL, 0);
            rec[idx].off = (uint16_t)strtoul(tok[3], NULL, 0);
        } else if (strcmp(tok[0], "recdata") == 0 && n >= 2) {
            unsigned long idx = strtoul(tok[1], NULL, 0);
            char *p = strstr(line, "recdata") + 7;
            size_t k = 0;
            if (idx >= NREC) return 1;
            while (*p == ' ' || *p == '\t') p++;
            p += strlen(tok[1]);
            while (*p && k < sizeof rec[idx].in) {
                int hi, lo;
                while (*p == ' ' || *p == '\t') p++;
                if (*p == '\n' || *p == '\0') break;
                hi = hex_nibble(p[0]);
                lo = hex_nibble(p[1]);
                if (hi < 0 || lo < 0) return 1;
                rec[idx].in[k++] = (uint8_t)(hi * 16 + lo);
                p += 2;
            }
            if (k != sizeof rec[idx].in) return 1;
        } else if (strcmp(tok[0], "active") == 0 && n == 2) {
            active = (int)strtoul(tok[1], NULL, 0);
        } else if (strcmp(tok[0], "npc_retarget") == 0 && n == 2) {
            npc_retarget = (int)strtol(tok[1], NULL, 0);
        } else if (strcmp(tok[0], "r0bb8_retarget") == 0 && n == 2) {
            r0bb8_retarget = (int)strtol(tok[1], NULL, 0);
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
