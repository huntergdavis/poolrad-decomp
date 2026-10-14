#include "poolrad/recovered/overlay_u3bd_leaf_call_07f9.h"

#include <assert.h>
#include <string.h>

static uint8_t ram[PR_U3BD_LOOP_RESET_RAM_BYTES];

static size_t at(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & 0xFFFFFu;
}

static uint16_t word(uint16_t segment, uint16_t offset)
{
    return (uint16_t)(ram[at(segment, offset)] |
                      ((uint16_t)ram[at(segment,
                                        (uint16_t)(offset + 1u))] << 8));
}

static void setup(pr_u3bd_leaf_call_frame *frame, uint8_t x, uint8_t y)
{
    pr_u3bd_leaf_call_frame initial = {
        0xA50Eu, 0x22u, 4u, 0xDu, 0x3E68u, 0x4Du, 0x3EB6u,
        0x3E9Eu, 0x14A0u, 0x3DA1u, 0x1D16u, 0x2D14u, 0x7202u
    };
    *frame = initial;
    memset(ram, 0xCC, sizeof ram);
    ram[at(frame->ss, (uint16_t)(frame->bp - 0x0Du))] = x;
    ram[at(frame->ss, (uint16_t)(frame->bp - 0x0Fu))] = y;
}

static void in_grid_preserves_high_byte_and_stack_history(void)
{
    pr_u3bd_leaf_call_frame frame;
    uint16_t ip = 0x07F9u;
    setup(&frame, 0x0Du, 4u);
    assert(pr_u3bd_leaf_call_07f9(ram, sizeof ram, &frame, &ip));
    assert(frame.ax == 0xA501u && frame.sp == 0x3E9Eu &&
           frame.bp == 0x3EB6u && frame.cs == 0x2D14u && ip == 0x080Au);
    assert(frame.flags == 0x7202u && frame.bx == 0x22u &&
           frame.cx == 4u && frame.dx == 0xDu &&
           frame.si == 0x3E68u && frame.di == 0x4Du);
    assert(ram[at(frame.ss, (uint16_t)(frame.bp - 0x14u))] == 0x0Eu);
    assert(word(frame.ss, 0x3E9Cu) == 0xA50Du);
    assert(word(frame.ss, 0x3E9Au) == 0xA504u);
    assert(word(frame.ss, 0x3E98u) == 0x2D14u);
    assert(word(frame.ss, 0x3E96u) == 0x0808u);
    assert(word(frame.ss, 0x3E94u) == 0x3EB6u);
    assert(ram[at(frame.ss, 0x3E93u)] == 1u);
}

static void out_of_grid_returns_zero_without_branch_execution(void)
{
    pr_u3bd_leaf_call_frame frame;
    uint16_t ip = 0x07F9u;
    setup(&frame, 16u, 4u);
    assert(pr_u3bd_leaf_call_07f9(ram, sizeof ram, &frame, &ip));
    assert(frame.ax == 0xA500u && ip == 0x080Au);
    assert((frame.flags & 0x08C5u) == 0x0044u);
    assert(ram[at(frame.ss, 0x3E93u)] == 0u);
    setup(&frame, 0xFFu, 4u);
    ip = 0x07F9u;
    assert(pr_u3bd_leaf_call_07f9(ram, sizeof ram, &frame, &ip));
    assert(frame.ax == 0xA500u && ip == 0x080Au);
    setup(&frame, 2u, 16u);
    ip = 0x07F9u;
    assert(pr_u3bd_leaf_call_07f9(ram, sizeof ram, &frame, &ip));
    assert(frame.ax == 0xA500u && ip == 0x080Au);
    setup(&frame, 2u, 0xFFu);
    ip = 0x07F9u;
    assert(pr_u3bd_leaf_call_07f9(ram, sizeof ram, &frame, &ip));
    assert(frame.ax == 0xA500u && ip == 0x080Au);
    setup(&frame, 15u, 15u);
    ip = 0x07F9u;
    assert(pr_u3bd_leaf_call_07f9(ram, sizeof ram, &frame, &ip));
    assert(frame.ax == 0xA501u && ip == 0x080Au);
}

static void stack_offsets_wrap_without_touching_neighbor(void)
{
    pr_u3bd_leaf_call_frame frame;
    uint16_t ip = 0x07F9u;
    setup(&frame, 2u, 3u);
    frame.sp = 4u;
    assert(pr_u3bd_leaf_call_07f9(ram, sizeof ram, &frame, &ip));
    assert(frame.sp == 4u && ip == 0x080Au);
    assert(word(frame.ss, 2u) == 0xA502u);
    assert(word(frame.ss, 0u) == 0xA503u);
    assert(word(frame.ss, 0xFFFEu) == 0x2D14u);
    assert(word(frame.ss, 0xFFFCu) == 0x0808u);
    assert(word(frame.ss, 0xFFFAu) == 0x3EB6u);
    assert(ram[at(frame.ss, 0xFFF9u)] == 1u);
}

static void invalid_host_inputs_have_no_effect(void)
{
    pr_u3bd_leaf_call_frame frame, saved;
    uint16_t ip = 0x07F9u;
    setup(&frame, 1u, 2u);
    saved = frame;
    assert(!pr_u3bd_leaf_call_07f9(NULL, sizeof ram, &frame, &ip));
    assert(!pr_u3bd_leaf_call_07f9(ram, sizeof ram - 1u, &frame, &ip));
    assert(!pr_u3bd_leaf_call_07f9(ram, sizeof ram, NULL, &ip));
    assert(!pr_u3bd_leaf_call_07f9(ram, sizeof ram, &frame, NULL));
    frame.cs = 0x2D15u;
    assert(!pr_u3bd_leaf_call_07f9(ram, sizeof ram, &frame, &ip));
    frame = saved;
    ip = 0x07FAu;
    assert(!pr_u3bd_leaf_call_07f9(ram, sizeof ram, &frame, &ip));
    assert(memcmp(&frame, &saved, sizeof frame) == 0);
    assert(ram[at(frame.ss, (uint16_t)(frame.bp - 0x14u))] == 0xCCu);
}

int main(void)
{
    in_grid_preserves_high_byte_and_stack_history();
    out_of_grid_returns_zero_without_branch_execution();
    stack_offsets_wrap_without_touching_neighbor();
    invalid_host_inputs_have_no_effect();
    return 0;
}
