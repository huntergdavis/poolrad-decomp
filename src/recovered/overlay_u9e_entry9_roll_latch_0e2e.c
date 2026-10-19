/* GAME.OVR U9E public entry 9, +0E2E..+0E51 (36 original bytes). */
#include "poolrad/recovered/overlay_u9e_entry9_roll_latch_0e2e.h"

#define RAM_MASK (PR_U9E_0E2E_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u /* OF SF ZF AF PF CF */

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & RAM_MASK;
}

static uint8_t read8(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return ram[physical(segment, offset)];
}

static uint16_t read16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    size_t a = physical(segment, offset);
    return (uint16_t)(ram[a] | ((uint16_t)ram[(a + 1u) & RAM_MASK] << 8));
}

static void write8(uint8_t *ram, uint16_t segment, uint16_t offset,
                   uint8_t value)
{
    ram[physical(segment, offset)] = value;
}

static void write16(uint8_t *ram, uint16_t segment, uint16_t offset,
                    uint16_t value)
{
    size_t a = physical(segment, offset);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & RAM_MASK] = (uint8_t)(value >> 8);
}

static void push16(uint8_t *ram, pr_u9e_0e2e_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static uint16_t pop16(const uint8_t *ram, pr_u9e_0e2e_cpu *cpu)
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

static void sub16_sp(pr_u9e_0e2e_cpu *cpu, uint16_t value)
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

pr_u9e_0e2e_status pr_u9e_entry9_roll_latch_0e2e(
    uint8_t *ram, size_t ram_size, pr_u9e_0e2e_cpu *cpu,
    pr_u9e_0e2e_roll roll, void *context)
{
    if (ram == NULL || cpu == NULL || roll == NULL ||
        ram_size < PR_U9E_0E2E_RAM_SIZE || cpu->ip != 0x0E2Eu)
        return PR_U9E_0E2E_ARGUMENT;
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    sub16_sp(cpu, 2u);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8u)));
    write8(ram, cpu->ds, 0x681Cu, (uint8_t)cpu->ax);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8u)));
    push16(ram, cpu, cpu->ax);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         read8(ram, cpu->ss, (uint16_t)(cpu->bp + 6u)));
    push16(ram, cpu, cpu->ax);
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, 0x0E46u);
    cpu->ip = 0x0DE3u;
    if (!roll(context, ram, ram_size, cpu)) return PR_U9E_0E2E_CALL_ABORT;
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 4u); /* entry 8 RETF 4 */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u), (uint8_t)cpu->ax);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u)));
    cpu->sp = cpu->bp;
    cpu->bp = pop16(ram, cpu);
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 4u);
    return PR_U9E_0E2E_OK;
}

/* Complete original instructions; docs/re/U38_LINK_ROLL_2B20_CPU.md. */
#include "poolrad/recovered/overlay_u9e_entry9_roll_latch_0e2e.h"
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

pr_roll_chain_status pr_u9e_roll_latch_0e2e_cpu(uint8_t *m,size_t size,pr_roll_chain_cpu *c,unsigned max_system_calls,pr_roll_chain_diagnostic *d)
{
    size_t at;
    uint16_t source_cs; int status;
    if(m==NULL||c==NULL||size!=PR_U9E_0E2E_RAM_SIZE||c->ip!=0x0E2Eu||c->sp<0x0020u||c->sp>0xFFFCu) return PR_ROLL_CHAIN_ARGUMENT;
    if(d!=NULL){d->latch_status=-1;d->dice_status=-1;}
    for(;;) switch(c->ip) {
    case 0x0E2Eu: /* push bp */
        push(m,c,c->bp);
        c->ip=0x0E2Fu;
        break;
    case 0x0E2Fu: /* mov bp, sp */
        c->bp=(uint16_t)(c->sp);
        c->ip=0x0E31u;
        break;
    case 0x0E31u: /* sub sp, 2 */
        c->sp=(uint16_t)(sub(c,c->sp,0x2u,16));
        c->ip=0x0E34u;
        break;
    case 0x0E34u: /* mov al, byte ptr [bp + 8] */
        c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(get(m,c->ss,(uint16_t)(c->bp + 0x8u),8)));
        c->ip=0x0E37u;
        break;
    case 0x0E37u: /* mov byte ptr [0x681c], al */
        put(m,c->ds,(uint16_t)(0x681Cu),8,(uint8_t)(c->ax));
        c->ip=0x0E3Au;
        break;
    case 0x0E3Au: /* mov al, byte ptr [bp + 8] */
        c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(get(m,c->ss,(uint16_t)(c->bp + 0x8u),8)));
        c->ip=0x0E3Du;
        break;
    case 0x0E3Du: /* push ax */
        push(m,c,c->ax);
        c->ip=0x0E3Eu;
        break;
    case 0x0E3Eu: /* mov al, byte ptr [bp + 6] */
        c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(get(m,c->ss,(uint16_t)(c->bp + 0x6u),8)));
        c->ip=0x0E41u;
        break;
    case 0x0E41u: /* push ax */
        push(m,c,c->ax);
        c->ip=0x0E42u;
        break;
    case 0x0E42u: /* push cs */
        push(m,c,c->cs);
        c->ip=0x0E43u;
        break;
    case 0x0E43u: /* call 0xde3 */
        if(get(m,c->cs,0x0E43u,8)!=0xE8u||get(m,c->cs,0x0E44u,16)!=0xFF9Du)return PR_ROLL_CHAIN_CODE;
        source_cs=c->cs;
        push(m,c,0x0E46u);
        c->ip=0x0DE3u;
        status=pr_u9e_entry8_fullstate_compose_0de3(m,size,c,max_system_calls);
        /* The legacy dice API retains undefined XOR AF. At LIMIT the last
         * FLAGS writer was XOR AH,AH at +0E0B. This raw entry selects the
         * measured Unicorn AF-clear profile for that observable stop. */
        if(status==PR_U9E_ENTRY8_COMPOSE_LIMIT)c->flags&=(uint16_t)~0x10u;
        if(d!=NULL)d->dice_status=status;
        if(status!=0)return PR_ROLL_CHAIN_CALLEE;
        if(c->cs!=source_cs||c->ip!=0x0E46u)return PR_ROLL_CHAIN_TRANSFER;
        break;
    case 0x0E46u: /* mov byte ptr [bp - 1], al */
        put(m,c->ss,(uint16_t)(c->bp - 0x1u),8,(uint8_t)(c->ax));
        c->ip=0x0E49u;
        break;
    case 0x0E49u: /* mov al, byte ptr [bp - 1] */
        c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(get(m,c->ss,(uint16_t)(c->bp - 0x1u),8)));
        c->ip=0x0E4Cu;
        break;
    case 0x0E4Cu: /* mov sp, bp */
        c->sp=(uint16_t)(c->bp);
        c->ip=0x0E4Eu;
        break;
    case 0x0E4Eu: /* pop bp */
        c->bp=(uint16_t)(get(m,c->ss,c->sp,16));
        c->sp=(uint16_t)(c->sp+2u);
        c->ip=0x0E4Fu;
        break;
    case 0x0E4Fu: /* retf 4 */
        at=phys(c->ss,c->sp);
        c->ip=word_at(m,at);
        c->cs=word_at(m,at+2u);
        c->sp=(uint16_t)(c->sp+8u);
        return PR_ROLL_CHAIN_RETURNED;
    default:return PR_ROLL_CHAIN_CODE;
    }
}
