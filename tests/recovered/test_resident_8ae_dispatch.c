#include "poolrad/recovered/resident_8ae_dispatch.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    pr_rec_8ae_call calls[8];
    uint8_t count;
    uint16_t fail_site;
    uint16_t mutate_site;
    uint8_t mutate_mode;
    uint16_t mutate_active;
} fixture;

static int record(void *context, pr_rec_8ae_state *state,
                  const pr_rec_8ae_call *call)
{
    fixture *f = context;
    assert(f->count < 8u);
    f->calls[f->count++] = *call;
    if (call->site == f->mutate_site) {
        state->byte_5372 = f->mutate_mode;
        state->word_5374 = f->mutate_active;
    }
    return call->site == f->fail_site;
}

static pr_rec_8ae_state setup(const pr_rec_8ae_hooks *hooks,
                               uint8_t mode, uint16_t active,
                               uint8_t previous)
{
    pr_rec_8ae_state state = {mode, active, previous, hooks};
    return state;
}

static void expect(const fixture *f, uint8_t index, uint16_t site,
                   pr_rec_8ae_target target, uint8_t has_word,
                   uint16_t word)
{
    assert(index < f->count);
    assert(f->calls[index].site == site);
    assert(f->calls[index].target == target);
    assert(f->calls[index].has_word == has_word);
    assert(f->calls[index].word == word);
}

static void test_gate_and_direct_leaf(void)
{
    pr_rec_8ae_result result;
    pr_rec_8ae_state state = {2u, 1u, 9u, NULL};
    assert(pr_rec_res_08ae_00dc(&state, 0u, &result) == PR_REC_8AE_OK);
    assert(result.completed_calls == 0u && result.stop_offset == 0x017bu);
    assert(state.driver_byte_020d == 9u);

    state.byte_5372 = 0u;
    state.word_5374 = 0u;
    assert(pr_rec_res_08ae_00dc(&state, 1u, &result) == PR_REC_8AE_OK);
    assert(result.completed_calls == 0u && result.stop_offset == 0x017bu);
    assert(state.driver_byte_020d == 1u);
    assert(pr_rec_res_08ae_00dc(NULL, 1u, &result) == PR_REC_8AE_ARGUMENT);
    assert(result.stop_offset == 0x00dcu);
}

static void test_argument_branches(void)
{
    fixture f = {0};
    pr_rec_8ae_hooks hooks = {&f, record};
    pr_rec_8ae_result result;
    pr_rec_8ae_state state = setup(&hooks, 0u, 0u, 7u);

    assert(pr_rec_res_08ae_00dc(&state, 0u, &result) == PR_REC_8AE_OK);
    assert(f.count == 2u && result.completed_calls == 2u);
    expect(&f, 0u, 0x0100u, PR_REC_8AE_CALL_17AE, 1u, 0u);
    expect(&f, 1u, 0x0107u, PR_REC_8AE_CALL_1742, 1u, 0u);
    assert(state.driver_byte_020d == 0u);

    f.count = 0u;
    state.driver_byte_020d = 7u;
    assert(pr_rec_res_08ae_00dc(&state, 0x00ffu, &result) == PR_REC_8AE_OK);
    assert(f.count == 2u);
    expect(&f, 0u, 0x012bu, PR_REC_8AE_CALL_17AE, 1u, 0u);
    expect(&f, 1u, 0x0132u, PR_REC_8AE_CALL_1742, 1u, 0u);
    assert(state.driver_byte_020d == 7u);

    f.count = 0u;
    assert(pr_rec_res_08ae_00dc(&state, 0x0100u, &result) == PR_REC_8AE_OK);
    assert(f.count == 1u);
    expect(&f, 0u, 0x0153u, PR_REC_8AE_CALL_17AE, 1u, 0x00ffu);

    f.count = 0u;
    state.byte_5372 = 1u;
    assert(pr_rec_res_08ae_00dc(&state, 2u, &result) == PR_REC_8AE_OK);
    assert(f.count == 1u);
    expect(&f, 0u, 0x0144u, PR_REC_8AE_CALL_1742, 1u, 1u);

    f.count = 0u;
    state.byte_5372 = 3u;
    assert(pr_rec_res_08ae_00dc(&state, 2u, &result) == PR_REC_8AE_OK);
    assert(f.count == 0u);
}

static void test_active_and_reread(void)
{
    fixture f = {0};
    pr_rec_8ae_hooks hooks = {&f, record};
    pr_rec_8ae_result result;
    pr_rec_8ae_state state = setup(&hooks, 1u, 1u, 6u);

    assert(pr_rec_res_08ae_00dc(&state, 2u, &result) == PR_REC_8AE_OK);
    assert(f.count == 6u && result.completed_calls == 6u);
    expect(&f, 0u, 0x00f1u, PR_REC_8AE_CALL_137C, 0u, 0u);
    expect(&f, 1u, 0x0144u, PR_REC_8AE_CALL_1742, 1u, 1u);
    expect(&f, 2u, 0x0162u, PR_REC_8AE_CALL_006C, 1u, 10u);
    expect(&f, 3u, 0x0169u, PR_REC_8AE_CALL_17AE, 1u, 0u);
    expect(&f, 4u, 0x0170u, PR_REC_8AE_CALL_1742, 1u, 0u);
    expect(&f, 5u, 0x0174u, PR_REC_8AE_CALL_13B1, 0u, 0u);
    assert(result.stop_offset == 0x017bu);

    f.count = 0u;
    f.mutate_site = 0x00f1u;
    f.mutate_mode = 0u;
    f.mutate_active = 0u;
    state.byte_5372 = 1u;
    state.word_5374 = 1u;
    assert(pr_rec_res_08ae_00dc(&state, 2u, &result) == PR_REC_8AE_OK);
    assert(f.count == 2u);
    expect(&f, 1u, 0x0153u, PR_REC_8AE_CALL_17AE, 1u, 1u);

    f.count = 0u;
    f.mutate_site = 0x0144u;
    f.mutate_mode = 1u;
    f.mutate_active = 1u;
    state.byte_5372 = 1u;
    state.word_5374 = 0u;
    assert(pr_rec_res_08ae_00dc(&state, 2u, &result) == PR_REC_8AE_OK);
    assert(f.count == 5u);
    expect(&f, 0u, 0x0144u, PR_REC_8AE_CALL_1742, 1u, 1u);
    expect(&f, 1u, 0x0162u, PR_REC_8AE_CALL_006C, 1u, 10u);
}

static void test_reached_boundary(void)
{
    fixture f = {0};
    pr_rec_8ae_hooks hooks = {&f, record};
    pr_rec_8ae_result result;
    pr_rec_8ae_state state = setup(&hooks, 0u, 0u, 5u);

    f.fail_site = 0x0107u;
    assert(pr_rec_res_08ae_00dc(&state, 0u, &result) ==
           PR_REC_8AE_BOUNDARY);
    assert(f.count == 2u && result.completed_calls == 1u);
    assert(result.stop_offset == 0x0107u);
    assert(state.driver_byte_020d == 5u);

    state.word_5374 = 1u;
    state.hooks = NULL;
    assert(pr_rec_res_08ae_00dc(&state, 1u, &result) ==
           PR_REC_8AE_BOUNDARY);
    assert(result.stop_offset == 0x00f1u && result.completed_calls == 0u);
    assert(state.driver_byte_020d == 5u);
}

int main(void)
{
    test_gate_and_direct_leaf();
    test_argument_branches();
    test_active_and_reread();
    test_reached_boundary();
    return 0;
}
