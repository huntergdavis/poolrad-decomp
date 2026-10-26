/* Candidate-scout draft, no credit claimed. GAME.OVR unit 1Ch +[0EBC,0F08): ECL opcode 0x35 SAVE TABLE. Far Pascal, no arguments, RETF. Loads 3 operands; dest = operand_word(slot 2) + slot_value(slot 3), value = slot_value(slot 1); then a word store. */
#include "poolrad/recovered/overlay_u1c_save_table_0ebc_cpu.h"

#define RAM_SIZE ((size_t)1u << 20)
#define MASK (RAM_SIZE - 1u)
#define STATUS(s) PR_U1C_SAVE_TABLE_0EBC_##s
typedef pr_candidate_cpu pr_cpu_t;
typedef pr_u1c_save_table_0ebc_status pr_status_t;
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

pr_u1c_save_table_0ebc_status pr_u1c_save_table_0ebc_cpu(uint8_t *b, size_t ram_size,
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
        case 0x0EBC: push(b,c,c->bp);c->ip=0x0EBDu;break; /* push bp */
        case 0x0EBD: c->bp=(uint16_t)(c->sp);c->ip=0x0EBFu;break; /* mov bp, sp */
        case 0x0EBF: c->sp=(uint16_t)(arith(c,c->sp,0x0008u,0u,16u,1u));c->ip=0x0EC2u;break; /* sub sp, 8 */
        case 0x0EC2: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(0x03u));c->ip=0x0EC4u;break; /* mov al, 3 */
        case 0x0EC4: push(b,c,c->ax);c->ip=0x0EC5u;break; /* push ax */
        case 0x0EC5: push(b,c,c->cs);push(b,c,0x0ECAu);c->cs=rd16(b,c->cs,0x0EC8u);c->ip=0x002Au;{int s=pr_candidate_load_operands(b,c,dg);if(s)return (pr_status_t)s;}if(c->cs!=caller||c->ip!=0x0ECAu)return STATUS(TRANSFER);break; /* lcall 0x2b, 0x2a */
        case 0x0ECA: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(0x01u));c->ip=0x0ECCu;break; /* mov al, 1 */
        case 0x0ECC: push(b,c,c->ax);c->ip=0x0ECDu;break; /* push ax */
        case 0x0ECD: push(b,c,c->cs);push(b,c,0x0ED2u);c->cs=rd16(b,c->cs,0x0ED0u);c->ip=0x0025u;{int s=pr_candidate_slot_value(b,c,dg);if(s)return (pr_status_t)s;}if(c->cs!=caller||c->ip!=0x0ED2u)return STATUS(TRANSFER);break; /* lcall 0x2b, 0x25 */
        case 0x0ED2: wr16(b,c->ss,(uint16_t)(c->bp+0xFFFAu),(uint16_t)(c->ax));c->ip=0x0ED5u;break; /* mov word ptr [bp - 6], ax */
        case 0x0ED5: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ds,(uint16_t)(0x7087u))));c->ip=0x0ED8u;break; /* mov al, byte ptr [0x7087] */
        case 0x0ED8: push(b,c,c->ax);c->ip=0x0ED9u;break; /* push ax */
        case 0x0ED9: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ds,(uint16_t)(0x7047u))));c->ip=0x0EDCu;break; /* mov al, byte ptr [0x7047] */
        case 0x0EDC: push(b,c,c->ax);c->ip=0x0EDDu;break; /* push ax */
        case 0x0EDD: push(b,c,c->cs);push(b,c,0x0EE2u);c->cs=rd16(b,c->cs,0x0EE0u);c->ip=0x004Du;{int s=pr_candidate_operand_word(b,c,dg);if(s)return (pr_status_t)s;}if(c->cs!=caller||c->ip!=0x0EE2u)return STATUS(TRANSFER);break; /* lcall 0x2b, 0x4d */
        case 0x0EE2: wr16(b,c->ss,(uint16_t)(c->bp+0xFFFCu),(uint16_t)(c->ax));c->ip=0x0EE5u;break; /* mov word ptr [bp - 4], ax */
        case 0x0EE5: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(0x03u));c->ip=0x0EE7u;break; /* mov al, 3 */
        case 0x0EE7: push(b,c,c->ax);c->ip=0x0EE8u;break; /* push ax */
        case 0x0EE8: push(b,c,c->cs);push(b,c,0x0EEDu);c->cs=rd16(b,c->cs,0x0EEBu);c->ip=0x0025u;{int s=pr_candidate_slot_value(b,c,dg);if(s)return (pr_status_t)s;}if(c->cs!=caller||c->ip!=0x0EEDu)return STATUS(TRANSFER);break; /* lcall 0x2b, 0x25 */
        case 0x0EED: wr16(b,c->ss,(uint16_t)(c->bp+0xFFFEu),(uint16_t)(c->ax));c->ip=0x0EF0u;break; /* mov word ptr [bp - 2], ax */
        case 0x0EF0: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFCu)));c->ip=0x0EF3u;break; /* mov ax, word ptr [bp - 4] */
        case 0x0EF3: c->ax=(uint16_t)(arith(c,c->ax,rd16(b,c->ss,(uint16_t)(c->bp+0xFFFEu)),0u,16u,0u));c->ip=0x0EF6u;break; /* add ax, word ptr [bp - 2] */
        case 0x0EF6: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF8u),(uint16_t)(c->ax));c->ip=0x0EF9u;break; /* mov word ptr [bp - 8], ax */
        case 0x0EF9: push(b,c,rd16(b,c->ss,(uint16_t)(c->bp+0xFFF8u)));c->ip=0x0EFCu;break; /* push word ptr [bp - 8] */
        case 0x0EFC: push(b,c,rd16(b,c->ss,(uint16_t)(c->bp+0xFFFAu)));c->ip=0x0EFFu;break; /* push word ptr [bp - 6] */
        case 0x0EFF: push(b,c,c->cs);push(b,c,0x0F04u);c->cs=rd16(b,c->cs,0x0F02u);c->ip=0x0052u;{int s=pr_candidate_store_word(b,c,dg);if(s)return (pr_status_t)s;}if(c->cs!=caller||c->ip!=0x0F04u)return STATUS(TRANSFER);break; /* lcall 0x2b, 0x52 */
        case 0x0F04: c->sp=(uint16_t)(c->bp);c->ip=0x0F06u;break; /* mov sp, bp */
        case 0x0F06: c->bp=(uint16_t)(pop(b,c));c->ip=0x0F07u;break; /* pop bp */
        case 0x0F07: c->ip=pop(b,c);c->cs=pop(b,c);c->sp=(uint16_t)(c->sp+0u);return STATUS(OK);break; /* retf  */
        default: return STATUS(TRANSFER);
        }
    }
}
