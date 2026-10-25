#include "poolrad/recovered/overlay_u25_address_class_06fb_cpu.h"
#include "poolrad/recovered/overlay_u25_char_remap_06d3_cpu.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Synthetic state only: neither entry reads code bytes. Frame at 2000:8000: return 8000:9000, then the argument word. */
#define SS_ 0x2000u
#define S0 0x8000u
static size_t at(uint16_t s, uint16_t o) { return (((size_t)s << 4u) + o) & 0xFFFFFu; }
static void put16(uint8_t *r, uint16_t s, uint16_t o, uint16_t v) { size_t p = at(s, o); r[p] = (uint8_t)v; r[(p + 1u) & 0xFFFFFu] = (uint8_t)(v >> 8u); }

static void frame(uint8_t *r, uint16_t sp, uint16_t arg)
{
    memset(r, 0xA5, PR_U25_06D3_RAM_SIZE);
    put16(r, SS_, sp, 0x9000u); put16(r, SS_, (uint16_t)(sp + 2u), 0x8000u); put16(r, SS_, (uint16_t)(sp + 4u), arg);
}

static pr_u25_06d3_status remap(uint8_t *r, uint16_t sp, uint16_t arg, pr_u25_06d3_cpu *c)
{
    frame(r, sp, arg); memset(c, 0, sizeof *c);
    c->cs = 0x6000u; c->ip = 0x06D3u; c->ss = SS_; c->sp = sp; c->bp = 0x7777u; c->ax = 0xAB00u; c->flags = 0x0ED7u;
    return pr_u25_char_remap_06d3_cpu(r, PR_U25_06D3_RAM_SIZE, c);
}

static pr_u25_06fb_status classify(uint8_t *r, uint16_t sp, uint16_t arg, pr_u25_06fb_cpu *c)
{
    frame(r, sp, arg); memset(c, 0, sizeof *c);
    c->cs = 0x6000u; c->ip = 0x06FBu; c->ss = SS_; c->sp = sp; c->bp = 0x7777u; c->ax = 0xAB00u; c->flags = 0x0202u;
    return pr_u25_address_class_06fb_cpu(r, PR_U25_06FB_RAM_SIZE, c);
}

int main(void)
{
    uint8_t *r = malloc(PR_U25_06D3_RAM_SIZE), *before = malloc(PR_U25_06D3_RAM_SIZE); pr_u25_06d3_cpu a, saved_a; pr_u25_06fb_cpu b, saved_b;
    assert(r && before);
    /* 06D3, codes up to 1Fh: AX := code + 40h (AH cleared); ADD's flags reach the RETF */
    assert(remap(r, S0, 0x5500u, &a) == PR_U25_06D3_RETURNED); assert(a.ax == 0x0040u && (a.flags & 0x08D5u) == 0x0000u);
    assert(a.sp == S0 + 6u && a.bp == 0x7777u && a.cs == 0x8000u && a.ip == 0x9000u && r[at(SS_, (uint16_t)(S0 - 3u))] == 0x40u);
    assert(remap(r, S0, 0x001Fu, &a) == PR_U25_06D3_RETURNED); assert(a.ax == 0x005Fu && (a.flags & 0x08D5u) == 0x0004u);   /* 5Fh: even parity */
    /* codes above 1Fh are kept, AH kept; CMP code,1Fh's flags reach the RETF (20h-1Fh = 1: AF; FFh-1Fh = E0h: SF) */
    assert(remap(r, S0, 0x0020u, &a) == PR_U25_06D3_RETURNED); assert(a.ax == 0xAB20u && (a.flags & 0x08D5u) == 0x0010u);
    assert(remap(r, S0, 0x12FFu, &a) == PR_U25_06D3_RETURNED); assert(a.ax == 0xABFFu && (a.flags & 0x08D5u) == 0x0080u);
    /* 06FB: the four ranges and the default; the last CMP executed reaches the RETF */
    assert(classify(r, S0, 0x4900u, &b) == PR_U25_06FB_RETURNED); assert(b.ax == 0xAB00u && (b.flags & 0x08D5u) == 0x0885u);
    assert(classify(r, S0, 0x48FFu, &b) == PR_U25_06FB_RETURNED); assert((b.ax & 0xFFu) == 4u);
    assert(classify(r, S0, 0x6EFFu, &b) == PR_U25_06FB_RETURNED); assert((b.ax & 0xFFu) == 1u);
    assert(classify(r, S0, 0x98FFu, &b) == PR_U25_06FB_RETURNED); assert((b.ax & 0xFFu) == 2u);
    assert(classify(r, S0, 0x9900u, &b) == PR_U25_06FB_RETURNED); assert((b.ax & 0xFFu) == 3u);
    assert(classify(r, S0, 0xB6FFu, &b) == PR_U25_06FB_RETURNED); assert((b.ax & 0xFFu) == 3u && (b.flags & 0x08D5u) == 0x0044u);
    assert(classify(r, S0, 0xC000u, &b) == PR_U25_06FB_RETURNED); assert(b.ax == 0xAB04u && (b.flags & 0x08D5u) == 0x0010u);
    assert(b.sp == S0 + 6u && b.bp == 0x7777u && b.cs == 0x8000u && b.ip == 0x9000u);
    /* the SP edges the guards allow */
    assert(remap(r, 0x0004u, 0x0001u, &a) == PR_U25_06D3_RETURNED && a.ax == 0x0041u);
    assert(remap(r, 0xFFFAu, 0x0001u, &a) == PR_U25_06D3_RETURNED && a.ax == 0x0041u && a.sp == 0x0000u);
    assert(classify(r, 0xFFFAu, 0x6B00u, &b) == PR_U25_06FB_RETURNED && (b.ax & 0xFFu) == 1u);
    /* argument rejection is atomic */
    (void)remap(r, S0, 1u, &a); frame(r, S0, 1u); memcpy(before, r, PR_U25_06D3_RAM_SIZE);
    a.ip = 0x06D3u; a.sp = 0x0003u; saved_a = a; assert(pr_u25_char_remap_06d3_cpu(r, PR_U25_06D3_RAM_SIZE, &a) == PR_U25_06D3_ARGUMENT);
    a.sp = 0xFFFBu; saved_a = a; assert(pr_u25_char_remap_06d3_cpu(r, PR_U25_06D3_RAM_SIZE, &a) == PR_U25_06D3_ARGUMENT);
    a.sp = S0; a.ip = 0x06D4u; saved_a = a; assert(pr_u25_char_remap_06d3_cpu(r, PR_U25_06D3_RAM_SIZE, &a) == PR_U25_06D3_ARGUMENT);
    assert(!memcmp(&a, &saved_a, sizeof a) && !memcmp(r, before, PR_U25_06D3_RAM_SIZE)); a.ip = 0x06D3u;
    assert(pr_u25_char_remap_06d3_cpu(NULL, PR_U25_06D3_RAM_SIZE, &a) == PR_U25_06D3_ARGUMENT);
    assert(pr_u25_char_remap_06d3_cpu(r, 0, &a) == PR_U25_06D3_ARGUMENT && pr_u25_char_remap_06d3_cpu(r, PR_U25_06D3_RAM_SIZE, NULL) == PR_U25_06D3_ARGUMENT);
    (void)classify(r, S0, 1u, &b); frame(r, S0, 1u); memcpy(before, r, PR_U25_06FB_RAM_SIZE);
    b.ip = 0x06FBu; b.sp = 0x0003u; saved_b = b; assert(pr_u25_address_class_06fb_cpu(r, PR_U25_06FB_RAM_SIZE, &b) == PR_U25_06FB_ARGUMENT);
    b.sp = 0xFFFBu; saved_b = b; assert(pr_u25_address_class_06fb_cpu(r, PR_U25_06FB_RAM_SIZE, &b) == PR_U25_06FB_ARGUMENT);
    b.sp = S0; b.ip = 0x06FCu; saved_b = b; assert(pr_u25_address_class_06fb_cpu(r, PR_U25_06FB_RAM_SIZE, &b) == PR_U25_06FB_ARGUMENT);
    assert(!memcmp(&b, &saved_b, sizeof b) && !memcmp(r, before, PR_U25_06FB_RAM_SIZE)); b.ip = 0x06FBu;
    assert(pr_u25_address_class_06fb_cpu(NULL, PR_U25_06FB_RAM_SIZE, &b) == PR_U25_06FB_ARGUMENT);
    assert(pr_u25_address_class_06fb_cpu(r, 0, &b) == PR_U25_06FB_ARGUMENT && pr_u25_address_class_06fb_cpu(r, PR_U25_06FB_RAM_SIZE, NULL) == PR_U25_06FB_ARGUMENT);
    free(before); free(r); puts("U25 06D3 remap and 06FB classifier: rules, flags, SP edges and guard cases passed"); return 0;
}
