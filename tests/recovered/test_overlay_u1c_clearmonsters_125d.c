/* Synthetic test for the unit 0x1c +0x125D CLEARMONSTERS handler model. The monster list, node views and the free hook are
 * fabricated: the hook records the nodes it is given and does not touch any heap. Lists of 0..6 nodes are checked for the free
 * order, the size argument, the head left nil, the zero fill of the 28-byte DS block (and nothing beyond it), the three DS bytes,
 * the PC advance, and the retained effects at a failing or missing hook, a bad link view and a cycle. */
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
static pr_u1c_cmon_node node[MAXN];

typedef struct { const pr_u1c_cmon_node *seen[8]; uint16_t size[8]; unsigned n; unsigned fail_at; } Rec;

static int h_free(void *c, pr_u1c_cmon_env *e, const pr_u1c_cmon_node *nd, uint16_t size)
{
    Rec *r = c; (void)e;
    if (r->n < 8) { r->seen[r->n] = nd; r->size[r->n] = size; }
    ++r->n;
    return r->n == r->fail_at ? 9 : 0;
}

static void make_list(unsigned len)
{
    unsigned i;
    for (i = 0; i < MAXN; i++) {
        memset(seg[i], 0x5A, sizeof seg[i]);
        node[i].segment = seg[i]; node[i].segment_size = sizeof seg[i]; node[i].offset = 0x10;
        node[i].next = i + 1 < len ? &node[i + 1] : NULL;
    }
}

static void env_init(pr_u1c_cmon_env *e, Rec *r, unsigned len)
{
    memset(e, 0, sizeof *e); memset(r, 0, sizeof *r);
    memset(e->block_67f4, 0xAA, sizeof e->block_67f4);
    e->b4435 = 0x11; e->b84e7 = 0x22; e->b6f83 = 0x33;
    e->head = len ? &node[0] : NULL;
    e->hooks.free_af8_0254 = h_free; e->ctx = r;
}

int main(void)
{
    pr_u1c_cmon_env env; Rec rec; pr_u1c_cmon_report rep;
    unsigned len, i;

    for (len = 0; len <= MAXN; len++) {
        make_list(len); env_init(&env, &rec, len);
        memset(&st, 0, sizeof st); st.pc = 0xB1AFu;
        CHECK(pr_u1c_op_clearmonsters(&st, &env, &rep) == PR_U1C_CMON_OK && rep.freed == len && rec.n == len && env.head == NULL);
        for (i = 0; i < len; i++) CHECK(rec.seen[i] == &node[i] && rec.size[i] == 0x3F);        /* head first, then each link */
        for (i = 0; i < sizeof env.block_67f4; i++) CHECK(env.block_67f4[i] == 0);              /* the whole 28-byte block is zero */
        CHECK(env.b4435 == 0 && env.b84e7 == 0 && env.b6f83 == 8 && st.pc == 0xB1B0u);
    }
    /* the PC advance wraps at 16 bits */
    make_list(0); env_init(&env, &rec, 0); memset(&st, 0, sizeof st); st.pc = 0xFFFFu;
    CHECK(pr_u1c_op_clearmonsters(&st, &env, NULL) == PR_U1C_CMON_OK && st.pc == 0);
    /* the node views are not modified (the model only reads the link words) */
    make_list(3); { uint8_t copy[0x100]; memcpy(copy, seg[1], sizeof copy); env_init(&env, &rec, 3); memset(&st, 0, sizeof st);
      CHECK(pr_u1c_op_clearmonsters(&st, &env, NULL) == PR_U1C_CMON_OK && memcmp(copy, seg[1], sizeof copy) == 0); }

    /* a failing free: earlier frees stand, the head stays at the failing node, the zero fill and DS bytes were already done */
    make_list(4); env_init(&env, &rec, 4); rec.fail_at = 3; memset(&st, 0, sizeof st); st.pc = 7;
    CHECK(pr_u1c_op_clearmonsters(&st, &env, &rep) == PR_U1C_CMON_CALLBACK && rep.callback_status == 9 && rep.freed == 2 && rec.n == 3);
    CHECK(env.head == &node[2] && env.block_67f4[0] == 0 && env.b6f83 == 8 && st.pc == 8);
    /* a missing hook with a nonempty list: HOOK at the head; with an empty list it is not needed */
    make_list(2); env_init(&env, &rec, 2); env.hooks.free_af8_0254 = NULL; memset(&st, 0, sizeof st);
    CHECK(pr_u1c_op_clearmonsters(&st, &env, &rep) == PR_U1C_CMON_HOOK && rep.freed == 0 && env.head == &node[0] && env.block_67f4[27] == 0);
    make_list(0); env_init(&env, &rec, 0); env.hooks.free_af8_0254 = NULL; memset(&st, 0, sizeof st);
    CHECK(pr_u1c_op_clearmonsters(&st, &env, &rep) == PR_U1C_CMON_OK);

    /* host guards: a view too small for the four link bytes (+0x2A..+0x2D), a null view, a cycle */
    make_list(3); node[1].segment_size = 0x2D; env_init(&env, &rec, 3); memset(&st, 0, sizeof st);        /* +0x2D is outside */
    CHECK(pr_u1c_op_clearmonsters(&st, &env, &rep) == PR_U1C_CMON_LIST && rep.freed == 1 && env.head == &node[1]);
    make_list(3); node[0].offset = 0xFFF0; node[0].segment_size = 0x10000; env_init(&env, &rec, 3); memset(&st, 0, sizeof st);   /* link bytes wrap to 0x001A..: inside */
    CHECK(pr_u1c_op_clearmonsters(&st, &env, &rep) == PR_U1C_CMON_OK && rep.freed == 3);
    make_list(3); node[1].segment = NULL; env_init(&env, &rec, 3); memset(&st, 0, sizeof st);
    CHECK(pr_u1c_op_clearmonsters(&st, &env, &rep) == PR_U1C_CMON_LIST && rep.freed == 1);
    make_list(2); node[1].next = &node[0]; env_init(&env, &rec, 2); memset(&st, 0, sizeof st);          /* a cycle: the cap stops it */
    CHECK(pr_u1c_op_clearmonsters(&st, &env, &rep) == PR_U1C_CMON_LIST && rep.freed == PR_U1C_CMON_LIST_CAP);

    CHECK(pr_u1c_op_clearmonsters(NULL, &env, &rep) == PR_U1C_CMON_ARGUMENT && pr_u1c_op_clearmonsters(&st, NULL, &rep) == PR_U1C_CMON_ARGUMENT);
    puts("overlay_u1c_clearmonsters_125d: ok");
    return 0;
}
