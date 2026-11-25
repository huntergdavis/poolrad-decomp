/* Focused behaviour test for the unit 0x1c +0x2B7E ADD NPC model. The loader, slot values, add-NPC routine and the two
 * residents are fakes that log their calls in order ('L' loader, '1' and '2' slot values, 'N' add-NPC, 'A' resident
 * 0xBA:0x0BB8, 'B' resident 0xBA:0x0767). The composed case binds the real ECL loader (pr_rec_ecl_load_operands and
 * pr_rec_ecl_slot_value) to the same model. The oracle that compares these paths with the original bytes is
 * tests/overlay/ovl1c_add_npc_2b7e_oracle.py. */
#include "poolrad/recovered/overlay_u1c_add_npc_2b7e.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

typedef struct {
    pr_rec_ecl_loader_status load_st;
    int load_count;
    uint16_t slot_word[3];
    pr_rec_ecl_window_status slot_st[3];
    uint16_t npc_word;
    uint16_t r0bb8_seg, r0bb8_off, r0767_seg, r0767_off;
    char order[16];
    size_t order_n;
} fake_t;

typedef struct {
    fake_t f;
    pr_rec_ecl_loader_state st;
} ctx_t;

static void mark(fake_t *f, char c)
{
    if (f->order_n + 1 < sizeof f->order) {
        f->order[f->order_n++] = c;
        f->order[f->order_n] = '\0';
    }
}

static pr_rec_ecl_loader_status f_load(void *c, uint8_t count)
{
    fake_t *f = c;
    mark(f, 'L');
    f->load_count = count;
    return f->load_st;
}

static pr_rec_ecl_window_status f_slot(void *c, uint8_t slot, uint16_t *value)
{
    fake_t *f = c;
    mark(f, slot == 1u ? '1' : '2');
    *value = f->slot_word[slot];
    return f->slot_st[slot];
}

static void f_npc(void *c, uint16_t word)
{
    fake_t *f = c;
    mark(f, 'N');
    f->npc_word = word;
}

static void f_0bb8(void *c, uint16_t seg, uint16_t off)
{
    fake_t *f = c;
    mark(f, 'A');
    f->r0bb8_seg = seg;
    f->r0bb8_off = off;
}

static void f_0767(void *c, uint16_t seg, uint16_t off)
{
    fake_t *f = c;
    mark(f, 'B');
    f->r0767_seg = seg;
    f->r0767_off = off;
}

static pr_u1c_add_npc_hooks fake_hooks(void *ctx)
{
    pr_u1c_add_npc_hooks h;
    h.context = ctx;
    h.load_operands = f_load;
    h.slot_value = f_slot;
    h.add_npc = f_npc;
    h.resident_0bb8 = f_0bb8;
    h.resident_0767 = f_0767;
    return h;
}

static void reset(fake_t *f, uint16_t s1, uint16_t s2)
{
    memset(f, 0, sizeof *f);
    f->load_st = PR_REC_ECL_LOADER_OK;
    f->slot_word[1] = s1;
    f->slot_word[2] = s2;
    f->slot_st[1] = f->slot_st[2] = PR_REC_ECL_WINDOW_OK;
}

static int untouched(const uint8_t *rec)
{
    return rec[0x84] == 0xEE && rec[0x10E] == 0xEE;
}

static int case_full_path(void)
{
    fake_t f;
    uint8_t rec[PR_U1C_ADD_NPC_RECORD_SIZE];
    pr_u1c_add_npc_record r = { rec, sizeof rec, 0x5678u, 0x3000u };
    pr_u1c_add_npc_hooks h;
    pr_u1c_add_npc_report rep;
    pr_u1c_add_npc_status s;

    memset(rec, 0xEE, sizeof rec);
    reset(&f, 0x1234u, 0x000Bu);
    h = fake_hooks(&f);
    s = pr_u1c_op_add_npc(&h, &r, &rep);
    CHECK(s == PR_U1C_ADD_NPC_OK);
    CHECK(strcmp(f.order, "L1N2AB") == 0);
    CHECK(f.load_count == 2);
    CHECK(f.npc_word == 0x1234u);                /* the full AX word, AH included */
    CHECK(rec[0x84] == 0x85u);                   /* (0x0B >> 1) | 0x80 */
    CHECK(rec[0x10E] == 0u);                     /* low byte 0x34 is not 0x18 */
    CHECK(f.r0bb8_seg == 0x3000u && f.r0bb8_off == 0x5678u);
    CHECK(f.r0767_seg == 0x3000u && f.r0767_off == 0x5678u);
    CHECK(rep.core_status == PR_REC_ECL_HANDLER_OK);
    CHECK(rep.npc_called && rep.residents_called);
    CHECK(rep.class_byte == 0x85u && rep.flag_byte == 0u);
    CHECK(rep.slot1_word == 0x1234u && rep.slot2_word == 0x000Bu);
    CHECK(rep.slot_status[0] == PR_REC_ECL_WINDOW_OK && rep.slot_status[1] == PR_REC_ECL_WINDOW_OK);
    return 0;
}

static int case_flag(void)
{
    static const struct { uint16_t s1; uint8_t flag; } cases[] = {
        { 0x0018u, 1 }, { 0x1918u, 1 }, { 0x1800u, 0 }, { 0x0017u, 0 }, { 0x0019u, 0 }, { 0x0118u, 1 }
    };
    size_t i;
    for (i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        fake_t f;
        uint8_t rec[PR_U1C_ADD_NPC_RECORD_SIZE];
        pr_u1c_add_npc_record r = { rec, sizeof rec, 0x0010u, 0x3000u };
        pr_u1c_add_npc_hooks h;
        pr_u1c_add_npc_report rep;
        memset(rec, 0xEE, sizeof rec);
        reset(&f, cases[i].s1, 0x04u);
        h = fake_hooks(&f);
        CHECK(pr_u1c_op_add_npc(&h, &r, &rep) == PR_U1C_ADD_NPC_OK);
        CHECK(rec[0x10E] == cases[i].flag);
        CHECK(f.npc_word == cases[i].s1);        /* the AH byte is passed through to the add-NPC word */
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
        fake_t f;
        uint8_t rec[PR_U1C_ADD_NPC_RECORD_SIZE];
        pr_u1c_add_npc_record r = { rec, sizeof rec, 0x0010u, 0x3000u };
        pr_u1c_add_npc_hooks h;
        pr_u1c_add_npc_report rep;
        memset(rec, 0xEE, sizeof rec);
        reset(&f, 0x0010u, cases[i].s2);
        h = fake_hooks(&f);
        CHECK(pr_u1c_op_add_npc(&h, &r, &rep) == PR_U1C_ADD_NPC_OK);
        CHECK(rec[0x84] == cases[i].cls);        /* only the low byte of slot 2 counts */
    }
    return 0;
}

static int case_loader_bounds(void)
{
    fake_t f;
    uint8_t rec[PR_U1C_ADD_NPC_RECORD_SIZE];
    pr_u1c_add_npc_record r = { rec, sizeof rec, 0x0010u, 0x3000u };
    pr_u1c_add_npc_hooks h;
    pr_u1c_add_npc_report rep;
    memset(rec, 0xEE, sizeof rec);
    reset(&f, 0x0018u, 0x0Bu);
    f.load_st = PR_REC_ECL_LOADER_BOUNDS;
    h = fake_hooks(&f);
    CHECK(pr_u1c_op_add_npc(&h, &r, &rep) == PR_U1C_ADD_NPC_CORE);
    CHECK(rep.core_status == PR_REC_ECL_HANDLER_BOUNDS);
    CHECK(strcmp(f.order, "L") == 0);
    CHECK(!rep.npc_called && !rep.residents_called);
    CHECK(untouched(rec));
    f.load_st = PR_REC_ECL_LOADER_COUNT;
    reset(&f, 0x0018u, 0x0Bu);
    f.load_st = PR_REC_ECL_LOADER_COUNT;
    CHECK(pr_u1c_op_add_npc(&h, &r, &rep) == PR_U1C_ADD_NPC_CORE);
    CHECK(rep.core_status == PR_REC_ECL_HANDLER_ARGUMENT);
    CHECK(strcmp(f.order, "L") == 0 && untouched(rec));
    return 0;
}

static int case_slot1_bounds(void)
{
    fake_t f;
    uint8_t rec[PR_U1C_ADD_NPC_RECORD_SIZE];
    pr_u1c_add_npc_record r = { rec, sizeof rec, 0x0010u, 0x3000u };
    pr_u1c_add_npc_hooks h;
    pr_u1c_add_npc_report rep;
    memset(rec, 0xEE, sizeof rec);
    reset(&f, 0x0018u, 0x0Bu);
    f.slot_st[1] = PR_REC_ECL_WINDOW_BOUNDS;
    h = fake_hooks(&f);
    CHECK(pr_u1c_op_add_npc(&h, &r, &rep) == PR_U1C_ADD_NPC_CORE);
    CHECK(strcmp(f.order, "L1") == 0);
    CHECK(rep.core_status == PR_REC_ECL_HANDLER_BOUNDS);
    CHECK(rep.slot_status[0] == PR_REC_ECL_WINDOW_BOUNDS);
    CHECK(!rep.npc_called && untouched(rec));
    return 0;
}

static int case_slot2_unresolved(void)
{
    fake_t f;
    uint8_t rec[PR_U1C_ADD_NPC_RECORD_SIZE];
    pr_u1c_add_npc_record r = { rec, sizeof rec, 0x0010u, 0x3000u };
    pr_u1c_add_npc_hooks h;
    pr_u1c_add_npc_report rep;
    memset(rec, 0xEE, sizeof rec);
    reset(&f, 0x0018u, 0x0Bu);
    f.slot_st[2] = PR_REC_ECL_WINDOW_UNRESOLVED;
    h = fake_hooks(&f);
    CHECK(pr_u1c_op_add_npc(&h, &r, &rep) == PR_U1C_ADD_NPC_CORE);
    CHECK(strcmp(f.order, "L1N2") == 0);      /* add-NPC already ran; the store and residents did not */
    CHECK(rep.npc_called && !rep.residents_called);
    CHECK(rep.core_status == PR_REC_ECL_HANDLER_UNRESOLVED);
    CHECK(untouched(rec));
    return 0;
}

static int case_hooks_continue(void)
{
    static const int which[] = { 1, 2 };
    size_t i;
    for (i = 0; i < 2; i++) {
        fake_t f;
        uint8_t rec[PR_U1C_ADD_NPC_RECORD_SIZE];
        pr_u1c_add_npc_record r = { rec, sizeof rec, 0x0010u, 0x3000u };
        pr_u1c_add_npc_hooks h;
        pr_u1c_add_npc_report rep;
        memset(rec, 0xEE, sizeof rec);
        reset(&f, 0x0018u, 0x0Bu);
        f.slot_st[which[i]] = PR_REC_ECL_WINDOW_HOOK;
        h = fake_hooks(&f);
        CHECK(pr_u1c_op_add_npc(&h, &r, &rep) == PR_U1C_ADD_NPC_CORE);
        CHECK(strcmp(f.order, "L1N2AB") == 0);   /* a HOOK completes the handler's effects */
        CHECK(rep.core_status == PR_REC_ECL_HANDLER_HOOK);
        CHECK(rec[0x84] == 0x85u && rec[0x10E] == 1u);
    }
    return 0;
}

static int case_argument(void)
{
    fake_t f;
    uint8_t rec[PR_U1C_ADD_NPC_RECORD_SIZE];
    uint8_t small[PR_U1C_ADD_NPC_RECORD_SIZE - 1];
    pr_u1c_add_npc_record r = { rec, sizeof rec, 0x0010u, 0x3000u };
    pr_u1c_add_npc_record rs = { small, sizeof small, 0x0010u, 0x3000u };
    pr_u1c_add_npc_record rn = { NULL, sizeof rec, 0x0010u, 0x3000u };
    pr_u1c_add_npc_hooks h;
    pr_u1c_add_npc_report rep;
    memset(rec, 0xEE, sizeof rec);
    reset(&f, 0x0018u, 0x0Bu);
    h = fake_hooks(&f);
    CHECK(pr_u1c_op_add_npc(&h, &rs, &rep) == PR_U1C_ADD_NPC_ARGUMENT);
    CHECK(pr_u1c_op_add_npc(&h, &rn, &rep) == PR_U1C_ADD_NPC_ARGUMENT);
    CHECK(pr_u1c_op_add_npc(NULL, &r, &rep) == PR_U1C_ADD_NPC_ARGUMENT);
    CHECK(pr_u1c_op_add_npc(&h, NULL, &rep) == PR_U1C_ADD_NPC_ARGUMENT);
    CHECK(f.order_n == 0);                    /* nothing called on any argument failure */
    CHECK(untouched(rec));
    CHECK(pr_u1c_op_add_npc(&h, &r, NULL) == PR_U1C_ADD_NPC_OK);   /* report is optional */
    return 0;
}

/* The composed case: the real ECL loader supplies operands and slot values. Script: opcode 0x36, operand 1 byte
 * 0x18 (tag 0x00), operand 2 byte 0x0A (tag 0x00). Slot 1 is the zero-extended byte 0x18, slot 2 is 0x0A. */
static pr_rec_ecl_loader_status c_load(void *c, uint8_t count)
{
    ctx_t *x = c;
    mark(&x->f, 'L');
    return pr_rec_ecl_load_operands(&x->st, count);
}

static pr_rec_ecl_window_status c_slot(void *c, uint8_t slot, uint16_t *value)
{
    ctx_t *x = c;
    mark(&x->f, slot == 1u ? '1' : '2');
    return pr_rec_ecl_slot_value(&x->st, slot, value);
}

static void c_npc(void *c, uint16_t word)
{
    ctx_t *x = c;
    f_npc(&x->f, word);
}

static void c_0bb8(void *c, uint16_t seg, uint16_t off)
{
    ctx_t *x = c;
    f_0bb8(&x->f, seg, off);
}

static void c_0767(void *c, uint16_t seg, uint16_t off)
{
    ctx_t *x = c;
    f_0767(&x->f, seg, off);
}

static int case_composed(void)
{
    static uint8_t script[16];
    ctx_t x;
    uint8_t rec[PR_U1C_ADD_NPC_RECORD_SIZE];
    pr_u1c_add_npc_record r = { rec, sizeof rec, 0x0010u, 0x3000u };
    pr_u1c_add_npc_hooks h;
    pr_u1c_add_npc_report rep;
    pr_u1c_add_npc_status s;

    memset(&x, 0, sizeof x);
    memset(rec, 0xEE, sizeof rec);
    script[0] = 0x36; script[1] = 0x00; script[2] = 0x18; script[3] = 0x00; script[4] = 0x0A;
    x.st.script = script;
    x.st.script_size = 5;
    x.st.pc = 0x9900u;
    x.st.latch = 0x36;
    h.context = &x;
    h.load_operands = c_load;
    h.slot_value = c_slot;
    h.add_npc = c_npc;
    h.resident_0bb8 = c_0bb8;
    h.resident_0767 = c_0767;
    s = pr_u1c_op_add_npc(&h, &r, &rep);
    CHECK(s == PR_U1C_ADD_NPC_OK);
    CHECK(strcmp(x.f.order, "L1N2AB") == 0);
    CHECK(x.f.npc_word == 0x0018u);
    CHECK(rec[0x84] == 0x85u && rec[0x10E] == 1u);
    CHECK(rep.slot1_word == 0x0018u && rep.slot2_word == 0x000Au);
    return 0;
}

int main(void)
{
    int rc = 0;
    rc |= case_full_path();
    rc |= case_flag();
    rc |= case_class();
    rc |= case_loader_bounds();
    rc |= case_slot1_bounds();
    rc |= case_slot2_unresolved();
    rc |= case_hooks_continue();
    rc |= case_argument();
    rc |= case_composed();
    if (rc == 0)
        printf("OK overlay_u1c_add_npc_2b7e: all focused cases pass\n");
    return rc;
}
