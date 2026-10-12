/* Synthetic test for the LOAD MONSTER +0x0537 tail model. Every object, link and the GetMem allocator are fabricated. Checked: the
 * party tail walk and A's link, the +0xBF slot byte and nil links, the copy loop's exit tests (n > count, the [0x4435] >= 0x3F cap), the
 * record clones, both sidecar chain copies (prepended, so reversed), the DS epilogue, the 16-bit wrap of object reads, and the
 * UNMAPPED/VIEW/CYCLE/HOOK/CALLBACK stops with the earlier effects standing. */
#include "poolrad/recovered/overlay_u1c_load_monster_tail_0537.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

#define NOBJ 64
#define OBJSZ 0x180
static uint8_t mem[NOBJ][OBJSZ];
static pr_u25_e5_object table[NOBJ];
static unsigned nobj, pool_next, pool_first;
static pr_u1c_lmt_env env;
static pr_u1c_lmt_in in;
static pr_u1c_lmt_ds ds;
static int fail_at;     /* fail the Nth GetMem call (1-based), 0 = never */
static unsigned calls;

static pr_far16 add_obj(uint16_t off, uint16_t seg)
{
    pr_far16 p; p.off = off; p.seg = seg;
    table[nobj].segment = mem[nobj]; table[nobj].segment_size = OBJSZ; table[nobj].offset = off; table[nobj].segment_id = seg;
    nobj++;
    return p;
}
static uint16_t w(pr_far16 p, uint16_t d) { unsigned i; for (i = 0; i < nobj; i++) if (table[i].offset == p.off && table[i].segment_id == p.seg) return (uint16_t)(mem[i][(uint16_t)(p.off + d)] | (mem[i][(uint16_t)(p.off + d + 1)] << 8)); return 0xDEAD; }
static uint8_t *bytes(pr_far16 p) { unsigned i; for (i = 0; i < nobj; i++) if (table[i].offset == p.off && table[i].segment_id == p.seg) return mem[i] + p.off; return NULL; }
static void setw(pr_far16 p, uint16_t d, uint16_t v) { uint8_t *b = bytes(p); b[d] = (uint8_t)v; b[d + 1] = (uint8_t)(v >> 8); }
static pr_far16 ptr_at(pr_far16 p, uint16_t d) { pr_far16 q; q.off = w(p, d); q.seg = w(p, (uint16_t)(d + 2)); return q; }
static void setp(pr_far16 p, uint16_t d, pr_far16 v) { setw(p, d, v.off); setw(p, (uint16_t)(d + 2), v.seg); }

static int h_getmem(void *c, const pr_u1c_lmt_env *e, uint16_t size, pr_far16 *res)
{
    (void)c; (void)e; (void)size;
    calls++;
    if (fail_at && calls == (unsigned)fail_at) return 5;
    res->off = table[pool_next].offset; res->seg = table[pool_next].segment_id;
    pool_next++;
    return 0;
}

static pr_far16 party[3], A, B1, B2, C1, C2;

static void setup(void)
{
    unsigned i;
    memset(mem, 0, sizeof mem); memset(table, 0, sizeof table); nobj = 0; calls = 0; fail_at = 0;
    for (i = 0; i < 3; i++) party[i] = add_obj(0x10, (uint16_t)(0x1000 + i));
    setp(party[0], 0x104, party[1]); setp(party[1], 0x104, party[2]);       /* party[2] is the tail (nil link) */
    A = add_obj(0x20, 0x2000);
    B1 = add_obj(0x30, 0x3001); B2 = add_obj(0x30, 0x3002);
    C1 = add_obj(0x40, 0x4001); C2 = add_obj(0x40, 0x4002);
    setp(B1, 0x2A, B2); setp(C1, 5, C2);
    for (i = 0; i < 0x3F; i++) { bytes(B1)[i] = (uint8_t)(0x10 + i); bytes(B2)[i] = (uint8_t)(0x90 + i); }
    bytes(B1)[0x2A] = 0; bytes(B1)[0x2B] = 0;
    setp(B1, 0x2A, B2); setp(B2, 0x2A, (pr_far16){ 0, 0 });
    for (i = 0; i < 9; i++) { bytes(C1)[i] = (uint8_t)(0x50 + i); bytes(C2)[i] = (uint8_t)(0xA0 + i); }
    setp(C1, 5, C2); setp(C2, 5, (pr_far16){ 0, 0 });
    pool_first = nobj; pool_next = nobj;
    for (i = 0; i < 24; i++) add_obj(0x08, (uint16_t)(0x5000 + i));          /* the heap: handed out in order by the hook */
    memset(&env, 0, sizeof env);
    env.objects = table; env.object_count = nobj; env.party_head = party[0]; env.hooks.getmem_af8_023f = h_getmem;
    memset(&in, 0, sizeof in);
    in.var_a = A; in.head_b = (pr_far16){ 0, 0 }; in.head_c = (pr_far16){ 0, 0 }; in.count = 1; in.n = 1; in.saved_w5d92 = 0xBEEF; in.saved_w5d94 = 0xCAFE;
    for (i = 0; i < PR_U1C_LMT_RECORD; i++) in.record_copy[i] = (uint8_t)(i * 3 + 1);
    memset(&ds, 0, sizeof ds); ds.b6f83 = 8;
}

int main(void)
{
    pr_u1c_lmt_report rep;
    pr_far16 p1, p2, m;
    unsigned i;
    CHECK(sizeof(mem[0]) >= 0x150);

    /* count = 1: A is linked at the party tail, no copies, DS epilogue */
    setup();
    CHECK(pr_u1c_load_monster_tail(&ds, &env, &in, &rep) == PR_U1C_LMT_OK);
    CHECK(rep.walked == 2 && rep.tail.seg == 0x1002 && rep.monsters == 1 && rep.copies == 0 && rep.getmem_calls == 0 && !rep.cap_stop);
    CHECK(ptr_at(party[2], 0x104).seg == 0x2000 && ptr_at(party[2], 0x104).off == 0x20);
    CHECK(bytes(A)[0xBF] == 8 && w(A, 0x104) == 0 && w(A, 0x106) == 0);
    CHECK(ds.b4435 == 1 && ds.b6f83 == 9 && ds.b84e7 == 1 && ds.w5d92 == 0xBEEF && ds.w5d94 == 0xCAFE);

    /* count = 3: two clones, each the local record copy with +0xBF, links and the two chain pairs rewritten */
    setup(); in.count = 3;
    CHECK(pr_u1c_load_monster_tail(&ds, &env, &in, &rep) == PR_U1C_LMT_OK && rep.copies == 2 && rep.monsters == 3 && ds.b4435 == 3 && rep.getmem_calls == 2);
    p1 = ptr_at(A, 0x104); p2 = ptr_at(p1, 0x104);
    CHECK(p1.seg == 0x5000 && p2.seg == 0x5001 && w(p2, 0x104) == 0 && w(p2, 0x106) == 0);
    CHECK(bytes(p1)[0xBF] == 8 && bytes(p2)[0xBF] == 8);
    CHECK(bytes(p1)[0] == in.record_copy[0] && bytes(p1)[0x11C] == in.record_copy[0x11C] && bytes(p1)[0xBE] == in.record_copy[0xBE] && bytes(p1)[0xC0] == in.record_copy[0xC0]);
    CHECK(w(p1, 0x7F) == 0 && w(p1, 0x81) == 0 && w(p1, 0xC8) == 0 && w(p1, 0xCA) == 0);
    CHECK(ds.b6f83 == 9);

    /* sidecar chains (count = 2, one clone): prepended copies, so reversed; the source chains are unchanged */
    setup(); in.count = 2; in.head_b = B1; in.head_c = C1;
    CHECK(pr_u1c_load_monster_tail(&ds, &env, &in, &rep) == PR_U1C_LMT_OK && rep.copies == 1 && rep.b_nodes == 2 && rep.c_nodes == 2 && rep.getmem_calls == 5);
    p1 = ptr_at(A, 0x104);
    m = ptr_at(p1, 0xC8);                                                   /* newest B copy = copy of B2 */
    CHECK(m.seg == 0x5002 && memcmp(bytes(m), bytes(B2), 0x2A) == 0 && memcmp(bytes(m) + 0x2E, bytes(B2) + 0x2E, 0x3F - 0x2E) == 0);
    CHECK(ptr_at(m, 0x2A).seg == 0x5001 && memcmp(bytes(ptr_at(m, 0x2A)), bytes(B1), 0x2A) == 0 && w(ptr_at(m, 0x2A), 0x2A) == 0 && w(ptr_at(m, 0x2A), 0x2C) == 0);
    m = ptr_at(p1, 0x7F);                                                   /* newest C copy = copy of C2 */
    CHECK(m.seg == 0x5004 && memcmp(bytes(m), bytes(C2), 5) == 0 && ptr_at(m, 5).seg == 0x5003 && memcmp(bytes(ptr_at(m, 5)), bytes(C1), 5) == 0 && w(ptr_at(m, 5), 5) == 0);
    CHECK(w(B1, 0x2A) == 0x30 && ptr_at(B1, 0x2A).seg == 0x3002);           /* the source chain is untouched */
    /* A itself keeps whatever chain words it had: the first monster is not cloned */
    CHECK(w(A, 0xC8) == 0 && w(A, 0x7F) == 0);
    /* a nil B head with a non-nil C head: only the C chain is copied */
    setup(); in.count = 2; in.head_c = C1;
    CHECK(pr_u1c_load_monster_tail(&ds, &env, &in, &rep) == PR_U1C_LMT_OK && rep.b_nodes == 0 && rep.c_nodes == 2 && w(ptr_at(A, 0x104), 0xC8) == 0);

    /* the [0x4435] cap: starting at 0x3E, A makes it 0x3F, so no clone even though count asks for more; the exit test order */
    setup(); in.count = 5; ds.b4435 = 0x3E;
    CHECK(pr_u1c_load_monster_tail(&ds, &env, &in, &rep) == PR_U1C_LMT_OK && rep.cap_stop == 1 && rep.copies == 0 && ds.b4435 == 0x3F && ds.b6f83 == 9);
    setup(); in.count = 5; ds.b4435 = 0x3C;                                  /* 0x3D after A, 0x3E and 0x3F after two clones, then the cap */
    CHECK(pr_u1c_load_monster_tail(&ds, &env, &in, &rep) == PR_U1C_LMT_OK && rep.copies == 2 && rep.cap_stop == 1 && ds.b4435 == 0x3F);
    /* n > count is an unsigned compare: count 0 exits after A */
    setup(); in.count = 0;
    CHECK(pr_u1c_load_monster_tail(&ds, &env, &in, &rep) == PR_U1C_LMT_OK && rep.copies == 0);
    setup(); in.count = 0xFF; in.n = 0xFE; ds.b4435 = 0x3C;                  /* n = 0xFF after A is not > 0xFF, so clones run (n wraps to 0) until the cap */
    CHECK(pr_u1c_load_monster_tail(&ds, &env, &in, &rep) == PR_U1C_LMT_OK && rep.copies == 2 && rep.cap_stop == 1);
    /* the slot byte wraps at the end */
    setup(); ds.b6f83 = 0xFF;
    CHECK(pr_u1c_load_monster_tail(&ds, &env, &in, &rep) == PR_U1C_LMT_OK && ds.b6f83 == 0 && bytes(A)[0xBF] == 0xFF);

    /* 16-bit wrap: a party node at offset 0xFF00 reads its link at 0x0004 (0xFF00 + 0x104 wraps) */
    setup();
    { static uint8_t big[0x10000]; pr_far16 q = { 0xFF00, 0x7000 };
      memset(big, 0, sizeof big);
      table[nobj].segment = big; table[nobj].segment_size = sizeof big; table[nobj].offset = 0xFF00; table[nobj].segment_id = 0x7000; nobj++;
      env.object_count = nobj; env.party_head = q;
      CHECK(pr_u1c_load_monster_tail(&ds, &env, &in, &rep) == PR_U1C_LMT_OK && rep.walked == 0 && big[0x0004] == 0x20 && big[0x0006] == 0x00 && big[0x0005] == 0 && big[0x0007] == 0x20); }

    /* stops */
    setup(); env.party_head.off = 0; env.party_head.seg = 0;                 /* nil party head */
    CHECK(pr_u1c_load_monster_tail(&ds, &env, &in, &rep) == PR_U1C_LMT_UNMAPPED && rep.bad_ptr.off == 0 && ds.b4435 == 0);
    setup(); table[0].segment_size = 0x10 + 0x105;                           /* the first party node cannot hold its link */
    CHECK(pr_u1c_load_monster_tail(&ds, &env, &in, &rep) == PR_U1C_LMT_VIEW && ds.b4435 == 0 && ds.b84e7 == 0);
    setup(); setp(party[2], 0x104, party[0]);                                /* a cyclic party list */
    CHECK(pr_u1c_load_monster_tail(&ds, &env, &in, &rep) == PR_U1C_LMT_CYCLE && ds.b4435 == 0);
    setup(); in.var_a.seg = 0x2999;                                          /* A not in the table: the tail was already linked to it */
    CHECK(pr_u1c_load_monster_tail(&ds, &env, &in, &rep) == PR_U1C_LMT_UNMAPPED && ptr_at(party[2], 0x104).seg == 0x2999 && ds.b4435 == 0);
    setup(); in.count = 4; env.hooks.getmem_af8_023f = NULL;
    CHECK(pr_u1c_load_monster_tail(&ds, &env, &in, &rep) == PR_U1C_LMT_HOOK && ds.b4435 == 1 && bytes(A)[0xBF] == 8 && ds.b84e7 == 0);
    setup(); in.count = 4; fail_at = 2;                                      /* the second clone's GetMem fails: the first clone stays linked */
    CHECK(pr_u1c_load_monster_tail(&ds, &env, &in, &rep) == PR_U1C_LMT_CALLBACK && rep.callback_status == 5 && rep.copies == 1 && ds.b4435 == 2 && ptr_at(A, 0x104).seg == 0x5000 && w(ptr_at(A, 0x104), 0x104) == 0 && ds.b6f83 == 8);
    setup(); in.count = 2; in.head_b = B1; fail_at = 3;                      /* a sidecar GetMem fails mid-chain: the first node copy stays */
    CHECK(pr_u1c_load_monster_tail(&ds, &env, &in, &rep) == PR_U1C_LMT_CALLBACK && rep.b_nodes == 1 && ptr_at(ptr_at(A, 0x104), 0xC8).seg == 0x5001);
    setup(); in.count = 2; in.head_b = B1; setp(B2, 0x2A, B1);               /* a cyclic sidecar chain */
    CHECK(pr_u1c_load_monster_tail(&ds, &env, &in, &rep) != PR_U1C_LMT_OK);
    for (i = 0; i < 1; i++) { setup(); in.count = 2; in.head_b.seg = 0x3999; in.head_b.off = 0x30;   /* sidecar head not in the table */
      CHECK(pr_u1c_load_monster_tail(&ds, &env, &in, &rep) == PR_U1C_LMT_UNMAPPED); }

    /* argument guards; NULL report accepted */
    setup();
    CHECK(pr_u1c_load_monster_tail(NULL, &env, &in, &rep) == PR_U1C_LMT_ARGUMENT && pr_u1c_load_monster_tail(&ds, NULL, &in, &rep) == PR_U1C_LMT_ARGUMENT && pr_u1c_load_monster_tail(&ds, &env, NULL, &rep) == PR_U1C_LMT_ARGUMENT);
    env.objects = NULL;
    CHECK(pr_u1c_load_monster_tail(&ds, &env, &in, NULL) == PR_U1C_LMT_ARGUMENT);
    puts("overlay_u1c_load_monster_tail_0537: ok");
    return 0;
}
