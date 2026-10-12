/* Bounded ECL EXIT gate; private input is a hash-checked ignored save. */
#include "poolrad/formats/dos_savgam.h"
#include "poolrad/recovered/overlay_u1c_exit_0001.h"
#include "poolrad/replay/slot_a_native_save.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #x); \
    return 1; \
} } while (0)

typedef struct {
    pr_u1c_exit_env env;
    pr_u1c_exit_report report;
    pr_u1c_exit_status result;
    unsigned calls;
    uint16_t handler_pc;
    uint8_t handler_stop;
} exit_context;

static pr_rec_ecl_handler_status exit_handler(pr_rec_ecl_loader_state *st, void *arg)
{
    exit_context *ctx = arg;
    ctx->calls++;
    ctx->result = pr_u1c_op_exit(st, &ctx->env, &ctx->report);
    ctx->handler_pc = st->pc;
    ctx->handler_stop = st->b442e;
    return ctx->result == PR_U1C_EXIT_OK ? PR_REC_ECL_HANDLER_OK : PR_REC_ECL_HANDLER_BOUNDS;
}

static pr_rec_ecl_run_status run_exit(pr_rec_ecl_loader_state *st, uint16_t pc,
                                      exit_context *ctx, unsigned *steps)
{
    pr_rec_ecl_dispatch_table table = {0};
    table.ctx = ctx;
    table.handler[0] = exit_handler;
    return pr_rec_ecl_run(st, &table, pc, 1u, steps, NULL);
}

static void map_words(uint16_t *dst, const uint8_t *src, size_t count)
{
    size_t i;
    for (i = 0; i < count; ++i)
        dst[i] = (uint16_t)((uint16_t)src[2u * i] | ((uint16_t)src[2u * i + 1u] << 8));
}

static int synthetic(void)
{
    static const uint8_t script[] = {0};
    pr_rec_ecl_loader_state st = {0};
    exit_context ctx = {0};
    unsigned steps = 0;
    st.script = script;
    st.script_size = sizeof script;
    st.latch = 0x36u;
    st.b84c8 = 7u;
    st.b84c9 = 8u;
    st.b84e0 = 9u;
    st.b5e25 = 4u;
    ctx.env.b84de = 5u;
    CHECK(run_exit(&st, 0x9900u, &ctx, &steps) == PR_REC_ECL_RUN_OK);
    CHECK(steps == 1u && ctx.calls == 1u && ctx.result == PR_U1C_EXIT_OK);
    CHECK(st.pc == 0x9901u && ctx.handler_pc == st.pc);
    CHECK(ctx.handler_stop == 1u && st.b442e == 0u);
    CHECK(st.prev_opcode == 0x36u && st.latch == 0u);
    CHECK(st.b84c8 == 0u && st.b84c9 == 0u && st.b84e0 == 0u);
    CHECK(st.b5e24 == 1u && st.b5e25 == 0x11u && ctx.env.b84de == 0u);
    CHECK(ctx.report.boxes == 0u && ctx.report.freed_nodes == 0u);

    /* The box hook is required only when the first branch reaches it. */
    memset(&st, 0, sizeof st);
    memset(&ctx, 0, sizeof ctx);
    st.script = script;
    st.script_size = sizeof script;
    st.b84e2 = 1u;
    CHECK(run_exit(&st, 0x9900u, &ctx, &steps) == PR_REC_ECL_RUN_HANDLER);
    CHECK(steps == 1u && ctx.calls == 1u && ctx.result == PR_U1C_EXIT_HOOK);
    CHECK(st.pc == 0x9900u && st.b84e2 == 1u && st.b5e24 == 0u);
    return 0;
}

static int private_save(const char *path)
{
    static pr_slot_a_native_state saved;
    static pr_rec_ecl_loader_state st;
    uint8_t data[PR_DOS_SAVGAM_SIZE];
    uint16_t before0[PR_REC_ECL_WINDOW0_WORDS];
    uint16_t before1[PR_REC_ECL_WINDOW1_WORDS];
    uint16_t before2[PR_REC_ECL_WINDOW2_WORDS];
    exit_context ctx = {0};
    unsigned steps = 0;
    FILE *f = fopen(path, "rb");
    CHECK(f != NULL);
    CHECK(fread(data, 1u, sizeof data, f) == sizeof data);
    CHECK(fgetc(f) == EOF);
    CHECK(fclose(f) == 0);
    CHECK(pr_slot_a_native_restore(data, sizeof data, &saved) == PR_SLOT_A_NATIVE_OK);
    st.script = saved.script;
    st.script_size = sizeof saved.script;
    map_words(st.window0, saved.window0, PR_REC_ECL_WINDOW0_WORDS);
    map_words(st.window1, saved.window1, PR_REC_ECL_WINDOW1_WORDS);
    map_words(st.window2, saved.window2, PR_REC_ECL_WINDOW2_WORDS);
    CHECK(st.script[0x9B67u - PR_REC_ECL_SCRIPT_BASE] == 0u);
    CHECK(st.window0[PR_U1C_EXIT_W0_WORD] == 1u);
    CHECK(st.window0[PR_SLOT_A_CLOCK_MINUTE_OFFSET / 2u] == 0u);
    CHECK(st.window0[PR_SLOT_A_CLOCK_TENS_OFFSET / 2u] == 5u);
    CHECK(st.window0[PR_SLOT_A_CLOCK_HOUR_OFFSET / 2u] == 10u);
    /* The minute changed after the save. The run010 EXIT-entry RAM snapshot
       independently measures 1; the save itself supplies 0. */
    st.window0[PR_SLOT_A_CLOCK_MINUTE_OFFSET / 2u] = 1u;
    st.window2[0x21Eu / 2u] = 10u; /* run010 measurement after seeded RANDOM */
    st.latch = 0x1Au;             /* dispatcher-06 / EXIT-entry RAM */
    st.b49ff = 0u;
    st.b84e2 = 0u;               /* run010: skip resident box call */
    ctx.env.b4430 = 0u;         /* run010: skip character-pointer restore */
    st.call_depth = 0u;          /* run010: [6F70:6F72] = 0:0, skip FreeMem */
    st.b5e24 = 1u;
    st.b5e25 = 0x11u;
    memcpy(before0, st.window0, sizeof before0);
    memcpy(before1, st.window1, sizeof before1);
    memcpy(before2, st.window2, sizeof before2);
    CHECK(run_exit(&st, 0x9B67u, &ctx, &steps) == PR_REC_ECL_RUN_OK);
    CHECK(steps == 1u && ctx.calls == 1u && ctx.result == PR_U1C_EXIT_OK);
    CHECK(ctx.handler_pc == 0x9B68u && ctx.handler_stop == 1u);
    CHECK(st.pc == 0x9B68u && st.b442e == 0u && st.b49ff == 0u);
    CHECK(st.prev_opcode == 0x1Au && st.latch == 0u);
    CHECK(st.b84c8 == 0u && st.b84c9 == 0u && st.b84e0 == 0u && st.b84e2 == 0u);
    CHECK(st.b5e24 == 1u && st.b5e25 == 0x11u && ctx.env.b84de == 0u);
    CHECK(ctx.report.boxes == 0u && ctx.report.freed_nodes == 0u &&
          ctx.report.restored_character == 0u);
    CHECK(memcmp(st.window0, before0, sizeof before0) == 0);
    CHECK(memcmp(st.window1, before1, sizeof before1) == 0);
    CHECK(memcmp(st.window2, before2, sizeof before2) == 0);
    puts("saved EXIT gate: one dispatch, no reached hook, PC 0x9B68, runner stop reset");
    return 0;
}

int main(int argc, char **argv)
{
    if (argc > 2 || synthetic() != 0) return 1;
    if (argc == 2 && private_save(argv[1]) != 0) return 1;
    puts("synthetic EXIT gate passed");
    return 0;
}
