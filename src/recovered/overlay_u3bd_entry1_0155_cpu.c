/* Original unit 0x3BD +0155, public entry 1: four argument bytes to DS:[6A60..6A63] (docs/re/OVL_03BD_0155_CPU.md). */
#include "poolrad/recovered/overlay_u3bd_entry1_0155_cpu.h"

#define MASK (PR_U3BD_0155_RAM_SIZE - 1u)
typedef pr_u3bd_0155_cpu cpu_t;

static size_t physical(uint16_t seg, uint16_t off) { return (((size_t)seg << 4u) + off) & MASK; }
static uint8_t rd8(const uint8_t *ram, uint16_t seg, uint16_t off) { return ram[physical(seg, off)]; }
static void wr8(uint8_t *ram, uint16_t seg, uint16_t off, uint8_t v) { ram[physical(seg, off)] = v; }
static uint16_t rd16(const uint8_t *ram, uint16_t seg, uint16_t off)
{ size_t p = physical(seg, off); return (uint16_t)(ram[p] | ((uint16_t)ram[(p + 1u) & MASK] << 8u)); }
static void wr16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t v)
{ size_t p = physical(seg, off); ram[p] = (uint8_t)v; ram[(p + 1u) & MASK] = (uint8_t)(v >> 8u); }
static void push(uint8_t *ram, cpu_t *c, uint16_t v) { c->sp = (uint16_t)(c->sp - 2u); wr16(ram, c->ss, c->sp, v); }
static uint16_t pop(const uint8_t *ram, cpu_t *c) { uint16_t v = rd16(ram, c->ss, c->sp); c->sp = (uint16_t)(c->sp + 2u); return v; }
static void mov_al(cpu_t *c, uint8_t v) { c->ax = (uint16_t)((c->ax & 0xFF00u) | v); }

pr_u3bd_0155_status pr_u3bd_entry1_0155_cpu(uint8_t *ram, size_t ram_size, pr_u3bd_0155_cpu *c, size_t *budget)
{
    uint16_t ret_ip, ret_cs;
    if (ram == NULL || c == NULL || budget == NULL || ram_size != PR_U3BD_0155_RAM_SIZE || c->ip != 0x0155u || c->sp < 0x0002u || c->sp > 0xFFF4u)
        return PR_U3BD_0155_ARGUMENT;
    ret_ip = rd16(ram, c->ss, c->sp); ret_cs = rd16(ram, c->ss, (uint16_t)(c->sp + 2u));
#define STEP() do { if (*budget == 0u) return PR_U3BD_0155_LIMIT; --*budget; } while (0)
#define AT(next_ip) (c->ip = (next_ip))
    STEP(); push(ram, c, c->bp); AT(0x0156u);                                                 /* 0155 PUSH BP */
    STEP(); c->bp = c->sp; AT(0x0158u);                                                       /* 0156 MOV BP,SP */
    STEP(); mov_al(c, rd8(ram, c->ss, (uint16_t)(c->bp + 0x0Cu))); AT(0x015Bu);               /* 0158 MOV AL,[BP+0C] */
    STEP(); wr8(ram, c->ds, 0x6A60u, (uint8_t)c->ax); AT(0x015Eu);                            /* 015B MOV [6A60],AL */
    STEP(); mov_al(c, rd8(ram, c->ss, (uint16_t)(c->bp + 0x0Au))); AT(0x0161u);               /* 015E MOV AL,[BP+0A] */
    STEP(); wr8(ram, c->ds, 0x6A61u, (uint8_t)c->ax); AT(0x0164u);                            /* 0161 MOV [6A61],AL */
    STEP(); mov_al(c, rd8(ram, c->ss, (uint16_t)(c->bp + 0x08u))); AT(0x0167u);               /* 0164 MOV AL,[BP+8] */
    STEP(); wr8(ram, c->ds, 0x6A62u, (uint8_t)c->ax); AT(0x016Au);                            /* 0167 MOV [6A62],AL */
    STEP(); mov_al(c, rd8(ram, c->ss, (uint16_t)(c->bp + 0x06u))); AT(0x016Du);               /* 016A MOV AL,[BP+6] */
    STEP(); wr8(ram, c->ds, 0x6A63u, (uint8_t)c->ax); AT(0x0170u);                            /* 016D MOV [6A63],AL */
    STEP(); c->bp = pop(ram, c); AT(0x0171u);                                                 /* 0170 POP BP */
    STEP(); c->ip = pop(ram, c); c->cs = pop(ram, c); c->sp = (uint16_t)(c->sp + 8u);         /* 0171 RETF 8 */
#undef AT
#undef STEP
    if (c->ip != ret_ip || c->cs != ret_cs) return PR_U3BD_0155_TRANSFER;
    return PR_U3BD_0155_RETURNED;
}
