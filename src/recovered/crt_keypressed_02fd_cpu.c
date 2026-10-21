/* START.EXE 0A40:02FD, Turbo Pascal Crt.KeyPressed, raw CPU (docs/re/CRT_KEYPRESSED_02FD_CPU.md). */
#include "poolrad/recovered/crt_keyboard_cpu.h"

#define MASK (PR_CRT_KEYBOARD_RAM_SIZE - 1u)

static size_t physical(uint16_t seg, uint16_t off) { return (((size_t)seg << 4u) + off) & MASK; }
static uint16_t rd16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t at = physical(seg, off);
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8u));
}
static unsigned parity_even(unsigned byte)
{
    byte ^= byte >> 4u; byte ^= byte >> 2u; byte ^= byte >> 1u;
    return (~byte) & 1u;
}

pr_crt_status pr_crt_keypressed_02fd(uint8_t *ram, size_t ram_size, pr_crt_cpu *c,
                                     pr_crt_int16_service service, void *context)
{
    pr_crt_cpu at_int, after;
    uint8_t buffered;
    if (ram == NULL || c == NULL || service == NULL || ram_size != PR_CRT_KEYBOARD_RAM_SIZE || c->ip != 0x02FDu ||
        c->sp > 0xFFFCu)
        return PR_CRT_ARGUMENT;
    /* 02FD CMP BYTE [8501],0: CF OF AF clear, ZF/SF/PF from the byte. */
    buffered = ram[physical(c->ds, 0x8501u)];
    c->flags = (uint16_t)((c->flags & (uint16_t)~0x08D5u) | (buffered == 0u ? 0x0040u : 0u) |
                          (buffered & 0x80u) | (parity_even(buffered) ? 0x0004u : 0u));
    if (buffered != 0u) {                                             /* 0302 JNE 030C */
        c->ax = (uint16_t)((c->ax & 0xFF00u) | 0x01u);                /* 030C MOV AL,1 */
    } else {
        c->ax = (uint16_t)((c->ax & 0x00FFu) | 0x0100u);              /* 0304 MOV AH,1 */
        c->ip = 0x0306u;                                              /* 0306 INT 16h (declared) */
        if ((c->ax >> 8u) > 0x01u)
            return PR_CRT_ENV;
        at_int = *c;
        if (service(context, &at_int, &after) != 0 || after.cs != c->cs || after.ip != 0x0308u || after.ss != c->ss ||
            after.sp != c->sp || after.bp != c->bp)
            return PR_CRT_SERVICE;
        *c = after;
        c->ax &= 0xFF00u;                                             /* 0308 MOV AL,0 */
        if ((c->flags & 0x0040u) == 0u)                               /* 030A JE 030E */
            c->ax |= 0x01u;                                           /* 030C MOV AL,1 */
    }
    c->ip = rd16(ram, c->ss, c->sp); c->cs = rd16(ram, c->ss, (uint16_t)(c->sp + 2u));  /* 030E RETF */
    c->sp = (uint16_t)(c->sp + 4u);
    return PR_CRT_RETURNED;
}
