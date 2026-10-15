#include "poolrad/recovered/overlay_u3bd_post_up_02c4_tile_call.h"
#include "poolrad/recovered/overlay_u40c_tile_plane.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "check failed at line %d\n", __LINE__); \
        return 1; \
    } \
} while (0)

enum {
    RAM_BYTES = 0xA0000u, OBJECT_BYTES = 14135u,
    SS = 0x1D16u, DS = 0x14A0u, BP = 0x3EBAu,
    ENTRY_SP = 0x3EACu, CALL_SP = 0x3EA2u
};

static uint32_t physical(uint16_t segment, uint16_t offset)
{
    return (((uint32_t)segment << 4) + offset) & 0xFFFFFu;
}

static uint16_t word_at(const uint8_t *p)
{
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}

static void put_word(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

static void seed(uint8_t *ram, pr_u3bd_post_up_02c4_tile_call_state *s)
{
    memset(s, 0, sizeof *s);
    s->ram = ram;
    s->ram_size = RAM_BYTES;
    s->ss = SS;
    s->ds = DS;
    s->bp = BP;
    s->sp = ENTRY_SP;
    s->ax = 0x0888u;
    s->cx = 5u;
    s->es = 0x3D0Eu;
    s->di = 0x0895u;
    s->flags = 0x7202u;
    s->known_arith_flags = 0x08D5u;
    put_word(ram + physical(SS, BP - 6u), 4u);
    put_word(ram + physical(SS, BP - 8u), 6u);
    ram[physical(SS, BP - 9u)] = 5u;
    ram[physical(SS, BP + 8u)] = 14u;
    put_word(ram + physical(DS, 0x6A58u), 8u);
    put_word(ram + physical(DS, 0x6A5Au), 0x3D0Eu);
    ram[physical(0x3D0Eu, 0x07F9u)] = 19u;
}

typedef struct {
    pr_u3bd_post_up_02c4_tile_call_state *state;
    const uint8_t *native_call;
    const uint8_t *native_return;
    unsigned calls;
    unsigned bytes_written, bytes_changed;
} callback_context;

static int callback(void *opaque, uint16_t x, uint16_t y, uint16_t item,
                    uint16_t one_a, uint16_t one_b)
{
    callback_context *ctx = opaque;
    pr_u3bd_post_up_02c4_tile_call_state *s = ctx->state;
    uint32_t stack = physical(SS, CALL_SP);
    uint32_t source, aux, dest;
    pr_u40c_tile_plane_state plane = {0};
    pr_u40c_tile_plane_report report;
    pr_u40c_tile_plane_status status;
    ++ctx->calls;
    if (x != 6u || y != 8u || item != 19u ||
        one_a != 1u || one_b != 1u || s->sp != CALL_SP ||
        word_at(s->ram + stack) != 1u ||
        word_at(s->ram + stack + 2u) != 1u ||
        word_at(s->ram + stack + 4u) != 19u ||
        word_at(s->ram + stack + 6u) != 8u ||
        word_at(s->ram + stack + 8u) != 6u)
        return 1;
    if (ctx->native_call == NULL) return 0;
    if (memcmp(s->ram, ctx->native_call, RAM_BYTES) != 0)
        return 1;

    /* The already recovered entry-2 model is the typed external callback. */
    source = physical(word_at(s->ram + physical(DS, 0x5E3Cu)),
                      word_at(s->ram + physical(DS, 0x5E3Au)));
    aux = physical(word_at(s->ram + source + 0x15u),
                   word_at(s->ram + source + 0x13u));
    dest = physical(word_at(s->ram + physical(DS, 0x4A22u)),
                    word_at(s->ram + physical(DS, 0x4A20u)));
    if (source >= RAM_BYTES || aux >= RAM_BYTES ||
        dest + OBJECT_BYTES > RAM_BYTES)
        return 1;
    plane.item_id = item;
    plane.group0_remap = word_at(s->ram + physical(DS, 0x2722u));
    plane.x = (uint8_t)x;
    plane.y = (uint8_t)y;
    plane.flag = (uint8_t)one_a;
    plane.video_state = s->ram[physical(DS, 0x4A1Eu)];
    plane.source = s->ram + source;
    plane.source_size = RAM_BYTES - source;
    plane.aux_mask = s->ram + aux;
    plane.aux_mask_size = RAM_BYTES - aux;
    plane.destination = s->ram + dest;
    plane.destination_size = RAM_BYTES - dest;
    status = pr_u40c_entry2_mode1_tile_plane(&plane, &report);
    if (status != PR_U40C_TILE_PLANE_OK) return 1;
    ctx->bytes_written = report.bytes_written;
    ctx->bytes_changed = report.bytes_changed;
    if (memcmp(s->ram + dest, ctx->native_return + dest, OBJECT_BYTES) != 0 ||
        memcmp(s->ram + physical(DS, 0x4A28u),
               ctx->native_return + physical(DS, 0x4A28u), 1176u) != 0)
        return 1;
    return 0;
}

static int fail_callback(void *opaque, uint16_t x, uint16_t y, uint16_t item,
                         uint16_t one_a, uint16_t one_b)
{
    (void)opaque; (void)x; (void)y; (void)item; (void)one_a; (void)one_b;
    return 1;
}

static int synthetic(void)
{
    uint8_t *ram = calloc(RAM_BYTES, 1u);
    pr_u3bd_post_up_02c4_tile_call_state s;
    pr_u3bd_post_up_02c4_tile_call_report report;
    callback_context ctx = {0};
    uint32_t stack = physical(SS, CALL_SP);
    CHECK(ram != NULL);

    seed(ram, &s);
    CHECK(pr_u3bd_post_up_02c4_tile_call(&s, NULL, NULL, &report) ==
          PR_U3BD_POST_UP_02C4_TILE_CALL_CALLBACK_REQUIRED);
    CHECK(report.x == 6u && report.y == 8u && report.item == 19u &&
          report.one_a == 1u && report.one_b == 1u &&
          report.call_sp == CALL_SP && report.next_ip == 0x02FBu);
    CHECK(s.sp == CALL_SP && s.ax == 1u && s.cx == 5u && s.dx == 0u &&
          s.es == 0x3D0Eu && s.di == 0x0895u);
    CHECK(s.flags == 0x7246u && s.known_arith_flags == 0x08C5u);
    CHECK(word_at(ram + stack) == 1u &&
          word_at(ram + stack + 2u) == 1u &&
          word_at(ram + stack + 4u) == 19u &&
          word_at(ram + stack + 6u) == 8u &&
          word_at(ram + stack + 8u) == 6u);

    seed(ram, &s);
    ctx.state = &s;
    CHECK(pr_u3bd_post_up_02c4_tile_call(&s, callback, &ctx, &report) ==
          PR_U3BD_POST_UP_02C4_TILE_CALL_OK);
    CHECK(ctx.calls == 1u && report.next_ip == 0x0300u && s.sp == ENTRY_SP);

    seed(ram, &s);
    CHECK(pr_u3bd_post_up_02c4_tile_call(&s, fail_callback, NULL, &report) ==
          PR_U3BD_POST_UP_02C4_TILE_CALL_CALLBACK_ERROR);
    CHECK(s.sp == CALL_SP && report.next_ip == 0x02FBu);

    /* A missing item preserves the two earlier coordinate pushes. */
    seed(ram, &s);
    put_word(ram + physical(DS, 0x6A5Au), 0x4000u);
    s.ram_size = 0x30000u;
    CHECK(pr_u3bd_post_up_02c4_tile_call(&s, callback, &ctx, &report) ==
          PR_U3BD_POST_UP_02C4_TILE_CALL_BOUNDARY);
    CHECK(s.sp == ENTRY_SP - 4u &&
          word_at(ram + physical(SS, s.sp)) == 8u &&
          word_at(ram + physical(SS, s.sp + 2u)) == 6u);

    s.ram = NULL;
    CHECK(pr_u3bd_post_up_02c4_tile_call(&s, NULL, NULL, &report) ==
          PR_U3BD_POST_UP_02C4_TILE_CALL_ARGUMENT);
    free(ram);
    return 0;
}

static int load_ram(const char *path, uint8_t *ram)
{
    FILE *fp = fopen(path, "rb");
    size_t count;
    int extra;
    if (fp == NULL) return 0;
    count = fread(ram, 1u, RAM_BYTES, fp);
    extra = fgetc(fp);
    if (fclose(fp) != 0) return 0;
    return count == RAM_BYTES && extra == EOF;
}

static int private_triplet(const char *entry_path, const char *call_path,
                           const char *return_path)
{
    uint8_t *entry = malloc(RAM_BYTES);
    uint8_t *call = malloc(RAM_BYTES);
    uint8_t *ret = malloc(RAM_BYTES);
    pr_u3bd_post_up_02c4_tile_call_state s = {0};
    pr_u3bd_post_up_02c4_tile_call_report report;
    callback_context ctx = {0};
    CHECK(entry != NULL && call != NULL && ret != NULL);
    CHECK(load_ram(entry_path, entry) && load_ram(call_path, call) &&
          load_ram(return_path, ret));
    s.ram = entry;
    s.ram_size = RAM_BYTES;
    s.ss = SS;
    s.ds = DS;
    s.bp = BP;
    s.sp = ENTRY_SP;
    s.ax = 0x0888u;
    s.cx = 5u;
    s.es = 0x3D0Eu;
    s.di = 0x0895u;
    s.flags = 0x7202u;
    s.known_arith_flags = 0x08D5u;
    ctx.state = &s;
    ctx.native_call = call;
    ctx.native_return = ret;
    CHECK(pr_u3bd_post_up_02c4_tile_call(&s, callback, &ctx, &report) ==
          PR_U3BD_POST_UP_02C4_TILE_CALL_OK);
    CHECK(ctx.calls == 1u && report.item_physical == 0x3D8D9u &&
          report.next_ip == 0x0300u && s.sp == ENTRY_SP);
    CHECK(ctx.bytes_written == 32u && ctx.bytes_changed == 8u);
    puts("pre-call RAM 655360/655360 exact; fourth callback object "
         "14135/14135 and metadata 1176/1176 exact");
    free(ret);
    free(call);
    free(entry);
    return 0;
}

int main(int argc, char **argv)
{
    if (synthetic() != 0) return 1;
    if (argc == 1) return 0;
    if (argc != 4) return 2;
    return private_triplet(argv[1], argv[2], argv[3]);
}
