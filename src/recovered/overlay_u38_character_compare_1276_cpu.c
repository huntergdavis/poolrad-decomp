/* Complete U38 [1276,12B2); raw SYSTEM StrCmp composed.
 * Memory/flags/composition reuse merged U38 +0679/+0947. */
#include "poolrad/recovered/overlay_u38_character_compare_1276_cpu.h"
#include "poolrad/recovered/system_string_move_leaves_cpu.h"

#define MASK (PR_U38_1276_RAM_SIZE - 1u)
#define STRCMP_IP 0x0C10u

#define COPY_CPU(to, from)                                                                                   \
    do {                                                                                                     \
        (to)->ax = (from)->ax; (to)->bx = (from)->bx; (to)->cx = (from)->cx; (to)->dx = (from)->dx;          \
        (to)->si = (from)->si; (to)->di = (from)->di; (to)->bp = (from)->bp; (to)->sp = (from)->sp;          \
        (to)->cs = (from)->cs; (to)->ip = (from)->ip; (to)->ds = (from)->ds; (to)->es = (from)->es;          \
        (to)->ss = (from)->ss; (to)->flags = (from)->flags;                                                  \
    } while (0)

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint16_t read16_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static uint8_t rd8(const uint8_t *ram, uint16_t segment, uint16_t offset) { return ram[physical(segment, offset)]; }
static uint16_t rd16(const uint8_t *ram, uint16_t segment, uint16_t offset) { return read16_at(ram, physical(segment, offset)); }

static void wr16(uint8_t *ram, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static void push16(uint8_t *ram, pr_u38_1276_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    wr16(ram, cpu->ss, cpu->sp, value);
}

/* LES DI,segment:[offset], the pair read physically contiguous. */
static void les(const uint8_t *ram, pr_u38_1276_cpu *cpu, uint16_t segment, uint16_t offset)
{
    size_t at = physical(segment, offset);
    cpu->di = read16_at(ram, at);
    cpu->es = read16_at(ram, (at + 2u) & MASK);
}

static int even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

static void szp(pr_u38_1276_cpu *cpu, uint16_t result, uint16_t sign)
{
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if ((result & (uint16_t)((sign << 1) - 1u)) == 0u) cpu->flags |= 0x0040u;
    if (result & sign) cpu->flags |= 0x0080u;
}

static void sub_flags(pr_u38_1276_cpu *cpu, uint16_t a, uint16_t b, uint16_t sign)
{
    uint16_t result = (uint16_t)((a - b) & (uint16_t)((sign << 1) - 1u));
    cpu->flags &= (uint16_t)~0x08D5u;
    if (a < b) cpu->flags |= 0x0001u;
    if ((a ^ b ^ result) & 0x0010u) cpu->flags |= 0x0010u;
    if ((a ^ b) & (a ^ result) & sign) cpu->flags |= 0x0800u;
    szp(cpu, result, sign);
}

/* LCALL 0AF8:0C10 at `site`: checks 9A and the offset, reads the segment from RAM, pushes CS and site + 5, and runs the
 * merged raw-CPU SYSTEM string compare. */
static pr_u38_1276_status compare(uint8_t *ram, size_t ram_size, pr_u38_1276_cpu *cpu, uint16_t site,
                                          int *callee_status)
{
    pr_system_leaf_cpu state;
    pr_system_leaf_status status;
    uint16_t caller_cs = cpu->cs, back = (uint16_t)(site + 5u);
    cpu->ip = site;
    if (rd8(ram, cpu->cs, site) != 0x9Au || rd16(ram, cpu->cs, (uint16_t)(site + 1u)) != STRCMP_IP)
        return PR_U38_1276_CODE;
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, back);
    cpu->cs = rd16(ram, caller_cs, (uint16_t)(site + 3u));
    cpu->ip = STRCMP_IP;
    COPY_CPU(&state, cpu);
    status = pr_system_strcmp_0c10(ram, ram_size, &state);
    COPY_CPU(cpu, &state);
    if (callee_status != NULL)
        *callee_status = (int)status;
    if (status != PR_SYSTEM_LEAF_RETURNED)
        return PR_U38_1276_CALLEE;
    if (cpu->ip != back || cpu->cs != caller_cs)
        return PR_U38_1276_TRANSFER;
    return PR_U38_1276_OK;
}

/* Reload the active far pointer before each comparison. Push the live
 * left pointer followed by CS:constant; the leaf consumes RETF8. */
static pr_u38_1276_status compare_active(uint8_t *ram,size_t size,
    pr_u38_1276_cpu *cpu,uint16_t constant,uint16_t site,int *callee_status)
{
    les(ram,cpu,cpu->ds,0x5D92u);
    push16(ram,cpu,cpu->es);push16(ram,cpu,cpu->di);
    cpu->di=constant;push16(ram,cpu,cpu->cs);push16(ram,cpu,cpu->di);
    return compare(ram,size,cpu,site,callee_status);
}

pr_u38_1276_status pr_u38_character_compare_1276_cpu(uint8_t *ram,size_t size,
    pr_u38_1276_cpu *cpu,int *callee_status)
{
    pr_u38_1276_status status;uint8_t byte;size_t frame;
    if(callee_status!=NULL)*callee_status=-1;
    if(ram==NULL || cpu==NULL || size!=PR_U38_1276_RAM_SIZE ||
       cpu->ip!=0x1276u || cpu->sp<0x0Eu || cpu->sp>0xFFF2u)
        return PR_U38_1276_ARGUMENT;
    push16(ram,cpu,cpu->bp);cpu->bp=cpu->sp;                    /* +1276 PUSH BP; MOV BP,SP */
    les(ram,cpu,cpu->ds,0x5D92u);
    byte=rd8(ram,cpu->es,(uint16_t)(cpu->di+0x9Fu));
    sub_flags(cpu,byte,1u,0x80u);                              /* +127D CMP marker,1 */
    if(byte==1u) {
        status=compare_active(ram,size,cpu,0x1268u,0x1290u,callee_status);
        if(status!=PR_U38_1276_OK)return status;
        if(!(cpu->flags&0x40u)) {                             /* +1295 JE +12A9 */
            status=compare_active(ram,size,cpu,0x1270u,0x12A2u,callee_status);
            if(status!=PR_U38_1276_OK)return status;
        }
        if(cpu->flags&0x40u) {                               /* +12A7 JNE +12AE */
            byte=rd8(ram,cpu->ds,0x6822u);
            sub_flags(cpu,byte,4u,0x80u);
            ram[physical(cpu->ds,0x6822u)]=(uint8_t)(byte-4u);
        }
    }
    cpu->bp=rd16(ram,cpu->ss,cpu->sp);cpu->sp=(uint16_t)(cpu->sp+2u); /* +12AE POP BP */
    frame=physical(cpu->ss,cpu->sp);cpu->ip=read16_at(ram,frame);
    cpu->cs=read16_at(ram,(frame+2u)&MASK);
    cpu->sp=(uint16_t)(cpu->sp+14u);return PR_U38_1276_OK;       /* +12AF RETF0Ah */
}
