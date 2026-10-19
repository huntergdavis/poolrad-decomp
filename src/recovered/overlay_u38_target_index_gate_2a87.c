/* GAME.OVR U38 public entry 99, +2A87..+2AE6 (95 original bytes). */
#include "poolrad/recovered/overlay_u38_target_index_gate_2a87.h"

#define RAM_MASK (PR_U38_101D_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u /* OF SF ZF AF PF CF */

static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & RAM_MASK;
}

static uint8_t read8(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    return ram[physical(seg, off)];
}

static uint16_t read16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t a = physical(seg, off);
    return (uint16_t)(ram[a] | ((uint16_t)ram[(a + 1u) & RAM_MASK] << 8));
}

static void write8(uint8_t *ram, uint16_t seg, uint16_t off, uint8_t value)
{
    ram[physical(seg, off)] = value;
}

static void write16(uint8_t *ram, uint16_t seg, uint16_t off,
                    uint16_t value)
{
    size_t a = physical(seg, off);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & RAM_MASK] = (uint8_t)(value >> 8);
}

static void push16(uint8_t *ram, pr_u38_101d_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static uint16_t pop16(const uint8_t *ram, pr_u38_101d_cpu *cpu)
{
    uint16_t value = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return value;
}

static uint8_t parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (uint8_t)((value & 1u) == 0u);
}

static void logic16(pr_u38_101d_cpu *cpu, uint16_t value)
{
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (parity((uint8_t)value)) cpu->flags |= 4u;
    if (value == 0u) cpu->flags |= 0x40u;
    if (value & 0x8000u) cpu->flags |= 0x80u;
}

static void compare8_zero(pr_u38_101d_cpu *cpu, uint8_t value)
{
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (parity(value)) cpu->flags |= 4u;
    if (value == 0u) cpu->flags |= 0x40u;
    if (value & 0x80u) cpu->flags |= 0x80u;
}

static void sub16_sp(pr_u38_101d_cpu *cpu, uint16_t value)
{
    uint16_t before = cpu->sp;
    uint16_t result = (uint16_t)(before - value);
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (before < value) cpu->flags |= 1u;
    if (parity((uint8_t)result)) cpu->flags |= 4u;
    if ((before ^ value ^ result) & 0x10u) cpu->flags |= 0x10u;
    if (result == 0u) cpu->flags |= 0x40u;
    if (result & 0x8000u) cpu->flags |= 0x80u;
    if (((before ^ value) & (before ^ result) & 0x8000u) != 0u)
        cpu->flags |= 0x800u;
    cpu->sp = result;
}

pr_u38_2a87_status pr_u38_target_index_gate_2a87(
    uint8_t *ram, size_t ram_size, pr_u38_101d_cpu *cpu,
    pr_u38_101d_select select_target, void *context)
{
    uint16_t amount;
    uint8_t flag;
    if (ram == NULL || cpu == NULL || select_target == NULL ||
        ram_size < PR_U38_101D_RAM_SIZE || cpu->ip != 0x2A87u)
        return PR_U38_2A87_ARGUMENT;
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    sub16_sp(cpu, 4u);
    push16(ram, cpu, read16(ram, cpu->ds, 0x5D94u));
    push16(ram, cpu, read16(ram, cpu->ds, 0x5D92u));
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, 0x2A99u);
    cpu->ip = 0x101Du;
    if (pr_u38_target_select_101d(ram, ram_size, cpu,
                                    select_target, context) != PR_U38_101D_OK)
        return PR_U38_2A87_CALL_ABORT;
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u), cpu->ax);
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), cpu->dx);
    cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u));
    cpu->ax |= read16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u));
    logic16(cpu, cpu->ax);
    if (cpu->ax != 0u) {
        cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u));
        cpu->es = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u));
        cpu->ax = read8(ram, cpu->es, (uint16_t)(cpu->di + 0x2Eu));
        logic16(cpu, 0u); /* XOR AH,AH */
        cpu->di = (uint16_t)(cpu->ax << 4);
        cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | 4u);
        flag = read8(ram, cpu->ds, (uint16_t)(cpu->di + 0x5589u));
        compare8_zero(cpu, flag);
        if (flag != 0u) {
            cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u));
            cpu->es = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u));
            cpu->ax = read8(ram, cpu->es, (uint16_t)(cpu->di + 0x2Eu));
            logic16(cpu, 0u); /* XOR AH,AH */
            cpu->di = (uint16_t)(cpu->ax << 4);
            cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | 4u);
            cpu->ax = (uint16_t)(read8(ram, cpu->ds,
                                      (uint16_t)(cpu->di + 0x5589u)) & 1u);
            compare8_zero(cpu, (uint8_t)cpu->ax); /* OR AL,AL */
            if (cpu->ax == 0u) goto result;
        }
        amount = read8(ram, cpu->ds, 0x6818u);
        cpu->ax = amount; /* MOV AL; XOR AH,AH */
        logic16(cpu, 0u);
        cpu->ax = (uint16_t)(amount >> 1);
        logic16(cpu, cpu->ax); /* SHR AX,1: OF=0 for zero-extended AX */
        if (amount & 1u) cpu->flags |= 1u;
        write8(ram, cpu->ds, 0x6818u, (uint8_t)cpu->ax);
    }
result:
    cpu->sp = cpu->bp;
    cpu->bp = pop16(ram, cpu);
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 10u);
    return PR_U38_2A87_OK;
}

/* Complete original instructions; docs/re/U38_TARGET_INDEX_GATE_2A87_CPU.md. */
#define MASK (PR_TARGET_CPU_RAM_SIZE - 1u)
static size_t phys(uint16_t s,uint16_t o) {return (((size_t)s<<4)+o)&MASK;}
static uint16_t word_at(const uint8_t *m,size_t p) {return (uint16_t)(m[p&MASK]|((uint16_t)m[(p+1u)&MASK]<<8));}
static uint16_t get(const uint8_t *m,uint16_t s,uint16_t o,unsigned bits)
{size_t p=phys(s,o);return bits==8?m[p]:word_at(m,p);}
static void put(uint8_t *m,uint16_t s,uint16_t o,unsigned bits,uint16_t v)
{size_t p=phys(s,o);m[p]=(uint8_t)v;if(bits==16)m[(p+1u)&MASK]=(uint8_t)(v>>8);}
static void push(uint8_t *m,pr_target_cpu *c,uint16_t v)
{c->sp=(uint16_t)(c->sp-2u);put(m,c->ss,c->sp,16,v);}
static uint16_t logic(pr_target_cpu *c,uint16_t v,unsigned bits)
{
    unsigned p=v&255u;p^=p>>4;p^=p>>2;p^=p>>1;
    c->flags&=(uint16_t)~0x08D5u;
    if(!(p&1u))c->flags|=4u;
    if(!v)c->flags|=64u;
    if(v&(bits==8?128u:32768u))c->flags|=128u;
    return v;
}
static uint16_t sub(pr_target_cpu *c,uint16_t a,uint16_t b,unsigned bits)
{
    uint16_t r=(uint16_t)((a-b)&(bits==8?255u:65535u));
    (void)logic(c,r,bits);
    if(a<b)c->flags|=1u;
    if((a^b^r)&16u)c->flags|=16u;
    if((a^b)&(a^r)&(bits==8?128u:32768u))c->flags|=0x800u;
    return r;
}

pr_target_status pr_u38_target_index_gate_2a87_cpu(uint8_t *m,size_t size,pr_target_cpu *c,pr_target_shift_profile shift_profile,pr_target_diagnostic *d)
{
    size_t at;
    uint16_t source_cs; int status;
    uint16_t value;
    if(m==NULL||c==NULL||size!=PR_TARGET_CPU_RAM_SIZE||c->ip!=0x2A87u||c->sp<0x002Eu||c->sp>0xFFFCu||(shift_profile!=PR_TARGET_SHIFT_UNICORN&&shift_profile!=PR_TARGET_SHIFT_DOSBOX386)) return PR_TARGET_ARGUMENT;
    if(d!=NULL){d->selector_status=-1;d->resident_status=-1;}
    for(;;) switch(c->ip) {
    case 0x2A87u: /* push bp */
        push(m,c,c->bp);
        c->ip=0x2A88u;
        break;
    case 0x2A88u: /* mov bp, sp */
        c->bp=(uint16_t)(c->sp);
        c->ip=0x2A8Au;
        break;
    case 0x2A8Au: /* sub sp, 4 */
        c->sp=(uint16_t)(sub(c,c->sp,0x4u,16));
        c->ip=0x2A8Du;
        break;
    case 0x2A8Du: /* push word ptr [0x5d94] */
        push(m,c,get(m,c->ds,(uint16_t)(0x5D94u),16));
        c->ip=0x2A91u;
        break;
    case 0x2A91u: /* push word ptr [0x5d92] */
        push(m,c,get(m,c->ds,(uint16_t)(0x5D92u),16));
        c->ip=0x2A95u;
        break;
    case 0x2A95u: /* push cs */
        push(m,c,c->cs);
        c->ip=0x2A96u;
        break;
    case 0x2A96u: /* call 0x101d */
        if(get(m,c->cs,0x2A96u,8)!=0xE8u||get(m,c->cs,0x2A97u,16)!=0xE584u)return PR_TARGET_CODE;
        source_cs=c->cs;
        push(m,c,0x2A99u);
        c->ip=0x101Du;
        status=pr_u38_target_select_101d_cpu(m,size,c,d);
        if(d!=NULL)d->selector_status=status;
        if(status!=PR_TARGET_RETURNED)return PR_TARGET_CALLEE;
        if(c->cs!=source_cs||c->ip!=0x2A99u)return PR_TARGET_TRANSFER;
        break;
    case 0x2A99u: /* mov word ptr [bp - 4], ax */
        put(m,c->ss,(uint16_t)(c->bp - 0x4u),16,c->ax);
        c->ip=0x2A9Cu;
        break;
    case 0x2A9Cu: /* mov word ptr [bp - 2], dx */
        put(m,c->ss,(uint16_t)(c->bp - 0x2u),16,c->dx);
        c->ip=0x2A9Fu;
        break;
    case 0x2A9Fu: /* mov ax, word ptr [bp - 4] */
        c->ax=(uint16_t)(get(m,c->ss,(uint16_t)(c->bp - 0x4u),16));
        c->ip=0x2AA2u;
        break;
    case 0x2AA2u: /* or ax, word ptr [bp - 2] */
        c->ax=(uint16_t)(logic(c,(uint16_t)(c->ax|get(m,c->ss,(uint16_t)(c->bp - 0x2u),16)),16));
        c->ip=0x2AA5u;
        break;
    case 0x2AA5u: /* je 0x2ae0 */
        c->ip=(c->flags&64u)?0x2AE0u:0x2AA7u;
        break;
    case 0x2AA7u: /* les di, ptr [bp - 4] */
        at=phys(c->ss,(uint16_t)(c->bp - 0x4u));
        c->di=(uint16_t)(word_at(m,at));
        c->es=word_at(m,at+2u);
        c->ip=0x2AAAu;
        break;
    case 0x2AAAu: /* mov al, byte ptr es:[di + 0x2e] */
        c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(get(m,c->es,(uint16_t)(c->di + 0x2Eu),8)));
        c->ip=0x2AAEu;
        break;
    case 0x2AAEu: /* xor ah, ah */
        c->ax=(uint16_t)((c->ax&255u)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax >> 8)^(uint8_t)(c->ax >> 8)),8))<<8));
        c->ip=0x2AB0u;
        break;
    case 0x2AB0u: /* mov di, ax */
        c->di=(uint16_t)(c->ax);
        c->ip=0x2AB2u;
        break;
    case 0x2AB2u: /* mov cl, 4 */
        c->cx=(uint16_t)((c->cx&0xFF00u)|(uint8_t)(0x4u));
        c->ip=0x2AB4u;
        break;
    case 0x2AB4u: /* shl di, cl */
        /* CL=4; flags are overwritten by subsequent CMP or AND/OR before observation. */
        c->di=(uint16_t)(c->di<<4);
        c->ip=0x2AB6u;
        break;
    case 0x2AB6u: /* cmp byte ptr [di + 0x5589], 0 */
        (void)sub(c,get(m,c->ds,(uint16_t)(c->di + 0x5589u),8),0x0u,8);
        c->ip=0x2ABBu;
        break;
    case 0x2ABBu: /* je 0x2ad6 */
        c->ip=(c->flags&64u)?0x2AD6u:0x2ABDu;
        break;
    case 0x2ABDu: /* les di, ptr [bp - 4] */
        at=phys(c->ss,(uint16_t)(c->bp - 0x4u));
        c->di=(uint16_t)(word_at(m,at));
        c->es=word_at(m,at+2u);
        c->ip=0x2AC0u;
        break;
    case 0x2AC0u: /* mov al, byte ptr es:[di + 0x2e] */
        c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(get(m,c->es,(uint16_t)(c->di + 0x2Eu),8)));
        c->ip=0x2AC4u;
        break;
    case 0x2AC4u: /* xor ah, ah */
        c->ax=(uint16_t)((c->ax&255u)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax >> 8)^(uint8_t)(c->ax >> 8)),8))<<8));
        c->ip=0x2AC6u;
        break;
    case 0x2AC6u: /* mov di, ax */
        c->di=(uint16_t)(c->ax);
        c->ip=0x2AC8u;
        break;
    case 0x2AC8u: /* mov cl, 4 */
        c->cx=(uint16_t)((c->cx&0xFF00u)|(uint8_t)(0x4u));
        c->ip=0x2ACAu;
        break;
    case 0x2ACAu: /* shl di, cl */
        /* CL=4; flags are overwritten by subsequent CMP or AND/OR before observation. */
        c->di=(uint16_t)(c->di<<4);
        c->ip=0x2ACCu;
        break;
    case 0x2ACCu: /* mov al, byte ptr [di + 0x5589] */
        c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(get(m,c->ds,(uint16_t)(c->di + 0x5589u),8)));
        c->ip=0x2AD0u;
        break;
    case 0x2AD0u: /* and al, 1 */
        c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax)&0x1u),8)));
        c->ip=0x2AD2u;
        break;
    case 0x2AD2u: /* or al, al */
        c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax)|(uint8_t)(c->ax)),8)));
        c->ip=0x2AD4u;
        break;
    case 0x2AD4u: /* je 0x2ae0 */
        c->ip=(c->flags&64u)?0x2AE0u:0x2AD6u;
        break;
    case 0x2AD6u: /* mov al, byte ptr [0x6818] */
        c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(get(m,c->ds,(uint16_t)(0x6818u),8)));
        c->ip=0x2AD9u;
        break;
    case 0x2AD9u: /* xor ah, ah */
        c->ax=(uint16_t)((c->ax&255u)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax >> 8)^(uint8_t)(c->ax >> 8)),8))<<8));
        c->ip=0x2ADBu;
        break;
    case 0x2ADBu: /* shr ax, 1 */
        value=c->ax;
        c->ax=logic(c,(uint16_t)(value>>1),16);
        if(value&1u)c->flags|=1u;
        if(value&0x8000u)c->flags|=0x800u;
        if(shift_profile==PR_TARGET_SHIFT_DOSBOX386)c->flags|=0x10u;
        c->ip=0x2ADDu;
        break;
    case 0x2ADDu: /* mov byte ptr [0x6818], al */
        put(m,c->ds,(uint16_t)(0x6818u),8,(uint8_t)(c->ax));
        c->ip=0x2AE0u;
        break;
    case 0x2AE0u: /* mov sp, bp */
        c->sp=(uint16_t)(c->bp);
        c->ip=0x2AE2u;
        break;
    case 0x2AE2u: /* pop bp */
        c->bp=(uint16_t)(get(m,c->ss,c->sp,16));
        c->sp=(uint16_t)(c->sp+2u);
        c->ip=0x2AE3u;
        break;
    case 0x2AE3u: /* retf 0xa */
        at=phys(c->ss,c->sp);
        c->ip=word_at(m,at);
        c->cs=word_at(m,at+2u);
        c->sp=(uint16_t)(c->sp+14u);
        return PR_TARGET_RETURNED;
    default:return PR_TARGET_CODE;
    }
}
