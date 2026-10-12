/* Synthetic test for the unit 0x6f entry 6 model. The constants, blocks, objects and every hook are fabricated. Checked: the exact name/ext
 * strings handed to the block reader, the 0x11D-byte record move and the field clears, the class-byte rule for every value of +0x2E, the
 * five roll stores and +0xC4, the [0x557C] flag, the 9-byte and 0x3F-byte do-while unit chains (including the one-unit boundary and the read of
 * the whole last unit), FreeMem sizes, the overlap-safe move, size-0 handling (ERROR_EXIT for block 1, skip for blocks 2 and 3), 16-bit wrap,
 * every stop (UNMAPPED/VIEW/RANGE/HOOK/CALLBACK) with earlier effects standing, and the argument guards. */
#include "poolrad/recovered/overlay_u6f_entry6_0ec4.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

#define NOBJ 40
#define OBJSZ 0x200
static uint8_t mem[NOBJ][OBJSZ];
static pr_u25_e5_object table[NOBJ];
static unsigned nobj, heap_next;
static pr_u6f_e6_env env;
static pr_u6f_e6_ds ds;

static const uint8_t K98[] = { 3, 'a', 'b', 'c' }, K9C[] = { 3, 'x', 'y', 'z' }, KA0[] = { 4, 'e', 'x', 't', '!' }, KBC[] = { 3, 'q', 'r', 's' }, KC0[] = { 3, 'u', 'v', 'w' };

typedef struct {
    int log[40000]; unsigned n;
    uint8_t names[3][260], exts[3][8]; unsigned blk_calls;
    uint16_t sizes[3]; pr_far16 ptrs[3];            /* what the block hook hands out for blocks 1..3 */
    uint8_t ids[3];
    uint16_t freed_size[3]; pr_far16 freed_ptr[3]; unsigned frees;
    uint8_t rolls[8]; unsigned roll_i; unsigned roll_args_ok;
    uint8_t b557c_at_block[3];
    int fail_kind, fail_nth, fail_val; unsigned kind_count[8];
    uint8_t digit_len, digit;
    uint16_t getmem_sizes[16]; unsigned gm;
} Rec;
static Rec rec;

static int should_fail(int kind) { rec.kind_count[kind]++; return rec.fail_kind == kind && (int)rec.kind_count[kind] == rec.fail_nth ? rec.fail_val : 0; }

static pr_far16 add_obj(uint16_t off, uint16_t seg)
{
    pr_far16 p; p.off = off; p.seg = seg;
    table[nobj].segment = mem[nobj]; table[nobj].segment_size = OBJSZ; table[nobj].offset = off; table[nobj].segment_id = seg;
    nobj++;
    return p;
}
static uint8_t *bytes(pr_far16 p) { unsigned i; for (i = 0; i < nobj; i++) if (table[i].offset == p.off && table[i].segment_id == p.seg) return mem[i] + p.off; return NULL; }
static uint16_t w(pr_far16 p, uint16_t d) { uint8_t *b = bytes(p); return (uint16_t)(b[d] | (b[d + 1] << 8)); }
static pr_far16 ptr_at(pr_far16 p, uint16_t d) { pr_far16 q; q.off = w(p, d); q.seg = w(p, (uint16_t)(d + 2)); return q; }

static int h_str(void *c, const pr_u6f_e6_env *e, uint32_t v, uint8_t out[2])
{ Rec *r = c; (void)e; (void)v; r->log[r->n++] = 1; out[0] = r->digit_len; out[1] = r->digit; return should_fail(1); }
static int h_blk(void *c, const pr_u6f_e6_env *e, const uint8_t *nm, const uint8_t *ext, uint8_t id, uint16_t *size, pr_far16 *ptr)
{
    Rec *r = c; unsigned k = r->blk_calls++;
    (void)e; r->log[r->n++] = 2;
    if (k < 3) {
        memcpy(r->names[k], nm, 1u + nm[0]); memcpy(r->exts[k], ext, 1u + ext[0]); r->ids[k] = id;
        *size = r->sizes[k]; *ptr = r->ptrs[k]; r->b557c_at_block[k] = ds.b557c;
    }
    return should_fail(2);
}
static int h_gm(void *c, const pr_u6f_e6_env *e, uint16_t size, pr_far16 *res)
{
    Rec *r = c; (void)e;
    r->log[r->n++] = 3; if (r->gm < 16) r->getmem_sizes[r->gm] = size; r->gm++;
    *res = (pr_far16){ table[heap_next].offset, table[heap_next].segment_id }; heap_next++;
    return should_fail(3);
}
static int h_free(void *c, const pr_u6f_e6_env *e, pr_far16 p, uint16_t size)
{ Rec *r = c; (void)e; if (r->n < 39990) r->log[r->n] = 4; r->n++; r->freed_ptr[r->frees] = p; r->freed_size[r->frees++] = size; return should_fail(4); }
static int h_roll(void *c, const pr_u6f_e6_env *e, uint8_t cnt, uint8_t rng, uint8_t *al)
{ Rec *r = c; (void)e; r->log[r->n++] = 5; if (cnt != 1 || rng != 6) r->roll_args_ok = 0; *al = r->rolls[r->roll_i++]; return should_fail(5); }
static int h_flush(void *c, const pr_u6f_e6_env *e) { Rec *r = c; (void)e; r->log[r->n++] = 6; return should_fail(6); }

static pr_far16 OBJ, B1, B2, B3;

static void setup(void)
{
    unsigned i;
    memset(mem, 0, sizeof mem); memset(table, 0, sizeof table); nobj = 0;
    memset(&rec, 0, sizeof rec); rec.roll_args_ok = 1; rec.digit_len = 1; rec.digit = '7';
    OBJ = add_obj(0x10, 0x1000); B1 = add_obj(0x10, 0x2001); B2 = add_obj(0x10, 0x2002); B3 = add_obj(0x10, 0x2003);
    heap_next = nobj;
    for (i = 0; i < 24; i++) add_obj(0x08, (uint16_t)(0x3000 + i));
    for (i = 0; i < 0x120; i++) bytes(B1)[i] = (uint8_t)(i * 5 + 3);
    bytes(B1)[0x2E] = 3;
    for (i = 0; i < 0x40; i++) { bytes(B2)[i] = (uint8_t)(0x40 + i); bytes(B3)[i] = (uint8_t)(0xC0 + i); }
    for (i = 0x40; i < 0x100; i++) bytes(B3)[i] = (uint8_t)(i ^ 0x77);
    rec.sizes[0] = 0x120; rec.sizes[1] = 20; rec.sizes[2] = 0x80; rec.ptrs[0] = B1; rec.ptrs[1] = B2; rec.ptrs[2] = B3;
    for (i = 0; i < 8; i++) rec.rolls[i] = (uint8_t)(i + 1);
    memset(&env, 0, sizeof env);
    env.objects = table; env.object_count = nobj; env.c0e98 = K98; env.c0e9c = K9C; env.c0ea0 = KA0; env.c0ebc = KBC; env.c0ec0 = KC0; env.b5376 = 7;
    env.hooks.str_af8_1366 = h_str; env.hooks.block_802_0716 = h_blk; env.hooks.getmem_af8_023f = h_gm; env.hooks.free_af8_0254 = h_free;
    env.hooks.roll_b0_0048 = h_roll; env.hooks.flush_899_0104 = h_flush; env.ctx = &rec;
    memset(&ds, 0, sizeof ds);
    for (i = 0; i < 0x11D; i++) bytes(OBJ)[i] = 0xEE;                         /* residue the move must overwrite */
}

int main(void)
{
    pr_u6f_e6_report rep;
    pr_far16 u0, u1, u2;
    unsigned b, i;

    /* the whole path */
    setup();
    CHECK(pr_u6f_entry6(&ds, &env, OBJ, 0x2A, &rep) == PR_U6F_E6_OK);
    CHECK(memcmp(rec.names[0], "\x07" "abc7xyz", 8) == 0 && memcmp(rec.names[1], "\x07" "abc7qrs", 8) == 0 && memcmp(rec.names[2], "\x07" "abc7uvw", 8) == 0);
    CHECK(memcmp(rec.exts[0], "\x04" "ext!", 5) == 0 && memcmp(rec.exts[2], "\x04" "ext!", 5) == 0 && rec.ids[0] == 0x2A && rec.ids[1] == 0x2A && rec.ids[2] == 0x2A);
    CHECK(rec.b557c_at_block[0] == 0 && rec.b557c_at_block[1] == 1 && rec.b557c_at_block[2] == 1 && ds.b557c == 0);
    CHECK(rec.roll_args_ok && rep.n_rolls == 5 && rec.frees == 3 && rec.freed_size[0] == 0x120 && rec.freed_size[1] == 20 && rec.freed_size[2] == 0x80);
    CHECK(rec.freed_ptr[0].seg == 0x2001 && rec.freed_ptr[1].seg == 0x2002 && rec.freed_ptr[2].seg == 0x2003);
    /* the record: block bytes except the rewritten fields */
    CHECK(bytes(OBJ)[0] == bytes(B1)[0] && bytes(OBJ)[0x11C] == bytes(B1)[0x11C] && bytes(OBJ)[0x2E] == 3);
    CHECK(w(OBJ, 0x104) == 0 && w(OBJ, 0x106) == 0 && w(OBJ, 0x108) == 0 && w(OBJ, 0x10A) == 0);
    CHECK(rep.class_byte == 1 && bytes(OBJ)[0xC4] == 0xC4);
    /* rolls 1..5 go to +0xC1,+0xC2,+0xC3,+0xC5,+0xC6 as ((r+8)<<4)+r (low byte) */
    CHECK(bytes(OBJ)[0xC1] == (uint8_t)(((1 + 8) << 4) + 1) && bytes(OBJ)[0xC2] == (uint8_t)(((2 + 8) << 4) + 2) && bytes(OBJ)[0xC3] == (uint8_t)(((3 + 8) << 4) + 3));
    CHECK(bytes(OBJ)[0xC5] == (uint8_t)(((4 + 8) << 4) + 4) && bytes(OBJ)[0xC6] == (uint8_t)(((5 + 8) << 4) + 5) && bytes(OBJ)[0xC0] == 1);
    /* block 2: 20 bytes -> 3 nine-byte units (the last one reads bytes 18..26), chained on +0x7F, links at +5/+7 */
    CHECK(rep.size[0] == 0x120 && rep.size[1] == 20 && rep.size[2] == 0x80 && rep.units9 == 3 && rep.units3f == 3 && rep.blocks_loaded == 3);
    u0 = ptr_at(OBJ, 0x7F); u1 = ptr_at(u0, 5); u2 = ptr_at(u1, 5);
    CHECK(u0.seg == 0x3000 && u1.seg == 0x3001 && u2.seg == 0x3002 && w(u2, 5) == 0 && w(u2, 7) == 0);
    CHECK(memcmp(bytes(u0), bytes(B2), 5) == 0 && memcmp(bytes(u1), bytes(B2) + 9, 5) == 0 && memcmp(bytes(u2), bytes(B2) + 18, 5) == 0 && bytes(u2)[4] == bytes(B2)[22]);
    /* block 3: 0x80 bytes -> 3 units of 0x3F (the last reads 0x7E..0xBC), chain on +0xC8, links at +0x2A/+0x2C */
    u0 = ptr_at(OBJ, 0xC8); u1 = ptr_at(u0, 0x2A); u2 = ptr_at(u1, 0x2A);
    CHECK(u0.seg == 0x3003 && u1.seg == 0x3004 && u2.seg == 0x3005 && w(u2, 0x2A) == 0 && w(u2, 0x2C) == 0);
    CHECK(memcmp(bytes(u0), bytes(B3), 0x2A) == 0 && memcmp(bytes(u1), bytes(B3) + 0x3F, 0x2A) == 0 && memcmp(bytes(u2), bytes(B3) + 0x7E, 0x2A) == 0 && bytes(u2)[0x3E] == bytes(B3)[0xBC]);
    CHECK(rec.gm == 6 && rec.getmem_sizes[0] == 9 && rec.getmem_sizes[3] == 0x3F);

    /* the class byte rule for every +0x2E value: {1,3,5} -> 1, {2,4,6,7} -> 2, else the loaded byte stays */
    for (b = 0; b < 256; b++) {
        uint8_t want = (b == 1 || b == 3 || b == 5) ? 1 : (b == 2 || b == 4 || b == 6 || b == 7) ? 2 : bytes(B1)[0xC0];
        setup(); bytes(B1)[0x2E] = (uint8_t)b; rec.sizes[1] = 0; rec.sizes[2] = 0;
        CHECK(pr_u6f_entry6(&ds, &env, OBJ, 1, &rep) == PR_U6F_E6_OK && rep.class_byte == want);
    }

    /* unit boundaries: size 9 -> 1 unit, 10 -> 2 units, 0xFFFF handled as a RANGE stop is below */
    setup(); rec.sizes[1] = 9; rec.sizes[2] = 0x3F;
    CHECK(pr_u6f_entry6(&ds, &env, OBJ, 1, &rep) == PR_U6F_E6_OK && rep.units9 == 1 && rep.units3f == 1 && w(ptr_at(OBJ, 0x7F), 5) == 0);
    setup(); rec.sizes[1] = 10; rec.sizes[2] = 0x40;
    CHECK(pr_u6f_entry6(&ds, &env, OBJ, 1, &rep) == PR_U6F_E6_OK && rep.units9 == 2 && rep.units3f == 2);
    /* the roll values wrap in the byte arithmetic: r = 255 -> ((255 + 8) << 4) + 255 = 0x116F -> 0x6F */
    setup(); rec.rolls[0] = 255; rec.sizes[1] = 0; rec.sizes[2] = 0;
    CHECK(pr_u6f_entry6(&ds, &env, OBJ, 1, &rep) == PR_U6F_E6_OK && bytes(OBJ)[0xC1] == 0x6F);

    /* sizes 0: blocks 2 and 3 skip (no GetMem, no free for them); block 1 size 0 is the error exit before any effect */
    setup(); rec.sizes[1] = 0; rec.sizes[2] = 0;
    CHECK(pr_u6f_entry6(&ds, &env, OBJ, 1, &rep) == PR_U6F_E6_OK && rec.gm == 0 && rec.frees == 1 && rep.blocks_loaded == 1 && ds.b557c == 0 && w(OBJ, 0x7F) == 0 && w(OBJ, 0xC8) == 0);
    setup(); rec.sizes[0] = 0;
    CHECK(pr_u6f_entry6(&ds, &env, OBJ, 1, &rep) == PR_U6F_E6_ERROR_EXIT && rec.n == 2 && rec.gm == 0 && rec.frees == 0 && ds.b557c == 0 && bytes(OBJ)[0] == 0xEE && rec.roll_i == 0);

    /* digit string: length 0 and the 255 cap */
    setup(); rec.digit_len = 0; rec.sizes[1] = 0; rec.sizes[2] = 0;
    CHECK(pr_u6f_entry6(&ds, &env, OBJ, 1, &rep) == PR_U6F_E6_OK && memcmp(rec.names[0], "\x06" "abcxyz", 7) == 0);

    /* overlap-safe move: the block is in the same segment bytes as the record, below it (backward copy) or above it (forward copy) */
    setup();
    { pr_far16 lo = { 0x0C, 0x1000 }, hi = { 0x14, 0x1000 }; uint8_t ref[0x11D];
      table[nobj].segment = table[0].segment; table[nobj].segment_size = OBJSZ; table[nobj].offset = lo.off; table[nobj].segment_id = lo.seg; nobj++;
      table[nobj].segment = table[0].segment; table[nobj].segment_size = OBJSZ; table[nobj].offset = hi.off; table[nobj].segment_id = hi.seg; nobj++;
      env.object_count = nobj; rec.sizes[1] = 0; rec.sizes[2] = 0;
      for (i = 0; i < 0x11D + 8; i++) table[0].segment[0x0C + i] = (uint8_t)(i * 7 + 1);
      memcpy(ref, table[0].segment + 0x0C, 0x11D); rec.ptrs[0] = lo;                      /* source 0x0C, destination 0x10: backward */
      CHECK(pr_u6f_entry6(&ds, &env, OBJ, 1, &rep) == PR_U6F_E6_OK);
      CHECK(memcmp(table[0].segment + 0x10, ref, 0x2E) == 0);                              /* the head of the record before the rewritten fields */
      for (i = 0; i < 0x11D + 8; i++) table[0].segment[0x14 + i] = (uint8_t)(i * 5 + 2);
      memcpy(ref, table[0].segment + 0x14, 0x11D); rec.ptrs[0] = hi; rec.roll_i = 0; rec.blk_calls = 0; ds.b557c = 0; heap_next = 0;
      CHECK(pr_u6f_entry6(&ds, &env, OBJ, 1, &rep) == PR_U6F_E6_OK && memcmp(table[0].segment + 0x10, ref, 0x2E) == 0); }       /* forward */

    /* stops with earlier effects standing */
    setup(); env.hooks.str_af8_1366 = NULL;
    CHECK(pr_u6f_entry6(&ds, &env, OBJ, 1, &rep) == PR_U6F_E6_HOOK && rep.failed_call == PR_U6F_E6_CALL_STR && rec.n == 0);
    setup(); env.hooks.block_802_0716 = NULL;
    CHECK(pr_u6f_entry6(&ds, &env, OBJ, 1, &rep) == PR_U6F_E6_HOOK && rep.failed_call == PR_U6F_E6_CALL_BLOCK);
    setup(); env.hooks.roll_b0_0048 = NULL;
    CHECK(pr_u6f_entry6(&ds, &env, OBJ, 1, &rep) == PR_U6F_E6_HOOK && rep.failed_call == PR_U6F_E6_CALL_ROLL && bytes(OBJ)[0] == bytes(B1)[0] && ds.b557c == 0);
    setup(); env.hooks.free_af8_0254 = NULL;
    CHECK(pr_u6f_entry6(&ds, &env, OBJ, 1, &rep) == PR_U6F_E6_HOOK && rep.failed_call == PR_U6F_E6_CALL_FREE && rep.n_rolls == 5 && ds.b557c == 0);
    setup(); env.hooks.getmem_af8_023f = NULL;
    CHECK(pr_u6f_entry6(&ds, &env, OBJ, 1, &rep) == PR_U6F_E6_HOOK && rep.failed_call == PR_U6F_E6_CALL_GETMEM && ds.b557c == 1);
    setup(); env.hooks.flush_899_0104 = NULL;
    CHECK(pr_u6f_entry6(&ds, &env, OBJ, 1, &rep) == PR_U6F_E6_HOOK && rep.failed_call == PR_U6F_E6_CALL_FLUSH && ds.b557c == 0);
    setup(); rec.fail_kind = 5; rec.fail_nth = 3; rec.fail_val = -2;               /* the third roll fails: two stores done */
    CHECK(pr_u6f_entry6(&ds, &env, OBJ, 1, &rep) == PR_U6F_E6_CALLBACK && rep.callback_status == -2 && rep.n_rolls == 2 && bytes(OBJ)[0xC2] == (uint8_t)(((2 + 8) << 4) + 2) && bytes(OBJ)[0xC4] == bytes(B1)[0xC4] && rec.frees == 0);
    setup(); rec.fail_kind = 3; rec.fail_nth = 2; rec.fail_val = 9;                 /* the second 9-byte GetMem fails: the first unit stays chained */
    CHECK(pr_u6f_entry6(&ds, &env, OBJ, 1, &rep) == PR_U6F_E6_CALLBACK && rep.failed_call == PR_U6F_E6_CALL_GETMEM && rep.units9 == 1 && ptr_at(OBJ, 0x7F).seg == 0x3000 && rec.frees == 1);
    setup(); rec.fail_kind = 2; rec.fail_nth = 3; rec.fail_val = 4;                 /* block 3 reader fails after blocks 1 and 2 */
    CHECK(pr_u6f_entry6(&ds, &env, OBJ, 1, &rep) == PR_U6F_E6_CALLBACK && rep.failed_call == PR_U6F_E6_CALL_BLOCK && rep.units9 == 3 && ds.b557c == 1);
    setup(); rec.ptrs[0].seg = 0x2999;
    CHECK(pr_u6f_entry6(&ds, &env, OBJ, 1, &rep) == PR_U6F_E6_UNMAPPED && rep.bad_ptr.seg == 0x2999 && bytes(OBJ)[0] == 0xEE);
    setup(); rec.sizes[0] = 0x120; table[1].segment_size = 0x10 + 0x11C;           /* the block view is one byte short of the record */
    CHECK(pr_u6f_entry6(&ds, &env, OBJ, 1, &rep) == PR_U6F_E6_VIEW && bytes(OBJ)[0] == 0xEE);
    setup(); table[0].segment_size = 0x10 + 0x11C;                                  /* the record object cannot hold 0x11D bytes */
    CHECK(pr_u6f_entry6(&ds, &env, OBJ, 1, &rep) == PR_U6F_E6_VIEW);
    setup(); rec.sizes[1] = 0xFFFF;                                                 /* a size whose 16-bit offsets wrap before the end */
    CHECK(pr_u6f_entry6(&ds, &env, OBJ, 1, &rep) != PR_U6F_E6_OK);
    setup(); table[2].segment_size = 0x10 + 20 + 3;                                 /* block 2's last unit would read past its view (bytes 18..26) */
    CHECK(pr_u6f_entry6(&ds, &env, OBJ, 1, &rep) == PR_U6F_E6_VIEW && rep.units9 == 2);

    /* argument guards; NULL report accepted */
    setup();
    CHECK(pr_u6f_entry6(NULL, &env, OBJ, 1, &rep) == PR_U6F_E6_ARGUMENT && pr_u6f_entry6(&ds, NULL, OBJ, 1, &rep) == PR_U6F_E6_ARGUMENT);
    env.c0ec0 = NULL; CHECK(pr_u6f_entry6(&ds, &env, OBJ, 1, &rep) == PR_U6F_E6_ARGUMENT && rec.n == 0);
    env.c0ec0 = KC0; env.objects = NULL; CHECK(pr_u6f_entry6(&ds, &env, OBJ, 1, NULL) == PR_U6F_E6_ARGUMENT);
    env.objects = table; CHECK(pr_u6f_entry6(&ds, &env, OBJ, 1, NULL) == PR_U6F_E6_OK);
    puts("overlay_u6f_entry6_0ec4: ok");
    return 0;
}
