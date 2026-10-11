#include "poolrad/recovered/resident_load_pieces_0767.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); return 1; \
} } while (0)

typedef struct {
    pr_rec_0767_far address;
    uint8_t b111, b11b;
    pr_rec_0767_far next;
} synthetic_node;

typedef struct {
    synthetic_node nodes[2];
    unsigned node_count;
    pr_rec_0767_far head;
    pr_rec_0767_event events[32];
    unsigned event_count;
    char trace[64];
    unsigned calls;
    unsigned fail_at;
    uint8_t final_al;
    int vary_al;
    int change_head_after_title;
    int change_fields_after_calls;
} fixture;

static int far_equal(pr_rec_0767_far a, pr_rec_0767_far b) {
    return a.segment == b.segment && a.offset == b.offset;
}

static synthetic_node *find_node(fixture *f, pr_rec_0767_far address) {
    unsigned i;
    for (i = 0; i < f->node_count; ++i)
        if (far_equal(f->nodes[i].address, address)) return &f->nodes[i];
    return NULL;
}

static int step(fixture *f, char code) {
    if (f->calls >= sizeof f->trace - 1) return 1;
    f->trace[f->calls++] = code;
    f->trace[f->calls] = '\0';
    return f->calls == f->fail_at;
}

static int read_head(void *ctx, pr_rec_0767_far *head) {
    fixture *f = ctx;
    if (step(f, 'H')) return 1;
    *head = f->head;
    return 0;
}

static int read_byte(void *ctx, pr_rec_0767_far address,
                     uint16_t displacement, uint8_t *value) {
    fixture *f = ctx;
    synthetic_node *node = find_node(f, address);
    char code = displacement == 0x111 ? '1' : '2';
    if (!node || (displacement != 0x111 && displacement != 0x11B)) return 1;
    if (step(f, code)) return 1;
    *value = displacement == 0x111 ? node->b111 : node->b11b;
    return 0;
}

static int read_next(void *ctx, pr_rec_0767_far address,
                     pr_rec_0767_far *next) {
    fixture *f = ctx;
    synthetic_node *node = find_node(f, address);
    if (!node || step(f, 'N')) return 1;
    *next = node->next;
    return 0;
}

static int effect(void *ctx, const pr_rec_0767_event *event, uint8_t *al) {
    fixture *f = ctx;
    const char codes[] = {'A', 'B', 'F', 'T', 'O', 'U', 'V'};
    if ((unsigned)event->kind >= sizeof codes ||
        f->event_count >= sizeof f->events / sizeof f->events[0]) return 1;
    f->events[f->event_count++] = *event;
    if (step(f, codes[event->kind])) return 1;
    if (f->change_head_after_title && event->kind == PR_REC_0767_TITLE_B)
        f->head = f->nodes[0].address;
    if (f->change_fields_after_calls && event->kind == PR_REC_0767_FIELD_111)
        f->nodes[0].b11b = 0x64;
    if (f->change_fields_after_calls && event->kind == PR_REC_0767_FIELD_11B)
        f->nodes[0].next = (pr_rec_0767_far){0, 0};
    *al = f->vary_al ? (uint8_t)f->event_count : f->final_al;
    return 0;
}

static pr_rec_0767_io io_for(fixture *f) {
    pr_rec_0767_io io = {f, read_head, read_byte, read_next, effect};
    return io;
}

static void one_node(fixture *f) {
    memset(f, 0, sizeof *f);
    f->node_count = 1;
    f->nodes[0].address = (pr_rec_0767_far){0x1234, 0x5678};
    f->nodes[0].b111 = 0x33;
    f->nodes[0].b11b = 0x0A;
    f->head = f->nodes[0].address;
    f->final_al = 0x9C;
}

static int test_empty_and_mode(void) {
    fixture f = {0};
    pr_rec_0767_io io = io_for(&f);
    pr_rec_0767_far active = {0x1234, 0x5678};
    uint8_t al = 0;
    f.final_al = 0xA5;
    CHECK(pr_rec_res_00ba_0767(&io, active, 0, 0, &al) == PR_REC_0767_OK);
    CHECK(strcmp(f.trace, "ABHF") == 0 && f.event_count == 3 && al == 0xA5);
    CHECK(f.events[0].kind == PR_REC_0767_TITLE_A &&
          f.events[0].arg_count == 3 && f.events[0].args[0] == 1 &&
          f.events[0].args[1] == 2 && f.events[0].args[2] == 0x0F);
    CHECK(f.events[1].kind == PR_REC_0767_TITLE_B &&
          f.events[1].args[0] == 0x21 && f.events[1].args[1] == 2);
    CHECK(f.events[2].kind == PR_REC_0767_FRAME &&
          f.events[2].arg_count == 4 && f.events[2].args[0] == 1 &&
          f.events[2].args[1] == 4 && f.events[2].args[2] == 0x26 &&
          f.events[2].args[3] == 4);

    memset(&f, 0, sizeof f);
    io = io_for(&f);
    f.final_al = 0x6D;
    CHECK(pr_rec_res_00ba_0767(&io, active, 1, 0, &al) == PR_REC_0767_OK);
    CHECK(f.events[0].args[0] == 0x11 && f.events[1].args[0] == 0x21 &&
          f.events[2].args[0] == 0x11 && al == 0x6D);

    memset(&f, 0, sizeof f);
    io = io_for(&f);
    f.vary_al = 1;
    CHECK(pr_rec_res_00ba_0767(&io, active, 0, 0, &al) == PR_REC_0767_OK);
    CHECK(al == 3); /* terminal frame, after both title effects */
    return 0;
}

static int test_two_nodes_and_identity(void) {
    fixture f;
    pr_rec_0767_io io;
    pr_rec_0767_far active;
    uint8_t al = 0;
    one_node(&f);
    f.node_count = 2;
    f.nodes[1].address = (pr_rec_0767_far){0x9999, 0x5678};
    f.nodes[1].b111 = 0x45;
    f.nodes[1].b11b = 9;
    f.nodes[0].next = f.nodes[1].address;
    active = f.nodes[1].address; /* same offset, different segment */
    io = io_for(&f);
    CHECK(pr_rec_res_00ba_0767(&io, active, 0, 2, &al) == PR_REC_0767_OK);
    CHECK(strcmp(f.trace, "ABHFO1U2VNFT1U2VNF") == 0 && al == 0x9C);
    CHECK(f.event_count == 11);
    CHECK(f.events[3].kind == PR_REC_0767_OTHER_TEXT &&
          far_equal(f.events[3].node, f.nodes[0].address) &&
          f.events[3].args[0] == 1 && f.events[3].args[1] == 4 &&
          f.events[3].args[2] == 0);
    CHECK(f.events[4].kind == PR_REC_0767_FIELD_111 &&
          f.events[4].args[0] == 0x22 && f.events[4].args[1] == 4);
    CHECK(f.events[5].kind == PR_REC_0767_FIELD_11B &&
          f.events[5].args[0] == 0x25 && f.events[5].args[1] == 4 &&
          f.events[5].args[2] == 0);
    CHECK(f.events[6].kind == PR_REC_0767_FRAME &&
          f.events[6].args[1] == 5 && f.events[6].args[3] == 5);
    CHECK(f.events[7].kind == PR_REC_0767_ACTIVE_TEXT &&
          far_equal(f.events[7].node, active) &&
          f.events[7].args[2] == 0x0F);
    CHECK(f.events[8].args[0] == 0x21 && f.events[9].args[0] == 0x26);
    CHECK(f.events[10].kind == PR_REC_0767_FRAME &&
          f.events[10].args[1] == 6);

    /* The DOS loop checks offset OR segment, not offset alone. */
    one_node(&f);
    f.nodes[0].address.offset = 0;
    f.head = f.nodes[0].address;
    io = io_for(&f);
    CHECK(pr_rec_res_00ba_0767(&io, f.nodes[0].address, 0, 1,
                                 &al) == PR_REC_0767_OK);
    CHECK(f.event_count == 7 && f.events[3].kind == PR_REC_0767_ACTIVE_TEXT);
    return 0;
}

static int test_classification_boundaries(void) {
    static const struct { uint8_t input, kind; } a[] = {
        {0, 1}, {0x32, 1}, {0x33, 2}, {0x3C, 2},
        {0x3D, 1}, {0x45, 1}, {0x46, 0}, {0xFF, 0}
    };
    static const struct { uint8_t input, kind; } b[] = {
        {0, 2}, {9, 2}, {10, 1}, {0x63, 1}, {0x64, 0}, {0xFF, 0}
    };
    unsigned i, j;
    for (i = 0; i < sizeof a / sizeof a[0]; ++i) {
        for (j = 0; j < sizeof b / sizeof b[0]; ++j) {
            fixture f;
            pr_rec_0767_io io;
            uint8_t al = 0;
            one_node(&f);
            f.nodes[0].b111 = a[i].input;
            f.nodes[0].b11b = b[j].input;
            io = io_for(&f);
            CHECK(pr_rec_res_00ba_0767(&io, f.nodes[0].address, 0, 1,
                                         &al) == PR_REC_0767_OK);
            CHECK(f.events[4].args[0] == 0x20u + a[i].kind);
            CHECK(f.events[5].args[0] == 0x24u + b[j].kind);
        }
    }
    return 0;
}

static int test_late_memory_reads_and_limit(void) {
    fixture f;
    pr_rec_0767_io io;
    uint8_t al = 0x77;
    one_node(&f);
    f.head = (pr_rec_0767_far){0, 0};
    f.change_head_after_title = 1;
    f.change_fields_after_calls = 1;
    f.nodes[0].next = f.nodes[0].address;
    io = io_for(&f);
    CHECK(pr_rec_res_00ba_0767(&io, f.nodes[0].address, 0, 1,
                                 &al) == PR_REC_0767_OK);
    CHECK(strcmp(f.trace, "ABHFT1U2VNF") == 0 &&
          f.events[5].args[0] == 0x24 && al == 0x9C);

    one_node(&f);
    f.nodes[0].next = f.nodes[0].address;
    io = io_for(&f);
    al = 0x77;
    CHECK(pr_rec_res_00ba_0767(&io, f.nodes[0].address, 0, 1,
                                 &al) == PR_REC_0767_NODE_LIMIT);
    CHECK(strcmp(f.trace, "ABHFT1U2VN") == 0 && al == 0x77);
    return 0;
}

static int test_boundary_failures_and_arguments(void) {
    unsigned fail;
    for (fail = 1; fail <= 11; ++fail) {
        fixture f;
        pr_rec_0767_io io;
        uint8_t al = 0x77;
        one_node(&f);
        io = io_for(&f);
        f.fail_at = fail;
        CHECK(pr_rec_res_00ba_0767(&io, f.nodes[0].address, 0, 1,
                                     &al) == PR_REC_0767_BOUNDARY);
        CHECK(f.calls == fail && al == 0x77);
    }
    {
        fixture f;
        pr_rec_0767_io io;
        uint8_t al = 0x77;
        one_node(&f);
        io = io_for(&f);
        CHECK(pr_rec_res_00ba_0767(NULL, f.nodes[0].address, 0, 1,
                                     &al) == PR_REC_0767_ARGUMENT);
        CHECK(pr_rec_res_00ba_0767(&io, f.nodes[0].address, 0, 1,
                                     NULL) == PR_REC_0767_ARGUMENT);
        io.effect = NULL;
        CHECK(pr_rec_res_00ba_0767(&io, f.nodes[0].address, 0, 1,
                                     &al) == PR_REC_0767_BOUNDARY);
        CHECK(f.calls == 0 && al == 0x77);
    }
    {
        fixture f;
        pr_rec_0767_io io;
        uint8_t al = 0x77;
        one_node(&f);
        io = io_for(&f);
        io.read_head = NULL;
        CHECK(pr_rec_res_00ba_0767(&io, f.nodes[0].address, 0, 1,
                                     &al) == PR_REC_0767_BOUNDARY);
        CHECK(strcmp(f.trace, "AB") == 0 && f.event_count == 2 &&
              al == 0x77);
    }
    {
        fixture f;
        pr_rec_0767_io io;
        uint8_t al = 0x77;
        one_node(&f);
        f.head = (pr_rec_0767_far){0, 0};
        io = io_for(&f);
        io.read_byte = NULL;
        io.read_next = NULL;
        CHECK(pr_rec_res_00ba_0767(&io, f.nodes[0].address, 0, 0,
                                     &al) == PR_REC_0767_OK);
        CHECK(strcmp(f.trace, "ABHF") == 0 && al == 0x9C);
    }
    {
        fixture f;
        pr_rec_0767_io io;
        uint8_t al = 0x77;
        one_node(&f);
        io = io_for(&f);
        io.read_byte = NULL;
        CHECK(pr_rec_res_00ba_0767(&io, f.nodes[0].address, 0, 1,
                                     &al) == PR_REC_0767_BOUNDARY);
        CHECK(strcmp(f.trace, "ABHFT") == 0 && al == 0x77);

        one_node(&f);
        io = io_for(&f);
        io.read_next = NULL;
        CHECK(pr_rec_res_00ba_0767(&io, f.nodes[0].address, 0, 1,
                                     &al) == PR_REC_0767_BOUNDARY);
        CHECK(strcmp(f.trace, "ABHFT1U2V") == 0 && al == 0x77);
    }
    return 0;
}

int main(void) {
    if (test_empty_and_mode() || test_two_nodes_and_identity() ||
        test_classification_boundaries() || test_late_memory_reads_and_limit() ||
        test_boundary_failures_and_arguments()) return 1;
    puts("resident LOAD PIECES 0767 tests passed");
    return 0;
}
