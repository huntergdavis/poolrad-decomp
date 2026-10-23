#include "poolrad/recovered/overlay_u9e_dispatch_0000_cpu.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Hand-checked cases for the dispatcher model. The differential (tools/overlay/u9e_dispatch_0000_audit.py) is the evidence against the original bytes; this test pins the contract
 * statements it cannot see: REFUSED touches nothing, GAP stops at the LCALL, CODE stops before anything is pushed for the call, CALLEE retains the handler's raw status, ARGUMENT bounds. */
#define DS_ 0x1000u
#define SS_ 0x2000u
#define HOME_CS 0x3000u
#define LOAD_CS 0x5000u
#define S0 0x8000u
static size_t at(uint16_t s, uint16_t o) { return (((size_t)s << 4u) + o) & 0xFFFFFu; }
static void put16(uint8_t *r, uint16_t s, uint16_t o, uint16_t v) { size_t p = at(s, o); r[p] = (uint8_t)v; r[(p + 1u) & 0xFFFFFu] = (uint8_t)(v >> 8u); }
static uint16_t get16(const uint8_t *r, uint16_t s, uint16_t o) { size_t p = at(s, o); return (uint16_t)(r[p] | (r[(p + 1u) & 0xFFFFFu] << 8u)); }
static const pr_u9e_0000_limits LIM = {4096, 4096, 4096, 4096};

/* entry frame: IP 9000h, CS 8000h, then the byte word, three words and the far pointer words, then the index word */
static pr_u9e_0000_cpu setup(uint8_t *r, unsigned idx)
{
    pr_u9e_0000_cpu c; unsigned s;
    memset(&c, 0, sizeof c); memset(r, 0xA5, PR_U9E_0000_RAM_SIZE);
    c.ax = 0xAB00u | 0x11u; c.bx = 0x2222; c.cx = 0x3333; c.dx = 0x4444; c.si = 0x5555; c.di = 0x6666; c.bp = 0x7777; c.sp = S0; c.cs = HOME_CS; c.ip = 0; c.ds = DS_; c.es = 0x8888; c.ss = SS_;
    c.flags = 0x0202;
    put16(r, SS_, S0, 0x9000); put16(r, SS_, S0 + 2, 0x8000); put16(r, SS_, S0 + 4, 0xCC77); put16(r, SS_, S0 + 6, 0x1111); put16(r, SS_, S0 + 8, 0x2222);
    put16(r, SS_, S0 + 10, 0x3333); put16(r, SS_, S0 + 12, 0x4444); put16(r, SS_, S0 + 14, (uint16_t)(0xDD00u | idx));
    for (s = 1; s < 140; ++s) { put16(r, DS_, (uint16_t)(0x6828u + 4u * s), pr_u9e_dispatch_0000_slot_stub_off(s)); put16(r, DS_, (uint16_t)(0x682Au + 4u * s), pr_u9e_dispatch_0000_slot_stub_seg(s)); }
    /* the loaded stub of every raw-composed slot: EA, the handler's offset, the loaded segment */
    for (s = 1; s < 140; ++s) {
        uint16_t seg, off;
        if (pr_u9e_dispatch_0000_slot_kind(s) != 1 && pr_u9e_dispatch_0000_slot_kind(s) != 5) continue;
        seg = pr_u9e_dispatch_0000_slot_stub_seg(s); off = pr_u9e_dispatch_0000_slot_stub_off(s);
        r[at(seg, off)] = 0xEA; put16(r, seg, (uint16_t)(off + 1u), pr_u9e_dispatch_0000_slot_entry(s)); put16(r, seg, (uint16_t)(off + 3u), LOAD_CS);
    }
    return c;
}

int main(void)
{
    uint8_t *r = malloc(PR_U9E_0000_RAM_SIZE), *before = malloc(PR_U9E_0000_RAM_SIZE);
    pr_u9e_0000_cpu c, saved; pr_u9e_0000_diagnostic d; size_t budget, b0; unsigned i;
    assert(r && before);
    /* the table the model was built on: 1..139 are handlers (88 via unit 0x86), the kinds are the documented counts */
    {
        unsigned n[6] = {0, 0, 0, 0, 0, 0};
        for (i = 0; i < 256; ++i) n[pr_u9e_dispatch_0000_slot_kind(i)]++;
        assert(n[0] == 117 && n[1] == 42 && n[2] == 34 && n[3] == 61 && n[4] == 1 && n[5] == 1);
        assert(pr_u9e_dispatch_0000_slot_kind(0) == 0 && pr_u9e_dispatch_0000_slot_kind(140) == 0 && pr_u9e_dispatch_0000_slot_kind(88) == 3 && pr_u9e_dispatch_0000_slot_kind(135) == 4);
        assert(pr_u9e_dispatch_0000_slot_kind(16) == 1 && pr_u9e_dispatch_0000_slot_entry(16) == 0x0647u && pr_u9e_dispatch_0000_slot_stub_off(16) == 0x006Bu && pr_u9e_dispatch_0000_slot_stub_seg(16) == 0x41u);
        assert(pr_u9e_dispatch_0000_slot_stub_seg(88) == 0x96u && pr_u9e_dispatch_0000_slot_stub_off(88) == 0x015Bu && pr_u9e_dispatch_0000_slot_id(88) == 0x008630D9u);
    }
    /* REFUSED: every index outside 1..139 leaves CPU, RAM and budget exactly as they were and calls nothing */
    for (i = 0; i < 256; ++i) {
        if (i >= 1 && i < 140) continue;
        c = setup(r, i); saved = c; memcpy(before, r, PR_U9E_0000_RAM_SIZE); budget = 100000; b0 = budget;
        assert(pr_u9e_dispatch_0000_cpu(r, PR_U9E_0000_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U9E_0000_REFUSED);
        assert(memcmp(&c, &saved, sizeof c) == 0 && memcmp(r, before, PR_U9E_0000_RAM_SIZE) == 0 && budget == b0 && d.slot == (int)i);
    }
    /* GAP: slot 4 (uncredited +017F), slot 2 (semantic +011D), slot 88 (unit 0x86), slot 135 (deferred +3134): stop at the LCALL, the five words pushed, nothing called */
    {
        static const struct { unsigned slot; pr_u9e_0000_gap gap; uint32_t id; } G[] = {
            {4, PR_U9E_0000_GAP_UNCREDITED, 0x0038017Fu}, {2, PR_U9E_0000_GAP_SEMANTIC, 0x0038011Du}, {88, PR_U9E_0000_GAP_UNCREDITED, 0x008630D9u}, {135, PR_U9E_0000_GAP_DEFERRED, 0x00383134u}};
        for (i = 0; i < 4; ++i) {
            c = setup(r, G[i].slot); budget = 100000;
            assert(pr_u9e_dispatch_0000_cpu(r, PR_U9E_0000_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U9E_0000_GAP);
            assert(c.cs == HOME_CS && c.ip == 0x001Eu && c.sp == (uint16_t)(S0 - 2u - 8u - 2u) && c.bp == (uint16_t)(S0 - 2u));
            assert(budget == 100000u - 13u);                                /* 13 instructions executed before the LCALL; the LCALL itself is not charged */
            assert(d.slot == (int)G[i].slot && d.handler_id == G[i].id && d.gap == G[i].gap);
            /* the five words as the handler would see them: AX (AH from the entry AX, AL from [BP+6]), then [BP+8], [BP+0A], [BP+0C], [BP+0E] */
            assert(get16(r, SS_, c.sp) == 0xAB77u && get16(r, SS_, c.sp + 2) == 0x1111u && get16(r, SS_, c.sp + 4) == 0x2222u && get16(r, SS_, c.sp + 6) == 0x3333u && get16(r, SS_, c.sp + 8) == 0x4444u);
            assert(c.di == (uint16_t)(4u * G[i].slot) && c.ax == (uint16_t)G[i].slot);
        }
        /* LIMIT beats GAP when the budget ends at the LCALL */
        c = setup(r, 4); budget = 13;
        assert(pr_u9e_dispatch_0000_cpu(r, PR_U9E_0000_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U9E_0000_LIMIT && c.ip == 0x001Eu && budget == 0);
    }
    /* RETURNED through the NOP handler (slot 16, +0647: PUSH BP; MOV BP,SP; POP BP; RETF 0Ah): SP, BP and CS:IP restored; flags are the second SHL's (pinned: AF set) */
    c = setup(r, 16); budget = 100000;
    assert(pr_u9e_dispatch_0000_cpu(r, PR_U9E_0000_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U9E_0000_RETURNED);
    assert(c.cs == 0x8000u && c.ip == 0x9000u && c.sp == (uint16_t)(S0 + 0x10u) && c.bp == 0x7777u && c.di == 64u && c.ax == 16u);
    assert(c.flags == (uint16_t)((0x0202u & ~0x08D5u) | 0x0010u));          /* 64: CF=0, ZF=0, SF=0, OF=0, PF=0 (one bit set), AF=1 */
    assert(budget == 100000u - 16u);                                          /* 13 before the LCALL + the LCALL + POP BP + RETF = 16 charged; the stub JMP and the handler are free */
    /* ADAPTED slot 1 (+010D: ADD byte [6825],5; INC byte [6822]), hand-checked: [6825]=FEh -> 03h (CF=1, AF=1, ZF=0, SF=0, OF=0, PF=1 since 03h has two bits), then [6822]=7Fh -> 80h
     * (INC keeps CF=1; AF=1, OF=1, SF=1, ZF=0, PF=0 since 80h has one bit): flags = 0202h kept bits | CF 0001 | AF 0010 | SF 0080 | OF 0800 = 0A93h. The handler's PUSH BP writes the dispatcher's BP below its return slot. */
    c = setup(r, 1); r[at(DS_, 0x6825)] = 0xFE; r[at(DS_, 0x6822)] = 0x7F; budget = 100000;
    assert(pr_u9e_dispatch_0000_cpu(r, PR_U9E_0000_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U9E_0000_RETURNED);
    assert(r[at(DS_, 0x6825)] == 0x03u && r[at(DS_, 0x6822)] == 0x80u && c.flags == 0x0A93u && c.bp == 0x7777u);
    assert(c.cs == 0x8000u && c.ip == 0x9000u && c.sp == (uint16_t)(S0 + 0x10u) && c.di == 4u && c.ax == 1u);
    assert(get16(r, SS_, (uint16_t)(S0 - 16u - 2u)) == (uint16_t)(S0 - 2u));   /* the handler's PUSH BP writes the dispatcher's BP (S0-2) below its return slot */
    /* the budget sweep: LIMIT stops at each instruction start in order */
    {
        static const uint16_t IPS[] = {0x0000, 0x0001, 0x0003, 0x0006, 0x0009, 0x000C, 0x000F, 0x0012, 0x0013, 0x0016, 0x0018, 0x001A, 0x001C, 0x001E, 0x0022, 0x0023};
        for (i = 0; i < 16; ++i) {
            c = setup(r, 16); budget = i;
            assert(pr_u9e_dispatch_0000_cpu(r, PR_U9E_0000_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U9E_0000_LIMIT);
            assert(c.ip == IPS[i] && budget == 0 && c.cs == HOME_CS);
        }
    }
    /* CODE: the table word, the stub opcode and the stub's target are checked before the LCALL runs; nothing is pushed for the call */
    c = setup(r, 16); put16(r, DS_, (uint16_t)(0x6828u + 4u * 16u), 0x1234);
    budget = 100000; assert(pr_u9e_dispatch_0000_cpu(r, PR_U9E_0000_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U9E_0000_CODE && c.ip == 0x001Eu && c.sp == (uint16_t)(S0 - 12u));
    c = setup(r, 16); put16(r, DS_, (uint16_t)(0x682Au + 4u * 16u), 0x1234);
    budget = 100000; assert(pr_u9e_dispatch_0000_cpu(r, PR_U9E_0000_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U9E_0000_CODE && c.ip == 0x001Eu);
    c = setup(r, 16); r[at(pr_u9e_dispatch_0000_slot_stub_seg(16), pr_u9e_dispatch_0000_slot_stub_off(16))] = 0xCD;      /* an unloaded stub (INT) */
    budget = 100000; assert(pr_u9e_dispatch_0000_cpu(r, PR_U9E_0000_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U9E_0000_CODE && c.ip == 0x001Eu && budget == 100000u - 13u);
    c = setup(r, 16); put16(r, pr_u9e_dispatch_0000_slot_stub_seg(16), (uint16_t)(pr_u9e_dispatch_0000_slot_stub_off(16) + 1u), 0x0648);   /* a stub that enters another offset */
    budget = 100000; assert(pr_u9e_dispatch_0000_cpu(r, PR_U9E_0000_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U9E_0000_CODE && c.ip == 0x001Eu);
    /* CALLEE: slot 31 (+133F) checks its own LCALL bytes before calling the resident; the RAM here holds no original code, so the composed handler stops (CODE) and its raw status is kept */
    c = setup(r, 31); budget = 100000;
    {
        pr_u9e_0000_status st = pr_u9e_dispatch_0000_cpu(r, PR_U9E_0000_RAM_SIZE, &c, &budget, &LIM, &d);
        assert(pr_u9e_dispatch_0000_slot_entry(31) == 0x133Fu && st == PR_U9E_0000_CALLEE && d.callee_status > 0 && d.slot == 31 && d.handler_id == 0x0038133Fu);
    }
    /* ARGUMENT: null, size, IP and SP bounds change nothing */
    c = setup(r, 16); budget = 100000; saved = c; memcpy(before, r, PR_U9E_0000_RAM_SIZE);
    assert(pr_u9e_dispatch_0000_cpu(NULL, PR_U9E_0000_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U9E_0000_ARGUMENT);
    assert(pr_u9e_dispatch_0000_cpu(r, PR_U9E_0000_RAM_SIZE - 1u, &c, &budget, &LIM, &d) == PR_U9E_0000_ARGUMENT);
    assert(pr_u9e_dispatch_0000_cpu(r, PR_U9E_0000_RAM_SIZE, &c, NULL, &LIM, &d) == PR_U9E_0000_ARGUMENT);
    assert(pr_u9e_dispatch_0000_cpu(r, PR_U9E_0000_RAM_SIZE, &c, &budget, NULL, &d) == PR_U9E_0000_ARGUMENT);
    c.ip = 1; assert(pr_u9e_dispatch_0000_cpu(r, PR_U9E_0000_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U9E_0000_ARGUMENT); c.ip = 0;
    c.sp = 0x01FF; assert(pr_u9e_dispatch_0000_cpu(r, PR_U9E_0000_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U9E_0000_ARGUMENT);
    c.sp = 0xFFEF; assert(pr_u9e_dispatch_0000_cpu(r, PR_U9E_0000_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U9E_0000_ARGUMENT);
    c.sp = S0; assert(memcmp(&c, &saved, sizeof c) == 0 && memcmp(r, before, PR_U9E_0000_RAM_SIZE) == 0 && budget == 100000u);
    free(before); free(r); puts("U9E0000 dispatcher refusal, gap, code, callee, argument and return cases passed");
    return 0;
}
