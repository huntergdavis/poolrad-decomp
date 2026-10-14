#include "poolrad/recovered/overlay_u77_spell_299c.h"

#include <assert.h>
#include <stdio.h>

typedef struct {
    pr_u77_299c_call calls[16];
    unsigned count;
    uint8_t select_value;
    uint8_t final_value;
} trace;

static uint8_t external(void *context, pr_u77_299c_call call)
{
    trace *t = context;
    assert(t->count < 16u);
    t->calls[t->count++] = call;
    if (call == PR_U77_299C_SELECT) return t->select_value;
    if (call == PR_U77_299C_FINAL) return t->final_value;
    return 0u;
}

static void test_path(uint8_t mode, uint8_t selected, int16_t state,
                      uint8_t final, unsigned expected_count)
{
    uint8_t slot = 0xA5u, result = 0xDEu;
    trace t = {{0}, 0, selected, final};
    unsigned position = 0;
    assert(pr_u77_spell_299c(mode, &slot, &state, external, &t, &result) ==
           PR_U77_299C_OK);
    assert(slot == selected);
    assert(result == (selected ? final : 0u));
    assert(t.count == expected_count);
    if (mode <= 6u) assert(t.calls[position++] == (pr_u77_299c_call)mode);
    assert(t.calls[position++] == PR_U77_299C_SELECT);
    if (selected && state < 0) {
        assert(t.calls[position++] == PR_U77_299C_MESSAGE_1);
        assert(t.calls[position++] == PR_U77_299C_PREPARE);
        assert(t.calls[position++] == PR_U77_299C_ACTIVE_CALL);
        assert(t.calls[position++] == PR_U77_299C_MESSAGE_2);
        assert(t.calls[position++] == PR_U77_299C_APPEND);
        assert(t.calls[position++] == PR_U77_299C_DISPLAY);
    }
    if (selected) assert(t.calls[position++] == PR_U77_299C_FINAL);
    assert(position == expected_count);
}

int main(void)
{
    uint8_t slot = 0xA5u, result = 0xDEu;
    int16_t state = -1;
    trace t = {{0}, 0, 1, 2};
    for (uint8_t mode = 0; mode <= 6u; ++mode) {
        test_path(mode, 0u, -1, 0x79u, 2u);
        test_path(mode, 1u, 0, 0x79u, 3u);
        test_path(mode, 2u, -1, 0x79u, 9u);
    }
    test_path(7u, 0u, -1, 0x79u, 1u);
    test_path(255u, 1u, 32767, 0x88u, 2u);
    test_path(255u, 255u, -32768, 0x88u, 8u);
    assert(pr_u77_spell_299c(0, NULL, &state, external, &t, &result) ==
           PR_U77_299C_ARGUMENT);
    assert(pr_u77_spell_299c(0, &slot, NULL, external, &t, &result) ==
           PR_U77_299C_ARGUMENT);
    assert(pr_u77_spell_299c(0, &slot, &state, NULL, &t, &result) ==
           PR_U77_299C_ARGUMENT);
    assert(pr_u77_spell_299c(0, &slot, &state, external, &t, NULL) ==
           PR_U77_299C_ARGUMENT);
    assert(slot == 0xA5u && result == 0xDEu && t.count == 0u);
    puts("unit 0x77 +299C control tests passed");
    return 0;
}
