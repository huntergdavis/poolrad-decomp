#include "poolrad/recovered/overlay_u3bd_leaf_call_07f9.h"

static uint32_t physical(uint16_t segment, uint16_t offset)
{
    return (((uint32_t)segment << 4) + offset) & 0xFFFFFu;
}

static uint8_t read_byte(const uint8_t *ram, uint16_t segment,
                         uint16_t offset)
{
    return ram[physical(segment, offset)];
}

static uint16_t read_word(const uint8_t *ram, uint16_t segment,
                          uint16_t offset)
{
    return (uint16_t)(read_byte(ram, segment, offset) |
                      ((uint16_t)read_byte(ram, segment,
                                           (uint16_t)(offset + 1u)) << 8));
}

static void write_byte(uint8_t *ram, uint16_t segment,
                       uint16_t offset, uint8_t value)
{
    ram[physical(segment, offset)] = value;
}

static void push_word(uint8_t *ram, pr_u3bd_leaf_call_frame *frame,
                      uint16_t value)
{
    frame->sp = (uint16_t)(frame->sp - 2u);
    write_byte(ram, frame->ss, frame->sp, (uint8_t)value);
    write_byte(ram, frame->ss, (uint16_t)(frame->sp + 1u),
               (uint8_t)(value >> 8));
}

int pr_u3bd_leaf_call_07f9(uint8_t *ram, size_t ram_size,
                            pr_u3bd_leaf_call_frame *frame, uint16_t *ip)
{
    uint16_t leaf_bp;
    uint8_t x, y, result;

    if (ram == NULL || frame == NULL || ip == NULL ||
        ram_size < PR_U3BD_LOOP_RESET_RAM_BYTES ||
        frame->cs != 0x2D14u || *ip != 0x07F9u)
        return 0;

    /* +07F9..+0805: save entry-2 AL, push x, y, CS and return IP. MOV AL
     * preserves AH; the leaf reads only the low bytes of pushed words. */
    write_byte(ram, frame->ss, (uint16_t)(frame->bp - 0x14u),
               (uint8_t)frame->ax);
    frame->ax = (uint16_t)((frame->ax & 0xFF00u) |
                           read_byte(ram, frame->ss,
                                     (uint16_t)(frame->bp - 0x0Du)));
    push_word(ram, frame, frame->ax);
    frame->ax = (uint16_t)((frame->ax & 0xFF00u) |
                           read_byte(ram, frame->ss,
                                     (uint16_t)(frame->bp - 0x0Fu)));
    push_word(ram, frame, frame->ax);
    push_word(ram, frame, frame->cs);
    push_word(ram, frame, 0x0808u);

    /* +031F..+034D: PUSH BP, SUB SP,2, signed byte bounds checks,
     * [BP-1]=0/1, then MOV AL and RETF 4. The merged entry-2/entry-4
     * models prove this same leaf's result rule. Retain its stack writes. */
    push_word(ram, frame, frame->bp);
    leaf_bp = frame->sp;
    frame->bp = leaf_bp;
    frame->sp = (uint16_t)(frame->sp - 2u);
    x = read_byte(ram, frame->ss, (uint16_t)(leaf_bp + 8u));
    y = read_byte(ram, frame->ss, (uint16_t)(leaf_bp + 6u));
    result = (uint8_t)(x <= 15u && y <= 15u);
    write_byte(ram, frame->ss, (uint16_t)(leaf_bp - 1u), result);
    frame->ax = (uint16_t)((frame->ax & 0xFF00u) |
                           read_byte(ram, frame->ss,
                                     (uint16_t)(leaf_bp - 1u)));
    frame->sp = leaf_bp;
    frame->bp = read_word(ram, frame->ss, frame->sp);
    frame->sp = (uint16_t)(frame->sp + 2u);
    *ip = read_word(ram, frame->ss, frame->sp);
    frame->sp = (uint16_t)(frame->sp + 2u);
    frame->cs = read_word(ram, frame->ss, frame->sp);
    frame->sp = (uint16_t)(frame->sp + 6u); /* saved CS, then RETF 4 args */

    /* +0808 OR AL,AL: CF/OF clear; ZF/PF set iff the leaf returned zero.
     * AF is architecturally undefined for OR and is left host supplied. */
    frame->flags = (uint16_t)((frame->flags & (uint16_t)~0x08D5u) |   /* OR clears AF in pinned DOSBox-X (flags.cpp:629-643) */
                              (result == 0u ? 0x0044u : 0u));
    *ip = 0x080Au;
    return 1;
}
