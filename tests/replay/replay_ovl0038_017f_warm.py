#!/usr/bin/env python3
"""OVL_0038_017F, U38 slot 4 (warm call, run ovl38-017f-native-003): whole-trace store replay with a per-byte account.

Evidence only. Inputs under the ignored run directory: warm/parts/LOGCPU-part{1,2,3}.txt (the armed LOGL trace in three parts),
warm/S0..S9.BIN (entry RAM, 640 KiB), warm/exit/S0..S9.BIN (exit RAM). Rows are streamed with a one-row look-behind and
look-ahead. Stop-duplicate rule: the last row of each part is the stop line (logged before it executes); its executed copy is
the next part's first row, so the last row of every part is dropped. The final BP row (9FE0:0100) is also a stop line.
"""
import argparse
import collections
import json
import os
import re
import sys
from pathlib import Path

RAM_SIZE, VIDEO_BASE = 655360, 0xA0000
BDA_HEAD, BDA_TAIL, BDA_BUF, BDA_TICK = 0x41A, 0x41C, 0x400, 0x46C
MUT = os.environ.get("POOLRAD_017F_MUTANT", "")

LINE = re.compile(
    r"^(\d+\.\d+)\s+([0-9A-F]{4}):([0-9A-F]{8})\s+(.*?)\s{2,}((?:[0-9A-F]{2} )*[0-9A-F]{2})\s+"
    r"EAX:([0-9A-F]{8}) EBX:([0-9A-F]{8}) ECX:([0-9A-F]{8}) EDX:([0-9A-F]{8}) ESI:([0-9A-F]{8}) "
    r"EDI:([0-9A-F]{8}) EBP:([0-9A-F]{8}) ESP:([0-9A-F]{8}) DS:([0-9A-F]{4}) ES:([0-9A-F]{4}) "
    r"FS:([0-9A-F]{4}) GS:([0-9A-F]{4}) SS:([0-9A-F]{4}) CF:(\d) ZF:(\d) SF:(\d) OF:(\d) AF:(\d) "
    r"PF:(\d) IF:(\d) TF:(\d) VM:\d FLG:([0-9A-F]{8})")
ANNOT = re.compile(r"\b(ds|es|ss|cs|fs|gs):\[([0-9A-F]{4})\]=([0-9A-F]+)\b")


def check(condition, message):
    if not condition:
        raise SystemExit(message)


def parse_line(line, index):
    m = LINE.match(line)
    check(m, "unparsed trace line %d" % index)
    g = m.groups()
    regs = dict(zip(("ax", "bx", "cx", "dx", "si", "di", "bp", "sp"), [int(x, 16) & 0xFFFF for x in g[5:13]]))
    regs.update(ds=int(g[13], 16), es=int(g[14], 16), ss=int(g[17], 16), cs=int(g[1], 16))
    cf, zf, sf, of, af, pf, iff, tf = (int(x) for x in g[18:26])
    flags = ((int(g[26], 16) & 0xF000) | 0x0002 | cf | (pf << 2) | (af << 4) | (zf << 6) | (sf << 7) | (tf << 8)
             | (iff << 9) | (of << 11))
    # row: (cs, ip, text, bytes, regs, flags)
    return (int(g[1], 16), int(g[2], 16) & 0xFFFF, g[3], bytes(int(x, 16) for x in g[4].split()), regs, flags)


def stream(paths):
    """All rows of all parts, minus the last row of each part (the stop line)."""
    for p in paths:
        pending = None
        with open(p, "r") as handle:
            for n, line in enumerate(handle):
                row = parse_line(line, n)
                if pending is not None:
                    yield pending
                pending = row


def windowed(rows):
    prv, cur = None, next(rows, None)
    nxt = next(rows, None) if cur is not None else None
    i = 0
    while cur is not None:
        yield i, prv, cur, nxt
        prv, cur = cur, nxt
        nxt = next(rows, None)
        i += 1


def mnemonic(text):
    parts = text.split()
    return parts[0] if parts else ""


def replay(rows, base, notes, windows_out):
    import capstone
    from capstone import x86 as X
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_16)
    md.detail = True
    mem, vcount = {}, [0]
    jcount, written = collections.Counter(), set()
    taint = [False]
    label = ["body"]
    checks = collections.Counter()
    bad = collections.defaultdict(list)

    def rd(a):
        a &= 0xFFFFF
        if a >= VIDEO_BASE:
            notes["video-read"] += 1
            taint[0] = True
            return 0
        return mem.get(a, base[a])

    def rdw(a):
        return rd(a) | (rd(a + 1) << 8)

    active = [None]

    def wr(i, a, n, v, kind, cls):
        a &= 0xFFFFF
        for k in range(n):
            b, byte = (a + k) & 0xFFFFF, (v >> (8 * k)) & 255
            if b >= VIDEO_BASE:
                vcount[0] += 1
                continue
            mem[b] = byte
            if active[0] is not None:
                active[0]['writes'].append((b, byte))
            jcount[(cls, kind)] += 1
            written.add(b)

    ids = {X.X86_REG_AX: ("ax", 0xFFFF, 0), X.X86_REG_BX: ("bx", 0xFFFF, 0), X.X86_REG_CX: ("cx", 0xFFFF, 0),
           X.X86_REG_DX: ("dx", 0xFFFF, 0), X.X86_REG_SI: ("si", 0xFFFF, 0), X.X86_REG_DI: ("di", 0xFFFF, 0),
           X.X86_REG_BP: ("bp", 0xFFFF, 0), X.X86_REG_SP: ("sp", 0xFFFF, 0),
           X.X86_REG_AL: ("ax", 0xFF, 0), X.X86_REG_AH: ("ax", 0xFF, 8), X.X86_REG_BL: ("bx", 0xFF, 0),
           X.X86_REG_BH: ("bx", 0xFF, 8), X.X86_REG_CL: ("cx", 0xFF, 0), X.X86_REG_CH: ("cx", 0xFF, 8),
           X.X86_REG_DL: ("dx", 0xFF, 0), X.X86_REG_DH: ("dx", 0xFF, 8),
           X.X86_REG_DS: ("ds", 0xFFFF, 0), X.X86_REG_ES: ("es", 0xFFFF, 0), X.X86_REG_SS: ("ss", 0xFFFF, 0),
           X.X86_REG_CS: ("cs", 0xFFFF, 0)}

    def reg(r, rid):
        name, mask, shift = ids[rid]
        return (r[name] >> shift) & mask

    def ea(r, op):
        m = op.mem
        off = m.disp + (reg(r, m.base) if m.base else 0) + (reg(r, m.index) if m.index else 0)
        seg = reg(r, m.segment) if m.segment else (r["ss"] if m.base == X.X86_REG_BP else r["ds"])
        return ((seg << 4) + (off & 0xFFFF)) & 0xFFFFF

    def val(r, op, size):
        if op.type == X.X86_OP_REG:
            return reg(r, op.reg)
        if op.type == X.X86_OP_IMM:
            return op.imm & ((1 << (8 * size)) - 1)
        a = ea(r, op)
        return rd(a) if size == 1 else rdw(a)

    alu = {"add": lambda o, s, c: o + s, "sub": lambda o, s, c: o - s, "adc": lambda o, s, c: o + s + c,
           "sbb": lambda o, s, c: o - s - c, "and": lambda o, s, c: o & s, "or": lambda o, s, c: o | s,
           "xor": lambda o, s, c: o ^ s}
    branchy = ("call", "lcall", "jmp", "ljmp", "ret", "retf", "iret", "int", "loop", "loope", "loopne", "jcxz")
    nostore = ("cmp", "test", "lea", "les", "lds", "mul", "imul", "div", "idiv", "jmp", "ljmp", "call", "lcall", "push",
               "lodsb", "lodsw", "scasb", "scasw", "cmpsb", "cmpsw", "xlatb", "in", "out", "cbw", "cwd", "popf",
               "popfw", "sahf", "lahf", "iret", "iretw", "lret", "retf", "ret", "leave", "enter", "bound", "clc", "stc",
               "cli", "sti", "cld", "std", "cmc", "xlat")
    executed = collections.Counter()
    for i, prv, cur, nxt in windowed(rows):
        cs, ip, text, byt, r, flags = cur
        if active[0] is not None and cs == BODY_SEG_RT[0] and ip == active[0]['ret']:
            active[0]['regs'] = dict(r); active[0]['flags'] = flags; active[0]['sp'] = r['sp']
            windows_out.append(active[0]); active[0] = None
        sspb, sp = r["ss"] << 4, r["sp"]
        cls = "rom-stub" if cs == 0xF000 else "game"
        label[0] = "body" if cs == BODY_SEG_RT[0] else "cs:%04X" % cs
        taint[0] = False
        executed[label[0]] += 1
        m = ANNOT.search(text)
        if m and not text.startswith("callback"):
            seg, off, hexv = r[m.group(1)], int(m.group(2), 16), m.group(3)
            a = ((seg << 4) + off) & 0xFFFFF
            if a + len(hexv) // 2 > VIDEO_BASE:
                notes["annot-display"] += 1
            else:
                got = bytes(mem.get((a + k) & 0xFFFFF, base[(a + k) & 0xFFFFF]) for k in range(len(hexv) // 2))
                if got == int(hexv, 16).to_bytes(len(hexv) // 2, "little"):
                    checks["ann-match"] += 1
                else:
                    bad["ann-bad"].append(i)
        # hardware interrupt entry: the row after an instruction shows IP/CS/FLAGS pushed by the hardware
        if prv is not None and not prv[2].startswith("callback"):
            pm = mnemonic(prv[2])
            if pm not in branchy and not pm.startswith("j") and not pm.startswith("rep"):
                pins = next(md.disasm(prv[3], prv[1]), None)
                if pins is not None and (cs, ip) != (prv[0], (prv[1] + pins.size) & 0xFFFF):
                    notes["hw-irq-entry"] += 1
                    if MUT != "no-hw-frame":
                        wr(i, sspb + sp, 2, (prv[1] + pins.size) & 0xFFFF, "irq-frame", "hw-frame")
                        wr(i, sspb + sp + 2, 2, prv[0], "irq-frame", "hw-frame")
                        wr(i, sspb + sp + 4, 2, prv[5] | 0x0200, "irq-frame", "hw-frame")
        if text.startswith("callback"):
            tag = text.split()[1]
            nregs = nxt[4] if nxt is not None else r
            nflags = nxt[5] if nxt is not None else flags
            if tag == "0008":
                tick = rdw(BDA_TICK) | (rdw(BDA_TICK + 2) << 16)
                if MUT != "tick-skip":
                    wr(i, BDA_TICK, 4, (tick + 1) & 0xFFFFFFFF, "bios-tick", "bios-callback")
                checks["tick"] += 1
            elif tag == "0010":
                notes["callback-0010"] += 1
            else:
                raise SystemExit("undeclared callback %s at row %d" % (tag, i))
            continue
        ins = next(md.disasm(byt, ip), None)
        check(ins is not None and ins.addr_size == 2, "undecoded instruction at row %d" % i)
        mn, ops = ins.mnemonic, ins.operands
        rep, base_mn = mn.startswith("rep"), mn.split()[-1]
        if mn in ("pushf", "pushfw"):
            wr(i, sspb + ((sp - 2) & 0xFFFF), 2, flags & 0xFFFF, "pushf", cls)
        elif mn == "push":
            wr(i, sspb + ((sp - 2) & 0xFFFF), 2, val(r, ops[0], 2) & 0xFFFF, "push", cls)
        elif mn == "call":
            wr(i, sspb + ((sp - 2) & 0xFFFF), 2, (ip + ins.size) & 0xFFFF, "call", cls)
        elif mn == "lcall":
            wr(i, sspb + ((sp - 2) & 0xFFFF), 2, cs, "lcall", cls)
            wr(i, sspb + ((sp - 4) & 0xFFFF), 2, (ip + ins.size) & 0xFFFF, "lcall", cls)
        elif mn == "int":
            if MUT != "int-frame-skip":
                wr(i, sspb + ((sp - 2) & 0xFFFF), 2, flags, "int", "int-frame", cls)
                wr(i, sspb + ((sp - 4) & 0xFFFF), 2, cs, "int", "int-frame", cls)
                wr(i, sspb + ((sp - 6) & 0xFFFF), 2, (ip + ins.size) & 0xFFFF, "int", "int-frame", cls)
        elif base_mn in ("stosb", "stosw", "movsb", "movsw"):
            w = 2 if base_mn.endswith("w") else 1
            n = ((r["cx"] - nxt[4]["cx"]) & 0xFFFF) if rep else 1
            check(nxt is not None, "string row %d has no following row" % i)
            check((n >= 1 and (not rep or n <= r["cx"])) or (rep and r["cx"] == 0 and n == 0),
                  "rep chunk count out of range at row %d" % i)
            check(((nxt[4]["di"] - r["di"]) & 0xFFFF) == n * w, "DI progression disagrees at row %d" % i)
            sseg = reg(r, ops[1].mem.segment) if base_mn.startswith("movs") and ops[1].mem.segment else r["ds"]
            if MUT == "stos-skip" and base_mn.startswith("stos"):
                n = 0
            for k in range(n):
                dst = (r["es"] << 4) + ((r["di"] + k * w) & 0xFFFF)
                if base_mn.startswith("stos"):
                    wr(i, dst, w, r["ax"] & ((1 << (8 * w)) - 1), ("rep-" if rep else "") + "stos", cls)
                else:
                    src = (sseg << 4) + ((r["si"] + k * w) & 0xFFFF)
                    wr(i, dst, w, rd(src) | (rd(src + 1) << 8 if w == 2 else 0), ("rep-" if rep else "") + "movs", cls)
            checks["string-rows"] += 1
        elif mn in ("iret", "iretw"):
            check(nxt is not None, "iret row %d has no following row" % i)
            popped = (rdw(sspb + sp + 2), rdw(sspb + sp), rdw(sspb + sp + 4))
            want = (nxt[0], nxt[1], nxt[5] & 0xFFFF)
            if popped != want:
                bad["iret-bad"].append((i, popped, want))
            else:
                checks["iret-ok"] += 1
        elif mn == "pop" and ops[0].type == X.X86_OP_MEM:
            wr(i, ea(r, ops[0]), 2, rdw(sspb + sp), "pop-mem", cls)
        elif mn == "xchg":
            memops = [o for o in ops if o.type == X.X86_OP_MEM]
            if memops:
                other = [o for o in ops if o.type == X.X86_OP_REG][0]
                wr(i, ea(r, memops[0]), memops[0].size, val(r, other, memops[0].size), "xchg", cls)
        elif base_mn in nostore or mn.startswith("j") or (ops and ops[0].type != X.X86_OP_MEM):
            pass
        elif ops and ops[0].type == X.X86_OP_MEM:
            a, size = ea(r, ops[0]), ops[0].size
            check(size in (1, 2), "unsupported operand size at row %d" % i)
            old = rd(a) if size == 1 else rdw(a)
            if mn == "mov":
                new = val(r, ops[1], size)
            elif mn in alu:
                new = alu[mn](old, val(r, ops[1], size), flags & 1)
            elif mn in ("inc", "dec"):
                new = old + (1 if mn == "inc" else -1)
            elif mn == "neg":
                new = -old
            elif mn == "not":
                new = ~old
            elif mn in ("shl", "sal", "shr", "sar", "rol", "ror"):
                bits = 8 * size
                cnt = val(r, ops[1], 1) & 0x1F if len(ops) > 1 else 1
                mask = (1 << bits) - 1
                if cnt == 0:
                    new = old
                elif mn in ("shl", "sal"):
                    new = (old << cnt) & mask
                elif mn == "shr":
                    new = old >> cnt
                elif mn == "sar":
                    sv = old - (1 << bits) if old & (1 << (bits - 1)) else old
                    new = (sv >> cnt) & mask
                else:
                    c = cnt % bits
                    new = (((old << c) | (old >> (bits - c))) & mask) if mn == "rol" else (((old >> c) | (old << (bits - c))) & mask)
            else:
                raise SystemExit("unsupported memory-destination form %s at row %d" % (mn, i))
            wr(i, a, size, new & ((1 << (8 * size)) - 1), mn, cls)
        if mn == "lcall" and cs == BODY_SEG_RT[0]:
            active[0] = {'k': len(windows_out), 'ret': (ip + ins.size) & 0xFFFF, 'writes': []}
    return mem, jcount, written, vcount[0], checks, bad, executed


BODY_SEG_RT = [0]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--run", required=True, type=Path)
    ap.add_argument("--body-seg", required=True, type=lambda s: int(s, 16))
    ap.add_argument("--out", type=Path)
    ap.add_argument("--max-rows", type=int, default=0)
    a = ap.parse_args()
    BODY_SEG_RT[0] = a.body_seg
    run = a.run
    entry = b"".join((run / f"warm/S{i}.BIN").read_bytes() for i in range(10))
    exit_img = b"".join((run / f"warm/exit/S{i}.BIN").read_bytes() for i in range(10))
    check(len(entry) == RAM_SIZE and len(exit_img) == RAM_SIZE, "RAM images must be 640 KiB")
    parts = [run / f"warm/parts/LOGCPU-part{k}.txt" for k in (1, 2, 3)]
    notes = collections.Counter()
    import itertools
    rows = stream(parts)
    if a.max_rows:
        rows = itertools.islice(rows, a.max_rows)
    windows = []
    mem, jcount, written, vcount, checks, bad, executed = replay(rows, entry, notes, windows)
    eff = run / 'warm' / 'effects-017f.txt'
    with open(eff, 'w') as fh:
        for w in windows:
            fh.write('CALL %d\n' % w['k'])
            for addr, val in w['writes']:
                fh.write('W %05X %02X\n' % (addr, val))
            rr = w['regs']
            fh.write('R ' + ' '.join('%04X' % rr[x] for x in ('ax', 'bx', 'cx', 'dx', 'si', 'di', 'bp', 'es', 'ds')) + ' %04X\n' % w['flags'])
            fh.write('SP %04X\n' % w['sp'])
    notes['windows'] = len(windows)
    # the replayed image against the native exit image, below the video window
    diff = 0
    changed = 0
    unexplained = 0
    for addr in range(VIDEO_BASE):
        got = mem.get(addr, entry[addr])
        if exit_img[addr] != entry[addr]:
            changed += 1
        if got != exit_img[addr]:
            diff += 1
            if addr not in written:
                unexplained += 1
    summary = {"rows_executed_by_segment": dict(executed), "journal_bytes_by_class": {"%s/%s" % k: v for k, v in jcount.items()},
               "bytes_changed_entry_to_exit": changed, "replayed_vs_exit_diff_bytes": diff,
               "diff_without_writer": unexplained, "video_stores_counted": vcount,
               "checks": dict(checks), "bad": {k: len(v) for k, v in bad.items()}, "notes": dict(notes),
               "mutant": MUT or "none"}
    print(json.dumps(summary, indent=1, sort_keys=True))
    if a.out:
        a.out.write_text(json.dumps(summary, indent=1, sort_keys=True))
    return 0 if (diff == 0 and not any(bad.values())) else 1


if __name__ == "__main__":
    sys.exit(main())
