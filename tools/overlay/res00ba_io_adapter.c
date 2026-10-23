/* RAM-backed io adapters for the RES_00BA pr_rec_* models (1509, 15F7, 23B7, 23DB, 2637, 2E03, 2E46, 2E8B).
 * Reusable: tools/overlay/res00ba_io_adapter.py builds this file with the model sources and exposes one function per body.
 * Every memory callback reads or writes the caller's 1 MiB A20-off RAM image at the exact segment:offset the model passes
 * (offsets already wrapped by the model; physical addresses wrap at 1 MiB). DS-global accesses use the supplied DS.
 * The models' declared boundaries are recorded in call order as (kind, a, b, c) entries:
 *   'A' GetMem(bytes)             -> returns the block the caller supplies (alloc_off/alloc_seg)
 *   'M' Move(src seg:off -> dst)  -> performs the byte copy in RAM (16-bit offset wrap)
 *   'R' FreeMem(seg, off, bytes)  -> recorded; returns 1 (success)
 *   'L' StrLoad literal (seg, off)-> recorded
 *   'P' message parameter         -> recorded */
#include "poolrad/recovered/resident_sidecar_remove.h"
#include "poolrad/recovered/resident_sidecar_append.h"
#include "poolrad/recovered/resident_character_field_zero.h"
#include "poolrad/recovered/resident_party_count.h"
#include "poolrad/recovered/resident_character_target_reset.h"
#include "poolrad/recovered/resident_character_link_check.h"
#include "poolrad/recovered/resident_character_link_flags.h"
#include "poolrad/recovered/resident_character_target_select.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define RAM (1u << 20)
#define LOG_MAX 64u

typedef struct {
    uint8_t *ram;
    uint16_t ds;
    uint16_t alloc_off, alloc_seg;
    uint16_t *log;          /* LOG_MAX entries of 4 words: kind, a, b, c */
    unsigned n;
} adapter;

static size_t at(uint16_t seg, uint16_t off) { return (((size_t)seg << 4) + off) & (RAM - 1u); }
static uint8_t rb(adapter *a, uint16_t s, uint16_t o) { return a->ram[at(s, o)]; }
static void wb(adapter *a, uint16_t s, uint16_t o, uint8_t v) { a->ram[at(s, o)] = v; }
static uint16_t rw(adapter *a, uint16_t s, uint16_t o) { return (uint16_t)(rb(a, s, o) | (rb(a, s, (uint16_t)(o + 1u)) << 8)); }
static void ww(adapter *a, uint16_t s, uint16_t o, uint16_t v) { wb(a, s, o, (uint8_t)v); wb(a, s, (uint16_t)(o + 1u), (uint8_t)(v >> 8)); }
static void note(adapter *a, char kind, uint16_t x, uint16_t y, uint16_t z)
{
    if (a->n < LOG_MAX) { uint16_t *e = a->log + 4u * a->n; e[0] = (uint16_t)kind; e[1] = x; e[2] = y; e[3] = z; }
    a->n++;
}

/* int-returning callbacks */
static int i_read_word(void *c, uint16_t s, uint16_t o, uint16_t *v) { *v = rw(c, s, o); return 1; }
static int i_write_word(void *c, uint16_t s, uint16_t o, uint16_t v) { ww(c, s, o, v); return 1; }
static int i_read_byte(void *c, uint16_t s, uint16_t o, uint8_t *v) { *v = rb(c, s, o); return 1; }
static int i_read_gword(void *c, uint16_t o, uint16_t *v) { adapter *a = c; *v = rw(a, a->ds, o); return 1; }
static int i_read_gbyte(void *c, uint16_t o, uint8_t *v) { adapter *a = c; *v = rb(a, a->ds, o); return 1; }
static int i_write_gbyte(void *c, uint16_t o, uint8_t v) { adapter *a = c; wb(a, a->ds, o, v); return 1; }
/* bool-returning callbacks */
static bool b_read_word(void *c, uint16_t s, uint16_t o, uint16_t *v) { *v = rw(c, s, o); return true; }
static bool b_read_byte(void *c, uint16_t s, uint16_t o, uint8_t *v) { *v = rb(c, s, o); return true; }
static bool b_write_byte(void *c, uint16_t s, uint16_t o, uint8_t v) { wb(c, s, o, v); return true; }
static bool b_write_word(void *c, uint16_t s, uint16_t o, uint16_t v) { ww(c, s, o, v); return true; }
static bool b_read_gbyte(void *c, uint16_t o, uint8_t *v) { adapter *a = c; *v = rb(a, a->ds, o); return true; }
/* boundaries */
static void copy_literal(void *c, uint16_t seg, uint16_t off) { note(c, 'L', seg, off, 0); }
static void present_message(void *c, uint8_t p) { note(c, 'P', p, 0, 0); }
static int release_block(void *c, pr_rec_far_ptr b, uint16_t bytes) { note(c, 'R', b.segment, b.offset, bytes); return 1; }
static int allocate_block(void *c, uint16_t bytes, pr_rec_far_ptr *b)
{
    adapter *a = c;
    note(a, 'A', bytes, 0, 0);
    b->offset = a->alloc_off; b->segment = a->alloc_seg;
    return 1;
}
static int copy_block(void *c, pr_rec_far_ptr s, pr_rec_far_ptr d, uint16_t bytes)
{
    adapter *a = c;
    uint16_t k;
    note(a, 'M', s.segment, s.offset, bytes);
    for (k = 0; k < bytes; ++k)
        wb(a, d.segment, (uint16_t)(d.offset + k), rb(a, s.segment, (uint16_t)(s.offset + k)));
    return 1;
}

#define ADAPTER(r, d, l, cnt) adapter A = {r, d, 0, 0, l, 0}; unsigned *count = cnt
#define DONE(x) do { int status_ = (int)(x); if (count) *count = A.n; return status_; } while (0)

int adapt_res00ba_1509(uint8_t *ram, uint16_t ds, uint16_t t_off, uint16_t t_seg, uint16_t c_off, uint16_t c_seg,
                       size_t max_nodes, uint16_t *log, unsigned *n)
{
    ADAPTER(ram, ds, log, n);
    pr_rec_remove_io io = {&A, i_read_word, i_write_word, copy_literal, present_message, release_block, max_nodes};
    pr_rec_far_ptr t = {t_off, t_seg}, c = {c_off, c_seg};
    DONE(pr_rec_res_00ba_1509(&io, t, c));
}

int adapt_res00ba_15f7(uint8_t *ram, uint16_t ds, uint16_t s_off, uint16_t s_seg, uint16_t c_off, uint16_t c_seg,
                       uint16_t alloc_off, uint16_t alloc_seg, size_t max_nodes, uint16_t *log, unsigned *n)
{
    ADAPTER(ram, ds, log, n);
    pr_rec_append_io io = {&A, allocate_block, copy_block, i_read_word, i_write_word, max_nodes};
    pr_rec_far_ptr s = {s_off, s_seg}, c = {c_off, c_seg};
    A.alloc_off = alloc_off; A.alloc_seg = alloc_seg;
    DONE(pr_rec_res_00ba_15f7(&io, s, c));
}

int adapt_res00ba_23b7(uint8_t *ram, uint16_t ds, uint16_t c_off, uint16_t c_seg, uint8_t *is_zero)
{
    ADAPTER(ram, ds, NULL, NULL);
    pr_rec_character_field_zero_io io = {&A, i_read_byte};
    pr_rec_far_ptr c = {c_off, c_seg};
    DONE(pr_rec_res_00ba_23b7(&io, c, is_zero));
}

int adapt_res00ba_23db(uint8_t *ram, uint16_t ds, size_t max_nodes)
{
    ADAPTER(ram, ds, NULL, NULL);
    pr_rec_party_count_io io = {&A, i_read_gword, i_read_gbyte, i_write_gbyte, i_read_word, i_read_byte, max_nodes};
    DONE(pr_rec_res_00ba_23db(&io));
}

int adapt_res00ba_2637(uint8_t *ram, uint16_t ds, uint16_t c_off, uint16_t c_seg, uint16_t *ax)
{
    ADAPTER(ram, ds, NULL, NULL);
    pr_rec_target_reset_io io = {&A, b_read_word, b_write_byte};
    pr_rec_reset_far_ptr c = {c_off, c_seg};
    DONE(pr_rec_res_00ba_2637(&io, c, ax));
}

int adapt_res00ba_2e03(uint8_t *ram, uint16_t ds, uint16_t c_off, uint16_t c_seg, uint8_t *result)
{
    ADAPTER(ram, ds, NULL, NULL);
    pr_rec_character_link_check_io io = {&A, i_read_word, i_read_byte, i_read_gbyte};
    pr_rec_far_ptr c = {c_off, c_seg};
    DONE(pr_rec_res_00ba_2e03(&io, c, result));
}

int adapt_res00ba_2e46(uint8_t *ram, uint16_t ds, uint16_t c_off, uint16_t c_seg, uint8_t *result)
{
    ADAPTER(ram, ds, NULL, NULL);
    pr_rec_character_link_check_io io = {&A, i_read_word, i_read_byte, i_read_gbyte};
    pr_rec_far_ptr c = {c_off, c_seg};
    DONE(pr_rec_res_00ba_2e46(&io, c, result));
}

int adapt_res00ba_2e8b(uint8_t *ram, uint16_t ds, uint16_t c_off, uint16_t c_seg, uint16_t o_off, uint16_t o_seg, uint16_t *ax)
{
    ADAPTER(ram, ds, NULL, NULL);
    pr_rec_target_select_io io = {&A, b_read_word, b_read_byte, b_read_gbyte, b_write_word};
    pr_rec_select_far_ptr c = {c_off, c_seg}, o = {o_off, o_seg};
    DONE(pr_rec_res_00ba_2e8b(&io, c, o, ax));
}
