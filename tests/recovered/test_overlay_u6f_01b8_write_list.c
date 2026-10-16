#include "poolrad/recovered/overlay_u6f_01b8_write_list.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

/* Synthetic literals of the originals' lengths (the real constants stay private). */
static const char k_name_s[] = "NAME.TXT    ";          /* 12 characters */
static const char k_prefix_s[] = "PREFIX-SEVENTEEN-";    /* 17 characters */
static const char k_suffix_s[] = ":";                    /* 1 character */
#define k_name ((const uint8_t *)k_name_s)
#define k_prefix ((const uint8_t *)k_prefix_s)
#define k_suffix ((const uint8_t *)k_suffix_s)

typedef struct {
    char log[256];
    int fail_ensure, fail_assign, fail_rewrite, fail_io, fail_display, fail_write, fail_close;
    uint16_t io_sequence[8]; size_t io_calls;
    uint8_t last_path[256]; uint8_t last_message[256]; uint8_t last_color;
    uint8_t lines[8][256]; size_t line_count;
} fake_t;

static void put(fake_t *f, const char *s) { if (strlen(f->log) + strlen(s) < sizeof f->log) strcat(f->log, s); }
static int h_ensure(void *c) { fake_t *f = c; put(f, "E"); return f->fail_ensure; }
static int h_assign(void *c, const uint8_t *p, size_t n) { fake_t *f = c; put(f, "A"); memcpy(f->last_path, p, n); return f->fail_assign; }
static int h_rewrite(void *c) { fake_t *f = c; put(f, "R"); return f->fail_rewrite; }
static int h_io(void *c, uint16_t *r) { fake_t *f = c; put(f, "I"); *r = f->io_sequence[f->io_calls < 8 ? f->io_calls : 7]; f->io_calls++; return f->fail_io; }
static int h_display(void *c, const uint8_t *m, size_t n, uint8_t color) { fake_t *f = c; put(f, "D"); memcpy(f->last_message, m, n); f->last_color = color; return f->fail_display; }
static int h_write(void *c, const uint8_t *s, size_t n) { fake_t *f = c; put(f, "W"); if (f->line_count < 8) memcpy(f->lines[f->line_count++], s, n); return f->fail_write; }
static int h_close(void *c) { fake_t *f = c; put(f, "C"); return f->fail_close; }

static void reset(fake_t *f, pr_u6f_01b8_hooks *h) {
    memset(f, 0, sizeof *f); memset(h, 0, sizeof *h); h->context = f;
    h->ensure_directory = h_ensure; h->text_assign = h_assign; h->text_rewrite = h_rewrite; h->io_result = h_io;
    h->display_and_input = h_display; h->write_line = h_write; h->text_close = h_close;
}

static int pascal_equals(const uint8_t *p, const char *s) { size_t n = strlen(s); return p[0] == n && memcmp(p + 1, s, n) == 0; }

static int test_success_path(void) {
    fake_t f; pr_u6f_01b8_hooks h; pr_u6f_01b8_result r;
    static const uint8_t live[] = { 3, 'C', ':', '\\' };
    static const uint8_t n1[] = { 5, 'A', 'L', 'P', 'H', 'A', 0, 0 };
    static const uint8_t n2[] = { 4, '*', 'X', 'Y', 'Z', 0, 0 };           /* first character 0x2A: skipped */
    static const uint8_t n3[] = { 0, 'Q' };                                 /* empty name: the byte at +1 is still read */
    pr_u6f_01b8_node c = { n3, sizeof n3, NULL }, b = { n2, sizeof n2, &c }, a = { n1, sizeof n1, &b };
    reset(&f, &h); f.io_sequence[0] = 0;
    CHECK(pr_u6f_01b8_write_list(&a, live, sizeof live, k_name, 12, k_prefix, 17, k_suffix, 1, &h, 4, 16, &r) == PR_U6F_01B8_OK);
    CHECK(strcmp(f.log, "EARIWWC") == 0);
    CHECK(pascal_equals(f.last_path, "C:\\NAME.TXT    "));
    CHECK(r.open_attempts == 1 && r.displays == 0 && r.nodes_visited == 3 && r.lines_written == 2 && r.nodes_skipped == 1 && r.closed == 1);
    CHECK(r.last_io_result == 0 && r.next_offset == PR_U6F_01B8_END_OFFSET);
    CHECK(f.line_count == 2 && pascal_equals(f.lines[0], "ALPHA") && f.lines[1][0] == 0);
    return 0;
}

static int test_nil_list_closes(void) {
    fake_t f; pr_u6f_01b8_hooks h; pr_u6f_01b8_result r;
    static const uint8_t live[] = { 1, 'D' };
    reset(&f, &h);
    CHECK(pr_u6f_01b8_write_list(NULL, live, sizeof live, k_name, 12, k_prefix, 17, k_suffix, 1, &h, 4, 16, &r) == PR_U6F_01B8_OK);
    CHECK(strcmp(f.log, "EARIC") == 0 && r.nodes_visited == 0 && r.closed == 1);
    return 0;
}

static int test_open_retry_and_message(void) {
    fake_t f; pr_u6f_01b8_hooks h; pr_u6f_01b8_result r;
    static const uint8_t live[] = { 2, 'B', ':' };
    reset(&f, &h); f.io_sequence[0] = 0x66; f.io_sequence[1] = 0x66; f.io_sequence[2] = 0;
    CHECK(pr_u6f_01b8_write_list(NULL, live, sizeof live, k_name, 12, k_prefix, 17, k_suffix, 1, &h, 8, 16, &r) == PR_U6F_01B8_OK);
    CHECK(strcmp(f.log, "EARIDARIDARIC") == 0);                 /* two failures, two prompts, then success */
    CHECK(r.open_attempts == 3 && r.displays == 2 && r.last_io_result == 0);
    CHECK(f.last_color == 0x0E && pascal_equals(f.last_message, "PREFIX-SEVENTEEN-B:"));   /* Chr(DS:0x537B) is the path's first character */
    /* the retry loop is unbounded in the engine; the host guard reports it */
    reset(&f, &h); f.io_sequence[0] = f.io_sequence[1] = f.io_sequence[2] = f.io_sequence[7] = 0x66;
    CHECK(pr_u6f_01b8_write_list(NULL, live, sizeof live, k_name, 12, k_prefix, 17, k_suffix, 1, &h, 3, 16, &r) == PR_U6F_01B8_RETRY_LIMIT);
    CHECK(r.open_attempts == 3 && r.displays == 3 && r.closed == 0 && r.next_offset == 0x01C3);
    return 0;
}

static int test_boundaries_and_failures(void) {
    fake_t f; pr_u6f_01b8_hooks h; pr_u6f_01b8_result r;
    static const uint8_t live[] = { 1, 'A' };
    static const uint8_t n1[] = { 1, 'Z' };
    pr_u6f_01b8_node a = { n1, sizeof n1, NULL };
    /* every boundary without its callback, in reach order */
    reset(&f, &h); h.ensure_directory = NULL;
    CHECK(pr_u6f_01b8_write_list(&a, live, sizeof live, k_name, 12, k_prefix, 17, k_suffix, 1, &h, 4, 16, &r) == PR_U6F_01B8_NEED_HOOK && r.next_offset == 0x01BF);
    reset(&f, &h); h.text_assign = NULL;
    CHECK(pr_u6f_01b8_write_list(&a, live, sizeof live, k_name, 12, k_prefix, 17, k_suffix, 1, &h, 4, 16, &r) == PR_U6F_01B8_NEED_HOOK && r.next_offset == 0x01E3 && strcmp(f.log, "E") == 0);
    reset(&f, &h); h.text_rewrite = NULL;
    CHECK(pr_u6f_01b8_write_list(&a, live, sizeof live, k_name, 12, k_prefix, 17, k_suffix, 1, &h, 4, 16, &r) == PR_U6F_01B8_NEED_HOOK && r.next_offset == 0x01EE);
    reset(&f, &h); h.io_result = NULL;
    CHECK(pr_u6f_01b8_write_list(&a, live, sizeof live, k_name, 12, k_prefix, 17, k_suffix, 1, &h, 4, 16, &r) == PR_U6F_01B8_NEED_HOOK && r.next_offset == 0x01F3);
    reset(&f, &h); h.display_and_input = NULL; f.io_sequence[0] = 1;
    CHECK(pr_u6f_01b8_write_list(&a, live, sizeof live, k_name, 12, k_prefix, 17, k_suffix, 1, &h, 4, 16, &r) == PR_U6F_01B8_NEED_HOOK && r.next_offset == 0x0234);
    reset(&f, &h); h.write_line = NULL;
    CHECK(pr_u6f_01b8_write_list(&a, live, sizeof live, k_name, 12, k_prefix, 17, k_suffix, 1, &h, 4, 16, &r) == PR_U6F_01B8_NEED_HOOK && r.next_offset == 0x025E);
    reset(&f, &h); h.text_close = NULL;
    CHECK(pr_u6f_01b8_write_list(&a, live, sizeof live, k_name, 12, k_prefix, 17, k_suffix, 1, &h, 4, 16, &r) == PR_U6F_01B8_NEED_HOOK && r.next_offset == 0x0289 && r.lines_written == 1);
    /* host failures keep the earlier effects */
    reset(&f, &h); f.fail_close = 1;
    CHECK(pr_u6f_01b8_write_list(&a, live, sizeof live, k_name, 12, k_prefix, 17, k_suffix, 1, &h, 4, 16, &r) == PR_U6F_01B8_HOST_FAILURE && r.lines_written == 1 && r.closed == 0);
    reset(&f, &h); f.fail_ensure = 1;
    CHECK(pr_u6f_01b8_write_list(&a, live, sizeof live, k_name, 12, k_prefix, 17, k_suffix, 1, &h, 4, 16, &r) == PR_U6F_01B8_HOST_FAILURE && strcmp(f.log, "E") == 0);
    /* a cycle: the engine never terminates; the host guard reports it */
    { pr_u6f_01b8_node x = { n1, sizeof n1, NULL }; x.next = &x; reset(&f, &h);
      CHECK(pr_u6f_01b8_write_list(&x, live, sizeof live, k_name, 12, k_prefix, 17, k_suffix, 1, &h, 4, 5, &r) == PR_U6F_01B8_NODE_LIMIT && r.nodes_visited == 5 && r.lines_written == 5 && r.closed == 0); }
    /* path overflow: a 250-character live path plus the 12-character name exceeds 255 */
    { static uint8_t longp[251]; longp[0] = 250; memset(longp + 1, 'P', 250); reset(&f, &h);
      CHECK(pr_u6f_01b8_write_list(&a, longp, sizeof longp, k_name, 12, k_prefix, 17, k_suffix, 1, &h, 4, 16, &r) == PR_U6F_01B8_BOUNDS && strcmp(f.log, "E") == 0); }
    /* arguments: wrong literal lengths, missing live string, null output */
    reset(&f, &h);
    CHECK(pr_u6f_01b8_write_list(&a, live, sizeof live, k_name, 11, k_prefix, 17, k_suffix, 1, &h, 4, 16, &r) == PR_U6F_01B8_ARGUMENT);
    CHECK(pr_u6f_01b8_write_list(&a, live, sizeof live, k_name, 12, k_prefix, 16, k_suffix, 1, &h, 4, 16, &r) == PR_U6F_01B8_ARGUMENT);
    CHECK(pr_u6f_01b8_write_list(&a, live, 1, k_name, 12, k_prefix, 17, k_suffix, 1, &h, 4, 16, &r) == PR_U6F_01B8_ARGUMENT);
    /* host-span corrections (PM review i8476) */
    { static const uint8_t empty1[] = { 0 };                     /* length 0 with no byte after it: the message path would read DS:0x537B */
      reset(&f, &h);
      CHECK(pr_u6f_01b8_write_list(&a, empty1, sizeof empty1, k_name, 12, k_prefix, 17, k_suffix, 1, &h, 4, 16, &r) == PR_U6F_01B8_ARGUMENT && strcmp(f.log, "") == 0); }
    { static const uint8_t empty2[] = { 0, 'Q' };                 /* length 0 with the raw next byte supplied: accepted; the message carries it */
      reset(&f, &h); f.io_sequence[0] = 1; f.io_sequence[1] = 0;
      CHECK(pr_u6f_01b8_write_list(&a, empty2, sizeof empty2, k_name, 12, k_prefix, 17, k_suffix, 1, &h, 4, 16, &r) == PR_U6F_01B8_OK && r.displays == 1);
      CHECK(pascal_equals(f.last_message, "PREFIX-SEVENTEEN-Q:") && pascal_equals(f.last_path, "NAME.TXT    ")); }
    { static const uint8_t one[] = { 0 }; pr_u6f_01b8_node short_node = { one, 1, NULL };   /* no byte 1 to read */
      reset(&f, &h);
      CHECK(pr_u6f_01b8_write_list(&short_node, live, sizeof live, k_name, 12, k_prefix, 17, k_suffix, 1, &h, 4, 16, &r) == PR_U6F_01B8_ARGUMENT && r.nodes_visited == 1 && r.lines_written == 0 && strcmp(f.log, "EARI") == 0); }
    { static const uint8_t claims5[] = { 5, 'A', 'B' }; pr_u6f_01b8_node lying = { claims5, sizeof claims5, NULL };   /* length byte past the span */
      reset(&f, &h);
      CHECK(pr_u6f_01b8_write_list(&lying, live, sizeof live, k_name, 12, k_prefix, 17, k_suffix, 1, &h, 4, 16, &r) == PR_U6F_01B8_ARGUMENT && r.nodes_visited == 1 && r.lines_written == 0 && r.next_offset == 0x025E); }
    { static const uint8_t good[] = { 1, 'G' }; static const uint8_t zero_q[] = { 0, 'Q' }; static const uint8_t zero_star[] = { 0, '*' };
      pr_u6f_01b8_node third = { zero_star, sizeof zero_star, NULL }, second = { zero_q, sizeof zero_q, &third }, first = { good, sizeof good, &second };
      reset(&f, &h);                                              /* zero-length names: byte 1 decides, the write forwards one byte */
      CHECK(pr_u6f_01b8_write_list(&first, live, sizeof live, k_name, 12, k_prefix, 17, k_suffix, 1, &h, 4, 16, &r) == PR_U6F_01B8_OK);
      CHECK(r.nodes_visited == 3 && r.lines_written == 2 && r.nodes_skipped == 1 && f.line_count == 2 && f.lines[1][0] == 0); }
    { static const uint8_t bad_first[] = { 1, 'Z' }; static const uint8_t short2[] = { 0 };
      pr_u6f_01b8_node second = { short2, 1, NULL }, first = { bad_first, sizeof bad_first, &second };
      reset(&f, &h);                                              /* the earlier node's effects stand when a later node's span fails */
      CHECK(pr_u6f_01b8_write_list(&first, live, sizeof live, k_name, 12, k_prefix, 17, k_suffix, 1, &h, 4, 16, &r) == PR_U6F_01B8_ARGUMENT && r.lines_written == 1 && r.nodes_visited == 2 && r.closed == 0); }
    CHECK(pr_u6f_01b8_write_list(&a, live, sizeof live, k_name, 12, k_prefix, 17, k_suffix, 1, NULL, 4, 16, &r) == PR_U6F_01B8_ARGUMENT);
    CHECK(pr_u6f_01b8_write_list(&a, live, sizeof live, k_name, 12, k_prefix, 17, k_suffix, 1, &h, 4, 16, NULL) == PR_U6F_01B8_ARGUMENT);
    { pr_u6f_01b8_node bad = { NULL, 2, NULL }; reset(&f, &h);
      CHECK(pr_u6f_01b8_write_list(&bad, live, sizeof live, k_name, 12, k_prefix, 17, k_suffix, 1, &h, 4, 16, &r) == PR_U6F_01B8_ARGUMENT && r.nodes_visited == 1); }
    return 0;
}

int main(void) {
    int rc = 0;
    rc |= test_success_path();
    rc |= test_nil_list_closes();
    rc |= test_open_retry_and_message();
    rc |= test_boundaries_and_failures();
    if (rc == 0) {
        puts("overlay U6F +01B8 write-list tests passed");
    }
    return rc;
}
