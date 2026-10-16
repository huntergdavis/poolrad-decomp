#include "poolrad/recovered/overlay_u6f_22d5_transition_prompt.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

/* A synthetic literal of the original's length (the real constant stays private). */
static const char k_message_s[] = "TWENTY-EIGHT-CHARACTER-TEXT#";   /* 28 characters */
#define k_message ((const uint8_t *)k_message_s)

typedef struct {
    char log[64];
    int fail_select, fail_key, fail_restore, fail_save, fail_exit;
    uint8_t select_al, key_al, key_extended;
    uint8_t select_name[64]; uint8_t title[8]; uint8_t menu[64];
    uint8_t row, a, b, c, d;
} fake_t;

static void put(fake_t *f, const char *s) { if (strlen(f->log) + strlen(s) < sizeof f->log) strcat(f->log, s); }
static int h_select(void *c, const uint8_t *n, size_t span, uint8_t *al) { fake_t *f = c; put(f, "P"); memcpy(f->select_name, n, span < 64 ? span : 64); *al = f->select_al; return f->fail_select; }
static int h_key(void *c, const uint8_t *t, size_t ts, const uint8_t *m, size_t ms, uint8_t row, uint8_t a, uint8_t b, uint8_t cc, uint8_t d, uint8_t *ext, uint8_t *al) {
    fake_t *f = c; put(f, "K"); memcpy(f->title, t, ts < 8 ? ts : 8); memcpy(f->menu, m, ms < 64 ? ms : 64);
    f->row = row; f->a = a; f->b = b; f->c = cc; f->d = d; *ext = f->key_extended; *al = f->key_al; return f->fail_key;
}
static int h_restore(void *c) { fake_t *f = c; put(f, "R"); return f->fail_restore; }
static int h_save(void *c) { fake_t *f = c; put(f, "S"); return f->fail_save; }
static int h_exit(void *c) { fake_t *f = c; put(f, "X"); return f->fail_exit; }

static const uint8_t k_dx[9] = { 0, 1, 1, 1, 0, 0xFF, 0xFF, 0xFF, 0 };   /* synthetic facing tables */
static const uint8_t k_dy[9] = { 0xFF, 0xFF, 0, 1, 1, 1, 0, 0xFF, 0 };

static void reset(fake_t *f, pr_u6f_22d5_hooks *h, pr_u6f_22d5_state *s) {
    memset(f, 0, sizeof *f); memset(h, 0, sizeof *h); memset(s, 0, sizeof *s); h->context = f;
    h->select_file = h_select; h->key_input = h_key; h->restore_archive_number = h_restore; h->save = h_save; h->exit_game = h_exit;
    f->key_al = 0x53; s->archive_number = 7; s->x = 5; s->y = 9; s->facing = 1;
    s->dx_table = k_dx; s->dx_table_size = sizeof k_dx; s->dy_table = k_dy; s->dy_table_size = sizeof k_dy;
}

static int pascal_equals(const uint8_t *p, const char *s) { size_t n = strlen(s); return p[0] == n && memcmp(p + 1, s, n) == 0; }

static const uint8_t k_name[] = { 8, 'E', 'C', 'L', '9', '.', 'D', 'A', 'X' };
#define CALL(hp, sp, rp) pr_u6f_22d5_transition_prompt(k_name, sizeof k_name, k_message, 28, (sp), (hp), (rp))

static int test_file_present(void) {
    fake_t f; pr_u6f_22d5_hooks h; pr_u6f_22d5_state s; pr_u6f_22d5_result r;
    reset(&f, &h, &s); f.select_al = 1;
    CHECK(CALL(&h, &s, &r) == PR_U6F_22D5_OK);
    CHECK(strcmp(f.log, "P") == 0 && r.selected == 1 && r.prompted == 0 && r.moved == 0 && r.next_offset == PR_U6F_22D5_END_OFFSET);
    CHECK(pascal_equals(f.select_name, "ECL9.DAX") && s.x == 5 && s.y == 9);
    return 0;
}

static int test_prompt_and_save(void) {
    fake_t f; pr_u6f_22d5_hooks h; pr_u6f_22d5_state s; pr_u6f_22d5_result r;
    reset(&f, &h, &s);                                           /* absent file, key 0x53, facing 1: dx +1, dy -1 */
    CHECK(CALL(&h, &s, &r) == PR_U6F_22D5_OK);
    CHECK(strcmp(f.log, "PKRSX") == 0);
    CHECK(r.selected == 0 && r.prompted == 1 && r.key == 0x53 && r.extended == 0 && r.moved == 1 && r.restored == 1 && r.saved == 1 && r.exited == 1);
    CHECK(r.next_offset == PR_U6F_22D5_END_OFFSET);
    CHECK(f.title[0] == 0 && pascal_equals(f.menu, "TWENTY-EIGHT-CHARACTER-TEXT#7"));   /* 28 + the digit */
    CHECK(f.row == 0x0D && f.a == 0x0A && f.b == 0x0F && f.c == 0 && f.d == 0);
    CHECK(s.x == 4 && s.y == 10);                                /* x - 1, y - (-1) */
    /* the digit is the first decimal digit of the byte */
    reset(&f, &h, &s); s.archive_number = 123; CHECK(CALL(&h, &s, &r) == PR_U6F_22D5_OK && f.menu[29] == '1');
    reset(&f, &h, &s); s.archive_number = 0;   CHECK(CALL(&h, &s, &r) == PR_U6F_22D5_OK && f.menu[29] == '0');
    reset(&f, &h, &s); s.archive_number = 10;  CHECK(CALL(&h, &s, &r) == PR_U6F_22D5_OK && f.menu[29] == '1');
    reset(&f, &h, &s); s.archive_number = 255; CHECK(CALL(&h, &s, &r) == PR_U6F_22D5_OK && f.menu[29] == '2' && f.menu[0] == 29);
    return 0;
}

static int test_other_keys(void) {
    fake_t f; pr_u6f_22d5_hooks h; pr_u6f_22d5_state s; pr_u6f_22d5_result r;
    reset(&f, &h, &s); f.key_al = 0x49;                          /* another letter: nothing more */
    CHECK(CALL(&h, &s, &r) == PR_U6F_22D5_OK && strcmp(f.log, "PK") == 0 && r.key == 0x49 && r.moved == 0 && s.x == 5 && r.next_offset == PR_U6F_22D5_END_OFFSET);
    reset(&f, &h, &s); f.key_extended = 1;                       /* extended key with AL still 0x53: the extended test comes first */
    CHECK(CALL(&h, &s, &r) == PR_U6F_22D5_OK && strcmp(f.log, "PK") == 0 && r.extended == 1 && r.moved == 0);
    reset(&f, &h, &s); f.key_al = 0x73;                          /* lower-case s is not 0x53 here (the callee upcases, the caller does not) */
    CHECK(CALL(&h, &s, &r) == PR_U6F_22D5_OK && r.moved == 0);
    return 0;
}

static int test_wrap(void) {
    fake_t f; pr_u6f_22d5_hooks h; pr_u6f_22d5_state s; pr_u6f_22d5_result r;
    reset(&f, &h, &s); s.facing = 1; s.x = 0; s.y = 0x0F;        /* x - 1 = 0xFF (negative) -> 0x0F; y + 1 = 0x10 (above 15) -> 0 */
    CHECK(CALL(&h, &s, &r) == PR_U6F_22D5_OK && s.x == 0x0F && s.y == 0);
    reset(&f, &h, &s); s.facing = 5; s.x = 0x7F; s.y = 0x80;     /* x - (-1) = 0x80, negative -> 0x0F; y - 1 = 0x7F, above 15 -> 0 */
    CHECK(CALL(&h, &s, &r) == PR_U6F_22D5_OK && s.x == 0x0F && s.y == 0);
    reset(&f, &h, &s); s.facing = 0; s.x = 0x10; s.y = 0x7F;     /* dx 0: 0x10 stays above 15 -> 0; dy -1: 0x80 -> 0x0F */
    CHECK(CALL(&h, &s, &r) == PR_U6F_22D5_OK && s.x == 0 && s.y == 0x0F);
    reset(&f, &h, &s); s.facing = 8; s.x = 0x0F; s.y = 0x0F;     /* zero deltas keep 15 */
    CHECK(CALL(&h, &s, &r) == PR_U6F_22D5_OK && s.x == 0x0F && s.y == 0x0F);
    return 0;
}

static int test_boundaries_and_arguments(void) {
    fake_t f; pr_u6f_22d5_hooks h; pr_u6f_22d5_state s; pr_u6f_22d5_result r;
    /* each boundary without its callback, in reach order */
    reset(&f, &h, &s); h.select_file = NULL;
    CHECK(CALL(&h, &s, &r) == PR_U6F_22D5_NEED_HOOK && r.next_offset == 0x22F4 && strcmp(f.log, "") == 0);
    reset(&f, &h, &s); h.key_input = NULL;
    CHECK(CALL(&h, &s, &r) == PR_U6F_22D5_NEED_HOOK && r.next_offset == 0x236A && strcmp(f.log, "P") == 0 && r.prompted == 0);
    reset(&f, &h, &s); h.restore_archive_number = NULL;
    CHECK(CALL(&h, &s, &r) == PR_U6F_22D5_NEED_HOOK && r.next_offset == 0x23CC && r.moved == 1 && s.x == 4);
    reset(&f, &h, &s); h.save = NULL;
    CHECK(CALL(&h, &s, &r) == PR_U6F_22D5_NEED_HOOK && r.next_offset == 0x23D2 && r.restored == 1 && r.saved == 0);
    reset(&f, &h, &s); h.exit_game = NULL;
    CHECK(CALL(&h, &s, &r) == PR_U6F_22D5_NEED_HOOK && r.next_offset == 0x23D5 && r.saved == 1 && r.exited == 0);
    /* host failures keep the earlier effects */
    reset(&f, &h, &s); f.fail_key = 1;
    CHECK(CALL(&h, &s, &r) == PR_U6F_22D5_HOST_FAILURE && r.next_offset == 0x236A && r.prompted == 1 && r.moved == 0);
    reset(&f, &h, &s); f.fail_save = 1;
    CHECK(CALL(&h, &s, &r) == PR_U6F_22D5_HOST_FAILURE && r.next_offset == 0x23D2 && r.moved == 1 && r.restored == 1 && r.saved == 0 && s.x == 4 && s.y == 10);
    reset(&f, &h, &s); f.fail_exit = 1;
    CHECK(CALL(&h, &s, &r) == PR_U6F_22D5_HOST_FAILURE && r.exited == 0 && r.saved == 1 && strcmp(f.log, "PKRSX") == 0);
    reset(&f, &h, &s); f.fail_select = 1;
    CHECK(CALL(&h, &s, &r) == PR_U6F_22D5_HOST_FAILURE && r.next_offset == 0x22F4);
    /* the name copy keeps at most 40 characters */
    { static uint8_t longn[61]; longn[0] = 60; memset(longn + 1, 'N', 60); reset(&f, &h, &s); f.select_al = 1;
      CHECK(pr_u6f_22d5_transition_prompt(longn, sizeof longn, k_message, 28, &s, &h, &r) == PR_U6F_22D5_OK && f.select_name[0] == 40); }
    /* arguments: name span, literal length, nulls, table span at the facing (after the prompt, before any move) */
    reset(&f, &h, &s);
    CHECK(pr_u6f_22d5_transition_prompt(k_name, 8, k_message, 28, &s, &h, &r) == PR_U6F_22D5_ARGUMENT);   /* length 8 needs 9 bytes */
    CHECK(pr_u6f_22d5_transition_prompt(k_name, sizeof k_name, k_message, 27, &s, &h, &r) == PR_U6F_22D5_ARGUMENT);
    CHECK(pr_u6f_22d5_transition_prompt(k_name, sizeof k_name, k_message, 28, NULL, &h, &r) == PR_U6F_22D5_ARGUMENT);
    CHECK(pr_u6f_22d5_transition_prompt(k_name, sizeof k_name, k_message, 28, &s, NULL, &r) == PR_U6F_22D5_ARGUMENT);
    CHECK(pr_u6f_22d5_transition_prompt(k_name, sizeof k_name, k_message, 28, &s, &h, NULL) == PR_U6F_22D5_ARGUMENT);
    CHECK(strcmp(f.log, "") == 0);
    reset(&f, &h, &s); s.facing = 9;                             /* past both 9-byte tables */
    CHECK(CALL(&h, &s, &r) == PR_U6F_22D5_ARGUMENT && r.next_offset == 0x237E && r.prompted == 1 && r.moved == 0 && s.x == 5 && strcmp(f.log, "PK") == 0);
    reset(&f, &h, &s); s.dy_table_size = 1;                      /* facing 1 needs 2 bytes */
    CHECK(CALL(&h, &s, &r) == PR_U6F_22D5_ARGUMENT && r.moved == 0 && s.x == 5);
    reset(&f, &h, &s); s.facing = 9; f.select_al = 1;            /* the table is not read on the early paths */
    CHECK(CALL(&h, &s, &r) == PR_U6F_22D5_OK);
    reset(&f, &h, &s); s.facing = 9; f.key_al = 0x41;
    CHECK(CALL(&h, &s, &r) == PR_U6F_22D5_OK);
    return 0;
}

int main(void) {
    int rc = 0;
    rc |= test_file_present();
    rc |= test_prompt_and_save();
    rc |= test_other_keys();
    rc |= test_wrap();
    rc |= test_boundaries_and_arguments();
    if (rc == 0) {
        puts("overlay U6F +22D5 transition-prompt tests passed");
    }
    return rc;
}
