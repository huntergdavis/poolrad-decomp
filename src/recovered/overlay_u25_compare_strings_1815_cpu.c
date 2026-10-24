/* GAME.OVR unit 0x25 +1815 (entry 15): compare two strings into six flag bytes, full CPU state (docs/re/OVL_0025_1815_CPU.md). */
#include "poolrad/recovered/overlay_u25_compare_strings_1815_cpu.h"
#include "poolrad/recovered/system_runtime_leaves_cpu.h"
#include "poolrad/recovered/system_string_move_leaves_cpu.h"
#include "poolrad/recovered/system_strstore_fullstate_0b25.h"
#include <string.h>

#define MASK (PR_U25_1815_RAM_SIZE - 1u)
#define ARITH 0x08D5u
typedef pr_u25_1815_cpu cpu_t;
#define COPY(dst, src) do { (void)sizeof(char[sizeof *(dst) == sizeof *(src) ? 1 : -1]); memcpy((dst), (src), sizeof *(dst)); } while (0)   /* the same 14-word layout */

static size_t physical(uint16_t seg, uint16_t off) { return (((size_t)seg << 4u) + off) & MASK; }
static uint8_t rd8(const uint8_t *ram, uint16_t seg, uint16_t off) { return ram[physical(seg, off)]; }
static void wr8(uint8_t *ram, uint16_t seg, uint16_t off, uint8_t v) { ram[physical(seg, off)] = v; }
static uint16_t rd16(const uint8_t *ram, uint16_t seg, uint16_t off)
{ size_t p = physical(seg, off); return (uint16_t)(ram[p] | ((uint16_t)ram[(p + 1u) & MASK] << 8u)); }
static void wr16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t v)
{ size_t p = physical(seg, off); ram[p] = (uint8_t)v; ram[(p + 1u) & MASK] = (uint8_t)(v >> 8u); }
/* LES as pinned DOSBox-X: the segment word at (offset+2) mod 64K */
static void les(const uint8_t *ram, cpu_t *c, uint16_t seg, uint16_t off)
{ uint16_t o = rd16(ram, seg, off); c->es = rd16(ram, seg, (uint16_t)(off + 2u)); c->di = o; }
static void push(uint8_t *ram, cpu_t *c, uint16_t v) { c->sp = (uint16_t)(c->sp - 2u); wr16(ram, c->ss, c->sp, v); }
static uint16_t pop(const uint8_t *ram, cpu_t *c) { uint16_t v = rd16(ram, c->ss, c->sp); c->sp = (uint16_t)(c->sp + 2u); return v; }
static uint16_t parity(uint8_t v) { v ^= (uint8_t)(v >> 4u); v ^= (uint8_t)(v >> 2u); v ^= (uint8_t)(v >> 1u); return (v & 1u) ? 0u : 0x0004u; }
/* SUB SP,200h: the six arithmetic flags (PF of the low byte) */
static uint16_t sub16(cpu_t *c, uint16_t a, uint16_t b)
{
    uint16_t r = (uint16_t)(a - b), f = parity((uint8_t)r);
    if (a < b) f |= 0x0001u;
    if ((a ^ b ^ r) & 0x10u) f |= 0x0010u;
    if (!r) f |= 0x0040u;
    if (r & 0x8000u) f |= 0x0080u;
    if ((a ^ b) & (a ^ r) & 0x8000u) f |= 0x0800u;
    c->flags = (uint16_t)((c->flags & ~ARITH) | f); return r;
}

#define STEP(ip_) do { if (*budget == 0u) { c->ip = (ip_); return PR_U25_1815_LIMIT; } --*budget; } while (0)
/* LCALL 0AF8:off_ at site_ (9Ah, the offset word, the relocated SYSTEM segment word); the callee runs as its raw model and must return to next_ */
#define SYSCALL(site_, next_, off_, call_) do { \
    if (*budget == 0u) { c->ip = (site_); return PR_U25_1815_LIMIT; } \
    if (rd8(ram, cs, (site_)) != 0x9Au || rd16(ram, cs, (uint16_t)((site_) + 1u)) != (off_)) { c->ip = (site_); return PR_U25_1815_CODE; } \
    --*budget; push(ram, c, cs); push(ram, c, (next_)); c->cs = rd16(ram, cs, (uint16_t)((site_) + 3u)); c->ip = (off_); \
    call_; \
    if (c->cs != cs || c->ip != (next_)) return PR_U25_1815_TRANSFER; } while (0)
#define STRSTORE() { pr_system_0b25_cpu x; COPY(&x, c); (void)pr_system_strstore_0b25(ram, ram_size, &x); COPY(c, &x); }
#define FILLCHAR() { pr_system_leaf_cpu x; COPY(&x, c); (void)pr_system_fillchar_16a8(ram, ram_size, &x); COPY(c, &x); }
#define STRCMP() { pr_system_leaf_cpu x; COPY(&x, c); (void)pr_system_strcmp_0c10(ram, ram_size, &x); COPY(c, &x); }
/* one comparison block: LEA DI,[BP-100h]; PUSH SS; PUSH DI; LEA DI,[BP-200h]; PUSH SS; PUSH DI; LCALL 0C10; Jcc over; MOV BYTE [flag],1 */
#define COMPARE(at_, flag_, set_) do { \
    STEP((at_)); c->di = (uint16_t)(c->bp - 0x0100u); \
    STEP((uint16_t)((at_) + 4u)); push(ram, c, c->ss); \
    STEP((uint16_t)((at_) + 5u)); push(ram, c, c->di); \
    STEP((uint16_t)((at_) + 6u)); c->di = (uint16_t)(c->bp - 0x0200u); \
    STEP((uint16_t)((at_) + 10u)); push(ram, c, c->ss); \
    STEP((uint16_t)((at_) + 11u)); push(ram, c, c->di); \
    SYSCALL((uint16_t)((at_) + 12u), (uint16_t)((at_) + 17u), 0x0C10u, STRCMP()); \
    STEP((uint16_t)((at_) + 17u)); \
    if (set_) { STEP((uint16_t)((at_) + 19u)); wr8(ram, c->ds, (flag_), 1u); } } while (0)
#define ZF (c->flags & 0x0040u)
#define CF (c->flags & 0x0001u)

pr_u25_1815_status pr_u25_compare_strings_1815_cpu(uint8_t *ram, size_t ram_size, pr_u25_1815_cpu *c, size_t *budget)
{
    uint16_t cs, ret_ip, ret_cs;
    if (ram == NULL || c == NULL || budget == NULL || ram_size != PR_U25_1815_RAM_SIZE || c->ip != 0x1815u || c->sp < 0x0220u || c->sp > 0xFFF4u)
        return PR_U25_1815_ARGUMENT;
    cs = c->cs; ret_ip = rd16(ram, c->ss, c->sp); ret_cs = rd16(ram, c->ss, (uint16_t)(c->sp + 2u));
    STEP(0x1815u); push(ram, c, c->bp);                                                                  /* 1815 PUSH BP */
    STEP(0x1816u); c->bp = c->sp;                                                                        /* 1816 MOV BP,SP */
    STEP(0x1818u); c->sp = sub16(c, c->sp, 0x0200u);                                                     /* 1818 SUB SP,200h */
    STEP(0x181Cu); les(ram, c, c->ss, (uint16_t)(c->bp + 0x0Au));                                        /* 181C LES DI,[BP+0Ah] */
    STEP(0x181Fu); push(ram, c, c->es);                                                                  /* 181F PUSH ES */
    STEP(0x1820u); push(ram, c, c->di);                                                                  /* 1820 PUSH DI */
    STEP(0x1821u); c->di = (uint16_t)(c->bp - 0x0100u);                                                  /* 1821 LEA DI,[BP-100h] */
    STEP(0x1825u); push(ram, c, c->ss);                                                                  /* 1825 PUSH SS */
    STEP(0x1826u); push(ram, c, c->di);                                                                  /* 1826 PUSH DI */
    STEP(0x1827u); c->ax = 0x00FFu;                                                                      /* 1827 MOV AX,FFh */
    STEP(0x182Au); push(ram, c, c->ax);                                                                  /* 182A PUSH AX */
    SYSCALL(0x182Bu, 0x1830u, 0x0B25u, STRSTORE());                                                      /* 182B LCALL 0AF8:0B25 */
    STEP(0x1830u); les(ram, c, c->ss, (uint16_t)(c->bp + 0x06u));                                        /* 1830 LES DI,[BP+6] */
    STEP(0x1833u); push(ram, c, c->es);                                                                  /* 1833 PUSH ES */
    STEP(0x1834u); push(ram, c, c->di);                                                                  /* 1834 PUSH DI */
    STEP(0x1835u); c->di = (uint16_t)(c->bp - 0x0200u);                                                  /* 1835 LEA DI,[BP-200h] */
    STEP(0x1839u); push(ram, c, c->ss);                                                                  /* 1839 PUSH SS */
    STEP(0x183Au); push(ram, c, c->di);                                                                  /* 183A PUSH DI */
    STEP(0x183Bu); c->ax = 0x00FFu;                                                                      /* 183B MOV AX,FFh */
    STEP(0x183Eu); push(ram, c, c->ax);                                                                  /* 183E PUSH AX */
    SYSCALL(0x183Fu, 0x1844u, 0x0B25u, STRSTORE());                                                      /* 183F LCALL 0AF8:0B25 */
    STEP(0x1844u); c->di = 0x6F78u;                                                                      /* 1844 MOV DI,6F78h */
    STEP(0x1847u); push(ram, c, c->ds);                                                                  /* 1847 PUSH DS */
    STEP(0x1848u); push(ram, c, c->di);                                                                  /* 1848 PUSH DI */
    STEP(0x1849u); c->ax = 0x0006u;                                                                      /* 1849 MOV AX,6 */
    STEP(0x184Cu); push(ram, c, c->ax);                                                                  /* 184C PUSH AX */
    STEP(0x184Du); c->ax = (uint16_t)(c->ax & 0xFF00u);                                                  /* 184D MOV AL,0 */
    STEP(0x184Fu); push(ram, c, c->ax);                                                                  /* 184F PUSH AX */
    SYSCALL(0x1850u, 0x1855u, 0x16A8u, FILLCHAR());                                                      /* 1850 LCALL 0AF8:16A8 */
    COMPARE(0x1855u, 0x6F78u, ZF);                                                                       /* 1866 JNE: a = b */
    COMPARE(0x186Du, 0x6F79u, !ZF);                                                                      /* 187E JE: a <> b */
    COMPARE(0x1885u, 0x6F7Au, CF);                                                                       /* 1896 JAE: a < b */
    COMPARE(0x189Du, 0x6F7Bu, !CF && !ZF);                                                               /* 18AE JBE: a > b */
    COMPARE(0x18B5u, 0x6F7Cu, CF || ZF);                                                                 /* 18C6 JA: a <= b */
    COMPARE(0x18CDu, 0x6F7Du, !CF);                                                                      /* 18DE JB: a >= b */
    STEP(0x18E5u); c->sp = c->bp;                                                                        /* 18E5 MOV SP,BP */
    STEP(0x18E7u); c->bp = pop(ram, c);                                                                  /* 18E7 POP BP */
    STEP(0x18E8u); c->ip = pop(ram, c); c->cs = pop(ram, c); c->sp = (uint16_t)(c->sp + 8u);             /* 18E8 RETF 8 */
    if (c->ip != ret_ip || c->cs != ret_cs) return PR_U25_1815_TRANSFER;
    return PR_U25_1815_RETURNED;
}
