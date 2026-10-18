/* Focused synthetic test for raw-CPU U1C +266A composing U25 +0000/+0136 through the overlay stub and SYSTEM
 * FillChar (docs/re/U1C_PARTY_ITEM_SCAN_266A.md). No game bytes: only the three five-byte far-call sites, the two
 * loaded-stub EA jumps, a tag-0 ECL operand and synthetic party and item lists. */
#include "poolrad/recovered/overlay_u1c_party_item_scan_266a_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM PR_U1C_266A_RAM_SIZE
#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "u1c 266a line %d: %s\n", __LINE__, #x);              \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { CS = 0x8800, U25 = 0x7800, STUB = 0x002B, SYS = 0x0AF8, DS = 0x1800, SS = 0x2800, SP0 = 0x8000, BP1 = SP0 - 2, BP0 = 0x7777,
       PARTY = 0x5000, ITEMS = 0x5800, RET_IP = 0x7000, RET_CS = 0x9001 };

static uint8_t *ram;
static unsigned checks;

static size_t at(unsigned seg, unsigned off) { return (((size_t)seg << 4) + (off & 0xFFFFu)) & (RAM - 1u); }
static void w8(unsigned seg, unsigned off, unsigned v) { ram[at(seg, off)] = (uint8_t)v; }
static uint8_t r8(unsigned seg, unsigned off) { return ram[at(seg, off)]; }
static void w16(unsigned seg, unsigned off, unsigned v) { w8(seg, off, v & 0xFFu); w8(seg, off + 1u, v >> 8); }
static unsigned r16(unsigned seg, unsigned off) { return r8(seg, off) | (unsigned)r8(seg, off + 1u) << 8; }
static unsigned member_off(unsigned k) { return 0x0100u + 0x200u * k; }
static unsigned item_off(unsigned k, unsigned j) { return 0x0100u + 0x100u * k + 0x40u * j; }

/* members x items types; cycle: 0 none, 1 party list loops to the head, 2 member 0's items loop. */
static pr_u1c_266a_cpu world(unsigned value, unsigned members, unsigned items, const uint8_t *types, int cycle)
{
    pr_u1c_266a_cpu cpu = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu, BP0, SP0, CS, 0x266Au, DS, 0x4444u, SS, 0x0202u};
    memset(ram, 0x5C, RAM);
    w8(CS, 0x2673u, 0x9Au); w16(CS, 0x2674u, 0x002Au); w16(CS, 0x2676u, STUB);
    w8(CS, 0x267Fu, 0x9Au); w16(CS, 0x2680u, 0x0025u); w16(CS, 0x2682u, STUB);
    w8(CS, 0x2693u, 0x9Au); w16(CS, 0x2694u, 0x16A8u); w16(CS, 0x2696u, SYS);
    w8(STUB, 0x0025u, 0xEAu); w16(STUB, 0x0026u, 0x0136u); w16(STUB, 0x0028u, U25);
    w8(STUB, 0x002Au, 0xEAu); w16(STUB, 0x002Bu, 0x0000u); w16(STUB, 0x002Du, U25);
    w16(DS, 0x49DEu, 0x1000u); w16(DS, 0x49E0u, 0x6300u); w16(DS, 0x49EDu, 0x9900u);       /* one tag-0 operand */
    w8(0x6300u, 0x1001u, 0u); w8(0x6300u, 0x1002u, value);
    for (unsigned k = 0u; k < members; ++k) {
        unsigned next = k + 1u < members ? member_off(k + 1u) : (cycle == 1 ? member_off(0u) : 0u);
        w16(PARTY, member_off(k) + 0x104u, next); w16(PARTY, member_off(k) + 0x106u, next ? PARTY : 0u);
        w16(PARTY, member_off(k) + 0xC8u, items ? item_off(k, 0u) : 0u); w16(PARTY, member_off(k) + 0xCAu, items ? ITEMS : 0u);
        for (unsigned j = 0u; j < items; ++j) {
            unsigned inext = j + 1u < items ? item_off(k, j + 1u) : (cycle == 2 && k == 0u ? item_off(0u, 0u) : 0u);
            w8(ITEMS, item_off(k, j) + 0x2Eu, types[k * items + j]);
            w16(ITEMS, item_off(k, j) + 0x2Au, inext); w16(ITEMS, item_off(k, j) + 0x2Cu, inext ? ITEMS : 0u);
        }
    }
    w16(DS, 0x5D96u, members ? member_off(0u) : 0u); w16(DS, 0x5D98u, members ? PARTY : 0u);
    w16(SS, SP0, RET_IP); w16(SS, SP0 + 2u, RET_CS);
    return cpu;
}

static void scan(unsigned value, unsigned members, unsigned items, const uint8_t *types)
{
    pr_u1c_266a_cpu cpu = world(value, members, items, types, 0);
    pr_u1c_266a_limits limits = {SYS, 4u, 400u, 1000u};
    pr_u1c_266a_callee callee;
    int callee_status = 99, found = 0;
    for (unsigned i = 0u; i < members * items; ++i) found |= types[i] == value;
    CHECK(pr_u1c_party_item_scan_266a_cpu(ram, RAM, &cpu, &limits, &callee, &callee_status) == PR_U1C_266A_OK);
    CHECK(callee == PR_U1C_266A_FILLCHAR && callee_status == 0);
    CHECK(r8(SS, BP1 - 1u) == value && r8(SS, BP1 - 0x0Bu) == (unsigned)found);
    CHECK(r8(DS, 0x6F78u) == (unsigned)found && r8(DS, 0x6F79u) == (unsigned)!found);
    for (unsigned i = 2u; i < 6u; ++i) CHECK(r8(DS, 0x6F78u + i) == 0u);
    CHECK(r8(DS, 0x7006u) == 0u && r8(DS, 0x7086u) == value && r16(DS, 0x49EDu) == 0x9903u);   /* the loaded operand; PC += 2, then 1 on exit */
    CHECK(cpu.bp == BP0 && cpu.sp == SP0 + 4u && cpu.cs == RET_CS && cpu.ip == RET_IP && cpu.ds == DS && (cpu.flags & 0x0400u) == 0u);
}

static void scans(void)
{
    static const uint8_t types[12] = {3, 9, 27, 81, 4, 16, 64, 255, 5, 25, 125, 0};
    scan(27u, 3u, 4u, types);                                              /* found in member 0 */
    scan(255u, 3u, 4u, types);                                             /* found in member 1 */
    scan(0u, 3u, 4u, types);                                               /* the last item of the last member */
    scan(7u, 3u, 4u, types);                                               /* absent */
    scan(7u, 0u, 0u, types);                                               /* an empty party */
    scan(3u, 2u, 0u, types);                                               /* members without items */
    for (unsigned v = 0u; v < 256u; v += 17u) scan(v, 3u, 4u, types);
}

static void stops(void)
{
    static const uint8_t types[12] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    pr_u1c_266a_limits limits = {SYS, 4u, 400u, 5u};
    pr_u1c_266a_cpu cpu;
    pr_u1c_266a_callee callee;
    int callee_status = 99;
    cpu = world(0x77u, 3u, 0u, types, 1);                                  /* a cyclic party: LIMIT at the outer head */
    CHECK(pr_u1c_party_item_scan_266a_cpu(ram, RAM, &cpu, &limits, &callee, &callee_status) == PR_U1C_266A_LIMIT);
    CHECK(cpu.ip == 0x26AAu && cpu.cs == CS);
    cpu = world(0x77u, 2u, 3u, types, 2);                                  /* a cyclic item list: LIMIT at the inner head */
    CHECK(pr_u1c_party_item_scan_266a_cpu(ram, RAM, &cpu, &limits, &callee, &callee_status) == PR_U1C_266A_LIMIT);
    CHECK(cpu.ip == 0x26CBu);
    limits.max_loopbacks = 1000u;
    cpu = world(3u, 1u, 1u, types, 0);                                     /* the +0136 stub not loaded: CODE at the second site */
    w8(STUB, 0x0025u, 0xCDu);
    CHECK(pr_u1c_party_item_scan_266a_cpu(ram, RAM, &cpu, &limits, &callee, &callee_status) == PR_U1C_266A_CODE);
    CHECK(cpu.ip == 0x267Fu && callee == PR_U1C_266A_LOAD_OPERANDS && callee_status == 0);
    cpu = world(3u, 1u, 1u, types, 0);                                     /* the first site rewritten */
    w8(CS, 0x2673u, 0x90u);
    CHECK(pr_u1c_party_item_scan_266a_cpu(ram, RAM, &cpu, &limits, &callee, &callee_status) == PR_U1C_266A_CODE);
    CHECK(cpu.ip == 0x2673u && callee == PR_U1C_266A_NO_CALLEE && callee_status == -1 && cpu.sp == BP1 - 0x0Cu - 2u);
    cpu = world(3u, 1u, 1u, types, 0);                                     /* U25 +0000's operand budget of zero: CALLEE */
    limits.max_operands = 0u;
    CHECK(pr_u1c_party_item_scan_266a_cpu(ram, RAM, &cpu, &limits, &callee, &callee_status) == PR_U1C_266A_CALLEE);
    CHECK(callee == PR_U1C_266A_LOAD_OPERANDS && callee_status == 1 && cpu.ip == 0x0000u && cpu.cs == U25);
}

static void arguments(void)
{
    static const uint8_t types[1] = {1};
    pr_u1c_266a_limits limits = {SYS, 4u, 400u, 1000u};
    pr_u1c_266a_cpu cpu = world(1u, 1u, 1u, types, 0), before = cpu;
    pr_u1c_266a_callee callee = PR_U1C_266A_FILLCHAR;
    int callee_status = 99;
    CHECK(pr_u1c_party_item_scan_266a_cpu(NULL, RAM, &cpu, &limits, &callee, &callee_status) == PR_U1C_266A_ARGUMENT);
    CHECK(callee == PR_U1C_266A_NO_CALLEE && callee_status == -1);
    CHECK(pr_u1c_party_item_scan_266a_cpu(ram, RAM, NULL, &limits, NULL, NULL) == PR_U1C_266A_ARGUMENT);
    CHECK(pr_u1c_party_item_scan_266a_cpu(ram, RAM, &cpu, NULL, NULL, NULL) == PR_U1C_266A_ARGUMENT);
    CHECK(pr_u1c_party_item_scan_266a_cpu(ram, RAM - 1u, &cpu, &limits, NULL, NULL) == PR_U1C_266A_ARGUMENT);
    CHECK(memcmp(&cpu, &before, sizeof cpu) == 0);
}

int main(void)
{
    ram = malloc(RAM);
    CHECK(ram != NULL);
    scans();
    stops();
    arguments();
    free(ram);
    printf("u1c +266a: %u checks passed\n", checks);
    return 0;
}
