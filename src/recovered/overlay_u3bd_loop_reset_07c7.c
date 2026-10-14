#include "poolrad/recovered/overlay_u3bd_loop_reset_07c7.h"

enum { PR_ARITH_FLAGS = 0x08D5u, PR_ZERO_MINUS_FOUR_FLAGS = 0x0095u };

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

static void write_word(uint8_t *ram, uint16_t segment,
                       uint16_t offset, uint16_t value)
{
    write_byte(ram, segment, offset, (uint8_t)value);
    write_byte(ram, segment, (uint16_t)(offset + 1u),
               (uint8_t)(value >> 8));
}

int pr_u3bd_loop_reset_07c7(uint8_t *ram, size_t ram_size,
                             pr_u3bd_loop_reset_frame *frame,
                             uint16_t *ip)
{
    uint16_t bp;

    if (ram == NULL || frame == NULL || ip == NULL ||
        ram_size < PR_U3BD_LOOP_RESET_RAM_BYTES ||
        frame->cs != 0x2D14u || *ip != 0x07C7u)
        return 0;

    bp = frame->bp;
    /* +07C7..+07D3: restore the two coordinate locals from saved words,
     * then clear the loop counter. MOV instructions preserve flags. */
    frame->ax = read_word(ram, frame->ss, (uint16_t)(bp - 5u));
    write_word(ram, frame->ss, (uint16_t)(bp - 0x0Du), frame->ax);
    frame->ax = read_word(ram, frame->ss, (uint16_t)(bp - 7u));
    write_word(ram, frame->ss, (uint16_t)(bp - 0x0Fu), frame->ax);
    write_byte(ram, frame->ss, (uint16_t)(bp - 0x10u), 0u);

    /* +07D7..+07DC: XOR AX,AX and clear the offset and old-wall locals. */
    frame->ax = 0u;
    write_word(ram, frame->ss, (uint16_t)(bp - 0x12u), 0u);
    write_byte(ram, frame->ss, (uint16_t)(bp - 0x17u), 0u);

    /* +07E0 CMP counter,4 sees the just-cleared counter. JB at +07E4
     * therefore reaches +07E9. CMP replaces the XOR arithmetic flags. */
    frame->flags = (uint16_t)((frame->flags &
                               (uint16_t)~PR_ARITH_FLAGS) |
                              PR_ZERO_MINUS_FOUR_FLAGS);
    *ip = 0x07E9u;
    return 1;
}
