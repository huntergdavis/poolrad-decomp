#!/usr/bin/env python3
"""Store replay of the run-034 `2E60:0747 -> 3582:3492` LOGL window; print no game or RAM payload.

Hash-locks the private run-034 inputs (window LOGL trace, native RAM images at both stops, CPU
transcriptions) and `GAME.OVR`, then replays every memory store of the window over the native 0747
image and compares the result with the native 3492 image byte by byte.  Stores are decoded from the
logged instruction bytes (capstone, 16-bit) and addressed with the logged registers.  Declared host
effects (each one printed with its status, `measured` or `hypothesis`):

  * rep-prefixed string ops: one trace line covers a chunk of iterations; the count is taken from the
    CX change to the next line, the chunk rule `min(CX, 4)` is checked and its exceptions are counted;
  * DOS INT 21h callback: nine saved registers + PSP stack pointer (the run003/030 binding), plus the
    data of AH=3Fh reads copied from `GAME.OVR` at the position set by AH=42h (hypothesis, supported
    only by byte equality with native);
  * BIOS IRQ0 clock callback: tick dword + 1 (measured: the two anchors differ by exactly one);
  * hardware IRQ0 frame derived from the matching iret; INT 1Ch callback and the 164 idle callbacks
    declared store-free (checked: registers unchanged across each);
  * 452 display OUTs and one EOI OUT: no effect on the 640 KiB image; stores at or above 0xA0000 are
    counted, not compared and never given bytes (rule below).

Needs `capstone` (not stdlib); run without `python3 -I`.  Environment `POOLRAD_AUDIT_MUTANT=<name>`
perturbs one new branch (used by check_slot_a_window_store_replay_034.sh); the baseline passes and
every mutant must fail or be listed as equivalent in the check script.
"""

from hashlib import sha256
from pathlib import Path
import collections
import os
import re
import sys

MUT = os.environ.get("POOLRAD_AUDIT_MUTANT", "")
RUN_FILES = {
    "LOGL-WIN-0747-to-3492.TXT": "0cbc471604cc342ed9939d787d5366bd763a2e36b798433ddbe1e5fe056d902e",
    "RAM-2E60-0747.BIN": "f9b539f0defdda7ddb826d4242509b72350c22bd92869343ab35062ec809dbb5",
    "RAM-3582-3492.BIN": "ed613cd5f0c27f282e1948698b0c82b515efc5cfe24a0564ad52d6f5e3c61d59",
    "CPU-2E60-0747.TXT": "44c10f70a0a5abed96adc1baa9195cc0fce505d763ff9c666ec602691bb0f2a1",
    "CPU-3582-3492.TXT": "ccbac0c055fc1868b5ceef5b4942d2eb72422111678d9cc7aebb0ae7d4e95433",
}
GAME_HASH = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
RAM_SIZE = 655360
VIDEO_BASE = 0xA0000
LINES = 25589
DOS_CALLS = (("seek", 0x23DCC), ("read", 0xE65), ("read", 0x58), ("seek", 0x2E0CE), ("read", 0x7C5), ("read", 0x64))
READ_DEST = (0x2F6B, 0x3052, 0x3052, 0x30CF)
RELOC = 0x0824
# Run B (images only, a different native run): manifests and the intermediate stops inside the window.
RUNB_MANIFESTS = {"t4492-slot7-images-035": "c7fedbace0f4b90af8282f589c7563d4cc0651770a8ff991cf998b1eef9b1262",
                  "t4492-slot7-fill-036": "82ed962a7ce7e193560ef8d114c7e3daff970f136e8e709fa52ac8c8035e0b76"}
# (name, 0-based window row of the stop, cs, ip, run dir, image file)
RUNB_STOPS = (
    ("07A1", 25, 0x2E60, 0x07A1, "t4492-slot7-fill-036", "RAM-2E60-07A1.BIN"),
    ("07ED", 171, 0x2E60, 0x07ED, "t4492-slot7-images-035", "RAM-2E60-07ED.BIN"),
    ("028F#1", 240, 0x12C6, 0x028F, "t4492-slot7-fill-036", "RAM-12C6-028F.BIN"),
    ("029F#1", 261, 0x12C6, 0x029F, "t4492-slot7-images-035", "RAM-12C6-029F.BIN"),
    ("02C0#1", 1590, 0x12C6, 0x02C0, "t4492-slot7-images-035", "RAM-12C6-02C0.BIN"),
    ("02E1#1", 1818, 0x12C6, 0x02E1, "t4492-slot7-images-035", "RAM-12C6-02E1.BIN"),
    ("03D5", 2019, 0x2F6B, 0x03D5, "t4492-slot7-images-035", "RAM-2F6B-03D5.BIN"),
    ("07F2", 3264, 0x2E60, 0x07F2, "t4492-slot7-images-035", "RAM-2E60-07F2.BIN"),
    ("290B", 3268, 0x08DE, 0x290B, "t4492-slot7-images-035", "RAM-08DE-290B.BIN"),
    ("0C15:0137", 4351, 0x0C15, 0x0137, "t4492-slot7-images-035", "RAM-0C15-0137.BIN"),
    ("0F2D:0626", 5232, 0x0F2D, 0x0626, "t4492-slot7-images-035", "RAM-0F2D-0626.BIN"),
    ("0BF4:002F#1", 23912, 0x0BF4, 0x002F, "t4492-slot7-images-035", "RAM-0BF4-002F.BIN"),
    ("028F#2", 24260, 0x12C6, 0x028F, "t4492-slot7-images-035", "RAM-12C6-028F.BIN"),
    ("029F#2", 24281, 0x12C6, 0x029F, "t4492-slot7-images-035", "RAM-12C6-029F-b.BIN"),
    ("02C0#2", 25012, 0x12C6, 0x02C0, "t4492-slot7-images-035", "RAM-12C6-02C0-b.BIN"),
    ("02E1#2", 25264, 0x12C6, 0x02E1, "t4492-slot7-images-035", "RAM-12C6-02E1-b.BIN"),
    ("0BF4:002F#2", 25504, 0x0BF4, 0x002F, "t4492-slot7-images-035", "RAM-0BF4-002F-b.BIN"),
    ("2595#1", 25557, 0x0C88, 0x2595, "t4492-slot7-images-035", "RAM-0C88-2595.BIN"),
    ("2595#2", 25572, 0x0C88, 0x2595, "t4492-slot7-images-035", "RAM-0C88-2595-b.BIN"),
    ("3492", 25588, 0x3582, 0x3492, "t4492-slot7-images-035", "RAM-3582-3492.BIN"),
)
STACK_SEG = 0x1D16


def check(condition, message):
    if not condition:
        raise SystemExit(message)


LINE = re.compile(
    r"^(\d+\.\d+)\s+([0-9A-F]{4}):([0-9A-F]{8})\s+(.*?)\s{2,}((?:[0-9A-F]{2} )*[0-9A-F]{2})\s+"
    r"EAX:([0-9A-F]{8}) EBX:([0-9A-F]{8}) ECX:([0-9A-F]{8}) EDX:([0-9A-F]{8}) ESI:([0-9A-F]{8}) "
    r"EDI:([0-9A-F]{8}) EBP:([0-9A-F]{8}) ESP:([0-9A-F]{8}) DS:([0-9A-F]{4}) ES:([0-9A-F]{4}) "
    r"FS:([0-9A-F]{4}) GS:([0-9A-F]{4}) SS:([0-9A-F]{4}) CF:(\d) ZF:(\d) SF:(\d) OF:(\d) AF:(\d) "
    r"PF:(\d) IF:(\d) TF:(\d) VM:\d FLG:([0-9A-F]{8})")
ANNOT = re.compile(r"\b(ds|es|ss|cs|fs|gs):\[([0-9A-F]{4})\]=([0-9A-F]+)\b")


def parse(path):
    """Rows: (ts, cs, ip, text, bytes, regs, flags).  Flags are rebuilt from the per-flag columns
    (the FLG column is lazily evaluated); DF is not logged (the DI progression is checked instead)."""
    rows = []
    with open(path, "r") as handle:
        for line in handle:
            m = LINE.match(line)
            check(m, "unparsed trace line in " + Path(path).name)
            g = m.groups()
            regs = dict(zip(("ax", "bx", "cx", "dx", "si", "di", "bp", "sp"),
                            [int(x, 16) & 0xFFFF for x in g[5:13]]))
            regs.update(ds=int(g[13], 16), es=int(g[14], 16), fs=int(g[15], 16), gs=int(g[16], 16),
                        ss=int(g[17], 16), cs=int(g[1], 16))
            cf, zf, sf, of, af, pf, iff, tf = (int(x) for x in g[18:26])
            flags = ((int(g[26], 16) & 0xF000) | 0x0002 | cf | (pf << 2) | (af << 4) | (zf << 6) |
                     (sf << 7) | (tf << 8) | (iff << 9) | (of << 11))
            rows.append((float(g[0]), int(g[1], 16), int(g[2], 16) & 0xFFFF, g[3],
                         bytes(int(x, 16) for x in g[4].split()), regs, flags))
    return rows


def mnemonic(text):
    parts = text.split()
    return parts[0] if parts else ""


def replay(rows, base, game, snap_rows=()):
    import capstone
    from capstone import x86 as X
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_16)
    md.detail = True
    mem = {}
    stats = collections.Counter()
    video = collections.Counter()     # stores at/above 0xA0000, by (segment, class)
    notes = {}

    taint = [False]

    def rd(a):
        a &= 0xFFFFF
        if a >= VIDEO_BASE:
            stats["video-read"] += 1
            taint[0] = True            # value unobserved: may only flow back into display memory
            return 0
        return mem.get(a, base[a])

    def rdw(a):
        return rd(a) | (rd(a + 1) << 8)

    stores = []

    def wr(i, a, n, v, kind, cls):
        a &= 0xFFFFF
        for k in range(n):
            b = (a + k) & 0xFFFFF
            byte = (v >> (8 * k)) & 255
            if MUT == "video-wrap" and b >= VIDEO_BASE:
                mem[b - VIDEO_BASE] = byte
                continue
            if b >= VIDEO_BASE:
                video[(b >> 4 & 0xF000, cls)] += 1
                if taint[0]:
                    stats["video-read-to-video"] += 1
                continue
            if taint[0]:
                stats["video-read-to-ram"] += 1
            mem[b] = byte
        stores.append((i, a, n, kind, cls))

    reg_ids = {X.X86_REG_AX: ("ax", 0xFFFF, 0), X.X86_REG_BX: ("bx", 0xFFFF, 0), X.X86_REG_CX: ("cx", 0xFFFF, 0),
               X.X86_REG_DX: ("dx", 0xFFFF, 0), X.X86_REG_SI: ("si", 0xFFFF, 0), X.X86_REG_DI: ("di", 0xFFFF, 0),
               X.X86_REG_BP: ("bp", 0xFFFF, 0), X.X86_REG_SP: ("sp", 0xFFFF, 0),
               X.X86_REG_AL: ("ax", 0xFF, 0), X.X86_REG_AH: ("ax", 0xFF, 8), X.X86_REG_BL: ("bx", 0xFF, 0),
               X.X86_REG_BH: ("bx", 0xFF, 8), X.X86_REG_CL: ("cx", 0xFF, 0), X.X86_REG_CH: ("cx", 0xFF, 8),
               X.X86_REG_DL: ("dx", 0xFF, 0), X.X86_REG_DH: ("dx", 0xFF, 8), X.X86_REG_DS: ("ds", 0xFFFF, 0),
               X.X86_REG_ES: ("es", 0xFFFF, 0), X.X86_REG_SS: ("ss", 0xFFFF, 0), X.X86_REG_CS: ("cs", 0xFFFF, 0),
               X.X86_REG_FS: ("fs", 0xFFFF, 0), X.X86_REG_GS: ("gs", 0xFFFF, 0)}

    def reg(r, rid):
        name, mask, shift = reg_ids[rid]
        return (r[name] >> shift) & mask

    def ea(r, op):
        m = op.mem
        off = m.disp + (reg(r, m.base) if m.base else 0) + (reg(r, m.index) if m.index else 0)
        seg = reg(r, m.segment) if m.segment else (r["ss"] if m.base in (X.X86_REG_BP,) else r["ds"])
        return ((seg << 4) + (off & 0xFFFF)) & 0xFFFFF

    def val(r, op, size):
        if op.type == X.X86_OP_REG:
            return reg(r, op.reg)
        if op.type == X.X86_OP_IMM:
            return op.imm & ((1 << (8 * size)) - 1)
        a = ea(r, op)
        return rd(a) if size == 1 else rdw(a)

    def classify(cs):
        return "rom-stub" if cs == 0xF000 else "game"

    unknown = collections.Counter()
    alu = {"add": lambda o, s, c: o + s, "sub": lambda o, s, c: o - s, "adc": lambda o, s, c: o + s + c,
           "sbb": lambda o, s, c: o - s - c, "and": lambda o, s, c: o & s, "or": lambda o, s, c: o | s,
           "xor": lambda o, s, c: o ^ s}
    dos_state = {"pos": None, "calls": 0, "reads": 0}
    annot = collections.Counter()
    annot_bad = []
    chunk_exceptions = 0
    rep_lines = collections.Counter()
    hw_entries = 0
    last = len(rows) - 1
    snaps = {}
    for i, (ts, cs, ip, text, byt, r, flags) in enumerate(rows):
        if i in snap_rows:
            snaps[i] = dict(mem)       # state before this row executes (a debugger stop at this address)
        if i == 0:
            continue               # first line at a breakpoint is logged twice (second is the same instruction)
        if i == last and MUT != "apply-stop-row":
            continue               # the final line is the 3492 stop state: not executed
        sspb, sp = r["ss"] << 4, r["sp"]
        cls = classify(cs)
        taint[0] = False
        # measured memory-read annotations of the trace vs the replayed shadow memory (pre-state)
        m = ANNOT.search(text)
        if m and not text.startswith("callback"):
            seg = r[m.group(1)]
            off = int(m.group(2), 16)
            hexv = m.group(3)
            a = ((seg << 4) + off) & 0xFFFFF
            if a + len(hexv) // 2 > VIDEO_BASE:
                annot["video-skipped"] += 1
            else:
                got = bytes(mem.get((a + k) & 0xFFFFF, base[(a + k) & 0xFFFFF]) for k in range(len(hexv) // 2))
                want = int(hexv, 16).to_bytes(len(hexv) // 2, "little")
                if got == want:
                    annot["match"] += 1
                else:
                    annot["mismatch"] += 1
                    annot_bad.append((i, a))
        if (cs, ip) == (0xF000, 0xFEA5) and mnemonic(rows[i - 1][3]) not in ("jmp", "call", "ljmp", "int"):
            hw_entries += 1
            j = i + 1
            while j < len(rows) and not (mnemonic(rows[j][3]) == "iret" and rows[j][5]["sp"] == sp):
                j += 1
            check(j + 1 < len(rows), "IRQ0 frame has no matching iret")
            ret_cs, ret_ip = rows[j + 1][1], rows[j + 1][2]
            if MUT != "no-hw-frame":
                wr(i, sspb + sp, 2, ret_ip, "irq-frame", "hw-frame")
                wr(i, sspb + sp + 2, 2, ret_cs, "irq-frame", "hw-frame")
                wr(i, sspb + sp + 4, 2, flags | 0x0200, "irq-frame", "hw-frame")
        if text.startswith("callback"):
            tag = text.split()[1]
            nxt = rows[i + 1][5] if i + 1 < len(rows) else r
            if tag == "0038":          # DOS INT 21h callback
                check(r["di"] == RELOC, "DOS call without the measured relocation constant in DI")
                regs9 = [r["ax"], r["bx"], r["cx"], r["dx"], r["si"], r["di"], r["bp"], r["ds"], r["es"]]
                if MUT == "regsave-order":
                    regs9[0], regs9[1] = regs9[1], regs9[0]
                for k, v in enumerate(regs9):
                    wr(i, sspb + ((sp - 18) & 0xFFFF) + 2 * k, 2, v, "dos-regsave", "dos-callback")
                psp_sp = (sp - 18) & 0xFFFF
                if MUT == "psp-ptr":
                    psp_sp = (psp_sp + 2) & 0xFFFF
                wr(i, 0x8140 + 0x2E, 2, psp_sp, "dos-psp", "dos-callback")
                wr(i, 0x8140 + 0x30, 2, r["ss"], "dos-psp", "dos-callback")
                ah, al = r["ax"] >> 8, r["ax"] & 255
                want_kind, want_arg = DOS_CALLS[dos_state["calls"]]
                if ah == 0x42:
                    check(want_kind == "seek" and al == 0 and r["bx"] == 5, "unexpected DOS seek")
                    pos = (r["cx"] << 16) | r["dx"]
                    if MUT == "seek-high":
                        pos = r["dx"]
                    check(pos == want_arg, "DOS seek position mismatch")
                    dos_state["pos"] = pos
                elif ah == 0x3F:
                    check(want_kind == "read" and r["bx"] == 5 and r["cx"] == want_arg, "unexpected DOS read")
                    n, pos = r["cx"], dos_state["pos"]
                    check(pos is not None and pos + n <= len(game), "DOS read outside GAME.OVR")
                    dseg = r["ds"] + (1 if MUT == "read-dest-seg" else 0)
                    check(dseg == READ_DEST[dos_state["reads"]] or MUT == "read-dest-seg", "DOS read destination mismatch")
                    if MUT == "read-pos":
                        pos += 1
                    if MUT == "read-short":
                        n -= 1
                    if MUT != "read-skip":
                        for k in range(n):
                            wr(i, (dseg << 4) + ((r["dx"] + k) & 0xFFFF), 1, game[pos + k], "dos-read", "dos-read")
                    dos_state["pos"] = dos_state["pos"] + r["cx"]
                    dos_state["reads"] += 1
                    dos_state["last_read_n"] = r["cx"]
                else:
                    raise SystemExit("unsupported DOS function %02X" % ah)
                dos_state["calls"] += 1
            elif tag == "0008":        # BIOS IRQ0 clock callback: tick dword + 1
                tick = rdw(0x46C) | (rdw(0x46E) << 16)
                inc = 2 if MUT == "tick-two" else 1
                if MUT != "tick-skip":
                    wr(i, 0x46C, 4, (tick + inc) & 0xFFFFFFFF, "bios-tick", "bios-callback")
            elif tag in ("0010", "0002"):   # INT 1Ch timer callback, idle callback: declared store-free
                changed = [k for k in ("ax", "bx", "cx", "dx", "si", "di", "bp", "sp", "ds", "es", "ss") if nxt[k] != r[k]]
                if changed == ["ax"] and tag == "0002" and nxt["ax"] == dos_state.get("last_read_n"):
                    # the delayed DOS read completion: AX = bytes read, a register effect only
                    stats["callback-0002-read-completion"] += 1
                else:
                    check(not changed, "registers %s change across callback %s" % (changed, tag))
                    stats["callback-" + tag] += 1
            else:
                raise SystemExit("undeclared callback " + tag)
            continue
        try:
            ins = next(md.disasm(byt, ip))
        except StopIteration:
            unknown["undecoded"] += 1
            continue
        mn, ops = ins.mnemonic, ins.operands
        check(ins.addr_size == 2, "32-bit address size at row %d" % i)
        rep = mn.startswith("rep")
        base_mn = mn.split()[-1]
        if mn == "push":
            wr(i, sspb + ((sp - 2) & 0xFFFF), 2, val(r, ops[0], 2) & 0xFFFF, "push", cls)
        elif mn == "call":
            wr(i, sspb + ((sp - 2) & 0xFFFF), 2, (ip + ins.size) & 0xFFFF, "call", cls)
        elif mn == "lcall":
            wr(i, sspb + ((sp - 2) & 0xFFFF), 2, cs, "lcall", cls)
            wr(i, sspb + ((sp - 4) & 0xFFFF), 2, (ip + ins.size) & 0xFFFF, "lcall", cls)
        elif mn == "int":
            wr(i, sspb + ((sp - 2) & 0xFFFF), 2, flags, "int", cls)
            wr(i, sspb + ((sp - 4) & 0xFFFF), 2, cs, "int", cls)
            wr(i, sspb + ((sp - 6) & 0xFFFF), 2, (ip + ins.size) & 0xFFFF, "int", cls)
        elif base_mn in ("stosb", "stosw", "movsb", "movsw"):
            w = 2 if base_mn.endswith("w") else 1
            nxt = rows[i + 1][5]
            if rep:
                check(ins.prefix[0] == 0xF3, "unsupported rep prefix at row %d" % i)
                n = (r["cx"] - nxt["cx"]) & 0xFFFF
                chunk = min(r["cx"], 4)
                if n != chunk:
                    chunk_exceptions += 1
                if MUT == "rep-chunk4":
                    n = chunk
                check(1 <= n <= r["cx"], "rep chunk count out of range at row %d" % i)
                rep_lines["rep " + base_mn] += 1
                rep_lines["rep-iterations"] += n
            else:
                n = 1
                rep_lines[base_mn] += 1
            check(((nxt["di"] - r["di"]) & 0xFFFF) == n * w, "DI progression disagrees with DF=0 at row %d" % i)
            if base_mn.startswith("movs"):
                sseg = reg(r, ops[1].mem.segment) if ops[1].mem.segment else r["ds"]
                if MUT == "movs-src-seg":
                    sseg = r["es"]
                check(((nxt["si"] - r["si"]) & 0xFFFF) == n * w, "SI progression disagrees at row %d" % i)
            for k in range(n):
                dst = (r["es"] << 4) + ((r["di"] + k * w) & 0xFFFF)
                if base_mn.startswith("stos"):
                    wv = r["ax"] & ((1 << (8 * (1 if MUT == "stos-width" else w))) - 1)
                    wr(i, dst, 1 if MUT == "stos-width" else w, wv, ("rep-" if rep else "") + "stos", cls)
                else:
                    src = (sseg << 4) + ((r["si"] + k * w) & 0xFFFF)
                    wv = rd(src) | (rd(src + 1) << 8 if w == 2 else 0)
                    wr(i, dst, w, wv, ("rep-" if rep else "") + "movs", cls)
        elif mn == "xchg":
            memops = [o for o in ops if o.type == X.X86_OP_MEM]
            if memops:
                other = [o for o in ops if o.type == X.X86_OP_REG][0]
                size = memops[0].size
                v = val(r, other, size)
                if MUT == "xchg-skip":
                    pass
                else:
                    wr(i, ea(r, memops[0]), size, v, "xchg", cls)
        elif ops and ops[0].type == X.X86_OP_MEM and mn not in (
                "cmp", "test", "lea", "les", "lds", "mul", "imul", "div", "idiv", "jmp", "ljmp", "call", "lcall", "push") \
                and not mn.startswith("j"):
            a, size = ea(r, ops[0]), ops[0].size
            check(size in (1, 2), "unsupported operand size at row %d" % i)
            old = rd(a) if size == 1 else rdw(a)
            carry = flags & 1
            if mn == "mov":
                new = val(r, ops[1], size)
            elif mn in alu:
                new = alu[mn](old, val(r, ops[1], size), carry)
            elif mn == "inc":
                new = old + 1
            elif mn == "dec":
                new = old - 1
            else:
                unknown["memdest:" + mn] += 1
                continue
            wr(i, a, size, new & ((1 << (8 * size)) - 1), mn, cls)
        elif mn in ("pushf", "pusha", "pop", "popf") and ops and ops[0].type == X.X86_OP_MEM:
            unknown["unhandled:" + mn] += 1
    check(not unknown, "unsupported store forms: " + str(dict(unknown)))
    check(dos_state["calls"] == len(DOS_CALLS), "DOS call count mismatch")
    check(hw_entries == 1, "hardware IRQ entry count mismatch")
    notes.update(rep_lines=dict(rep_lines), chunk_exceptions=chunk_exceptions, annot=dict(annot),
                 annot_bad=annot_bad, stats=dict(stats), hw_entries=hw_entries, snaps=snaps)
    return mem, stores, video, notes


def runb_control(rows, base, snaps, runs_dir):
    """Replayed prefix (Run A 0747 + stores up to each stop row) vs the Run B image at that stop.  Run B is a
    different native run: bytes where its own 0747 image differs from Run A's (the noise set) are excluded,
    and every other mismatch must lie in the stack segment (run-dependent pushed values, and the IRQ0 landing
    on a different instruction), never in game or DOS data."""
    zero = {}
    for name, digest in RUNB_MANIFESTS.items():
        mpath = runs_dir / name / "MANIFEST-CAPTURE.sha256"
        check(sha256(mpath.read_bytes()).hexdigest() == digest, name + " manifest identity mismatch")
        zero[name] = {}
        for line in mpath.read_text().splitlines():
            parts = line.split()
            if len(parts) == 2:
                zero[name][parts[1].lstrip("*").split("/")[-1]] = parts[0]

    def load(name, fname):
        data = (runs_dir / name / fname).read_bytes()
        check(zero[name].get(fname) == sha256(data).hexdigest() and len(data) == RAM_SIZE,
              name + "/" + fname + " is not the manifest image")
        return data

    zero_img = {name: load(name, "RAM-2E60-0747.BIN") for name in RUNB_MANIFESTS}
    out_total, stack_lo, stack_hi = 0, STACK_SEG << 4, (STACK_SEG << 4) + 0x10000
    print("Run B intermediate-stop control (noise = Run B 0747 image vs Run A 0747 image):")
    for name, k, cs, ip, rdir, fname in RUNB_STOPS:
        check((rows[k][1], rows[k][2]) == (cs, ip), "stop row %s does not sit at %04X:%04X" % (name, cs, ip))
        img = load(rdir, fname)
        pred = bytearray(base)
        for a, v in snaps[k].items():
            if a < RAM_SIZE:
                pred[a] = v
        noise = {a for a in range(RAM_SIZE) if zero_img[rdir][a] != base[a]}
        mism = {a for a in range(RAM_SIZE) if pred[a] != img[a]}
        outside = sorted(mism - noise)
        off_stack = [a for a in outside if not stack_lo <= a < stack_hi]
        print("  %-12s row %5d  noise %3d  mismatch %3d  in noise %3d  outside %3d  outside the stack segment %d" %
              (name, k, len(noise), len(mism), len(mism & noise), len(outside), len(off_stack)))
        out_total += len(off_stack)
    check(out_total == 0, "Run B control: %d mismatch byte(s) outside noise and the stack segment" % out_total)


def main():
    check(len(sys.argv) in (3, 4), "usage: audit_slot_a_window_store_replay_034.py RUN034_DIR GAME.OVR [OVERRIDE_RUNS_DIR]")
    run, game_path = Path(sys.argv[1]), Path(sys.argv[2])
    runs_dir = Path(sys.argv[3]) if len(sys.argv) == 4 else None
    for name, digest in RUN_FILES.items():
        h = sha256()
        with open(run / name, "rb") as handle:
            for chunk in iter(lambda: handle.read(1 << 20), b""):
                h.update(chunk)
        check(h.hexdigest() == digest, name + " identity mismatch")
    game = game_path.read_bytes()
    check(len(game) == 206105 and sha256(game).hexdigest() == GAME_HASH, "GAME.OVR identity mismatch")
    try:
        import capstone  # noqa: F401
    except ImportError:
        raise SystemExit("capstone is required for the store decode (pip install capstone)")
    rows = parse(run / "LOGL-WIN-0747-to-3492.TXT")
    check(len(rows) == LINES, "window line count mismatch")
    check((rows[0][1], rows[0][2]) == (0x2E60, 0x0747) and rows[0][1:3] == rows[1][1:3] and
          (rows[-1][1], rows[-1][2]) == (0x3582, 0x3492), "window start/stop rows mismatch")
    base = (run / "RAM-2E60-0747.BIN").read_bytes()
    native = (run / "RAM-3582-3492.BIN").read_bytes()
    check(len(base) == RAM_SIZE and len(native) == RAM_SIZE, "RAM size mismatch")
    snap_rows = {st[1] for st in RUNB_STOPS} if runs_dir else ()
    mem, stores, video, notes = replay(rows, base, game, snap_rows)
    check(sum(1 for r in rows if (r[1], r[2]) == (0xF000, 0xFEA5)) == 1 and
          sum(1 for r in rows if r[3].startswith("callback 0008")) == 1 and
          sum(1 for r in rows if r[3].startswith("callback 0010")) == 1,
          "BIOS IRQ0 chain counts mismatch (expected one F000:FEA5 entry, one IRQ0 callback, one INT 1Ch callback)")
    pred = bytearray(base)
    for a, v in mem.items():
        if a < RAM_SIZE:
            pred[a] = v
    delta = [i for i in range(RAM_SIZE) if base[i] != native[i]]
    mism = [i for i in range(RAM_SIZE) if pred[i] != native[i]]
    tick0, tick1 = (int.from_bytes(x[0x46C:0x470], "little") for x in (base, native))
    check(tick1 == tick0 + 1, "BIOS tick words differ by other than one")
    writers = collections.defaultdict(set)
    kinds = collections.defaultdict(set)
    lastw = {}
    for i, a, n, kind, cls in stores:
        for k in range(n):
            b = (a + k) & 0xFFFFF
            if b < RAM_SIZE:
                writers[b].add(cls)
                kinds[b].add(kind)
                lastw[b] = (cls, kind)
    # per-byte account
    explained = [a for a in delta if writers[a] and pred[a] == native[a]]
    bad_value = [a for a in delta if writers[a] and pred[a] != native[a]]
    no_writer = [a for a in delta if not writers[a]]
    spurious = [a for a in mism if base[a] == native[a]]
    benign = [a for a in writers if base[a] == native[a] and pred[a] == native[a]]
    print("window rows %d (executed %d), decoded stores %d, native delta %d bytes (tick %d -> %d)" %
          (len(rows), len(rows) - 2, len(stores), len(delta), tick0, tick1))
    print("per-byte account: explained %d, writer-but-wrong-value %d, native-changed-without-writer %d, "
          "spurious (replay changes an unchanged byte) %d, written-but-unchanged %d" %
          (len(explained), len(bad_value), len(no_writer), len(spurious), len(benign)))
    print("unattributed (delta bytes with no replayed writer): %d; image mismatches: %d" % (len(no_writer), len(mism)))
    by_class = collections.Counter("+".join(sorted(writers[a])) for a in delta if writers[a])
    print("explained delta bytes by writer-class set:", dict(sorted(by_class.items())))
    last_by_class = collections.Counter(lastw[a][0] for a in delta if a in lastw)
    print("explained delta bytes by last writer class:", dict(sorted(last_by_class.items())))
    last_by_kind = collections.Counter(lastw[a][1] for a in delta if a in lastw)
    print("explained delta bytes by last writer kind:", dict(sorted(last_by_kind.items())))
    print("rep/string lines:", notes["rep_lines"], "chunk-rule (min(CX,4)) exceptions:", notes["chunk_exceptions"])
    print("trace memory-read annotations vs replayed shadow memory:", notes["annot"],
          "mismatches at rows", [x[0] for x in notes["annot_bad"][:10]])
    cbs = {k: v for k, v in notes["stats"].items() if k.startswith("callback")}
    print("callbacks declared store-free (registers unchanged across, except the DOS read completion setting AX):", cbs)
    print("display-memory reads (unobserved): %d, all feeding display-memory stores; feeding the 640 KiB image: %d" %
          (notes["stats"].get("video-read", 0), notes["stats"].get("video-read-to-ram", 0)))
    print("stores at/above 0xA0000 (counted, not compared, no bytes invented):",
          sum(video.values()), dict(sorted((("%04X" % s, c), n) for (s, c), n in video.items())) if video else {})
    if mism:
        by_seg = collections.Counter()
        for a in mism:
            by_seg[a >> 12] += 1
        print("mismatch bytes by 4 KiB page:", dict(sorted(by_seg.items())))
    # Non-vacuity: omitting each writer class / kind (by last writer) must leave bytes unexplained or be reported.
    def omit(select):
        alt = bytearray(base)
        for a, v in mem.items():
            if a < RAM_SIZE and not select(lastw.get(a, ("?", "?"))):
                alt[a] = v
        return sum(1 for i in range(RAM_SIZE) if alt[i] != native[i])
    classes = sorted({c for c, _ in lastw.values()})
    kinds_all = sorted({k for _, k in lastw.values()})
    print("omission controls by writer class (bytes left unexplained):",
          {c: omit(lambda w, c=c: w[0] == c) for c in classes})
    print("omission controls by writer kind (bytes left unexplained):",
          {k: omit(lambda w, k=k: w[1] == k) for k in kinds_all})
    print("measured relocation constant at every DOS call: DI = %04X" % RELOC)
    if runs_dir:
        runb_control(rows, base, notes["snaps"], runs_dir)
    check(notes["stats"].get("video-read-to-ram", 0) == 0, "an unobserved display-memory read feeds a store into the 640 KiB image")
    check(not notes["annot_bad"], "trace read annotations disagree with replayed memory at %d row(s)" % len(notes["annot_bad"]))
    check(not no_writer and not bad_value and not mism,
          "store replay leaves %d unattributed byte(s), %d wrong value(s), %d image mismatch(es)" %
          (len(no_writer), len(bad_value), len(mism)))
    print("run-034 window store replay verified")


main()
