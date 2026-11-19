#!/usr/bin/env python3
"""Audit the run-030 LOGL traces and replay their stores; print no game or RAM payload.

Hash-locks the private run-030 inputs, derives the measured replay inputs from the traces
(DOS time polls, IRQ0 entries, IN 61h and the ordered OUTs) and replays every memory store of
the 00F4->0165 window (decoded from the logged instruction bytes with capstone, addressed with
the logged registers) over the native 00F4 image, comparing with the native 0165 image.
Two host callbacks are declared, not traced: the DOS INT 21h callback (nine saved registers
below the interrupt frame and the PSP stack pointer, as the run003 binding) and the BIOS IRQ0
clock callback (BIOS tick dword + 1). Needs `capstone` (not stdlib) for the store decode.
"""

from hashlib import sha256
from pathlib import Path
import collections
import re
import sys

RUN_FILES = {
    "LOGL-A-13A3-to-00F4.TXT": "be5611e0cf08bca6516b9827750a65275273d123ea2548f6a65b4d804074d854",
    "LOGL-W1-00F4-to-0165.TXT": "0ec136c9104063de9448ef956719b0b73607db06492686748eb174bbf5722a6d",
    "LOGL-W3-13C6-to-0747.TXT": "4960c5b94fe3c09bf487a1e71069a1297c0648bab9927d07c9409835b1432fc0",
    "RAM-10D2-00F4.BIN": "dbbc1e3e4abd68aad0de729be534bace84ebb6c8cd074a4263096af194f73b37",
    "RAM-10D2-0165.BIN": "b227e2c1fadb3b1b8d3dd00a1020d83edddcdc193e16acbe26c93f181c5890c8",
    "RAM-10D2-13C6.BIN": "342882515c7dd4f852a17a15bc78c4d7bf147b2cb287e3e7169abbcaee02e270",
    "RAM-2E60-0747.BIN": "99d632bb8349102c6f41a495ce212367e07a4d35b32206b9654703e2e065e270",
    "CPU-10D2-00F4.TXT": "a1be6e91ab4f9386a3c495cfdb3c776e2758f6e802de20c56b30877918b301bb",
    "CPU-10D2-0165.TXT": "be674fba164d5b5bde0407d34d6cfd3f6f4515a05247c80bd227a2c5d3f73218",
    "CPU-10D2-13C6.TXT": "c5403b94eb3b242d82c7646dff8c566bcd54412e02af6e3d4e0112c97bf77c35",
    "CPU-2E60-0747.TXT": "c4b279854701fd67463ae3f7cb58a1d9eb805c21891786d10175bddbcfa56396",
}
RAM_SIZE = 655360
# Facts the traces must reproduce (measured, run t4492-slot4-logc-030).
W1_LINES = 411601
GETTIME_POLLS = 924
READINGS = ((561, 0x0813, 0x2C4C), (362, 0x0813, 0x2C52), (1, 0x0813, 0x2C57))   # 08:19:44.76/.82/.87
BODY_VALUES = (2932940, 2932946, 2932951)
IRQ_ENTRIES = 26
IRQ_PERIOD_MS = 4.225
STORES_TOTAL = 41844 + 2          # decoded stores + two BIOS tick callbacks
NATIVE_DELTA_BYTES = 131


def check(condition, message):
    if not condition:
        raise SystemExit(message)


LINE = re.compile(
    r"^(\d+\.\d+)\s+([0-9A-F]{4}):([0-9A-F]{8})\s+(.*?)\s{2,}((?:[0-9A-F]{2} )*[0-9A-F]{2})\s+"
    r"EAX:([0-9A-F]{8}) EBX:([0-9A-F]{8}) ECX:([0-9A-F]{8}) EDX:([0-9A-F]{8}) ESI:([0-9A-F]{8}) "
    r"EDI:([0-9A-F]{8}) EBP:([0-9A-F]{8}) ESP:([0-9A-F]{8}) DS:([0-9A-F]{4}) ES:([0-9A-F]{4}) "
    r"FS:([0-9A-F]{4}) GS:([0-9A-F]{4}) SS:([0-9A-F]{4}) CF:(\d) ZF:(\d) SF:(\d) OF:(\d) AF:(\d) "
    r"PF:(\d) IF:(\d) TF:(\d) VM:\d FLG:([0-9A-F]{8})")


def parse(path):
    """Rows: (ts, cs, ip, text, bytes, regs, flags). Flags are rebuilt from the individual flag
    columns because the FLG column is the lazily-evaluated value."""
    rows = []
    with open(path, "r") as handle:
        for line in handle:
            m = LINE.match(line)
            check(m, "unparsed trace line in " + Path(path).name)
            g = m.groups()
            regs = dict(zip(("ax", "bx", "cx", "dx", "si", "di", "bp", "sp"),
                            [int(x, 16) & 0xFFFF for x in g[5:13]]))
            regs.update(ds=int(g[13], 16), es=int(g[14], 16), ss=int(g[17], 16), cs=int(g[1], 16))
            cf, zf, sf, of, af, pf, iff, tf = (int(x) for x in g[18:26])
            flags = ((int(g[26], 16) & 0xF000) | 0x0002 | cf | (pf << 2) | (af << 4) | (zf << 6) |
                     (sf << 7) | (tf << 8) | (iff << 9) | (of << 11))
            rows.append((float(g[0]), int(g[1], 16), int(g[2], 16) & 0xFFFF, g[3],
                         bytes(int(x, 16) for x in g[4].split()), regs, flags))
    return rows


def mnemonic(text):
    parts = text.split()
    return parts[0] if parts else ""


def derive_inputs(w1, w3, w0):
    polls = [(r[0], r[5]["cx"], r[5]["dx"]) for r in w1 if r[1] == 0x1256 and r[2] == 0x00A9]
    check(len(polls) == GETTIME_POLLS, "GetTime poll count mismatch")
    seq = []
    for _, cx, dx in polls:
        if seq and seq[-1][1:] == [cx, dx]:
            seq[-1][0] += 1
        else:
            seq.append([1, cx, dx])
    check(tuple(tuple(x) for x in seq) == READINGS, "GetTime reading sequence mismatch")
    body = []
    for _, cx, dx in READINGS:
        hour, minute, second, hs = cx >> 8, cx & 255, dx >> 8, dx & 255
        body.append(hour * 360000 + ((minute * 6000) & 0xFFFF) + second * 100 + hs)
    target = BODY_VALUES[0] + 10          # first reading + the delay argument (10)
    check(tuple(body) == BODY_VALUES and BODY_VALUES[1] < target <= BODY_VALUES[2], "time body values mismatch")
    check(sum(1 for r in w1 if r[1] == 0x1256 and r[2] == 0x00A7 and mnemonic(r[3]) == "int") == GETTIME_POLLS,
          "INT 21h site count mismatch")
    check(sum(1 for r in w1 if r[1] == 0xF000 and r[2] == 0xDA9C and mnemonic(r[3]) == "int") == GETTIME_POLLS,
          "INT 1A count mismatch")
    last = w1[-1]
    check((last[1], last[2]) == (0x10D2, 0x0165) and last[5]["ax"] == 0xC0D7 and last[5]["dx"] == 0x002C and
          last[5]["cx"] == 10 and last[5]["si"] == 8 and last[5]["bx"] == 0, "0165 registers mismatch")
    check(last[5]["dx"] << 16 | last[5]["ax"] == BODY_VALUES[2], "0165 DX:AX is not the last poll value")
    irq = [i for i, r in enumerate(w1) if (r[1], r[2]) == (0x10D2, 0x13EA)]
    check(len(irq) == IRQ_ENTRIES, "IRQ0 entry count mismatch")
    gaps = [w1[b][0] - w1[a][0] for a, b in zip(irq, irq[1:])]
    check(all(abs(g - IRQ_PERIOD_MS) < 0.002 for g in gaps), "IRQ0 period mismatch")
    interrupted = collections.Counter((w1[i - 1][1], w1[i - 1][2]) for i in irq)
    check(interrupted[(0xF000, 0xD10B)] == 21, "IRQ0 interruption site mismatch")
    chain_at = [k for k, i in enumerate(irq, 1) if any((w1[j][1], w1[j][2]) == (0xF000, 0xFEA5) for j in range(i, i + 400))]
    check(chain_at == [13, 26], "BIOS chain ordinals mismatch (expected IRQ0 #13 and #26)")
    check(sum(1 for r in w1 if r[1] == 0xF000 and r[2] == 0xFEA5) == 2 and
          sum(1 for r in w1 if r[3].startswith("callback 0008")) == 2, "BIOS clock chain mismatch")
    # W3: IN 61h and the ordered port writes (value = AL in the register file *before* the next line).
    outs = []
    for k, r in enumerate(w3):
        m = mnemonic(r[3])
        if m in ("in", "out"):
            nxt = w3[k + 1][5] if k + 1 < len(w3) else r[5]
            outs.append((m, r[3].split()[1].split(",")[0] if m == "out" else "61", r, nxt))
    check([(o[0], o[1]) for o in outs] == [("out", "40"), ("out", "40"), ("in", "61"), ("out", "61")] + [("out", "C0")] * 4,
          "W3 port sequence mismatch")
    in_value = outs[2][3]["ax"] & 0xFF
    check(in_value == 0x30, "IN 61h value mismatch")
    out_values = [o[2][5]["ax"] & 0xFF for o in outs if o[0] == "out"]
    check(out_values == [0xFF, 0xFF, 0x30, 0x9F, 0xBF, 0xDF, 0xFF], "W3 OUT values mismatch")
    check(w3[-1][1:3] == (0x2E60, 0x0747), "W3 end mismatch")
    # W0 (13A3..13B0): OUT 43h<-B6h, OUT 40h<-B1h, OUT 40h<-13h
    o0 = [(r[3].split()[1].split(",")[0], r[5]["ax"] & 0xFF) for r in w0 if mnemonic(r[3]) == "out"]
    check(o0 == [("43", 0xB6), ("43", 0xB6), ("40", 0xB1), ("40", 0x13)], "W0 port sequence mismatch")
    return len(polls), len(irq)


def replay(rows, base):
    import capstone
    from capstone import x86 as X
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_16)
    md.detail = True
    mem = {}

    def rd(a):
        a &= 0xFFFFF
        return mem.get(a, base[a] if a < len(base) else 0)

    def rdw(a):
        return rd(a) | (rd(a + 1) << 8)

    stores = []

    def wr(i, a, n, v, kind, cls):
        a &= 0xFFFFF
        for k in range(n):
            mem[(a + k) & 0xFFFFF] = (v >> (8 * k)) & 255
        stores.append((i, a, n, kind, cls))

    reg_ids = {X.X86_REG_AX: ("ax", 0xFFFF, 0), X.X86_REG_BX: ("bx", 0xFFFF, 0), X.X86_REG_CX: ("cx", 0xFFFF, 0),
               X.X86_REG_DX: ("dx", 0xFFFF, 0), X.X86_REG_SI: ("si", 0xFFFF, 0), X.X86_REG_DI: ("di", 0xFFFF, 0),
               X.X86_REG_BP: ("bp", 0xFFFF, 0), X.X86_REG_SP: ("sp", 0xFFFF, 0),
               X.X86_REG_AL: ("ax", 0xFF, 0), X.X86_REG_AH: ("ax", 0xFF, 8), X.X86_REG_BL: ("bx", 0xFF, 0),
               X.X86_REG_BH: ("bx", 0xFF, 8), X.X86_REG_CL: ("cx", 0xFF, 0), X.X86_REG_CH: ("cx", 0xFF, 8),
               X.X86_REG_DL: ("dx", 0xFF, 0), X.X86_REG_DH: ("dx", 0xFF, 8), X.X86_REG_DS: ("ds", 0xFFFF, 0),
               X.X86_REG_ES: ("es", 0xFFFF, 0), X.X86_REG_SS: ("ss", 0xFFFF, 0), X.X86_REG_CS: ("cs", 0xFFFF, 0)}

    def reg(r, rid):
        name, mask, shift = reg_ids[rid]
        return (r[name] >> shift) & mask

    def ea(r, op):
        m = op.mem
        off = m.disp + (reg(r, m.base) if m.base else 0) + (reg(r, m.index) if m.index else 0)
        seg = reg(r, m.segment) if m.segment else (r["ss"] if m.base == X.X86_REG_BP else r["ds"])
        return ((seg << 4) + (off & 0xFFFF)) & 0xFFFFF

    def val(r, op):
        if op.type == X.X86_OP_REG:
            return reg(r, op.reg)
        if op.type == X.X86_OP_IMM:
            return op.imm & ((1 << (8 * (op.size or 2))) - 1)
        a = ea(r, op)
        return rd(a) if op.size == 1 else rdw(a)

    def classify(cs, ip):
        if cs == 0xF000:
            return "rom-stub"
        if cs == 0x10D2 and 0x13EA <= ip < 0x1742:
            return "irq0-hook"
        return "modeled"

    unknown = collections.Counter()
    alu = {"add": lambda o, s, c: o + s, "sub": lambda o, s, c: o - s, "adc": lambda o, s, c: o + s + c,
           "sbb": lambda o, s, c: o - s - c, "and": lambda o, s, c: o & s, "or": lambda o, s, c: o | s,
           "xor": lambda o, s, c: o ^ s}
    for i, (ts, cs, ip, text, byt, r, flags) in enumerate(rows):
        sspb, sp = r["ss"] << 4, r["sp"]
        cls = classify(cs, ip)
        if (cs, ip) == (0x10D2, 0x13EA) and mnemonic(rows[i - 1][3]) not in ("jmp", "call", "ljmp"):
            # hardware IRQ0 entry: the frame is already pushed at this line's SP; the interrupted
            # successor is the line after the matching iret (SP back at the frame).
            j = i + 1
            while j < len(rows) and not (mnemonic(rows[j][3]) == "iret" and rows[j][5]["sp"] == sp):
                j += 1
            check(j + 1 < len(rows), "IRQ0 frame has no matching iret")
            ret_cs, ret_ip = rows[j + 1][1], rows[j + 1][2]
            wr(i, sspb + sp, 2, ret_ip, "irq-frame", "hw-frame")
            wr(i, sspb + sp + 2, 2, ret_cs, "irq-frame", "hw-frame")
            wr(i, sspb + sp + 4, 2, flags | 0x0200, "irq-frame", "hw-frame")
        if text.startswith("callback"):
            tag = text.split()[1]
            if tag == "0038":      # DOS INT 21h callback: nine saved registers + PSP stack pointer (declared binding)
                regs9 = [r["ax"], r["bx"], r["cx"], r["dx"], r["si"], r["di"], r["bp"], r["ds"], r["es"]]
                for k, v in enumerate(regs9):
                    wr(i, sspb + ((sp - 18) & 0xFFFF) + 2 * k, 2, v, "dos-regsave", "dos-callback")
                wr(i, 0x8140 + 0x2E, 2, (sp - 18) & 0xFFFF, "dos-psp", "dos-callback")
                wr(i, 0x8140 + 0x30, 2, r["ss"], "dos-psp", "dos-callback")
            elif tag == "0008":    # BIOS IRQ0 clock callback: tick dword + 1
                tick = rdw(0x46C) | (rdw(0x46E) << 16)
                wr(i, 0x46C, 4, (tick + 1) & 0xFFFFFFFF, "bios-tick", "bios-callback")
            continue
        try:
            ins = next(md.disasm(byt, ip))
        except StopIteration:
            unknown["undecoded"] += 1
            continue
        mn, ops = ins.mnemonic, ins.operands
        if mn == "push":
            wr(i, sspb + ((sp - 2) & 0xFFFF), 2, val(r, ops[0]) & 0xFFFF, "push", cls)
        elif mn == "call":
            wr(i, sspb + ((sp - 2) & 0xFFFF), 2, (ip + ins.size) & 0xFFFF, "call", cls)
        elif mn == "lcall":
            wr(i, sspb + ((sp - 2) & 0xFFFF), 2, cs, "lcall", cls)
            wr(i, sspb + ((sp - 4) & 0xFFFF), 2, (ip + ins.size) & 0xFFFF, "lcall", cls)
        elif mn == "int":
            wr(i, sspb + ((sp - 2) & 0xFFFF), 2, flags, "int", cls)
            wr(i, sspb + ((sp - 4) & 0xFFFF), 2, cs, "int", cls)
            wr(i, sspb + ((sp - 6) & 0xFFFF), 2, (ip + ins.size) & 0xFFFF, "int", cls)
        elif mn in ("stosw", "stosb"):
            n = 2 if mn == "stosw" else 1
            wr(i, (r["es"] << 4) + r["di"], n, r["ax"] & ((1 << (8 * n)) - 1), "stos", cls)
        elif mn in ("movsw", "movsb", "pushf", "pusha", "rep stosw", "rep stosb") or ins.prefix[0] in (0xF2, 0xF3):
            unknown["unhandled:" + mn] += 1
        elif ops and ops[0].type == X.X86_OP_MEM and mn not in (
                "cmp", "test", "lea", "les", "lds", "mul", "imul", "div", "idiv", "jmp", "ljmp", "call", "lcall", "push") \
                and not mn.startswith("j"):
            a, size = ea(r, ops[0]), ops[0].size
            old = rd(a) if size == 1 else rdw(a)
            carry = flags & 1
            if mn == "mov":
                new = val(r, ops[1])
            elif mn in alu:
                new = alu[mn](old, val(r, ops[1]), carry)
            elif mn == "inc":
                new = old + 1
            elif mn == "dec":
                new = old - 1
            else:
                unknown["memdest:" + mn] += 1
                continue
            wr(i, a, size, new & ((1 << (8 * size)) - 1), mn, cls)
    check(not unknown, "unsupported store forms: " + str(dict(unknown)))
    return mem, stores


def main():
    check(len(sys.argv) in (2, 3), "usage: audit_slot_a_logl_030.py RUN030_DIR [RESIDUE_FILE_FROM_C_TEST]")
    run = Path(sys.argv[1])
    for name, digest in RUN_FILES.items():
        h = sha256()
        with open(run / name, "rb") as handle:
            for chunk in iter(lambda: handle.read(1 << 20), b""):
                h.update(chunk)
        check(h.hexdigest() == digest, name + " identity mismatch")
    for name in ("RAM-10D2-00F4.BIN", "RAM-10D2-0165.BIN", "RAM-10D2-13C6.BIN", "RAM-2E60-0747.BIN"):
        check((run / name).stat().st_size == RAM_SIZE, name + " size mismatch")
    try:
        import capstone  # noqa: F401
    except ImportError:
        raise SystemExit("capstone is required for the store decode (pip install capstone)")
    w1, w3, w0 = (parse(run / n) for n in ("LOGL-W1-00F4-to-0165.TXT", "LOGL-W3-13C6-to-0747.TXT",
                                           "LOGL-A-13A3-to-00F4.TXT"))
    check(len(w1) == W1_LINES, "W1 line count mismatch")
    polls, irqs = derive_inputs(w1, w3, w0)
    base = (run / "RAM-10D2-00F4.BIN").read_bytes()
    native = (run / "RAM-10D2-0165.BIN").read_bytes()
    mem, stores = replay(w1, base)
    check(len(stores) == STORES_TOTAL, "store count mismatch: %d" % len(stores))
    pred = bytearray(base)
    for a, v in mem.items():
        if a < len(pred):
            pred[a] = v
    mism = [i for i in range(RAM_SIZE) if pred[i] != native[i]]
    delta = [i for i in range(RAM_SIZE) if base[i] != native[i]]
    check(len(delta) == NATIVE_DELTA_BYTES, "native 00F4->0165 delta size mismatch")
    tick0, tick1 = (int.from_bytes(x[0x46C:0x470], "little") for x in (base, native))
    check((tick0, tick1) == (545918, 545920), "BIOS tick words mismatch")
    check(not mism, "store replay leaves %d unattributed byte(s)" % len(mism))
    # Non-vacuity: omitting each declared or untraced writer class must leave bytes unexplained.
    def omit(classes):
        alt = bytearray(base)
        last = {}
        for i, a, n, kind, cls in stores:
            for k in range(n):
                last[(a + k) & 0xFFFFF] = (i, cls)
        # rebuild the final value of each byte from the full replay, dropping bytes whose last writer is omitted
        for a, v in mem.items():
            if a < len(alt) and last[a][1] not in classes:
                alt[a] = v
        return sum(1 for i in range(RAM_SIZE) if alt[i] != native[i])
    controls = {c: omit({c}) for c in ("hw-frame", "dos-callback", "bios-callback", "irq0-hook", "rom-stub")}
    # The IRQ0 frames are real stores but every byte they write is overwritten before 0165, so omitting
    # them changes no final byte (reported, not required); every other class must be needed.
    check(controls["hw-frame"] == 0 and all(v > 0 for c, v in controls.items() if c != "hw-frame"),
          "writer-class omission controls unexpected: " + str(controls))
    writers = collections.defaultdict(set)
    for i, a, n, kind, cls in stores:
        for k in range(n):
            writers[(a + k) & 0xFFFFF].add(cls)
    by_class = collections.Counter()
    for a in delta:
        cl = writers[a]
        by_class["+".join(sorted(cl))] += 1
    check(all(writers[a] for a in delta), "a native delta byte has no replayed writer")
    print("polls %d (readings %s), IRQ0 entries %d (BIOS chain at #13 and #26, tick %d -> %d), stores %d, native delta %d, unattributed 0" %
          (polls, "/".join(str(x[0]) for x in READINGS), irqs, tick0, tick1, len(stores), len(delta)))
    print("omission controls (bytes left unexplained):", dict(controls))
    print("native delta bytes by writer class:", dict(by_class))
    if len(sys.argv) == 3:
        residue = [int(line.split()[0], 16) for line in Path(sys.argv[2]).read_text().splitlines() if line.strip()]
        unmodeled = {"hw-frame", "dos-callback", "bios-callback", "irq0-hook", "rom-stub"}
        orphans = [a for a in residue if not (writers[a] & unmodeled)]
        classes = collections.Counter("+".join(sorted(writers[a] & unmodeled)) or "NONE" for a in residue)
        print("C-replay residue %d byte(s) by unmodeled writer: %s; unattributed %d" % (len(residue), dict(classes), len(orphans)))
        check(not orphans, "residue bytes with no unmodeled writer: %d" % len(orphans))
    print("run-030 LOGL inputs, store replay and writer attribution verified")


main()
