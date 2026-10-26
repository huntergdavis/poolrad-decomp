/* Candidate-scout draft, no credit claimed. GAME.OVR unit 1Ch +[0292,02E0): ECL opcode 0x09 SAVE. Far Pascal, no arguments, RETF. Loads 2 operands; slot 1 is the value, slot 2 the destination; tag[1] >= 80h is a string SAVE, else a word SAVE. */
#include "poolrad/recovered/overlay_u1c_save_0292_cpu.h"

#define RAM_SIZE ((size_t)1u << 20)
#define MASK (RAM_SIZE - 1u)
#define STATUS(s) PR_U1C_SAVE_0292_##s
typedef pr_candidate_cpu pr_cpu_t;
typedef pr_u1c_save_0292_status pr_status_t;
static inline size_t physical(uint16_t seg, uint16_t off) { return (((size_t)seg << 4u) + off) & MASK; }
static inline uint16_t word_at(uint8_t *b, size_t at) { return (uint16_t)(b[at] | ((uint16_t)b[(at + 1u) & MASK] << 8u)); }
static inline uint8_t rd8(uint8_t *b, uint16_t seg, uint16_t off) { return b[physical(seg, off)]; }
static inline uint16_t rd16(uint8_t *b, uint16_t seg, uint16_t off) { return word_at(b, physical(seg, off)); }
static inline void wr16(uint8_t *b, uint16_t seg, uint16_t off, uint16_t v)
{
    size_t at = physical(seg, off);
    b[at] = (uint8_t)v; b[(at + 1u) & MASK] = (uint8_t)(v >> 8u);
}
static inline void push(uint8_t *b, pr_cpu_t *c, uint16_t v) { c->sp = (uint16_t)(c->sp - 2u); wr16(b, c->ss, c->sp, v); }
static inline uint16_t pop(uint8_t *b, pr_cpu_t *c) { uint16_t v = rd16(b, c->ss, c->sp); c->sp = (uint16_t)(c->sp + 2u); return v; }
static inline void szp(pr_cpu_t *c, uint16_t r, unsigned bits)
{
    uint8_t p = (uint8_t)r;
    p ^= (uint8_t)(p >> 4u); p ^= (uint8_t)(p >> 2u); p ^= (uint8_t)(p >> 1u);
    if ((p & 1u) == 0u) c->flags |= 0x04u;
    if ((r & (bits == 8u ? 0xFFu : 0xFFFFu)) == 0u) c->flags |= 0x40u;
    if (r & (bits == 8u ? 0x80u : 0x8000u)) c->flags |= 0x80u;
}
static inline uint16_t arith(pr_cpu_t *c, uint16_t l, uint16_t r, unsigned carry, unsigned bits, unsigned sub)
{
    uint32_t mask = bits == 8u ? 0xFFu : 0xFFFFu, sign = bits == 8u ? 0x80u : 0x8000u;
    uint32_t wide = sub ? (uint32_t)l - r - carry : (uint32_t)l + r + carry;
    uint16_t res = (uint16_t)(wide & mask);
    c->flags &= (uint16_t)~0x08D5u;
    if (sub ? (uint32_t)l < (uint32_t)r + carry : wide > mask) c->flags |= 1u;
    if ((l ^ r ^ res) & 0x10u) c->flags |= 0x10u;
    if ((sub ? (l ^ r) & (l ^ res) : (uint16_t)~(l ^ r) & (l ^ res)) & sign) c->flags |= 0x800u;
    szp(c, res, bits); return res;
}

pr_u1c_save_0292_status pr_u1c_save_0292_cpu(uint8_t *b, size_t ram_size,
                               pr_candidate_cpu *c, uint16_t caller_cs,
                               void *context)
{
    pr_candidate_ctx *dg = (pr_candidate_ctx *)context;
    uint16_t caller;
    (void)caller_cs;
    if (b == NULL || c == NULL || ram_size != RAM_SIZE) return STATUS(ARGUMENT);
    caller = c->cs;
    for (;;) {
        switch (c->ip) {
        case 0x0292: push(b,c,c->bp);c->ip=0x0293u;break; /* push bp */
        case 0x0293: c->bp=(uint16_t)(c->sp);c->ip=0x0295u;break; /* mov bp, sp */
        case 0x0295: c->sp=(uint16_t)(arith(c,c->sp,0x0004u,0u,16u,1u));c->ip=0x0298u;break; /* sub sp, 4 */
        case 0x0298: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(0x02u));c->ip=0x029Au;break; /* mov al, 2 */
        case 0x029A: push(b,c,c->ax);c->ip=0x029Bu;break; /* push ax */
        case 0x029B: push(b,c,c->cs);push(b,c,0x02A0u);c->cs=rd16(b,c->cs,0x029Eu);c->ip=0x002Au;{int s=pr_candidate_load_operands(b,c,dg);if(s)return (pr_status_t)s;}if(c->cs!=caller||c->ip!=0x02A0u)return STATUS(TRANSFER);break; /* lcall 0x2b, 0x2a */
        case 0x02A0: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ds,(uint16_t)(0x7087u))));c->ip=0x02A3u;break; /* mov al, byte ptr [0x7087] */
        case 0x02A3: push(b,c,c->ax);c->ip=0x02A4u;break; /* push ax */
        case 0x02A4: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ds,(uint16_t)(0x7047u))));c->ip=0x02A7u;break; /* mov al, byte ptr [0x7047] */
        case 0x02A7: push(b,c,c->ax);c->ip=0x02A8u;break; /* push ax */
        case 0x02A8: push(b,c,c->cs);push(b,c,0x02ADu);c->cs=rd16(b,c->cs,0x02ABu);c->ip=0x004Du;{int s=pr_candidate_operand_word(b,c,dg);if(s)return (pr_status_t)s;}if(c->cs!=caller||c->ip!=0x02ADu)return STATUS(TRANSFER);break; /* lcall 0x2b, 0x4d */
        case 0x02AD: wr16(b,c->ss,(uint16_t)(c->bp+0xFFFCu),(uint16_t)(c->ax));c->ip=0x02B0u;break; /* mov word ptr [bp - 4], ax */
        case 0x02B0: (void)arith(c,rd8(b,c->ds,(uint16_t)(0x7006u)),0x80u,0u,8u,1u);c->ip=0x02B5u;break; /* cmp byte ptr [0x7006], 0x80 */
        case 0x02B5: if(!(c->flags&1u)){c->ip=0x02CFu;}else c->ip=0x02B7u;break; /* jae 0x2cf */
        case 0x02B7: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(0x01u));c->ip=0x02B9u;break; /* mov al, 1 */
        case 0x02B9: push(b,c,c->ax);c->ip=0x02BAu;break; /* push ax */
        case 0x02BA: push(b,c,c->cs);push(b,c,0x02BFu);c->cs=rd16(b,c->cs,0x02BDu);c->ip=0x0025u;{int s=pr_candidate_slot_value(b,c,dg);if(s)return (pr_status_t)s;}if(c->cs!=caller||c->ip!=0x02BFu)return STATUS(TRANSFER);break; /* lcall 0x2b, 0x25 */
        case 0x02BF: wr16(b,c->ss,(uint16_t)(c->bp+0xFFFEu),(uint16_t)(c->ax));c->ip=0x02C2u;break; /* mov word ptr [bp - 2], ax */
        case 0x02C2: push(b,c,rd16(b,c->ss,(uint16_t)(c->bp+0xFFFCu)));c->ip=0x02C5u;break; /* push word ptr [bp - 4] */
        case 0x02C5: push(b,c,rd16(b,c->ss,(uint16_t)(c->bp+0xFFFEu)));c->ip=0x02C8u;break; /* push word ptr [bp - 2] */
        case 0x02C8: push(b,c,c->cs);push(b,c,0x02CDu);c->cs=rd16(b,c->cs,0x02CBu);c->ip=0x0052u;{int s=pr_candidate_store_word(b,c,dg);if(s)return (pr_status_t)s;}if(c->cs!=caller||c->ip!=0x02CDu)return STATUS(TRANSFER);break; /* lcall 0x2b, 0x52 */
        case 0x02CD: c->ip=0x02DCu;break; /* jmp 0x2dc */
        case 0x02CF: push(b,c,rd16(b,c->ss,(uint16_t)(c->bp+0xFFFCu)));c->ip=0x02D2u;break; /* push word ptr [bp - 4] */
        case 0x02D2: c->di=(uint16_t)(0x70C8u);c->ip=0x02D5u;break; /* mov di, 0x70c8 */
        case 0x02D5: push(b,c,c->ds);c->ip=0x02D6u;break; /* push ds */
        case 0x02D6: push(b,c,c->di);c->ip=0x02D7u;break; /* push di */
        case 0x02D7: push(b,c,c->cs);push(b,c,0x02DCu);c->cs=rd16(b,c->cs,0x02DAu);c->ip=0x005Cu;{int s=pr_candidate_store_string(b,c,dg);if(s)return (pr_status_t)s;}if(c->cs!=caller||c->ip!=0x02DCu)return STATUS(TRANSFER);break; /* lcall 0x2b, 0x5c */
        case 0x02DC: c->sp=(uint16_t)(c->bp);c->ip=0x02DEu;break; /* mov sp, bp */
        case 0x02DE: c->bp=(uint16_t)(pop(b,c));c->ip=0x02DFu;break; /* pop bp */
        case 0x02DF: c->ip=pop(b,c);c->cs=pop(b,c);c->sp=(uint16_t)(c->sp+0u);return STATUS(OK);break; /* retf  */
        default: return STATUS(TRANSFER);
        }
    }
}
