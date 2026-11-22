#!/usr/bin/env python3
"""OVL_007F_0294 (unit 0x7F body [0294,0441), 429 bytes) in a real native process: forced-call capture (run t578-ovl7f0294-040),
whole-trace store replay with a per-byte account, and the C composition hook (stage 2).

FORCED CALL, not natural reach. In the reviewed session the playable view was reached, the editor was entered from the stub, and a
debugger frame was written for a direct far call to 2F6B:0294 (return 9FE0:0100, colour 0Ah, prompt 9FE0:0000 'How much will you
take? ', max 64h). One armed LOGL traced the call from the entry to the RETF at 2F6B:043E. Three mid-call breakpoint stops at
2F6B:02FB were used to inject the keys 4, 2 and Enter into the BIOS keyboard buffer (declared host writes: the debugger's memory
writes are not traced). Every stop is a duplicated pair of trace rows (the stop line and the executed instruction, identical
registers); the replay executes one copy of each pair.

Declared host effects (the only ones; each is listed in the note with its evidence):
  * the debugger's key injections: buffer word at 0040:tail, then tail + 2 (the keys are the callback results of the trace);
  * INT 16h AH=00 (callback 002Eh): reads the buffer word at the head, head + 2 (wrap 1Eh..3Eh); the read value must equal the
    AX the trace shows after the callback; AH=01 is store-free and its ZF must equal (head == tail);
  * IRQ0 landing (callback 0008h at F000:FEA6): the BIOS tick word +1; the hardware frame (IP, CS, FLAGS at the post-push SP) is
    the only other host store (the rest of the handler is traced ROM code);
  * INT 1Ch (callback 0010h) is store-free; INT instructions and IRQ landings push their frames as traced.
No DOS call occurs in this trace, so no file binding is declared."""
from hashlib import sha256
from pathlib import Path
import collections
import os
import re
import sys

MUT = os.environ.get("POOLRAD_T578_MUTANT", "")
PINS = {
    "LOGCPU.TXT": "36259e90d7ef853fc6526e87791b5c8a5dddd76834916849228732f4533184b0",
    "RAM-2F6B-0294.BIN": "4c6280dd2fb66da131cc1a6fd790b0b23ba5ae6d5fc9d114da57543aca904762",
    "RAM-2F6B-043E.BIN": "19778b2cee78f6c1ded2ce5afad2c55b5bbe2caf2cff53f1c9e4b44df5c9567d",
    "CPU-2F6B-0294.raw.txt": "d9d8d865e60144d1a203a701756169de0eb1a1b4efab74fd2f3ea8e065f0ae25",
    "CPU-2F6B-043E.raw.txt": "3b822a44b76c42959a36f86a27da165e7464d07024b0a978b95fc6586357a892",
}
GAME_HASH = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
BODY_SHA = "66d7b9df95a5aad97b6d084354e56f72b70122a796018be24e48ab21605aafe8"
BODY_SEG, ENTRY, END, RETF_AT = 0x2F6B, 0x0294, 0x0441, 0x043E
BODY_LEN, UNIT = 429, 0x7F
LINES = 134363
RAM_SIZE, VIDEO_BASE = 655360, 0xA0000
KEYS = (0x0534, 0x0332, 0x1C0D)          # the three words the debugger injected (the trace returns them, checked below)
BDA_HEAD, BDA_TAIL, BDA_BUF, BDA_TICK = 0x41A, 0x41C, 0x400, 0x46C
STOP_IP = 0x02FB
DOS_DIR = Path(__file__).resolve().parent.parent.parent / "resources" / "DOS" / "POOLRAD"


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
    rows = []
    with open(path, "r") as handle:
        for line in handle:
            m = LINE.match(line)
            check(m, "unparsed trace line %d" % len(rows))
            g = m.groups()
            regs = dict(zip(("ax", "bx", "cx", "dx", "si", "di", "bp", "sp"), [int(x, 16) & 0xFFFF for x in g[5:13]]))
            regs.update(ds=int(g[13], 16), es=int(g[14], 16), ss=int(g[17], 16), cs=int(g[1], 16))
            cf, zf, sf, of, af, pf, iff, tf = (int(x) for x in g[18:26])
            flags = ((int(g[26], 16) & 0xF000) | 0x0002 | cf | (pf << 2) | (af << 4) | (zf << 6) | (sf << 7) | (tf << 8) | (iff << 9) | (of << 11))
            rows.append((float(g[0]), int(g[1], 16), int(g[2], 16) & 0xFFFF, g[3], bytes(int(x, 16) for x in g[4].split()), regs, flags))
    return rows


def raw_cpu(path):
    """The EV lines of cap.sh: 'AX BX CX DX SI DI BP SP CS IP DS ES SS | flags N' (machine output, not hand transcription)."""
    text = Path(path).read_text().strip()
    regs_part, _, flags_part = text.partition("| flags ")
    vals = [int(x, 16) for x in regs_part.split()]
    check(len(vals) == 13, "CPU raw line does not carry 13 register words")
    d = dict(zip(("ax", "bx", "cx", "dx", "si", "di", "bp", "sp", "cs", "ip", "ds", "es", "ss"), vals))
    d["flags"] = int(flags_part.strip(), 16) if flags_part.strip() else None
    check(d["flags"] is not None, "CPU raw line carries no flags")
    return d


def mnemonic(text):
    parts = text.split()
    return parts[0] if parts else ""


def stops(rows):
    """The first row of every identical adjacent pair at the stop address: the stop line of a duplicated stop."""
    out = []
    for i in range(len(rows) - 1):
        a, b = rows[i], rows[i + 1]
        if a[1] == BODY_SEG and a[2] == STOP_IP and (b[1], b[2]) == (a[1], a[2]) and a[4] == b[4] and a[5] == b[5] and a[6] == b[6]:
            out.append(i)
    return out


def replay(rows, base, skip_first_of_pair, host_at, notes):
    import capstone
    from capstone import x86 as X
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_16)
    md.detail = True
    mem, journal, vcount = {}, [], [0]
    taint = [False]
    label = ["body"]
    checks = {"ann-match": 0, "ann-bad": [], "kbd-read": 0, "kbd-bad": [], "zf-bad": [], "host-writes": 0, "peek": 0, "iret-ok": 0, "iret-bad": []}

    def rd(a):
        a &= 0xFFFFF
        if a >= VIDEO_BASE:
            notes["video-read"] += 1
            taint[0] = True
            return 0
        return mem.get(a, base[a])

    def rdw(a):
        return rd(a) | (rd(a + 1) << 8)

    def wr(i, a, n, v, kind, cls):
        a &= 0xFFFFF
        for k in range(n):
            b, byte = (a + k) & 0xFFFFF, (v >> (8 * k)) & 255
            if b >= VIDEO_BASE:
                vcount[0] += 1
                continue
            mem[b] = byte
            journal.append((i, b, byte, cls, kind, label[0]))

    ids = {X.X86_REG_AX: ("ax", 0xFFFF, 0), X.X86_REG_BX: ("bx", 0xFFFF, 0), X.X86_REG_CX: ("cx", 0xFFFF, 0), X.X86_REG_DX: ("dx", 0xFFFF, 0),
           X.X86_REG_SI: ("si", 0xFFFF, 0), X.X86_REG_DI: ("di", 0xFFFF, 0), X.X86_REG_BP: ("bp", 0xFFFF, 0), X.X86_REG_SP: ("sp", 0xFFFF, 0),
           X.X86_REG_AL: ("ax", 0xFF, 0), X.X86_REG_AH: ("ax", 0xFF, 8), X.X86_REG_BL: ("bx", 0xFF, 0), X.X86_REG_BH: ("bx", 0xFF, 8),
           X.X86_REG_CL: ("cx", 0xFF, 0), X.X86_REG_CH: ("cx", 0xFF, 8), X.X86_REG_DL: ("dx", 0xFF, 0), X.X86_REG_DH: ("dx", 0xFF, 8),
           X.X86_REG_DS: ("ds", 0xFFFF, 0), X.X86_REG_ES: ("es", 0xFFFF, 0), X.X86_REG_SS: ("ss", 0xFFFF, 0), X.X86_REG_CS: ("cs", 0xFFFF, 0)}

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

    alu = {"add": lambda o, s, c: o + s, "sub": lambda o, s, c: o - s, "adc": lambda o, s, c: o + s + c, "sbb": lambda o, s, c: o - s - c,
           "and": lambda o, s, c: o & s, "or": lambda o, s, c: o | s, "xor": lambda o, s, c: o ^ s}
    branchy = ("call", "lcall", "jmp", "ljmp", "ret", "retf", "iret", "int", "loop", "loope", "loopne", "jcxz")
    nostore = ("cmp", "test", "lea", "les", "lds", "mul", "imul", "div", "idiv", "jmp", "ljmp", "call", "lcall", "push", "lodsb", "lodsw",
               "scasb", "scasw", "cmpsb", "cmpsw", "xlatb", "in", "out", "cbw", "cwd", "popf", "popfw", "sahf", "lahf", "iret", "iretw",
               "lret", "retf", "ret", "leave", "enter", "bound", "clc", "stc", "cli", "sti", "cld", "std", "cmc", "xlat")
    rows_n = len(rows)
    for i, (ts, cs, ip, text, byt, r, flags) in enumerate(rows):
        if i in host_at:
            for a, v, nbytes in host_at[i]:
                wr(i, a, nbytes, v, "debugger-inject", "host")
                checks["host-writes"] += 1
        if skip_first_of_pair and i in skip_first_of_pair:
            continue
        sspb, sp = r["ss"] << 4, r["sp"]
        cls = "rom-stub" if cs == 0xF000 else "game"
        if cs == BODY_SEG:
            label[0] = "body"
        else:
            label[0] = "cs:%04X" % cs
        taint[0] = False
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
                    checks["ann-bad"].append(i)
        # hardware interrupt entry: the landing row shows the state AFTER the hardware pushed IP, CS, FLAGS at [SP], [SP+2], [SP+4]
        if i > 0 and (i - 1) not in (skip_first_of_pair or ()):
            pcs, pip, ptext, pbyt, prow, pflags = rows[i - 1][1], rows[i - 1][2], rows[i - 1][3], rows[i - 1][4], rows[i - 1][5], rows[i - 1][6]
            pm = mnemonic(ptext)
            if not ptext.startswith("callback") and pm not in branchy and not pm.startswith("j") and not pm.startswith("rep"):
                pins = next(md.disasm(pbyt, pip), None)
                if pins is not None and (cs, ip) != (pcs, (pip + pins.size) & 0xFFFF):
                    notes["hw-irq-entry"] += 1
                    if MUT != "no-hw-frame":
                        wr(i, sspb + sp, 2, (pip + pins.size) & 0xFFFF, "irq-frame", "hw-frame")
                        wr(i, sspb + sp + 2, 2, pcs, "irq-frame", "hw-frame")
                        wr(i, sspb + sp + 4, 2, pflags | 0x0200, "irq-frame", "hw-frame")
        if text.startswith("callback"):
            tag = text.split()[1]
            nxt = rows[i + 1][5] if i + 1 < rows_n else r
            nflags = rows[i + 1][6] if i + 1 < rows_n else flags
            if tag == "002E":
                ah = r["ax"] >> 8
                head, tail = rdw(BDA_HEAD), rdw(BDA_TAIL)
                if ah == 0x00:
                    val_at = rdw(BDA_BUF + head)
                    if val_at != nxt["ax"] or head == tail:
                        checks["kbd-bad"].append((i, hex(val_at), hex(nxt["ax"])))
                    else:
                        checks["kbd-read"] += 1
                    nh = head + 2 if head + 2 < 0x3E else 0x1E
                    if MUT != "kbd-skip":
                        wr(i, BDA_HEAD, 2, nh, "bios-kbd-head", "bios-callback")
                elif ah == 0x01:
                    if bool(nflags & 0x40) != (head == tail):
                        checks["zf-bad"].append(i)
                    else:
                        checks["peek"] += 1
                else:
                    raise SystemExit("undeclared keyboard function AH=%02X at row %d" % (ah, i))
            elif tag == "0008":
                tick = rdw(BDA_TICK) | (rdw(BDA_TICK + 2) << 16)
                if MUT != "tick-skip":
                    wr(i, BDA_TICK, 4, (tick + 1) & 0xFFFFFFFF, "bios-tick", "bios-callback")
            elif tag == "0010":
                notes["callback-0010"] += 1
            else:
                raise SystemExit("undeclared callback " + tag)
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
                wr(i, sspb + ((sp - 2) & 0xFFFF), 2, flags, "int", "int-frame")
                wr(i, sspb + ((sp - 4) & 0xFFFF), 2, cs, "int", "int-frame")
                wr(i, sspb + ((sp - 6) & 0xFFFF), 2, (ip + ins.size) & 0xFFFF, "int", "int-frame")
        elif base_mn in ("stosb", "stosw", "movsb", "movsw"):
            w = 2 if base_mn.endswith("w") else 1
            nxt = rows[i + 1][5]
            n = ((r["cx"] - nxt["cx"]) & 0xFFFF) if rep else 1
            check((n >= 1 and (not rep or n <= r["cx"])) or (rep and r["cx"] == 0 and n == 0), "rep chunk count out of range at row %d" % i)
            check(((nxt["di"] - r["di"]) & 0xFFFF) == n * w, "DI progression disagrees at row %d" % i)
            sseg = reg(r, ops[1].mem.segment) if base_mn.startswith("movs") and ops[1].mem.segment else r["ds"]
            for k in range(n):
                dst = (r["es"] << 4) + ((r["di"] + k * w) & 0xFFFF)
                if base_mn.startswith("stos"):
                    wr(i, dst, w, r["ax"] & ((1 << (8 * w)) - 1), ("rep-" if rep else "") + "stos", cls)
                else:
                    src = (sseg << 4) + ((r["si"] + k * w) & 0xFFFF)
                    wr(i, dst, w, rd(src) | (rd(src + 1) << 8 if w == 2 else 0), ("rep-" if rep else "") + "movs", cls)
        elif mn in ("iret", "iretw"):
            # the IRET pops IP, CS and FLAGS from the frame in memory: they must be the next trace row's CS:IP and FLAGS
            nxt = rows[i + 1]
            popped = (rdw(sspb + sp + 2), rdw(sspb + sp), rdw(sspb + sp + 4))
            if popped != (nxt[1], nxt[2], nxt[6] & 0xFFFF):
                checks["iret-bad"].append((i, popped, (nxt[1], nxt[2], nxt[6] & 0xFFFF)))
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
                    new = ((old << c) | (old >> (bits - c))) & mask if mn == "rol" else ((old >> c) | (old << (bits - c))) & mask
            else:
                raise SystemExit("unsupported memory-destination form %s at row %d" % (mn, i))
            wr(i, a, size, new & ((1 << (8 * size)) - 1), mn, cls)
    check(notes["video-read-to-ram"] == 0, "a display read feeds a store into the 640 KiB image")
    return mem, journal, vcount[0], notes, checks


def body_checks(ovr, entry_img, rows):
    """Re-derivation of the 429-byte body from GAME.OVR: identity, decode, loaded-image equality except the 17 loader words,
    and the executed instruction starts of this trace."""
    import capstone
    sys.path.insert(0, str(Path(__file__).resolve().parent.parent.parent / "tools" / "overlay"))
    from overlay_inventory import find_units, parse_mz
    exe = (DOS_DIR / "START.EXE").read_bytes()
    unit = next(u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr) if u["stub_para"] == UNIT)
    base = unit["ovr_offset"]
    span = ovr[base + ENTRY:base + END]
    check(len(span) == BODY_LEN and sha256(span).hexdigest() == BODY_SHA, "body bytes differ from the pinned SHA")
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_16)
    ins = list(md.disasm(span, ENTRY))
    check(sum(i.size for i in ins) == BODY_LEN, "decode does not tile the body")
    starts = {i.address for i in ins}
    fix = sorted(f for f in unit["fixups"] if ENTRY <= f < END)
    fixmask = set()
    for f in fix:
        fixmask.update((f - ENTRY, f - ENTRY + 1))
    loaded = entry_img[(BODY_SEG << 4) + ENTRY:(BODY_SEG << 4) + END]
    diff = [k for k in range(BODY_LEN) if loaded[k] != span[k] and k not in fixmask]
    check(not diff, "the loaded body differs from GAME.OVR outside the loader fixup words: %s" % diff[:8])
    executed = set()
    for i, r in enumerate(rows):
        if r[1] == BODY_SEG and ENTRY <= r[2] < END:
            executed.add(r[2])
    unexec = sorted(starts - executed)
    print("body: %d bytes SHA ok, %d decoded instructions, %d loader fixup words (%s); loaded image equal to GAME.OVR outside them"
          % (BODY_LEN, len(ins), len(fix), "far-call segment words" if all(i.mnemonic == "lcall" for i in ins if i.address + 3 in fix) else "mixed"))
    print("executed instruction starts in this call: %d of %d (not executed: %d)" % (len(executed & starts), len(starts), len(unexec)))
    return {"starts": len(starts), "executed": len(executed & starts), "unexecuted": unexec, "fixups": fix, "span": span, "ins": ins}


MODEL_MUTANTS = {
    "m-enter-0e": ("arith(c, *b(ram, c, -0x3C), 0x0Du, 1, 8u);", "arith(c, *b(ram, c, -0x3C), 0x0Eu, 1, 8u);"),
    "m-esc-1c": ("arith(c, *b(ram, c, -0x3C), 0x1Bu, 1, 8u);                    /* 03F9", "arith(c, *b(ram, c, -0x3C), 0x1Cu, 1, 8u);                    /* 03F9"),
    "m-clamp-always": ("if (!flag(c, ZF) && flag(c, SF) == flag(c, OF)) over = 0;", "if (!flag(c, ZF) && flag(c, SF) == flag(c, OF)) over = 1;"),
    "m-result-plus1": ("wr16(ram, c->ss, (uint16_t)(c->bp - 0x44), c->ax);            /* 042F", "wr16(ram, c->ss, (uint16_t)(c->bp - 0x44), (uint16_t)(c->ax + 1u));            /* 042F"),
    "m-bs-inc": ("*b(ram, c, -0x2C) = (uint8_t)incdec8(c, *b(ram, c, -0x2C), 1);", "*b(ram, c, -0x2C) = (uint8_t)incdec8(c, *b(ram, c, -0x2C), 0);"),
    "m-limit-plus1": ("if (loops == env->key_limit)", "if (loops == env->key_limit + 1u)"),
    "m-space-027": ("push_al(ram, c, 0x00); push_al(ram, c, 0x18); push_al(ram, c, 0x00); push_al(ram, c, 0x28);   /* 0402", "push_al(ram, c, 0x00); push_al(ram, c, 0x18); push_al(ram, c, 0x00); push_al(ram, c, 0x27);   /* 0402"),
    "m-colour-0e": ("push_al(ram, c, 0x18); push_al(ram, c, 0x0F);", "push_al(ram, c, 0x18); push_al(ram, c, 0x0E);"),
    "m-frame-size": ("lea_push(ram, c, -0x244);", "lea_push(ram, c, -0x243);"),
}


def build_mutant(U, build_dir, label):
    """The 0294 model compiled from a copy with one anchored source edit (the anchor must occur exactly once)."""
    import ctypes
    import subprocess
    root = Path(__file__).resolve().parent.parent.parent
    old, new = MODEL_MUTANTS[label]
    text = (root / "src/recovered/overlay_u7f_number_0294_cpu.c").read_text()
    check(text.count(old) == 1, "model mutant %s: its anchor does not occur exactly once" % label)
    mdir = build_dir / "mutant-src" / label
    mdir.mkdir(parents=True, exist_ok=True)
    (mdir / "overlay_u7f_number_0294_cpu.c").write_text(text.replace(old, new))
    path = build_dir / ("number-%s.so" % label)
    cmd = ["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2", "-fPIC", "-shared", "-I", str(root / "include")] + \
          [str(mdir / "overlay_u7f_number_0294_cpu.c") if x == "overlay_u7f_number_0294_cpu" else str(root / ("src/recovered/%s.c" % x)) for x in U.SOURCES] + \
          ["-o", str(path)]
    subprocess.run(cmd, check=True)
    lib = ctypes.CDLL(str(path))
    f = lib.pr_u7f_number_0294_cpu
    f.argtypes = [ctypes.POINTER(U.P.Bus), ctypes.c_size_t, ctypes.POINTER(U.Cpu), ctypes.POINTER(U.Env), ctypes.POINTER(U.Diag)]
    f.restype = ctypes.c_int
    lib.pr_ega_reset.argtypes = [ctypes.POINTER(U.P.Ega)]
    return lib


HOST = ("host", "bios-callback", "int-frame", "hw-frame", "rom-stub")


def compose(run, rows, entry, exit_img, journal, build_dir, video_native):
    """C. The recovered C model, pr_u7f_number_0294_cpu, run from the native entry state (CPU14 from the raw EV lines, the native
    640 KiB image, zeros above A0000 never observed) with the keys delivered in the trace's three typing groups (the keyboard
    results the trace shows). Compared with the native return: CPU14, the key calls, the OUT events, the display write count,
    and RAM below A0000 after the declared host effects (the BIOS keyboard and tick words, the INT and IRQ0 frames and the ROM
    handler's pushes, which the C model does not execute) are applied."""
    root = Path(__file__).resolve().parent.parent.parent
    sys.path.insert(0, str(root / "tools" / "overlay"))
    import u7f_number_0294_cpu_audit as U
    build_dir.mkdir(parents=True, exist_ok=True)
    lib = build_mutant(U, build_dir, MUT) if MUT.startswith("m-") else U.library(build_dir)
    if MUT.startswith("m-"):
        print("MODEL MUTANT BUILT:", MUT)
    fields = raw_cpu(run / "CPU-2F6B-0294.raw.txt")
    if MUT == "comp-entry-cf":
        fields["flags"] ^= 0x0001
    cpu = U.Cpu(fields["ax"], fields["bx"], fields["cx"], fields["dx"], fields["si"], fields["di"], fields["bp"], fields["sp"],
                fields["cs"], fields["ip"], fields["ds"], fields["es"], fields["ss"], fields["flags"])
    ram = bytearray(U.SIZE)
    ram[:RAM_SIZE] = entry
    envspec = dict(table={8: (0x0123, 0xF000)}, in61=0x30, time_start=4736147, time_step=7)
    groups = [[k] for k in KEYS] if MUT != "comp-keys-merged" else [list(KEYS)]
    status, actual, cram, env, d, events, writes, ega = U.run_c(lib, ram, cpu, envspec, groups, key_limit=64)
    status = str(status)
    check(status == "RETURN", "C model status %s, not RETURN" % status)
    want_pre = raw_cpu(run / "CPU-2F6B-043E.raw.txt")
    # The return stop is BEFORE the RETF 8 (the trace's last row, not executed): the post-RETF state is derived from the frame words
    # the return image holds at SS:SP (checked here) and the RETF's own semantics (IP, CS popped; SP += 4 + 8).
    imm = int(rows[-1][3].split()[1], 16)
    check(rows[-1][3].split()[0] == "retf" and imm == 8, "the last row is not RETF 8")
    sp0, ss0 = want_pre["sp"], want_pre["ss"]
    w = lambda a: exit_img[a] | (exit_img[a + 1] << 8)
    ret_ip, ret_cs = w((ss0 << 4) + sp0), w((ss0 << 4) + sp0 + 2)
    check((ret_ip, ret_cs) == (0x0100, 0x9FE0), "the frame words in the return image are not the debugger's return address")
    want = dict(want_pre, ip=ret_ip, cs=ret_cs, sp=(sp0 + 4 + imm) & 0xFFFF)
    print("post-RETF state derived: CS:IP %04X:%04X, SP %04X (frame words at SS:SP in the return image checked)" % (ret_cs, ret_ip, want["sp"]))
    got = {"ax": actual.ax, "bx": actual.bx, "cx": actual.cx, "dx": actual.dx, "si": actual.si, "di": actual.di, "bp": actual.bp,
           "sp": actual.sp, "cs": actual.cs, "ip": actual.ip, "ds": actual.ds, "es": actual.es, "ss": actual.ss, "flags": actual.flags}
    cpu_diff = sorted(k for k in want if want[k] != got[k])
    print("C composition from the native entry: status %s; CPU14 differing fields vs native return: %s" % (status, cpu_diff or "none"))
    cram = cram[:RAM_SIZE]
    diff = [x for x in range(RAM_SIZE) if cram[x] != exit_img[x]]
    last = {}
    for i, a, byte, cls, kind, lab in journal:
        last[a] = (cls, byte)
    host_last = {a for a, (cls, _) in last.items() if cls in HOST}
    outside = [x for x in diff if x not in host_last]
    healed = bytearray(cram)
    for a, (cls, byte) in last.items():
        if cls in HOST:
            healed[a] = byte
    after = [x for x in range(RAM_SIZE) if healed[x] != exit_img[x]]
    by_cls = collections.Counter(last[x][0] for x in diff if x in last)
    print("C RAM vs native return image: %d differing bytes (by declared host class: %s); outside the host set: %d; after applying the declared "
          "host effects: %d differing bytes" % (len(diff), dict(by_cls), len(outside), len(after)))
    if outside:
        print("  outside addresses:", [hex(x) for x in outside][:16])
    # key calls: the C model's keyboard log against the trace's INT 16h callbacks
    c_keys = [(e[1], e[2]) for e in env.log if e[0] == "16"]
    n_keys = []
    for i, r in enumerate(rows):
        if r[3].split()[:2] == ["int", "16"]:
            ah = r[5]["ax"] >> 8
            ret = rows[i + 3][5]["ax"]          # int, sti, callback, iret rows: AX after the callback (the iret row)
            n_keys.append((ah, ret if ah == 0 else None))
    n_keys = [(0 if ah == 0 else 1, k) for ah, k in n_keys]
    c_norm = [(0 if f == 0 else 1, (k if f == 0 else None)) for f, k in c_keys]
    print("keyboard calls: C %d, trace %d; equal (function, key): %s" % (len(c_norm), len(n_keys), c_norm == n_keys))
    check(c_norm == n_keys, "keyboard call sequence differs: C %s trace %s" % (c_norm, n_keys))
    n_out = [(rr[5]["dx"], rr[5]["ax"] & 0xFF) for rr in rows if rr[3].split()[:1] == ["out"] and rr[1] != 0xF000]
    c_out = [(e[0], e[1]) for e in events]
    if MUT == "comp-out-drop":
        n_out = n_out[:-1]
    print("OUT sequence: C %d events, native game-code `out` rows %d, equal: %s" % (len(c_out), len(n_out), c_out == n_out))
    print("display writes: C %d bus writes; native display byte stores counted by the replay: %d" % (len(writes), video_native))
    check(len(writes) == video_native, "C display write count %d differs from the native display store count %d" % (len(writes), video_native))
    check(not outside and not after, "C RAM differs from native outside the declared host effects")
    check(cpu_diff == [], "composed CPU14 differs from the native return")
    check(c_out == n_out, "composed OUT sequence differs from the native trace")
    return dict(cpu_diff=cpu_diff, diff=len(diff), outside=len(outside), healed=len(after), out=len(c_out), writes=len(writes), keys=len(c_norm))


def main():
    check(len(sys.argv) in (3, 4), "usage: audit_slot_a_ovl7f0294_native_t578.py RUN040_DIR GAME.OVR [BUILD_DIR]")
    run, ovr_path = Path(sys.argv[1]), Path(sys.argv[2])
    for name, digest in PINS.items():
        check(sha256((run / name).read_bytes()).hexdigest() == digest, name + " identity mismatch")
    ovr = ovr_path.read_bytes()
    check(sha256(ovr).hexdigest() == GAME_HASH, "GAME.OVR identity mismatch")
    rows = parse(run / "LOGCPU.TXT")
    check(len(rows) == LINES, "trace line count %d, not %d" % (len(rows), LINES))
    check((rows[0][1], rows[0][2]) == (BODY_SEG, ENTRY) and (rows[-1][1], rows[-1][2]) == (BODY_SEG, RETF_AT), "trace first/last rows differ")
    entry = (run / "RAM-2F6B-0294.BIN").read_bytes()
    exit_img = (run / "RAM-2F6B-043E.BIN").read_bytes()
    for row, name in ((rows[0], "CPU-2F6B-0294.raw.txt"), (rows[-1], "CPU-2F6B-043E.raw.txt")):
        want = raw_cpu(run / name)
        got = dict(row[5])
        got.update(cs=row[1], ip=row[2], flags=row[6])
        bad = [k for k in want if k in got and want[k] != got[k]]
        check(not bad, "%s disagrees with the trace in %s" % (name, bad))
    print("CPU raw files (EV output): 13 register words and flags equal the trace's first and last rows")
    body = body_checks(ovr, entry, rows)
    st = stops(rows)
    print("breakpoint stops at %04X:%04X: %d (each a duplicated pair; one copy replayed): rows %s" % (BODY_SEG, STOP_IP, len(st), st))
    check(len(st) == len(KEYS), "expected %d stops, found %d" % (len(KEYS), len(st)))
    skip = set(st) if MUT != "pair-dup" else set()
    # the three debugger key injections: BDA tail (from the entry image) and the buffer word, written at the stop before the key is read
    tail = int.from_bytes(entry[BDA_TAIL:BDA_TAIL + 2], "little")
    host_at = {}
    for k, stop in enumerate(st):
        writes = [(BDA_BUF + tail, KEYS[k], 2)]
        tail = tail + 2 if tail + 2 < 0x3E else 0x1E
        writes.append((BDA_TAIL, tail, 2))
        host_at[stop] = writes
    if MUT == "host-inject-skip":
        host_at = {}
    if MUT == "host-tail-skip":
        host_at = {s_: [w for w in ws if w[0] != BDA_TAIL] for s_, ws in host_at.items()}
    notes = collections.Counter()
    mem, journal, video, notes, checks = replay(rows, entry, skip, host_at, notes)
    check(not checks["ann-bad"], "trace read annotations disagree with replayed memory at %d row(s)" % len(checks["ann-bad"]))
    check(not checks["kbd-bad"], "keyboard callback values disagree with the replayed buffer: %s" % checks["kbd-bad"][:3])
    check(not checks["zf-bad"], "keyboard AH=01 ZF disagrees with head==tail at %s" % checks["zf-bad"][:3])
    check(not checks["iret-bad"], "an IRET pops a frame that disagrees with the next trace row at %d place(s): %s"
          % (len(checks["iret-bad"]), checks["iret-bad"][:2]))
    print("IRET frames: %d popped words agree with the next trace row (%d disagree)" % (checks["iret-ok"], len(checks["iret-bad"])))
    print("store replay: %d byte stores journaled; %d trace read annotations match the shadow memory (%d mismatches); %d keyboard reads "
          "match the buffer (%d mismatches); %d peeks have ZF = (head == tail) (%d mismatches); %d host writes; %d display stores counted, "
          "not compared; hw IRQ entries %d; INT 1Ch callbacks %d" %
          (len(journal), checks["ann-match"], len(checks["ann-bad"]), checks["kbd-read"], len(checks["kbd-bad"]), checks["peek"], len(checks["zf-bad"]),
           checks["host-writes"], video, notes["hw-irq-entry"], notes["callback-0010"]))
    pred = bytearray(entry)
    for a, v in mem.items():
        pred[a] = v
    delta = [x for x in range(RAM_SIZE) if entry[x] != exit_img[x]]
    mism = [x for x in range(RAM_SIZE) if pred[x] != exit_img[x]]
    writers = {}
    for i, a, byte, cls, kind, lab in journal:
        writers[a] = (cls, kind, lab, i)
    explained = [x for x in delta if x in writers and pred[x] == exit_img[x]]
    unattributed = [x for x in delta if x not in writers]
    wrong = [x for x in delta if x in writers and pred[x] != exit_img[x]]
    spurious = [x for x in mism if entry[x] == exit_img[x]]
    print("per-byte account of the %d native delta bytes (entry -> return image): explained %d, writer-but-wrong-value %d, UNATTRIBUTED %d, "
          "spurious %d; replayed image vs native: %d mismatching byte(s)" % (len(delta), len(explained), len(wrong), len(unattributed), len(spurious), len(mism)))
    by_cls = collections.Counter(writers[x][0] for x in explained)
    by_kind = collections.Counter(writers[x][1] for x in explained)
    print("explained delta bytes by class:", dict(sorted(by_cls.items())))
    print("explained delta bytes by last writing instruction kind:", dict(sorted(by_kind.items())))
    if unattributed or mism or wrong:
        print("unattributed addresses:", [hex(x) for x in unattributed][:24])
        print("mismatch addresses:", [hex(x) for x in mism][:24])
    check(not unattributed and not mism and not wrong and not spurious, "the replay does not account for every native delta byte")
    print("ACCOUNT CLOSED: 0 unattributed, 0 mismatching bytes")
    if len(sys.argv) == 4:
        compose(run, rows, entry, exit_img, journal, Path(sys.argv[3]), video)
    return dict(rows=rows, entry=entry, exit=exit_img, body=body, journal=journal, pred=pred, run=run, stops=st)


if __name__ == "__main__":
    main()
