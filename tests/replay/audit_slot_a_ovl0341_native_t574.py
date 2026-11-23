#!/usr/bin/env python3
"""OVL_0025_0341 (unit 0x25 [0341,0411)) in a real native process: re-derivation, whole-trace store replay, C composition.

Print no game or RAM payload.  A FORCED CALL, not natural reach: in a real run of the game (saved slot A loaded, playable view) the gate
DS:[49F8] is 1, so the gameplay loop never calls this function; the debugger therefore pushed a far-call frame (return address = the stop,
block id = the real map id 14h) and set CS:IP to the resident body 308A:0341 (the unit-0x25 entry-4 stub 084F:0034 holds EA 41 03 8A 30).
`LOGL` traced the whole call (one pass, 165,373 lines) to its return breakpoint; CPU14 and the ten-segment RAM image were captured at entry
and at return.  This audit locks those files and does:
  A. an independent re-derivation of the 208-byte body from GAME.OVR (decode, instruction/far-call/branch counts, body SHA, the loaded
     bytes of the native image vs the file with every relocated word accounted, the executed addresses of the trace);
  B. a store replay of the whole trace over the native entry image with declared host writers (DOS INT 21h open/read/seek/close bound to
     the game directory's files, BIOS tick, hardware IRQ frames, INT 1Ch, INT 15h and keyboard callbacks store-free), compared byte by
     byte with the native exit image, with writers attributed to the body's own code and to each callee.
Needs capstone; run without `python3 -I`.  `POOLRAD_T574_MUTANT=<name>` perturbs one branch (see the check script)."""
from hashlib import sha256
from pathlib import Path
import collections
import os
import re
import sys

MUT = os.environ.get("POOLRAD_T574_MUTANT", "")
PINS = {
    "LOGL-CALL-308A-0341.TXT": "8a97b6060b394336b2426c81d77135dd7953376617ed8a6179773a3ed402342b",
    "RAM-308A-0341.BIN": "12f3a96ca8ee49a46ed1546f0ac4ee87f900614e3d9198fbf0b19c6573381d71",
    "RAM-1264-0302.BIN": "d35faaa931f7e5a3b686a499205fa44905b0d0e5e6add05124078a077e72e65c",
    "CPU-308A-0341.TXT": "e05b334fe232f03a9e9c51d9580c888ca7157841e356a434ee1b2bdb8c1d2b0b",
    "CPU-1264-0302.TXT": "e3c1264ea9db4bf4e34c09acf1f952c37b62017c54433831b49838cc0717148d",
}
GAME_HASH = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
BODY_SHA = "b8d29a86b654e5fd944e52f2b36335cc891fc25395196501926abb581cb44e18"
BODY_FILE, BODY_LEN, BODY_SEG, ENTRY, RETF_AT, RELOC = 30740, 208, 0x308A, 0x0341, 0x040E, 0x0824
RAM_SIZE, VIDEO_BASE = 655360, 0xA0000
SITES = {0x0355: "fill", 0x035F: "rowclear", 0x0377: "strload", 0x037C: "print", 0x0396: "str", 0x03A6: "strload",
         0x03B0: "strcat", 0x03C0: "strload", 0x03D3: "load", 0x03F3: "move", 0x0401: "freemem", 0x0406: "rowclear"}
LINES = 165373
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
            check(m, "unparsed trace line")
            g = m.groups()
            regs = dict(zip(("ax", "bx", "cx", "dx", "si", "di", "bp", "sp"), [int(x, 16) & 0xFFFF for x in g[5:13]]))
            regs.update(ds=int(g[13], 16), es=int(g[14], 16), ss=int(g[17], 16), cs=int(g[1], 16))
            cf, zf, sf, of, af, pf, iff, tf = (int(x) for x in g[18:26])
            flags = ((int(g[26], 16) & 0xF000) | 0x0002 | cf | (pf << 2) | (af << 4) | (zf << 6) | (sf << 7) | (tf << 8) | (iff << 9) | (of << 11))
            rows.append((float(g[0]), int(g[1], 16), int(g[2], 16) & 0xFFFF, g[3], bytes(int(x, 16) for x in g[4].split()), regs, flags))
    return rows


def mnemonic(text):
    parts = text.split()
    return parts[0] if parts else ""


def cpu_fields(path):
    d = {}
    for part in Path(path).read_text().split():
        name, _, value = part.partition("=")
        d[name.lower()] = int(value, 16)
    check(len(d) == 14, "CPU file is not 14 fields")
    return d


def rederive(ovr, entry_img, rows):
    """A. The body decoded independently of the model: counts, SHA, relocated words, executed addresses, branch outcome."""
    import capstone
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_16)
    md.detail = True
    span = ovr[BODY_FILE:BODY_FILE + BODY_LEN]
    check(sha256(span).hexdigest() == BODY_SHA, "body bytes differ from the pinned SHA")
    ins = list(md.disasm(span, ENTRY))
    check(sum(i.size for i in ins) == BODY_LEN, "decode does not tile the 208-byte span")
    starts = [i.address for i in ins]
    far = [i for i in ins if i.bytes[0] == 0x9A]
    jcc = [i for i in ins if i.mnemonic.startswith("j") and i.mnemonic != "jmp"]
    rets = [i for i in ins if i.mnemonic.startswith("ret")]
    check(len(ins) == 89 and len(far) == 12 and len(jcc) == 1 and jcc[0].mnemonic == "jb" and jcc[0].address == 0x03DC and
          len(rets) == 1 and rets[0].mnemonic == "retf" and rets[0].address == RETF_AT, "re-derived instruction/call/branch counts differ")
    far_sites = {i.address: int.from_bytes(i.bytes[1:3], "little") for i in far}
    check(sorted(far_sites) == sorted(SITES), "far-call sites differ from the call-site table")
    base = (BODY_SEG << 4) + ENTRY
    loaded = entry_img[base:base + BODY_LEN]
    diff = [k for k in range(BODY_LEN) if loaded[k] != span[k]]
    seg_words = {i.address - ENTRY + 3 + b for i in far for b in (0, 1)}
    check(set(diff) <= seg_words, "the loaded body differs from GAME.OVR outside the far-call segment words")
    moved = 0
    for i in far:
        k = i.address - ENTRY + 3
        w_file, w_mem = int.from_bytes(span[k:k + 2], "little"), int.from_bytes(loaded[k:k + 2], "little")
        check(w_mem == (w_file + RELOC) & 0xFFFF, "a far-call segment word is not file + 0x0824 at %04X" % i.address)
        moved += 1
    inbody = [i for i, r in enumerate(rows) if r[1] == BODY_SEG]
    ips = [rows[i][2] for i in inbody]
    check(collections.Counter(ips) == collections.Counter(starts), "the executed addresses are not each instruction start exactly once")
    jb_row = next(i for i in inbody if rows[i][2] == 0x03DC)
    check(rows[jb_row + 1][2] == 0x03DE and rows[jb_row][5]["ax"] is not None, "JB 035F was taken")
    print("re-derivation: body SHA, 208 B = 89 instructions, 12 far calls (all segment words = file word + 0x%04X: %d relocated words), one conditional jump "
          "(JB at 03DC, not taken), one RETF at 040E; the loaded image bytes equal GAME.OVR except those words; the trace executes each of the 89 "
          "instruction starts exactly once (a single pass, no retry)" % (RELOC, moved))
    return ins


def replay(rows, base, files, skip):
    import capstone
    from capstone import x86 as X
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_16)
    md.detail = True
    mem, journal, vcount = {}, [], [0]
    notes = collections.Counter()
    notes_dos = []
    taint = [False]

    def rd(a):
        a &= 0xFFFFF
        if a >= VIDEO_BASE:
            notes["video-read"] += 1
            taint[0] = True
            return 0
        return mem.get(a, base[a])

    def rdw(a):
        return rd(a) | (rd(a + 1) << 8)

    label = ["body"]

    def wr(i, a, n, v, kind, cls):
        a &= 0xFFFFF
        for k in range(n):
            b, byte = (a + k) & 0xFFFFF, (v >> (8 * k)) & 255
            if b >= VIDEO_BASE:
                vcount[0] += 1
                continue
            if taint[0]:
                notes["video-read-to-ram"] += 1
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
    handles, annot = {}, {"match": 0, "bad": []}
    last = len(rows) - 1
    site_label = "body"
    for i, (ts, cs, ip, text, byt, r, flags) in enumerate(rows):
        if i in skip:
            continue
        sspb, sp = r["ss"] << 4, r["sp"]
        cls = "rom-stub" if cs == 0xF000 else "game"
        if cs == BODY_SEG:
            label[0] = "body"
        elif i > 0 and rows[i - 1][1] == BODY_SEG and rows[i - 1][2] in SITES and mnemonic(rows[i - 1][3]) in ("call", "lcall"):
            label[0] = SITES[rows[i - 1][2]]
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
                    annot["match"] += 1
                else:
                    annot["bad"].append(i)
        # a hardware interrupt entry: control reaches a new address without a branch of the previous instruction
        if i > 0 and rows[i - 1][1] in (BODY_SEG,) is False:
            pass
        if i > 0 and i - 1 not in skip:
            pcs, pip, ptext, pbyt = rows[i - 1][1], rows[i - 1][2], rows[i - 1][3], rows[i - 1][4]
            if not ptext.startswith("callback") and mnemonic(ptext) not in branchy and not mnemonic(ptext).startswith("j") and \
                    not mnemonic(ptext).startswith("rep"):
                pins = next(md.disasm(pbyt, pip), None)
                if pins is not None and (cs, ip) != (pcs, (pip + pins.size) & 0xFFFF):
                    notes["hw-irq-entry"] += 1
                    if MUT != "no-hw-frame":
                        wr(i, sspb + sp, 2, (pip + pins.size) & 0xFFFF, "irq-frame", "hw-frame")
                        wr(i, sspb + sp + 2, 2, pcs, "irq-frame", "hw-frame")
                        wr(i, sspb + sp + 4, 2, flags | 0x0200, "irq-frame", "hw-frame")
        if text.startswith("callback"):
            tag = text.split()[1]
            nxt = rows[i + 1][5] if i + 1 < len(rows) else r
            if tag == "0038":
                regs9 = [r["ax"], r["bx"], r["cx"], r["dx"], r["si"], r["di"], r["bp"], r["ds"], r["es"]]
                if MUT == "regsave-order":
                    regs9[0], regs9[1] = regs9[1], regs9[0]
                for k, v in enumerate(regs9):
                    wr(i, sspb + ((sp - 18) & 0xFFFF) + 2 * k, 2, v, "dos-regsave", "dos-callback")
                if MUT != "psp-skip":
                    wr(i, 0x8140 + 0x2E, 2, (sp - 18) & 0xFFFF, "dos-psp", "dos-callback")
                    wr(i, 0x8140 + 0x30, 2, r["ss"], "dos-psp", "dos-callback")
                ah, al = r["ax"] >> 8, r["ax"] & 255
                if ah == 0x3D:
                    name = bytes(rd((r["ds"] << 4) + r["dx"] + k) for k in range(80)).split(b"\0")[0].decode("latin-1").upper().replace("/", "\\").split("\\")[-1]
                    path = files.get(name)
                    check(path is not None, "DOS open of a file not present in the game directory: %s" % name)
                    handle, okc = nxt["ax"], not (rows[i + 1][6] & 1)
                    check(okc, "the native DOS open of %s failed" % name)
                    handles[handle] = [name, 0]
                    notes_dos.append(("open", name, handle))
                elif ah == 0x3E:
                    handles.pop(r["bx"], None)
                    notes_dos.append(("close", r["bx"]))
                elif ah == 0x42:
                    h = handles[r["bx"]]
                    size = len(files[h[0]].read_bytes())
                    off = (r["cx"] << 16) | r["dx"]
                    h[1] = (off + (1 if MUT == "seek-origin" else 0)) if al == 0 else (h[1] + off if al == 1 else size + off)
                    check(((nxt["dx"] << 16) | nxt["ax"]) == h[1], "the seek result in the trace differs from the replayed file position")
                    notes_dos.append(("seek", al, h[1]))
                elif ah == 0x3F:
                    h = handles[r["bx"]]
                    off0 = h[1] + (1 if MUT == "read-pos" else 0)
                    data = files[h[0]].read_bytes()[off0:off0 + r["cx"]]
                    if MUT != "read-skip":
                        for k, byte in enumerate(data):
                            wr(i, (r["ds"] << 4) + ((r["dx"] + k) & 0xFFFF), 1, byte, "dos-read", "dos-read")
                    notes_dos.append(("read", h[0], len(data)))
                    h[1] += len(data)
                    notes["read-bytes"] += len(data)
                else:
                    raise SystemExit("unsupported DOS function %02X" % ah)
            elif tag == "0008":
                tick = rdw(0x46C) | (rdw(0x46E) << 16)
                if MUT != "tick-skip":
                    wr(i, 0x46C, 4, (tick + 1) & 0xFFFFFFFF, "bios-tick", "bios-callback")
            elif tag == "002F":
                # DECLARED HYPOTHESIS: the IRQ1 host handler of the left-Alt release (scancode B8, read from AL of the trace) clears the
                # shift-state bits 0x417 bit 3 and 0x418 bit 1 (the break-in key of the debugger session); supported only by byte equality.
                check(r["ax"] & 0xFF == 0xB8, "IRQ1 callback with a scancode other than the Alt release")
                if MUT != "kbd-skip":
                    wr(i, 0x417, 1, rd(0x417) & ~0x08, "kbd-flags", "kbd-callback")
                    wr(i, 0x418, 1, rd(0x418) & ~0x02, "kbd-flags", "kbd-callback")
                notes["callback-002F"] += 1
            elif tag in ("0010", "0002", "000D"):
                notes["callback-" + tag] += 1
            else:
                raise SystemExit("undeclared callback " + tag)
            continue
        ins = next(md.disasm(byt, ip), None)
        check(ins is not None and ins.addr_size == 2, "undecoded instruction at row %d" % i)
        mn, ops = ins.mnemonic, ins.operands
        rep, base_mn = mn.startswith("rep"), mn.split()[-1]
        if mn == "push":
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
            check(n >= 1 and (not rep or n <= r["cx"]), "rep chunk count out of range at row %d" % i)
            check(((nxt["di"] - r["di"]) & 0xFFFF) == n * w, "DI progression disagrees at row %d" % i)
            if MUT == "rep-short" and rep and n > 1:
                n -= 1
            sseg = reg(r, ops[1].mem.segment) if base_mn.startswith("movs") and ops[1].mem.segment else r["ds"]
            for k in range(n):
                dst = (r["es"] << 4) + ((r["di"] + k * w) & 0xFFFF)
                if base_mn.startswith("stos"):
                    wr(i, dst, w, r["ax"] & ((1 << (8 * w)) - 1), ("rep-" if rep else "") + "stos", cls)
                else:
                    src = (sseg << 4) + ((r["si"] + k * w) & 0xFFFF)
                    wr(i, dst, w, rd(src) | (rd(src + 1) << 8 if w == 2 else 0), ("rep-" if rep else "") + "movs", cls)
        elif mn == "pop" and ops[0].type == X.X86_OP_MEM:
            wr(i, ea(r, ops[0]), 2, rdw(sspb + sp), "pop-mem", cls)
        elif mn == "xchg":
            memops = [o for o in ops if o.type == X.X86_OP_MEM]
            if memops:
                other = [o for o in ops if o.type == X.X86_OP_REG][0]
                wr(i, ea(r, memops[0]), memops[0].size, val(r, other, memops[0].size), "xchg", cls)
        elif ops and ops[0].type == X.X86_OP_MEM and mn not in ("cmp", "test", "lea", "les", "lds", "mul", "imul", "div", "idiv", "jmp", "ljmp",
                                                                 "call", "lcall", "push") and not mn.startswith("j"):
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
            else:
                raise SystemExit("unsupported memory-destination form %s at row %d" % (mn, i))
            wr(i, a, size, new & ((1 << (8 * size)) - 1), mn, cls)
    check(notes["video-read-to-ram"] == 0, "an unobserved display read feeds a store into the 640 KiB image")
    return mem, journal, vcount[0], notes, annot, notes_dos


MODEL_MUTANTS = {
    "m-fill-len": ("c->ax = 0x1E00u; push(&m, c->ax);", "c->ax = 0x1E01u; push(&m, c->ax);"),
    "m-557c": ("wr8(ram, c->ds, 0x557Cu, 0u); ", "if (0) wr8(ram, c->ds, 0x557Cu, 0u); "),
    "m-skip-freemem": ("    CALL(0x0401u, PR_U25_0341_FREEMEM);\n", ""),
    "m-add2": ("c->di = (uint16_t)(c->di + 2u);", "c->di = (uint16_t)(c->di + 3u);"),
    "m-dec-once": ("    dec16_flags(c, c->ax); c->ax = (uint16_t)(c->ax - 1u); /* 03F1 DEC AX */\n", ""),
    "m-digit-ds": ("rd8(ram, c->ds, 0x5376u)", "rd8(ram, c->ds, 0x5377u)"),
    "m-retf-pop": ("c->sp = (uint16_t)(c->sp + 4u + 2u);", "c->sp = (uint16_t)(c->sp + 4u);"),
    "m-jb-invert": ("if (!(c->flags & 1u)) break; ", "if (c->flags & 1u) break; "),
}


HOST = ("dos-callback", "bios-callback", "kbd-callback", "hw-frame", "rom-stub", "int-frame")


def compose(run, rows, entry, exit_img, journal, dos, video, build):
    """C. The recovered C model composed from the native entry state: CPU14 from CPU-308A-0341.TXT, the native 640 KiB image (upper 384 KiB
    zero, not observed), the real ECL2.DAX behind the declared 0716 file service; compared with the native return state."""
    root = Path(__file__).resolve().parent.parent.parent
    sys.path.insert(0, str(root / "tools" / "overlay"))
    import ovl_0025_0341_cpu_audit as T
    import ovl_006f_0ec4_cpu_audit as E
    import res0802_0716_composed_audit as M
    build.mkdir(parents=True, exist_ok=True)
    f, lib = T.library(build) if not MUT.startswith("m-") else (None, None)
    fields = cpu_fields(run / "CPU-308A-0341.TXT")
    if MUT == "comp-entry-ds":
        fields["ds"] ^= 1
    if MUT == "comp-flags-cf":
        fields["flags"] ^= 0x0001
    if MUT == "comp-flags-if":
        fields["flags"] ^= 0x0200
    override = None
    if MUT.startswith("m-"):
        src = (root / "src/recovered/overlay_u25_entry4_0341_cpu.c").read_text()
        old, new = MODEL_MUTANTS[MUT]
        check(src.count(old) == 1, "model mutant %s: the replaced text is not unique" % MUT)
        mdir = build / "mutant-src"
        mdir.mkdir(parents=True, exist_ok=True)
        (mdir / "overlay_u25_entry4_0341_cpu.c").write_text(src.replace(old, new))
        override = {"overlay_u25_entry4_0341_cpu": mdir / "overlay_u25_entry4_0341_cpu.c"}
        f, lib = T.library(build, override=override, mutation_label=MUT)
        print("MODEL MUTANT BUILT:", MUT)
    cpu = T.Cpu(*(fields[name] for name, _ in T.Cpu._fields_))
    mem = bytearray(entry) + bytes(T.SIZE - len(entry))
    ecl2 = (DOS_DIR / "ECL2.DAX").read_bytes()
    spec = dict(blobs={"b1": ecl2}, suffixes={"b1": "ECL2.DAX"}, first_handle=(5 if MUT == "comp-handle-5" else next(d[2] for d in dos if d[0] == "open")), appear_after=None, gone_from=None, seek_fail=False,
                keys=(), typeahead=(), retries=M.WIDE, max_seams=M.WIDE, max_disk_prompts=M.WIDE)
    r = E.run_c(f, lib, mem, cpu, spec, diag_cls=T.Diag0341, status_map=T.STATUS, make_env=T.make_env(3))
    print("C model status", r["status"], "calls", r["diag"].calls)
    want = cpu_fields(run / "CPU-1264-0302.TXT")
    got = {name: getattr(r["cpu"], name) for name, _ in T.Cpu._fields_}
    cpu_diff = sorted(k for k in want if want[k] != got[k])
    print("C composition from the native entry: CPU14 differing fields vs native return: %s" % (cpu_diff or "none"))
    comp = r["ram"][:RAM_SIZE]
    diff = [x for x in range(RAM_SIZE) if comp[x] != exit_img[x]]
    last = {}
    for i, a, byte, cls, kind, lab in journal:
        last[a] = (cls, byte)
    host = tuple(h for h in HOST if not (MUT == "comp-host-narrow" and h == "int-frame"))
    host_final = {a for a, (cls, _) in last.items() if cls in host}
    outside = [x for x in diff if x not in host_final]
    healed = bytearray(comp)
    for a, (cls, byte) in last.items():
        if cls in host:
            healed[a] = byte
    after = [x for x in range(RAM_SIZE) if healed[x] != exit_img[x]]
    print("C composed RAM vs native return image: %d differing bytes; %d of them outside the %d declared host-writer bytes; with the declared host "
          "effects applied: %d differing bytes" % (len(diff), len(outside), len(host_final), len(after)))
    if outside:
        lastinfo = {}
        for i, a, byte, cls, kind, lab in journal:
            lastinfo[a] = (cls, kind, lab, i)
        for x in outside[:24]:
            print("  outside %05X entry %02X native %02X composed %02X writer %s" % (x, entry[x], exit_img[x], comp[x], lastinfo.get(x)))
        print("  entry SS:SP", "%04X:%04X" % (fields["ss"], fields["sp"]), "exit SS:SP", "%04X:%04X" % (want["ss"], want["sp"]))

    def norm_c(e):
        if e[0] == "3D":
            return ("open", e[2].upper().replace("/", "\\").split("\\")[-1], e[3])
        if e[0] == "3E":
            return ("close", e[1])
        if e[0] == "42":
            return ("seek", e[1], e[5])
        return ("read", e[3])
    c_dos = [norm_c(e) for e in r["fs"].log]
    n_dos = [("read", d[2]) if d[0] == "read" else d for d in dos]
    n_dos = [("seek", d[1], d[2]) if d[0] == "seek" else d for d in n_dos]
    print("DOS call sequence: C %d events, native %d events, equal: %s" % (len(c_dos), len(n_dos), c_dos == n_dos))
    if c_dos != n_dos:
        print("first C", c_dos[:6], "first native", n_dos[:6])
    n_out = [(rr[5]["dx"], rr[5]["ax"] & 0xFF) for rr in rows if rr[3].split()[:1] == ["out"] and rr[1] != 0xF000]
    if MUT == "comp-out-drop":
        n_out = n_out[:-1]
    c_out = [(e[0], e[1]) for e in r["events"]]
    print("OUT sequence: C %d events, native trace %d game `out dx,al` rows, equal: %s" % (len(c_out), len(n_out), c_out == n_out))
    n_int = [(rr[5]["ss"], rr[5]["sp"]) for rr in rows if rr[3].split()[:1] == ["int"] and rr[1] != 0xF000]
    print("INT instructions: native trace %d (game code), C service calls %d; SS:SP of the INT equal: %s" %
          (len(n_int), len(r["int_sps"]), n_int == [tuple(x) for x in r["int_sps"]]))
    if n_int != [tuple(x) for x in r["int_sps"]]:
        print("  first native", [(hex(a), hex(b)) for a, b in n_int[:3]], "first C", [(hex(a), hex(b)) for a, b in r["int_sps"][:3]])
    print("display writes: C %d bus writes, native replay %d byte stores >= A0000" % (len(r["writes"]), video))
    check(r["status"] == "RETURN", "C model status %s" % r["status"])
    check(not cpu_diff, "composed CPU14 differs from the native return in %s" % cpu_diff)
    check(not outside and not after, "composed RAM differs from native outside the declared host effects")
    check(c_dos == n_dos, "composed DOS call sequence differs from the native one")
    check(c_out == n_out, "composed OUT sequence differs from the native trace")
    check(n_int == [tuple(x) for x in r["int_sps"]], "composed INT SS:SP sequence differs from the native trace")
    check(len(r["writes"]) == video, "composed display write count differs from the native replay")
    print("composition verdict: CPU14 equal, RAM equal outside %d declared host-writer bytes (0 left after applying them), DOS %d events equal, "
          "OUT %d events equal, INT SS:SP %d equal, display writes %d equal" % (len(host_final), len(c_dos), len(c_out), len(n_int), video))
    return dict(cpu_diff=cpu_diff, diff=len(diff), outside=len(outside), healed=len(after), dos=c_dos == n_dos, out=c_out == n_out, writes=len(r["writes"]))


def main():
    check(len(sys.argv) in (3, 4), "usage: audit_slot_a_ovl0341_native_t574.py RUN039_DIR GAME.OVR [BUILD_DIR]")
    run, ovr_path = Path(sys.argv[1]), Path(sys.argv[2])
    for name, digest in PINS.items():
        check(sha256((run / name).read_bytes()).hexdigest() == digest, name + " identity mismatch")
    ovr = ovr_path.read_bytes()
    check(sha256(ovr).hexdigest() == GAME_HASH, "GAME.OVR identity mismatch")
    rows = parse(run / "LOGL-CALL-308A-0341.TXT")
    check(len(rows) == LINES and (rows[0][1], rows[0][2]) == (BODY_SEG, ENTRY) and (rows[-1][1], rows[-1][2]) == (0x1264, 0x0302),
          "trace first/last rows differ")
    entry = (run / "RAM-308A-0341.BIN").read_bytes()
    exit_img = (run / "RAM-1264-0302.BIN").read_bytes()
    for row, name in ((rows[0], "CPU-308A-0341.TXT"), (rows[-1], "CPU-1264-0302.TXT")):
        want = cpu_fields(run / name)
        got = dict(row[5])
        got.update(cs=row[1], ip=row[2], flags=row[6])
        check(all(want[k] == got[k] for k in want), "%s disagrees with the trace" % name)
    print("transcriptions: all 14 fields (flags included) of both CPU files equal the trace's first and last lines")
    rederive(ovr, entry, rows)
    files = {p.name.upper(): p for p in DOS_DIR.iterdir() if p.is_file()}
    mem, journal, video, notes, annot, dos = replay(rows, entry, files, {len(rows) - 1})
    check(not annot["bad"], "trace read annotations disagree with replayed memory at %d row(s)" % len(annot["bad"]))
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
    print("store replay of the whole call: %d byte stores journaled, %d trace read annotations match the shadow memory (%d mismatches), "
          "%d display stores counted not compared, %d hardware IRQ entr%s, callbacks %s" %
          (len(journal), annot["match"], len(annot["bad"]), video, notes["hw-irq-entry"], "y" if notes["hw-irq-entry"] == 1 else "ies",
           {k[9:]: v for k, v in sorted(notes.items()) if k.startswith("callback-")}))
    print("DOS events (declared bindings to the game directory's files):", collections.Counter(d[0] for d in dos),
          "files opened", sorted({d[1] for d in dos if d[0] == "open"}), "bytes read", notes["read-bytes"])
    print("per-byte account of the %d native delta bytes (entry -> return image): explained %d, writer-but-wrong-value %d, UNATTRIBUTED %d, spurious %d; "
          "replayed image vs native: %d mismatching byte(s)" % (len(delta), len(explained), len(wrong), len(unattributed), len(spurious), len(mism)))
    by_label = collections.Counter(writers[x][2] for x in explained)
    by_class = collections.Counter(writers[x][0] for x in explained)
    print("explained delta bytes by last writer: body/callee %s; class %s" % (dict(sorted(by_label.items())), dict(sorted(by_class.items()))))
    if unattributed or mism:
        def reg(x):
            return "BIOS data 0400-04FF" if 0x400 <= x < 0x500 else ("stack SS" if 0x1D160 <= x < 0x2D160 else "other")
        print("unattributed by region:", dict(collections.Counter(reg(x) for x in unattributed)), "addresses", [hex(x) for x in unattributed][:24])
        print("mismatch by region:", dict(collections.Counter(reg(x) for x in mism)), "addresses", [hex(x) for x in mism][:24])
    check(not unattributed and not mism and not wrong, "the replay does not account for every native delta byte")
    if len(sys.argv) == 4:
        compose(run, rows, entry, exit_img, journal, dos, video, Path(sys.argv[3]))


if __name__ == "__main__":
    main()
