#include "poolrad/recovered/overlay_u6f_0010_read_list.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

/* Synthetic literals of the originals' lengths (the real constants stay private). */
static const char k_name_s[] = "NAME.TXT    ";          /* 12 characters */
static const char k_marker_s[] = "#-";                   /* 2 characters */
#define k_name ((const uint8_t *)k_name_s)
#define k_marker ((const uint8_t *)k_marker_s)

typedef struct {
    char log[256];
    int fail_ensure, fail_select, fail_assign, fail_reset, fail_eof, fail_alloc, fail_read, fail_close;
    uint8_t select_al;                          /* the selector's Boolean */
    const char *lines[8]; size_t line_count;    /* the file's lines; eof after the last */
    size_t line_index;
    int never_eof;                              /* a file that never ends (the read loop guard) */
    size_t over_read;                           /* a read service claiming more than max_count */
    int alloc_null;                             /* alloc returning 0 with no node */
    pr_u6f_0010_node pool[8]; size_t allocated;
    pr_u6f_0010_node **head_ref; pr_u6f_0010_node spare;        /* a hostile callback moves *head_ref to &spare (or NULL) */
    int move_head_in_ensure, move_head_after_read, move_head_to_null_after_read;
    uint8_t select_path[256]; uint8_t assign_path[256];
} fake_t;

static void put(fake_t *f, const char *s) { if (strlen(f->log) + strlen(s) < sizeof f->log) strcat(f->log, s); }
static int h_ensure(void *c) { fake_t *f = c; put(f, "E"); if (f->move_head_in_ensure) *f->head_ref = &f->spare; return f->fail_ensure; }
static int h_select(void *c, const uint8_t *p, size_t n, uint8_t *al) { fake_t *f = c; put(f, "S"); memcpy(f->select_path, p, n); *al = f->select_al; return f->fail_select; }
static int h_assign(void *c, const uint8_t *p, size_t n) { fake_t *f = c; put(f, "A"); memcpy(f->assign_path, p, n); return f->fail_assign; }
static int h_reset(void *c) { fake_t *f = c; put(f, "R"); return f->fail_reset; }
static int h_eof(void *c, uint8_t *al) { fake_t *f = c; put(f, "F"); *al = (f->never_eof || f->line_index < f->line_count) ? 0u : 1u; return f->fail_eof; }
static int h_alloc(void *c, pr_u6f_0010_node **node) {
    fake_t *f = c; put(f, "G");
    if (f->alloc_null) { *node = NULL; return 0; }
    if (f->allocated >= 8) return 1;
    memset(&f->pool[f->allocated], 0xCC, sizeof f->pool[0]);   /* uninitialized heap: the model must clear it */
    *node = &f->pool[f->allocated++];
    return f->fail_alloc;
}
static int h_read(void *c, uint8_t *chars, size_t max_count, size_t *count) {
    fake_t *f = c; put(f, "L");
    if (f->over_read) { *count = f->over_read; return 0; }
    {
        const char *line = f->line_index < f->line_count ? f->lines[f->line_index] : "";
        size_t n = strlen(line);
        if (n > max_count) n = max_count;                        /* 0x08E1 stops at the count */
        memcpy(chars, line, n);
        *count = n;
    }
    f->line_index++;
    if (f->move_head_after_read && f->line_index == 1) *f->head_ref = &f->spare;
    if (f->move_head_to_null_after_read && f->line_index == 1) *f->head_ref = NULL;
    return f->fail_read;
}
static int h_close(void *c) { fake_t *f = c; put(f, "C"); return f->fail_close; }

static void reset(fake_t *f, pr_u6f_0010_hooks *h) {
    memset(f, 0, sizeof *f); memset(h, 0, sizeof *h); h->context = f; f->select_al = 1;
    h->ensure_directory = h_ensure; h->select_file = h_select; h->text_assign = h_assign; h->text_reset = h_reset;
    h->eof = h_eof; h->alloc_node = h_alloc; h->read_line = h_read; h->text_close = h_close;
}

static int pascal_equals(const uint8_t *p, const char *s) { size_t n = strlen(s); return p[0] == n && memcmp(p + 1, s, n) == 0; }

static const uint8_t k_live[] = { 3, 'C', ':', '\\' };
#define CALL(headp, partyp, hp, maxn, maxp, rp) \
    pr_u6f_0010_read_list((headp), k_live, sizeof k_live, k_name, 12, k_marker, 2, (partyp), (hp), (maxn), (maxp), (rp))

static int test_selector_false(void) {
    fake_t f; pr_u6f_0010_hooks h; pr_u6f_0010_result r; pr_u6f_0010_node *head = (pr_u6f_0010_node *)1;
    reset(&f, &h); f.select_al = 0;
    CHECK(CALL(&head, NULL, &h, 16, 16, &r) == PR_U6F_0010_OK);
    CHECK(head == NULL && strcmp(f.log, "ES") == 0);              /* head cleared first; nothing opened, nothing closed */
    CHECK(r.selected == 0 && r.closed == 0 && r.nodes == 0 && r.next_offset == PR_U6F_0010_END_OFFSET);
    CHECK(pascal_equals(f.select_path, "C:\\NAME.TXT    "));
    return 0;
}

static int test_empty_file(void) {
    fake_t f; pr_u6f_0010_hooks h; pr_u6f_0010_result r; pr_u6f_0010_node *head = (pr_u6f_0010_node *)1;
    reset(&f, &h);
    CHECK(CALL(&head, NULL, &h, 16, 16, &r) == PR_U6F_0010_OK);
    CHECK(head == NULL && strcmp(f.log, "ESARFC") == 0);
    CHECK(r.selected == 1 && r.closed == 1 && r.nodes == 0 && r.party_steps == 0 && r.next_offset == PR_U6F_0010_END_OFFSET);
    CHECK(pascal_equals(f.assign_path, "C:\\NAME.TXT    ") && memcmp(f.select_path, f.assign_path, 16) == 0);
    return 0;
}

static int test_lines_and_party(void) {
    fake_t f; pr_u6f_0010_hooks h; pr_u6f_0010_result r; pr_u6f_0010_node *head = NULL;
    static const uint8_t zed[] = { 3, 'Z', 'E', 'D' };
    static const uint8_t bravo[] = { 5, 'B', 'R', 'A', 'V', 'O' };
    pr_u6f_0010_party second = { bravo, sizeof bravo, NULL }, first = { zed, sizeof zed, &second };
    reset(&f, &h); f.lines[0] = "ALPHA"; f.lines[1] = "BRAVO"; f.lines[2] = ""; f.lines[3] = "bravo"; f.line_count = 4;
    CHECK(CALL(&head, &first, &h, 16, 16, &r) == PR_U6F_0010_OK);
    CHECK(strcmp(f.log, "ESARFGLFGLFGLFGLFC") == 0);
    CHECK(r.nodes == 4 && r.marked == 1 && r.party_steps == 8 && r.closed == 1);
    CHECK(head == &f.pool[0] && head->next == &f.pool[1] && head->next->next == &f.pool[2] && head->next->next->next == &f.pool[3] && f.pool[3].next == NULL);
    CHECK(pascal_equals(f.pool[0].name, "ALPHA"));
    CHECK(pascal_equals(f.pool[1].name, "#-BRAVO"));                /* in the party: marker + name */
    CHECK(f.pool[2].name[0] == 0);                                  /* an empty line still makes a node */
    CHECK(pascal_equals(f.pool[3].name, "bravo"));                  /* the compare is case-exact */
    CHECK(f.pool[0].reserved == 0 && f.pool[0].name[40] == 0);      /* FillChar cleared the 0xCC fill */
    return 0;
}

static int test_truncation(void) {
    fake_t f; pr_u6f_0010_hooks h; pr_u6f_0010_result r; pr_u6f_0010_node *head = NULL;
    static char long_line[] = "0123456789012345678901234567890123456789XYZ";   /* 43 characters */
    static uint8_t forty[41];
    pr_u6f_0010_party one = { forty, sizeof forty, NULL };
    forty[0] = 40; memcpy(forty + 1, long_line, 40);
    reset(&f, &h); f.lines[0] = long_line; f.lines[1] = "0123456789012345678901234567890123456789"; f.line_count = 2;
    CHECK(CALL(&head, &one, &h, 16, 16, &r) == PR_U6F_0010_OK);
    CHECK(r.nodes == 2 && r.marked == 2);
    /* the read keeps 40 characters; the marked copy keeps the first 40 of marker + name */
    CHECK(pascal_equals(f.pool[0].name, "#-01234567890123456789012345678901234567"));
    CHECK(pascal_equals(f.pool[1].name, "#-01234567890123456789012345678901234567"));
    /* an empty party name matches an empty line */
    { static const uint8_t empty[] = { 0 }; pr_u6f_0010_party e = { empty, sizeof empty, NULL }; head = NULL;
      reset(&f, &h); f.lines[0] = ""; f.line_count = 1;
      CHECK(CALL(&head, &e, &h, 16, 16, &r) == PR_U6F_0010_OK && r.marked == 1 && pascal_equals(f.pool[0].name, "#-")); }
    return 0;
}

static int test_boundaries_and_failures(void) {
    fake_t f; pr_u6f_0010_hooks h; pr_u6f_0010_result r; pr_u6f_0010_node *head;
    static const uint8_t zed[] = { 3, 'Z', 'E', 'D' };
    pr_u6f_0010_party one = { zed, sizeof zed, NULL };
    /* every boundary without its callback, in reach order */
    reset(&f, &h); h.ensure_directory = NULL; head = (pr_u6f_0010_node *)1;
    CHECK(CALL(&head, &one, &h, 16, 16, &r) == PR_U6F_0010_NEED_HOOK && r.next_offset == 0x0024 && head == NULL);
    reset(&f, &h); h.select_file = NULL;
    CHECK(CALL(&head, &one, &h, 16, 16, &r) == PR_U6F_0010_NEED_HOOK && r.next_offset == 0x0041 && strcmp(f.log, "E") == 0);
    reset(&f, &h); h.text_assign = NULL;
    CHECK(CALL(&head, &one, &h, 16, 16, &r) == PR_U6F_0010_NEED_HOOK && r.next_offset == 0x006D && r.selected == 1);
    reset(&f, &h); h.text_reset = NULL;
    CHECK(CALL(&head, &one, &h, 16, 16, &r) == PR_U6F_0010_NEED_HOOK && r.next_offset == 0x0078);
    reset(&f, &h); h.eof = NULL;
    CHECK(CALL(&head, &one, &h, 16, 16, &r) == PR_U6F_0010_NEED_HOOK && r.next_offset == 0x0083);
    reset(&f, &h); h.alloc_node = NULL; f.lines[0] = "A"; f.line_count = 1;
    CHECK(CALL(&head, &one, &h, 16, 16, &r) == PR_U6F_0010_NEED_HOOK && r.next_offset == 0x0093);
    reset(&f, &h); h.read_line = NULL; f.lines[0] = "A"; f.line_count = 1;
    CHECK(CALL(&head, &one, &h, 16, 16, &r) == PR_U6F_0010_NEED_HOOK && r.next_offset == 0x010E && r.nodes == 1 && head == &f.pool[0]);
    reset(&f, &h); h.text_close = NULL; f.lines[0] = "A"; f.line_count = 1;
    CHECK(CALL(&head, &one, &h, 16, 16, &r) == PR_U6F_0010_NEED_HOOK && r.next_offset == 0x018C && r.nodes == 1 && pascal_equals(f.pool[0].name, "A"));
    /* host failures keep the earlier effects; the list stays reachable */
    reset(&f, &h); f.fail_close = 1; f.lines[0] = "A"; f.lines[1] = "B"; f.line_count = 2;
    CHECK(CALL(&head, &one, &h, 16, 16, &r) == PR_U6F_0010_HOST_FAILURE && r.nodes == 2 && r.closed == 0 && head == &f.pool[0] && head->next == &f.pool[1]);
    reset(&f, &h); f.fail_select = 1;
    CHECK(CALL(&head, &one, &h, 16, 16, &r) == PR_U6F_0010_HOST_FAILURE && strcmp(f.log, "ES") == 0 && r.selected == 0);
    reset(&f, &h); f.fail_alloc = 1; f.lines[0] = "A"; f.line_count = 1;
    CHECK(CALL(&head, &one, &h, 16, 16, &r) == PR_U6F_0010_HOST_FAILURE && r.next_offset == 0x0093 && r.nodes == 0 && head == NULL);
    reset(&f, &h); f.fail_read = 1; f.lines[0] = "A"; f.line_count = 1;
    CHECK(CALL(&head, &one, &h, 16, 16, &r) == PR_U6F_0010_HOST_FAILURE && r.next_offset == 0x010E && r.nodes == 1);
    /* host contract breaks: a NULL node, a read past the count */
    reset(&f, &h); f.alloc_null = 1; f.lines[0] = "A"; f.line_count = 1;
    CHECK(CALL(&head, &one, &h, 16, 16, &r) == PR_U6F_0010_ARGUMENT && r.next_offset == 0x0093 && r.nodes == 0);
    reset(&f, &h); f.over_read = 41; f.lines[0] = "A"; f.line_count = 1;
    CHECK(CALL(&head, &one, &h, 16, 16, &r) == PR_U6F_0010_ARGUMENT && r.next_offset == 0x010E && r.nodes == 1);
    /* the read loop never ends without Eof; the host guard reports it */
    reset(&f, &h); f.never_eof = 1; f.lines[0] = "A"; f.line_count = 1;
    CHECK(CALL(&head, &one, &h, 3, 16, &r) == PR_U6F_0010_LINE_LIMIT && r.nodes == 3 && r.closed == 0 && r.next_offset == 0x0083);
    /* a party cycle never terminates; the host guard reports it */
    { pr_u6f_0010_party cyc = { zed, sizeof zed, NULL }; cyc.next = &cyc;
      reset(&f, &h); f.lines[0] = "A"; f.line_count = 1;
      CHECK(CALL(&head, &cyc, &h, 16, 5, &r) == PR_U6F_0010_PARTY_LIMIT && r.party_steps == 5 && r.nodes == 1 && r.next_offset == 0x0125); }
    /* path overflow: a 250-character live path plus the 12-character name exceeds 255 */
    { static uint8_t longp[251]; longp[0] = 250; memset(longp + 1, 'P', 250); reset(&f, &h);
      CHECK(pr_u6f_0010_read_list(&head, longp, sizeof longp, k_name, 12, k_marker, 2, &one, &h, 16, 16, &r) == PR_U6F_0010_BOUNDS && strcmp(f.log, "E") == 0 && r.next_offset == 0x0027); }
    /* arguments: wrong literal lengths, live-string span, null hooks, null head, null output */
    reset(&f, &h);
    CHECK(pr_u6f_0010_read_list(&head, k_live, sizeof k_live, k_name, 11, k_marker, 2, &one, &h, 16, 16, &r) == PR_U6F_0010_ARGUMENT);
    CHECK(pr_u6f_0010_read_list(&head, k_live, sizeof k_live, k_name, 12, k_marker, 1, &one, &h, 16, 16, &r) == PR_U6F_0010_ARGUMENT);
    CHECK(pr_u6f_0010_read_list(&head, k_live, 3, k_name, 12, k_marker, 2, &one, &h, 16, 16, &r) == PR_U6F_0010_ARGUMENT);   /* length 3 needs 4 bytes */
    CHECK(pr_u6f_0010_read_list(&head, k_live, sizeof k_live, k_name, 12, k_marker, 2, &one, NULL, 16, 16, &r) == PR_U6F_0010_ARGUMENT);
    CHECK(pr_u6f_0010_read_list(NULL, k_live, sizeof k_live, k_name, 12, k_marker, 2, &one, &h, 16, 16, &r) == PR_U6F_0010_ARGUMENT);
    CHECK(pr_u6f_0010_read_list(&head, k_live, sizeof k_live, k_name, 12, k_marker, 2, &one, &h, 16, 16, NULL) == PR_U6F_0010_ARGUMENT);
    CHECK(strcmp(f.log, "") == 0);
    { static const uint8_t empty0[] = { 0 }; reset(&f, &h);        /* a zero-length live path is accepted: no byte after it is read here */
      CHECK(pr_u6f_0010_read_list(&head, empty0, sizeof empty0, k_name, 12, k_marker, 2, NULL, &h, 16, 16, &r) == PR_U6F_0010_OK && pascal_equals(f.assign_path, "NAME.TXT    ")); }
    /* party record spans: checked before the compare, after the node was read; earlier nodes stand */
    { static const uint8_t claims5[] = { 5, 'A', 'B' }; pr_u6f_0010_party lying = { claims5, sizeof claims5, NULL };
      reset(&f, &h); f.lines[0] = "A"; f.line_count = 1;
      CHECK(CALL(&head, &lying, &h, 16, 16, &r) == PR_U6F_0010_ARGUMENT && r.next_offset == 0x0137 && r.nodes == 1 && r.party_steps == 1 && pascal_equals(f.pool[0].name, "A")); }
    /* a callback that moves the published head (PM review i8549): fail closed at the engine's
     * reread of head^ (+00AF), before the new node is linked through the stale tail */
    { reset(&f, &h); f.head_ref = &head; f.move_head_after_read = 1; f.lines[0] = "A"; f.lines[1] = "B"; f.line_count = 2;
      CHECK(CALL(&head, &one, &h, 16, 16, &r) == PR_U6F_0010_ARGUMENT && r.next_offset == 0x00AF && r.nodes == 2);
      CHECK(head == &f.spare && f.pool[0].next == NULL && pascal_equals(f.pool[0].name, "A") && f.pool[1].name[0] == 0);   /* nothing appended through the old tail; the first node stands */
      CHECK(strcmp(f.log, "ESARFGLFG") == 0); }
    { reset(&f, &h); f.head_ref = &head; f.move_head_to_null_after_read = 1; f.lines[0] = "A"; f.lines[1] = "B"; f.line_count = 2;
      CHECK(CALL(&head, &one, &h, 16, 16, &r) == PR_U6F_0010_ARGUMENT && r.next_offset == 0x00AF && r.nodes == 2 && head == NULL && f.pool[0].next == NULL); }
    { reset(&f, &h); f.head_ref = &head; f.move_head_in_ensure = 1; f.lines[0] = "A"; f.line_count = 1;   /* moved before the first link */
      CHECK(CALL(&head, &one, &h, 16, 16, &r) == PR_U6F_0010_ARGUMENT && r.next_offset == 0x00AF && r.nodes == 1 && head == &f.spare && f.spare.next == NULL);
      CHECK(strcmp(f.log, "ESARFG") == 0); }
    { reset(&f, &h); f.head_ref = &head; f.move_head_in_ensure = 1;   /* moved before the first link, but no line follows: the engine never rereads head^ */
      CHECK(CALL(&head, &one, &h, 16, 16, &r) == PR_U6F_0010_OK && r.nodes == 0 && r.closed == 1 && head == &f.spare); }
    { pr_u6f_0010_party bad = { NULL, 2, NULL }; reset(&f, &h); f.lines[0] = "A"; f.line_count = 1;
      CHECK(CALL(&head, &bad, &h, 16, 16, &r) == PR_U6F_0010_ARGUMENT && r.next_offset == 0x0137); }
    { static const uint8_t a1[] = { 1, 'A' }; static const uint8_t claims5[] = { 5, 'A', 'B' };
      pr_u6f_0010_party second = { claims5, sizeof claims5, NULL }, first = { a1, sizeof a1, &second };
      reset(&f, &h); f.lines[0] = "A"; f.lines[1] = "Q"; f.line_count = 2;   /* the first line matches before the bad record is reached; the second line reaches it */
      CHECK(CALL(&head, &first, &h, 16, 16, &r) == PR_U6F_0010_ARGUMENT && r.nodes == 2 && r.marked == 1 && pascal_equals(f.pool[0].name, "#-A") && pascal_equals(f.pool[1].name, "Q")); }
    return 0;
}

int main(void) {
    int rc = 0;
    rc |= test_selector_false();
    rc |= test_empty_file();
    rc |= test_lines_and_party();
    rc |= test_truncation();
    rc |= test_boundaries_and_failures();
    if (rc == 0) {
        puts("overlay U6F +0010 read-list tests passed");
    }
    return rc;
}
