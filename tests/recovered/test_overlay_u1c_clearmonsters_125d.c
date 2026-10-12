/* Synthetic test for the unit 0x1c +0x125D CLEARMONSTERS handler model. The node bytes, the far pointers and the free hook are
 * fabricated: every node's link is a real little-endian offset/segment word pair at node+0x2A/+0x2C, decoded by the model before
 * the free, and mapped to a node through the host's table (segment identity + offset); the hook records the nodes it is given and
 * touches no heap. Checked: the free order follows the raw links (not the table order), zero:zero is nil and a single zero word is
 * not, an unmapped non-nil pointer stops the loop after the earlier frees, the head words are replaced only after a successful free,
 * the zero fill of the 28-byte DS block, the DS bytes, the PC advance, the 16-bit wrap of the link byte addresses, and the guards. */
#include "poolrad/recovered/overlay_u1c_clearmonsters_125d.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

#define MAXN 6
static pr_rec_ecl_loader_state st;
static uint8_t seg[MAXN][0x100];
static pr_u1c_cmon_node tab[MAXN];

typedef struct { const pr_u1c_cmon_node *seen[8]; uint16_t size[8]; unsigned n; unsigned fail_at; } Rec;

static int h_free(void *c, pr_u1c_cmon_env *e, const pr_u1c_cmon_node *nd, uint16_t size)
{
    Rec *r = c; (void)e;
    if (r->n < 8) { r->seen[r->n] = nd; r->size[r->n] = size; }
    ++r->n;
    return r->n == r->fail_at ? 9 : 0;
}

/* table entry i is far pointer (0x2000 + i : 0x0010 + 0x20 * i); the raw links are written separately */
static uint16_t soff(unsigned i) { return (uint16_t)(0x0010u + 0x20u * i); }
static uint16_t sseg(unsigned i) { return (uint16_t)(0x2000u + i); }

static void put_link(unsigned i, uint16_t off, uint16_t sg)
{
    seg[i][(uint16_t)(soff(i) + 0x2A)] = (uint8_t)(off & 0xFF); seg[i][(uint16_t)(soff(i) + 0x2B)] = (uint8_t)(off >> 8);
    seg[i][(uint16_t)(soff(i) + 0x2C)] = (uint8_t)(sg & 0xFF);  seg[i][(uint16_t)(soff(i) + 0x2D)] = (uint8_t)(sg >> 8);
}

/* nodes 0..len-1 linked in table order by real encoded links; the last link is 0:0 */
static void make_list(unsigned len)
{
    unsigned i;
    for (i = 0; i < MAXN; i++) {
        memset(seg[i], 0x5A, sizeof seg[i]);                  /* garbage that must never be followed */
        tab[i].segment = seg[i]; tab[i].segment_size = sizeof seg[i]; tab[i].segment_id = sseg(i); tab[i].offset = soff(i);
    }
    for (i = 0; i < len; i++) {
        if (i + 1 < len) put_link(i, soff(i + 1), sseg(i + 1)); else put_link(i, 0, 0);
    }
}

static void env_init(pr_u1c_cmon_env *e, Rec *r, unsigned len)
{
    memset(e, 0, sizeof *e); memset(r, 0, sizeof *r);
    memset(e->block_67f4, 0xAA, sizeof e->block_67f4);
    e->b4435 = 0x11; e->b84e7 = 0x22; e->b6f83 = 0x33;
    e->head_off = len ? soff(0) : 0; e->head_seg = len ? sseg(0) : 0;
    e->nodes = tab; e->node_count = MAXN;
    e->hooks.free_af8_0254 = h_free; e->ctx = r;
}

int main(void)
{
    pr_u1c_cmon_env env; Rec rec; pr_u1c_cmon_report rep;
    unsigned len, i;

    for (len = 0; len <= MAXN; len++) {
        make_list(len); env_init(&env, &rec, len);
        memset(&st, 0, sizeof st); st.pc = 0xB1AFu;
        CHECK(pr_u1c_op_clearmonsters(&st, &env, &rep) == PR_U1C_CMON_OK && rep.freed == len && rec.n == len && env.head_off == 0 && env.head_seg == 0);
        for (i = 0; i < len; i++) CHECK(rec.seen[i] == &tab[i] && rec.size[i] == 0x3F);
        for (i = 0; i < sizeof env.block_67f4; i++) CHECK(env.block_67f4[i] == 0);
        CHECK(env.b4435 == 0 && env.b84e7 == 0 && env.b6f83 == 8 && st.pc == 0xB1B0u);
    }
    /* the PC advance wraps at 16 bits */
    make_list(0); env_init(&env, &rec, 0); memset(&st, 0, sizeof st); st.pc = 0xFFFFu;
    CHECK(pr_u1c_op_clearmonsters(&st, &env, NULL) == PR_U1C_CMON_OK && st.pc == 0);

    /* the raw links, not the table order, decide the free order: head -> node 3 -> node 1 -> node 4 -> nil */
    make_list(5);
    put_link(3, soff(1), sseg(1)); put_link(1, soff(4), sseg(4)); put_link(4, 0, 0);
    env_init(&env, &rec, 5); env.head_off = soff(3); env.head_seg = sseg(3); memset(&st, 0, sizeof st);
    CHECK(pr_u1c_op_clearmonsters(&st, &env, &rep) == PR_U1C_CMON_OK && rep.freed == 3);
    CHECK(rec.seen[0] == &tab[3] && rec.seen[1] == &tab[1] && rec.seen[2] == &tab[4]);

    /* a deliberately inconsistent mapping: node 0's raw link names a far pointer that no table node has -> UNMAPPED after the first free;
       the head pair holds the raw link, not nil */
    make_list(3); put_link(0, 0x7777, sseg(1));                                     /* right segment identity, wrong offset */
    env_init(&env, &rec, 3); memset(&st, 0, sizeof st);
    CHECK(pr_u1c_op_clearmonsters(&st, &env, &rep) == PR_U1C_CMON_UNMAPPED && rep.freed == 1 && rec.n == 1 && env.head_off == 0x7777 && env.head_seg == sseg(1));
    make_list(3); put_link(0, soff(1), 0x2999);                                     /* right offset, wrong segment */
    env_init(&env, &rec, 3); memset(&st, 0, sizeof st);
    CHECK(pr_u1c_op_clearmonsters(&st, &env, &rep) == PR_U1C_CMON_UNMAPPED && rep.freed == 1 && env.head_seg == 0x2999);
    /* one zero word is not nil: 0:segment and offset:0 need a table entry */
    make_list(3); put_link(0, 0, sseg(1));
    env_init(&env, &rec, 3); memset(&st, 0, sizeof st);
    CHECK(pr_u1c_op_clearmonsters(&st, &env, &rep) == PR_U1C_CMON_UNMAPPED && rep.freed == 1 && env.head_off == 0 && env.head_seg == sseg(1));
    make_list(3); put_link(0, soff(1), 0);
    env_init(&env, &rec, 3); memset(&st, 0, sizeof st);
    CHECK(pr_u1c_op_clearmonsters(&st, &env, &rep) == PR_U1C_CMON_UNMAPPED && rep.freed == 1 && env.head_off == soff(1) && env.head_seg == 0);
    /* an unmapped head at the start: nothing is freed, nothing is touched but the DS bytes, the fill and the PC */
    make_list(2); env_init(&env, &rec, 2); env.head_off = 0x1234; env.head_seg = 0x5678; memset(&st, 0, sizeof st);
    CHECK(pr_u1c_op_clearmonsters(&st, &env, &rep) == PR_U1C_CMON_UNMAPPED && rep.freed == 0 && env.head_off == 0x1234 && env.b6f83 == 8 && st.pc == 1);

    /* a failing free: earlier frees stand, the head pair stays at the failing node, the fill and DS bytes were already done */
    make_list(4); env_init(&env, &rec, 4); rec.fail_at = 3; memset(&st, 0, sizeof st); st.pc = 7;
    CHECK(pr_u1c_op_clearmonsters(&st, &env, &rep) == PR_U1C_CMON_CALLBACK && rep.callback_status == 9 && rep.freed == 2 && rec.n == 3);
    CHECK(env.head_off == soff(2) && env.head_seg == sseg(2) && env.block_67f4[0] == 0 && env.b6f83 == 8 && st.pc == 8);
    /* a missing hook with a nonempty list: HOOK at the head; with an empty list it is not needed */
    make_list(2); env_init(&env, &rec, 2); env.hooks.free_af8_0254 = NULL; memset(&st, 0, sizeof st);
    CHECK(pr_u1c_op_clearmonsters(&st, &env, &rep) == PR_U1C_CMON_HOOK && rep.freed == 0 && env.head_off == soff(0) && env.block_67f4[27] == 0);
    make_list(0); env_init(&env, &rec, 0); env.hooks.free_af8_0254 = NULL; memset(&st, 0, sizeof st);
    CHECK(pr_u1c_op_clearmonsters(&st, &env, &rep) == PR_U1C_CMON_OK);

    /* the node bytes are not modified */
    make_list(3); { uint8_t copy[0x100]; memcpy(copy, seg[1], sizeof copy); env_init(&env, &rec, 3); memset(&st, 0, sizeof st);
      CHECK(pr_u1c_op_clearmonsters(&st, &env, NULL) == PR_U1C_CMON_OK && memcmp(copy, seg[1], sizeof copy) == 0); }

    /* guards: a view too small for the four link bytes; a null view; a cycle (the cap) */
    make_list(3); tab[1].segment_size = (size_t)soff(1) + 0x2D; env_init(&env, &rec, 3); memset(&st, 0, sizeof st);        /* +0x2D is outside */
    CHECK(pr_u1c_op_clearmonsters(&st, &env, &rep) == PR_U1C_CMON_LIST && rep.freed == 1 && env.head_off == soff(1));
    make_list(3); tab[1].segment = NULL; env_init(&env, &rec, 3); memset(&st, 0, sizeof st);
    CHECK(pr_u1c_op_clearmonsters(&st, &env, &rep) == PR_U1C_CMON_LIST && rep.freed == 1);
    make_list(2); put_link(1, soff(0), sseg(0)); env_init(&env, &rec, 2); memset(&st, 0, sizeof st);                    /* 0 -> 1 -> 0 -> ... */
    CHECK(pr_u1c_op_clearmonsters(&st, &env, &rep) == PR_U1C_CMON_LIST && rep.freed == PR_U1C_CMON_LIST_CAP);

    /* 16-bit wrap of the link byte addresses: a node at far offset 0xFFF0 in a 64 KiB view: +0x2A..+0x2D wrap to 0x001A..0x001D */
    {
        static uint8_t big[0x10000]; pr_u1c_cmon_node w[2];
        memset(big, 0x5A, sizeof big);
        w[0].segment = big; w[0].segment_size = sizeof big; w[0].segment_id = 0x3000; w[0].offset = 0xFFF0;
        w[1].segment = seg[0]; w[1].segment_size = sizeof seg[0]; w[1].segment_id = 0x3001; w[1].offset = 0x0010;
        big[0x001A] = 0x10; big[0x001B] = 0x00; big[0x001C] = 0x01; big[0x001D] = 0x30;            /* link -> 3001:0010 */
        memset(seg[0], 0, sizeof seg[0]);                                                              /* node 1's link is 0:0 (bytes at 0x3A..0x3D) */
        make_list(0); memset(seg[0], 0, sizeof seg[0]);
        env_init(&env, &rec, 0); env.head_off = 0xFFF0; env.head_seg = 0x3000; env.nodes = w; env.node_count = 2; memset(&st, 0, sizeof st);
        CHECK(pr_u1c_op_clearmonsters(&st, &env, &rep) == PR_U1C_CMON_OK && rep.freed == 2 && rec.seen[0] == &w[0] && rec.seen[1] == &w[1]);
    }

    CHECK(pr_u1c_op_clearmonsters(NULL, &env, &rep) == PR_U1C_CMON_ARGUMENT && pr_u1c_op_clearmonsters(&st, NULL, &rep) == PR_U1C_CMON_ARGUMENT);
    puts("overlay_u1c_clearmonsters_125d: ok");
    return 0;
}
