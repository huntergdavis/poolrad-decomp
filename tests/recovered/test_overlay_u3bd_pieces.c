/* Synthetic tests for the unit 0x3bd entry 6 model. The block, the pieces
 * buffer and the tables are fabricated; callbacks capture their calls. */
#include "poolrad/recovered/overlay_u3bd_pieces.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

typedef struct {
    uint8_t block[4 * PR_U3BD_PIECE_SIZE + 64];
    uint16_t size;
    size_t capacity;
    int give_block;
    unsigned reads, notices, exits, releases, announces;
    uint8_t read_file, read_id;
    const uint8_t *released; uint16_t released_size;
    uint16_t args[8]; uint8_t slots_called[8];
    char order[32];
} capture;

static void ev(capture *c, char ch) { size_t n = strlen(c->order); if (n + 1 < sizeof c->order) { c->order[n] = ch; c->order[n + 1] = 0; } }

static void on_read(void *ctx, uint8_t file, uint8_t id, const uint8_t **block, uint16_t *size, size_t *capacity)
{
    capture *c = (capture *)ctx; c->reads++; c->read_file = file; c->read_id = id; ev(c, 'R');
    *block = c->give_block ? c->block : NULL; *size = c->size; *capacity = c->capacity;
}
static void on_notice(void *ctx) { capture *c = (capture *)ctx; c->notices++; ev(c, 'N'); }
static void on_exit(void *ctx) { capture *c = (capture *)ctx; c->exits++; ev(c, 'X'); }
static void on_release(void *ctx, const uint8_t *b, uint16_t s) { capture *c = (capture *)ctx; c->releases++; c->released = b; c->released_size = s; ev(c, 'F'); }
static void on_piece(void *ctx, uint16_t arg, uint8_t slot)
{
    capture *c = (capture *)ctx;
    if (c->announces < 8) { c->args[c->announces] = arg; c->slots_called[c->announces] = slot; }
    c->announces++; ev(c, 'P');
}

static pr_u3bd_callouts all(capture *c) { pr_u3bd_callouts co = {NULL, on_read, on_notice, on_exit, on_release, on_piece}; co.ctx = c; return co; }

static void setup(pr_u3bd_state *s, uint8_t *pieces, capture *c, uint16_t size)
{
    unsigned i;
    memset(s, 0, sizeof *s); memset(c, 0, sizeof *c);
    s->file_number = 3; s->pieces = pieces;
    s->table_2722[0] = 0x1000; s->table_2722[1] = 0x0020; s->table_2722[2] = 0x0025; s->table_2722[3] = 0x0130;
    s->threshold_272c = 0x80;
    for (i = 0; i < 5; ++i) { s->slots[i].w6aae = (uint16_t)(0x1100 + i); s->slots[i].w6ab0 = (uint16_t)(0x2200 + i); }
    memset(pieces, 0xEE, PR_U3BD_BUFFER_SIZE);
    for (i = 0; i < sizeof c->block; ++i) c->block[i] = (uint8_t)(i & 0xFF);
    c->size = size; c->capacity = sizeof c->block; c->give_block = 1;
}

static uint8_t remap(uint8_t b, uint16_t t, uint8_t d) { return (uint16_t)b >= t ? (uint8_t)(b + d) : b; }

static int untouched(const pr_u3bd_state *s, const uint8_t *pieces)
{
    unsigned k;
    for (k = 0; k < 5u; ++k) if (s->slots[k].w6aae != 0x1100 + k || s->slots[k].w6ab0 != 0x2200 + k) return 0;
    for (k = 0; k < PR_U3BD_BUFFER_SIZE; ++k) if (pieces[k] != 0xEE) return 0;
    return 1;
}

static int test_gate_and_arguments(void)
{
    pr_u3bd_state s; uint8_t pieces[PR_U3BD_BUFFER_SIZE]; capture c; pr_u3bd_callouts co; pr_u3bd_report rep;
    unsigned v;
    setup(&s, pieces, &c, 780); co = all(&c);
    for (v = 0; v < 256u; ++v) {
        pr_u3bd_state before;
        if (v >= 1u && v <= 3u) continue;
        before = s;
        CHECK(pr_u3bd_entry6(&s, 7, (uint8_t)v, &co, &rep) == PR_U3BD_OK);
        CHECK(rep.slot_ok == 0 && c.reads == 0 && c.order[0] == 0);
        CHECK(memcmp(&before, &s, sizeof s) == 0 && untouched(&s, pieces));
    }
    CHECK(pr_u3bd_entry6(NULL, 1, 1, &co, &rep) == PR_U3BD_ARGUMENT);
    CHECK(pr_u3bd_entry6(&s, 1, 1, NULL, &rep) == PR_U3BD_ARGUMENT);
    { pr_u3bd_state t = s; t.pieces = NULL; CHECK(pr_u3bd_entry6(&t, 1, 1, &co, &rep) == PR_U3BD_ARGUMENT); }
    /* missing callbacks at the point reached, earlier effects done */
    { pr_u3bd_callouts no = co; no.read_block = NULL;
      CHECK(pr_u3bd_entry6(&s, 1, 1, &no, &rep) == PR_U3BD_CALLOUT && rep.slot_ok == 1 && c.reads == 0); }
    { pr_u3bd_callouts no = co; no.release = NULL; setup(&s, pieces, &c, 780);
      CHECK(pr_u3bd_entry6(&s, 1, 1, &no, &rep) == PR_U3BD_CALLOUT && rep.pieces_copied == 1 && pieces[0] == 0 && c.releases == 0); }
    { pr_u3bd_callouts no = co; no.piece_loaded = NULL; setup(&s, pieces, &c, 780);
      CHECK(pr_u3bd_entry6(&s, 1, 1, &no, &rep) == PR_U3BD_CALLOUT && rep.remapped == 1 && c.releases == 1 && s.slots[1].w6aae == 0xFFFF); }
    { pr_u3bd_callouts no = co; no.error_notice = NULL; setup(&s, pieces, &c, 0);
      CHECK(pr_u3bd_entry6(&s, 1, 1, &no, &rep) == PR_U3BD_CALLOUT && c.reads == 1 && rep.notice == 0 && c.exits == 0); }
    { pr_u3bd_callouts no = co; no.exit_program = NULL; setup(&s, pieces, &c, 0);
      CHECK(pr_u3bd_entry6(&s, 1, 1, &no, &rep) == PR_U3BD_CALLOUT && rep.notice == 1 && rep.exited == 0 && strcmp(c.order, "RN") == 0); }
    return 0;
}

static int test_single_piece(void)
{
    pr_u3bd_state s; uint8_t pieces[PR_U3BD_BUFFER_SIZE]; capture c; pr_u3bd_callouts co; pr_u3bd_report rep;
    unsigned k, slot;
    for (slot = 1; slot <= 3u; ++slot) {
        setup(&s, pieces, &c, 780); co = all(&c);
        CHECK(pr_u3bd_entry6(&s, 0x2A, (uint8_t)slot, &co, &rep) == PR_U3BD_OK);
        CHECK(strcmp(c.order, "RFP") == 0);
        CHECK(c.read_file == 3 && c.read_id == 0x2A && c.released == c.block && c.released_size == 780);
        CHECK(rep.count == 1 && rep.pieces_copied == 1 && rep.remapped == 1 && rep.announced == 1 && rep.notice == 0 && rep.exited == 0);
        CHECK(c.args[0] == 0x2A && c.slots_called[0] == slot);       /* one piece: arg = id */
        {
            uint8_t delta = (uint8_t)(s.table_2722[slot] - s.table_2722[1]);
            for (k = 0; k < 780u; ++k) CHECK(pieces[(slot - 1) * 780 + k] == remap((uint8_t)(k & 0xFF), 0x80, delta));
            for (k = 0; k < PR_U3BD_BUFFER_SIZE; ++k) if (k / 780 != slot - 1) CHECK(pieces[k] == 0xEE);
        }
        CHECK(s.slots[slot].w6aae == 0x2A && s.slots[slot].w6ab0 == slot);
        for (k = 0; k < 5u; ++k) if (k != slot) CHECK(s.slots[k].w6aae == 0x1100 + k && s.slots[k].w6ab0 == 0x2200 + k);
    }
    /* threshold above 255: nothing remapped; delta wraps otherwise */
    setup(&s, pieces, &c, 780); co = all(&c); s.threshold_272c = 0x100;
    CHECK(pr_u3bd_entry6(&s, 1, 1, &co, &rep) == PR_U3BD_OK);
    for (k = 0; k < 780u; ++k) CHECK(pieces[k] == (uint8_t)(k & 0xFF));
    setup(&s, pieces, &c, 780); co = all(&c); s.threshold_272c = 0; s.table_2722[1] = 1; s.table_2722[2] = 0;  /* delta 0xFF */
    CHECK(pr_u3bd_entry6(&s, 1, 2, &co, &rep) == PR_U3BD_OK);
    for (k = 0; k < 780u; ++k) CHECK(pieces[780 + k] == (uint8_t)((k & 0xFF) - 1u));
    /* id 10 with one piece is announced as 10 but stored as 0 */
    setup(&s, pieces, &c, 780); co = all(&c);
    CHECK(pr_u3bd_entry6(&s, 10, 1, &co, &rep) == PR_U3BD_OK && c.args[0] == 10 && s.slots[1].w6aae == 0 && rep.final_id == 0);
    return 0;
}

static int test_multi_piece(void)
{
    pr_u3bd_state s; uint8_t pieces[PR_U3BD_BUFFER_SIZE]; capture c; pr_u3bd_callouts co; pr_u3bd_report rep;
    unsigned k;
    /* three pieces from slot 1: all three in the set; args (id*10 + i) */
    setup(&s, pieces, &c, 3 * 780); co = all(&c);
    CHECK(pr_u3bd_entry6(&s, 5, 1, &co, &rep) == PR_U3BD_OK);
    CHECK(strcmp(c.order, "RFPPP") == 0 && rep.count == 3 && rep.remapped == 3);
    CHECK(c.args[0] == 51 && c.args[1] == 52 && c.args[2] == 53);
    CHECK(c.slots_called[0] == 1 && c.slots_called[1] == 2 && c.slots_called[2] == 3);
    CHECK(s.slots[1].w6aae == 5 && s.slots[1].w6ab0 == 1);             /* final write wins for slot 1 */
    CHECK(s.slots[2].w6aae == 0xFFFF && s.slots[2].w6ab0 == 0xFFFF);   /* others keep the 0xFFFF marks */
    CHECK(s.slots[3].w6aae == 0xFFFF && s.slots[4].w6aae == 0x1104);
    for (k = 0; k < 3 * 780u; ++k) CHECK(pieces[k] == (uint8_t)(k & 0xFF));   /* delta 0 for slot 1 */
    /* id 0 with several pieces becomes 10 for the announcements (101, 102) and 0 in the table */
    setup(&s, pieces, &c, 2 * 780); co = all(&c);
    CHECK(pr_u3bd_entry6(&s, 0, 2, &co, &rep) == PR_U3BD_OK);
    CHECK(c.args[0] == 101 && c.args[1] == 102 && c.slots_called[0] == 2 && c.slots_called[1] == 3);
    CHECK(s.slots[2].w6aae == 0 && s.slots[2].w6ab0 == 2 && rep.final_id == 0);
    /* slot 4 is reachable only through a trailing partial piece: 781 bytes from slot 3
     * (780 + 1 -> floor 1, 1 + 3 = 4 passes; ceil 2 pieces). Slot 4 is copied raw and is
     * neither marked, remapped nor announced (not in the set). */
    setup(&s, pieces, &c, 781); co = all(&c);
    CHECK(pr_u3bd_entry6(&s, 7, 3, &co, &rep) == PR_U3BD_OK);
    CHECK(strcmp(c.order, "RFP") == 0 && rep.notice == 0 && rep.count == 2 && rep.pieces_copied == 2 && rep.remapped == 1 && rep.announced == 1);
    CHECK(c.args[0] == 71 && c.slots_called[0] == 3 && c.released_size == 781);
    CHECK(s.slots[4].w6aae == 0x1104 && s.slots[4].w6ab0 == 0x2204);
    for (k = 0; k < 780u; ++k) CHECK(pieces[3 * 780 + k] == (uint8_t)((780 + k) & 0xFF));   /* raw copy past size */
    /* four pieces from slot 1 with 2341 bytes: floor 3 + 1 = 4 passes, ceil 4 copies fill slots 1..4 */
    setup(&s, pieces, &c, 2341); co = all(&c);
    CHECK(pr_u3bd_entry6(&s, 9, 1, &co, &rep) == PR_U3BD_OK && rep.pieces_copied == 4 && rep.remapped == 3 && rep.announced == 3);
    CHECK(c.args[2] == 93 && c.slots_called[2] == 3);
    /* BLOCK_BOUNDS when the host block cannot supply the trailing piece */
    setup(&s, pieces, &c, 781); co = all(&c); c.capacity = 781;
    CHECK(pr_u3bd_entry6(&s, 1, 1, &co, &rep) == PR_U3BD_BLOCK_BOUNDS && rep.pieces_copied == 1 && c.releases == 0);
    return 0;
}

static int test_terminal_notice(void)
{
    pr_u3bd_state s; uint8_t pieces[PR_U3BD_BUFFER_SIZE]; capture c; pr_u3bd_callouts co; pr_u3bd_report rep;
    /* size 0 (loader failure): read, notice, exit; nothing copied, released or written */
    setup(&s, pieces, &c, 0); co = all(&c); c.give_block = 0;
    CHECK(pr_u3bd_entry6(&s, 1, 1, &co, &rep) == PR_U3BD_EXITED);
    CHECK(strcmp(c.order, "RNX") == 0 && rep.notice == 1 && rep.exited == 1 && rep.pieces_copied == 0 && c.releases == 0 && c.announces == 0);
    CHECK(untouched(&s, pieces));
    /* size 0 with a readable block: the same, the block is never read */
    setup(&s, pieces, &c, 0); co = all(&c);
    CHECK(pr_u3bd_entry6(&s, 4, 1, &co, &rep) == PR_U3BD_EXITED && strcmp(c.order, "RNX") == 0 && untouched(&s, pieces));
    /* too many pieces for the slot: 3 + 3, 2 + 3, 3 + 2 all exit; 1 + 3 and 2 + 2 do not */
    setup(&s, pieces, &c, 3 * 780); co = all(&c);
    CHECK(pr_u3bd_entry6(&s, 2, 3, &co, &rep) == PR_U3BD_EXITED && strcmp(c.order, "RNX") == 0 && untouched(&s, pieces));
    setup(&s, pieces, &c, 2 * 780); co = all(&c);
    CHECK(pr_u3bd_entry6(&s, 2, 3, &co, &rep) == PR_U3BD_EXITED && untouched(&s, pieces));
    setup(&s, pieces, &c, 3 * 780); co = all(&c);
    CHECK(pr_u3bd_entry6(&s, 2, 2, &co, &rep) == PR_U3BD_EXITED && untouched(&s, pieces));
    setup(&s, pieces, &c, 780); co = all(&c);
    CHECK(pr_u3bd_entry6(&s, 2, 3, &co, &rep) == PR_U3BD_OK && rep.notice == 0);
    setup(&s, pieces, &c, 2 * 780); co = all(&c);
    CHECK(pr_u3bd_entry6(&s, 2, 2, &co, &rep) == PR_U3BD_OK && rep.notice == 0 && rep.pieces_copied == 2);
    /* the gate uses the integer quotient: 1559 bytes from slot 3 is 1 + 3 = 4, no exit */
    setup(&s, pieces, &c, 1559); co = all(&c);
    CHECK(pr_u3bd_entry6(&s, 2, 3, &co, &rep) == PR_U3BD_OK && rep.notice == 0 && rep.pieces_copied == 2);
    /* 1560 bytes from slot 3 is 2 + 3 = 5: exit */
    setup(&s, pieces, &c, 1560); co = all(&c);
    CHECK(pr_u3bd_entry6(&s, 2, 3, &co, &rep) == PR_U3BD_EXITED);
    return 0;
}

int main(void)
{
    int rc = 0;
    rc |= test_gate_and_arguments();
    rc |= test_single_piece();
    rc |= test_multi_piece();
    rc |= test_terminal_notice();
    CHECK(pr_u3bd_slot_in_set(0) == 0 && pr_u3bd_slot_in_set(1) && pr_u3bd_slot_in_set(3) && pr_u3bd_slot_in_set(4) == 0);
    if (rc == 0) puts("unit 0x3bd entry 6 model tests passed");
    return rc;
}
