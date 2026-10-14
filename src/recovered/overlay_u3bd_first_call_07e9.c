#include "poolrad/recovered/overlay_u3bd_first_call_07e9.h"

static uint32_t physical(uint16_t segment, uint16_t offset)
{
    return (((uint32_t)segment << 4) + offset) & 0xFFFFFu;
}

static uint8_t read_byte(const uint8_t *ram, uint16_t segment,
                         uint16_t offset)
{
    return ram[physical(segment, offset)];
}

static void write_word(uint8_t *ram, uint16_t segment,
                       uint16_t offset, uint16_t value)
{
    ram[physical(segment, offset)] = (uint8_t)value;
    ram[physical(segment, (uint16_t)(offset + 1u))] =
        (uint8_t)(value >> 8);
}

static void push_word(uint8_t *ram, pr_u3bd_first_call_frame *frame,
                      uint16_t value)
{
    frame->sp = (uint16_t)(frame->sp - 2u);
    write_word(ram, frame->ss, frame->sp, value);
}

int pr_u3bd_first_call_07e9(uint8_t *ram, size_t ram_size,
                            pr_u3bd_first_call_frame *frame, uint16_t *ip)
{
    if (ram == NULL || frame == NULL || ip == NULL ||
        ram_size < PR_U3BD_LOOP_RESET_RAM_BYTES ||
        frame->cs != 0x2D14u || *ip != 0x07E9u)
        return 0;

    /* The three MOV AL instructions leave AH and FLAGS unchanged. Each
     * following PUSH AX writes a full word, including the preserved AH. */
    frame->ax = (uint16_t)((frame->ax & 0xFF00u) |
                           read_byte(ram, frame->ss,
                                     (uint16_t)(frame->bp - 0x0Du)));
    push_word(ram, frame, frame->ax);
    frame->ax = (uint16_t)((frame->ax & 0xFF00u) |
                           read_byte(ram, frame->ss,
                                     (uint16_t)(frame->bp - 0x0Fu)));
    push_word(ram, frame, frame->ax);
    frame->ax = (uint16_t)((frame->ax & 0xFF00u) |
                           read_byte(ram, frame->ss,
                                     (uint16_t)(frame->bp + 6u)));
    push_word(ram, frame, frame->ax);
    push_word(ram, frame, frame->cs); /* +07F5 PUSH CS */
    push_word(ram, frame, 0x07F9u);   /* +07F6 near CALL return IP */
    *ip = 0x047Bu;
    return 1;
}
