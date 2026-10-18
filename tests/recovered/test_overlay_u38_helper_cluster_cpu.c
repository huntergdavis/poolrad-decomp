/* Focused synthetic test for the raw-CPU U38 helper cluster (docs/re/U38_HELPER_CLUSTER_CPU.md). No game bytes. */
#include "poolrad/recovered/overlay_u38_helper_cluster_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

_Static_assert(sizeof(pr_u38c_cpu) == 28, "all fourteen CPU words");

#define RAM PR_U38C_RAM_SIZE
#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "u38 cluster line %d: %s\n", __LINE__, #x);          \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { CS = 0x3000, DS = 0x1000, SS = 0x2000, SP0 = 0x8000 };

static uint8_t *ram;
static unsigned checks;

static size_t at(unsigned seg, unsigned off) { return (((size_t)seg << 4) + (off & 0xFFFFu)) & (RAM - 1u); }
static void w8(unsigned seg, unsigned off, unsigned v) { ram[at(seg, off)] = (uint8_t)v; }
static uint8_t r8(unsigned seg, unsigned off) { return ram[at(seg, off)]; }
static void w16(unsigned seg, unsigned off, unsigned v) { w8(seg, off, v & 0xFFu); w8(seg, off + 1u, v >> 8); }
static unsigned r16(unsigned seg, unsigned off) { return r8(seg, off) | (unsigned)r8(seg, off + 1u) << 8; }

/* DS bytes 6816/6817/6818/6819/681C/682A and a far frame at SS:SP0 (key word for the helper, else 5 words). */
static pr_u38c_cpu world(unsigned ip, unsigned b16, unsigned b17, unsigned b18, unsigned b19, unsigned b1c, unsigned b2a,
                         unsigned key)
{
    pr_u38c_cpu cpu = {0xA500u, 0x2222u, 0x3333u, 0x4444u, 0x5555u, 0x6666u, 0x7777u, SP0, CS, 0, DS, 0x9999u, SS, 0x0202u};
    unsigned i;
    memset(ram, 0x5C, RAM);
    w8(DS, 0x6816u, b16); w8(DS, 0x6817u, b17); w8(DS, 0x6818u, b18);
    w8(DS, 0x6819u, b19); w8(DS, 0x681Cu, b1c); w8(DS, 0x682Au, b2a);
    w16(SS, SP0, 0x7000u);
    w16(SS, SP0 + 2u, 0x7001u);
    if (ip == 0u)
        w16(SS, SP0 + 4u, 0xC300u | key);
    else
        for (i = 0; i < 5u; ++i) w16(SS, SP0 + 4u + 2u * i, 0x1111u * (i + 1u));
    cpu.ip = (uint16_t)ip;
    return cpu;
}

static void returned(const pr_u38c_cpu *cpu, unsigned pops)
{
    CHECK(cpu->ip == 0x7000u && cpu->cs == 0x7001u && cpu->sp == SP0 + 4u + pops && cpu->bp == 0x7777u);
    CHECK(cpu->ds == DS && cpu->ss == SS && cpu->bx == 0x2222u && cpu->es == 0x9999u);
}

/* The helper as seen from an entry: the pushed key word sits at SP0 - 4 (AH kept), the return frame below it. */
static void helper_frame(unsigned key, unsigned return_ip)
{
    CHECK(r16(SS, SP0 - 4u) == (0xA500u | key));
    CHECK(r16(SS, SP0 - 6u) == CS && r16(SS, SP0 - 8u) == return_ip && r16(SS, SP0 - 10u) == SP0 - 2u);
}

int main(void)
{
    pr_u38c_cpu cpu;
    ram = malloc(RAM);
    CHECK(ram != NULL);

    /* Helper: key 0 clears 6817/6818 with CMP 0,0's flags; key = [6817] clears with CMP k,k's; otherwise only AL changes. */
    cpu = world(0u, 1u, 0x34u, 0x77u, 0u, 0u, 0u, 0u);
    CHECK(pr_u38_helper_0000_cpu(ram, RAM, &cpu) == PR_U38C_OK);
    CHECK(r8(DS, 0x6817u) == 0u && r8(DS, 0x6818u) == 0u && cpu.ax == 0xA500u && (cpu.flags & 0x08D5u) == 0x0044u);
    returned(&cpu, 2u);
    cpu = world(0u, 1u, 0x34u, 0x77u, 0u, 0u, 0u, 0x34u);
    CHECK(pr_u38_helper_0000_cpu(ram, RAM, &cpu) == PR_U38C_OK);
    CHECK(r8(DS, 0x6817u) == 0u && r8(DS, 0x6818u) == 0u && cpu.ax == 0xA534u && (cpu.flags & 0x08D5u) == 0x0044u);
    returned(&cpu, 2u);
    cpu = world(0u, 1u, 0x35u, 0x77u, 0u, 0u, 0u, 0x34u);
    CHECK(pr_u38_helper_0000_cpu(ram, RAM, &cpu) == PR_U38C_OK);
    CHECK(r8(DS, 0x6817u) == 0x35u && r8(DS, 0x6818u) == 0x77u && cpu.ax == 0xA535u && (cpu.flags & 0x08D5u) == 0x0000u);
    returned(&cpu, 2u);

    /* Entry 93 (+29CB): helper(34h). */
    cpu = world(0x29CBu, 1u, 0x34u, 0x77u, 0u, 0u, 0u, 0u);
    CHECK(pr_u38_entry93_29cb_cpu(ram, RAM, &cpu) == PR_U38C_OK);
    CHECK(r8(DS, 0x6817u) == 0u && r8(DS, 0x6818u) == 0u);
    helper_frame(0x34u, 0x29D5u);
    returned(&cpu, 0x0Au);

    /* Entry 92 (+29B6): helper(0Bh) clears when [6817] = 0Bh; helper(35h) then sees 0 and only loads AL. */
    cpu = world(0x29B6u, 1u, 0x0Bu, 0x77u, 0u, 0u, 0u, 0u);
    CHECK(pr_u38_entry92_29b6_cpu(ram, RAM, &cpu) == PR_U38C_OK);
    CHECK(r8(DS, 0x6817u) == 0u && r8(DS, 0x6818u) == 0u && cpu.ax == 0xA500u && (cpu.flags & 0x0001u));
    helper_frame(0x35u, 0x29C7u);
    returned(&cpu, 0x0Au);

    /* Entries 94 (bit 1) and 96 (bit 0) call helper(0) only when their [6819] bit is set. */
    cpu = world(0x29D9u, 1u, 0x34u, 0x77u, 0x02u, 0u, 0u, 0u);
    CHECK(pr_u38_entry94_29d9_cpu(ram, RAM, &cpu) == PR_U38C_OK && r8(DS, 0x6817u) == 0u && r8(DS, 0x6818u) == 0u);
    returned(&cpu, 0x0Au);
    cpu = world(0x29D9u, 1u, 0x34u, 0x77u, 0xFDu, 0u, 0u, 0u);
    CHECK(pr_u38_entry94_29d9_cpu(ram, RAM, &cpu) == PR_U38C_OK && r8(DS, 0x6817u) == 0x34u && cpu.ax == 0xA500u);
    CHECK((cpu.flags & 0x08D5u) == 0x0044u);                  /* AND/OR result 0: ZF, PF */
    returned(&cpu, 0x0Au);
    cpu = world(0x2A11u, 1u, 0x34u, 0x77u, 0x01u, 0u, 0u, 0u);
    CHECK(pr_u38_entry96_2a11_cpu(ram, RAM, &cpu) == PR_U38C_OK && r8(DS, 0x6817u) == 0u);
    returned(&cpu, 0x0Au);
    cpu = world(0x2A11u, 1u, 0x34u, 0x77u, 0xFEu, 0u, 0u, 0u);
    CHECK(pr_u38_entry96_2a11_cpu(ram, RAM, &cpu) == PR_U38C_OK && r8(DS, 0x6817u) == 0x34u);
    returned(&cpu, 0x0Au);

    /* Entry 95 (+29F0): helper(37h), helper(34h), then [6816] := 64h when [682A] is 0. */
    cpu = world(0x29F0u, 1u, 0x34u, 0x77u, 0u, 0u, 0u, 0u);
    CHECK(pr_u38_entry95_29f0_cpu(ram, RAM, &cpu) == PR_U38C_OK);
    CHECK(r8(DS, 0x6817u) == 0u && r8(DS, 0x6816u) == 0x64u && (cpu.flags & 0x0040u));
    returned(&cpu, 0x0Au);
    cpu = world(0x29F0u, 1u, 0x34u, 0x77u, 0u, 0u, 5u, 0u);
    CHECK(pr_u38_entry95_29f0_cpu(ram, RAM, &cpu) == PR_U38C_OK && r8(DS, 0x6816u) == 1u && !(cpu.flags & 0x0040u));
    returned(&cpu, 0x0Au);

    /* Entry 50 (+1490) against a reference: n passes of [6818] -= 2, raised to [681C] when below it; [6816] += 4;
     * helper(0) unless [6819] bit 3. */
    for (unsigned n = 0; n < 6u; ++n) {
        for (unsigned v = 0; v < 256u; v += 37u) {
            for (unsigned bits = 1u; bits <= 9u; bits += 8u) {
                unsigned want = v, pass;
                cpu = world(0x1490u, 0x10u, 0x34u, v, bits, n, 0u, 0u);
                for (pass = 0; pass < n; ++pass) {
                    want = (want - 2u) & 0xFFu;
                    if (want < n) want = n;
                }
                if (bits & 8u) {
                    CHECK(pr_u38_entry50_1490_cpu(ram, RAM, &cpu, 254u) == PR_U38C_OK);
                    CHECK(r8(DS, 0x6818u) == want && r8(DS, 0x6817u) == 0x34u && (cpu.flags & 0x0040u) == 0u);
                } else {
                    CHECK(pr_u38_entry50_1490_cpu(ram, RAM, &cpu, 254u) == PR_U38C_OK);
                    CHECK(r8(DS, 0x6818u) == 0u && r8(DS, 0x6817u) == 0u);
                }
                CHECK(r8(DS, 0x6816u) == 0x14u);
                if (n != 0u)
                    CHECK(r8(SS, SP0 - 4u) == n && r8(SS, SP0 - 3u) == n);   /* [BP-2] count, [BP-1] counter */
                returned(&cpu, 0x0Au);
            }
        }
    }
    cpu = world(0x1490u, 0x10u, 0x34u, 0x77u, 0xFEu, 3u, 0u, 0u);   /* bit 0 clear: nothing but the frame */
    CHECK(pr_u38_entry50_1490_cpu(ram, RAM, &cpu, 254u) == PR_U38C_OK && r8(DS, 0x6816u) == 0x10u && r8(DS, 0x6818u) == 0x77u);
    returned(&cpu, 0x0Au);

    /* SS = DS with [BP-1] on DS:6818 (entry SP 681B; BP low byte 01h lands on DS:6819): about 128 loop-backs, so
     * a cap of 50 stops before the INC at +14B1 with SP = BP - 2. */
    cpu = world(0x1490u, 0x10u, 0x34u, 0x77u, 0u, 0x80u, 0u, 0u);
    cpu.ss = DS; cpu.sp = 0x681Bu; cpu.bp = 0x7701u;
    w16(DS, 0x681Bu, 0x7000u); w16(DS, 0x681Du, 0x7001u);
    CHECK(pr_u38_entry50_1490_cpu(ram, RAM, &cpu, 50u) == PR_U38C_LIMIT);
    CHECK(cpu.ip == 0x14B1u && cpu.cs == CS && cpu.bp == 0x6819u && cpu.sp == 0x6817u);

    /* SS = DS with entry SP 6820: the helper's pushes land on DS:6816..6819 and its clears zero the high byte of its
     * own return IP, so its RETF leaves for CS:00E5 and the entry stops with TRANSFER. */
    cpu = world(0x1490u, 0x10u, 0x34u, 0x77u, 0x01u, 3u, 0u, 0u);
    cpu.ss = DS; cpu.sp = 0x6820u;
    w16(DS, 0x6820u, 0x7000u); w16(DS, 0x6822u, 0x7001u);
    CHECK(pr_u38_entry50_1490_cpu(ram, RAM, &cpu, 254u) == PR_U38C_TRANSFER);
    CHECK(cpu.cs == CS && cpu.ip == 0x00E5u);

    /* Argument rejections. */
    cpu = world(0x29CBu, 1u, 0x34u, 0x77u, 0u, 0u, 0u, 0u);
    CHECK(pr_u38_entry93_29cb_cpu(NULL, RAM, &cpu) == PR_U38C_ARGUMENT);
    CHECK(pr_u38_helper_0000_cpu(ram, RAM - 1u, &cpu) == PR_U38C_ARGUMENT);
    CHECK(pr_u38_entry50_1490_cpu(ram, RAM, NULL, 5u) == PR_U38C_ARGUMENT);
    CHECK(cpu.sp == SP0 && cpu.ip == 0x29CBu && r8(DS, 0x6817u) == 0x34u);

    free(ram);
    printf("PASS: u38 helper cluster, %u checks (helper, entries 50/92/93/94/95/96, LIMIT, TRANSFER, arguments)\n", checks);
    return 0;
}
