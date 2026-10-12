/* One bounded saved ECL run from RANDOM through EXIT. No game bytes are here. */
#include "poolrad/formats/dos_savgam.h"
#include "poolrad/recovered/overlay_u1c_exit_0001.h"
#include "poolrad/recovered/overlay_u1c_random_0234.h"
#include "poolrad/replay/slot_a_native_save.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #x); \
    return 1; \
} } while (0)

typedef struct {
    uint16_t pc, next_pc;
    uint8_t previous, opcode, latch_after, stop_after;
    uint8_t flags_before[6], flags_after[6];
    uint16_t windows_before[6], windows_after[6];
} chain_step;

typedef struct {
    chain_step step[8];
    unsigned count;
    uint32_t *seed;
    pr_u1c_random_status random_status;
    pr_u1c_random_report random_report;
    pr_u1c_exit_status exit_status;
    pr_u1c_exit_report exit_report;
    pr_u1c_exit_env exit_env;
} chain_context;

static void measured_windows(const pr_rec_ecl_loader_state *st, uint16_t words[6])
{
    words[0] = st->window0[PR_SLOT_A_CLOCK_MINUTE_OFFSET / 2u];
    words[1] = st->window0[PR_SLOT_A_CLOCK_TENS_OFFSET / 2u];
    words[2] = st->window0[PR_SLOT_A_CLOCK_HOUR_OFFSET / 2u];
    words[3] = st->window1[0x594u / 2u];
    words[4] = st->window2[0x21Eu / 2u];
    words[5] = st->window0[0x216u / 2u];
}

static chain_step *begin_step(pr_rec_ecl_loader_state *st, chain_context *ctx)
{
    chain_step *step;
    if (ctx->count >= sizeof ctx->step / sizeof ctx->step[0]) return NULL;
    step = &ctx->step[ctx->count++];
    step->pc = st->pc;
    step->previous = st->prev_opcode;
    step->opcode = st->latch;
    memcpy(step->flags_before, st->cmp_flags, sizeof step->flags_before);
    measured_windows(st, step->windows_before);
    return step;
}

static void end_step(pr_rec_ecl_loader_state *st, chain_step *step)
{
    step->next_pc = st->pc;
    step->latch_after = st->latch;
    step->stop_after = st->b442e;
    memcpy(step->flags_after, st->cmp_flags, sizeof step->flags_after);
    measured_windows(st, step->windows_after);
}

static pr_rec_ecl_handler_status random_cb(pr_rec_ecl_loader_state *st, void *arg)
{
    chain_context *ctx = arg;
    chain_step *step = begin_step(st, ctx);
    if (!step) return PR_REC_ECL_HANDLER_BOUNDS;
    ctx->random_status = pr_u1c_op_random_seeded(st, ctx->seed, &ctx->random_report);
    end_step(st, step);
    if (ctx->random_status == PR_U1C_RANDOM_OK) return PR_REC_ECL_HANDLER_OK;
    if (ctx->random_status == PR_U1C_RANDOM_CORE) return ctx->random_report.core_status;
    return PR_REC_ECL_HANDLER_UNRESOLVED;
}

static pr_rec_ecl_handler_status compare_cb(pr_rec_ecl_loader_state *st, void *arg)
{
    chain_step *step = begin_step(st, arg);
    pr_rec_ecl_handler_status status;
    if (!step) return PR_REC_ECL_HANDLER_BOUNDS;
    status = pr_rec_ecl_op_compare(st);
    end_step(st, step);
    return status;
}

static pr_rec_ecl_handler_status if_cb(pr_rec_ecl_loader_state *st, void *arg)
{
    chain_step *step = begin_step(st, arg);
    pr_rec_ecl_handler_status status;
    if (!step) return PR_REC_ECL_HANDLER_BOUNDS;
    status = pr_rec_ecl_op_if(st);
    end_step(st, step);
    return status;
}

static pr_rec_ecl_handler_status exit_cb(pr_rec_ecl_loader_state *st, void *arg)
{
    chain_context *ctx = arg;
    chain_step *step = begin_step(st, ctx);
    if (!step) return PR_REC_ECL_HANDLER_BOUNDS;
    ctx->exit_status = pr_u1c_op_exit(st, &ctx->exit_env, &ctx->exit_report);
    end_step(st, step);
    return ctx->exit_status == PR_U1C_EXIT_OK ? PR_REC_ECL_HANDLER_OK : PR_REC_ECL_HANDLER_UNRESOLVED;
}

static pr_rec_ecl_run_status run_chain(pr_rec_ecl_loader_state *st, uint16_t entry,
                                       chain_context *ctx, unsigned *steps,
                                       pr_rec_ecl_handler_status *handler_status)
{
    pr_rec_ecl_dispatch_table table = {0};
    table.ctx = ctx;
    table.handler[0x08u] = random_cb;
    table.handler[0x03u] = compare_cb;
    table.handler[0x16u] = if_cb;
    table.handler[0x1Au] = if_cb;
    table.handler[0x00u] = exit_cb;
    return pr_rec_ecl_run(st, &table, entry, 8u, steps, handler_status);
}

static void map_words(uint16_t *dst, const uint8_t *src, size_t count)
{
    size_t i;
    for (i = 0; i < count; ++i)
        dst[i] = (uint16_t)((uint16_t)src[2u * i] | ((uint16_t)src[2u * i + 1u] << 8));
}

static int synthetic(void)
{
    /* Four handlers at a different PC, operands and destination. */
    static const uint8_t script[] = {
        0x08u, 0x00u, 4u, 0x01u, 0x10u, 0x97u,
        0x03u, 0x01u, 0x10u, 0x97u, 0x00u, 5u,
        0x1Au, 0x00u
    };
    pr_rec_ecl_loader_state st = {0};
    chain_context ctx = {0};
    pr_rec_ecl_handler_status hs;
    unsigned steps = 0;
    uint32_t seed = 0u;
    st.script = script;
    st.script_size = sizeof script;
    st.window2[0x10u] = 17u;
    ctx.seed = &seed;
    CHECK(run_chain(&st, 0x9900u, &ctx, &steps, &hs) == PR_REC_ECL_RUN_OK);
    CHECK(hs == PR_REC_ECL_HANDLER_OK && steps == 4u && ctx.count == 4u);
    CHECK(ctx.random_status == PR_U1C_RANDOM_OK && ctx.exit_status == PR_U1C_EXIT_OK);
    CHECK(ctx.random_report.result == 0u && ctx.random_report.dest == 0x9710u);
    CHECK(seed == 1u && st.window2[0x10u] == 0u);
    CHECK(ctx.step[0].pc == 0x9900u && ctx.step[0].next_pc == 0x9906u);
    CHECK(ctx.step[1].pc == 0x9906u && ctx.step[1].next_pc == 0x990Cu);
    CHECK(ctx.step[2].pc == 0x990Cu && ctx.step[2].next_pc == 0x990Du);
    CHECK(ctx.step[3].pc == 0x990Du && ctx.step[3].next_pc == 0x990Eu);
    CHECK(ctx.step[3].stop_after == 1u && st.b442e == 0u && st.pc == 0x990Eu);

    memset(&st, 0, sizeof st);
    memset(&ctx, 0, sizeof ctx);
    st.script = script;
    st.script_size = sizeof script;
    st.window2[0x10u] = 17u;
    CHECK(run_chain(&st, 0x9900u, &ctx, &steps, &hs) == PR_REC_ECL_RUN_HANDLER);
    CHECK(hs == PR_REC_ECL_HANDLER_UNRESOLVED && steps == 1u && ctx.count == 1u);
    CHECK(ctx.random_status == PR_U1C_RANDOM_SEED_MISSING && st.window2[0x10u] == 17u);
    CHECK(st.pc == 0x9906u && st.b442e == 0u);
    return 0;
}

static int private_save(const char *path)
{
    static const uint8_t below[6] = {0u, 1u, 1u, 0u, 1u, 0u};
    static const uint16_t pc[8] = {0x9B3Au, 0x9B40u, 0x9B46u, 0x9B50u,
                                    0x9B56u, 0x9B60u, 0x9B66u, 0x9B67u};
    static const uint16_t next[8] = {0x9B40u, 0x9B46u, 0x9B50u, 0x9B56u,
                                      0x9B60u, 0x9B66u, 0x9B67u, 0x9B68u};
    static const uint8_t previous[8] = {0x00u, 0x08u, 0x03u, 0x04u,
                                         0x03u, 0x04u, 0x03u, 0x1Au};
    static const uint8_t opcode[8] = {0x08u, 0x03u, 0x16u, 0x03u,
                                       0x16u, 0x03u, 0x1Au, 0x00u};
    static const uint8_t latch_after[8] = {0x08u, 0x03u, 0x04u, 0x03u,
                                            0x04u, 0x03u, 0x1Au, 0x00u};
    static const uint16_t before_random[6] = {1u, 5u, 10u, 0u, 16u, 0u};
    static const uint16_t after_random[6] = {1u, 5u, 10u, 0u, 10u, 0u};
    static pr_slot_a_native_state saved;
    static pr_rec_ecl_loader_state st;
    uint8_t data[PR_DOS_SAVGAM_SIZE];
    uint16_t before0[PR_REC_ECL_WINDOW0_WORDS], before1[PR_REC_ECL_WINDOW1_WORDS];
    uint16_t before2[PR_REC_ECL_WINDOW2_WORDS];
    chain_context ctx = {0};
    pr_rec_ecl_handler_status hs;
    uint32_t seed = UINT32_C(3140872353);
    unsigned steps = 0, i;
    FILE *f = fopen(path, "rb");
    CHECK(f != NULL);
    CHECK(fread(data, 1u, sizeof data, f) == sizeof data);
    CHECK(fgetc(f) == EOF && fclose(f) == 0);
    CHECK(pr_slot_a_native_restore(data, sizeof data, &saved) == PR_SLOT_A_NATIVE_OK);
    st.script = saved.script;
    st.script_size = sizeof saved.script;
    map_words(st.window0, saved.window0, PR_REC_ECL_WINDOW0_WORDS);
    map_words(st.window1, saved.window1, PR_REC_ECL_WINDOW1_WORDS);
    map_words(st.window2, saved.window2, PR_REC_ECL_WINDOW2_WORDS);
    CHECK(st.window0[PR_SLOT_A_CLOCK_MINUTE_OFFSET / 2u] == 0u);
    CHECK(st.window0[PR_U1C_EXIT_W0_WORD] == 1u);
    CHECK(st.window1[0x594u / 2u] == 0u && st.window0[0x216u / 2u] == 0u);
    CHECK(st.window2[0x21Eu / 2u] == 16u);
    CHECK(st.script[0x9B3Au - PR_REC_ECL_SCRIPT_BASE] == 0x08u);
    CHECK(st.script[0x9B67u - PR_REC_ECL_SCRIPT_BASE] == 0x00u);
    /* Pre-RNG run010 RAM, after the seed was restored, supplies these live
       post-key fields. The save itself has no minute increment or seed. */
    st.window0[PR_SLOT_A_CLOCK_MINUTE_OFFSET / 2u] = 1u;
    memcpy(st.cmp_flags, below, sizeof below);
    /* At the native +026C stop, previous/current is 00/08. The runner
       copies the pre-fetch latch to previous before fetching opcode 08. */
    st.latch = 0x00u;
    st.b49ff = 0u;
    st.b84e2 = 0u;
    st.b5e24 = 1u;
    st.b5e25 = 0x11u;
    ctx.exit_env.b4430 = 0u;
    ctx.exit_env.b84de = 0u;
    ctx.seed = &seed;
    memcpy(before0, st.window0, sizeof before0);
    memcpy(before1, st.window1, sizeof before1);
    memcpy(before2, st.window2, sizeof before2);
    CHECK(run_chain(&st, 0x9B3Au, &ctx, &steps, &hs) == PR_REC_ECL_RUN_OK);
    CHECK(hs == PR_REC_ECL_HANDLER_OK && steps == 8u && ctx.count == 8u);
    CHECK(ctx.random_status == PR_U1C_RANDOM_OK && ctx.exit_status == PR_U1C_EXIT_OK);
    CHECK(ctx.random_report.slot_byte == 13u && ctx.random_report.argument == 14u);
    CHECK(ctx.random_report.dest == 0x980Fu && ctx.random_report.result == 10u &&
          ctx.random_report.stored == 1u);
    CHECK(seed == UINT32_C(3238563622));
    for (i = 0u; i < 8u; ++i) {
        const chain_step *step = &ctx.step[i];
        CHECK(step->pc == pc[i] && step->next_pc == next[i]);
        CHECK(step->previous == previous[i] && step->opcode == opcode[i]);
        CHECK(step->latch_after == latch_after[i]);
        CHECK(step->stop_after == (i == 7u ? 1u : 0u));
        CHECK(memcmp(step->flags_before, below, sizeof below) == 0);
        CHECK(memcmp(step->flags_after, below, sizeof below) == 0);
        CHECK(memcmp(step->windows_before, i == 0u ? before_random : after_random,
                     sizeof before_random) == 0);
        CHECK(memcmp(step->windows_after, after_random, sizeof after_random) == 0);
    }
    CHECK(ctx.exit_report.boxes == 0u && ctx.exit_report.freed_nodes == 0u &&
          ctx.exit_report.restored_character == 0u);
    CHECK(st.pc == 0x9B68u && st.b442e == 0u && st.b49ff == 0u);
    CHECK(st.prev_opcode == 0x1Au && st.latch == 0u);
    CHECK(st.b84c8 == 0u && st.b84c9 == 0u && st.b84e0 == 0u && st.b84e2 == 0u);
    CHECK(st.b5e24 == 1u && st.b5e25 == 0x11u && ctx.exit_env.b84de == 0u);
    CHECK(memcmp(st.window0, before0, sizeof before0) == 0);
    CHECK(memcmp(st.window1, before1, sizeof before1) == 0);
    before2[0x21Eu / 2u] = 10u;
    CHECK(memcmp(st.window2, before2, sizeof before2) == 0);
    puts("saved chain: RANDOM + six COMPARE/IF + EXIT, 8 dispatches, PC 0x9B68, stop clear");
    return 0;
}

int main(int argc, char **argv)
{
    if (argc > 2 || synthetic() != 0) return 1;
    if (argc == 2 && private_save(argv[1]) != 0) return 1;
    puts("synthetic RANDOM-to-EXIT chain passed");
    return 0;
}
