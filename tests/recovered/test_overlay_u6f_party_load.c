/* Synthetic test for the unit 0x6f party-building host model. The world save, basename slots and character
 * records are fabricated. It checks the plan parse (count, Pascal slots, host bounds), the record fix-ups
 * (far-pointer slots cleared, +0xBF first free index), list order and links, skipped absent files, the source
 * left untouched, and the composition with the PARTYSTRENGTH model against an independent sum. */
#include "poolrad/recovered/overlay_u6f_party_load.h"
#include "poolrad/recovered/overlay_u1c_party_strength_12c6.h"
#include "poolrad/recovered/overlay_u9b_record_incr_01af.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

static uint8_t save[PR_U6F_SAVGAM_SIZE];
static uint8_t rec[PR_U6F_PARTY_SLOTS][PR_U6F_PARTY_RECORD_SIZE];
static uint8_t orig[PR_U6F_PARTY_SLOTS][PR_U6F_PARTY_RECORD_SIZE];
static pr_u6f_party party;

static void make_save(unsigned count)
{
    unsigned k, i;
    memset(save, 0x5A, sizeof save);
    save[PR_U6F_PARTY_COUNT_OFFSET] = (uint8_t)count;
    for (k = 0; k < PR_U6F_PARTY_SLOTS; k++) {
        uint8_t *s = save + PR_U6F_PARTY_NAMES_OFFSET + k * PR_U6F_PARTY_NAME_STRIDE;
        s[0] = (uint8_t)(3 + k);
        for (i = 1; i <= s[0]; i++) s[i] = (uint8_t)('a' + k);
    }
}

static void make_records(void)
{
    unsigned k, i;
    for (k = 0; k < PR_U6F_PARTY_SLOTS; k++) {
        for (i = 0; i < PR_U6F_PARTY_RECORD_SIZE; i++) rec[k][i] = (uint8_t)(0x30 + 7 * k + i);
        memcpy(orig[k], rec[k], PR_U6F_PARTY_RECORD_SIZE);
    }
}

int main(void)
{
    pr_u6f_party_plan plan;
    const uint8_t *recs[PR_U6F_PARTY_SLOTS];
    unsigned k, i;

    /* plan */
    make_save(6);
    CHECK(pr_u6f_party_plan_from_savgam(save, sizeof save, &plan) == PR_U6F_PARTY_OK && plan.count == 6);
    for (k = 0; k < 6; k++) {
        CHECK(plan.name_len[k] == 3 + k && plan.name_text[k] == save + PR_U6F_PARTY_NAMES_OFFSET + k * 41 + 1);
        CHECK(plan.name_text[k][0] == (uint8_t)('a' + k));
    }
    CHECK(plan.name_text[6] == NULL && plan.name_len[7] == 0);                /* unused slots are not examined */
    make_save(0);
    CHECK(pr_u6f_party_plan_from_savgam(save, sizeof save, &plan) == PR_U6F_PARTY_OK && plan.count == 0);
    make_save(8);
    CHECK(pr_u6f_party_plan_from_savgam(save, sizeof save, &plan) == PR_U6F_PARTY_OK && plan.count == 8 && plan.name_len[7] == 10);
    make_save(9);
    CHECK(pr_u6f_party_plan_from_savgam(save, sizeof save, &plan) == PR_U6F_PARTY_COUNT);
    make_save(3); save[PR_U6F_PARTY_NAMES_OFFSET + 41] = 41;                  /* slot 1 length past 40 */
    CHECK(pr_u6f_party_plan_from_savgam(save, sizeof save, &plan) == PR_U6F_PARTY_NAME && plan.count == 0);
    make_save(3); save[PR_U6F_PARTY_NAMES_OFFSET + 5 * 41] = 200;             /* a slot past the count is not examined */
    CHECK(pr_u6f_party_plan_from_savgam(save, sizeof save, &plan) == PR_U6F_PARTY_OK);
    CHECK(pr_u6f_party_plan_from_savgam(save, sizeof save - 1, &plan) == PR_U6F_PARTY_ARGUMENT);
    CHECK(pr_u6f_party_plan_from_savgam(NULL, sizeof save, &plan) == PR_U6F_PARTY_ARGUMENT && pr_u6f_party_plan_from_savgam(save, sizeof save, NULL) == PR_U6F_PARTY_ARGUMENT);

    /* build: order, links, fix-ups, source untouched */
    make_records();
    for (k = 0; k < PR_U6F_PARTY_SLOTS; k++) recs[k] = rec[k];
    CHECK(pr_u6f_party_build(recs, 6, &party) == PR_U6F_PARTY_OK && party.loaded == 6 && party.head == &party.node[0]);
    for (k = 0; k < 6; k++) {
        const uint8_t *r = party.record[k];
        uint8_t want[PR_U6F_PARTY_RECORD_SIZE];
        pr_u9b_record view;
        memcpy(want, orig[k], sizeof want);                                   /* the 0x9b +0x01AF step on a copy of the source */
        view.segment = want; view.segment_size = sizeof want; view.offset = 0;
        CHECK(pr_u9b_record_incr(&view, NULL) == PR_U9B_INCR_OK);
        CHECK(party.node[k].segment == party.record[k] && party.node[k].segment_size == PR_U6F_PARTY_RECORD_SIZE && party.node[k].offset == 0);
        CHECK(party.node[k].next == (k < 5 ? &party.node[k + 1] : NULL));
        CHECK(party.slot[k] == k && r[PR_U6F_PARTY_SLOT_BYTE] == k);
        for (i = 0; i < PR_U6F_PARTY_RECORD_SIZE; i++) {
            int cleared = (i >= 0x7F && i <= 0x82) || (i >= 0xC8 && i <= 0xCB) || (i >= 0x104 && i <= 0x10B);
            if (cleared) CHECK(r[i] == 0);
            else if (i != PR_U6F_PARTY_SLOT_BYTE) CHECK(r[i] == want[i]);
        }
        CHECK(memcmp(rec[k], orig[k], PR_U6F_PARTY_RECORD_SIZE) == 0);       /* the caller's bytes are not modified */
    }
    /* empty party and count 0 */
    CHECK(pr_u6f_party_build(recs, 0, &party) == PR_U6F_PARTY_OK && party.head == NULL && party.loaded == 0);
    /* absent files take no slot and are not in the list */
    recs[1] = NULL; recs[3] = NULL;
    CHECK(pr_u6f_party_build(recs, 5, &party) == PR_U6F_PARTY_OK && party.loaded == 3);
    CHECK(party.slot[0] == 0 && party.slot[1] == 1 && party.slot[2] == 2);
    CHECK(memcmp(party.record[0] + 1, orig[0] + 1, 0x7E) == 0 && memcmp(party.record[1] + 1, orig[2] + 1, 0x7E) == 0 && memcmp(party.record[2] + 1, orig[4] + 1, 0x7E) == 0);   /* before +0x7F: untouched by any step */
    CHECK(party.head == &party.node[0] && party.node[2].next == NULL);
    recs[0] = NULL; recs[1] = NULL; recs[2] = NULL;
    CHECK(pr_u6f_party_build(recs, 3, &party) == PR_U6F_PARTY_OK && party.head == NULL && party.loaded == 0);
    CHECK(pr_u6f_party_build(recs, 9, &party) == PR_U6F_PARTY_COUNT);
    CHECK(pr_u6f_party_build(NULL, 1, &party) == PR_U6F_PARTY_ARGUMENT && pr_u6f_party_build(recs, 1, NULL) == PR_U6F_PARTY_ARGUMENT);

    /* eight members use every slot; the count and the window-1 counter agree */
    make_records();
    for (k = 0; k < PR_U6F_PARTY_SLOTS; k++) recs[k] = rec[k];
    CHECK(pr_u6f_party_build(recs, 8, &party) == PR_U6F_PARTY_OK && party.loaded == 8 && party.slot[7] == 7);

    /* composition with PARTYSTRENGTH: independent per-member quotients summed in a byte */
    {
        static uint8_t srec[4][PR_U6F_PARTY_RECORD_SIZE];
        static uint8_t script[4] = { 0x1D, 0x01, 0x10, 0x97 };
        static pr_rec_ecl_loader_state st;
        const uint8_t *r4[PR_U6F_PARTY_SLOTS];
        int f[4][5] = { { 11, 59, 41, 0, 0 }, { 7, 56, 42, 1, 1 }, { 200, 90, 90, 9, 9 }, { 5, 53, 40, 1, 0 } };
        int expect = 0;
        pr_u1c_pstr_report rep;
        memset(srec, 0, sizeof srec);
        for (k = 0; k < 4; k++) {
            int b = f[k][1] > 0x3C ? f[k][1] - 0x3C : 0, c = f[k][2] > 0x27 ? f[k][2] - 0x27 : 0;
            srec[k][PR_U1C_PSTR_A] = (uint8_t)f[k][0]; srec[k][PR_U1C_PSTR_B] = (uint8_t)f[k][1]; srec[k][PR_U1C_PSTR_C] = (uint8_t)f[k][2];
            srec[k][PR_U1C_PSTR_D] = (uint8_t)f[k][3]; srec[k][PR_U1C_PSTR_E] = (uint8_t)f[k][4];
            expect += (f[k][0] + 5 * b + 5 * c + 8 * f[k][3] + 4 * f[k][4]) / 10;
            r4[k] = srec[k];
        }
        CHECK(pr_u6f_party_build(r4, 4, &party) == PR_U6F_PARTY_OK);
        memset(&st, 0, sizeof st); st.script = script; st.script_size = sizeof script; st.pc = 0x9900u; st.latch = 0x1D;
        CHECK(pr_u1c_op_party_strength(&st, party.head, &rep) == PR_U1C_PSTR_OK && rep.visited == 4);
        CHECK(rep.sum == (uint8_t)expect && st.window2[0x10] == (uint16_t)(uint8_t)expect);
    }
    puts("overlay_u6f_party_load: ok");
    return 0;
}
