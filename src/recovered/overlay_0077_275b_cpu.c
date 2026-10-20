/* U77 [275B,27DF): 132-byte capacity check composing two credited residents. */
#include "poolrad/recovered/overlay_0077_275b_cpu.h"

#define MASK (PR_U9E_10E4_RAM_SIZE - 1u)
#define RESIDENT_CS 0x00BAu
#define SEGMENT_0BB8 0x0BB8u
#define SEGMENT_13AB 0x13ABu
#define CONSTANT_1500 0x05DCu
#define TYPE_LIMIT 0x0Fu
#define RESIDENT_BUDGET (PR_U9E_10E4_RAM_SIZE)

typedef struct { uint8_t *ram; pr_ovl0077_275b_state *cpu; } machine;

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint16_t word(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static uint16_t rd(machine *m, uint16_t segment, uint16_t offset)
{
    return word(m->ram, physical(segment, offset));
}

static void wr(machine *m, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t at = physical(segment, offset);
    m->ram[at] = (uint8_t)value;
    m->ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static void push(machine *m, uint16_t value)
{
    m->cpu->sp = (uint16_t)(m->cpu->sp - 2u);
    wr(m, m->cpu->ss, m->cpu->sp, value);
}

/* LES di,[bp+disp]: consecutive words through 16-bit effective-address wrap. */
static void les(machine *m, int displacement)
{
    size_t at = physical(m->cpu->ss, (uint16_t)(m->cpu->bp + displacement));
    m->cpu->di = word(m->ram, at);
    m->cpu->es = word(m->ram, (at + 2u) & MASK);
}

static unsigned parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0;
}

static void set_szp(pr_ovl0077_275b_state *c, uint16_t result, uint16_t sign)
{
    if (parity((uint8_t)result)) c->flags |= 4;
    if (result == 0) c->flags |= 0x40;
    if (sign && (result & sign)) c->flags |= 0x80;
}

static uint16_t add16(pr_ovl0077_275b_state *c, uint16_t a, uint16_t b)
{
    uint16_t result = (uint16_t)(a + b);
    c->flags &= (uint16_t)~0x8D5u;
    if ((uint32_t)a + b > 0xFFFFu) c->flags |= 1;
    if ((a ^ b ^ result) & 0x10) c->flags |= 0x10;
    if ((uint16_t)~(a ^ b) & (a ^ result) & 0x8000) c->flags |= 0x800;
    set_szp(c, result, 0x8000);
    return result;
}

static void cmp16(pr_ovl0077_275b_state *c, uint16_t a, uint16_t b)
{
    uint16_t result = (uint16_t)(a - b);
    c->flags &= (uint16_t)~0x8D5u;
    if (a < b) c->flags |= 1;
    if ((a ^ b ^ result) & 0x10) c->flags |= 0x10;
    if ((a ^ b) & (a ^ result) & 0x8000) c->flags |= 0x800;
    set_szp(c, result, 0x8000);
}

/* MUL r/m16: DX:AX = AX * operand, CF/OF when the high half is non-zero.
 * PF/AF/SF/ZF are architecturally undefined here; the original leaves them
 * from the preceding instruction, so they are preserved as the 8086 profile
 * measured from the native oracle. */
static void mul16(pr_ovl0077_275b_state *c, uint16_t operand)
{
    uint32_t product = (uint32_t)c->ax * operand;
    c->ax = (uint16_t)product;
    c->dx = (uint16_t)(product >> 16);
    c->flags &= (uint16_t)~0x801u;
    if (c->dx) c->flags |= 0x801;
}

pr_ovl0077_275b_status pr_ovl0077_275b_cpu(uint8_t *ram, size_t ram_size,
                                          pr_ovl0077_275b_state *c,
                                          size_t *budget,
                                          pr_ovl0077_275b_diagnostic *diag)
{
    machine m;
    uint16_t caller_segment, return_offset, far_offset, far_segment, saved, flag, total;
    uint8_t type_byte, count_byte;
    int status;

    if (ram == NULL || c == NULL || budget == NULL || ram_size != PR_U9E_10E4_RAM_SIZE)
        return PR_OVL0077_275B_ARGUMENT;
    if (c->ip != 0x275Bu || c->sp < 0x0080u || c->sp > 0xFFF8u)
        return PR_OVL0077_275B_ARGUMENT;
    if (diag != NULL) { diag->first_status = -1; diag->second_status = -1; diag->calls = 0u; }

    m.ram = ram;
    m.cpu = c;
    caller_segment = c->cs;

    /* Prologue: push bp ; mov bp,sp ; sub sp,4 */
    push(&m, c->bp);
    c->bp = c->sp;
    c->sp = (uint16_t)(c->sp - 4u);

    /* push [bp+0xC], push [bp+0xA], lcall 00BA:0BB8 */
    far_offset = rd(&m, c->ss, (uint16_t)(c->bp + 0x0A));
    far_segment = rd(&m, c->ss, (uint16_t)(c->bp + 0x0C));
    push(&m, far_segment);
    push(&m, far_offset);
    push(&m, caller_segment);
    return_offset = 0x276Cu;
    push(&m, return_offset);
    c->cs = RESIDENT_CS;
    c->ip = SEGMENT_0BB8;
    if (diag != NULL) diag->calls = 1u;
    *budget = RESIDENT_BUDGET;   /* fresh per resident call; LIMIT would be a CALLEE stop */
    status = (int)pr_res00ba_0bb8_cpu(ram, ram_size, c, budget);
    if (diag != NULL) diag->first_status = status;
    if (status != (int)PR_RES00BA_OK) return PR_OVL0077_275B_CALLEE;
    if (c->cs != caller_segment || c->ip != return_offset) return PR_OVL0077_275B_TRANSFER;

    /* [bp-2] = 0; es:di = [bp+0xA]; if es:[di+0xC7] > 15 then [bp-2] = 1 */
    flag = 0;
    wr(&m, c->ss, (uint16_t)(c->bp - 2), flag);
    les(&m, 0x0A);
    type_byte = ram[physical(c->es, (uint16_t)(c->di + 0xC7))];
    if (type_byte > TYPE_LIMIT) {
        flag = 1;
        wr(&m, c->ss, (uint16_t)(c->bp - 2), flag);
    }

    /* es:di = [bp+6]; [bp-4] = es:[di+0x37] */
    les(&m, 0x06);
    saved = rd(&m, c->es, (uint16_t)(c->di + 0x37));
    c->ax = saved;                     /* 2782: mov ax, word es:[di+0x37] */
    wr(&m, c->ss, (uint16_t)(c->bp - 4), saved);

    /* es:di = [bp+6]; if es:[di+0x39] > 0 then [bp-4] = byte * [bp-4] */
    les(&m, 0x06);
    count_byte = ram[physical(c->es, (uint16_t)(c->di + 0x39))];
    if (count_byte > 0) {
        c->ax = count_byte;
        c->ax = (uint16_t)(c->ax & 0x00FFu);          /* xor ah,ah */
        mul16(c, rd(&m, c->ss, (uint16_t)(c->bp - 4)));
        saved = c->ax;
        wr(&m, c->ss, (uint16_t)(c->bp - 4), saved);
    }

    /* push [bp+0xC], push [bp+0xA], lcall 00BA:13AB */
    push(&m, far_segment);
    push(&m, far_offset);
    push(&m, caller_segment);
    return_offset = 0x27ADu;
    push(&m, return_offset);
    c->cs = RESIDENT_CS;
    c->ip = SEGMENT_13AB;
    if (diag != NULL) diag->calls = 2u;
    *budget = RESIDENT_BUDGET;
    status = (int)pr_res00ba_helpers_cpu(ram, ram_size, c, budget);
    if (diag != NULL) diag->second_status = status;
    if (status != (int)PR_RES00BA_OK) return PR_OVL0077_275B_CALLEE;
    if (c->cs != caller_segment || c->ip != return_offset) return PR_OVL0077_275B_TRANSFER;

    /* add ax,1500 ; cdq ; cx=ax ; bx=dx */
    c->ax = add16(c, c->ax, CONSTANT_1500);
    c->dx = (c->ax & 0x8000u) ? 0xFFFFu : 0x0000u;      /* CDQ sign-extends AX */
    c->cx = c->ax;
    c->bx = c->dx;

    /* es:di = [bp+0xA]; ax = es:[di+0x102]; ax += [bp-4]; dx = 0 ; compare */
    les(&m, 0x0A);
    total = rd(&m, c->es, (uint16_t)(c->di + 0x102));
    total = add16(c, total, rd(&m, c->ss, (uint16_t)(c->bp - 4)));
    c->ax = total;
    c->dx = 0;
    cmp16(c, c->dx, c->bx);                             /* cmp dx,bx */
    if ((c->flags & 0x40) == 0) {                       /* not equal */
        int greater = (((c->flags & 0x800) != 0) == ((c->flags & 0x80) != 0));
        if (greater) {
            flag = 1;
            wr(&m, c->ss, (uint16_t)(c->bp - 2), flag);
        }
    } else {
        cmp16(c, c->ax, c->cx);                         /* cmp ax,cx */
        if ((c->flags & 1) == 0) {                      /* jbe not taken -> flag */
            flag = 1;
            wr(&m, c->ss, (uint16_t)(c->bp - 2), flag);
        }
    }

    /* al = [bp-2]; [bp-1] = al; al = [bp-1]; sp=bp; pop bp; retf 8 */
    flag = ram[physical(c->ss, (uint16_t)(c->bp - 2))];
    wr(&m, c->ss, (uint16_t)(c->bp - 1), flag);
    c->ax = (uint16_t)((c->ax & 0xFF00u) | ram[physical(c->ss, (uint16_t)(c->bp - 1))]);
    c->sp = c->bp;
    c->bp = rd(&m, c->ss, c->sp);
    c->sp = (uint16_t)(c->sp + 2u);
    {
        size_t at = physical(c->ss, c->sp);
        c->ip = word(ram, at);
        c->cs = word(ram, (at + 2u) & MASK);
    }
    c->sp = (uint16_t)(c->sp + 4u + 8u);   /* pop IP+CS, then drop the 8 argument bytes */
    return PR_OVL0077_275B_RETURNED;
}
