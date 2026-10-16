#include "poolrad/replay/u6f_1446_fullstate.h"
#include "poolrad/recovered/overlay_u9b_entry_zero_0000.h"

#define RAM_SIZE (1u << 20)
#define RAM_MASK (RAM_SIZE - 1u)
enum reg { AX, BX, CX, DX, SI, DI, BP, SP, CS, DS, ES, SS };
enum address_kind { DS_ABS, SS_BP, ES_DI, SS_BP_DI, SS_BX, ES_BX_DI };
enum op { PUSH, POP, MOV, IMM, IMM_AL, LOAD, STORE, LOAD_AL, STORE_BYTE,
    LES, LEA_DI, SUB_SP, OR_AX, JE, JNE, JAE, JMP, FAR_CALL, FAR_RET,
    XOR_AH, CMP_BYTE, INC_BYTE, INC_WORD, PUSH_WORD, CLD, REP_STOS,
    MOV_BL_AL, XOR_BH, IMM_CL, SHR_BX, MOV_CL_AL, AND_CL, SHL_AL, TEST_SET };
typedef struct {
    uint16_t ip, next;
    unsigned op, a, b;
    uint16_t operand;
} instruction;

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & RAM_MASK;
}
static uint16_t word(const uint8_t *ram, size_t p)
{
    return (uint16_t)(ram[p] | ((uint16_t)ram[(p + 1u) & RAM_MASK] << 8));
}
static void put_word(uint8_t *ram, size_t p, uint16_t value)
{
    ram[p] = (uint8_t)value;
    ram[(p + 1u) & RAM_MASK] = (uint8_t)(value >> 8);
}
static size_t address(const pr_slot_a_directory_cpu *c, unsigned kind,
                       uint16_t displacement)
{
    uint16_t seg = c->ds, off = displacement;
    switch (kind) {
    case DS_ABS: break;
    case SS_BP: seg = c->ss; off = (uint16_t)(c->bp + off); break;
    case ES_DI: seg = c->es; off = (uint16_t)(c->di + off); break;
    case SS_BP_DI: seg = c->ss; off = (uint16_t)(c->bp + c->di + off); break;
    case SS_BX: seg = c->ss; off = (uint16_t)(c->bx + off); break;
    case ES_BX_DI: seg = c->es; off = (uint16_t)(c->bx + c->di + off); break;
    }
    return physical(seg, off);
}
static uint16_t logic_flags(uint16_t old, unsigned value, unsigned bits)
{
    unsigned parity = value & 255u;
    uint16_t f = (uint16_t)(old & (uint16_t)~0x08d5u);
    parity ^= parity >> 4; parity ^= parity >> 2; parity ^= parity >> 1;
    if (!(parity & 1u)) f |= 4u;
    if (!(value & (bits == 8u ? 255u : 65535u))) f |= 0x40u;
    if (value & (1u << (bits - 1u))) f |= 0x80u;
    return f;
}
static uint16_t arithmetic_flags(uint16_t old, unsigned a, unsigned b,
                                 unsigned bits, int subtract)
{
    unsigned mask = bits == 8u ? 255u : 65535u;
    unsigned sign = 1u << (bits - 1u);
    unsigned value = subtract ? (a - b) & mask : (a + b) & mask;
    uint16_t f = logic_flags(old, value, bits);
    if (subtract ? a < b : a + b > mask) f |= 1u;
    if ((a ^ b ^ value) & 0x10u) f |= 0x10u;
    if ((subtract ? (a ^ b) : ~(a ^ b)) & (a ^ value) & sign) f |= 0x800u;
    return f;
}

#define I(ip,next,op,a,b,n) {ip,next,op,a,b,n}
static const instruction caller[] = {
    I(0x1446,0x1447,PUSH,BP,0,0), I(0x1447,0x1449,MOV,BP,SP,0),
    I(0x1449,0x144c,SUB_SP,0,0,16), I(0x144c,0x144f,LES,0,SS_BP,6),
    I(0x144f,0x1455,STORE_BYTE,255,ES_DI,0xbf),
    I(0x1455,0x1458,LOAD,AX,DS_ABS,0x5d96),
    I(0x1458,0x145c,OR_AX,0,DS_ABS,0x5d98),
    I(0x145c,0x145e,JNE,0,0,0x147a),
    I(0x145e,0x1461,LOAD,AX,SS_BP,6), I(0x1461,0x1464,LOAD,DX,SS_BP,8),
    I(0x1464,0x1467,STORE,AX,DS_ABS,0x5d96),
    I(0x1467,0x146b,STORE,DX,DS_ABS,0x5d98),
    I(0x146b,0x146e,LOAD,AX,SS_BP,6), I(0x146e,0x1471,LOAD,DX,SS_BP,8),
    I(0x1471,0x1474,STORE,AX,DS_ABS,0x5d92),
    I(0x1474,0x1478,STORE,DX,DS_ABS,0x5d94),
    I(0x1478,0x147a,JMP,0,0,0x14be),
    I(0x147a,0x147d,LOAD,AX,DS_ABS,0x5d96),
    I(0x147d,0x1481,LOAD,DX,DS_ABS,0x5d98),
    I(0x1481,0x1484,STORE,AX,SS_BP,0xfffc),
    I(0x1484,0x1487,STORE,DX,SS_BP,0xfffe),
    I(0x1487,0x148a,LES,0,SS_BP,0xfffc),
    I(0x148a,0x148f,LOAD,AX,ES_DI,0x104),
    I(0x148f,0x1494,OR_AX,0,ES_DI,0x106),
    I(0x1494,0x1496,JE,0,0,0x14ab),
    I(0x1496,0x1499,LES,0,SS_BP,0xfffc),
    I(0x1499,0x149e,LOAD,AX,ES_DI,0x104),
    I(0x149e,0x14a3,LOAD,DX,ES_DI,0x106),
    I(0x14a3,0x14a6,STORE,AX,SS_BP,0xfffc),
    I(0x14a6,0x14a9,STORE,DX,SS_BP,0xfffe),
    I(0x14a9,0x14ab,JMP,0,0,0x1487),
    I(0x14ab,0x14ae,LOAD,AX,SS_BP,6), I(0x14ae,0x14b1,LOAD,DX,SS_BP,8),
    I(0x14b1,0x14b4,LES,0,SS_BP,0xfffc),
    I(0x14b4,0x14b9,STORE,AX,ES_DI,0x104),
    I(0x14b9,0x14be,STORE,DX,ES_DI,0x106),
    I(0x14be,0x14c1,LEA_DI,0,0,0xfff4),
    I(0x14c1,0x14c2,PUSH,SS,0,0), I(0x14c2,0x14c3,PUSH,DI,0,0),
    I(0x14c3,0x14c6,IMM,AX,0,8), I(0x14c6,0x14c7,PUSH,AX,0,0),
    I(0x14c7,0x14c9,IMM_AL,0,0,0), I(0x14c9,0x14ca,PUSH,AX,0,0),
    I(0x14ca,0x14cf,FAR_CALL,0x131c,0,0x16a8),
    I(0x14cf,0x14d2,LOAD,AX,DS_ABS,0x5d96),
    I(0x14d2,0x14d6,LOAD,DX,DS_ABS,0x5d98),
    I(0x14d6,0x14d9,STORE,AX,SS_BP,0xfffc),
    I(0x14d9,0x14dc,STORE,DX,SS_BP,0xfffe),
    I(0x14dc,0x14df,LOAD,AX,SS_BP,0xfffc),
    I(0x14df,0x14e2,OR_AX,0,SS_BP,0xfffe),
    I(0x14e2,0x14e4,JE,0,0,0x151e),
    I(0x14e4,0x14e7,LES,0,SS_BP,0xfffc),
    I(0x14e7,0x14ec,LOAD_AL,0,ES_DI,0xbf),
    I(0x14ec,0x14ed,PUSH,AX,0,0), I(0x14ed,0x14f0,IMM,DI,0,0x1426),
    I(0x14f0,0x14f1,PUSH,CS,0,0), I(0x14f1,0x14f2,PUSH,DI,0,0),
    I(0x14f2,0x14f7,FAR_CALL,0x131c,0,0x0dc0),
    I(0x14f7,0x14f9,JE,0,0,0x1509),
    I(0x14f9,0x14fc,LES,0,SS_BP,0xfffc),
    I(0x14fc,0x1501,LOAD_AL,0,ES_DI,0xbf),
    I(0x1501,0x1503,XOR_AH,0,0,0), I(0x1503,0x1505,MOV,DI,AX,0),
    I(0x1505,0x1509,STORE_BYTE,1,SS_BP_DI,0xfff4),
    I(0x1509,0x150c,LES,0,SS_BP,0xfffc),
    I(0x150c,0x1511,LOAD,AX,ES_DI,0x104),
    I(0x1511,0x1516,LOAD,DX,ES_DI,0x106),
    I(0x1516,0x1519,STORE,AX,SS_BP,0xfffc),
    I(0x1519,0x151c,STORE,DX,SS_BP,0xfffe),
    I(0x151c,0x151e,JMP,0,0,0x14dc),
    I(0x151e,0x1521,LOAD,AX,SS_BP,6), I(0x1521,0x1524,LOAD,DX,SS_BP,8),
    I(0x1524,0x1527,STORE,AX,DS_ABS,0x5d92),
    I(0x1527,0x152b,STORE,DX,DS_ABS,0x5d94),
    I(0x152b,0x152f,LES,0,DS_ABS,0x5d92),
    I(0x152f,0x1532,STORE,DI,SS_BP,0xfff0),
    I(0x1532,0x1535,STORE,ES,SS_BP,0xfff2),
    I(0x1535,0x1538,LES,0,SS_BP,0xfff0),
    I(0x1538,0x153e,STORE_BYTE,0,ES_DI,0xbf),
    I(0x153e,0x1541,LES,0,SS_BP,0xfff0),
    I(0x1541,0x1547,CMP_BYTE,8,ES_DI,0xbf),
    I(0x1547,0x1549,JAE,0,0,0x1565),
    I(0x1549,0x154c,LES,0,SS_BP,0xfff0),
    I(0x154c,0x1551,LOAD_AL,0,ES_DI,0xbf),
    I(0x1551,0x1553,XOR_AH,0,0,0), I(0x1553,0x1555,MOV,DI,AX,0),
    I(0x1555,0x1559,CMP_BYTE,0,SS_BP_DI,0xfff4),
    I(0x1559,0x155b,JE,0,0,0x1565),
    I(0x155b,0x155e,LES,0,SS_BP,0xfff0),
    I(0x155e,0x1563,INC_BYTE,0,ES_DI,0xbf),
    I(0x1563,0x1565,JMP,0,0,0x153e),
    I(0x1565,0x1569,LES,0,DS_ABS,0x49d6),
    I(0x1569,0x156e,INC_WORD,0,ES_DI,0x67c),
    I(0x156e,0x1571,PUSH_WORD,0,SS_BP,8),
    I(0x1571,0x1574,PUSH_WORD,0,SS_BP,6),
    I(0x1574,0x1579,FAR_CALL,0x08d0,0,0x0025),
    I(0x1579,0x157b,MOV,SP,BP,0), I(0x157b,0x157c,POP,BP,0,0),
    I(0x157c,0,FAR_RET,0,0,4)
};
static const instruction helpers[] = {
    I(0x16a8,0x16aa,MOV,BX,SP,0), I(0x16aa,0x16ae,LES,0,SS_BX,8),
    I(0x16ae,0x16b2,LOAD,CX,SS_BX,6), I(0x16b2,0x16b6,LOAD_AL,0,SS_BX,4),
    I(0x16b6,0x16b7,CLD,0,0,0), I(0x16b7,0x16b9,REP_STOS,0,0,0),
    I(0x16b9,0,FAR_RET,0,0,8),
    I(0x0dc0,0x0dc2,MOV,BX,SP,0), I(0x0dc2,0x0dc6,LES,0,SS_BX,4),
    I(0x0dc6,0x0dca,LOAD_AL,0,SS_BX,8),
    I(0x0dca,0x0dcc,MOV_BL_AL,0,0,0), I(0x0dcc,0x0dce,XOR_BH,0,0,0),
    I(0x0dce,0x0dd0,IMM_CL,0,0,3), I(0x0dd0,0x0dd2,SHR_BX,0,0,0),
    I(0x0dd2,0x0dd4,MOV_CL_AL,0,0,0), I(0x0dd4,0x0dd7,AND_CL,0,0,7),
    I(0x0dd7,0x0dd9,IMM_AL,0,0,1), I(0x0dd9,0x0ddb,SHL_AL,0,0,0),
    I(0x0ddb,0x0dde,TEST_SET,0,ES_BX_DI,0), I(0x0dde,0,FAR_RET,0,0,6)
};
#undef I

static uint8_t projected(const uint8_t *ram,
                         const pr_slot_a_directory_write *writes,
                         size_t count, size_t p)
{
    while (count) {
        --count;
        if (writes[count].linear == p) return writes[count].new_byte;
    }
    return ram[p];
}

static int valid_jump(const uint8_t *ram,
                       const pr_slot_a_directory_write *writes, size_t count)
{
    size_t p = physical(0x08d0u, 0x0025u);
    return projected(ram,writes,count,p) == 0xeau &&
           projected(ram,writes,count,p+1u) == 0u &&
           projected(ram,writes,count,p+2u) == 0u &&
           projected(ram,writes,count,p+3u) == 0xcdu &&
           projected(ram,writes,count,p+4u) == 0x2cu;
}

static uint16_t projected_word(const uint8_t *ram,
                               const pr_slot_a_directory_write *writes,
                               size_t count,size_t p)
{
    return (uint16_t)(projected(ram,writes,count,p)|
        ((uint16_t)projected(ram,writes,count,(p+1u)&RAM_MASK)<<8));
}

static pr_u6f_1446_status dispatch(
    pr_slot_a_directory_state *s, pr_u6f_1446_cold_dispatch callback,
    void *context, pr_slot_a_directory_write *writes, size_t capacity)
{
    size_t p = physical(0x08d0u, 0x0025u), count = 0, i;
    pr_slot_a_directory_cpu after;
    pr_u6f_1446_status result;
    if (s->ram[p] == 0xeau) {
        if (!valid_jump(s->ram, NULL, 0u)) return PR_U6F_1446_BLOCKED_THUNK;
        s->cpu.cs = 0x2ccdu; s->cpu.ip = 0u;
        return PR_U6F_1446_STEPPED;
    }
    if (s->ram[p] != 0xcdu || s->ram[p+1u] != 0x3fu ||
        word(s->ram,p+2u) != 0u) return PR_U6F_1446_BLOCKED_THUNK;
    if (!callback) return PR_U6F_1446_BLOCKED_COLD_DISPATCH;
    after = s->cpu;
    result = callback(context, &s->cpu, s->ram, s->ram_size,
                      &after, writes, capacity, &count);
    if (result == PR_U6F_1446_BLOCKED_COLD_DISPATCH) return result;
    if (result != PR_U6F_1446_STEPPED || count > capacity ||
        after.cs != 0x08d0u || after.ip != 0x0025u)
        return PR_U6F_1446_BAD_EVENT;
    for (i = 0; i < count; ++i)
        if (writes[i].linear >= RAM_SIZE ||
            writes[i].old_byte != projected(s->ram,writes,i,writes[i].linear))
            return PR_U6F_1446_BAD_EVENT;
    if (!valid_jump(s->ram,writes,count) || after.ss!=s->cpu.ss ||
        after.sp!=s->cpu.sp ||
        projected_word(s->ram,writes,count,
            physical(s->cpu.ss,(uint16_t)(s->cpu.sp-6u)))!=after.ip ||
        projected_word(s->ram,writes,count,
            physical(s->cpu.ss,(uint16_t)(s->cpu.sp-4u)))!=after.cs ||
        projected_word(s->ram,writes,count,
            physical(s->cpu.ss,(uint16_t)(s->cpu.sp-2u)))!=after.flags)
        return PR_U6F_1446_BAD_EVENT;
    for (i = 0; i < count; ++i)
        s->ram[writes[i].linear] = writes[i].new_byte;
    s->cpu = after;
    return PR_U6F_1446_STEPPED;
}

static pr_u6f_1446_status u9b(pr_slot_a_directory_state *s)
{
    pr_slot_a_directory_cpu *c = &s->cpu;
    pr_u9b_0000_cpu u = {
        c->ax,c->bx,c->cx,c->dx,c->si,c->di,c->bp,c->sp,
        c->cs,c->ip,c->ds,c->es,c->ss,c->flags
    };
    pr_u9b_0000_status result = pr_u9b_entry_zero_0000(s->ram,s->ram_size,&u);
    if (result == PR_U9B_0000_ARGUMENT) return PR_U6F_1446_ARGUMENT;
    c->ax=u.ax; c->bx=u.bx; c->cx=u.cx; c->dx=u.dx; c->si=u.si; c->di=u.di;
    c->bp=u.bp; c->sp=u.sp; c->cs=u.cs; c->ip=u.ip; c->ds=u.ds;
    c->es=u.es; c->ss=u.ss; c->flags=u.flags; /* FS/GS are not in u. */
    if (result == PR_U9B_0000_TRANSFER || c->cs != 0x2a8eu || c->ip != 0x1579u)
        return PR_U6F_1446_TRANSFER;
    return PR_U6F_1446_STEPPED;
}

pr_u6f_1446_status pr_u6f_1446_step(
    pr_slot_a_directory_state *s,
    pr_u6f_1446_cold_dispatch callback, void *context,
    pr_slot_a_directory_write *writes, size_t capacity)
{
    const instruction *table, *in = NULL;
    pr_slot_a_directory_cpu *c;
    size_t count, i, p;
    uint16_t next, value, saved_cf;
    unsigned n, v;
    int outer_return;
    if (!s || !s->ram || s->ram_size != RAM_SIZE || (capacity && !writes))
        return PR_U6F_1446_ARGUMENT;
    c = &s->cpu;
    if (c->cs == 0x08d0u && c->ip == 0x0025u)
        return dispatch(s,callback,context,writes,capacity);
    if (c->cs == 0x2ccdu && c->ip == 0u) return u9b(s);
    if (c->cs == 0x2a8eu) {
        table=caller; count=sizeof caller/sizeof caller[0];
    } else if (c->cs == 0x131cu) {
        table=helpers; count=sizeof helpers/sizeof helpers[0];
    } else return PR_U6F_1446_BLOCKED_PC;
    for (i=0; i<count; ++i) if (table[i].ip==c->ip) { in=table+i; break; }
    if (!in) return PR_U6F_1446_BLOCKED_PC;
    next=in->next;
    outer_return=c->cs==0x2a8eu && in->op==FAR_RET;
    {
        uint16_t *r[]={&c->ax,&c->bx,&c->cx,&c->dx,&c->si,&c->di,
                       &c->bp,&c->sp,&c->cs,&c->ds,&c->es,&c->ss};
        switch (in->op) {
        case PUSH: case PUSH_WORD:
            value=in->op==PUSH ? *r[in->a] :
                  word(s->ram,address(c,in->b,in->operand));
            c->sp=(uint16_t)(c->sp-2u);
            put_word(s->ram,physical(c->ss,c->sp),value); break;
        case POP:
            value=word(s->ram,physical(c->ss,c->sp));
            c->sp=(uint16_t)(c->sp+2u); *r[in->a]=value; break;
        case MOV: *r[in->a]=*r[in->b]; break;
        case IMM: *r[in->a]=in->operand; break;
        case IMM_AL: c->ax=(uint16_t)((c->ax&0xff00u)|in->operand); break;
        case LEA_DI: c->di=(uint16_t)(c->bp+in->operand); break;
        case SUB_SP:
            c->flags=arithmetic_flags(c->flags,c->sp,in->operand,16u,1);
            c->sp=(uint16_t)(c->sp-in->operand); break;
        case LOAD: case STORE: case LOAD_AL: case STORE_BYTE: case LES:
            p=address(c,in->b,in->operand);
            if (in->op==LOAD) *r[in->a]=word(s->ram,p);
            else if (in->op==STORE) put_word(s->ram,p,*r[in->a]);
            else if (in->op==LOAD_AL)
                c->ax=(uint16_t)((c->ax&0xff00u)|s->ram[p]);
            else if (in->op==STORE_BYTE) s->ram[p]=(uint8_t)in->a;
            else { c->di=word(s->ram,p); c->es=word(s->ram,(p+2u)&RAM_MASK); }
            break;
        case FAR_CALL:
            c->sp=(uint16_t)(c->sp-2u);
            put_word(s->ram,physical(c->ss,c->sp),c->cs);
            c->sp=(uint16_t)(c->sp-2u);
            put_word(s->ram,physical(c->ss,c->sp),next);
            c->cs=(uint16_t)in->a; next=in->operand; break;
        case FAR_RET:
            p=physical(c->ss,c->sp); next=word(s->ram,p);
            c->cs=word(s->ram,(p+2u)&RAM_MASK);
            c->sp=(uint16_t)(c->sp+4u+in->operand); break;
        case OR_AX:
            c->ax=(uint16_t)(c->ax|word(s->ram,address(c,in->b,in->operand)));
            c->flags=logic_flags(c->flags,c->ax,16u); break;
        case JE: if (c->flags&0x40u) next=in->operand; break;
        case JNE: if (!(c->flags&0x40u)) next=in->operand; break;
        case JAE: if (!(c->flags&1u)) next=in->operand; break;
        case JMP: next=in->operand; break;
        case XOR_AH:
            c->ax&=0xffu; c->flags=logic_flags(c->flags,0u,8u); break;
        case CMP_BYTE:
            c->flags=arithmetic_flags(c->flags,
                s->ram[address(c,in->b,in->operand)],in->a,8u,1); break;
        case INC_BYTE: case INC_WORD:
            p=address(c,in->b,in->operand); saved_cf=c->flags&1u;
            value=in->op==INC_BYTE ? s->ram[p] : word(s->ram,p);
            c->flags=(uint16_t)((arithmetic_flags(c->flags,value,1u,
                in->op==INC_BYTE ? 8u : 16u,0)&(uint16_t)~1u)|saved_cf);
            if (in->op==INC_BYTE) s->ram[p]=(uint8_t)(value+1u);
            else put_word(s->ram,p,(uint16_t)(value+1u));
            break;
        case CLD: c->flags&=(uint16_t)~0x400u; break;
        case REP_STOS:
            if (c->flags&0x400u) return PR_U6F_1446_BLOCKED_PC;
            n=c->cx;
            for (i=0; i<n; ++i) {
                s->ram[physical(c->es,c->di)]=(uint8_t)c->ax;
                ++c->di; --c->cx;
            }
            break;
        case MOV_BL_AL: c->bx=(uint16_t)((c->bx&0xff00u)|(c->ax&255u)); break;
        case XOR_BH:
            c->bx&=255u; c->flags=logic_flags(c->flags,0u,8u); break;
        case IMM_CL: c->cx=(uint16_t)((c->cx&0xff00u)|in->operand); break;
        case MOV_CL_AL: c->cx=(uint16_t)((c->cx&0xff00u)|(c->ax&255u)); break;
        case AND_CL:
            value=(uint16_t)((c->cx&255u)&in->operand);
            c->cx=(uint16_t)((c->cx&0xff00u)|value);
            c->flags=logic_flags(c->flags,value,8u); break;
        case SHR_BX:
            n=c->cx&255u;
            if (n>15u) return PR_U6F_1446_BLOCKED_PC;
            if (n) {
                v=c->bx; c->bx=(uint16_t)(v>>n);
                c->flags=(uint16_t)(logic_flags(c->flags,c->bx,16u)|
                                    ((v>>(n-1u))&1u));
                if (n==1u && (v&0x8000u)) c->flags|=0x800u;
            }
            break;
        case SHL_AL:
            n=c->cx&255u;
            if (n>7u) return PR_U6F_1446_BLOCKED_PC;
            if (n) {
                v=c->ax&255u; value=(uint16_t)((v<<n)&255u);
                c->ax=(uint16_t)((c->ax&0xff00u)|value);
                c->flags=(uint16_t)(logic_flags(c->flags,value,8u)|
                                    ((v>>(8u-n))&1u));
                if (n==1u && (((value>>7)^(c->flags&1u))&1u)) c->flags|=0x800u;
            }
            break;
        case TEST_SET:
            value=(uint16_t)((c->ax&255u)&s->ram[address(c,in->b,in->operand)]);
            c->flags=logic_flags(c->flags,value,8u); break;
        default: return PR_U6F_1446_BLOCKED_PC;
        }
    }
    c->ip=next;
    return outer_return ? PR_U6F_1446_RETURNED : PR_U6F_1446_STEPPED;
}
