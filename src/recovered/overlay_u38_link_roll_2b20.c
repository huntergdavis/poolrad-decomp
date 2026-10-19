/* GAME.OVR U38 public entry 101, +2B20..+2B63 (67 original bytes). */
#include "poolrad/recovered/overlay_u38_link_roll_2b20.h"

#define RAM_MASK (PR_U9E_0E2E_RAM_SIZE - 1u)
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

static void push16(uint8_t *ram, pr_u38_2b20_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static uint16_t pop16(const uint8_t *ram, pr_u38_2b20_cpu *cpu)
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

static void sub16_sp(pr_u38_2b20_cpu *cpu, uint16_t value)
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

static void logic16(pr_u38_2b20_cpu *cpu, uint16_t value)
{
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (parity((uint8_t)value)) cpu->flags |= 4u;
    if (value == 0u) cpu->flags |= 0x40u;
    if (value & 0x8000u) cpu->flags |= 0x80u;
}

static void compare8(pr_u38_2b20_cpu *cpu, uint8_t left, uint8_t right)
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

static void increment_ax(pr_u38_2b20_cpu *cpu)
{
    uint16_t before = cpu->ax;
    cpu->ax = (uint16_t)(before + 1u);
    cpu->flags &= (uint16_t)~(ARITH_FLAGS & ~1u);
    if (parity((uint8_t)cpu->ax)) cpu->flags |= 4u;
    if ((before & 0xFu) == 0xFu) cpu->flags |= 0x10u;
    if (cpu->ax == 0u) cpu->flags |= 0x40u;
    if (cpu->ax & 0x8000u) cpu->flags |= 0x80u;
    if (before == 0x7FFFu) cpu->flags |= 0x800u;
}

pr_u38_2b20_status pr_u38_link_roll_2b20(
    uint8_t *ram, size_t ram_size, pr_u38_2b20_cpu *cpu,
    pr_u38_2b20_roll_latch roll_latch, void *context)
{
    uint16_t far_off, far_seg;
    uint8_t kind;
    uint16_t caller_cs;
    if (ram == NULL || cpu == NULL || roll_latch == NULL ||
        ram_size < PR_U9E_0E2E_RAM_SIZE || cpu->ip != 0x2B20u)
        return PR_U38_2B20_ARGUMENT;
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    sub16_sp(cpu, 4u);
    cpu->di = read16(ram, cpu->ds, 0x5D92u);
    cpu->es = read16(ram, cpu->ds, 0x5D94u);
    far_off = read16(ram, cpu->es, (uint16_t)(cpu->di + 0xCCu));
    far_seg = read16(ram, cpu->es, (uint16_t)(cpu->di + 0xCEu));
    cpu->ax = far_off;
    cpu->dx = far_seg;
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u), cpu->ax);
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), cpu->dx);
    cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u));
    cpu->ax |= read16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u));
    logic16(cpu, cpu->ax);
    if (cpu->ax != 0u) {
        cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u));
        cpu->es = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u));
        kind = read8(ram, cpu->es, (uint16_t)(cpu->di + 0x2Eu));
        compare8(cpu, kind, 0x55u);
        if (kind == 0x55u) {
            cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | 1u);
            push16(ram, cpu, cpu->ax);
            cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | 6u);
            push16(ram, cpu, cpu->ax);
            caller_cs = cpu->cs;
            push16(ram, cpu, caller_cs);
            push16(ram, cpu, 0x2B57u);
            cpu->cs = read16(ram, caller_cs, 0x2B55u);
            cpu->ip = 0x004Du;
            if (!roll_latch(context, ram, ram_size, cpu) ||
                cpu->cs != caller_cs || cpu->ip != 0x2B57u)
                return PR_U38_2B20_CALL_ABORT;
            cpu->ax &= 0x00FFu; /* XOR AH,AH */
            logic16(cpu, 0u);
            increment_ax(cpu);
            write8(ram, cpu->ds, 0x6818u, (uint8_t)cpu->ax);
        }
    }
    cpu->sp = cpu->bp;
    cpu->bp = pop16(ram, cpu);
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 10u);
    return PR_U38_2B20_OK;
}

/* Complete original instructions; docs/re/U38_LINK_ROLL_2B20_CPU.md. */
#include "poolrad/recovered/overlay_u38_link_roll_2b20.h"
#define MASK (PR_U9E_0E2E_RAM_SIZE - 1u)
static size_t phys(uint16_t s,uint16_t o) {return (((size_t)s<<4)+o)&MASK;}
static uint16_t word_at(const uint8_t *m,size_t p) {return (uint16_t)(m[p&MASK]|((uint16_t)m[(p+1u)&MASK]<<8));}
static uint16_t get(const uint8_t *m,uint16_t s,uint16_t o,unsigned bits)
{size_t p=phys(s,o);return bits==8?m[p]:word_at(m,p);}
static void put(uint8_t *m,uint16_t s,uint16_t o,unsigned bits,uint16_t v)
{size_t p=phys(s,o);m[p]=(uint8_t)v;if(bits==16)m[(p+1u)&MASK]=(uint8_t)(v>>8);}
static void push(uint8_t *m,pr_roll_chain_cpu *c,uint16_t v)
{c->sp=(uint16_t)(c->sp-2u);put(m,c->ss,c->sp,16,v);}
static uint16_t logic(pr_roll_chain_cpu *c,uint16_t v,unsigned bits)
{
    unsigned p=v&255u;p^=p>>4;p^=p>>2;p^=p>>1;
    c->flags&=(uint16_t)~0x08D5u;
    if(!(p&1u))c->flags|=4u;
    if(!v)c->flags|=64u;
    if(v&(bits==8?128u:32768u))c->flags|=128u;
    return v;
}
static uint16_t sub(pr_roll_chain_cpu *c,uint16_t a,uint16_t b,unsigned bits)
{
    uint16_t r=(uint16_t)((a-b)&(bits==8?255u:65535u));
    (void)logic(c,r,bits);
    if(a<b)c->flags|=1u;
    if((a^b^r)&16u)c->flags|=16u;
    if((a^b)&(a^r)&(bits==8?128u:32768u))c->flags|=0x800u;
    return r;
}

pr_roll_chain_status pr_u38_link_roll_2b20_cpu(uint8_t *m,size_t size,pr_roll_chain_cpu *c,unsigned max_system_calls,pr_roll_chain_diagnostic *d)
{
    size_t at;
    uint16_t source_cs,segment; int status;
    if(m==NULL||c==NULL||size!=PR_U9E_0E2E_RAM_SIZE||c->ip!=0x2B20u||c->sp<0x0040u||c->sp>0xFFFCu) return PR_ROLL_CHAIN_ARGUMENT;
    if(d!=NULL){d->latch_status=-1;d->dice_status=-1;}
    for(;;) switch(c->ip) {
    case 0x2B20u: /* push bp */
        push(m,c,c->bp);
        c->ip=0x2B21u;
        break;
    case 0x2B21u: /* mov bp, sp */
        c->bp=(uint16_t)(c->sp);
        c->ip=0x2B23u;
        break;
    case 0x2B23u: /* sub sp, 4 */
        c->sp=(uint16_t)(sub(c,c->sp,0x4u,16));
        c->ip=0x2B26u;
        break;
    case 0x2B26u: /* les di, ptr [0x5d92] */
        at=phys(c->ds,(uint16_t)(0x5D92u));
        c->di=(uint16_t)(word_at(m,at));
        c->es=word_at(m,at+2u);
        c->ip=0x2B2Au;
        break;
    case 0x2B2Au: /* mov ax, word ptr es:[di + 0xcc] */
        c->ax=(uint16_t)(get(m,c->es,(uint16_t)(c->di + 0xCCu),16));
        c->ip=0x2B2Fu;
        break;
    case 0x2B2Fu: /* mov dx, word ptr es:[di + 0xce] */
        c->dx=(uint16_t)(get(m,c->es,(uint16_t)(c->di + 0xCEu),16));
        c->ip=0x2B34u;
        break;
    case 0x2B34u: /* mov word ptr [bp - 4], ax */
        put(m,c->ss,(uint16_t)(c->bp - 0x4u),16,c->ax);
        c->ip=0x2B37u;
        break;
    case 0x2B37u: /* mov word ptr [bp - 2], dx */
        put(m,c->ss,(uint16_t)(c->bp - 0x2u),16,c->dx);
        c->ip=0x2B3Au;
        break;
    case 0x2B3Au: /* mov ax, word ptr [bp - 4] */
        c->ax=(uint16_t)(get(m,c->ss,(uint16_t)(c->bp - 0x4u),16));
        c->ip=0x2B3Du;
        break;
    case 0x2B3Du: /* or ax, word ptr [bp - 2] */
        c->ax=(uint16_t)(logic(c,(uint16_t)(c->ax|get(m,c->ss,(uint16_t)(c->bp - 0x2u),16)),16));
        c->ip=0x2B40u;
        break;
    case 0x2B40u: /* je 0x2b5d */
        c->ip=(c->flags&64u)?0x2B5Du:0x2B42u;
        break;
    case 0x2B42u: /* les di, ptr [bp - 4] */
        at=phys(c->ss,(uint16_t)(c->bp - 0x4u));
        c->di=(uint16_t)(word_at(m,at));
        c->es=word_at(m,at+2u);
        c->ip=0x2B45u;
        break;
    case 0x2B45u: /* cmp byte ptr es:[di + 0x2e], 0x55 */
        (void)sub(c,get(m,c->es,(uint16_t)(c->di + 0x2Eu),8),0x55u,8);
        c->ip=0x2B4Au;
        break;
    case 0x2B4Au: /* jne 0x2b5d */
        c->ip=!(c->flags&64u)?0x2B5Du:0x2B4Cu;
        break;
    case 0x2B4Cu: /* mov al, 1 */
        c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(0x1u));
        c->ip=0x2B4Eu;
        break;
    case 0x2B4Eu: /* push ax */
        push(m,c,c->ax);
        c->ip=0x2B4Fu;
        break;
    case 0x2B4Fu: /* mov al, 6 */
        c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(0x6u));
        c->ip=0x2B51u;
        break;
    case 0x2B51u: /* push ax */
        push(m,c,c->ax);
        c->ip=0x2B52u;
        break;
    case 0x2B52u: /* lcall 0xb0, 0x4d */
        if(get(m,c->cs,0x2B52u,8)!=0x9Au||get(m,c->cs,0x2B53u,16)!=0x004Du)return PR_ROLL_CHAIN_CODE;
        segment=get(m,c->cs,0x2B55u,16);
        if(get(m,segment,0x004Du,8)!=0xEAu||get(m,segment,0x004Eu,16)!=0x0E2Eu)return PR_ROLL_CHAIN_CODE;
        source_cs=c->cs;
        push(m,c,source_cs);
        push(m,c,0x2B57u);
        c->ip=0x0E2Eu;
        c->cs=get(m,segment,0x0050u,16);
        status=pr_u9e_roll_latch_0e2e_cpu(m,size,c,max_system_calls,d);
        if(d!=NULL)d->latch_status=status;
        if(status!=0)return PR_ROLL_CHAIN_CALLEE;
        if(c->cs!=source_cs||c->ip!=0x2B57u)return PR_ROLL_CHAIN_TRANSFER;
        break;
    case 0x2B57u: /* xor ah, ah */
        c->ax=(uint16_t)((c->ax&255u)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax >> 8)^(uint8_t)(c->ax >> 8)),8))<<8));
        c->ip=0x2B59u;
        break;
    case 0x2B59u: /* inc ax */
        {uint16_t before=c->ax,carry=(uint16_t)(c->flags&1u);
        c->ax=logic(c,(uint16_t)(before+1u),16);c->flags|=carry;
        if((before&15u)==15u)c->flags|=16u;
        if(before==0x7FFFu)c->flags|=0x800u;}
        c->ip=0x2B5Au;
        break;
    case 0x2B5Au: /* mov byte ptr [0x6818], al */
        put(m,c->ds,(uint16_t)(0x6818u),8,(uint8_t)(c->ax));
        c->ip=0x2B5Du;
        break;
    case 0x2B5Du: /* mov sp, bp */
        c->sp=(uint16_t)(c->bp);
        c->ip=0x2B5Fu;
        break;
    case 0x2B5Fu: /* pop bp */
        c->bp=(uint16_t)(get(m,c->ss,c->sp,16));
        c->sp=(uint16_t)(c->sp+2u);
        c->ip=0x2B60u;
        break;
    case 0x2B60u: /* retf 0xa */
        at=phys(c->ss,c->sp);
        c->ip=word_at(m,at);
        c->cs=word_at(m,at+2u);
        c->sp=(uint16_t)(c->sp+14u);
        return PR_ROLL_CHAIN_RETURNED;
    default:return PR_ROLL_CHAIN_CODE;
    }
}
