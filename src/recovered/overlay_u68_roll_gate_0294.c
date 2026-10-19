/* Bounded C model of GAME.OVR unit 0x68 +0x0294; see
 * docs/re/U68_ROLL_GATE_0294.md. */
#include "poolrad/recovered/overlay_u68_roll_gate_0294.h"

#include <string.h>

static int null_far(pr_u68_far p)
{
    return (p.offset | p.segment) == 0u;
}

static int signed_byte(uint8_t v)
{
    return v < 0x80u ? (int)v : (int)v - 256;
}

typedef struct {
    unsigned sides;     /* the second pushed byte: 6 or 8; 0 = no roll */
    unsigned limit;     /* found when roll <= limit (unsigned compare on AL) */
    int equal;          /* the original compares `cmp al, 1; jne` (== limit) instead of `ja` (<= limit) */
    int clear_flag;     /* [0x6DDE] := 0 */
    int found_no_roll;  /* found without a roll */
} rule;

static const rule none = { 0, 0, 0, 0, 0 };

/* entry 3 result 3 */
static rule rule_hit(uint8_t type, uint8_t value)
{
    rule r = none;
    if (type == 0x12u) {                               /* +0x02E3..+0x0328 */
        if (value >= 0x5Bu && value <= 0x63u) { r.sides = 6; r.limit = 1; r.equal = 1; }
        else if (value == 0x64u)               { r.sides = 6; r.limit = 2; }
        else r.clear_flag = 1;                         /* +0x0328 */
    } else if (type == 0x13u || type == 0x14u) { r.sides = 6; r.limit = 3; }   /* +0x032F..+0x034A */
    else if (type == 0x15u || type == 0x16u)   { r.sides = 6; r.limit = 4; }   /* +0x034C..+0x0367 */
    else if (type == 0x17u)                    { r.sides = 6; r.limit = 5; }   /* +0x0369..+0x0380 */
    else if (type == 0x18u)                    { r.sides = 8; r.limit = 7; }   /* +0x0382..+0x0399 */
    else if (type == 0x19u)                    r.found_no_roll = 1;            /* +0x039B..+0x03A3 */
    else r.clear_flag = 1;                             /* +0x03A5 */
    return r;
}

/* entry 3 result other than 3 */
static rule rule_miss(uint8_t type, uint8_t value)
{
    rule r = none;
    if (type >= 3u && type <= 7u)        { r.sides = 6; r.limit = 1; r.equal = 1; }         /* +0x03B4..+0x03CF */
    else if (type >= 8u && type <= 0xFu) { r.sides = 6; r.limit = 2; }         /* +0x03D2..+0x03ED */
    else if (type == 0x10u || type == 0x11u) { r.sides = 6; r.limit = 3; }     /* +0x03EF..+0x040A */
    else if (type == 0x12u) {                                                  /* +0x040C..+0x0464 */
        if (value <= 0x32u)       { r.sides = 6; r.limit = 3; }
        else if (value <= 0x63u)  { r.sides = 6; r.limit = 4; }
        else if (value == 0x64u)  { r.sides = 6; r.limit = 5; }
    }
    return r;
}

pr_u68_roll_gate_status pr_u68_roll_gate(pr_u68_roll_gate_state *st, uint8_t *al_out,
                                         pr_u68_roll_gate_report *report)
{
    pr_u68_roll_gate_report local;
    pr_u68_roll_gate_report *r = report ? report : &local;
    pr_u68_far cur;
    size_t seen = 0, i;
    uint8_t found = 0;                                  /* [bp-6] */
    memset(r, 0, sizeof *r);
    if (!st || !al_out)
        return PR_U68_ROLL_ARGUMENT;
    cur = st->head;                                     /* +0x029E..+0x02A8 */

    /* +0x02AB..+0x02BC: nil or found ends the loop */
    while (!null_far(cur) && !found) {
        const pr_u68_segment_view *view = NULL;
        uint8_t e3 = 0, type, value;
        rule ru;
        uint16_t next_at, next_seg_at;

        if (!st->views || !st->view_count || !st->visited || !st->visited_capacity)
            return PR_U68_ROLL_ARGUMENT;
        for (i = 0; i < seen; i++)
            if (st->visited[i].offset == cur.offset && st->visited[i].segment == cur.segment)
                return PR_U68_ROLL_CYCLE;
        if (seen == st->visited_capacity)
            return PR_U68_ROLL_CAPACITY;
        st->visited[seen++] = cur;
        for (i = 0; i < st->view_count; i++)
            if (st->views[i].selector == cur.segment) { view = &st->views[i]; break; }
        if (!view || !view->bytes || view->size < PR_U68_ROLL_SEGMENT_SIZE)
            return PR_U68_ROLL_ARGUMENT;
        r->nodes++;

        /* +0x02C8..+0x02D4: entry 3 (x, y, facing) */
        if (pr_u3bd_entry3(&st->field, st->party_x, st->party_y, st->facing, &e3, NULL) != PR_U3BD_FIELD_OK)
            return PR_U68_ROLL_ARGUMENT;
        if (r->nodes == 1u)
            r->first_entry3 = e3;
        type = view->bytes[(uint16_t)(cur.offset + PR_U68_ROLL_TYPE_OFFSET)];      /* +0x02E3 / +0x03B0 */
        /* byte +0x16 is read only on the type 0x12 branches (+0x02EB, +0x0410), never for another type */
        value = (type == 0x12u) ? view->bytes[(uint16_t)(cur.offset + PR_U68_ROLL_VALUE_OFFSET)] : 0u;
        ru = (e3 == 3u) ? rule_hit(type, value) : rule_miss(type, value);

        if (ru.clear_flag) {
            st->ds_6dde = 0;
            r->cleared_6dde++;
        }
        if (ru.found_no_roll) {
            found = 1;
        } else if (ru.sides) {
            uint8_t roll;
            if (!st->hooks.roll_9e_entry8)
                return PR_U68_ROLL_HOOK;
            /* set AL = 1, push AX; set AL = sides, push AX; lcall 0xB0:0x48; the compare is unsigned on AL */
            roll = st->hooks.roll_9e_entry8(st->hooks.ctx, (uint8_t)ru.sides, 1u);
            r->rolls++;
            if (ru.equal ? roll == ru.limit : roll <= ru.limit)
                found = 1;
        }

        /* +0x0468..+0x0478: advance, also after a hit */
        next_at = (uint16_t)(cur.offset + PR_U68_ROLL_NEXT_OFFSET);
        next_seg_at = (uint16_t)(cur.offset + PR_U68_ROLL_NEXT_OFFSET + 2u);
        if (next_at == 0xFFFFu || next_seg_at == 0xFFFFu)
            return PR_U68_ROLL_ARGUMENT;      /* host views do not represent a word at FFFF */
        cur.offset = (uint16_t)(view->bytes[next_at] | ((uint16_t)view->bytes[(uint16_t)(next_at + 1u)] << 8));
        cur.segment = (uint16_t)(view->bytes[next_seg_at] | ((uint16_t)view->bytes[(uint16_t)(next_seg_at + 1u)] << 8));
    }

    /* +0x047E..+0x04D2: found -> +0x0109 twice */
    if (found) {
        uint8_t x2, y2, sel2;
        if (!st->hooks.mark_0109)
            return PR_U68_ROLL_HOOK;
        st->hooks.mark_0109(st->hooks.ctx, st->party_x, st->party_y, st->facing);     /* +0x0484..+0x0491 */
        r->marks++;
        if (!st->direction_table || st->direction_table_size < (size_t)st->facing + 10u)
            return PR_U68_ROLL_ARGUMENT;
        x2 = (uint8_t)(signed_byte(st->party_x) + signed_byte(st->direction_table[st->facing]));         /* +0x0494..+0x04A8 */
        y2 = (uint8_t)(signed_byte(st->party_y) + signed_byte(st->direction_table[(size_t)st->facing + 9u]));  /* +0x04A9..+0x04BD */
        sel2 = (uint8_t)(((unsigned)st->facing + 4u) % 8u);                                                /* +0x04BE..+0x04CD */
        st->hooks.mark_0109(st->hooks.ctx, x2, y2, sel2);                                                  /* +0x04CE..+0x04CF */
        r->marks++;
    }
    *al_out = found;                                    /* +0x04D2..+0x04DE */
    return PR_U68_ROLL_OK;
}

/* Raw upgrade: exactly U68 [0294,04DF), 587B / 247 instructions.
 * RETF at04DE is the last byte; no Pascal strings or padding are included.
 * The fixed C branches below preserve the original operand/flag ordering. */
#include "poolrad/recovered/overlay_u3bd_lookup_fullstate.h"
#include "poolrad/recovered/overlay_u9e_entry8_fullstate_compose_0de3.h"

#define MASK (PR_U68_0109_RAM_SIZE - 1u)

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

static void push16(uint8_t *ram, pr_u68_roll_gate_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    wr16(ram, cpu->ss, cpu->sp, value);
}

static int even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

static void szp(pr_u68_roll_gate_cpu *cpu, uint16_t result, uint16_t sign)
{
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if ((result & (uint16_t)((sign << 1) - 1u)) == 0u) cpu->flags |= 0x0040u;
    if (result & sign) cpu->flags |= 0x0080u;
}

static void sub_flags(pr_u68_roll_gate_cpu *cpu, uint16_t a, uint16_t b, uint16_t sign)
{
    uint16_t result = (uint16_t)((a - b) & (uint16_t)((sign << 1) - 1u));
    cpu->flags &= (uint16_t)~0x08D5u;
    if (a < b) cpu->flags |= 0x0001u;
    if ((a ^ b ^ result) & 0x0010u) cpu->flags |= 0x0010u;
    if ((a ^ b) & (a ^ result) & sign) cpu->flags |= 0x0800u;
    szp(cpu, result, sign);
}

static uint16_t add16(pr_u68_roll_gate_cpu *cpu, uint16_t a, uint16_t b)
{
    uint32_t wide = (uint32_t)a + b;
    uint16_t result = (uint16_t)wide;
    cpu->flags &= (uint16_t)~0x08D5u;
    if (wide > 0xFFFFu) cpu->flags |= 0x0001u;
    if ((a ^ b ^ result) & 0x0010u) cpu->flags |= 0x0010u;
    if (~(a ^ b) & (a ^ result) & 0x8000u) cpu->flags |= 0x0800u;
    szp(cpu, result, 0x8000u);
    return result;
}

static void logic_flags(pr_u68_roll_gate_cpu *cpu, uint16_t result, uint16_t sign)
{
    cpu->flags &= (uint16_t)~0x08D5u;
    szp(cpu, result, sign);
}

static void set_al(pr_u68_roll_gate_cpu *cpu, uint8_t value) { cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value); }
static void cbw(pr_u68_roll_gate_cpu *cpu) { cpu->ax = (uint16_t)(int16_t)(int8_t)(uint8_t)cpu->ax; }


static pr_u68_roll_gate_cpu_status raw_call(
    uint8_t *ram, size_t size, pr_u68_roll_gate_cpu *cpu,
    uint16_t site, unsigned max_system_calls, pr_u68_roll_gate_cpu_report *r)
{
    uint16_t caller = cpu->cs, back, stub, entry, target;
    int status;
    cpu->ip = site;
    r->callee = site == 0x02D4u ? PR_U68_ROLL_CPU_FIELD :
                (site == 0x0491u || site == 0x04CFu) ? PR_U68_ROLL_CPU_MAP : PR_U68_ROLL_CPU_DICE;
    r->status = -1;
    if (r->callee == PR_U68_ROLL_CPU_MAP) {
        back = (uint16_t)(site + 3u);
        if (rd8(ram, caller, site) != 0xE8u ||
            (uint16_t)(back + rd16(ram, caller, (uint16_t)(site + 1u))) != 0x0109u)
            return PR_U68_ROLL_CPU_CODE;
        /* PUSH CS has already executed at 0490/04CE. */
        push16(ram, cpu, back);
        cpu->ip = 0x0109u;
        status = (int)pr_u68_map_update_0109_cpu(ram, size, cpu);
    } else {
        uint16_t offset = r->callee == PR_U68_ROLL_CPU_FIELD ? 0x002Fu : 0x0048u;
        entry = r->callee == PR_U68_ROLL_CPU_FIELD ? 0x0350u : 0x0DE3u;
        back = (uint16_t)(site + 5u);
        if (rd8(ram, caller, site) != 0x9Au ||
            rd16(ram, caller, (uint16_t)(site + 1u)) != offset)
            return PR_U68_ROLL_CPU_CODE;
        stub = rd16(ram, caller, (uint16_t)(site + 3u));
        if (rd8(ram, stub, offset) != 0xEAu || rd16(ram, stub, (uint16_t)(offset + 1u)) != entry)
            return PR_U68_ROLL_CPU_CODE;
        target = rd16(ram, stub, (uint16_t)(offset + 3u));
        push16(ram, cpu, caller);
        push16(ram, cpu, back);
        cpu->cs = target;
        cpu->ip = entry;
        if (r->callee == PR_U68_ROLL_CPU_FIELD) {
            pr_u3bd_lookup_cpu field;
            COPY_CPU(&field, cpu);
            status = (int)pr_u3bd_field_0350_fullstate(ram, size, &field);
            COPY_CPU(cpu, &field);
        } else {
            pr_u9e_entry8_compose_cpu dice;
            COPY_CPU(&dice, cpu);
            status = (int)pr_u9e_entry8_fullstate_compose_0de3(ram, size, &dice, max_system_calls);
            COPY_CPU(cpu, &dice);
            /* The shared prefix leaves AF unspecified. At this measured
             * XOR boundary Unicorn uses AF=0; scope the adaptation
             * to LIMIT and leave the shared component untouched. */
            if (status == PR_U9E_ENTRY8_COMPOSE_LIMIT)
                cpu->flags &= (uint16_t)~0x0010u;
        }
    }
    r->status = status;
    if (status != 0) return PR_U68_ROLL_CPU_CALLEE;
    if (cpu->cs != caller || cpu->ip != back) return PR_U68_ROLL_CPU_TRANSFER;
    return PR_U68_ROLL_CPU_OK;
}

pr_u68_roll_gate_cpu_status pr_u68_roll_gate_0294_cpu(
    uint8_t *ram, size_t ram_size, pr_u68_roll_gate_cpu *cpu,
    unsigned max_loopbacks, unsigned max_system_calls,
    pr_u68_roll_gate_cpu_report *report)
{
    pr_u68_roll_gate_cpu_report local;
    pr_u68_roll_gate_cpu_report *r = report ? report : &local;
    pr_u68_roll_gate_cpu_status status;
    unsigned loopbacks = 0;
    uint16_t temp;
    size_t at;
    r->callee = PR_U68_ROLL_CPU_NONE;
    r->status = -1;
    if (!ram || !cpu || ram_size != PR_U68_0109_RAM_SIZE ||
        cpu->ip != 0x0294u || cpu->sp < 0x0040u || cpu->sp > 0xFFFCu)
        return PR_U68_ROLL_CPU_ARGUMENT;
    /* 0294 */ push16(ram, cpu, cpu->bp);
    /* 0295 */ cpu->bp = cpu->sp;
    /* 0297 */ sub_flags(cpu, cpu->sp, 0xau, 0x8000u); cpu->sp = (uint16_t)(cpu->sp - 0xau);
    /* 029A */ ram[physical(cpu->ss, (uint16_t)(cpu->bp - 6))] = (uint8_t)(0u);
    /* 029E */ cpu->ax = rd16(ram, cpu->ds, (uint16_t)(0x5d96));
    /* 02A1 */ cpu->dx = rd16(ram, cpu->ds, (uint16_t)(0x5d98));
    /* 02A5 */ wr16(ram, cpu->ss, (uint16_t)(cpu->bp - 5), cpu->ax);
    /* 02A8 */ wr16(ram, cpu->ss, (uint16_t)(cpu->bp - 3), cpu->dx);
L02AB:
    /* 02AB */ cpu->ax = rd16(ram, cpu->ss, (uint16_t)(cpu->bp - 5));
    /* 02AE */ cpu->ax = (uint16_t)(cpu->ax | rd16(ram, cpu->ss, (uint16_t)(cpu->bp - 3))); logic_flags(cpu, cpu->ax, 0x8000u);
    /* 02B1 */ if (!(cpu->flags & 0x0040u)) goto L02B6;
    /* 02B3 */ goto L047E;
L02B6:
    /* 02B6 */ sub_flags(cpu, rd8(ram, cpu->ss, (uint16_t)(cpu->bp - 6)), 0u, 0x80u);
    /* 02BA */ if (cpu->flags & 0x0040u) goto L02BF;
    /* 02BC */ goto L047E;
L02BF:
    /* 02BF */ at = physical(cpu->ss, (uint16_t)(cpu->bp - 5)); cpu->di = read16_at(ram, at); cpu->es = read16_at(ram, (at + 2u) & MASK);
    /* 02C2 */ wr16(ram, cpu->ss, (uint16_t)(cpu->bp - 0xa), cpu->di);
    /* 02C5 */ wr16(ram, cpu->ss, (uint16_t)(cpu->bp - 8), cpu->es);
    /* 02C8 */ set_al(cpu, rd8(ram, cpu->ds, (uint16_t)(0x6aad)));
    /* 02CB */ push16(ram, cpu, cpu->ax);
    /* 02CC */ set_al(cpu, rd8(ram, cpu->ds, (uint16_t)(0x6aae)));
    /* 02CF */ push16(ram, cpu, cpu->ax);
    /* 02D0 */ set_al(cpu, rd8(ram, cpu->ds, (uint16_t)(0x6aaf)));
    /* 02D3 */ push16(ram, cpu, cpu->ax);
    /* 02D4 */ status = raw_call(ram, ram_size, cpu, 0x02D4u, max_system_calls, r);
    if (status != PR_U68_ROLL_CPU_OK) return status;
    /* 02D9 */ sub_flags(cpu, (uint8_t)cpu->ax, 3u, 0x80u);
    /* 02DB */ if (cpu->flags & 0x0040u) goto L02E0;
    /* 02DD */ goto L03AD;
L02E0:
    /* 02E0 */ at = physical(cpu->ss, (uint16_t)(cpu->bp - 0xa)); cpu->di = read16_at(ram, at); cpu->es = read16_at(ram, (at + 2u) & MASK);
    /* 02E3 */ set_al(cpu, rd8(ram, cpu->es, (uint16_t)(cpu->di + 0x10)));
    /* 02E7 */ sub_flags(cpu, (uint8_t)cpu->ax, 0x12u, 0x80u);
    /* 02E9 */ if (!(cpu->flags & 0x0040u)) goto L032F;
    /* 02EB */ at = physical(cpu->ss, (uint16_t)(cpu->bp - 0xa)); cpu->di = read16_at(ram, at); cpu->es = read16_at(ram, (at + 2u) & MASK);
    /* 02EE */ set_al(cpu, rd8(ram, cpu->es, (uint16_t)(cpu->di + 0x16)));
    /* 02F2 */ sub_flags(cpu, (uint8_t)cpu->ax, 0x5bu, 0x80u);
    /* 02F4 */ if (cpu->flags & 0x0001u) goto L030F;
    /* 02F6 */ sub_flags(cpu, (uint8_t)cpu->ax, 0x63u, 0x80u);
    /* 02F8 */ if (!(cpu->flags & 0x0041u)) goto L030F;
    /* 02FA */ set_al(cpu, 1u);
    /* 02FC */ push16(ram, cpu, cpu->ax);
    /* 02FD */ set_al(cpu, 6u);
    /* 02FF */ push16(ram, cpu, cpu->ax);
    /* 0300 */ status = raw_call(ram, ram_size, cpu, 0x0300u, max_system_calls, r);
    if (status != PR_U68_ROLL_CPU_OK) return status;
    /* 0305 */ sub_flags(cpu, (uint8_t)cpu->ax, 1u, 0x80u);
    /* 0307 */ if (!(cpu->flags & 0x0040u)) goto L030D;
    /* 0309 */ ram[physical(cpu->ss, (uint16_t)(cpu->bp - 6))] = (uint8_t)(1u);
L030D:
    /* 030D */ goto L032D;
L030F:
    /* 030F */ sub_flags(cpu, (uint8_t)cpu->ax, 0x64u, 0x80u);
    /* 0311 */ if (!(cpu->flags & 0x0040u)) goto L0328;
    /* 0313 */ set_al(cpu, 1u);
    /* 0315 */ push16(ram, cpu, cpu->ax);
    /* 0316 */ set_al(cpu, 6u);
    /* 0318 */ push16(ram, cpu, cpu->ax);
    /* 0319 */ status = raw_call(ram, ram_size, cpu, 0x0319u, max_system_calls, r);
    if (status != PR_U68_ROLL_CPU_OK) return status;
    /* 031E */ sub_flags(cpu, (uint8_t)cpu->ax, 2u, 0x80u);
    /* 0320 */ if (!(cpu->flags & 0x0041u)) goto L0326;
    /* 0322 */ ram[physical(cpu->ss, (uint16_t)(cpu->bp - 6))] = (uint8_t)(1u);
L0326:
    /* 0326 */ goto L032D;
L0328:
    /* 0328 */ ram[physical(cpu->ds, (uint16_t)(0x6dde))] = (uint8_t)(0u);
L032D:
    /* 032D */ goto L03AA;
L032F:
    /* 032F */ sub_flags(cpu, (uint8_t)cpu->ax, 0x13u, 0x80u);
    /* 0331 */ if (cpu->flags & 0x0040u) goto L0337;
    /* 0333 */ sub_flags(cpu, (uint8_t)cpu->ax, 0x14u, 0x80u);
    /* 0335 */ if (!(cpu->flags & 0x0040u)) goto L034C;
L0337:
    /* 0337 */ set_al(cpu, 1u);
    /* 0339 */ push16(ram, cpu, cpu->ax);
    /* 033A */ set_al(cpu, 6u);
    /* 033C */ push16(ram, cpu, cpu->ax);
    /* 033D */ status = raw_call(ram, ram_size, cpu, 0x033Du, max_system_calls, r);
    if (status != PR_U68_ROLL_CPU_OK) return status;
    /* 0342 */ sub_flags(cpu, (uint8_t)cpu->ax, 3u, 0x80u);
    /* 0344 */ if (!(cpu->flags & 0x0041u)) goto L034A;
    /* 0346 */ ram[physical(cpu->ss, (uint16_t)(cpu->bp - 6))] = (uint8_t)(1u);
L034A:
    /* 034A */ goto L03AA;
L034C:
    /* 034C */ sub_flags(cpu, (uint8_t)cpu->ax, 0x15u, 0x80u);
    /* 034E */ if (cpu->flags & 0x0040u) goto L0354;
    /* 0350 */ sub_flags(cpu, (uint8_t)cpu->ax, 0x16u, 0x80u);
    /* 0352 */ if (!(cpu->flags & 0x0040u)) goto L0369;
L0354:
    /* 0354 */ set_al(cpu, 1u);
    /* 0356 */ push16(ram, cpu, cpu->ax);
    /* 0357 */ set_al(cpu, 6u);
    /* 0359 */ push16(ram, cpu, cpu->ax);
    /* 035A */ status = raw_call(ram, ram_size, cpu, 0x035Au, max_system_calls, r);
    if (status != PR_U68_ROLL_CPU_OK) return status;
    /* 035F */ sub_flags(cpu, (uint8_t)cpu->ax, 4u, 0x80u);
    /* 0361 */ if (!(cpu->flags & 0x0041u)) goto L0367;
    /* 0363 */ ram[physical(cpu->ss, (uint16_t)(cpu->bp - 6))] = (uint8_t)(1u);
L0367:
    /* 0367 */ goto L03AA;
L0369:
    /* 0369 */ sub_flags(cpu, (uint8_t)cpu->ax, 0x17u, 0x80u);
    /* 036B */ if (!(cpu->flags & 0x0040u)) goto L0382;
    /* 036D */ set_al(cpu, 1u);
    /* 036F */ push16(ram, cpu, cpu->ax);
    /* 0370 */ set_al(cpu, 6u);
    /* 0372 */ push16(ram, cpu, cpu->ax);
    /* 0373 */ status = raw_call(ram, ram_size, cpu, 0x0373u, max_system_calls, r);
    if (status != PR_U68_ROLL_CPU_OK) return status;
    /* 0378 */ sub_flags(cpu, (uint8_t)cpu->ax, 5u, 0x80u);
    /* 037A */ if (!(cpu->flags & 0x0041u)) goto L0380;
    /* 037C */ ram[physical(cpu->ss, (uint16_t)(cpu->bp - 6))] = (uint8_t)(1u);
L0380:
    /* 0380 */ goto L03AA;
L0382:
    /* 0382 */ sub_flags(cpu, (uint8_t)cpu->ax, 0x18u, 0x80u);
    /* 0384 */ if (!(cpu->flags & 0x0040u)) goto L039B;
    /* 0386 */ set_al(cpu, 1u);
    /* 0388 */ push16(ram, cpu, cpu->ax);
    /* 0389 */ set_al(cpu, 8u);
    /* 038B */ push16(ram, cpu, cpu->ax);
    /* 038C */ status = raw_call(ram, ram_size, cpu, 0x038Cu, max_system_calls, r);
    if (status != PR_U68_ROLL_CPU_OK) return status;
    /* 0391 */ sub_flags(cpu, (uint8_t)cpu->ax, 7u, 0x80u);
    /* 0393 */ if (!(cpu->flags & 0x0041u)) goto L0399;
    /* 0395 */ ram[physical(cpu->ss, (uint16_t)(cpu->bp - 6))] = (uint8_t)(1u);
L0399:
    /* 0399 */ goto L03AA;
L039B:
    /* 039B */ sub_flags(cpu, (uint8_t)cpu->ax, 0x19u, 0x80u);
    /* 039D */ if (!(cpu->flags & 0x0040u)) goto L03A5;
    /* 039F */ ram[physical(cpu->ss, (uint16_t)(cpu->bp - 6))] = (uint8_t)(1u);
    /* 03A3 */ goto L03AA;
L03A5:
    /* 03A5 */ ram[physical(cpu->ds, (uint16_t)(0x6dde))] = (uint8_t)(0u);
L03AA:
    /* 03AA */ goto L0468;
L03AD:
    /* 03AD */ at = physical(cpu->ss, (uint16_t)(cpu->bp - 0xa)); cpu->di = read16_at(ram, at); cpu->es = read16_at(ram, (at + 2u) & MASK);
    /* 03B0 */ set_al(cpu, rd8(ram, cpu->es, (uint16_t)(cpu->di + 0x10)));
    /* 03B4 */ sub_flags(cpu, (uint8_t)cpu->ax, 3u, 0x80u);
    /* 03B6 */ if (cpu->flags & 0x0001u) goto L03D2;
    /* 03B8 */ sub_flags(cpu, (uint8_t)cpu->ax, 7u, 0x80u);
    /* 03BA */ if (!(cpu->flags & 0x0041u)) goto L03D2;
    /* 03BC */ set_al(cpu, 1u);
    /* 03BE */ push16(ram, cpu, cpu->ax);
    /* 03BF */ set_al(cpu, 6u);
    /* 03C1 */ push16(ram, cpu, cpu->ax);
    /* 03C2 */ status = raw_call(ram, ram_size, cpu, 0x03C2u, max_system_calls, r);
    if (status != PR_U68_ROLL_CPU_OK) return status;
    /* 03C7 */ sub_flags(cpu, (uint8_t)cpu->ax, 1u, 0x80u);
    /* 03C9 */ if (!(cpu->flags & 0x0040u)) goto L03CF;
    /* 03CB */ ram[physical(cpu->ss, (uint16_t)(cpu->bp - 6))] = (uint8_t)(1u);
L03CF:
    /* 03CF */ goto L0468;
L03D2:
    /* 03D2 */ sub_flags(cpu, (uint8_t)cpu->ax, 8u, 0x80u);
    /* 03D4 */ if (cpu->flags & 0x0001u) goto L03EF;
    /* 03D6 */ sub_flags(cpu, (uint8_t)cpu->ax, 0xfu, 0x80u);
    /* 03D8 */ if (!(cpu->flags & 0x0041u)) goto L03EF;
    /* 03DA */ set_al(cpu, 1u);
    /* 03DC */ push16(ram, cpu, cpu->ax);
    /* 03DD */ set_al(cpu, 6u);
    /* 03DF */ push16(ram, cpu, cpu->ax);
    /* 03E0 */ status = raw_call(ram, ram_size, cpu, 0x03E0u, max_system_calls, r);
    if (status != PR_U68_ROLL_CPU_OK) return status;
    /* 03E5 */ sub_flags(cpu, (uint8_t)cpu->ax, 2u, 0x80u);
    /* 03E7 */ if (!(cpu->flags & 0x0041u)) goto L03ED;
    /* 03E9 */ ram[physical(cpu->ss, (uint16_t)(cpu->bp - 6))] = (uint8_t)(1u);
L03ED:
    /* 03ED */ goto L0468;
L03EF:
    /* 03EF */ sub_flags(cpu, (uint8_t)cpu->ax, 0x10u, 0x80u);
    /* 03F1 */ if (cpu->flags & 0x0001u) goto L040C;
    /* 03F3 */ sub_flags(cpu, (uint8_t)cpu->ax, 0x11u, 0x80u);
    /* 03F5 */ if (!(cpu->flags & 0x0041u)) goto L040C;
    /* 03F7 */ set_al(cpu, 1u);
    /* 03F9 */ push16(ram, cpu, cpu->ax);
    /* 03FA */ set_al(cpu, 6u);
    /* 03FC */ push16(ram, cpu, cpu->ax);
    /* 03FD */ status = raw_call(ram, ram_size, cpu, 0x03FDu, max_system_calls, r);
    if (status != PR_U68_ROLL_CPU_OK) return status;
    /* 0402 */ sub_flags(cpu, (uint8_t)cpu->ax, 3u, 0x80u);
    /* 0404 */ if (!(cpu->flags & 0x0041u)) goto L040A;
    /* 0406 */ ram[physical(cpu->ss, (uint16_t)(cpu->bp - 6))] = (uint8_t)(1u);
L040A:
    /* 040A */ goto L0468;
L040C:
    /* 040C */ sub_flags(cpu, (uint8_t)cpu->ax, 0x12u, 0x80u);
    /* 040E */ if (!(cpu->flags & 0x0040u)) goto L0468;
    /* 0410 */ at = physical(cpu->ss, (uint16_t)(cpu->bp - 0xa)); cpu->di = read16_at(ram, at); cpu->es = read16_at(ram, (at + 2u) & MASK);
    /* 0413 */ set_al(cpu, rd8(ram, cpu->es, (uint16_t)(cpu->di + 0x16)));
    /* 0417 */ sub_flags(cpu, (uint8_t)cpu->ax, 0u, 0x80u);
    /* 0419 */ if (cpu->flags & 0x0001u) goto L0434;
    /* 041B */ sub_flags(cpu, (uint8_t)cpu->ax, 0x32u, 0x80u);
    /* 041D */ if (!(cpu->flags & 0x0041u)) goto L0434;
    /* 041F */ set_al(cpu, 1u);
    /* 0421 */ push16(ram, cpu, cpu->ax);
    /* 0422 */ set_al(cpu, 6u);
    /* 0424 */ push16(ram, cpu, cpu->ax);
    /* 0425 */ status = raw_call(ram, ram_size, cpu, 0x0425u, max_system_calls, r);
    if (status != PR_U68_ROLL_CPU_OK) return status;
    /* 042A */ sub_flags(cpu, (uint8_t)cpu->ax, 3u, 0x80u);
    /* 042C */ if (!(cpu->flags & 0x0041u)) goto L0432;
    /* 042E */ ram[physical(cpu->ss, (uint16_t)(cpu->bp - 6))] = (uint8_t)(1u);
L0432:
    /* 0432 */ goto L0468;
L0434:
    /* 0434 */ sub_flags(cpu, (uint8_t)cpu->ax, 0x33u, 0x80u);
    /* 0436 */ if (cpu->flags & 0x0001u) goto L0451;
    /* 0438 */ sub_flags(cpu, (uint8_t)cpu->ax, 0x63u, 0x80u);
    /* 043A */ if (!(cpu->flags & 0x0041u)) goto L0451;
    /* 043C */ set_al(cpu, 1u);
    /* 043E */ push16(ram, cpu, cpu->ax);
    /* 043F */ set_al(cpu, 6u);
    /* 0441 */ push16(ram, cpu, cpu->ax);
    /* 0442 */ status = raw_call(ram, ram_size, cpu, 0x0442u, max_system_calls, r);
    if (status != PR_U68_ROLL_CPU_OK) return status;
    /* 0447 */ sub_flags(cpu, (uint8_t)cpu->ax, 4u, 0x80u);
    /* 0449 */ if (!(cpu->flags & 0x0041u)) goto L044F;
    /* 044B */ ram[physical(cpu->ss, (uint16_t)(cpu->bp - 6))] = (uint8_t)(1u);
L044F:
    /* 044F */ goto L0468;
L0451:
    /* 0451 */ sub_flags(cpu, (uint8_t)cpu->ax, 0x64u, 0x80u);
    /* 0453 */ if (!(cpu->flags & 0x0040u)) goto L0468;
    /* 0455 */ set_al(cpu, 1u);
    /* 0457 */ push16(ram, cpu, cpu->ax);
    /* 0458 */ set_al(cpu, 6u);
    /* 045A */ push16(ram, cpu, cpu->ax);
    /* 045B */ status = raw_call(ram, ram_size, cpu, 0x045Bu, max_system_calls, r);
    if (status != PR_U68_ROLL_CPU_OK) return status;
    /* 0460 */ sub_flags(cpu, (uint8_t)cpu->ax, 5u, 0x80u);
    /* 0462 */ if (!(cpu->flags & 0x0041u)) goto L0468;
    /* 0464 */ ram[physical(cpu->ss, (uint16_t)(cpu->bp - 6))] = (uint8_t)(1u);
L0468:
    /* 0468 */ at = physical(cpu->ss, (uint16_t)(cpu->bp - 5)); cpu->di = read16_at(ram, at); cpu->es = read16_at(ram, (at + 2u) & MASK);
    /* 046B */ cpu->ax = rd16(ram, cpu->es, (uint16_t)(cpu->di + 0x104));
    /* 0470 */ cpu->dx = rd16(ram, cpu->es, (uint16_t)(cpu->di + 0x106));
    /* 0475 */ wr16(ram, cpu->ss, (uint16_t)(cpu->bp - 5), cpu->ax);
    /* 0478 */ wr16(ram, cpu->ss, (uint16_t)(cpu->bp - 3), cpu->dx);
    /* 047B */ if (loopbacks == max_loopbacks) { cpu->ip = 0x02ABu; return PR_U68_ROLL_CPU_LIMIT; }
    ++loopbacks; goto L02AB;
L047E:
    /* 047E */ sub_flags(cpu, rd8(ram, cpu->ss, (uint16_t)(cpu->bp - 6)), 0u, 0x80u);
    /* 0482 */ if (cpu->flags & 0x0040u) goto L04D2;
    /* 0484 */ set_al(cpu, rd8(ram, cpu->ds, (uint16_t)(0x6aad)));
    /* 0487 */ push16(ram, cpu, cpu->ax);
    /* 0488 */ set_al(cpu, rd8(ram, cpu->ds, (uint16_t)(0x6aae)));
    /* 048B */ push16(ram, cpu, cpu->ax);
    /* 048C */ set_al(cpu, rd8(ram, cpu->ds, (uint16_t)(0x6aaf)));
    /* 048F */ push16(ram, cpu, cpu->ax);
    /* 0490 */ push16(ram, cpu, cpu->cs);
    /* 0491 */ status = raw_call(ram, ram_size, cpu, 0x0491u, max_system_calls, r);
    if (status != PR_U68_ROLL_CPU_OK) return status;
    /* 0494 */ set_al(cpu, rd8(ram, cpu->ds, (uint16_t)(0x6aaf)));
    /* 0497 */ cpu->ax &= 0x00FFu; logic_flags(cpu, 0u, 0x80u);
    /* 0499 */ cpu->di = cpu->ax;
    /* 049B */ set_al(cpu, rd8(ram, cpu->ds, (uint16_t)(cpu->di + 0x2736)));
    /* 049F */ cbw(cpu);
    /* 04A0 */ cpu->dx = cpu->ax;
    /* 04A2 */ set_al(cpu, rd8(ram, cpu->ds, (uint16_t)(0x6aad)));
    /* 04A5 */ cbw(cpu);
    /* 04A6 */ cpu->ax = add16(cpu, cpu->ax, cpu->dx);
    /* 04A8 */ push16(ram, cpu, cpu->ax);
    /* 04A9 */ set_al(cpu, rd8(ram, cpu->ds, (uint16_t)(0x6aaf)));
    /* 04AC */ cpu->ax &= 0x00FFu; logic_flags(cpu, 0u, 0x80u);
    /* 04AE */ cpu->di = cpu->ax;
    /* 04B0 */ set_al(cpu, rd8(ram, cpu->ds, (uint16_t)(cpu->di + 0x273f)));
    /* 04B4 */ cbw(cpu);
    /* 04B5 */ cpu->dx = cpu->ax;
    /* 04B7 */ set_al(cpu, rd8(ram, cpu->ds, (uint16_t)(0x6aae)));
    /* 04BA */ cbw(cpu);
    /* 04BB */ cpu->ax = add16(cpu, cpu->ax, cpu->dx);
    /* 04BD */ push16(ram, cpu, cpu->ax);
    /* 04BE */ set_al(cpu, rd8(ram, cpu->ds, (uint16_t)(0x6aaf)));
    /* 04C1 */ cpu->ax &= 0x00FFu; logic_flags(cpu, 0u, 0x80u);
    /* 04C3 */ cpu->ax = add16(cpu, cpu->ax, 4u);
    /* 04C6 */ cpu->dx = (cpu->ax & 0x8000u) ? 0xFFFFu : 0u;
    /* 04C7 */ cpu->cx = 8u;
    /* 04CA */ temp = cpu->ax; cpu->ax = (uint16_t)((int16_t)temp / 8); cpu->dx = (uint16_t)((int16_t)temp % 8);
    /* 04CC */ temp = cpu->dx; cpu->dx = cpu->ax; cpu->ax = temp;
    /* 04CD */ push16(ram, cpu, cpu->ax);
    /* 04CE */ push16(ram, cpu, cpu->cs);
    /* 04CF */ status = raw_call(ram, ram_size, cpu, 0x04CFu, max_system_calls, r);
    if (status != PR_U68_ROLL_CPU_OK) return status;
L04D2:
    /* 04D2 */ set_al(cpu, rd8(ram, cpu->ss, (uint16_t)(cpu->bp - 6)));
    /* 04D5 */ ram[physical(cpu->ss, (uint16_t)(cpu->bp - 1))] = (uint8_t)((uint8_t)cpu->ax);
    /* 04D8 */ set_al(cpu, rd8(ram, cpu->ss, (uint16_t)(cpu->bp - 1)));
    /* 04DB */ cpu->sp = cpu->bp;
    /* 04DD */ cpu->bp = rd16(ram, cpu->ss, cpu->sp); cpu->sp = (uint16_t)(cpu->sp + 2u);
    /* 04DE */ at = physical(cpu->ss, cpu->sp); cpu->ip = read16_at(ram, at); cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u); return PR_U68_ROLL_CPU_OK;
}
