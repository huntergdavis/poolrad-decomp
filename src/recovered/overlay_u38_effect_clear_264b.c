/* GAME.OVR U38 public entry 80, +264B..+268A, composed with +101D. */
#include "poolrad/recovered/overlay_u38_effect_clear_264b.h"

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

static void write16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
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

static void compare8(pr_u38_101d_cpu *cpu, uint8_t left, uint8_t right)
{
    uint8_t result = (uint8_t)(left - right);
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (left < right) cpu->flags |= 1u;
    if (parity(result)) cpu->flags |= 4u;
    if ((left ^ right ^ result) & 0x10u) cpu->flags |= 0x10u;
    if (result == 0u) cpu->flags |= 0x40u;
    if (result & 0x80u) cpu->flags |= 0x80u;
    if (((left ^ right) & (left ^ result) & 0x80u) != 0u)
        cpu->flags |= 0x800u;
}

pr_u38_264b_status pr_u38_effect_clear_264b(
    uint8_t *ram, size_t ram_size, pr_u38_101d_cpu *cpu,
    pr_u38_101d_select select_target, void *context)
{
    uint8_t byte32;
    if (ram == NULL || cpu == NULL || select_target == NULL ||
        ram_size < PR_U38_101D_RAM_SIZE || cpu->ip != 0x264Bu)
        return PR_U38_264B_ARGUMENT;
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 4u);
    push16(ram, cpu, read16(ram, cpu->ds, 0x5D94u));
    push16(ram, cpu, read16(ram, cpu->ds, 0x5D92u));
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, 0x265Du);
    cpu->ip = 0x101Du;
    if (pr_u38_target_select_101d(ram, ram_size, cpu,
                                    select_target, context) != PR_U38_101D_OK)
        return PR_U38_264B_CALL_ABORT;
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u), cpu->ax);
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), cpu->dx);
    cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u));
    cpu->ax |= read16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u));
    logic16(cpu, cpu->ax);
    if (cpu->ax == 0u) {
        write8(ram, cpu->ds, 0x6818u, 0u);
    } else {
        cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u));
        cpu->es = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u));
        byte32 = read8(ram, cpu->es, (uint16_t)(cpu->di + 0x32u));
        compare8(cpu, byte32, 0u);
        if (byte32 == 0u) {
            cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u));
            cpu->es = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u));
            if (read8(ram, cpu->es, (uint16_t)(cpu->di + 0x31u)) != 0xB1u) {
                compare8(cpu, read8(ram, cpu->es,
                                    (uint16_t)(cpu->di + 0x31u)), 0xB1u);
                write8(ram, cpu->ds, 0x6818u, 0u);
            } else {
                compare8(cpu, 0xB1u, 0xB1u);
            }
        }
    }
    cpu->sp = cpu->bp;
    cpu->bp = pop16(ram, cpu);
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 10u);
    return PR_U38_264B_OK;
}

/* Complete original instructions; docs/re/U38_EFFECT_CLEAR_264B_CPU.md. */
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

pr_target_status pr_u38_effect_clear_264b_cpu(uint8_t *m,size_t size,pr_target_cpu *c,pr_target_diagnostic *d)
{
    size_t at;
    uint16_t source_cs; int status;
    if(m==NULL||c==NULL||size!=PR_TARGET_CPU_RAM_SIZE||c->ip!=0x264Bu||c->sp<0x002Eu||c->sp>0xFFFCu) return PR_TARGET_ARGUMENT;
    if(d!=NULL){d->selector_status=-1;d->resident_status=-1;}
    for(;;) switch(c->ip) {
    case 0x264Bu: /* push bp */
        push(m,c,c->bp);
        c->ip=0x264Cu;
        break;
    case 0x264Cu: /* mov bp, sp */
        c->bp=(uint16_t)(c->sp);
        c->ip=0x264Eu;
        break;
    case 0x264Eu: /* sub sp, 4 */
        c->sp=(uint16_t)(sub(c,c->sp,0x4u,16));
        c->ip=0x2651u;
        break;
    case 0x2651u: /* push word ptr [0x5d94] */
        push(m,c,get(m,c->ds,(uint16_t)(0x5D94u),16));
        c->ip=0x2655u;
        break;
    case 0x2655u: /* push word ptr [0x5d92] */
        push(m,c,get(m,c->ds,(uint16_t)(0x5D92u),16));
        c->ip=0x2659u;
        break;
    case 0x2659u: /* push cs */
        push(m,c,c->cs);
        c->ip=0x265Au;
        break;
    case 0x265Au: /* call 0x101d */
        if(get(m,c->cs,0x265Au,8)!=0xE8u||get(m,c->cs,0x265Bu,16)!=0xE9C0u)return PR_TARGET_CODE;
        source_cs=c->cs;
        push(m,c,0x265Du);
        c->ip=0x101Du;
        status=pr_u38_target_select_101d_cpu(m,size,c,d);
        if(d!=NULL)d->selector_status=status;
        if(status!=PR_TARGET_RETURNED)return PR_TARGET_CALLEE;
        if(c->cs!=source_cs||c->ip!=0x265Du)return PR_TARGET_TRANSFER;
        break;
    case 0x265Du: /* mov word ptr [bp - 4], ax */
        put(m,c->ss,(uint16_t)(c->bp - 0x4u),16,c->ax);
        c->ip=0x2660u;
        break;
    case 0x2660u: /* mov word ptr [bp - 2], dx */
        put(m,c->ss,(uint16_t)(c->bp - 0x2u),16,c->dx);
        c->ip=0x2663u;
        break;
    case 0x2663u: /* mov ax, word ptr [bp - 4] */
        c->ax=(uint16_t)(get(m,c->ss,(uint16_t)(c->bp - 0x4u),16));
        c->ip=0x2666u;
        break;
    case 0x2666u: /* or ax, word ptr [bp - 2] */
        c->ax=(uint16_t)(logic(c,(uint16_t)(c->ax|get(m,c->ss,(uint16_t)(c->bp - 0x2u),16)),16));
        c->ip=0x2669u;
        break;
    case 0x2669u: /* je 0x267f */
        c->ip=(c->flags&64u)?0x267Fu:0x266Bu;
        break;
    case 0x266Bu: /* les di, ptr [bp - 4] */
        at=phys(c->ss,(uint16_t)(c->bp - 0x4u));
        c->di=(uint16_t)(word_at(m,at));
        c->es=word_at(m,at+2u);
        c->ip=0x266Eu;
        break;
    case 0x266Eu: /* cmp byte ptr es:[di + 0x32], 0 */
        (void)sub(c,get(m,c->es,(uint16_t)(c->di + 0x32u),8),0x0u,8);
        c->ip=0x2673u;
        break;
    case 0x2673u: /* jne 0x2684 */
        c->ip=!(c->flags&64u)?0x2684u:0x2675u;
        break;
    case 0x2675u: /* les di, ptr [bp - 4] */
        at=phys(c->ss,(uint16_t)(c->bp - 0x4u));
        c->di=(uint16_t)(word_at(m,at));
        c->es=word_at(m,at+2u);
        c->ip=0x2678u;
        break;
    case 0x2678u: /* cmp byte ptr es:[di + 0x31], 0xb1 */
        (void)sub(c,get(m,c->es,(uint16_t)(c->di + 0x31u),8),0xB1u,8);
        c->ip=0x267Du;
        break;
    case 0x267Du: /* je 0x2684 */
        c->ip=(c->flags&64u)?0x2684u:0x267Fu;
        break;
    case 0x267Fu: /* mov byte ptr [0x6818], 0 */
        put(m,c->ds,(uint16_t)(0x6818u),8,0x0u);
        c->ip=0x2684u;
        break;
    case 0x2684u: /* mov sp, bp */
        c->sp=(uint16_t)(c->bp);
        c->ip=0x2686u;
        break;
    case 0x2686u: /* pop bp */
        c->bp=(uint16_t)(get(m,c->ss,c->sp,16));
        c->sp=(uint16_t)(c->sp+2u);
        c->ip=0x2687u;
        break;
    case 0x2687u: /* retf 0xa */
        at=phys(c->ss,c->sp);
        c->ip=word_at(m,at);
        c->cs=word_at(m,at+2u);
        c->sp=(uint16_t)(c->sp+14u);
        return PR_TARGET_RETURNED;
    default:return PR_TARGET_CODE;
    }
}
