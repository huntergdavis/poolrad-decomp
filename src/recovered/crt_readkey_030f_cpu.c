/* START.EXE 0A40:030F, Turbo Pascal Crt.ReadKey with its 0143 break check, raw CPU (docs/re/CRT_READKEY_030F_CPU.md). */
#include "poolrad/recovered/crt_keyboard_cpu.h"

#define MASK (PR_CRT_KEYBOARD_RAM_SIZE - 1u)

static size_t physical(uint16_t seg, uint16_t off) { return (((size_t)seg << 4u) + off) & MASK; }
static uint16_t rd16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t at = physical(seg, off);
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8u));
}
static void wr16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t v)
{
    size_t at = physical(seg, off);
    ram[at] = (uint8_t)v; ram[(at + 1u) & MASK] = (uint8_t)(v >> 8u);
}
static unsigned parity_even(unsigned byte)
{
    byte ^= byte >> 4u; byte ^= byte >> 2u; byte ^= byte >> 1u;
    return (~byte) & 1u;
}
/* Flags of OR r8,r8 / XOR r8,r8 / CMP r/m8,0 on result v: CF OF AF clear, ZF SF PF from v. */
static void byte_flags(pr_crt_cpu *c, unsigned v)
{
    c->flags = (uint16_t)((c->flags & (uint16_t)~0x08D5u) | (v == 0u ? 0x0040u : 0u) | (v & 0x80u) |
                          (parity_even(v) ? 0x0004u : 0u));
}

pr_crt_status pr_crt_readkey_030f(uint8_t *ram, size_t ram_size, pr_crt_cpu *c,
                                  pr_crt_int16_service service, void *context)
{
    pr_crt_cpu at_int, after;
    if (ram == NULL || c == NULL || service == NULL || ram_size != PR_CRT_KEYBOARD_RAM_SIZE || c->ip != 0x030Fu ||
        c->sp < 0x0002u || c->sp > 0xFFFCu)
        return PR_CRT_ARGUMENT;
    c->ax = (uint16_t)((c->ax & 0xFF00u) | ram[physical(c->ds, 0x8501u)]);  /* 030F MOV AL,[8501] */
    ram[physical(c->ds, 0x8501u)] = 0u;                                       /* 0312 MOV BYTE [8501],0 */
    byte_flags(c, c->ax & 0xFFu);                                             /* 0317 OR AL,AL */
    if ((c->flags & 0x0040u) != 0u) {                                         /* 0319 JNE 032D */
        c->ax &= 0x00FFu; byte_flags(c, 0u);                                  /* 031B XOR AH,AH */
        c->ip = 0x031Du;                                                      /* 031D INT 16h (declared) */
        if ((c->ax >> 8u) > 0x01u)
            return PR_CRT_ENV;
        at_int = *c;
        if (service(context, &at_int, &after) != 0 || after.cs != c->cs || after.ip != 0x031Fu || after.ss != c->ss ||
            after.sp != c->sp || after.bp != c->bp)
            return PR_CRT_SERVICE;
        *c = after;
        byte_flags(c, c->ax & 0xFFu);                                         /* 031F OR AL,AL */
        if ((c->flags & 0x0040u) != 0u) {                                     /* 0321 JNE 032D */
            ram[physical(c->ds, 0x8501u)] = (uint8_t)(c->ax >> 8u);           /* 0323 MOV [8501],AH */
            byte_flags(c, c->ax >> 8u);                                       /* 0327 OR AH,AH */
            if ((c->flags & 0x0040u) != 0u)                                   /* 0329 JNE 032D */
                c->ax = (uint16_t)((c->ax & 0xFF00u) | 0x03u);                /* 032B MOV AL,3 */
        }
    }
    c->sp = (uint16_t)(c->sp - 2u); wr16(ram, c->ss, c->sp, 0x0330u);       /* 032D CALL 0143 */
    byte_flags(c, ram[physical(c->ds, 0x8502u)]);                             /* 0143 CMP BYTE [8502],0 */
    if ((c->flags & 0x0040u) == 0u) {                                         /* 0148 JNE 014B */
        c->ip = 0x014Bu;
        return PR_CRT_BREAK;
    }
    c->ip = rd16(ram, c->ss, c->sp); c->sp = (uint16_t)(c->sp + 2u);        /* 014A RET (to 0330: nothing writes RAM since the CALL) */
    c->ip = rd16(ram, c->ss, c->sp); c->cs = rd16(ram, c->ss, (uint16_t)(c->sp + 2u));  /* 0330 RETF */
    c->sp = (uint16_t)(c->sp + 4u);
    return PR_CRT_RETURNED;
}
