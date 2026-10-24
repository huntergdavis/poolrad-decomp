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
        assert(n[0] == 117 && n[1] == 42 && n[2] == 14 && n[3] == 61 && n[4] == 1 && n[5] == 21);
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
    /* GAP: slot 4 (uncredited +017F), slot 6 (semantic +01C3), slot 88 (unit 0x86), slot 135 (deferred +3134): stop at the LCALL, the five words pushed, nothing called */
    {
        static const struct { unsigned slot; pr_u9e_0000_gap gap; uint32_t id; } G[] = {
            {4, PR_U9E_0000_GAP_UNCREDITED, 0x0038017Fu}, {6, PR_U9E_0000_GAP_SEMANTIC, 0x003801C3u}, {88, PR_U9E_0000_GAP_UNCREDITED, 0x008630D9u}, {135, PR_U9E_0000_GAP_DEFERRED, 0x00383134u}};
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
    /* ADAPTED batch 1, hand-checked (flags start 0202h; PF set for an even number of ones in the low byte; each handler's last flag writer is the only one that reaches the return):
     *  slot 2 +011D  [6825]=03h < 5 -> CF=1, JAE not taken: [6825]=00h; DEC [6822]=01h -> 00h keeps CF, ZF=1, PF=1, AF=0 (a=1): 0202|CF 1|PF 4|ZF 40h = 0247h; AX = 0002h (the dispatcher's XOR AH,AH leaves AX = the slot number, AL from [BP+6])
     *  slot 36 +0C15 [6822]=04h,[6816]=03h: SUB [6822],4 -> 00h (dead flags), SUB [6816],4 -> FFh: CF=1, AF=1 (03h^04h^FFh has bit 4), SF=1, OF=0, ZF=0, PF=1 (8 ones): 0202|1|4|10h|80h = 0297h
     *  slot 93 +25A2 [6819]=01h (mask 01h hit), [6818]=83h: AX = 0041h, [6818]=41h; SHR AX,1: CF=1, pinned AF=1, PF=1 (41h has two ones), ZF=0, SF=0, OF=0 (AH was cleared): 0202|1|4|10h = 0217h
     *  slot 93 +25A2 [6819]=02h (mask clear): AL=00h only (AX=0000h), OR AL,AL: ZF=1, PF=1, CF=OF=AF=0: 0246h; [6818] untouched
     *  slot 42 +10C1 [681A]=02h: AX=0001h, [681A]=01h; SHR: CF=0, AF=1, PF=0 (01h has one one), ZF=0: 0212h */
    c = setup(r, 2); r[at(DS_, 0x6825)] = 0x03; r[at(DS_, 0x6822)] = 0x01; budget = 100000;
    assert(pr_u9e_dispatch_0000_cpu(r, PR_U9E_0000_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U9E_0000_RETURNED);
    assert(r[at(DS_, 0x6825)] == 0x00u && r[at(DS_, 0x6822)] == 0x00u && c.flags == 0x0247u && c.ax == 0x0002u && c.sp == (uint16_t)(S0 + 0x10u) && c.cs == 0x8000u && c.ip == 0x9000u);
    c = setup(r, 36); r[at(DS_, 0x6822)] = 0x04; r[at(DS_, 0x6816)] = 0x03; budget = 100000;
    assert(pr_u9e_dispatch_0000_cpu(r, PR_U9E_0000_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U9E_0000_RETURNED);
    assert(r[at(DS_, 0x6822)] == 0x00u && r[at(DS_, 0x6816)] == 0xFFu && c.flags == 0x0297u);
    c = setup(r, 93); r[at(DS_, 0x6819)] = 0x01; r[at(DS_, 0x6818)] = 0x83; budget = 100000;
    assert(pr_u9e_dispatch_0000_cpu(r, PR_U9E_0000_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U9E_0000_RETURNED);
    assert(r[at(DS_, 0x6818)] == 0x41u && c.ax == 0x0041u && c.flags == 0x0217u);
    c = setup(r, 93); r[at(DS_, 0x6819)] = 0x02; r[at(DS_, 0x6818)] = 0x83; budget = 100000;
    assert(pr_u9e_dispatch_0000_cpu(r, PR_U9E_0000_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U9E_0000_RETURNED);
    assert(r[at(DS_, 0x6818)] == 0x83u && c.ax == 0x0000u && c.flags == 0x0246u);
    c = setup(r, 42); r[at(DS_, 0x681A)] = 0x02; budget = 100000;
    assert(pr_u9e_dispatch_0000_cpu(r, PR_U9E_0000_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U9E_0000_RETURNED);
    assert(r[at(DS_, 0x681A)] == 0x01u && c.ax == 0x0001u && c.flags == 0x0212u);
    /* ADAPTED batch S, hand-checked (flags start 0202h; AX = the slot number at entry, AH = 0):
     *  slot 10 +03E1 [6819]=02h (mask hit), [6818]=83h, [6816]=FEh: AX = 0041h, [6818]=41h, [6816]=01h; the ADD is the last flag writer: FEh+3 = 01h carry, AF=1 (bit 4 of FEh^03h^01h), OF=0, PF=0 (one one) -> 0202|1|10h = 0213h
     *  slot 10 +03E1 [6819]=01h (mask clear): AX = 0000h (AL = the masked 0), OR AL,AL: ZF, PF -> 0246h, [6818] and [6816] untouched
     *  slot 20 +06E4 [6819]=01h, [6818]=81h, [6816]=7Fh: AX = 0040h, [6818]=40h, [6816]=82h; ADD 7Fh+3: CF=0, AF=1, SF=1, OF=1 (positive+positive = negative), PF=1 (82h has two ones) -> 0202|10h|80h|800h|4 = 0A96h
     *  slot 29 +0A3D [6818]=80h: DX = 20h, AX = 60h, [6818]=60h; SUB AX,DX 0080h-0020h: CF=0, AF=0, ZF=0, SF=0, OF=0, PF=1 (60h has two ones) -> 0206h
     *  slot 113 +2A28 [6819]=01h, [681C]=03h, [6818]=10h: three iterations (10h -> 0Dh, never below 03h), locals [BP-2]=03h and [BP-1]=03h (S0-20, S0-19 here), AX = 0003h, CMP 3,3: ZF, PF -> 0246h
     *  slot 113 +2A28 [6819]=01h, [681C]=00h: JA at once, AL=1, [BP-2]=00h, [BP-1] untouched (A5h), CMP 1,0 -> 0202h (no flag set: PF is clear, one one in 01h) */
    c = setup(r, 10); r[at(DS_, 0x6819)] = 0x02; r[at(DS_, 0x6818)] = 0x83; r[at(DS_, 0x6816)] = 0xFE; budget = 100000;
    assert(pr_u9e_dispatch_0000_cpu(r, PR_U9E_0000_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U9E_0000_RETURNED);
    assert(r[at(DS_, 0x6818)] == 0x41u && r[at(DS_, 0x6816)] == 0x01u && c.ax == 0x0041u && c.flags == 0x0213u);
    c = setup(r, 10); r[at(DS_, 0x6819)] = 0x01; r[at(DS_, 0x6818)] = 0x83; r[at(DS_, 0x6816)] = 0xFE; budget = 100000;
    assert(pr_u9e_dispatch_0000_cpu(r, PR_U9E_0000_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U9E_0000_RETURNED);
    assert(r[at(DS_, 0x6818)] == 0x83u && r[at(DS_, 0x6816)] == 0xFEu && c.ax == 0x0000u && c.flags == 0x0246u);
    c = setup(r, 20); r[at(DS_, 0x6819)] = 0x01; r[at(DS_, 0x6818)] = 0x81; r[at(DS_, 0x6816)] = 0x7F; budget = 100000;
    assert(pr_u9e_dispatch_0000_cpu(r, PR_U9E_0000_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U9E_0000_RETURNED);
    assert(r[at(DS_, 0x6818)] == 0x40u && r[at(DS_, 0x6816)] == 0x82u && c.ax == 0x0040u && c.flags == 0x0A96u);
    c = setup(r, 29); r[at(DS_, 0x6818)] = 0x80; budget = 100000;
    assert(pr_u9e_dispatch_0000_cpu(r, PR_U9E_0000_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U9E_0000_RETURNED);
    assert(r[at(DS_, 0x6818)] == 0x60u && c.ax == 0x0060u && c.dx == 0x0020u && c.flags == 0x0206u);
    c = setup(r, 113); r[at(DS_, 0x6819)] = 0x01; r[at(DS_, 0x681C)] = 0x03; r[at(DS_, 0x6818)] = 0x10; budget = 100000;
    assert(pr_u9e_dispatch_0000_cpu(r, PR_U9E_0000_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U9E_0000_RETURNED);
    assert(r[at(DS_, 0x6818)] == 0x0Du && c.ax == 0x0003u && c.flags == 0x0246u && r[at(SS_, (uint16_t)(S0 - 20u))] == 0x03u && r[at(SS_, (uint16_t)(S0 - 19u))] == 0x03u);
    c = setup(r, 113); r[at(DS_, 0x6819)] = 0x01; r[at(DS_, 0x681C)] = 0x00; r[at(DS_, 0x6818)] = 0x10; budget = 100000;
    assert(pr_u9e_dispatch_0000_cpu(r, PR_U9E_0000_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U9E_0000_RETURNED);
    assert(r[at(DS_, 0x6818)] == 0x10u && c.ax == 0x0001u && c.flags == 0x0202u && r[at(SS_, (uint16_t)(S0 - 20u))] == 0x00u && r[at(SS_, (uint16_t)(S0 - 19u))] == 0xA5u);
    /* ADAPTED batch P1 (pointer handlers), hand-checked. The frame words are [BP+6] byte, [BP+8]/[BP+0A] one far pointer (offset, segment), [BP+0C]/[BP+0E] the record far pointer; AX at entry = the slot number.
     * PW(seg, off, k, v) writes ES:[off+k] with the original's 16-bit offset wrap. Flags start 0202h.
     *  slot 14 +05D6 source 6C00:0200 [3]=9Ch, record 6000:0100: record[15h] = 9Ch, AL = 9Ch, ES:DI = 6000:0100, no flag writer: FLAGS stay as the dispatcher's second SHL left them (0212h: AF set by the pinned shift rule)
     *  slot 33 +0BAC record 6000:FEFF (record[111h], [112h] wrap to 6000:0010/0011) = 03h/80h, [6822]=00h, [6816]=04h: FFh/7Ch, [6822]=FCh, [6816]=00h, the last SUB 4-4: ZF, PF -> 0246h
     *  slot 3 +013B P = 6400:0300, Q = 6800:0400 (P[0Ah..0Dh]), Q[9Fh]=4, [6822]=[6818]=FEh: both become 00h, ADD FEh+2: CF, AF (bit 4 of FEh^02h^00h), ZF, PF -> 0257h; AX = 0400h, DX = 6800h, ES:DI = 6800:0400, scratch [6826:6828] = Q
     *  slot 98 +2700 record[11Bh]=FEh, [32h]=20h: FEh+3 = 01h <= 20h keeps 01h; CMP 01h,20h = E1h: CF, SF, PF -> 0287h, AL = 01h. Second: 30h+3 = 33h > 20h clamps to 20h; CMP 33h,20h = 13h (odd parity): flags 0202h, AL = 20h
     *  slot 89 +2490 source 6C00:FFFE (source[3] wraps to 6C00:0001): 35h, [6DE3]=0,[6822]=0: source[3] = 05h = AL, AND flags (PF) -> 0206h; with [6822]=FFh bit 4 set: AL = 10h, flags 0202h, nothing stored; source[3]=25h (bit 4 clear): [6822]=FFh, 35h stored, AL = 35h, flags 0206h
     *  slot 17 +064E record[111h]=20h -> 39h, [6816]=FFh -> 00h, [681B]=0Fh clears [6818]; CMP 0Fh,0Fh: ZF, PF -> 0246h
     *  slot 37 +0C26 record[108h] -> P = 6400:0300, P[3]=05h > 0: [681E]=01h, [6822]=FFh; CMP 05h,0: PF -> 0206h; ES:DI = P
     *  slot 58 +1452 record[108h] -> P = 6400:FFFC (P[6] wraps to 6400:0002 = 77h -> 00h), [681D]=01h clears [681A]; CMP 01h,0: flags 0202h; ES:DI = P */
#define PW(seg, off, k, v) (r[at((seg), (uint16_t)((off) + (k)))] = (uint8_t)(v))
#define FRAME(sofs, sseg, rofs, rseg) do { put16(r, SS_, (uint16_t)(S0 + 6u), (sofs)); put16(r, SS_, (uint16_t)(S0 + 8u), (sseg)); put16(r, SS_, (uint16_t)(S0 + 10u), (rofs)); put16(r, SS_, (uint16_t)(S0 + 12u), (rseg)); } while (0)
#define RUN() do { budget = 100000; assert(pr_u9e_dispatch_0000_cpu(r, PR_U9E_0000_RAM_SIZE, &c, &budget, &LIM, &d) == PR_U9E_0000_RETURNED); } while (0)
    c = setup(r, 14); FRAME(0x0200, 0x6C00, 0x0100, 0x6000); PW(0x6C00, 0x0200, 3, 0x9C); RUN();
    assert(r[at(0x6000, 0x0115)] == 0x9Cu && c.ax == 0x009Cu && c.es == 0x6000u && c.di == 0x0100u && c.flags == 0x0212u);
    c = setup(r, 33); FRAME(0x0100, 0x6C00, 0xFEFF, 0x6000); PW(0x6000, 0xFEFF, 0x111, 0x03); PW(0x6000, 0xFEFF, 0x112, 0x80); r[at(DS_, 0x6822)] = 0x00; r[at(DS_, 0x6816)] = 0x04; RUN();
    assert(r[at(0x6000, 0x0010)] == 0xFFu && r[at(0x6000, 0x0011)] == 0x7Cu && r[at(DS_, 0x6822)] == 0xFCu && r[at(DS_, 0x6816)] == 0x00u && c.flags == 0x0246u && c.es == 0x6000u && c.di == 0xFEFFu && c.ax == 0x0021u);
    c = setup(r, 3); FRAME(0x0100, 0x6C00, 0x0100, 0x6000);
    PW(0x6000, 0x0100, 0x108, 0x00); PW(0x6000, 0x0100, 0x109, 0x03); PW(0x6000, 0x0100, 0x10A, 0x00); PW(0x6000, 0x0100, 0x10B, 0x64);
    PW(0x6400, 0x0300, 0xA, 0x00); PW(0x6400, 0x0300, 0xB, 0x04); PW(0x6400, 0x0300, 0xC, 0x00); PW(0x6400, 0x0300, 0xD, 0x68); PW(0x6800, 0x0400, 0x9F, 0x04);
    r[at(DS_, 0x6822)] = 0xFE; r[at(DS_, 0x6818)] = 0xFE; RUN();
    assert(r[at(DS_, 0x6822)] == 0x00u && r[at(DS_, 0x6818)] == 0x00u && c.flags == 0x0257u && c.ax == 0x0400u && c.dx == 0x6800u && c.es == 0x6800u && c.di == 0x0400u && get16(r, DS_, 0x6826u) == 0x0400u && get16(r, DS_, 0x6828u) == 0x6800u);
    c = setup(r, 98); FRAME(0x0100, 0x6C00, 0x0100, 0x6000); PW(0x6000, 0x0100, 0x11B, 0xFE); PW(0x6000, 0x0100, 0x32, 0x20); RUN();
    assert(r[at(0x6000, 0x021B)] == 0x01u && c.ax == 0x0001u && c.flags == 0x0287u);
    c = setup(r, 98); FRAME(0x0100, 0x6C00, 0x0100, 0x6000); PW(0x6000, 0x0100, 0x11B, 0x30); PW(0x6000, 0x0100, 0x32, 0x20); RUN();
    assert(r[at(0x6000, 0x021B)] == 0x20u && c.ax == 0x0020u && c.flags == 0x0202u);
    c = setup(r, 89); FRAME(0xFFFE, 0x6C00, 0x0100, 0x6000); PW(0x6C00, 0xFFFE, 3, 0x35); r[at(DS_, 0x6DE3)] = 0; r[at(DS_, 0x6822)] = 0; RUN();
    assert(r[at(0x6C00, 0x0001)] == 0x05u && c.ax == 0x0005u && c.flags == 0x0206u && c.es == 0x6C00u && c.di == 0xFFFEu);
    c = setup(r, 89); FRAME(0xFFFE, 0x6C00, 0x0100, 0x6000); PW(0x6C00, 0xFFFE, 3, 0x35); r[at(DS_, 0x6DE3)] = 0; r[at(DS_, 0x6822)] = 0xFF; RUN();
    assert(r[at(0x6C00, 0x0001)] == 0x35u && r[at(DS_, 0x6822)] == 0xFFu && c.ax == 0x0010u && c.flags == 0x0202u);
    c = setup(r, 89); FRAME(0xFFFE, 0x6C00, 0x0100, 0x6000); PW(0x6C00, 0xFFFE, 3, 0x25); r[at(DS_, 0x6DE3)] = 0; r[at(DS_, 0x6822)] = 0xFF; RUN();
    assert(r[at(0x6C00, 0x0001)] == 0x35u && r[at(DS_, 0x6822)] == 0xFFu && c.ax == 0x0035u && c.flags == 0x0206u);
    c = setup(r, 17); FRAME(0x0100, 0x6C00, 0x0100, 0x6000); PW(0x6000, 0x0100, 0x111, 0x20); r[at(DS_, 0x6816)] = 0xFF; r[at(DS_, 0x681B)] = 0x0F; r[at(DS_, 0x6818)] = 0x44; RUN();
    assert(r[at(0x6000, 0x0211)] == 0x39u && r[at(DS_, 0x6816)] == 0x00u && r[at(DS_, 0x6818)] == 0x00u && c.flags == 0x0246u && c.es == 0x6000u && c.di == 0x0100u && c.ax == 0x0011u);
    c = setup(r, 37); FRAME(0x0100, 0x6C00, 0x0100, 0x6000);
    PW(0x6000, 0x0100, 0x108, 0x00); PW(0x6000, 0x0100, 0x109, 0x03); PW(0x6000, 0x0100, 0x10A, 0x00); PW(0x6000, 0x0100, 0x10B, 0x64); PW(0x6400, 0x0300, 3, 0x05); r[at(DS_, 0x681E)] = 0; r[at(DS_, 0x6822)] = 0; RUN();
    assert(r[at(DS_, 0x681E)] == 0x01u && r[at(DS_, 0x6822)] == 0xFFu && c.flags == 0x0206u && c.es == 0x6400u && c.di == 0x0300u);
    c = setup(r, 58); FRAME(0x0100, 0x6C00, 0x0100, 0x6000);
    PW(0x6000, 0x0100, 0x108, 0xFC); PW(0x6000, 0x0100, 0x109, 0xFF); PW(0x6000, 0x0100, 0x10A, 0x00); PW(0x6000, 0x0100, 0x10B, 0x64); PW(0x6400, 0xFFFC, 6, 0x77); r[at(DS_, 0x681D)] = 1; r[at(DS_, 0x681A)] = 0x5A; RUN();
    assert(r[at(0x6400, 0x0002)] == 0x00u && r[at(DS_, 0x681A)] == 0x00u && c.flags == 0x0202u && c.es == 0x6400u && c.di == 0xFFFCu);
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
