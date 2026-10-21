/* START.EXE 0A32:00A2, Turbo Pascal Dos.GetTime, raw CPU (docs/re/DOS_GETTIME_00A2_CPU.md). */
#include "poolrad/recovered/dos_gettime_00a2_cpu.h"

#define MASK (PR_DOS_GETTIME_RAM_SIZE - 1u)

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

/* LES DI,[BP+disp]; STOSW with AX; DI then steps by 2 in the direction DF selects. */
static void store(uint8_t *ram, pr_dos_gettime_cpu *c, uint16_t disp)
{
    uint16_t off = (uint16_t)(c->bp + disp);
    c->di = rd16(ram, c->ss, off);
    c->es = rd16(ram, c->ss, (uint16_t)(off + 2u));
    wr16(ram, c->es, c->di, c->ax);
    c->di = (uint16_t)((c->flags & 0x0400u) ? c->di - 2u : c->di + 2u);
}

pr_dos_gettime_status pr_dos_gettime_00a2(uint8_t *ram, size_t ram_size, pr_dos_gettime_cpu *c,
                                          pr_dos_gettime_service service, void *context)
{
    pr_dos_gettime_cpu at_int, after;
    if (ram == NULL || c == NULL || service == NULL || ram_size != PR_DOS_GETTIME_RAM_SIZE || c->ip != 0x00A2u ||
        c->sp < 0x0002u || c->sp > 0xFFECu)
        return PR_DOS_GETTIME_ARGUMENT;
    c->sp = (uint16_t)(c->sp - 2u); wr16(ram, c->ss, c->sp, c->bp);     /* 00A2 PUSH BP */
    c->bp = c->sp;                                                    /* 00A3 MOV BP,SP */
    c->ax = (uint16_t)((c->ax & 0x00FFu) | 0x2C00u);                  /* 00A5 MOV AH,2Ch */
    c->ip = 0x00A7u;                                                  /* 00A7 INT 21h (declared) */
    at_int = *c;
    if (service(context, &at_int, &after) != 0 || after.cs != c->cs || after.ip != 0x00A9u || after.ss != c->ss ||
        after.sp != c->sp || after.bp != c->bp)
        return PR_DOS_GETTIME_SERVICE;
    *c = after;
    /* 00A9 XOR AH,AH: AH := 0; CF, OF, AF, SF clear; ZF and PF set (result byte 0). */
    c->ax &= 0x00FFu;
    c->flags = (uint16_t)((c->flags & (uint16_t)~0x08D5u) | 0x0044u);
    c->ax = (uint16_t)(c->dx & 0x00FFu);          store(ram, c, 0x06u);   /* 00AB MOV AL,DL; 00AD LES; 00B0 STOSW */
    c->ax = (uint16_t)(c->dx >> 8u);              store(ram, c, 0x0Au);   /* 00B1 MOV AL,DH; 00B3 LES; 00B6 STOSW */
    c->ax = (uint16_t)(c->cx & 0x00FFu);          store(ram, c, 0x0Eu);   /* 00B7 MOV AL,CL; 00B9 LES; 00BC STOSW */
    c->ax = (uint16_t)(c->cx >> 8u);              store(ram, c, 0x12u);   /* 00BD MOV AL,CH; 00BF LES; 00C2 STOSW */
    c->bp = rd16(ram, c->ss, c->sp); c->sp = (uint16_t)(c->sp + 2u);  /* 00C3 POP BP */
    c->ip = rd16(ram, c->ss, c->sp); c->cs = rd16(ram, c->ss, (uint16_t)(c->sp + 2u));  /* 00C4 RETF 10h */
    c->sp = (uint16_t)(c->sp + 4u + 0x10u);
    return PR_DOS_GETTIME_RETURNED;
}
