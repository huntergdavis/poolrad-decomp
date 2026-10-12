/* Synthetic test for the unit 0x6f entry 6 -> pr_rec_res_0802_0716 adapter. The three archives, the constants and every other hook are
 * fabricated; the reader is the merged model. Checked: end to end through pr_u6f_entry6 (record, chains, exact release-once), the
 * reader status mapping (MISSING_FILE/ID -> zero outputs, PACKED_LIMIT_ERROR and host statuses -> distinct nonzero), borrowed exact-size views
 * (a unit that would read past the size is VIEW), teardown after a stop before FreeMem, double/mismatched/unknown release, slot exhaustion,
 * Pascal string conversion and the init guards. */
#include "poolrad/recovered/overlay_u6f_reader_adapter.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

#define NOBJ 48
#define OBJSZ 0x200
#define POOL 24
static uint8_t mem[POOL + 1][OBJSZ];
static pr_u25_e5_object table[NOBJ];
static pr_u6f_e6_env env;
static pr_u6f_e6_ds ds;
static pr_far16 OBJ;

static const uint8_t K98[] = { 3, 'a', 'b', 'c' }, K9C[] = { 3, 'x', 'y', 'z' }, KA0[] = { 4, 'e', 'x', 't', '!' }, KBC[] = { 3, 'q', 'r', 's' }, KC0[] = { 3, 'u', 'v', 'w' };

typedef struct {
    pr_u6f_ra ra;                      /* first member: env.ctx points here */
    pr_rec_res_ecl_dax_io io;
    uint8_t file[3][160]; size_t file_size[3];
    int missing[3];                    /* archive n is not found */
    int fail_alloc, allocations, frees; size_t free_sizes[16];
    char seen_name[3][32], seen_ext[3][32]; unsigned opens;
    unsigned roll_i; int roll_fail, flush_calls;
    unsigned heap_next;
} Fx;
static Fx fx;

static int which(const char *name)       /* the archive a name selects: suffix of the constants */
{
    size_t n = strlen(name);
    if (n >= 3 && memcmp(name + n - 3, "xyz", 3) == 0) return 0;
    if (n >= 3 && memcmp(name + n - 3, "qrs", 3) == 0) return 1;
    return 2;
}
static int open_archive(void *ctx, const char *name, const char *ext, const uint8_t **data, size_t *size)
{
    Fx *f = ctx; int k = which(name);
    if (f->opens < 3) { strcpy(f->seen_name[f->opens], name); strcpy(f->seen_ext[f->opens], ext); }
    f->opens++;
    if (f->missing[k]) return 1;
    *data = f->file[k]; *size = f->file_size[k];
    return 0;
}
static void close_archive(void *ctx) { (void)ctx; }
static void *alloc_(void *ctx, size_t size) { Fx *f = ctx; f->allocations++; return f->allocations == f->fail_alloc ? NULL : malloc(size); }
static void dealloc_(void *ctx, void *p, size_t size) { Fx *f = ctx; if (f->frees < 16) f->free_sizes[f->frees] = size; f->frees++; free(p); }

/* one-row archive: header word 9, row (id, offset 0, unpacked, packed), then the packed bytes at file + 11 */
static void make_archive(int k, uint8_t id, unsigned unpacked, unsigned packed_size, const uint8_t *packed)
{
    uint8_t *p = fx.file[k];
    memset(p, 0, 160);
    p[0] = 9; p[2] = id; p[7] = (uint8_t)unpacked; p[8] = (uint8_t)(unpacked >> 8); p[9] = (uint8_t)packed_size; p[10] = (uint8_t)(packed_size >> 8);
    if (packed) memcpy(p + 11, packed, packed_size);
    fx.file_size[k] = 11 + packed_size;
}

static int h_str(void *c, const pr_u6f_e6_env *e, uint32_t v, uint8_t out[2]) { (void)c; (void)e; (void)v; out[0] = 1; out[1] = '7'; return 0; }
static int h_gm(void *c, const pr_u6f_e6_env *e, uint16_t size, pr_far16 *res)
{ Fx *f = (Fx *)c; (void)e; (void)size; res->off = table[f->heap_next].offset; res->seg = table[f->heap_next].segment_id; f->heap_next++; return 0; }
static int h_roll(void *c, const pr_u6f_e6_env *e, uint8_t cnt, uint8_t rng, uint8_t *al)
{ Fx *f = (Fx *)c; (void)e; (void)cnt; (void)rng; *al = (uint8_t)(++f->roll_i); return f->roll_fail; }
static int h_flush(void *c, const pr_u6f_e6_env *e) { Fx *f = (Fx *)c; (void)e; f->flush_calls++; return 0; }

static const uint8_t RLE_285[] = { 0x81, 0x41, 0x81, 0x42, 0xE1, 0x43 };    /* 127 x 'A', 127 x 'B', 31 x 'C' = 285 (0x11D) */
static const uint8_t RLE_18[] = { 0xEE, 0x5A };                              /* 18 x 'Z' */

static void setup(void)
{
    unsigned i;
    memset(mem, 0, sizeof mem); memset(table, 0, sizeof table); memset(&fx, 0, sizeof fx);
    table[0].segment = mem[0]; table[0].segment_size = OBJSZ; table[0].offset = 0x10; table[0].segment_id = 0x1000;
    OBJ.off = 0x10; OBJ.seg = 0x1000;
    for (i = 0; i < POOL; i++) { table[1 + i].segment = mem[1 + i]; table[1 + i].segment_size = OBJSZ; table[1 + i].offset = 0x08; table[1 + i].segment_id = (uint16_t)(0x3000 + i); }
    fx.heap_next = 1;
    fx.io.ctx = &fx; fx.io.open_archive = open_archive; fx.io.close_archive = close_archive; fx.io.allocate = alloc_; fx.io.deallocate = dealloc_;
    pr_u6f_ra_init(&fx.ra, &fx.io, &table[1 + POOL], 4, 0x7000);
    fx.ra.user = &fx;
    make_archive(0, 5, 0x11D, sizeof RLE_285, RLE_285);
    make_archive(1, 5, 18, sizeof RLE_18, RLE_18);
    { static const uint8_t r126[] = { 0x82, 0x6D };                              /* 126 x 'm' = 0x7E = two 0x3F units */
      make_archive(2, 5, 126, sizeof r126, r126); }
    memset(&env, 0, sizeof env);
    env.objects = table; env.object_count = 1 + POOL + 4;
    env.c0e98 = K98; env.c0e9c = K9C; env.c0ea0 = KA0; env.c0ebc = KBC; env.c0ec0 = KC0; env.b5376 = 7;
    env.hooks.str_af8_1366 = h_str; env.hooks.block_802_0716 = pr_u6f_ra_block_hook; env.hooks.getmem_af8_023f = h_gm;
    env.hooks.free_af8_0254 = pr_u6f_ra_free_hook; env.hooks.roll_b0_0048 = h_roll; env.hooks.flush_899_0104 = h_flush;
    env.ctx = &fx;                                                             /* the adapter is fx's first member, so fx.ra's address is env.ctx */
    memset(&ds, 0, sizeof ds);
}

int main(void)
{
    pr_u6f_e6_report rep;
    unsigned i;

    /* end to end: three archives through the real reader, the borrowed views, three exact-size releases */
    setup();
    CHECK(pr_u6f_entry6(&ds, &env, OBJ, 5, &rep) == PR_U6F_E6_OK);
    CHECK(fx.opens == 3 && strcmp(fx.seen_name[0], "abc7xyz") == 0 && strcmp(fx.seen_name[1], "abc7qrs") == 0 && strcmp(fx.seen_name[2], "abc7uvw") == 0 && strcmp(fx.seen_ext[0], "ext!") == 0);
    CHECK(rep.size[0] == 0x11D && rep.size[1] == 18 && rep.size[2] == 126 && rep.units9 == 2 && rep.units3f == 2);
    CHECK(mem[0][0x10] == 'A' && mem[0][0x10 + 126] == 'A' && mem[0][0x10 + 140] == 'B' && mem[0][0x10 + 0x11C] == 'C');
    CHECK(fx.ra.loads == 3 && fx.ra.releases == 3 && fx.ra.teardown_releases == 0 && pr_u6f_ra_teardown(&fx.ra) == 0);
    /* reader allocations: a packed temporary (rounded to 8) and the output per load; the output is freed exactly once per block */
    CHECK(fx.allocations == 6 && fx.frees == 6);
    for (i = 0; i < 4; i++) CHECK(table[1 + POOL + i].segment == NULL && table[1 + POOL + i].segment_size == 0 && fx.ra.slots[i].live == 0);

    /* a unit that would read past the returned size is VIEW (no padding): block 2 of 20 bytes, last unit reads 18..26 */
    setup(); { static const uint8_t r20[] = { 0xEC, 0x5A }; make_archive(1, 5, 20, sizeof r20, r20); }
    CHECK(pr_u6f_entry6(&ds, &env, OBJ, 5, &rep) == PR_U6F_E6_VIEW && rep.units9 == 2);
    CHECK(fx.ra.releases == 1 && fx.ra.slots[0].live == 1 && pr_u6f_ra_teardown(&fx.ra) == 1 && fx.ra.teardown_releases == 1 && fx.ra.slots[0].released == 1 && pr_u6f_ra_teardown(&fx.ra) == 0);
    CHECK(fx.frees == 4);                                                      /* block 1: packed temporary + released output; block 2: packed temporary + torn-down output */

    /* MISSING_FILE on block 1: the zero-size path (ERROR_EXIT), nothing live, nothing to release */
    setup(); fx.missing[0] = 1;
    CHECK(pr_u6f_entry6(&ds, &env, OBJ, 5, &rep) == PR_U6F_E6_ERROR_EXIT && fx.ra.loads == 1 && pr_u6f_ra_teardown(&fx.ra) == 0 && fx.frees == 0);
    /* MISSING_FILE and MISSING_ID on blocks 2 and 3: skipped */
    setup(); fx.missing[1] = 1; make_archive(2, 9, 126, 2, (const uint8_t[]){ 0x82, 0x6D });   /* id 9 only: id 5 is missing */
    CHECK(pr_u6f_entry6(&ds, &env, OBJ, 5, &rep) == PR_U6F_E6_OK && rep.size[1] == 0 && rep.size[2] == 0 && rep.units9 == 0 && rep.units3f == 0 && fx.ra.releases == 1);

    /* PACKED_LIMIT_ERROR: a distinct nonzero code surfaced as the callback status; nothing live */
    setup(); make_archive(0, 5, 0x11D, 0x8CA1, NULL); fx.file_size[0] = 11;
    CHECK(pr_u6f_entry6(&ds, &env, OBJ, 5, &rep) == PR_U6F_E6_CALLBACK && rep.failed_call == PR_U6F_E6_CALL_BLOCK && rep.callback_status == PR_U6F_RA_STOP_PACKED_LIMIT);
    CHECK(fx.ra.loads == 1 && pr_u6f_ra_teardown(&fx.ra) == 0);
    /* a host-only status stays distinct: allocation failure in the reader */
    setup(); fx.fail_alloc = 1;
    CHECK(pr_u6f_entry6(&ds, &env, OBJ, 5, &rep) == PR_U6F_E6_CALLBACK && rep.callback_status == PR_U6F_RA_STOP_HOST_BASE + PR_REC_RES_ECL_DAX_ALLOC && fx.ra.last_reader_status == PR_REC_RES_ECL_DAX_ALLOC);
    CHECK(rep.callback_status != PR_U6F_RA_STOP_PACKED_LIMIT);

    /* stop before FreeMem (a failing roll after block 1 loaded): the block is still live; teardown releases it once */
    setup(); fx.roll_fail = 7;
    CHECK(pr_u6f_entry6(&ds, &env, OBJ, 5, &rep) == PR_U6F_E6_CALLBACK && rep.failed_call == PR_U6F_E6_CALL_ROLL && fx.ra.slots[0].live == 1 && fx.ra.releases == 0);
    CHECK(table[1 + POOL].segment != NULL && table[1 + POOL].segment_size == 0x11D);
    CHECK(pr_u6f_ra_teardown(&fx.ra) == 1 && fx.ra.teardown_releases == 1 && fx.ra.releases == 0 && table[1 + POOL].segment == NULL && pr_u6f_ra_teardown(&fx.ra) == 0);

    /* release contract on its own: once, exact size, unknown key, stale key */
    setup();
    { uint16_t size = 0; pr_far16 key; static const uint8_t pname[] = { 7, 'a','b','c','7','x','y','z' };
      CHECK(pr_u6f_ra_block_hook(&fx.ra, &env, pname, KA0, 5, &size, &key) == PR_U6F_RA_OK && size == 0x11D && key.seg == 0x7000 && key.off == 0);
      CHECK(pr_u6f_ra_free_hook(&fx.ra, &env, key, 0x11C) == PR_U6F_RA_STOP_FREE_MISMATCH && fx.ra.slots[0].live == 1 && fx.frees == 1);      /* wrong size: nothing released */
      { pr_far16 other = { 0, 0x7002 }; CHECK(pr_u6f_ra_free_hook(&fx.ra, &env, other, 0x11D) == PR_U6F_RA_STOP_FREE_MISMATCH); }          /* a slot that is not live */
      { pr_far16 unk = { 0, 0x1234 }; CHECK(pr_u6f_ra_free_hook(&fx.ra, &env, unk, 0x11D) == PR_U6F_RA_STOP_FREE_MISMATCH); }
      CHECK(pr_u6f_ra_free_hook(&fx.ra, &env, key, 0x11D) == PR_U6F_RA_OK && fx.frees == 2 && fx.ra.releases == 1);
      CHECK(pr_u6f_ra_free_hook(&fx.ra, &env, key, 0x11D) == PR_U6F_RA_STOP_FREE_MISMATCH && fx.frees == 2);                                  /* double release */
      CHECK(table[1 + POOL].segment == NULL && table[1 + POOL].segment_size == 0); }

    /* slot exhaustion before the reader runs; Pascal string conversion */
    setup();
    { uint16_t size; pr_far16 key; unsigned n; static const uint8_t pname[] = { 7, 'a','b','c','7','x','y','z' };
      for (n = 0; n < 4; n++) CHECK(pr_u6f_ra_block_hook(&fx.ra, &env, pname, KA0, 5, &size, &key) == PR_U6F_RA_OK && key.seg == 0x7000 + n);
      CHECK(pr_u6f_ra_block_hook(&fx.ra, &env, pname, KA0, 5, &size, &key) == PR_U6F_RA_STOP_NO_SLOT && fx.ra.loads == 4 && size == 0 && key.seg == 0);
      CHECK(pr_u6f_ra_teardown(&fx.ra) == 4); }
    setup();
    { uint16_t size = 7; pr_far16 key = { 1, 1 }; static const uint8_t bad[] = { 3, 'a', 0, 'b' };
      CHECK(pr_u6f_ra_block_hook(&fx.ra, &env, bad, KA0, 5, &size, &key) == PR_U6F_RA_STOP_STRING && fx.ra.loads == 0 && size == 0 && key.seg == 0);
      CHECK(pr_u6f_ra_block_hook(&fx.ra, &env, NULL, KA0, 5, &size, &key) == PR_U6F_RA_STOP_STRING);
      { uint8_t longname[1 + 25]; memset(longname + 1, 'N', 25); longname[0] = 25; fx.missing[2] = 1;
        CHECK(pr_u6f_ra_block_hook(&fx.ra, &env, longname, KA0, 5, &size, &key) == PR_U6F_RA_OK && size == 0 && strlen(fx.seen_name[0]) == 20); } }  /* the reader truncates to 20; missing -> zero outputs */

    /* init guards */
    setup();
    { pr_u6f_ra r;
      CHECK(pr_u6f_ra_init(NULL, &fx.io, table, 1, 1) != 0 && pr_u6f_ra_init(&r, NULL, table, 1, 1) != 0 && pr_u6f_ra_init(&r, &fx.io, NULL, 1, 1) != 0);
      CHECK(pr_u6f_ra_init(&r, &fx.io, table, 0, 1) != 0 && pr_u6f_ra_init(&r, &fx.io, table, PR_U6F_RA_MAX_SLOTS + 1, 1) != 0 && pr_u6f_ra_init(&r, &fx.io, table, 1, 0) != 0);
      CHECK(pr_u6f_ra_init(&r, &fx.io, table, 2, 0xFFFF) == PR_U6F_RA_STOP_ARGUMENT);
      CHECK(pr_u6f_ra_block_hook(NULL, &env, KA0, KA0, 5, &(uint16_t){ 0 }, &(pr_far16){ 0, 0 }) == PR_U6F_RA_STOP_ARGUMENT); }
    setup();
    { uint16_t size = 7; pr_far16 key = { 1, 1 };
      static const uint8_t pname[] = { 7, 'a','b','c','7','x','y','z' };
      table[0].offset = 0; table[0].segment_id = 0x7000; /* shadows first reserved key */
      CHECK(pr_u6f_ra_block_hook(&fx.ra, &env, pname, KA0, 5, &size, &key) == PR_U6F_RA_STOP_ARGUMENT);
      CHECK(size == 0 && key.off == 0 && key.seg == 0 && fx.ra.loads == 0 && fx.opens == 0);
      table[0].segment_id = 0x1000; table[0].offset = 0x10;
      CHECK(pr_u6f_ra_block_hook(&fx.ra, NULL, pname, KA0, 5, &size, &key) == PR_U6F_RA_STOP_ARGUMENT);
      CHECK(fx.ra.loads == 0 && fx.opens == 0); }
    puts("overlay_u6f_reader_adapter: ok");
    return 0;
}
