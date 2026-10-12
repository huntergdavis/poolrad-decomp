/* Private smoke for the slot-A party input: reads the ignored world save and the ignored character
 * records its basename slots name, builds the party with pr_u6f_party_build, and runs the merged
 * PARTYSTRENGTH model over it. Prints only counts and aggregate numbers (no names, no record bytes). */
#include "poolrad/recovered/overlay_u1c_party_strength_12c6.h"
#include "poolrad/recovered/overlay_u6f_party_load.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint8_t save[PR_U6F_SAVGAM_SIZE];
static uint8_t recbuf[PR_U6F_PARTY_SLOTS][PR_U6F_PARTY_RECORD_SIZE];
static pr_u6f_party party;
static pr_rec_ecl_loader_state st;

int main(int argc, char **argv)
{
    FILE *f;
    pr_u6f_party_plan plan;
    const uint8_t *recs[PR_U6F_PARTY_SLOTS];
    unsigned k, present = 0;
    static const uint8_t script[4] = { 0x1D, 0x01, 0x10, 0x97 };   /* dest: window-2 word 0x9710 */
    pr_u1c_pstr_report rep;
    pr_u1c_pstr_status ps;
    uint16_t saved_count_word;
    if (argc != 3) {
        fprintf(stderr, "usage: %s WORLD_SAVE RECORD_DIR\n", argv[0]);
        return 2;
    }
    f = fopen(argv[1], "rb");
    if (!f || fread(save, 1, sizeof save, f) != sizeof save) { fprintf(stderr, "world save unreadable\n"); return 2; }
    fclose(f);
    if (pr_u6f_party_plan_from_savgam(save, sizeof save, &plan) != PR_U6F_PARTY_OK) { fprintf(stderr, "plan failed\n"); return 2; }
    memset(recs, 0, sizeof recs);
    for (k = 0; k < plan.count; k++) {
        char path[512];
        size_t n = strlen(argv[2]), i;
        if (n + 1 + plan.name_len[k] + 4 + 1 > sizeof path) return 2;
        memcpy(path, argv[2], n); path[n++] = '/';
        for (i = 0; i < plan.name_len[k]; i++) path[n++] = (char)toupper(plan.name_text[k][i]);
        memcpy(path + n, ".SAV", 5);
        f = fopen(path, "rb");
        if (f && fread(recbuf[k], 1, PR_U6F_PARTY_RECORD_SIZE, f) == PR_U6F_PARTY_RECORD_SIZE) { recs[k] = recbuf[k]; present++; }
        if (f) fclose(f);
    }
    if (pr_u6f_party_build(recs, plan.count, &party) != PR_U6F_PARTY_OK) { fprintf(stderr, "build failed\n"); return 2; }
    memset(&st, 0, sizeof st);
    st.script = script; st.script_size = sizeof script; st.pc = 0x9900u; st.latch = 0x1D;
    ps = pr_u1c_op_party_strength(&st, party.head, &rep);
    saved_count_word = (uint16_t)(save[0x801 + 2 * 0x33E] | (save[0x801 + 2 * 0x33E + 1] << 8));
    printf("count=%u present=%u loaded=%u slots=", plan.count, present, party.loaded);
    for (k = 0; k < party.loaded; k++) printf("%s%u", k ? "," : "", party.slot[k]);
    printf(" strength_status=%d visited=%u strength=%u saved_window1_counter=%u\n", (int)ps, rep.visited, rep.sum, saved_count_word);
    return ps == PR_U1C_PSTR_OK ? 0 : 1;
}
