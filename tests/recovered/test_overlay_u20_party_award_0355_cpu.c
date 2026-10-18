/* Focused synthetic test for raw-CPU U20 +0355 composing the SYSTEM LongInt divide (docs/re/U20_PARTY_AWARD_0355.md).
 * No game bytes: only the six five-byte far-call sites and synthetic party records. An independent reference of the
 * award rules predicts every member's LongInt field, and all RAM outside the stack window must match it. */
#include "poolrad/recovered/overlay_u20_party_award_0355_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM PR_U20_0355_RAM_SIZE
#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "u20 0355 line %d: %s\n", __LINE__, #x);              \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { CS = 0x8800, SYS = 0x0AF8, DS = 0x1800, SS = 0x2800, SP0 = 0x8000, BP0 = 0x7777, PARTY = 0x5000, RET_IP = 0x7000,
       RET_CS = 0x9001, CAP = 1000 };

typedef struct { uint8_t active, state, cls, stats[4]; uint32_t xp; } record;

static const unsigned sites[6] = {0x03BBu, 0x03E8u, 0x0415u, 0x0441u, 0x046Fu, 0x048Fu};
static uint8_t *ram, *want;
static unsigned checks;

static size_t at(unsigned seg, unsigned off) { return (((size_t)seg << 4) + (off & 0xFFFFu)) & (RAM - 1u); }
static void w8(uint8_t *mem, unsigned seg, unsigned off, unsigned v) { mem[at(seg, off)] = (uint8_t)v; }
static void w16(uint8_t *mem, unsigned seg, unsigned off, unsigned v) { w8(mem, seg, off, v & 0xFFu); w8(mem, seg, off + 1u, v >> 8); }
static unsigned member_off(unsigned k) { return 0x0100u + 0x200u * k; }

static pr_u20_0355_cpu world(const record *members, unsigned count, uint32_t x, int cyclic)
{
    static const unsigned stat_field[4] = {0x12u, 0x10u, 0x11u, 0x13u};
    pr_u20_0355_cpu cpu = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu, BP0, SP0, CS, 0x0355u, DS, 0x4444u, SS, 0x0202u};
    memset(ram, 0x5C, RAM);
    for (unsigned s = 0u; s < 6u; ++s) { w8(ram, CS, sites[s], 0x9Au); w16(ram, CS, sites[s] + 1u, 0x0A76u); w16(ram, CS, sites[s] + 3u, SYS); }
    for (unsigned k = 0u; k < count; ++k) {
        unsigned off = member_off(k), next = k + 1u < count ? member_off(k + 1u) : (cyclic ? member_off(0u) : 0u);
        w8(ram, PARTY, off + 0x10Du, members[k].active); w8(ram, PARTY, off + 0x10Cu, members[k].state); w8(ram, PARTY, off + 0x2Fu, members[k].cls);
        for (unsigned j = 0u; j < 4u; ++j) w8(ram, PARTY, off + stat_field[j], members[k].stats[j]);
        w16(ram, PARTY, off + 0xACu, members[k].xp & 0xFFFFu); w16(ram, PARTY, off + 0xAEu, members[k].xp >> 16);
        w16(ram, PARTY, off + 0x104u, next); w16(ram, PARTY, off + 0x106u, next ? PARTY : 0u);
    }
    w16(ram, DS, 0x5D96u, count ? member_off(0u) : 0u); w16(ram, DS, 0x5D98u, count ? PARTY : 0u);
    w16(ram, SS, SP0, RET_IP); w16(ram, SS, SP0 + 2u, RET_CS); w16(ram, SS, SP0 + 4u, x & 0xFFFFu); w16(ram, SS, SP0 + 6u, x >> 16);
    return cpu;
}

/* The award for one member, by the routine's class rules (signed truncating division). */
static uint32_t award(const record *m, uint32_t x)
{
    int32_t sx = (int32_t)x;
    int c = (signed char)m->cls;
    if (m->active == 0u || m->state == 1u) return 0u;
    switch (m->cls) {
    case 0u: return m->stats[0] > 15u ? (uint32_t)(sx / 10) + x : x;
    case 2u: return m->stats[1] > 15u ? (uint32_t)(sx / 10) + x : x;
    case 5u: return m->stats[2] > 15u ? (uint32_t)(sx / 10) + x : x;
    case 6u: return m->stats[3] > 15u ? (uint32_t)(sx / 10) + x : x;
    default: break;
    }
    if (m->cls == 8u || (c >= 10 && c <= 14) || m->cls == 16u) return (uint32_t)(sx / 2);
    if (m->cls == 9u || m->cls == 15u) return (uint32_t)(sx / 3);
    return x;
}

static void run_case(const record *members, unsigned count, uint32_t x)
{
    pr_u20_0355_cpu cpu = world(members, count, x, 0);
    int callee = 99;
    memcpy(want, ram, RAM);
    for (unsigned k = 0u; k < count; ++k) {
        uint32_t xp = members[k].xp + award(&members[k], x);
        w16(want, PARTY, member_off(k) + 0xACu, xp & 0xFFFFu); w16(want, PARTY, member_off(k) + 0xAEu, xp >> 16);
    }
    CHECK(pr_u20_party_award_0355_cpu(ram, RAM, &cpu, CAP, &callee) == PR_U20_0355_OK);
    for (unsigned off = SP0 - 0x40u; off < SP0 + 8u; ++off) want[at(SS, off)] = ram[at(SS, off)];
    CHECK(memcmp(ram, want, RAM) == 0);
    CHECK(cpu.bp == BP0 && cpu.sp == SP0 + 8u && cpu.cs == RET_CS && cpu.ip == RET_IP && cpu.ds == DS && cpu.ax == 0u);
    CHECK((cpu.flags & 0x08D5u) == 0x0044u);                               /* OR AX,[BP-2] on the nil link */
}

static void classes(void)
{
    static const uint32_t xs[] = {0u, 1u, 9u, 10u, 11u, 100u, 12345u, 0x7FFFFFFFu, 0x80000000u, 0xFFFFFFFFu, 0xFFFFFFF6u};
    record members[6];
    unsigned seed = 0x0355u;
    for (unsigned c = 0u; c < 256u; c += (c < 20u ? 1u : 17u))
        for (unsigned s = 15u; s <= 16u; ++s)
            for (unsigned i = 0u; i < sizeof xs / sizeof xs[0]; ++i) {
                record m = {1u, 0u, (uint8_t)c, {(uint8_t)s, (uint8_t)s, (uint8_t)s, (uint8_t)s}, 0x01020304u};
                run_case(&m, 1u, xs[i]);
            }
    for (unsigned n = 0u; n < 2000u; ++n) {
        unsigned count;
        uint32_t x;
        seed = seed * 1103515245u + 12345u; count = (seed >> 16) % 7u;
        seed = seed * 1103515245u + 12345u; x = seed ^ (seed << 13);
        for (unsigned k = 0u; k < 6u; ++k) {
            seed = seed * 1103515245u + 12345u;
            members[k].active = (uint8_t)((seed >> 16) % 3u);
            members[k].state = (uint8_t)((seed >> 18) % 3u);
            members[k].cls = (uint8_t)((seed >> 20) % 4u ? (seed >> 8) % 18u : (seed >> 24));
            for (unsigned j = 0u; j < 4u; ++j) { seed = seed * 1103515245u + 12345u; members[k].stats[j] = (uint8_t)(13u + (seed >> 16) % 6u); }
            seed = seed * 1103515245u + 12345u; members[k].xp = seed;
        }
        run_case(members, count, x);
    }
}

static void stops(void)
{
    record members[3] = {{1u, 0u, 0u, {20u, 0u, 0u, 0u}, 0u}, {1u, 0u, 9u, {0u, 0u, 0u, 0u}, 0u}, {1u, 0u, 3u, {0u, 0u, 0u, 0u}, 0u}};
    pr_u20_0355_cpu cpu;
    int callee = 99;
    cpu = world(members, 3u, 500u, 1);                                     /* a cyclic party: LIMIT at the loop head */
    CHECK(pr_u20_party_award_0355_cpu(ram, RAM, &cpu, 4u, &callee) == PR_U20_0355_LIMIT && callee == 0);
    CHECK(cpu.ip == 0x0368u && cpu.cs == CS);
    cpu = world(members, 1u, 500u, 0);                                     /* the class-0 site rewritten */
    w8(ram, CS, 0x03BBu, 0x90u);
    CHECK(pr_u20_party_award_0355_cpu(ram, RAM, &cpu, CAP, &callee) == PR_U20_0355_CODE && callee == -1);
    CHECK(cpu.ip == 0x03BBu && cpu.ax == 500u && cpu.dx == 0u && cpu.cx == 10u && cpu.bx == 0u && (cpu.flags & 0x08D5u) == 0x0044u);
}

static void arguments(void)
{
    record m = {1u, 0u, 0u, {0u, 0u, 0u, 0u}, 0u};
    pr_u20_0355_cpu cpu = world(&m, 1u, 5u, 0), before = cpu;
    int callee = 99;
    memcpy(want, ram, RAM);
    CHECK(pr_u20_party_award_0355_cpu(NULL, RAM, &cpu, CAP, &callee) == PR_U20_0355_ARGUMENT && callee == -1);
    CHECK(pr_u20_party_award_0355_cpu(ram, RAM, NULL, CAP, NULL) == PR_U20_0355_ARGUMENT);
    CHECK(pr_u20_party_award_0355_cpu(ram, RAM - 1u, &cpu, CAP, NULL) == PR_U20_0355_ARGUMENT);
    CHECK(pr_u20_party_award_0355_cpu(ram, RAM + 1u, &cpu, CAP, NULL) == PR_U20_0355_ARGUMENT);
    CHECK(memcmp(&cpu, &before, sizeof cpu) == 0 && memcmp(ram, want, RAM) == 0);
}

int main(void)
{
    ram = malloc(RAM);
    want = malloc(RAM);
    CHECK(ram != NULL && want != NULL);
    classes();
    stops();
    arguments();
    free(ram);
    free(want);
    printf("u20 +0355: %u checks passed\n", checks);
    return 0;
}
