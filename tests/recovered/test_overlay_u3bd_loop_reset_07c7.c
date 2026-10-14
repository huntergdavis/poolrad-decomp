#include "poolrad/recovered/overlay_u3bd_loop_reset_07c7.h"

#include <assert.h>
#include <string.h>

static uint8_t ram[PR_U3BD_LOOP_RESET_RAM_BYTES];

static size_t at(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & 0xFFFFFu;
}

static void put_word(uint16_t seg, uint16_t off, uint16_t value)
{
    ram[at(seg, off)] = (uint8_t)value;
    ram[at(seg, (uint16_t)(off + 1u))] = (uint8_t)(value >> 8);
}

static uint16_t get_word(uint16_t seg, uint16_t off)
{
    return (uint16_t)(ram[at(seg, off)] |
                      ((uint16_t)ram[at(seg, (uint16_t)(off + 1u))] << 8));
}

static void reset_restores_saved_coordinates(void)
{
    pr_u3bd_loop_reset_frame frame = {
        8u, 0u, 4u, 13u, 0x3E68u, 4u, 0x3EB6u, 0x3E9Eu,
        0x14A0u, 0x3DA1u, 0x1D16u, 0x2D14u, 0x7246u
    };
    uint16_t ip = 0x07C7u;
    memset(ram, 0xA5, sizeof ram);
    put_word(frame.ss, (uint16_t)(frame.bp - 5u), 0x1234u);
    put_word(frame.ss, (uint16_t)(frame.bp - 7u), 0xABCDu);
    put_word(frame.ss, (uint16_t)(frame.bp - 0x0Du), 0xFFFFu);
    put_word(frame.ss, (uint16_t)(frame.bp - 0x0Fu), 0xFFFFu);
    put_word(frame.ss, (uint16_t)(frame.bp - 0x12u), 0xFFFFu);
    ram[at(frame.ss, (uint16_t)(frame.bp - 0x10u))] = 4u;
    ram[at(frame.ss, (uint16_t)(frame.bp - 0x17u))] = 0xFFu;

    assert(pr_u3bd_loop_reset_07c7(ram, sizeof ram, &frame, &ip));
    assert(get_word(frame.ss, (uint16_t)(frame.bp - 0x0Du)) == 0x1234u);
    assert(get_word(frame.ss, (uint16_t)(frame.bp - 0x0Fu)) == 0xABCDu);
    assert(ram[at(frame.ss, (uint16_t)(frame.bp - 0x10u))] == 0u);
    assert(get_word(frame.ss, (uint16_t)(frame.bp - 0x12u)) == 0u);
    assert(ram[at(frame.ss, (uint16_t)(frame.bp - 0x17u))] == 0u);
    assert(get_word(frame.ss, (uint16_t)(frame.bp - 5u)) == 0x1234u);
    assert(get_word(frame.ss, (uint16_t)(frame.bp - 7u)) == 0xABCDu);
    assert(frame.ax == 0u && frame.flags == 0x7297u && ip == 0x07E9u);
    assert(frame.bx == 0u && frame.cx == 4u && frame.dx == 13u);
    assert(frame.si == 0x3E68u && frame.di == 4u && frame.sp == 0x3E9Eu);
}

static void wrapped_stack_offsets(void)
{
    pr_u3bd_loop_reset_frame frame = {
        1u, 2u, 3u, 4u, 5u, 6u, 5u, 8u,
        0u, 0u, 0xFFF0u, 0x2D14u, 0x0002u
    };
    uint16_t ip = 0x07C7u;
    memset(ram, 0x66, sizeof ram);
    put_word(frame.ss, 0u, 0x0201u);
    put_word(frame.ss, 0xFFFEu, 0x0403u);
    assert(pr_u3bd_loop_reset_07c7(ram, sizeof ram, &frame, &ip));
    assert(get_word(frame.ss, (uint16_t)(frame.bp - 0x0Du)) == 0x0201u);
    assert(get_word(frame.ss, (uint16_t)(frame.bp - 0x0Fu)) == 0x0403u);
    assert(ip == 0x07E9u);
}

static void invalid_input_has_no_effect(void)
{
    pr_u3bd_loop_reset_frame frame = {
        0u, 0u, 0u, 0u, 0u, 0u, 0x3EB6u, 0u,
        0u, 0u, 0x1D16u, 0x2D14u, 0x7246u
    };
    pr_u3bd_loop_reset_frame saved = frame;
    uint16_t ip = 0x07C7u;
    memset(ram, 0xCC, sizeof ram);
    assert(!pr_u3bd_loop_reset_07c7(NULL, sizeof ram, &frame, &ip));
    assert(!pr_u3bd_loop_reset_07c7(ram, sizeof ram - 1u, &frame, &ip));
    assert(!pr_u3bd_loop_reset_07c7(ram, sizeof ram, NULL, &ip));
    assert(!pr_u3bd_loop_reset_07c7(ram, sizeof ram, &frame, NULL));
    frame.cs = 0x2D15u;
    assert(!pr_u3bd_loop_reset_07c7(ram, sizeof ram, &frame, &ip));
    frame = saved;
    ip = 0x07C8u;
    assert(!pr_u3bd_loop_reset_07c7(ram, sizeof ram, &frame, &ip));
    assert(memcmp(&frame, &saved, sizeof frame) == 0);
    assert(ip == 0x07C8u);
    assert(ram[at(frame.ss, (uint16_t)(frame.bp - 0x0Du))] == 0xCCu);
}

int main(void)
{
    reset_restores_saved_coordinates();
    wrapped_stack_offsets();
    invalid_input_has_no_effect();
    return 0;
}
