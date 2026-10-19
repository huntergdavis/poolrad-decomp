/* GAME.OVR U38 near-called helper, +101D..+1085 (105 original bytes). */
#include "poolrad/recovered/overlay_u38_target_select_101d.h"

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

pr_u38_101d_status pr_u38_target_select_101d(
    uint8_t *ram, size_t ram_size, pr_u38_101d_cpu *cpu,
    pr_u38_101d_select select_target, void *context)
{
    uint16_t off, seg;
    if (ram == NULL || cpu == NULL || select_target == NULL ||
        ram_size < PR_U38_101D_RAM_SIZE || cpu->ip != 0x101Du)
        return PR_U38_101D_ARGUMENT;
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    sub16_sp(cpu, 10u);
    cpu->ax = 0u; /* XOR AX,AX */
    logic16(cpu, 0u);
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 8u), cpu->ax);
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 6u), cpu->ax);
    cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6u));
    cpu->es = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 8u));
    cpu->ax = read16(ram, cpu->es, (uint16_t)(cpu->di + 0xCCu));
    cpu->ax |= read16(ram, cpu->es, (uint16_t)(cpu->di + 0xCEu));
    logic16(cpu, cpu->ax);
    if (cpu->ax != 0u) {
        push16(ram, cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 8u)));
        push16(ram, cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6u)));
        cpu->di = (uint16_t)(cpu->bp - 8u);
        push16(ram, cpu, cpu->ss);
        push16(ram, cpu, cpu->di);
        push16(ram, cpu, cpu->cs);
        push16(ram, cpu, 0x104Au);
        cpu->cs = read16(ram, cpu->cs, 0x1048u);
        cpu->ip = 0x2E8Bu;
        if (!select_target(context, ram, ram_size, cpu))
            return PR_U38_101D_CALL_ABORT;
        cpu->ip = pop16(ram, cpu);
        cpu->cs = pop16(ram, cpu);
        cpu->sp = (uint16_t)(cpu->sp + 8u); /* BA:2E8B RETF 8 */
        write8(ram, cpu->ss, (uint16_t)(cpu->bp - 9u),
               (uint8_t)cpu->ax);
        compare8_zero(cpu, read8(ram, cpu->ss,
                                 (uint16_t)(cpu->bp - 9u)));
        if (read8(ram, cpu->ss, (uint16_t)(cpu->bp - 9u)) != 0u) {
            cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 8u));
            cpu->ax |= read16(ram, cpu->ss, (uint16_t)(cpu->bp - 6u));
            logic16(cpu, cpu->ax);
            if (cpu->ax != 0u) goto result;
        }
        cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6u));
        cpu->es = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 8u));
        off = read16(ram, cpu->es, (uint16_t)(cpu->di + 0xCCu));
        seg = read16(ram, cpu->es, (uint16_t)(cpu->di + 0xCEu));
        write16(ram, cpu->ss, (uint16_t)(cpu->bp - 8u), off);
        write16(ram, cpu->ss, (uint16_t)(cpu->bp - 6u), seg);
    }
result:
    cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 8u));
    cpu->dx = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 6u));
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u), cpu->ax);
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), cpu->dx);
    cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u));
    cpu->dx = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u));
    cpu->sp = cpu->bp;
    cpu->bp = pop16(ram, cpu);
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 4u);
    return PR_U38_101D_OK;
}

/* Complete original instructions; docs/re/U38_TARGET_SELECTOR_CPU.md. */

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

pr_target_status pr_u38_target_select_101d_cpu(uint8_t *m,size_t size,pr_target_cpu *c,pr_target_diagnostic *d)
{
    size_t at;
    uint16_t source_cs,segment; int status;
    if(m==NULL||c==NULL||size!=PR_TARGET_CPU_RAM_SIZE||c->ip!=0x101Du||c->sp<0x0020u||c->sp>0xFFF8u) return PR_TARGET_ARGUMENT;
    if(d!=NULL){d->selector_status=-1;d->resident_status=-1;}
    for(;;) switch(c->ip) {
    case 0x101Du: /* push bp */
        push(m,c,c->bp);
        c->ip=0x101Eu;
        break;
    case 0x101Eu: /* mov bp, sp */
        c->bp=(uint16_t)(c->sp);
        c->ip=0x1020u;
        break;
    case 0x1020u: /* sub sp, 0xa */
        c->sp=(uint16_t)(sub(c,c->sp,0xAu,16));
        c->ip=0x1023u;
        break;
    case 0x1023u: /* xor ax, ax */
        c->ax=(uint16_t)(logic(c,(uint16_t)(c->ax^c->ax),16));
        c->ip=0x1025u;
        break;
    case 0x1025u: /* mov word ptr [bp - 8], ax */
        put(m,c->ss,(uint16_t)(c->bp - 0x8u),16,c->ax);
        c->ip=0x1028u;
        break;
    case 0x1028u: /* mov word ptr [bp - 6], ax */
        put(m,c->ss,(uint16_t)(c->bp - 0x6u),16,c->ax);
        c->ip=0x102Bu;
        break;
    case 0x102Bu: /* les di, ptr [bp + 6] */
        at=phys(c->ss,(uint16_t)(c->bp + 0x6u));
        c->di=(uint16_t)(word_at(m,at));
        c->es=word_at(m,at+2u);
        c->ip=0x102Eu;
        break;
    case 0x102Eu: /* mov ax, word ptr es:[di + 0xcc] */
        c->ax=(uint16_t)(get(m,c->es,(uint16_t)(c->di + 0xCCu),16));
        c->ip=0x1033u;
        break;
    case 0x1033u: /* or ax, word ptr es:[di + 0xce] */
        c->ax=(uint16_t)(logic(c,(uint16_t)(c->ax|get(m,c->es,(uint16_t)(c->di + 0xCEu),16)),16));
        c->ip=0x1038u;
        break;
    case 0x1038u: /* je 0x106e */
        c->ip=(c->flags&64u)?0x106Eu:0x103Au;
        break;
    case 0x103Au: /* push word ptr [bp + 8] */
        push(m,c,get(m,c->ss,(uint16_t)(c->bp + 0x8u),16));
        c->ip=0x103Du;
        break;
    case 0x103Du: /* push word ptr [bp + 6] */
        push(m,c,get(m,c->ss,(uint16_t)(c->bp + 0x6u),16));
        c->ip=0x1040u;
        break;
    case 0x1040u: /* lea di, [bp - 8] */
        c->di=(uint16_t)((uint16_t)(c->bp - 0x8u));
        c->ip=0x1043u;
        break;
    case 0x1043u: /* push ss */
        push(m,c,c->ss);
        c->ip=0x1044u;
        break;
    case 0x1044u: /* push di */
        push(m,c,c->di);
        c->ip=0x1045u;
        break;
    case 0x1045u: /* lcall 0xba, 0x2e8b */
        if(get(m,c->cs,0x1045u,8)!=0x9Au||get(m,c->cs,0x1046u,16)!=0x2E8Bu)return PR_TARGET_CODE;
        source_cs=c->cs;
        segment=get(m,c->cs,0x1048u,16);
        push(m,c,source_cs);
        push(m,c,0x104Au);
        c->ip=0x2E8Bu;
        c->cs=segment;
        status=pr_res_target_select_2e8b_cpu(m,size,c);
        if(d!=NULL)d->resident_status=status;
        if(status!=PR_TARGET_RETURNED)return PR_TARGET_CALLEE;
        if(c->cs!=source_cs||c->ip!=0x104Au)return PR_TARGET_TRANSFER;
        break;
    case 0x104Au: /* mov byte ptr [bp - 9], al */
        put(m,c->ss,(uint16_t)(c->bp - 0x9u),8,(uint8_t)(c->ax));
        c->ip=0x104Du;
        break;
    case 0x104Du: /* cmp byte ptr [bp - 9], 0 */
        (void)sub(c,get(m,c->ss,(uint16_t)(c->bp - 0x9u),8),0x0u,8);
        c->ip=0x1051u;
        break;
    case 0x1051u: /* je 0x105b */
        c->ip=(c->flags&64u)?0x105Bu:0x1053u;
        break;
    case 0x1053u: /* mov ax, word ptr [bp - 8] */
        c->ax=(uint16_t)(get(m,c->ss,(uint16_t)(c->bp - 0x8u),16));
        c->ip=0x1056u;
        break;
    case 0x1056u: /* or ax, word ptr [bp - 6] */
        c->ax=(uint16_t)(logic(c,(uint16_t)(c->ax|get(m,c->ss,(uint16_t)(c->bp - 0x6u),16)),16));
        c->ip=0x1059u;
        break;
    case 0x1059u: /* jne 0x106e */
        c->ip=!(c->flags&64u)?0x106Eu:0x105Bu;
        break;
    case 0x105Bu: /* les di, ptr [bp + 6] */
        at=phys(c->ss,(uint16_t)(c->bp + 0x6u));
        c->di=(uint16_t)(word_at(m,at));
        c->es=word_at(m,at+2u);
        c->ip=0x105Eu;
        break;
    case 0x105Eu: /* mov ax, word ptr es:[di + 0xcc] */
        c->ax=(uint16_t)(get(m,c->es,(uint16_t)(c->di + 0xCCu),16));
        c->ip=0x1063u;
        break;
    case 0x1063u: /* mov dx, word ptr es:[di + 0xce] */
        c->dx=(uint16_t)(get(m,c->es,(uint16_t)(c->di + 0xCEu),16));
        c->ip=0x1068u;
        break;
    case 0x1068u: /* mov word ptr [bp - 8], ax */
        put(m,c->ss,(uint16_t)(c->bp - 0x8u),16,c->ax);
        c->ip=0x106Bu;
        break;
    case 0x106Bu: /* mov word ptr [bp - 6], dx */
        put(m,c->ss,(uint16_t)(c->bp - 0x6u),16,c->dx);
        c->ip=0x106Eu;
        break;
    case 0x106Eu: /* mov ax, word ptr [bp - 8] */
        c->ax=(uint16_t)(get(m,c->ss,(uint16_t)(c->bp - 0x8u),16));
        c->ip=0x1071u;
        break;
    case 0x1071u: /* mov dx, word ptr [bp - 6] */
        c->dx=(uint16_t)(get(m,c->ss,(uint16_t)(c->bp - 0x6u),16));
        c->ip=0x1074u;
        break;
    case 0x1074u: /* mov word ptr [bp - 4], ax */
        put(m,c->ss,(uint16_t)(c->bp - 0x4u),16,c->ax);
        c->ip=0x1077u;
        break;
    case 0x1077u: /* mov word ptr [bp - 2], dx */
        put(m,c->ss,(uint16_t)(c->bp - 0x2u),16,c->dx);
        c->ip=0x107Au;
        break;
    case 0x107Au: /* mov ax, word ptr [bp - 4] */
        c->ax=(uint16_t)(get(m,c->ss,(uint16_t)(c->bp - 0x4u),16));
        c->ip=0x107Du;
        break;
    case 0x107Du: /* mov dx, word ptr [bp - 2] */
        c->dx=(uint16_t)(get(m,c->ss,(uint16_t)(c->bp - 0x2u),16));
        c->ip=0x1080u;
        break;
    case 0x1080u: /* mov sp, bp */
        c->sp=(uint16_t)(c->bp);
        c->ip=0x1082u;
        break;
    case 0x1082u: /* pop bp */
        c->bp=(uint16_t)(get(m,c->ss,c->sp,16));
        c->sp=(uint16_t)(c->sp+2u);
        c->ip=0x1083u;
        break;
    case 0x1083u: /* retf 4 */
        at=phys(c->ss,c->sp);
        c->ip=word_at(m,at);
        c->cs=word_at(m,at+2u);
        c->sp=(uint16_t)(c->sp+8u);
        return PR_TARGET_RETURNED;
    default:return PR_TARGET_CODE;
    }
}
