/* Focused behaviour test for the unit 0x1c +0x2B7E ADD NPC model (revised for live active-record resolution).
 *
 * The world holds up to three far records (A, B, C), each a host buffer of 0x11D bytes at a segment:offset pair, and
 * one active pointer (the value DS:0x5D92/DS:0x5D94 would hold). The loader, slot values, add-NPC routine and both
 * residents are fakes that log their calls in order ('L' loader, '1' and '2' slot values, 'N' add-NPC, 'A' resident
 * 0xBA:0x0BB8, 'B' resident 0xBA:0x0767). A callee may retarget the active pointer, as a successful insertion does. A
 * record may be marked unresolvable, which makes record_bytes return NULL for its pair.
 *
 * The composed case binds the real ECL loader (pr_rec_ecl_load_operands and pr_rec_ecl_slot_value) to the same world.
 * The oracle that compares these paths with the original bytes is tests/overlay/ovl1c_add_npc_2b7e_oracle.py. */
#include "poolrad/recovered/overlay_u1c_add_npc_2b7e.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

#define NREC 3
#define REC_A 0
#define REC_B 1
#define REC_C 2

typedef struct {
    uint16_t seg, off;
    uint8_t bytes[PR_U1C_ADD_NPC_RECORD_SIZE];
    int resolvable;              /* 0: record_bytes returns NULL for this pair */
} rec_t;

typedef struct {
    rec_t rec[NREC];
    int active;                  /* index of the record the active pointer names */
    int npc_retarget;            /* record index the add-NPC routine selects, or -1 */
    int r0bb8_retarget;          /* record index the 0BB8 resident selects, or -1 */
    pr_rec_ecl_loader_status load_st;
    int load_count;
    uint16_t slot_word[3];
    pr_rec_ecl_window_status slot_st[3];
    uint16_t npc_word;
    uint16_t r0bb8_seg, r0bb8_off, r0767_seg, r0767_off;
    char order[16];
    size_t order_n;
    int read_fail;               /* read_active reports a failure */
} world_t;

typedef struct {
    world_t w;
    pr_rec_ecl_loader_state st;
} ctx_t;

static void mark(world_t *w, char c)
{
    if (w->order_n + 1 < sizeof w->order) {
        w->order[w->order_n++] = c;
        w->order[w->order_n] = '\0';
    }
}

static void world_init(world_t *w)
{
    int i;
    memset(w, 0, sizeof *w);
    w->rec[REC_A].seg = 0x3000u; w->rec[REC_A].off = 0x5678u; w->rec[REC_A].resolvable = 1;
    w->rec[REC_B].seg = 0x4000u; w->rec[REC_B].off = 0x0100u; w->rec[REC_B].resolvable = 1;
    w->rec[REC_C].seg = 0x5000u; w->rec[REC_C].off = 0x0200u; w->rec[REC_C].resolvable = 1;
    for (i = 0; i < NREC; i++)
        memset(w->rec[i].bytes, 0xEE, sizeof w->rec[i].bytes);
    w->active = REC_A;
    w->npc_retarget = -1;
    w->r0bb8_retarget = -1;
    w->load_st = PR_REC_ECL_LOADER_OK;
    w->slot_st[1] = w->slot_st[2] = PR_REC_ECL_WINDOW_OK;
}

static void world_slots(world_t *w, uint16_t s1, uint16_t s2)
{
    w->slot_word[1] = s1;
    w->slot_word[2] = s2;
}

static pr_rec_ecl_loader_status f_load(void *c, uint8_t count)
{
    world_t *w = c;
    mark(w, 'L');
    w->load_count = count;
    return w->load_st;
}

static pr_rec_ecl_window_status f_slot(void *c, uint8_t slot, uint16_t *value)
{
    world_t *w = c;
    mark(w, slot == 1u ? '1' : '2');
    *value = w->slot_word[slot];
    return w->slot_st[slot];
}

static void f_npc(void *c, uint16_t word)
{
    world_t *w = c;
    mark(w, 'N');
    w->npc_word = word;
    if (w->npc_retarget >= 0)
        w->active = w->npc_retarget;
}

static void f_0bb8(void *c, uint16_t seg, uint16_t off)
{
    world_t *w = c;
    mark(w, 'A');
    w->r0bb8_seg = seg;
    w->r0bb8_off = off;
    if (w->r0bb8_retarget >= 0)
        w->active = w->r0bb8_retarget;
}

static void f_0767(void *c, uint16_t seg, uint16_t off)
{
    world_t *w = c;
    mark(w, 'B');
    w->r0767_seg = seg;
    w->r0767_off = off;
}

static int f_read(void *c, uint16_t *seg, uint16_t *off)
{
    world_t *w = c;
    if (w->read_fail)
        return 1;
    *seg = w->rec[w->active].seg;
    *off = w->rec[w->active].off;
    return 0;
}

static uint8_t *f_bytes(void *c, uint16_t seg, uint16_t off, size_t size)
{
    world_t *w = c;
    int i;
    if (size > PR_U1C_ADD_NPC_RECORD_SIZE)
        return NULL;
    for (i = 0; i < NREC; i++) {
        if (w->rec[i].seg == seg && w->rec[i].off == off)
            return w->rec[i].resolvable ? w->rec[i].bytes : NULL;
    }
    return NULL;
}

static pr_u1c_add_npc_hooks world_hooks(void *ctx)
{
    pr_u1c_add_npc_hooks h;
    h.context = ctx;
    h.load_operands = f_load;
    h.slot_value = f_slot;
    h.add_npc = f_npc;
    h.read_active = f_read;
    h.record_bytes = f_bytes;
    h.resident_0bb8 = f_0bb8;
    h.resident_0767 = f_0767;
    return h;
}

static int untouched(const rec_t *r)
{
    size_t i;
    for (i = 0; i < PR_U1C_ADD_NPC_RECORD_SIZE; i++) {
        if (r->bytes[i] != 0xEE)
            return 0;
    }
    return 1;
}

static int case_full_path(void)
{
    world_t w;
    pr_u1c_add_npc_hooks h;
    pr_u1c_add_npc_report rep;
    world_init(&w);
    world_slots(&w, 0x1234u, 0x000Bu);
    h = world_hooks(&w);
    CHECK(pr_u1c_op_add_npc(&h, &rep) == PR_U1C_ADD_NPC_OK);
    CHECK(strcmp(w.order, "L1N2AB") == 0);
    CHECK(w.load_count == 2);
    CHECK(w.npc_word == 0x1234u);                       /* the full AX word, AH included */
    CHECK(w.rec[REC_A].bytes[0x84] == 0x85u);           /* (0x0B >> 1) | 0x80 */
    CHECK(w.rec[REC_A].bytes[0x10E] == 0u);             /* low byte 0x34 is not 0x18 */
    CHECK(w.r0bb8_seg == 0x3000u && w.r0bb8_off == 0x5678u);
    CHECK(w.r0767_seg == 0x3000u && w.r0767_off == 0x5678u);
    CHECK(rep.core_status == PR_REC_ECL_HANDLER_OK);
    CHECK(rep.npc_called && rep.residents_called == 2);
    CHECK(rep.class_written && rep.flag_written);
    CHECK(rep.class_byte == 0x85u && rep.flag_byte == 0u);
    CHECK(rep.slot1_word == 0x1234u && rep.slot2_word == 0x000Bu);
    CHECK(rep.use_resolved[PR_U1C_ADD_NPC_USE_CLASS] && rep.use_resolved[PR_U1C_ADD_NPC_USE_FLAG]);
    CHECK(rep.use_resolved[PR_U1C_ADD_NPC_USE_RES_0BB8] && rep.use_resolved[PR_U1C_ADD_NPC_USE_RES_0767]);
    CHECK(rep.use_segment[PR_U1C_ADD_NPC_USE_CLASS] == 0x3000u && rep.use_offset[PR_U1C_ADD_NPC_USE_CLASS] == 0x5678u);
    CHECK(untouched(&w.rec[REC_B]) && untouched(&w.rec[REC_C]));
    return 0;
}

/* Regression: the add-NPC routine selects a new active record B (as a successful insertion does). The stores and both
 * residents must use B, and the old record A must stay untouched. */
static int case_two_record_insertion(void)
{
    world_t w;
    pr_u1c_add_npc_hooks h;
    pr_u1c_add_npc_report rep;
    world_init(&w);
    world_slots(&w, 0x0018u, 0x0Bu);                    /* id 0x18: flag 1 */
    w.npc_retarget = REC_B;
    h = world_hooks(&w);
    CHECK(pr_u1c_op_add_npc(&h, &rep) == PR_U1C_ADD_NPC_OK);
    CHECK(strcmp(w.order, "L1N2AB") == 0);
    CHECK(untouched(&w.rec[REC_A]));                    /* old record untouched */
    CHECK(w.rec[REC_B].bytes[0x84] == 0x85u);           /* incoming class store */
    CHECK(w.rec[REC_B].bytes[0x10E] == 1u);             /* incoming flag store */
    CHECK(w.rec[REC_C].bytes[0x84] == 0xEEu);
    CHECK(w.r0bb8_seg == w.rec[REC_B].seg && w.r0bb8_off == w.rec[REC_B].off);
    CHECK(w.r0767_seg == w.rec[REC_B].seg && w.r0767_off == w.rec[REC_B].off);
    CHECK(rep.use_segment[PR_U1C_ADD_NPC_USE_CLASS] == w.rec[REC_B].seg);
    CHECK(rep.use_offset[PR_U1C_ADD_NPC_USE_CLASS] == w.rec[REC_B].off);
    CHECK(rep.use_segment[PR_U1C_ADD_NPC_USE_RES_0767] == w.rec[REC_B].seg);
    CHECK(rep.core_status == PR_REC_ECL_HANDLER_OK);
    return 0;
}

/* Retarget between the two resident sites: the first resident selects C, and the second resident must receive C. */
static int case_retarget_between_residents(void)
{
    world_t w;
    pr_u1c_add_npc_hooks h;
    pr_u1c_add_npc_report rep;
    world_init(&w);
    world_slots(&w, 0x0034u, 0x0Bu);
    w.r0bb8_retarget = REC_C;
    h = world_hooks(&w);
    CHECK(pr_u1c_op_add_npc(&h, &rep) == PR_U1C_ADD_NPC_OK);
    CHECK(strcmp(w.order, "L1N2AB") == 0);
    CHECK(w.r0bb8_seg == w.rec[REC_A].seg && w.r0bb8_off == w.rec[REC_A].off);   /* first resident: A */
    CHECK(w.r0767_seg == w.rec[REC_C].seg && w.r0767_off == w.rec[REC_C].off);   /* second resident: C */
    CHECK(rep.use_segment[PR_U1C_ADD_NPC_USE_RES_0767] == w.rec[REC_C].seg);
    CHECK(w.rec[REC_A].bytes[0x84] == 0x85u && w.rec[REC_A].bytes[0x10E] == 0u);
    CHECK(untouched(&w.rec[REC_C]) && untouched(&w.rec[REC_B]));
    return 0;
}

/* Replacement validation at the class site: the add-NPC routine selects a pointer the host cannot resolve. Nothing
 * is stored and no resident is called. */
static int case_invalid_replacement_at_class(void)
{
    world_t w;
    pr_u1c_add_npc_hooks h;
    pr_u1c_add_npc_report rep;
    world_init(&w);
    world_slots(&w, 0x0018u, 0x0Bu);
    w.npc_retarget = REC_B;
    w.rec[REC_B].resolvable = 0;
    h = world_hooks(&w);
    CHECK(pr_u1c_op_add_npc(&h, &rep) == PR_U1C_ADD_NPC_CORE);
    CHECK(rep.core_status == PR_REC_ECL_HANDLER_UNRESOLVED);
    CHECK(strcmp(w.order, "L1N2") == 0);
    CHECK(!rep.class_written && !rep.flag_written && rep.residents_called == 0);
    CHECK(untouched(&w.rec[REC_A]) && untouched(&w.rec[REC_B]));
    CHECK(rep.use_resolved[PR_U1C_ADD_NPC_USE_CLASS] == 0u);
    return 0;
}

/* Replacement validation at the second resident site: the first resident retargets to an unresolvable pointer.
 * The first resident has run; the second is not called; the stores made at the class and flag sites stand. */
static int case_invalid_replacement_at_res_0767(void)
{
    world_t w;
    pr_u1c_add_npc_hooks h;
    pr_u1c_add_npc_report rep;
    world_init(&w);
    world_slots(&w, 0x0018u, 0x0Bu);
    w.r0bb8_retarget = REC_B;
    w.rec[REC_B].resolvable = 0;
    h = world_hooks(&w);
    CHECK(pr_u1c_op_add_npc(&h, &rep) == PR_U1C_ADD_NPC_CORE);
    CHECK(rep.core_status == PR_REC_ECL_HANDLER_UNRESOLVED);
    CHECK(strcmp(w.order, "L1N2A") == 0);
    CHECK(rep.class_written && rep.flag_written && rep.residents_called == 1);
    CHECK(w.rec[REC_A].bytes[0x84] == 0x85u && w.rec[REC_A].bytes[0x10E] == 1u);
    CHECK(untouched(&w.rec[REC_B]));
    CHECK(rep.use_resolved[PR_U1C_ADD_NPC_USE_RES_0767] == 0u);
    return 0;
}

/* A failed live read at the first use point stops before any store. */
static int case_read_failure(void)
{
    world_t w;
    pr_u1c_add_npc_hooks h;
    pr_u1c_add_npc_report rep;
    world_init(&w);
    world_slots(&w, 0x0018u, 0x0Bu);
    w.read_fail = 1;
    h = world_hooks(&w);
    CHECK(pr_u1c_op_add_npc(&h, &rep) == PR_U1C_ADD_NPC_CORE);
    CHECK(rep.core_status == PR_REC_ECL_HANDLER_UNRESOLVED);
    CHECK(strcmp(w.order, "L1N2") == 0);
    CHECK(!rep.class_written && untouched(&w.rec[REC_A]));
    return 0;
}

static int case_loader_failure_before_slot1(void)
{
    world_t w;
    pr_u1c_add_npc_hooks h;
    pr_u1c_add_npc_report rep;
    world_init(&w);
    world_slots(&w, 0x0018u, 0x0Bu);
    w.load_st = PR_REC_ECL_LOADER_BOUNDS;
    h = world_hooks(&w);
    CHECK(pr_u1c_op_add_npc(&h, &rep) == PR_U1C_ADD_NPC_CORE);
    CHECK(rep.core_status == PR_REC_ECL_HANDLER_BOUNDS);
    CHECK(strcmp(w.order, "L") == 0);                   /* the loader callback ran; no slot 1 read */
    CHECK(!rep.npc_called && !rep.class_written && untouched(&w.rec[REC_A]));
    w.load_st = PR_REC_ECL_LOADER_COUNT;
    w.order_n = 0; w.order[0] = '\0';
    CHECK(pr_u1c_op_add_npc(&h, &rep) == PR_U1C_ADD_NPC_CORE);
    CHECK(rep.core_status == PR_REC_ECL_HANDLER_ARGUMENT);
    CHECK(strcmp(w.order, "L") == 0 && untouched(&w.rec[REC_A]));
    return 0;
}

static int case_flag(void)
{
    static const struct { uint16_t s1; uint8_t flag; } cases[] = {
        { 0x0018u, 1 }, { 0x1918u, 1 }, { 0x1800u, 0 }, { 0x0017u, 0 }, { 0x0019u, 0 }, { 0x0118u, 1 }
    };
    size_t i;
    for (i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        world_t w;
        pr_u1c_add_npc_hooks h;
        pr_u1c_add_npc_report rep;
        world_init(&w);
        world_slots(&w, cases[i].s1, 0x04u);
        h = world_hooks(&w);
        CHECK(pr_u1c_op_add_npc(&h, &rep) == PR_U1C_ADD_NPC_OK);
        CHECK(w.rec[REC_A].bytes[0x10E] == cases[i].flag);
        CHECK(w.npc_word == cases[i].s1);               /* the AH byte is passed through to the add-NPC word */
    }
    return 0;
}

static int case_class(void)
{
    static const struct { uint16_t s2; uint8_t cls; } cases[] = {
        { 0x0000u, 0x80u }, { 0x00FFu, 0xFFu }, { 0x0100u, 0x80u }, { 0x0101u, 0x80u },
        { 0x0203u, 0x81u }, { 0x0200u, 0x80u }, { 0x007Fu, 0xBFu }
    };
    size_t i;
    for (i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        world_t w;
        pr_u1c_add_npc_hooks h;
        pr_u1c_add_npc_report rep;
        world_init(&w);
        world_slots(&w, 0x0010u, cases[i].s2);
        h = world_hooks(&w);
        CHECK(pr_u1c_op_add_npc(&h, &rep) == PR_U1C_ADD_NPC_OK);
        CHECK(w.rec[REC_A].bytes[0x84] == cases[i].cls); /* only the low byte of slot 2 counts */
    }
    return 0;
}

static int case_slot_positions(void)
{
    static const struct { int slot; pr_rec_ecl_window_status st; pr_rec_ecl_handler_status core; } cases[] = {
        { 1, PR_REC_ECL_WINDOW_BOUNDS, PR_REC_ECL_HANDLER_BOUNDS },
        { 1, PR_REC_ECL_WINDOW_UNRESOLVED, PR_REC_ECL_HANDLER_UNRESOLVED },
        { 1, PR_REC_ECL_WINDOW_ARGUMENT, PR_REC_ECL_HANDLER_ARGUMENT },
        { 2, PR_REC_ECL_WINDOW_BOUNDS, PR_REC_ECL_HANDLER_BOUNDS },
        { 2, PR_REC_ECL_WINDOW_UNRESOLVED, PR_REC_ECL_HANDLER_UNRESOLVED },
        { 2, PR_REC_ECL_WINDOW_ARGUMENT, PR_REC_ECL_HANDLER_ARGUMENT }
    };
    size_t i;
    for (i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        world_t w;
        pr_u1c_add_npc_hooks h;
        pr_u1c_add_npc_report rep;
        world_init(&w);
        world_slots(&w, 0x0018u, 0x0Bu);
        w.slot_st[cases[i].slot] = cases[i].st;
        h = world_hooks(&w);
        CHECK(pr_u1c_op_add_npc(&h, &rep) == PR_U1C_ADD_NPC_CORE);
        CHECK(rep.core_status == cases[i].core);
        CHECK(rep.slot_status[cases[i].slot - 1] == cases[i].st);
        CHECK(!rep.class_written && !rep.flag_written && rep.residents_called == 0);
        CHECK(untouched(&w.rec[REC_A]));
        if (cases[i].slot == 2)
            CHECK(rep.npc_called && strcmp(w.order, "L1N2") == 0);
        else
            CHECK(!rep.npc_called && strcmp(w.order, "L1") == 0);
    }
    return 0;
}

static int case_hooks_continue(void)
{
    static const int which[] = { 1, 2 };
    size_t i;
    for (i = 0; i < 2; i++) {
        world_t w;
        pr_u1c_add_npc_hooks h;
        pr_u1c_add_npc_report rep;
        world_init(&w);
        world_slots(&w, 0x0018u, 0x0Bu);
        w.slot_st[which[i]] = PR_REC_ECL_WINDOW_HOOK;
        h = world_hooks(&w);
        CHECK(pr_u1c_op_add_npc(&h, &rep) == PR_U1C_ADD_NPC_CORE);
        CHECK(strcmp(w.order, "L1N2AB") == 0);          /* a HOOK completes the handler's effects */
        CHECK(rep.core_status == PR_REC_ECL_HANDLER_HOOK);
        CHECK(w.rec[REC_A].bytes[0x84] == 0x85u && w.rec[REC_A].bytes[0x10E] == 1u);
    }
    return 0;
}

static int case_argument(void)
{
    world_t w;
    pr_u1c_add_npc_hooks h, bad;
    pr_u1c_add_npc_report rep;
    world_init(&w);
    world_slots(&w, 0x0018u, 0x0Bu);
    h = world_hooks(&w);
    bad = h; bad.read_active = NULL;
    CHECK(pr_u1c_op_add_npc(&bad, &rep) == PR_U1C_ADD_NPC_ARGUMENT);
    bad = h; bad.record_bytes = NULL;
    CHECK(pr_u1c_op_add_npc(&bad, &rep) == PR_U1C_ADD_NPC_ARGUMENT);
    bad = h; bad.resident_0767 = NULL;
    CHECK(pr_u1c_op_add_npc(&bad, &rep) == PR_U1C_ADD_NPC_ARGUMENT);
    CHECK(pr_u1c_op_add_npc(NULL, &rep) == PR_U1C_ADD_NPC_ARGUMENT);
    CHECK(w.order_n == 0);                              /* nothing called on any argument failure */
    CHECK(untouched(&w.rec[REC_A]));
    CHECK(pr_u1c_op_add_npc(&h, NULL) == PR_U1C_ADD_NPC_OK);   /* report is optional */
    return 0;
}

/* Record smaller than the frame: the host cannot resolve it to the required size. */
static int case_small_record(void)
{
    world_t w;
    pr_u1c_add_npc_hooks h;
    pr_u1c_add_npc_report rep;
    world_init(&w);
    world_slots(&w, 0x0018u, 0x0Bu);
    h = world_hooks(&w);
    h.record_bytes = NULL;                              /* a host that cannot resolve any record */
    CHECK(pr_u1c_op_add_npc(&h, &rep) == PR_U1C_ADD_NPC_ARGUMENT);
    return 0;
}

/* The composed case: the real ECL loader supplies operands and slot values. Script: opcode 0x36, operand 1 byte
 * 0x18 (tag 0x00), operand 2 byte 0x0A (tag 0x00). Slot 1 is the zero-extended byte 0x18, slot 2 is 0x0A. */
static pr_rec_ecl_loader_status c_load(void *c, uint8_t count)
{
    ctx_t *x = c;
    mark(&x->w, 'L');
    return pr_rec_ecl_load_operands(&x->st, count);
}

static pr_rec_ecl_window_status c_slot(void *c, uint8_t slot, uint16_t *value)
{
    ctx_t *x = c;
    mark(&x->w, slot == 1u ? '1' : '2');
    return pr_rec_ecl_slot_value(&x->st, slot, value);
}

static void c_npc(void *c, uint16_t word)
{
    ctx_t *x = c;
    f_npc(&x->w, word);
}

static int c_read(void *c, uint16_t *seg, uint16_t *off)
{
    ctx_t *x = c;
    return f_read(&x->w, seg, off);
}

static uint8_t *c_bytes(void *c, uint16_t seg, uint16_t off, size_t size)
{
    ctx_t *x = c;
    return f_bytes(&x->w, seg, off, size);
}

static void c_0bb8(void *c, uint16_t seg, uint16_t off)
{
    ctx_t *x = c;
    f_0bb8(&x->w, seg, off);
}

static void c_0767(void *c, uint16_t seg, uint16_t off)
{
    ctx_t *x = c;
    f_0767(&x->w, seg, off);
}

static int case_composed(void)
{
    static uint8_t script[16];
    ctx_t x;
    pr_u1c_add_npc_hooks h;
    pr_u1c_add_npc_report rep;

    memset(&x, 0, sizeof x);
    world_init(&x.w);
    script[0] = 0x36; script[1] = 0x00; script[2] = 0x18; script[3] = 0x00; script[4] = 0x0A;
    x.st.script = script;
    x.st.script_size = 5;
    x.st.pc = 0x9900u;
    x.st.latch = 0x36;
    h.context = &x;
    h.load_operands = c_load;
    h.slot_value = c_slot;
    h.add_npc = c_npc;
    h.read_active = c_read;
    h.record_bytes = c_bytes;
    h.resident_0bb8 = c_0bb8;
    h.resident_0767 = c_0767;
    CHECK(pr_u1c_op_add_npc(&h, &rep) == PR_U1C_ADD_NPC_OK);
    CHECK(strcmp(x.w.order, "L1N2AB") == 0);
    CHECK(x.w.npc_word == 0x0018u);
    CHECK(x.w.rec[REC_A].bytes[0x84] == 0x85u && x.w.rec[REC_A].bytes[0x10E] == 1u);
    CHECK(rep.slot1_word == 0x0018u && rep.slot2_word == 0x000Au);
    return 0;
}

int main(void)
{
    int rc = 0;
    rc |= case_full_path();
    rc |= case_two_record_insertion();
    rc |= case_retarget_between_residents();
    rc |= case_invalid_replacement_at_class();
    rc |= case_invalid_replacement_at_res_0767();
    rc |= case_read_failure();
    rc |= case_loader_failure_before_slot1();
    rc |= case_flag();
    rc |= case_class();
    rc |= case_slot_positions();
    rc |= case_hooks_continue();
    rc |= case_argument();
    rc |= case_small_record();
    rc |= case_composed();
    if (rc == 0)
        printf("OK overlay_u1c_add_npc_2b7e: all focused cases pass\n");
    return rc;
}
