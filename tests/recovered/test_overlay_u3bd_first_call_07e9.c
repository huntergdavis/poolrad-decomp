#include "poolrad/recovered/overlay_u3bd_first_call_07e9.h"

#include <assert.h>
#include <string.h>

static uint8_t ram[PR_U3BD_LOOP_RESET_RAM_BYTES];

static size_t at(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & 0xFFFFFu;
}

static uint16_t get_word(uint16_t seg, uint16_t off)
{
    return (uint16_t)(ram[at(seg, off)] |
                      ((uint16_t)ram[at(seg, (uint16_t)(off + 1u))] << 8));
}

static void call_handoff_preserves_high_byte_and_flags(void)
{
    pr_u3bd_first_call_frame frame = {
        0xAB00u, 0x0022u, 0x0033u, 0x0044u, 0x0055u, 0x0066u,
        0x3EB6u, 0x3E9Eu, 0x14A0u, 0x3DA1u, 0x1D16u, 0x2D14u, 0x7297u
    };
    uint16_t ip = 0x07E9u;
    memset(ram, 0xCC, sizeof ram);
    ram[at(frame.ss, (uint16_t)(frame.bp - 0x0Du))] = 0x12u;
    ram[at(frame.ss, (uint16_t)(frame.bp - 0x0Fu))] = 0x34u;
    ram[at(frame.ss, (uint16_t)(frame.bp + 6u))] = 0x04u;
    assert(pr_u3bd_first_call_07e9(ram, sizeof ram, &frame, &ip));
    assert(frame.ax == 0xAB04u && frame.sp == 0x3E94u);
    assert(frame.flags == 0x7297u && ip == 0x047Bu);
    assert(get_word(frame.ss, 0x3E9Cu) == 0xAB12u);
    assert(get_word(frame.ss, 0x3E9Au) == 0xAB34u);
    assert(get_word(frame.ss, 0x3E98u) == 0xAB04u);
    assert(get_word(frame.ss, 0x3E96u) == 0x2D14u);
    assert(get_word(frame.ss, 0x3E94u) == 0x07F9u);
    assert(frame.bx == 0x0022u && frame.cx == 0x0033u &&
           frame.dx == 0x0044u && frame.si == 0x0055u &&
           frame.di == 0x0066u && frame.bp == 0x3EB6u);
}

static void stack_offset_wraps_at_16_bits(void)
{
    pr_u3bd_first_call_frame frame = {
        0x1100u, 0u, 0u, 0u, 0u, 0u, 0x2000u, 0x0004u,
        0u, 0u, 0xFFF0u, 0x2D14u, 0x0202u
    };
    uint16_t ip = 0x07E9u;
    memset(ram, 0u, sizeof ram);
    ram[at(frame.ss, (uint16_t)(frame.bp - 0x0Du))] = 1u;
    ram[at(frame.ss, (uint16_t)(frame.bp - 0x0Fu))] = 2u;
    ram[at(frame.ss, (uint16_t)(frame.bp + 6u))] = 4u;
    assert(pr_u3bd_first_call_07e9(ram, sizeof ram, &frame, &ip));
    assert(frame.sp == 0xFFFAu && ip == 0x047Bu);
    assert(get_word(frame.ss, 0x0002u) == 0x1101u);
    assert(get_word(frame.ss, 0x0000u) == 0x1102u);
    assert(get_word(frame.ss, 0xFFFEu) == 0x1104u);
    assert(get_word(frame.ss, 0xFFFCu) == 0x2D14u);
    assert(get_word(frame.ss, 0xFFFAu) == 0x07F9u);
}

static void invalid_host_inputs_have_no_effect(void)
{
    pr_u3bd_first_call_frame frame = {
        8u, 0u, 4u, 13u, 0x3E68u, 4u, 0x3EB6u, 0x3E9Eu,
        0x14A0u, 0x3DA1u, 0x1D16u, 0x2D14u, 0x7297u
    };
    pr_u3bd_first_call_frame saved = frame;
    uint16_t ip = 0x07E9u;
    memset(ram, 0xA5, sizeof ram);
    assert(!pr_u3bd_first_call_07e9(NULL, sizeof ram, &frame, &ip));
    assert(!pr_u3bd_first_call_07e9(ram, sizeof ram - 1u, &frame, &ip));
    assert(!pr_u3bd_first_call_07e9(ram, sizeof ram, NULL, &ip));
    assert(!pr_u3bd_first_call_07e9(ram, sizeof ram, &frame, NULL));
    frame.cs = 0x2D15u;
    assert(!pr_u3bd_first_call_07e9(ram, sizeof ram, &frame, &ip));
    frame = saved;
    ip = 0x07EAu;
    assert(!pr_u3bd_first_call_07e9(ram, sizeof ram, &frame, &ip));
    assert(memcmp(&frame, &saved, sizeof frame) == 0);
    assert(ip == 0x07EAu);
    assert(ram[at(frame.ss, (uint16_t)(frame.sp - 2u))] == 0xA5u);
}

int main(void)
{
    call_handoff_preserves_high_byte_and_flags();
    stack_offset_wraps_at_16_bits();
    invalid_host_inputs_have_no_effect();
    return 0;
}
