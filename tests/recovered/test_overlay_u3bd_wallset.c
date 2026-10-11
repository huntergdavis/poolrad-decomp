/* Synthetic tests for the unit 0x3bd entry 7 model: block, buffer and
 * window word are fabricated; callbacks capture calls and order. */
#include "poolrad/recovered/overlay_u3bd_wallset.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

typedef struct {
    uint8_t block[0x500];
    uint16_t size; size_t capacity; int give_block;
    unsigned reads, notices, exits, releases;
    uint8_t read_file, read_id;
    const uint8_t *released; uint16_t released_size;
    char order[16];
} capture;

static void ev(capture *c, char ch) { size_t n = strlen(c->order); if (n + 1 < sizeof c->order) { c->order[n] = ch; c->order[n + 1] = 0; } }
static void on_read(void *ctx, uint8_t file, uint8_t id, const uint8_t **block, uint16_t *size, size_t *capacity)
{ capture *c = (capture *)ctx; c->reads++; c->read_file = file; c->read_id = id; ev(c, 'R'); *block = c->give_block ? c->block : NULL; *size = c->size; *capacity = c->capacity; }
static void on_notice(void *ctx) { capture *c = (capture *)ctx; c->notices++; ev(c, 'N'); }
static void on_exit(void *ctx) { capture *c = (capture *)ctx; c->exits++; ev(c, 'X'); }
static void on_release(void *ctx, const uint8_t *b, uint16_t s) { capture *c = (capture *)ctx; c->releases++; c->released = b; c->released_size = s; ev(c, 'F'); }

static pr_u3bd_wallset_callouts all(capture *c) { pr_u3bd_wallset_callouts co = {NULL, on_read, on_notice, on_exit, on_release}; co.ctx = c; return co; }

static void setup(pr_u3bd_wallset_state *s, uint8_t *buffer, capture *c, uint16_t size)
{
    unsigned i;
    memset(s, 0, sizeof *s); memset(c, 0, sizeof *c);
    s->file_number = 2; s->buffer = buffer; s->window0_word_c5 = 0xBEEF;
    memset(buffer, 0xEE, PR_U3BD_WALLSET_BUFFER_SIZE);
    for (i = 0; i < sizeof c->block; ++i) c->block[i] = (uint8_t)(i * 7u + 1u);
    c->size = size; c->capacity = sizeof c->block; c->give_block = 1;
}

static int test_success(void)
{
    pr_u3bd_wallset_state s; uint8_t buffer[PR_U3BD_WALLSET_BUFFER_SIZE]; capture c; pr_u3bd_wallset_callouts co; pr_u3bd_wallset_report rep;
    unsigned k, id;
    for (id = 0; id < 256u; id += 51u) {         /* 0, 51, 102, 153, 204, 255 */
        setup(&s, buffer, &c, 0x402); co = all(&c);
        CHECK(pr_u3bd_entry7(&s, (uint8_t)id, &co, &rep) == PR_U3BD_WALLSET_OK);
        CHECK(strcmp(c.order, "RF") == 0 && c.read_file == 2 && c.read_id == id);
        CHECK(c.released == c.block && c.released_size == 0x402);
        CHECK(rep.read_size == 0x402 && rep.copied == 1 && rep.notice == 0 && rep.exited == 0 && rep.stored_id == id);
        for (k = 0; k < PR_U3BD_WALLSET_BUFFER_SIZE; ++k) CHECK(buffer[k] == (uint8_t)((k + 2u) * 7u + 1u));   /* from block + 2 */
        CHECK(s.window0_word_c5 == id);          /* zero-extended byte */
    }
    /* the two header bytes and anything past 0x402 are never copied */
    setup(&s, buffer, &c, 0x402); co = all(&c);
    c.block[0] = 0xAA; c.block[1] = 0xBB; c.block[0x402] = 0xCC;
    CHECK(pr_u3bd_entry7(&s, 1, &co, &rep) == PR_U3BD_WALLSET_OK);
    CHECK(buffer[0] == c.block[2] && buffer[1] == c.block[3]);      /* header bytes skipped */
    CHECK(buffer[0x3FF] == c.block[0x401]);                          /* last byte copied is 0x401 */
    for (k = 0; k < PR_U3BD_WALLSET_BUFFER_SIZE; ++k) CHECK(buffer[k] == c.block[k + 2]);
    /* quarter boundaries map 1:1 */
    CHECK(buffer[0x100] == c.block[0x102] && buffer[0x200] == c.block[0x202] && buffer[0x300] == c.block[0x302]);
    return 0;
}

static int test_terminal(void)
{
    pr_u3bd_wallset_state s; uint8_t buffer[PR_U3BD_WALLSET_BUFFER_SIZE]; capture c; pr_u3bd_wallset_callouts co; pr_u3bd_wallset_report rep;
    unsigned k;
    static const uint16_t sizes[] = {0, 1, 0x401, 0x403, 0x400, 0x804, 0xFFFF};
    size_t i;
    for (i = 0; i < sizeof sizes / sizeof sizes[0]; ++i) {
        setup(&s, buffer, &c, sizes[i]); co = all(&c);
        if (sizes[i] == 0) c.give_block = 0;
        CHECK(pr_u3bd_entry7(&s, 9, &co, &rep) == PR_U3BD_WALLSET_EXITED);
        CHECK(strcmp(c.order, "RNX") == 0 && rep.notice == 1 && rep.exited == 1 && rep.copied == 0);
        CHECK(c.releases == 0 && s.window0_word_c5 == 0xBEEF);
        for (k = 0; k < PR_U3BD_WALLSET_BUFFER_SIZE; ++k) CHECK(buffer[k] == 0xEE);
    }
    /* missing callbacks at the point reached */
    { pr_u3bd_wallset_callouts no; setup(&s, buffer, &c, 0x402); co = all(&c); no = co; no.read_block = NULL;
      CHECK(pr_u3bd_entry7(&s, 1, &no, &rep) == PR_U3BD_WALLSET_CALLOUT && c.reads == 0); }
    { pr_u3bd_wallset_callouts no; setup(&s, buffer, &c, 5); co = all(&c); no = co; no.error_notice = NULL;
      CHECK(pr_u3bd_entry7(&s, 1, &no, &rep) == PR_U3BD_WALLSET_CALLOUT && c.reads == 1 && rep.notice == 0); }
    { pr_u3bd_wallset_callouts no; setup(&s, buffer, &c, 5); co = all(&c); no = co; no.exit_program = NULL;
      CHECK(pr_u3bd_entry7(&s, 1, &no, &rep) == PR_U3BD_WALLSET_CALLOUT && rep.notice == 1 && rep.exited == 0 && strcmp(c.order, "RN") == 0); }
    { pr_u3bd_wallset_callouts no; setup(&s, buffer, &c, 0x402); co = all(&c); no = co; no.release = NULL;
      CHECK(pr_u3bd_entry7(&s, 1, &no, &rep) == PR_U3BD_WALLSET_CALLOUT && rep.copied == 1 && buffer[0] == c.block[2] && s.window0_word_c5 == 0xBEEF); }
    /* a host block reporting 0x402 bytes but shorter in capacity: bounds, nothing copied */
    setup(&s, buffer, &c, 0x402); co = all(&c); c.capacity = 0x401;
    CHECK(pr_u3bd_entry7(&s, 1, &co, &rep) == PR_U3BD_WALLSET_BLOCK_BOUNDS && rep.copied == 0 && buffer[0] == 0xEE && c.releases == 0);
    /* null contracts: state and callouts on entry */
    CHECK(pr_u3bd_entry7(NULL, 1, &co, &rep) == PR_U3BD_WALLSET_ARGUMENT);
    CHECK(pr_u3bd_entry7(&s, 1, NULL, &rep) == PR_U3BD_WALLSET_ARGUMENT);
    /* a null output buffer with a bad-size block still reads, notices and exits: the
     * original touches [0x6A5C] only on the copy path */
    { pr_u3bd_wallset_state t; setup(&t, buffer, &c, 0x401); co = all(&c); t.buffer = NULL;
      CHECK(pr_u3bd_entry7(&t, 3, &co, &rep) == PR_U3BD_WALLSET_EXITED);
      CHECK(strcmp(c.order, "RNX") == 0 && rep.notice == 1 && rep.exited == 1 && rep.copied == 0 && t.window0_word_c5 == 0xBEEF); }
    /* a null output buffer with a good block: ARGUMENT at the copies, after the read, nothing copied or released */
    { pr_u3bd_wallset_state t; setup(&t, buffer, &c, 0x402); co = all(&c); t.buffer = NULL;
      CHECK(pr_u3bd_entry7(&t, 3, &co, &rep) == PR_U3BD_WALLSET_ARGUMENT);
      CHECK(strcmp(c.order, "R") == 0 && rep.read_size == 0x402 && rep.copied == 0 && c.releases == 0 && t.window0_word_c5 == 0xBEEF); }
    /* a loader that reports size and capacity 0x402 but no block: boundary, nothing copied */
    setup(&s, buffer, &c, 0x402); co = all(&c); c.give_block = 0; c.capacity = 0x402;
    CHECK(pr_u3bd_entry7(&s, 3, &co, &rep) == PR_U3BD_WALLSET_BLOCK_BOUNDS);
    CHECK(strcmp(c.order, "R") == 0 && rep.copied == 0 && buffer[0] == 0xEE && c.releases == 0 && s.window0_word_c5 == 0xBEEF);
    setup(&s, buffer, &c, 0x402); co = all(&c);
    CHECK(pr_u3bd_entry7(&s, 1, &co, NULL) == PR_U3BD_WALLSET_OK);      /* report optional */
    return 0;
}

int main(void)
{
    int rc = 0;
    rc |= test_success();
    rc |= test_terminal();
    if (rc == 0) puts("unit 0x3bd entry 7 model tests passed");
    return rc;
}
