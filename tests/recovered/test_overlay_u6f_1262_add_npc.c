#include "poolrad/recovered/overlay_u6f_1262_add_npc.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

/* Synthetic tables (the real DS bytes stay private): value = table tag + index. */
static uint8_t k_t0[32], k_t5[32], k_t6[32], k_to[32], k_tp[48];

typedef struct {
    char log[64];
    int fail_alloc, fail_init, fail_random, fail_roll, fail_insert, fail_build;
    int alloc_null; size_t alloc_size;
    uint8_t record[0x140];
    uint8_t init_id; uint8_t random_al;
    uint8_t roll_results[4]; uint8_t roll_counts[4]; uint8_t roll_ranges[4]; size_t roll_calls;
    uint8_t *inserted_record; uint8_t build_flag;
    uint8_t init_fill_class[7]; uint8_t init_race;      /* what the fake initializer writes into the record */
} fake_t;

static void put(fake_t *f, const char *s) { if (strlen(f->log) + strlen(s) < sizeof f->log) strcat(f->log, s); }
static int h_alloc(void *c, uint8_t **rec, size_t *size) { fake_t *f = c; put(f, "G"); *rec = f->alloc_null ? NULL : f->record; *size = f->alloc_size; return f->fail_alloc; }
static int h_init(void *c, uint8_t *rec, size_t size, uint8_t id) {
    fake_t *f = c; put(f, "I"); f->init_id = id; (void)size;
    memset(rec, 0x11, 0x11D); memcpy(rec + 0x96, f->init_fill_class, 7); rec[0x9E] = f->init_race; return f->fail_init;
}
static int h_random(void *c, uint16_t range, uint8_t *al) { fake_t *f = c; put(f, range == 0x100 ? "R" : "r"); *al = f->random_al; return f->fail_random; }
static int h_roll(void *c, uint8_t count, uint8_t range, uint8_t *al) {
    fake_t *f = c; put(f, "D");
    if (f->roll_calls < 4) { f->roll_counts[f->roll_calls] = count; f->roll_ranges[f->roll_calls] = range; *al = f->roll_results[f->roll_calls]; }
    f->roll_calls++; return f->fail_roll;
}
static int h_insert(void *c, uint8_t *rec) { fake_t *f = c; put(f, "P"); f->inserted_record = rec; return f->fail_insert; }
static int h_build(void *c, uint8_t flag) { fake_t *f = c; put(f, "B"); f->build_flag = flag; return f->fail_build; }

static void reset(fake_t *f, pr_u6f_1262_hooks *h, pr_u6f_1262_state *s) {
    memset(f, 0, sizeof *f); memset(h, 0, sizeof *h); memset(s, 0, sizeof *s); h->context = f;
    h->alloc_record = h_alloc; h->init_record = h_init; h->random_byte = h_random; h->roll = h_roll; h->insert_party = h_insert; h->build_pictures = h_build;
    f->alloc_size = 0x11D; f->random_al = 0x5A;
    f->roll_results[0] = 1; f->roll_results[1] = 3; f->roll_results[2] = 4;
    { static const uint8_t cls[7] = { 0, 0, 1, 0, 0, 0, 0 }; memcpy(f->init_fill_class, cls, 7); }   /* class slot 2 positive */
    f->init_race = 2;
    for (int i = 0; i < 32; ++i) { k_t0[i] = (uint8_t)(0x00 + i); k_t5[i] = (uint8_t)(0x50 + i); k_t6[i] = (uint8_t)(0x60 + i); k_to[i] = (uint8_t)(0xA0 + i); }
    for (int i = 0; i < 48; ++i) { k_tp[i] = (uint8_t)(0xC0 + i); }
    s->party_count = 3;
    s->class_table_0.bytes = k_t0; s->class_table_0.size = 32; s->class_table_5.bytes = k_t5; s->class_table_5.size = 32;
    s->class_table_6.bytes = k_t6; s->class_table_6.size = 32; s->class_table_other.bytes = k_to; s->class_table_other.size = 32;
    s->picture_table.bytes = k_tp; s->picture_table.size = 48;
}

static int test_full_path(void) {
    fake_t f; pr_u6f_1262_hooks h; pr_u6f_1262_state s; pr_u6f_1262_result r;
    reset(&f, &h, &s);
    CHECK(pr_u6f_1262_add_npc(0x2A, &s, &h, &r) == PR_U6F_1262_OK);
    CHECK(strcmp(f.log, "GIRDDDPB") == 0);
    CHECK(f.init_id == 0x2A && r.record == f.record && r.initialized == 1);
    CHECK(f.record[0xAB] == 0x5A && r.random_value == 0x5A);
    CHECK(r.class_count == 1 && r.list[1] == 2 && r.selected_class == 2);                /* the one positive slot, index 2 */
    CHECK(f.roll_counts[0] == 1 && f.roll_ranges[0] == 1);                               /* Roll(1, count) */
    CHECK(f.roll_counts[1] == 1 && f.roll_ranges[1] == 5 && r.class_roll == 3);
    CHECK(f.record[0xBC] == (uint8_t)(0xA0 + 2 * 5 + 3));                                /* "other" table, race 2 */
    CHECK(f.roll_counts[2] == 1 && f.roll_ranges[2] == 8 && r.picture_roll == 4);
    CHECK(f.record[0xBB] == (uint8_t)(0xC0 + 2 * 8 + 4));
    CHECK(f.inserted_record == f.record && f.build_flag == 1 && r.inserted == 1 && r.built == 1);
    CHECK(r.next_offset == PR_U6F_1262_END_OFFSET && f.roll_calls == 3);
    return 0;
}

static int test_class_selection(void) {
    fake_t f; pr_u6f_1262_hooks h; pr_u6f_1262_state s; pr_u6f_1262_result r;
    /* several positive slots: the list keeps their indices in order; a negative byte is not positive */
    reset(&f, &h, &s);
    { static const uint8_t cls[7] = { 5, 0, 0xFF, 1, 0, 0x7F, 0 }; memcpy(f.init_fill_class, cls, 7); }
    f.roll_results[0] = 3;                                       /* list[3] = slot 5 */
    CHECK(pr_u6f_1262_add_npc(1, &s, &h, &r) == PR_U6F_1262_OK);
    CHECK(r.class_count == 3 && r.list[1] == 0 && r.list[2] == 3 && r.list[3] == 5 && r.selected_class == 5);
    CHECK(f.roll_ranges[0] == 3 && f.record[0xBC] == (uint8_t)(0x50 + 2 * 5 + 3));      /* class 5 table */
    /* class 0 and class 6 tables */
    reset(&f, &h, &s); { static const uint8_t cls[7] = { 1, 0, 0, 0, 0, 0, 0 }; memcpy(f.init_fill_class, cls, 7); }
    CHECK(pr_u6f_1262_add_npc(1, &s, &h, &r) == PR_U6F_1262_OK && r.selected_class == 0 && f.record[0xBC] == (uint8_t)(0x00 + 2 * 5 + 3));
    reset(&f, &h, &s); { static const uint8_t cls[7] = { 0, 0, 0, 0, 0, 0, 1 }; memcpy(f.init_fill_class, cls, 7); }
    CHECK(pr_u6f_1262_add_npc(1, &s, &h, &r) == PR_U6F_1262_OK && r.selected_class == 6 && f.record[0xBC] == (uint8_t)(0x60 + 2 * 5 + 3));
    /* no positive slot: one entry, class 2, through the "other" table */
    reset(&f, &h, &s); memset(f.init_fill_class, 0, 7);
    CHECK(pr_u6f_1262_add_npc(1, &s, &h, &r) == PR_U6F_1262_OK && r.class_count == 1 && r.list[1] == 2 && r.selected_class == 2 && f.roll_ranges[0] == 1);
    CHECK(f.record[0xBC] == (uint8_t)(0xA0 + 2 * 5 + 3));
    /* race 0 and the largest roll values */
    reset(&f, &h, &s); f.init_race = 0; f.roll_results[1] = 5; f.roll_results[2] = 8;
    CHECK(pr_u6f_1262_add_npc(1, &s, &h, &r) == PR_U6F_1262_OK && f.record[0xBC] == (uint8_t)(0xA0 + 5) && f.record[0xBB] == (uint8_t)(0xC0 + 8));
    return 0;
}

static int test_gate_and_boundaries(void) {
    fake_t f; pr_u6f_1262_hooks h; pr_u6f_1262_state s; pr_u6f_1262_result r;
    /* party count above 7: nothing happens; 7 proceeds */
    reset(&f, &h, &s); s.party_count = 8;
    CHECK(pr_u6f_1262_add_npc(1, &s, &h, &r) == PR_U6F_1262_OK && r.skipped == 1 && strcmp(f.log, "") == 0 && r.next_offset == PR_U6F_1262_END_OFFSET);
    reset(&f, &h, &s); s.party_count = 7;
    CHECK(pr_u6f_1262_add_npc(1, &s, &h, &r) == PR_U6F_1262_OK && r.skipped == 0 && r.built == 1);
    /* each boundary without its callback, in reach order */
    reset(&f, &h, &s); h.alloc_record = NULL;
    CHECK(pr_u6f_1262_add_npc(1, &s, &h, &r) == PR_U6F_1262_NEED_HOOK && r.next_offset == 0x127B && strcmp(f.log, "") == 0);
    reset(&f, &h, &s); h.init_record = NULL;
    CHECK(pr_u6f_1262_add_npc(1, &s, &h, &r) == PR_U6F_1262_NEED_HOOK && r.next_offset == 0x1291 && r.record == f.record);
    reset(&f, &h, &s); h.random_byte = NULL;
    CHECK(pr_u6f_1262_add_npc(1, &s, &h, &r) == PR_U6F_1262_NEED_HOOK && r.next_offset == 0x12A1 && r.initialized == 1);
    reset(&f, &h, &s); h.roll = NULL;
    CHECK(pr_u6f_1262_add_npc(1, &s, &h, &r) == PR_U6F_1262_NEED_HOOK && r.next_offset == 0x12FE && r.class_count == 1 && f.record[0xAB] == 0x5A);
    reset(&f, &h, &s); h.insert_party = NULL;
    CHECK(pr_u6f_1262_add_npc(1, &s, &h, &r) == PR_U6F_1262_NEED_HOOK && r.next_offset == 0x1416 && f.record[0xBB] != 0x11);
    reset(&f, &h, &s); h.build_pictures = NULL;
    CHECK(pr_u6f_1262_add_npc(1, &s, &h, &r) == PR_U6F_1262_NEED_HOOK && r.next_offset == 0x141D && r.inserted == 1 && r.built == 0);
    /* host failures keep the earlier effects; the branch-specific roll offsets */
    reset(&f, &h, &s); f.fail_insert = 1;
    CHECK(pr_u6f_1262_add_npc(1, &s, &h, &r) == PR_U6F_1262_HOST_FAILURE && r.next_offset == 0x1416 && r.inserted == 0 && f.record[0xBC] != 0x11);
    reset(&f, &h, &s); f.fail_build = 1;
    CHECK(pr_u6f_1262_add_npc(1, &s, &h, &r) == PR_U6F_1262_HOST_FAILURE && r.inserted == 1 && r.built == 0 && strcmp(f.log, "GIRDDDPB") == 0);
    reset(&f, &h, &s); f.fail_init = 1;
    CHECK(pr_u6f_1262_add_npc(1, &s, &h, &r) == PR_U6F_1262_HOST_FAILURE && r.next_offset == 0x1291 && r.initialized == 0);
    reset(&f, &h, &s); f.fail_roll = 1;
    CHECK(pr_u6f_1262_add_npc(1, &s, &h, &r) == PR_U6F_1262_HOST_FAILURE && r.next_offset == 0x12FE);
    /* host contract: a NULL or short record, a roll outside 1..count, table indices outside the spans */
    reset(&f, &h, &s); f.alloc_null = 1;
    CHECK(pr_u6f_1262_add_npc(1, &s, &h, &r) == PR_U6F_1262_ARGUMENT && r.next_offset == 0x127B && strcmp(f.log, "G") == 0);
    reset(&f, &h, &s); f.alloc_size = 0x11C;
    CHECK(pr_u6f_1262_add_npc(1, &s, &h, &r) == PR_U6F_1262_ARGUMENT && r.next_offset == 0x127B);
    reset(&f, &h, &s); f.roll_results[0] = 0;
    CHECK(pr_u6f_1262_add_npc(1, &s, &h, &r) == PR_U6F_1262_ARGUMENT && r.next_offset == 0x12FE && r.class_count == 1);
    reset(&f, &h, &s); f.roll_results[0] = 2;                   /* count is 1 */
    CHECK(pr_u6f_1262_add_npc(1, &s, &h, &r) == PR_U6F_1262_ARGUMENT && r.next_offset == 0x12FE);
    reset(&f, &h, &s); f.init_race = 0xFF;                      /* race -1: index 5*(-1) + 3 is below the table */
    CHECK(pr_u6f_1262_add_npc(1, &s, &h, &r) == PR_U6F_1262_ARGUMENT && r.next_offset == 0x13B9 && f.record[0xBC] == 0x11 && f.roll_calls == 2);
    reset(&f, &h, &s); f.init_race = 7;                         /* 7*5 + 3 = 38 past a 32-byte table */
    CHECK(pr_u6f_1262_add_npc(1, &s, &h, &r) == PR_U6F_1262_ARGUMENT && r.next_offset == 0x13B9);
    reset(&f, &h, &s); s.picture_table.size = 20;               /* 2*8 + 4 = 20 past the span; the class byte already written */
    CHECK(pr_u6f_1262_add_npc(1, &s, &h, &r) == PR_U6F_1262_ARGUMENT && r.next_offset == 0x13E9 && f.record[0xBC] == (uint8_t)(0xA0 + 13) && f.record[0xBB] == 0x11);
    /* arguments */
    reset(&f, &h, &s);
    CHECK(pr_u6f_1262_add_npc(1, NULL, &h, &r) == PR_U6F_1262_ARGUMENT);
    CHECK(pr_u6f_1262_add_npc(1, &s, NULL, &r) == PR_U6F_1262_ARGUMENT);
    CHECK(pr_u6f_1262_add_npc(1, &s, &h, NULL) == PR_U6F_1262_ARGUMENT);
    CHECK(strcmp(f.log, "") == 0);
    return 0;
}

int main(void) {
    int rc = 0;
    rc |= test_full_path();
    rc |= test_class_selection();
    rc |= test_gate_and_boundaries();
    if (rc == 0) {
        puts("overlay U6F +1262 add-npc tests passed");
    }
    return rc;
}
