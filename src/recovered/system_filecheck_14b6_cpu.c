/* START.EXE 0AF8:14B6, Turbo Pascal SYSTEM file-magic check, raw CPU (docs/re/SYSTEM_FILECHECK_14B6_CPU.md). */
#include "poolrad/recovered/system_filecheck_14b6_cpu.h"
#define MASK (PR_SYSTEM_FILECHECK_RAM_SIZE - 1u)
static size_t physical(uint16_t seg, uint16_t off) { return (((size_t)seg << 4) + off) & MASK; }
static uint16_t word(const uint8_t *r, size_t at) { return (uint16_t)(r[at] | ((uint16_t)r[(at + 1u) & MASK] << 8)); }
static unsigned even(uint8_t v) { v ^= (uint8_t)(v >> 4); v ^= (uint8_t)(v >> 2); v ^= (uint8_t)(v >> 1); return (v & 1u) == 0; }
pr_system_filecheck_status pr_system_filecheck_14b6(uint8_t *ram, size_t size, pr_system_filecheck_cpu *c)
{
    uint16_t a, b = 0xD7B3u, r;
    size_t slot;
    if (!ram || !c || size != PR_SYSTEM_FILECHECK_RAM_SIZE || c->ip != 0x14B6u || c->sp > 0xFFFCu) return PR_SYSTEM_FILECHECK_ARGUMENT;
    a = word(ram, physical(c->es, (uint16_t)(c->di + 2u)));                         /* 14B6 CMP WORD ES:[DI+2],D7B3 */
    r = (uint16_t)(a - b);
    c->flags &= (uint16_t)~0x08D5u;
    if (a < b) c->flags |= 0x01;
    if ((a ^ b ^ r) & 0x10) c->flags |= 0x10;
    if ((a ^ b) & (a ^ r) & 0x8000u) c->flags |= 0x800;
    if (even((uint8_t)r)) c->flags |= 0x04;
    if (r == 0) c->flags |= 0x40;
    if (r & 0x8000u) c->flags |= 0x80;
    if (r != 0) {                                                                    /* 14BC JE 14C4 not taken */
        slot = physical(c->ds, 0x4426u);                                             /* 14BE MOV WORD [4426],0067h */
        ram[slot] = 0x67u; ram[(slot + 1u) & MASK] = 0x00u;
    }
    c->ip = word(ram, physical(c->ss, c->sp));                                       /* 14C4 RET: the slot is reread, after the store */
    c->sp = (uint16_t)(c->sp + 2u);
    return PR_SYSTEM_FILECHECK_RETURNED;
}
