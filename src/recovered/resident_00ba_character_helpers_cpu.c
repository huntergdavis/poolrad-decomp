/* Six RES00BA character arithmetic helpers, +10E6..+148B.
 * Literal instruction operations preserve live stack aliases and each budget
 * boundary. Code is immutable by the public contract; no opcode decoding.
 */
#include "poolrad/recovered/resident_00ba_character_0bb8_cpu.h"

#define MASK 0xFFFFFu
#define CF 0x0001u
#define PF 0x0004u
#define AF 0x0010u
#define ZF 0x0040u
#define SF 0x0080u
#define OF 0x0800u
#define ARITH (CF | PF | AF | ZF | SF | OF)

static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & MASK;
}
static uint16_t word_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}
static uint16_t word(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    return word_at(ram, physical(seg, off));
}
static void put_word(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t at = physical(seg, off);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}
static void push(uint8_t *ram, pr_res00ba_cpu *c, uint16_t value)
{
    c->sp = (uint16_t)(c->sp - 2u);
    put_word(ram, c->ss, c->sp, value);
}
static uint16_t pop(const uint8_t *ram, pr_res00ba_cpu *c)
{
    uint16_t value = word(ram, c->ss, c->sp);
    c->sp = (uint16_t)(c->sp + 2u);
    return value;
}
static unsigned even(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}
static void flags(pr_res00ba_cpu *c, unsigned f)
{
    c->flags = (uint16_t)((c->flags & ~ARITH) | f);
}
static uint16_t arithmetic(pr_res00ba_cpu *c, uint16_t a, uint16_t b,
                           unsigned bits, unsigned subtract)
{
    uint32_t wide = subtract ? (uint32_t)a - b : (uint32_t)a + b;
    unsigned mask = bits == 8u ? 0xFFu : 0xFFFFu;
    unsigned sign = bits == 8u ? 0x80u : 0x8000u;
    uint16_t result = (uint16_t)(wide & mask);
    unsigned f = 0u;
    if (subtract ? a < b : wide > mask) f |= CF;
    if (even((uint8_t)result)) f |= PF;
    if ((a ^ b ^ result) & 0x10u) f |= AF;
    if (result == 0u) f |= ZF;
    if (result & sign) f |= SF;
    if ((subtract ? (a ^ b) : ~(a ^ b)) & (a ^ result) & sign) f |= OF;
    flags(c, f);
    return result;
}
static void al(pr_res00ba_cpu *c, uint8_t value)
{
    c->ax = (uint16_t)((c->ax & 0xFF00u) | value);
}
static void load_pointer(const uint8_t *ram, pr_res00ba_cpu *c)
{
    size_t at = physical(c->ss, (uint16_t)(c->bp + 6u));
    c->di = word_at(ram, at);
    c->es = word_at(ram, (at + 2u) & MASK);
}
/* Same pinned DOSBox-X normal/386 MULW profile as U86 +07D6.
 * SF/AF survive; ZF tests AX; PF is parity of the full 32-bit product.
 * These undefined bits are observable only at an instruction-budget stop:
 * the following ADD replaces every arithmetic flag on a completed path.
 */
static void multiply(pr_res00ba_cpu *c)
{
    uint32_t product = (uint32_t)c->ax * c->dx;
    unsigned f = c->flags & (SF | AF);
    c->ax = (uint16_t)product;
    c->dx = (uint16_t)(product >> 16);
    if (even((uint8_t)(product ^ (product >> 8) ^ (product >> 16) ^ (product >> 24)))) f |= PF;
    if (c->ax == 0u) f |= ZF;
    if (c->dx != 0u) f |= CF | OF;
    flags(c, f);
}
static pr_res00ba_status execute(uint8_t *ram, pr_res00ba_cpu *c, size_t *budget)
{
    for (;;) {
        if (*budget == 0u) return PR_RES00BA_LIMIT;
        --*budget;
        switch (c->ip) {
        case 0x10E6u: push(ram, c, c->bp); c->ip = 0x10E7u; break;
        case 0x10E7u: c->bp = c->sp; c->ip = 0x10E9u; break;
        case 0x10E9u: c->sp = arithmetic(c, c->sp, 2u, 16u, 1u); c->ip = 0x10ECu; break;
        case 0x10ECu: load_pointer(ram, c); c->ip = 0x10EFu; break;
        case 0x10EFu: al(c, (uint8_t)(ram[physical(c->es, (uint16_t)(c->di + 0x13u))])); c->ip = 0x10F3u; break;
        case 0x10F3u: ram[physical(c->ss, (uint16_t)(c->bp - 2u))] = (uint8_t)((uint8_t)c->ax); c->ip = 0x10F6u; break;
        case 0x10F6u: al(c, (uint8_t)(ram[physical(c->ss, (uint16_t)(c->bp - 2u))])); c->ip = 0x10F9u; break;
        case 0x10F9u: (void)arithmetic(c, (uint8_t)c->ax, 1u, 8u, 1u); c->ip = 0x10FBu; break;
        case 0x10FBu: c->ip = (c->flags & CF) != 0u ? 0x1107u : 0x10FDu; break;
        case 0x10FDu: (void)arithmetic(c, (uint8_t)c->ax, 3u, 8u, 1u); c->ip = 0x10FFu; break;
        case 0x10FFu: c->ip = (c->flags & (CF | ZF)) == 0u ? 0x1107u : 0x1101u; break;
        case 0x1101u: ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = (uint8_t)(0xfcu); c->ip = 0x1105u; break;
        case 0x1105u: c->ip = 0x115fu; break;
        case 0x1107u: (void)arithmetic(c, (uint8_t)c->ax, 4u, 8u, 1u); c->ip = 0x1109u; break;
        case 0x1109u: c->ip = (c->flags & CF) != 0u ? 0x111cu : 0x110Bu; break;
        case 0x110Bu: (void)arithmetic(c, (uint8_t)c->ax, 6u, 8u, 1u); c->ip = 0x110Du; break;
        case 0x110Du: c->ip = (c->flags & (CF | ZF)) == 0u ? 0x111cu : 0x110Fu; break;
        case 0x110Fu: al(c, (uint8_t)(ram[physical(c->ss, (uint16_t)(c->bp - 2u))])); c->ip = 0x1112u; break;
        case 0x1112u: c->ax &= 0x00FFu; flags(c, ZF | PF); c->ip = 0x1114u; break;
        case 0x1114u: c->ax = arithmetic(c, c->ax, 7u, 16u, 1u); c->ip = 0x1117u; break;
        case 0x1117u: ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = (uint8_t)((uint8_t)c->ax); c->ip = 0x111Au; break;
        case 0x111Au: c->ip = 0x115fu; break;
        case 0x111Cu: (void)arithmetic(c, (uint8_t)c->ax, 0xfu, 8u, 1u); c->ip = 0x111Eu; break;
        case 0x111Eu: c->ip = (c->flags & CF) != 0u ? 0x1131u : 0x1120u; break;
        case 0x1120u: (void)arithmetic(c, (uint8_t)c->ax, 0x12u, 8u, 1u); c->ip = 0x1122u; break;
        case 0x1122u: c->ip = (c->flags & (CF | ZF)) == 0u ? 0x1131u : 0x1124u; break;
        case 0x1124u: al(c, (uint8_t)(ram[physical(c->ss, (uint16_t)(c->bp - 2u))])); c->ip = 0x1127u; break;
        case 0x1127u: c->ax &= 0x00FFu; flags(c, ZF | PF); c->ip = 0x1129u; break;
        case 0x1129u: c->ax = arithmetic(c, c->ax, 0xeu, 16u, 1u); c->ip = 0x112Cu; break;
        case 0x112Cu: ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = (uint8_t)((uint8_t)c->ax); c->ip = 0x112Fu; break;
        case 0x112Fu: c->ip = 0x115fu; break;
        case 0x1131u: (void)arithmetic(c, (uint8_t)c->ax, 0x13u, 8u, 1u); c->ip = 0x1133u; break;
        case 0x1133u: c->ip = (c->flags & CF) != 0u ? 0x113fu : 0x1135u; break;
        case 0x1135u: (void)arithmetic(c, (uint8_t)c->ax, 0x14u, 8u, 1u); c->ip = 0x1137u; break;
        case 0x1137u: c->ip = (c->flags & (CF | ZF)) == 0u ? 0x113fu : 0x1139u; break;
        case 0x1139u: ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = (uint8_t)(4u); c->ip = 0x113Du; break;
        case 0x113Du: c->ip = 0x115fu; break;
        case 0x113Fu: (void)arithmetic(c, (uint8_t)c->ax, 0x15u, 8u, 1u); c->ip = 0x1141u; break;
        case 0x1141u: c->ip = (c->flags & CF) != 0u ? 0x114du : 0x1143u; break;
        case 0x1143u: (void)arithmetic(c, (uint8_t)c->ax, 0x17u, 8u, 1u); c->ip = 0x1145u; break;
        case 0x1145u: c->ip = (c->flags & (CF | ZF)) == 0u ? 0x114du : 0x1147u; break;
        case 0x1147u: ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = (uint8_t)(5u); c->ip = 0x114Bu; break;
        case 0x114Bu: c->ip = 0x115fu; break;
        case 0x114Du: (void)arithmetic(c, (uint8_t)c->ax, 0x18u, 8u, 1u); c->ip = 0x114Fu; break;
        case 0x114Fu: c->ip = (c->flags & CF) != 0u ? 0x115bu : 0x1151u; break;
        case 0x1151u: (void)arithmetic(c, (uint8_t)c->ax, 0x19u, 8u, 1u); c->ip = 0x1153u; break;
        case 0x1153u: c->ip = (c->flags & (CF | ZF)) == 0u ? 0x115bu : 0x1155u; break;
        case 0x1155u: ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = (uint8_t)(6u); c->ip = 0x1159u; break;
        case 0x1159u: c->ip = 0x115fu; break;
        case 0x115Bu: ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = (uint8_t)(0u); c->ip = 0x115Fu; break;
        case 0x115Fu: al(c, (uint8_t)(ram[physical(c->ss, (uint16_t)(c->bp - 1u))])); c->ip = 0x1162u; break;
        case 0x1162u: c->sp = c->bp; c->ip = 0x1164u; break;
        case 0x1164u: c->bp = pop(ram, c); c->ip = 0x1165u; break;
        case 0x1165u: c->ip = pop(ram, c); c->cs = pop(ram, c); c->sp = (uint16_t)(c->sp + 4u); return PR_RES00BA_OK;
        case 0x1168u: push(ram, c, c->bp); c->ip = 0x1169u; break;
        case 0x1169u: c->bp = c->sp; c->ip = 0x116Bu; break;
        case 0x116Bu: c->sp = arithmetic(c, c->sp, 2u, 16u, 1u); c->ip = 0x116Eu; break;
        case 0x116Eu: load_pointer(ram, c); c->ip = 0x1171u; break;
        case 0x1171u: al(c, (uint8_t)(ram[physical(c->es, (uint16_t)(c->di + 0x13u))])); c->ip = 0x1175u; break;
        case 0x1175u: ram[physical(c->ss, (uint16_t)(c->bp - 2u))] = (uint8_t)((uint8_t)c->ax); c->ip = 0x1178u; break;
        case 0x1178u: al(c, (uint8_t)(ram[physical(c->ss, (uint16_t)(c->bp - 2u))])); c->ip = 0x117Bu; break;
        case 0x117Bu: (void)arithmetic(c, (uint8_t)c->ax, 0u, 8u, 1u); c->ip = 0x117Du; break;
        case 0x117Du: c->ip = (c->flags & CF) != 0u ? 0x1189u : 0x117Fu; break;
        case 0x117Fu: (void)arithmetic(c, (uint8_t)c->ax, 2u, 8u, 1u); c->ip = 0x1181u; break;
        case 0x1181u: c->ip = (c->flags & (CF | ZF)) == 0u ? 0x1189u : 0x1183u; break;
        case 0x1183u: ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = (uint8_t)(0xfcu); c->ip = 0x1187u; break;
        case 0x1187u: c->ip = 0x11e1u; break;
        case 0x1189u: (void)arithmetic(c, (uint8_t)c->ax, 3u, 8u, 1u); c->ip = 0x118Bu; break;
        case 0x118Bu: c->ip = (c->flags & CF) != 0u ? 0x119eu : 0x118Du; break;
        case 0x118Du: (void)arithmetic(c, (uint8_t)c->ax, 5u, 8u, 1u); c->ip = 0x118Fu; break;
        case 0x118Fu: c->ip = (c->flags & (CF | ZF)) == 0u ? 0x119eu : 0x1191u; break;
        case 0x1191u: al(c, (uint8_t)(ram[physical(c->ss, (uint16_t)(c->bp - 2u))])); c->ip = 0x1194u; break;
        case 0x1194u: c->ax &= 0x00FFu; flags(c, ZF | PF); c->ip = 0x1196u; break;
        case 0x1196u: c->ax = arithmetic(c, c->ax, 6u, 16u, 1u); c->ip = 0x1199u; break;
        case 0x1199u: ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = (uint8_t)((uint8_t)c->ax); c->ip = 0x119Cu; break;
        case 0x119Cu: c->ip = 0x11e1u; break;
        case 0x119Eu: (void)arithmetic(c, (uint8_t)c->ax, 0x10u, 8u, 1u); c->ip = 0x11A0u; break;
        case 0x11A0u: c->ip = (c->flags & CF) != 0u ? 0x11b3u : 0x11A2u; break;
        case 0x11A2u: (void)arithmetic(c, (uint8_t)c->ax, 0x12u, 8u, 1u); c->ip = 0x11A4u; break;
        case 0x11A4u: c->ip = (c->flags & (CF | ZF)) == 0u ? 0x11b3u : 0x11A6u; break;
        case 0x11A6u: al(c, (uint8_t)(ram[physical(c->ss, (uint16_t)(c->bp - 2u))])); c->ip = 0x11A9u; break;
        case 0x11A9u: c->ax &= 0x00FFu; flags(c, ZF | PF); c->ip = 0x11ABu; break;
        case 0x11ABu: c->ax = arithmetic(c, c->ax, 0xfu, 16u, 1u); c->ip = 0x11AEu; break;
        case 0x11AEu: ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = (uint8_t)((uint8_t)c->ax); c->ip = 0x11B1u; break;
        case 0x11B1u: c->ip = 0x11e1u; break;
        case 0x11B3u: (void)arithmetic(c, (uint8_t)c->ax, 0x13u, 8u, 1u); c->ip = 0x11B5u; break;
        case 0x11B5u: c->ip = (c->flags & CF) != 0u ? 0x11c1u : 0x11B7u; break;
        case 0x11B7u: (void)arithmetic(c, (uint8_t)c->ax, 0x14u, 8u, 1u); c->ip = 0x11B9u; break;
        case 0x11B9u: c->ip = (c->flags & (CF | ZF)) == 0u ? 0x11c1u : 0x11BBu; break;
        case 0x11BBu: ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = (uint8_t)(3u); c->ip = 0x11BFu; break;
        case 0x11BFu: c->ip = 0x11e1u; break;
        case 0x11C1u: (void)arithmetic(c, (uint8_t)c->ax, 0x15u, 8u, 1u); c->ip = 0x11C3u; break;
        case 0x11C3u: c->ip = (c->flags & CF) != 0u ? 0x11cfu : 0x11C5u; break;
        case 0x11C5u: (void)arithmetic(c, (uint8_t)c->ax, 0x17u, 8u, 1u); c->ip = 0x11C7u; break;
        case 0x11C7u: c->ip = (c->flags & (CF | ZF)) == 0u ? 0x11cfu : 0x11C9u; break;
        case 0x11C9u: ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = (uint8_t)(4u); c->ip = 0x11CDu; break;
        case 0x11CDu: c->ip = 0x11e1u; break;
        case 0x11CFu: (void)arithmetic(c, (uint8_t)c->ax, 0x18u, 8u, 1u); c->ip = 0x11D1u; break;
        case 0x11D1u: c->ip = (c->flags & CF) != 0u ? 0x11ddu : 0x11D3u; break;
        case 0x11D3u: (void)arithmetic(c, (uint8_t)c->ax, 0x19u, 8u, 1u); c->ip = 0x11D5u; break;
        case 0x11D5u: c->ip = (c->flags & (CF | ZF)) == 0u ? 0x11ddu : 0x11D7u; break;
        case 0x11D7u: ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = (uint8_t)(5u); c->ip = 0x11DBu; break;
        case 0x11DBu: c->ip = 0x11e1u; break;
        case 0x11DDu: ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = (uint8_t)(0u); c->ip = 0x11E1u; break;
        case 0x11E1u: al(c, (uint8_t)(ram[physical(c->ss, (uint16_t)(c->bp - 1u))])); c->ip = 0x11E4u; break;
        case 0x11E4u: c->sp = c->bp; c->ip = 0x11E6u; break;
        case 0x11E6u: c->bp = pop(ram, c); c->ip = 0x11E7u; break;
        case 0x11E7u: c->ip = pop(ram, c); c->cs = pop(ram, c); c->sp = (uint16_t)(c->sp + 4u); return PR_RES00BA_OK;
        case 0x11EAu: push(ram, c, c->bp); c->ip = 0x11EBu; break;
        case 0x11EBu: c->bp = c->sp; c->ip = 0x11EDu; break;
        case 0x11EDu: c->sp = arithmetic(c, c->sp, 2u, 16u, 1u); c->ip = 0x11F0u; break;
        case 0x11F0u: load_pointer(ram, c); c->ip = 0x11F3u; break;
        case 0x11F3u: al(c, (uint8_t)(ram[physical(c->es, (uint16_t)(c->di + 0x10u))])); c->ip = 0x11F7u; break;
        case 0x11F7u: (void)arithmetic(c, (uint8_t)c->ax, 0u, 8u, 1u); c->ip = 0x11F9u; break;
        case 0x11F9u: c->ip = (c->flags & CF) != 0u ? 0x120bu : 0x11FBu; break;
        case 0x11FBu: (void)arithmetic(c, (uint8_t)c->ax, 0x11u, 8u, 1u); c->ip = 0x11FDu; break;
        case 0x11FDu: c->ip = (c->flags & (CF | ZF)) == 0u ? 0x120bu : 0x11FFu; break;
        case 0x11FFu: load_pointer(ram, c); c->ip = 0x1202u; break;
        case 0x1202u: al(c, (uint8_t)(ram[physical(c->es, (uint16_t)(c->di + 0x10u))])); c->ip = 0x1206u; break;
        case 0x1206u: ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = (uint8_t)((uint8_t)c->ax); c->ip = 0x1209u; break;
        case 0x1209u: c->ip = 0x1279u; break;
        case 0x120Bu: (void)arithmetic(c, (uint8_t)c->ax, 0x12u, 8u, 1u); c->ip = 0x120Du; break;
        case 0x120Du: c->ip = (c->flags & ZF) == 0u ? 0x1262u : 0x120Fu; break;
        case 0x120Fu: load_pointer(ram, c); c->ip = 0x1212u; break;
        case 0x1212u: al(c, (uint8_t)(ram[physical(c->es, (uint16_t)(c->di + 0x16u))])); c->ip = 0x1216u; break;
        case 0x1216u: (void)arithmetic(c, (uint8_t)c->ax, 0u, 8u, 1u); c->ip = 0x1218u; break;
        case 0x1218u: c->ip = (c->flags & ZF) == 0u ? 0x1220u : 0x121Au; break;
        case 0x121Au: ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = (uint8_t)(0x12u); c->ip = 0x121Eu; break;
        case 0x121Eu: c->ip = 0x1260u; break;
        case 0x1220u: (void)arithmetic(c, (uint8_t)c->ax, 1u, 8u, 1u); c->ip = 0x1222u; break;
        case 0x1222u: c->ip = (c->flags & CF) != 0u ? 0x122eu : 0x1224u; break;
        case 0x1224u: (void)arithmetic(c, (uint8_t)c->ax, 0x32u, 8u, 1u); c->ip = 0x1226u; break;
        case 0x1226u: c->ip = (c->flags & (CF | ZF)) == 0u ? 0x122eu : 0x1228u; break;
        case 0x1228u: ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = (uint8_t)(0x13u); c->ip = 0x122Cu; break;
        case 0x122Cu: c->ip = 0x1260u; break;
        case 0x122Eu: (void)arithmetic(c, (uint8_t)c->ax, 0x33u, 8u, 1u); c->ip = 0x1230u; break;
        case 0x1230u: c->ip = (c->flags & CF) != 0u ? 0x123cu : 0x1232u; break;
        case 0x1232u: (void)arithmetic(c, (uint8_t)c->ax, 0x4bu, 8u, 1u); c->ip = 0x1234u; break;
        case 0x1234u: c->ip = (c->flags & (CF | ZF)) == 0u ? 0x123cu : 0x1236u; break;
        case 0x1236u: ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = (uint8_t)(0x14u); c->ip = 0x123Au; break;
        case 0x123Au: c->ip = 0x1260u; break;
        case 0x123Cu: (void)arithmetic(c, (uint8_t)c->ax, 0x4cu, 8u, 1u); c->ip = 0x123Eu; break;
        case 0x123Eu: c->ip = (c->flags & CF) != 0u ? 0x124au : 0x1240u; break;
        case 0x1240u: (void)arithmetic(c, (uint8_t)c->ax, 0x5au, 8u, 1u); c->ip = 0x1242u; break;
        case 0x1242u: c->ip = (c->flags & (CF | ZF)) == 0u ? 0x124au : 0x1244u; break;
        case 0x1244u: ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = (uint8_t)(0x15u); c->ip = 0x1248u; break;
        case 0x1248u: c->ip = 0x1260u; break;
        case 0x124Au: (void)arithmetic(c, (uint8_t)c->ax, 0x5bu, 8u, 1u); c->ip = 0x124Cu; break;
        case 0x124Cu: c->ip = (c->flags & CF) != 0u ? 0x1258u : 0x124Eu; break;
        case 0x124Eu: (void)arithmetic(c, (uint8_t)c->ax, 0x63u, 8u, 1u); c->ip = 0x1250u; break;
        case 0x1250u: c->ip = (c->flags & (CF | ZF)) == 0u ? 0x1258u : 0x1252u; break;
        case 0x1252u: ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = (uint8_t)(0x16u); c->ip = 0x1256u; break;
        case 0x1256u: c->ip = 0x1260u; break;
        case 0x1258u: (void)arithmetic(c, (uint8_t)c->ax, 0x64u, 8u, 1u); c->ip = 0x125Au; break;
        case 0x125Au: c->ip = (c->flags & ZF) == 0u ? 0x1260u : 0x125Cu; break;
        case 0x125Cu: ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = (uint8_t)(0x17u); c->ip = 0x1260u; break;
        case 0x1260u: c->ip = 0x1279u; break;
        case 0x1262u: (void)arithmetic(c, (uint8_t)c->ax, 0x13u, 8u, 1u); c->ip = 0x1264u; break;
        case 0x1264u: c->ip = (c->flags & CF) != 0u ? 0x1279u : 0x1266u; break;
        case 0x1266u: (void)arithmetic(c, (uint8_t)c->ax, 0x19u, 8u, 1u); c->ip = 0x1268u; break;
        case 0x1268u: c->ip = (c->flags & (CF | ZF)) == 0u ? 0x1279u : 0x126Au; break;
        case 0x126Au: load_pointer(ram, c); c->ip = 0x126Du; break;
        case 0x126Du: al(c, (uint8_t)(ram[physical(c->es, (uint16_t)(c->di + 0x10u))])); c->ip = 0x1271u; break;
        case 0x1271u: c->ax &= 0x00FFu; flags(c, ZF | PF); c->ip = 0x1273u; break;
        case 0x1273u: c->ax = arithmetic(c, c->ax, 5u, 16u, 0u); c->ip = 0x1276u; break;
        case 0x1276u: ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = (uint8_t)((uint8_t)c->ax); c->ip = 0x1279u; break;
        case 0x1279u: al(c, (uint8_t)(ram[physical(c->ss, (uint16_t)(c->bp - 1u))])); c->ip = 0x127Cu; break;
        case 0x127Cu: c->sp = c->bp; c->ip = 0x127Eu; break;
        case 0x127Eu: c->bp = pop(ram, c); c->ip = 0x127Fu; break;
        case 0x127Fu: c->ip = pop(ram, c); c->cs = pop(ram, c); c->sp = (uint16_t)(c->sp + 4u); return PR_RES00BA_OK;
        case 0x1282u: push(ram, c, c->bp); c->ip = 0x1283u; break;
        case 0x1283u: c->bp = c->sp; c->ip = 0x1285u; break;
        case 0x1285u: c->sp = arithmetic(c, c->sp, 2u, 16u, 1u); c->ip = 0x1288u; break;
        case 0x1288u: ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = (uint8_t)(0u); c->ip = 0x128Cu; break;
        case 0x128Cu: push(ram, c, word(ram, c->ss, (uint16_t)(c->bp + 8u))); c->ip = 0x128Fu; break;
        case 0x128Fu: push(ram, c, word(ram, c->ss, (uint16_t)(c->bp + 6u))); c->ip = 0x1292u; break;
        case 0x1292u: push(ram, c, c->cs); c->ip = 0x1293u; break;
        case 0x1293u: {
            uint16_t caller_cs = c->cs;
            pr_res00ba_status status;
            push(ram, c, 0x1296u);
            c->ip = 0x11EAu;
            status = execute(ram, c, budget);
            if (status != PR_RES00BA_OK) return status;
            if (c->cs != caller_cs || c->ip != 0x1296u) return PR_RES00BA_TRANSFER;
            break;
        }
        case 0x1296u: ram[physical(c->ss, (uint16_t)(c->bp - 2u))] = (uint8_t)((uint8_t)c->ax); c->ip = 0x1299u; break;
        case 0x1299u: load_pointer(ram, c); c->ip = 0x129Cu; break;
        case 0x129Cu: (void)arithmetic(c, ram[physical(c->es, (uint16_t)(c->di + 0xaau))], 0u, 8u, 1u); c->ip = 0x12A2u; break;
        case 0x12A2u: c->ip = (c->flags & ZF) != 0u ? 0x131cu : 0x12A4u; break;
        case 0x12A4u: al(c, (uint8_t)(ram[physical(c->ss, (uint16_t)(c->bp - 2u))])); c->ip = 0x12A7u; break;
        case 0x12A7u: (void)arithmetic(c, (uint8_t)c->ax, 1u, 8u, 1u); c->ip = 0x12A9u; break;
        case 0x12A9u: c->ip = (c->flags & CF) != 0u ? 0x12b5u : 0x12ABu; break;
        case 0x12ABu: (void)arithmetic(c, (uint8_t)c->ax, 3u, 8u, 1u); c->ip = 0x12ADu; break;
        case 0x12ADu: c->ip = (c->flags & (CF | ZF)) == 0u ? 0x12b5u : 0x12AFu; break;
        case 0x12AFu: ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = (uint8_t)(0xfdu); c->ip = 0x12B3u; break;
        case 0x12B3u: c->ip = 0x131cu; break;
        case 0x12B5u: (void)arithmetic(c, (uint8_t)c->ax, 4u, 8u, 1u); c->ip = 0x12B7u; break;
        case 0x12B7u: c->ip = (c->flags & CF) != 0u ? 0x12c3u : 0x12B9u; break;
        case 0x12B9u: (void)arithmetic(c, (uint8_t)c->ax, 5u, 8u, 1u); c->ip = 0x12BBu; break;
        case 0x12BBu: c->ip = (c->flags & (CF | ZF)) == 0u ? 0x12c3u : 0x12BDu; break;
        case 0x12BDu: ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = (uint8_t)(0xfeu); c->ip = 0x12C1u; break;
        case 0x12C1u: c->ip = 0x131cu; break;
        case 0x12C3u: (void)arithmetic(c, (uint8_t)c->ax, 6u, 8u, 1u); c->ip = 0x12C5u; break;
        case 0x12C5u: c->ip = (c->flags & CF) != 0u ? 0x12d1u : 0x12C7u; break;
        case 0x12C7u: (void)arithmetic(c, (uint8_t)c->ax, 7u, 8u, 1u); c->ip = 0x12C9u; break;
        case 0x12C9u: c->ip = (c->flags & (CF | ZF)) == 0u ? 0x12d1u : 0x12CBu; break;
        case 0x12CBu: ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = (uint8_t)(0xffu); c->ip = 0x12CFu; break;
        case 0x12CFu: c->ip = 0x131cu; break;
        case 0x12D1u: (void)arithmetic(c, (uint8_t)c->ax, 0x11u, 8u, 1u); c->ip = 0x12D3u; break;
        case 0x12D3u: c->ip = (c->flags & CF) != 0u ? 0x12dfu : 0x12D5u; break;
        case 0x12D5u: (void)arithmetic(c, (uint8_t)c->ax, 0x13u, 8u, 1u); c->ip = 0x12D7u; break;
        case 0x12D7u: c->ip = (c->flags & (CF | ZF)) == 0u ? 0x12dfu : 0x12D9u; break;
        case 0x12D9u: ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = (uint8_t)(1u); c->ip = 0x12DDu; break;
        case 0x12DDu: c->ip = 0x131cu; break;
        case 0x12DFu: (void)arithmetic(c, (uint8_t)c->ax, 0x14u, 8u, 1u); c->ip = 0x12E1u; break;
        case 0x12E1u: c->ip = (c->flags & CF) != 0u ? 0x12edu : 0x12E3u; break;
        case 0x12E3u: (void)arithmetic(c, (uint8_t)c->ax, 0x16u, 8u, 1u); c->ip = 0x12E5u; break;
        case 0x12E5u: c->ip = (c->flags & (CF | ZF)) == 0u ? 0x12edu : 0x12E7u; break;
        case 0x12E7u: ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = (uint8_t)(2u); c->ip = 0x12EBu; break;
        case 0x12EBu: c->ip = 0x131cu; break;
        case 0x12EDu: (void)arithmetic(c, (uint8_t)c->ax, 0x17u, 8u, 1u); c->ip = 0x12EFu; break;
        case 0x12EFu: c->ip = (c->flags & CF) != 0u ? 0x12fbu : 0x12F1u; break;
        case 0x12F1u: (void)arithmetic(c, (uint8_t)c->ax, 0x19u, 8u, 1u); c->ip = 0x12F3u; break;
        case 0x12F3u: c->ip = (c->flags & (CF | ZF)) == 0u ? 0x12fbu : 0x12F5u; break;
        case 0x12F5u: ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = (uint8_t)(3u); c->ip = 0x12F9u; break;
        case 0x12F9u: c->ip = 0x131cu; break;
        case 0x12FBu: (void)arithmetic(c, (uint8_t)c->ax, 0x1au, 8u, 1u); c->ip = 0x12FDu; break;
        case 0x12FDu: c->ip = (c->flags & CF) != 0u ? 0x1309u : 0x12FFu; break;
        case 0x12FFu: (void)arithmetic(c, (uint8_t)c->ax, 0x1bu, 8u, 1u); c->ip = 0x1301u; break;
        case 0x1301u: c->ip = (c->flags & (CF | ZF)) == 0u ? 0x1309u : 0x1303u; break;
        case 0x1303u: ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = (uint8_t)(4u); c->ip = 0x1307u; break;
        case 0x1307u: c->ip = 0x131cu; break;
        case 0x1309u: (void)arithmetic(c, (uint8_t)c->ax, 0x1cu, 8u, 1u); c->ip = 0x130Bu; break;
        case 0x130Bu: c->ip = (c->flags & CF) != 0u ? 0x131cu : 0x130Du; break;
        case 0x130Du: (void)arithmetic(c, (uint8_t)c->ax, 0x1eu, 8u, 1u); c->ip = 0x130Fu; break;
        case 0x130Fu: c->ip = (c->flags & (CF | ZF)) == 0u ? 0x131cu : 0x1311u; break;
        case 0x1311u: al(c, (uint8_t)(ram[physical(c->ss, (uint16_t)(c->bp - 2u))])); c->ip = 0x1314u; break;
        case 0x1314u: c->ax &= 0x00FFu; flags(c, ZF | PF); c->ip = 0x1316u; break;
        case 0x1316u: c->ax = arithmetic(c, c->ax, 0x17u, 16u, 1u); c->ip = 0x1319u; break;
        case 0x1319u: ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = (uint8_t)((uint8_t)c->ax); c->ip = 0x131Cu; break;
        case 0x131Cu: al(c, (uint8_t)(ram[physical(c->ss, (uint16_t)(c->bp - 1u))])); c->ip = 0x131Fu; break;
        case 0x131Fu: c->sp = c->bp; c->ip = 0x1321u; break;
        case 0x1321u: c->bp = pop(ram, c); c->ip = 0x1322u; break;
        case 0x1322u: c->ip = pop(ram, c); c->cs = pop(ram, c); c->sp = (uint16_t)(c->sp + 4u); return PR_RES00BA_OK;
        case 0x1325u: push(ram, c, c->bp); c->ip = 0x1326u; break;
        case 0x1326u: c->bp = c->sp; c->ip = 0x1328u; break;
        case 0x1328u: c->sp = arithmetic(c, c->sp, 2u, 16u, 1u); c->ip = 0x132Bu; break;
        case 0x132Bu: ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = (uint8_t)(0u); c->ip = 0x132Fu; break;
        case 0x132Fu: push(ram, c, word(ram, c->ss, (uint16_t)(c->bp + 8u))); c->ip = 0x1332u; break;
        case 0x1332u: push(ram, c, word(ram, c->ss, (uint16_t)(c->bp + 6u))); c->ip = 0x1335u; break;
        case 0x1335u: push(ram, c, c->cs); c->ip = 0x1336u; break;
        case 0x1336u: {
            uint16_t caller_cs = c->cs;
            pr_res00ba_status status;
            push(ram, c, 0x1339u);
            c->ip = 0x11EAu;
            status = execute(ram, c, budget);
            if (status != PR_RES00BA_OK) return status;
            if (c->cs != caller_cs || c->ip != 0x1339u) return PR_RES00BA_TRANSFER;
            break;
        }
        case 0x1339u: ram[physical(c->ss, (uint16_t)(c->bp - 2u))] = (uint8_t)((uint8_t)c->ax); c->ip = 0x133Cu; break;
        case 0x133Cu: load_pointer(ram, c); c->ip = 0x133Fu; break;
        case 0x133Fu: (void)arithmetic(c, ram[physical(c->es, (uint16_t)(c->di + 0xaau))], 0u, 8u, 1u); c->ip = 0x1345u; break;
        case 0x1345u: c->ip = (c->flags & ZF) != 0u ? 0x13a2u : 0x1347u; break;
        case 0x1347u: al(c, (uint8_t)(ram[physical(c->ss, (uint16_t)(c->bp - 2u))])); c->ip = 0x134Au; break;
        case 0x134Au: (void)arithmetic(c, (uint8_t)c->ax, 1u, 8u, 1u); c->ip = 0x134Cu; break;
        case 0x134Cu: c->ip = (c->flags & CF) != 0u ? 0x1358u : 0x134Eu; break;
        case 0x134Eu: (void)arithmetic(c, (uint8_t)c->ax, 2u, 8u, 1u); c->ip = 0x1350u; break;
        case 0x1350u: c->ip = (c->flags & (CF | ZF)) == 0u ? 0x1358u : 0x1352u; break;
        case 0x1352u: ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = (uint8_t)(0xfeu); c->ip = 0x1356u; break;
        case 0x1356u: c->ip = 0x13a2u; break;
        case 0x1358u: (void)arithmetic(c, (uint8_t)c->ax, 3u, 8u, 1u); c->ip = 0x135Au; break;
        case 0x135Au: c->ip = (c->flags & CF) != 0u ? 0x1366u : 0x135Cu; break;
        case 0x135Cu: (void)arithmetic(c, (uint8_t)c->ax, 5u, 8u, 1u); c->ip = 0x135Eu; break;
        case 0x135Eu: c->ip = (c->flags & (CF | ZF)) == 0u ? 0x1366u : 0x1360u; break;
        case 0x1360u: ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = (uint8_t)(0xffu); c->ip = 0x1364u; break;
        case 0x1364u: c->ip = 0x13a2u; break;
        case 0x1366u: (void)arithmetic(c, (uint8_t)c->ax, 0x10u, 8u, 1u); c->ip = 0x1368u; break;
        case 0x1368u: c->ip = (c->flags & ZF) == 0u ? 0x1370u : 0x136Au; break;
        case 0x136Au: ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = (uint8_t)(1u); c->ip = 0x136Eu; break;
        case 0x136Eu: c->ip = 0x13a2u; break;
        case 0x1370u: (void)arithmetic(c, (uint8_t)c->ax, 0x11u, 8u, 1u); c->ip = 0x1372u; break;
        case 0x1372u: c->ip = (c->flags & CF) != 0u ? 0x1385u : 0x1374u; break;
        case 0x1374u: (void)arithmetic(c, (uint8_t)c->ax, 0x13u, 8u, 1u); c->ip = 0x1376u; break;
        case 0x1376u: c->ip = (c->flags & (CF | ZF)) == 0u ? 0x1385u : 0x1378u; break;
        case 0x1378u: al(c, (uint8_t)(ram[physical(c->ss, (uint16_t)(c->bp - 2u))])); c->ip = 0x137Bu; break;
        case 0x137Bu: c->ax &= 0x00FFu; flags(c, ZF | PF); c->ip = 0x137Du; break;
        case 0x137Du: c->ax = arithmetic(c, c->ax, 0x10u, 16u, 1u); c->ip = 0x1380u; break;
        case 0x1380u: ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = (uint8_t)((uint8_t)c->ax); c->ip = 0x1383u; break;
        case 0x1383u: c->ip = 0x13a2u; break;
        case 0x1385u: (void)arithmetic(c, (uint8_t)c->ax, 0x14u, 8u, 1u); c->ip = 0x1387u; break;
        case 0x1387u: c->ip = (c->flags & CF) != 0u ? 0x139au : 0x1389u; break;
        case 0x1389u: (void)arithmetic(c, (uint8_t)c->ax, 0x1du, 8u, 1u); c->ip = 0x138Bu; break;
        case 0x138Bu: c->ip = (c->flags & (CF | ZF)) == 0u ? 0x139au : 0x138Du; break;
        case 0x138Du: al(c, (uint8_t)(ram[physical(c->ss, (uint16_t)(c->bp - 2u))])); c->ip = 0x1390u; break;
        case 0x1390u: c->ax &= 0x00FFu; flags(c, ZF | PF); c->ip = 0x1392u; break;
        case 0x1392u: c->ax = arithmetic(c, c->ax, 0x11u, 16u, 1u); c->ip = 0x1395u; break;
        case 0x1395u: ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = (uint8_t)((uint8_t)c->ax); c->ip = 0x1398u; break;
        case 0x1398u: c->ip = 0x13a2u; break;
        case 0x139Au: (void)arithmetic(c, (uint8_t)c->ax, 0x1eu, 8u, 1u); c->ip = 0x139Cu; break;
        case 0x139Cu: c->ip = (c->flags & ZF) == 0u ? 0x13a2u : 0x139Eu; break;
        case 0x139Eu: ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = (uint8_t)(0xeu); c->ip = 0x13A2u; break;
        case 0x13A2u: al(c, (uint8_t)(ram[physical(c->ss, (uint16_t)(c->bp - 1u))])); c->ip = 0x13A5u; break;
        case 0x13A5u: c->sp = c->bp; c->ip = 0x13A7u; break;
        case 0x13A7u: c->bp = pop(ram, c); c->ip = 0x13A8u; break;
        case 0x13A8u: c->ip = pop(ram, c); c->cs = pop(ram, c); c->sp = (uint16_t)(c->sp + 4u); return PR_RES00BA_OK;
        case 0x13ABu: push(ram, c, c->bp); c->ip = 0x13ACu; break;
        case 0x13ACu: c->bp = c->sp; c->ip = 0x13AEu; break;
        case 0x13AEu: c->sp = arithmetic(c, c->sp, 4u, 16u, 1u); c->ip = 0x13B1u; break;
        case 0x13B1u: push(ram, c, word(ram, c->ss, (uint16_t)(c->bp + 8u))); c->ip = 0x13B4u; break;
        case 0x13B4u: push(ram, c, word(ram, c->ss, (uint16_t)(c->bp + 6u))); c->ip = 0x13B7u; break;
        case 0x13B7u: push(ram, c, c->cs); c->ip = 0x13B8u; break;
        case 0x13B8u: {
            uint16_t caller_cs = c->cs;
            pr_res00ba_status status;
            push(ram, c, 0x13BBu);
            c->ip = 0x11EAu;
            status = execute(ram, c, budget);
            if (status != PR_RES00BA_OK) return status;
            if (c->cs != caller_cs || c->ip != 0x13BBu) return PR_RES00BA_TRANSFER;
            break;
        }
        case 0x13BBu: ram[physical(c->ss, (uint16_t)(c->bp - 3u))] = (uint8_t)((uint8_t)c->ax); c->ip = 0x13BEu; break;
        case 0x13BEu: al(c, (uint8_t)(ram[physical(c->ss, (uint16_t)(c->bp - 3u))])); c->ip = 0x13C1u; break;
        case 0x13C1u: (void)arithmetic(c, (uint8_t)c->ax, 1u, 8u, 1u); c->ip = 0x13C3u; break;
        case 0x13C3u: c->ip = (c->flags & CF) != 0u ? 0x13d1u : 0x13C5u; break;
        case 0x13C5u: (void)arithmetic(c, (uint8_t)c->ax, 3u, 8u, 1u); c->ip = 0x13C7u; break;
        case 0x13C7u: c->ip = (c->flags & (CF | ZF)) == 0u ? 0x13d1u : 0x13C9u; break;
        case 0x13C9u: put_word(ram, c->ss, (uint16_t)(c->bp - 2u), 0xfea2u); c->ip = 0x13CEu; break;
        case 0x13CEu: c->ip = 0x1482u; break;
        case 0x13D1u: (void)arithmetic(c, (uint8_t)c->ax, 4u, 8u, 1u); c->ip = 0x13D3u; break;
        case 0x13D3u: c->ip = (c->flags & CF) != 0u ? 0x13e1u : 0x13D5u; break;
        case 0x13D5u: (void)arithmetic(c, (uint8_t)c->ax, 5u, 8u, 1u); c->ip = 0x13D7u; break;
        case 0x13D7u: c->ip = (c->flags & (CF | ZF)) == 0u ? 0x13e1u : 0x13D9u; break;
        case 0x13D9u: put_word(ram, c->ss, (uint16_t)(c->bp - 2u), 0xff06u); c->ip = 0x13DEu; break;
        case 0x13DEu: c->ip = 0x1482u; break;
        case 0x13E1u: (void)arithmetic(c, (uint8_t)c->ax, 6u, 8u, 1u); c->ip = 0x13E3u; break;
        case 0x13E3u: c->ip = (c->flags & CF) != 0u ? 0x13f1u : 0x13E5u; break;
        case 0x13E5u: (void)arithmetic(c, (uint8_t)c->ax, 7u, 8u, 1u); c->ip = 0x13E7u; break;
        case 0x13E7u: c->ip = (c->flags & (CF | ZF)) == 0u ? 0x13f1u : 0x13E9u; break;
        case 0x13E9u: put_word(ram, c->ss, (uint16_t)(c->bp - 2u), 0xff6au); c->ip = 0x13EEu; break;
        case 0x13EEu: c->ip = 0x1482u; break;
        case 0x13F1u: (void)arithmetic(c, (uint8_t)c->ax, 0xcu, 8u, 1u); c->ip = 0x13F3u; break;
        case 0x13F3u: c->ip = (c->flags & CF) != 0u ? 0x1401u : 0x13F5u; break;
        case 0x13F5u: (void)arithmetic(c, (uint8_t)c->ax, 0xdu, 8u, 1u); c->ip = 0x13F7u; break;
        case 0x13F7u: c->ip = (c->flags & (CF | ZF)) == 0u ? 0x1401u : 0x13F9u; break;
        case 0x13F9u: put_word(ram, c->ss, (uint16_t)(c->bp - 2u), 0x64u); c->ip = 0x13FEu; break;
        case 0x13FEu: c->ip = 0x1482u; break;
        case 0x1401u: (void)arithmetic(c, (uint8_t)c->ax, 0xeu, 8u, 1u); c->ip = 0x1403u; break;
        case 0x1403u: c->ip = (c->flags & CF) != 0u ? 0x1410u : 0x1405u; break;
        case 0x1405u: (void)arithmetic(c, (uint8_t)c->ax, 0xfu, 8u, 1u); c->ip = 0x1407u; break;
        case 0x1407u: c->ip = (c->flags & (CF | ZF)) == 0u ? 0x1410u : 0x1409u; break;
        case 0x1409u: put_word(ram, c->ss, (uint16_t)(c->bp - 2u), 0xc8u); c->ip = 0x140Eu; break;
        case 0x140Eu: c->ip = 0x1482u; break;
        case 0x1410u: (void)arithmetic(c, (uint8_t)c->ax, 0x10u, 8u, 1u); c->ip = 0x1412u; break;
        case 0x1412u: c->ip = (c->flags & ZF) == 0u ? 0x141bu : 0x1414u; break;
        case 0x1414u: put_word(ram, c->ss, (uint16_t)(c->bp - 2u), 0x15eu); c->ip = 0x1419u; break;
        case 0x1419u: c->ip = 0x1482u; break;
        case 0x141Bu: (void)arithmetic(c, (uint8_t)c->ax, 0x11u, 8u, 1u); c->ip = 0x141Du; break;
        case 0x141Du: c->ip = (c->flags & CF) != 0u ? 0x1438u : 0x141Fu; break;
        case 0x141Fu: (void)arithmetic(c, (uint8_t)c->ax, 0x15u, 8u, 1u); c->ip = 0x1421u; break;
        case 0x1421u: c->ip = (c->flags & (CF | ZF)) == 0u ? 0x1438u : 0x1423u; break;
        case 0x1423u: al(c, (uint8_t)(ram[physical(c->ss, (uint16_t)(c->bp - 3u))])); c->ip = 0x1426u; break;
        case 0x1426u: c->ax &= 0x00FFu; flags(c, ZF | PF); c->ip = 0x1428u; break;
        case 0x1428u: c->ax = arithmetic(c, c->ax, 0x11u, 16u, 1u); c->ip = 0x142Bu; break;
        case 0x142Bu: c->dx = 0xfau; c->ip = 0x142Eu; break;
        case 0x142Eu: multiply(c); c->ip = 0x1430u; break;
        case 0x1430u: c->ax = arithmetic(c, c->ax, 0x1f4u, 16u, 0u); c->ip = 0x1433u; break;
        case 0x1433u: put_word(ram, c->ss, (uint16_t)(c->bp - 2u), c->ax); c->ip = 0x1436u; break;
        case 0x1436u: c->ip = 0x1482u; break;
        case 0x1438u: (void)arithmetic(c, (uint8_t)c->ax, 0x16u, 8u, 1u); c->ip = 0x143Au; break;
        case 0x143Au: c->ip = (c->flags & CF) != 0u ? 0x1455u : 0x143Cu; break;
        case 0x143Cu: (void)arithmetic(c, (uint8_t)c->ax, 0x1au, 8u, 1u); c->ip = 0x143Eu; break;
        case 0x143Eu: c->ip = (c->flags & (CF | ZF)) == 0u ? 0x1455u : 0x1440u; break;
        case 0x1440u: al(c, (uint8_t)(ram[physical(c->ss, (uint16_t)(c->bp - 3u))])); c->ip = 0x1443u; break;
        case 0x1443u: c->ax &= 0x00FFu; flags(c, ZF | PF); c->ip = 0x1445u; break;
        case 0x1445u: c->ax = arithmetic(c, c->ax, 0x16u, 16u, 1u); c->ip = 0x1448u; break;
        case 0x1448u: c->dx = 0x3e8u; c->ip = 0x144Bu; break;
        case 0x144Bu: multiply(c); c->ip = 0x144Du; break;
        case 0x144Du: c->ax = arithmetic(c, c->ax, 0x7d0u, 16u, 0u); c->ip = 0x1450u; break;
        case 0x1450u: put_word(ram, c->ss, (uint16_t)(c->bp - 2u), c->ax); c->ip = 0x1453u; break;
        case 0x1453u: c->ip = 0x1482u; break;
        case 0x1455u: (void)arithmetic(c, (uint8_t)c->ax, 0x1bu, 8u, 1u); c->ip = 0x1457u; break;
        case 0x1457u: c->ip = (c->flags & ZF) == 0u ? 0x1460u : 0x1459u; break;
        case 0x1459u: put_word(ram, c->ss, (uint16_t)(c->bp - 2u), 0x1d4cu); c->ip = 0x145Eu; break;
        case 0x145Eu: c->ip = 0x1482u; break;
        case 0x1460u: (void)arithmetic(c, (uint8_t)c->ax, 0x1cu, 8u, 1u); c->ip = 0x1462u; break;
        case 0x1462u: c->ip = (c->flags & CF) != 0u ? 0x147du : 0x1464u; break;
        case 0x1464u: (void)arithmetic(c, (uint8_t)c->ax, 0x1eu, 8u, 1u); c->ip = 0x1466u; break;
        case 0x1466u: c->ip = (c->flags & (CF | ZF)) == 0u ? 0x147du : 0x1468u; break;
        case 0x1468u: al(c, (uint8_t)(ram[physical(c->ss, (uint16_t)(c->bp - 3u))])); c->ip = 0x146Bu; break;
        case 0x146Bu: c->ax &= 0x00FFu; flags(c, ZF | PF); c->ip = 0x146Du; break;
        case 0x146Du: c->ax = arithmetic(c, c->ax, 0x1cu, 16u, 1u); c->ip = 0x1470u; break;
        case 0x1470u: c->dx = 0xbb8u; c->ip = 0x1473u; break;
        case 0x1473u: multiply(c); c->ip = 0x1475u; break;
        case 0x1475u: c->ax = arithmetic(c, c->ax, 0x2328u, 16u, 0u); c->ip = 0x1478u; break;
        case 0x1478u: put_word(ram, c->ss, (uint16_t)(c->bp - 2u), c->ax); c->ip = 0x147Bu; break;
        case 0x147Bu: c->ip = 0x1482u; break;
        case 0x147Du: c->ax = 0u; flags(c, ZF | PF); c->ip = 0x147Fu; break;
        case 0x147Fu: put_word(ram, c->ss, (uint16_t)(c->bp - 2u), c->ax); c->ip = 0x1482u; break;
        case 0x1482u: c->ax = word(ram, c->ss, (uint16_t)(c->bp - 2u)); c->ip = 0x1485u; break;
        case 0x1485u: c->sp = c->bp; c->ip = 0x1487u; break;
        case 0x1487u: c->bp = pop(ram, c); c->ip = 0x1488u; break;
        case 0x1488u: c->ip = pop(ram, c); c->cs = pop(ram, c); c->sp = (uint16_t)(c->sp + 4u); return PR_RES00BA_OK;
        default: return PR_RES00BA_TRANSFER;
        }
    }
}

pr_res00ba_status pr_res00ba_helpers_cpu(uint8_t *ram, size_t ram_size,
                                       pr_res00ba_cpu *cpu, size_t *budget)
{
    if (ram == NULL || cpu == NULL || budget == NULL || ram_size != (1u << 20))
        return PR_RES00BA_ARGUMENT;
    switch (cpu->ip) {
    case 0x10E6u: case 0x1168u: case 0x11EAu:
    case 0x1282u: case 0x1325u: case 0x13ABu:
        return execute(ram, cpu, budget);
    default:
        return PR_RES00BA_ARGUMENT;
    }
}
