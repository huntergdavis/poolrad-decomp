#!/usr/bin/env python3
"""Single-invocation chain `10D2:0165 -> 10D2:13C6 -> 2E60:0747 -> 3582:3492` (run 038); print no game or RAM payload.

One native invocation, one armed `LOGL 10000` log (26,378 lines) and four stops (breakpoints only at the four
addresses, one Up key).  This audit locks the run-038 files, then:

  A. replays every store of the whole log over the native 0165 image (capstone; declared host writers: the DOS
     INT 21h callback (AH 25h vector write, 42h seek, 3Fh read from GAME.OVR, nine saved registers, PSP pointer), the
     BIOS tick, the hardware IRQ0 frame; INT 1Ch and the idle callbacks store-free) with a shadow snapshot at each
     stop, and accounts every window's native delta (0165->13C6, 13C6->0747, 0747->3492) against the SAME run's images;
  B. composes the existing C models (dispatch_00dc, 17AE, 1742, 13B1, built with ctypes from the recovered
     sources) from this run's 0165 image and CPU transcription under the declared AH=25 / IN 61h / OUT environment and
     diffs the result with this run's native 13C6 and 0747 images (a single-run chain, not a cross-run one);
  C. compares each window with the filed separate-window results (run 037, run 030, run 034): delta address sets,
     values, and the cause of every difference.

Nothing here verifies the C models beyond this chain: their own audits are separate.  Needs capstone and a C
compiler; run without `python3 -I`.  `POOLRAD_CHAIN_MUTANT=<name>` perturbs one branch (see the check script).
"""

from hashlib import sha256
from pathlib import Path
import collections
import ctypes
import os
import re
import subprocess
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.parent
MUT = os.environ.get("POOLRAD_CHAIN_MUTANT", "")
RUN038 = {
    "LOGL-CHAIN-0165-to-3492.TXT": "82fa7c72252c08783feec7136b4c4d90cef1c17fdbe4ab7788ad7acfb9867542",
    "RAM-10D2-0165.BIN": "a2c94c585aeff44c1330cf4d1e37e849804a0d20bca4c8069700ca477709d550",
    "RAM-10D2-13C6.BIN": "66c64af48cf0bc777bb0a3da2e17e308d3a75f5b25794627aa233372d5777d61",
    "RAM-2E60-0747.BIN": "ad337c93641ec2f2f273cb42779af569d6a5293f2bb36d121f0030bf0063a853",
    "RAM-3582-3492.BIN": "59ac71b71ef34992f3875856da072bbbd55a22241afb04ea6aad6c1cf7f80ff8",
    "CPU-10D2-0165.TXT": "bb164902b169299404f75a56ecef2d88eab8e3791fbcbe6c99cacbf95e52ca0c",
    "CPU-10D2-13C6.TXT": "baef3d82414d7cec77a455d4acd035b5f9bbf219914ec01b331731dffbb63249",
    "CPU-2E60-0747.TXT": "9a27b7ec5c003435da1d78abfdeb84e31643602bfa82d93917c71e2c833c6d85",
    "CPU-3582-3492.TXT": "ccbac0c055fc1868b5ceef5b4942d2eb72422111678d9cc7aebb0ae7d4e95433",
}
FILED = {   # the separate-window runs this one is compared with (images only)
    "t4492-slot8-gap-037": {"RAM-10D2-0165.BIN": "0914b16f3278486195a111769095f97c55315330a12f278f2c3dc86a2d84b643",
                            "RAM-10D2-13C6.BIN": "7f255423f12188d9d961c9bb8f041bb69f51b175466f188d3bb84427332c841e"},
    "t4492-slot4-logc-030": {"RAM-10D2-13C6.BIN": "342882515c7dd4f852a17a15bc78c4d7bf147b2cb287e3e7169abbcaee02e270",
                             "RAM-2E60-0747.BIN": "99d632bb8349102c6f41a495ce212367e07a4d35b32206b9654703e2e065e270"},
    "t4492-slot6-window-034": {"RAM-2E60-0747.BIN": "f9b539f0defdda7ddb826d4242509b72350c22bd92869343ab35062ec809dbb5",
                               "RAM-3582-3492.BIN": "ed613cd5f0c27f282e1948698b0c82b515efc5cfe24a0564ad52d6f5e3c61d59"},
}
GAME_HASH = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
SOURCES = ("resident_08ae_dispatch_00dc_cpu", "resident_08ae_table_17ae_cpu", "resident_08ae_table_1742_cpu",
           "resident_08ae_vector_13b1_cpu")
RAM_SIZE, RAM_1M, VIDEO_BASE = 655360, 1 << 20, 0xA0000
LINES = 26378
SS_BASE = 0x1D160
FIELDS = "ax bx cx dx si di bp sp cs ip ds es ss flags".split()
STATUS = {0: "RETURN", 1: "ARGUMENT", 2: "CALL_137C", 3: "CALL_17AE", 4: "CALL_1742", 5: "CALL_181A", 6: "CALL_006C", 7: "CALL_13B1"}
EXPECT_OUTS = ((0x40, 0xFF), (0x40, 0xFF), (0x61, 0x30), (0xC0, 0x9F), (0xC0, 0xBF), (0xC0, 0xDF), (0xC0, 0xFF))
WINDOWS = (("0165->13C6", "RAM-10D2-0165.BIN", "RAM-10D2-13C6.BIN"), ("13C6->0747", "RAM-10D2-13C6.BIN", "RAM-2E60-0747.BIN"),
           ("0747->3492", "RAM-2E60-0747.BIN", "RAM-3582-3492.BIN"))
# Cross-run causes per window (changed only in the filed run, changed only here, differing final value).  SI is the
# host hour, the tick word is host time of day, and the BIOS IRQ0 handler's own writes depend on where IRQ0 landed;
# "=same-final" marks a byte that ends with the same value in both runs (a stale starting value that coincides).
EXPECT_CAUSES = {
    "0165->13C6": ({"push-ax/game=same-final": 1, "push-di/game=same-final": 1}, {}, {"push-si/game": 1}),
    "13C6->0747": ({"regsave-si": 1}, {}, {}),
    "0747->3492": ({"push-ax/rom-stub": 1, "push-ax/game=same-final": 1}, {"push-ax/rom-stub": 1},
                   {"bios-tick/bios-callback": 1, "int/rom-stub": 1, "push-dx/rom-stub": 1}),
}
DOS_FILE_CALLS = (("seek", 0x23DCC), ("read", 0xE65), ("read", 0x58), ("seek", 0x2E0CE), ("read", 0x7C5), ("read", 0x64))


def check(condition, message):
    if not condition:
        raise SystemExit(message)


class Cpu(ctypes.Structure):
    _fields_ = [(n, ctypes.c_uint16) for n in FIELDS]


DOS_HOOK = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8), ctypes.POINTER(Cpu), ctypes.POINTER(ctypes.c_uint16))
OUT_HOOK = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.c_uint16, ctypes.c_uint8)
IN_HOOK = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.c_uint16, ctypes.POINTER(ctypes.c_uint8))


class Env(ctypes.Structure):
    _fields_ = [("context", ctypes.c_void_p), ("dos", DOS_HOOK), ("port_out", OUT_HOOK), ("port_in", IN_HOOK)]


def cpu_from_file(path):
    d = {}
    for part in Path(path).read_text().split():
        name, _, value = part.partition("=")
        check(len(value) == 4 and name.lower() not in d, "malformed CPU field")
        d[name.lower()] = int(value, 16)
    check(len(d) == 14, "CPU file is not 14 fields")
    c = Cpu()
    for n in FIELDS:
        setattr(c, n, d[n])
    return c


def cpu_diff(c, want, skip=()):
    return {n: (getattr(c, n), getattr(want, n)) for n in FIELDS if n not in skip and getattr(c, n) != getattr(want, n)}


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


def cpu_from_row(row):
    c = Cpu()
    for n in FIELDS:
        if n in row[5]:
            setattr(c, n, row[5][n])
    c.cs, c.ip, c.flags = row[1], row[2], row[6]
    return c


def replay(rows, base, game, skip_rows, snap_rows):
    """Store replay of the whole chain over `base`.  Returns (shadow dict, snapshots, journal, notes)."""
    import capstone
    from capstone import x86 as X
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_16)
    md.detail = True
    mem, snaps, journal, vjournal = {}, {}, [], []
    notes = {"annot_match": 0, "annot_bad": [], "dos": [], "irq": 0, "video_read_to_ram": 0, "video_reads": 0}
    taint = [False]

    def rd(a):
        a &= 0xFFFFF
        if a >= VIDEO_BASE:
            notes["video_reads"] += 1
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
                vjournal.append((i, b - VIDEO_BASE, byte, cls))
                continue
            if taint[0]:
                notes["video_read_to_ram"] += 1
            mem[b] = byte
            journal.append((i, b, byte, cls, kind))

    ids = {X.X86_REG_AX: ("ax", 0xFFFF, 0), X.X86_REG_BX: ("bx", 0xFFFF, 0), X.X86_REG_CX: ("cx", 0xFFFF, 0),
           X.X86_REG_DX: ("dx", 0xFFFF, 0), X.X86_REG_SI: ("si", 0xFFFF, 0), X.X86_REG_DI: ("di", 0xFFFF, 0),
           X.X86_REG_BP: ("bp", 0xFFFF, 0), X.X86_REG_SP: ("sp", 0xFFFF, 0), X.X86_REG_AL: ("ax", 0xFF, 0),
           X.X86_REG_AH: ("ax", 0xFF, 8), X.X86_REG_BL: ("bx", 0xFF, 0), X.X86_REG_BH: ("bx", 0xFF, 8),
           X.X86_REG_CL: ("cx", 0xFF, 0), X.X86_REG_CH: ("cx", 0xFF, 8), X.X86_REG_DL: ("dx", 0xFF, 0),
           X.X86_REG_DH: ("dx", 0xFF, 8), X.X86_REG_DS: ("ds", 0xFFFF, 0), X.X86_REG_ES: ("es", 0xFFFF, 0),
           X.X86_REG_SS: ("ss", 0xFFFF, 0), X.X86_REG_CS: ("cs", 0xFFFF, 0)}

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
           "sbb": lambda o, s, c: o - s - c, "and": lambda o, s, c: o & s, "or": lambda o, s, c: o | s, "xor": lambda o, s, c: o ^ s}
    dos = {"pos": None}
    last = len(rows) - 1
    for i, (ts, cs, ip, text, byt, r, flags) in enumerate(rows):
        if i in snap_rows:
            snaps[i] = dict(mem)
        if i in skip_rows:
            continue
        taint[0] = False
        sspb, sp = r["ss"] << 4, r["sp"]
        cls = "rom-stub" if cs == 0xF000 else "game"
        m = ANNOT.search(text)
        if m and not text.startswith("callback"):
            seg, off, hexv = r[m.group(1)], int(m.group(2), 16), m.group(3)
            a = ((seg << 4) + off) & 0xFFFFF
            if a + len(hexv) // 2 > VIDEO_BASE:
                notes.setdefault("annot_video", 0)
                notes["annot_video"] += 1
            else:
                got = bytes(mem.get((a + k) & 0xFFFFF, base[(a + k) & 0xFFFFF]) for k in range(len(hexv) // 2))
                if got == int(hexv, 16).to_bytes(len(hexv) // 2, "little"):
                    notes["annot_match"] += 1
                else:
                    notes["annot_bad"].append(i)
        if (cs, ip) == (0xF000, 0xFEA5) and mnemonic(rows[i - 1][3]) not in ("jmp", "call", "ljmp", "int"):
            notes["irq"] += 1
            j = i + 1
            while j < len(rows) and not (mnemonic(rows[j][3]) == "iret" and rows[j][5]["sp"] == sp):
                j += 1
            check(j + 1 < len(rows), "IRQ0 frame has no matching iret")
            if MUT != "no-hw-frame":
                wr(i, sspb + sp, 2, rows[j + 1][2], "irq-frame", "hw-frame")
                wr(i, sspb + sp + 2, 2, rows[j + 1][1], "irq-frame", "hw-frame")
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
                wr(i, 0x8140 + 0x2E, 2, (sp - 18) & 0xFFFF, "dos-psp", "dos-callback")
                wr(i, 0x8140 + 0x30, 2, r["ss"], "dos-psp", "dos-callback")
                ah, al = r["ax"] >> 8, r["ax"] & 255
                if ah == 0x25:                               # set interrupt vector AL: DS:DX
                    if MUT != "ivt-skip":
                        wr(i, al * 4, 2, r["dx"], "dos-ivt", "dos-callback")
                        wr(i, al * 4 + 2, 2, r["ds"], "dos-ivt", "dos-callback")
                    notes["dos"].append(("setvect", al, r["ds"], r["dx"]))
                elif ah == 0x42:
                    check(al == 0 and r["bx"] == 5, "unexpected DOS seek")
                    dos["pos"] = (r["cx"] << 16) | r["dx"]
                    notes["dos"].append(("seek", dos["pos"]))
                elif ah == 0x3F:
                    check(r["bx"] == 5 and dos["pos"] is not None and dos["pos"] + r["cx"] <= len(game), "unexpected DOS read")
                    pos = dos["pos"] + (1 if MUT == "read-pos" else 0)
                    if MUT != "read-skip":
                        for k in range(r["cx"]):
                            wr(i, (r["ds"] << 4) + ((r["dx"] + k) & 0xFFFF), 1, game[pos + k], "dos-read", "dos-read")
                    notes["dos"].append(("read", r["cx"], r["ds"]))
                    dos["pos"] += r["cx"]
                    dos["last_read"] = r["cx"]
                else:
                    raise SystemExit("unsupported DOS function %02X" % ah)
            elif tag == "0008":
                tick = rdw(0x46C) | (rdw(0x46E) << 16)
                if MUT != "tick-skip":
                    wr(i, 0x46C, 4, (tick + 1) & 0xFFFFFFFF, "bios-tick", "bios-callback")
            elif tag in ("0010", "0002"):
                changed = [k for k in ("ax", "bx", "cx", "dx", "si", "di", "bp", "sp", "ds", "es", "ss") if nxt[k] != r[k]]
                check(not changed or (changed == ["ax"] and tag == "0002" and nxt["ax"] == dos.get("last_read")),
                      "registers %s change across callback %s" % (changed, tag))
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
            wr(i, sspb + ((sp - 2) & 0xFFFF), 2, flags, "int", cls)
            wr(i, sspb + ((sp - 4) & 0xFFFF), 2, cs, "int", cls)
            wr(i, sspb + ((sp - 6) & 0xFFFF), 2, (ip + ins.size) & 0xFFFF, "int", cls)
        elif base_mn in ("stosb", "stosw", "movsb", "movsw"):
            w = 2 if base_mn.endswith("w") else 1
            nxt = rows[i + 1][5]
            n = ((r["cx"] - nxt["cx"]) & 0xFFFF) if rep else 1
            check(1 <= n and (not rep or n <= r["cx"]), "rep chunk count out of range at row %d" % i)
            check(((nxt["di"] - r["di"]) & 0xFFFF) == n * w, "DI progression disagrees at row %d" % i)
            sseg = None
            if base_mn.startswith("movs"):
                sseg = reg(r, ops[1].mem.segment) if ops[1].mem.segment else r["ds"]
            for k in range(n):
                dst = (r["es"] << 4) + ((r["di"] + k * w) & 0xFFFF)
                if base_mn.startswith("stos"):
                    wr(i, dst, w, r["ax"] & ((1 << (8 * w)) - 1), ("rep-" if rep else "") + "stos", cls)
                else:
                    src = (sseg << 4) + ((r["si"] + k * w) & 0xFFFF)
                    wr(i, dst, w, rd(src) | (rd(src + 1) << 8 if w == 2 else 0), ("rep-" if rep else "") + "movs", cls)
        elif mn == "xchg":
            memops = [o for o in ops if o.type == X.X86_OP_MEM]
            if memops:
                other = [o for o in ops if o.type == X.X86_OP_REG][0]
                wr(i, ea(r, memops[0]), memops[0].size, val(r, other, memops[0].size), "xchg", cls)
        elif ops and ops[0].type == X.X86_OP_MEM and mn not in (
                "cmp", "test", "lea", "les", "lds", "mul", "imul", "div", "idiv", "jmp", "ljmp", "call", "lcall", "push") and not mn.startswith("j"):
            a, size = ea(r, ops[0]), ops[0].size
            check(size in (1, 2), "unsupported operand size at row %d" % i)
            old = rd(a) if size == 1 else rdw(a)
            if mn == "mov":
                new = val(r, ops[1], size)
            elif mn in alu:
                new = alu[mn](old, val(r, ops[1], size), flags & 1)
            elif mn == "inc":
                new = old + 1
            elif mn == "dec":
                new = old - 1
            else:
                raise SystemExit("unsupported memory-destination form %s at row %d" % (mn, i))
            wr(i, a, size, new & ((1 << (8 * size)) - 1), mn, cls)
    check(notes["video_read_to_ram"] == 0, "an unobserved display read feeds a store into the 640 KiB image")
    notes["vjournal"] = len(vjournal)
    return mem, snaps, journal, notes


def image(base, overlay=None):
    m = bytearray(base) + bytearray(RAM_1M - len(base))
    for a, v in (overlay or {}).items():
        m[a] = v
    return m


def build_library(build_dir):
    build_dir.mkdir(parents=True, exist_ok=True)
    path = build_dir / "chain_single_038.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                    *[str(ROOT / "src/recovered" / (s + ".c")) for s in SOURCES], "-o", str(path)], check=True)
    lib = ctypes.CDLL(str(path))
    u8, p = ctypes.POINTER(ctypes.c_uint8), ctypes.POINTER
    for name in ("pr_r08ae_dispatch_00dc_cpu", "pr_r08ae_table_17ae_cpu", "pr_r08ae_table_1742_cpu"):
        f = getattr(lib, name)
        f.argtypes, f.restype = [u8, ctypes.c_size_t, p(Cpu)], ctypes.c_int
    f = lib.pr_r08ae_vector_13b1_cpu
    f.argtypes, f.restype = [u8, ctypes.c_size_t, p(Cpu), p(Env)], ctypes.c_int
    return lib


def call_plain(lib, name, img, cpu):
    return getattr(lib, name)((ctypes.c_uint8 * RAM_1M).from_buffer(img), RAM_1M, ctypes.byref(cpu))


class Binding:
    """Declared environment of the 13B1 model: DOS AH=25 vector service, IN 61h and the OUT log."""

    def __init__(self, input61):
        self.input61, self.calls, self.outs, self.cpu_at_dos = input61, 0, [], None

    def dos(self, ctx, ram, cpu, flags_out):
        c = cpu.contents
        rd = lambda a: ram[a] | (ram[a + 1] << 8)
        if self.calls or c.ax != 0x2508 or c.dx != 0xFEA5 or c.ds != 0xF000 or c.ip != 0x13C6 or c.flags != 0x7046 or \
                rd(0x20) != 0x13EA or rd(0x22) != 0x10D2 or ram[0x8140] != 0xCD or ram[0x8141] != 0x20:
            return 1
        self.cpu_at_dos = Cpu.from_buffer_copy(c)
        stack = (c.ss << 4) + c.sp
        if rd(stack) != 0x13C8 or rd(stack + 2) != c.cs:
            return 1
        if MUT != "comp-bind-skip":
            regs = [c.ax, c.bx, c.cx, c.dx, c.si, c.di, c.bp, c.ds, c.es]
            for i, v in enumerate(regs):
                for k in (0, 1):
                    ram[stack - 18 + 2 * i + k] = (v >> (8 * k)) & 255
            for off, v in ((0x2E, (c.sp - 18) & 0xFFFF), (0x30, c.ss), (0, c.dx), (2, c.ds)):
                at = 0x8140 + off if off in (0x2E, 0x30) else 0x20 + off
                ram[at], ram[at + 1] = v & 255, v >> 8
        flags_out[0] = 0x7246
        self.calls += 1
        return 0

    def port_out(self, ctx, port, value):
        self.outs.append((port, value))
        return 0

    def port_in(self, ctx, port, value):
        value[0] = self.input61
        return 0

    def env(self):
        self._h = (DOS_HOOK(self.dos), OUT_HOOK(self.port_out), IN_HOOK(self.port_in))
        return Env(None, *self._h)


def run13(lib, img, cpu, binding):
    env = binding.env() if binding else None
    return lib.pr_r08ae_vector_13b1_cpu((ctypes.c_uint8 * RAM_1M).from_buffer(img), RAM_1M, ctypes.byref(cpu),
                                        ctypes.byref(env) if env else None)


def chain_to_13b1(lib, base, cpu0):
    img, c = image(base), Cpu.from_buffer_copy(cpu0)
    if MUT == "comp-entry-ds":
        c.ds = (c.ds + 1) & 0xFFFF
    for name, want in (("pr_r08ae_dispatch_00dc_cpu", 3), ("pr_r08ae_table_17ae_cpu", 0), ("pr_r08ae_dispatch_00dc_cpu", 4),
                       ("pr_r08ae_table_1742_cpu", 0), ("pr_r08ae_dispatch_00dc_cpu", 7)):
        st = call_plain(lib, name, img, c)
        check(st == want, "%s returned %s, expected %s" % (name, STATUS.get(st, st), STATUS.get(want, want)))
    return img, c


def region(a):
    if 0x46C <= a < 0x470:
        return "BIOS tick"
    if 0x20 <= a < 0x24:
        return "IVT 08h"
    if 0x816E <= a < 0x8172:
        return "PSP stack pointer"
    if 0x10F00 <= a < 0x11200:
        return "resident data"
    if SS_BASE + 0x3A00 <= a < SS_BASE + 0x3F00:
        return "stack SS:3Axx-3Exx"
    if 0x2F6B0 <= a < 0x30E00:
        return "loaded overlay buffers"
    return "other"


def main():
    check(len(sys.argv) == 5, "usage: audit_slot_a_chain_single_038.py RUN038_DIR RUNS_DIR GAME.OVR BUILD_DIR")
    run, runs, game_path, build = Path(sys.argv[1]), Path(sys.argv[2]), Path(sys.argv[3]), Path(sys.argv[4])
    for name, digest in RUN038.items():
        check(sha256((run / name).read_bytes()).hexdigest() == digest, "%s identity mismatch" % name)
    for d, locks in FILED.items():
        for name, digest in locks.items():
            check(sha256((runs / d / name).read_bytes()).hexdigest() == digest, "%s/%s identity mismatch" % (d, name))
    game = game_path.read_bytes()
    check(sha256(game).hexdigest() == GAME_HASH, "GAME.OVR identity mismatch")
    rows = parse(run / "LOGL-CHAIN-0165-to-3492.TXT")
    check(len(rows) == LINES, "chain log line count mismatch: %d" % len(rows))
    imgs = {n: (run / n).read_bytes() for n in RUN038 if n.endswith(".BIN")}
    cpus = {n: cpu_from_file(run / n) for n in RUN038 if n.endswith(".TXT") and n.startswith("CPU")}
    # boundaries: each stop is a duplicated pair, the last row is the 3492 stop state
    at = lambda cs, ip: [i for i, r in enumerate(rows) if (r[1], r[2]) == (cs, ip)]
    b1, b2 = at(0x10D2, 0x13C6), at(0x2E60, 0x0747)
    check(rows[0][1:3] == rows[1][1:3] == (0x10D2, 0x0165) and len(b1) == 2 and b1[1] == b1[0] + 1 and len(b2) == 2 and
          b2[1] == b2[0] + 1 and (rows[-1][1], rows[-1][2]) == (0x3582, 0x3492) and len(at(0x3582, 0x3492)) == 1,
          "stop boundary structure changed")
    s1, s2, s3 = b1[0], b2[0], len(rows) - 1
    skip = {0, s1, s2, s3}
    if MUT == "dup-exec":
        skip = {0, s3}                       # mutant: execute the stop rows too
    print("single invocation: one armed log of %d lines, stops at rows %d (13C6), %d (0747), %d (3492); every stop is a duplicated pair" %
          (len(rows), s1, s2, s3))
    # per-window transcription check: trace rows at the stops vs the CPU files
    for row, name in ((rows[1], "CPU-10D2-0165.TXT"), (rows[s1 + 1], "CPU-10D2-13C6.TXT"), (rows[s2 + 1], "CPU-2E60-0747.TXT"), (rows[s3], "CPU-3582-3492.TXT")):
        check(not cpu_diff(cpu_from_row(row), cpus[name]), "%s disagrees with the trace" % name)
    print("transcriptions: all 14 fields (flags included) of the four CPU files equal the trace rows at the stops")
    # A. whole-chain store replay with snapshots at the stops
    base0 = imgs["RAM-10D2-0165.BIN"]
    mem, snaps, journal, notes = replay(rows, base0, game, skip, {s1, s2, s3})
    check(not notes["annot_bad"], "trace read annotations disagree with replayed memory at %d row(s)" % len(notes["annot_bad"]))
    print("store replay of the whole chain: %d byte stores journaled, %d trace read annotations match the shadow memory (%d mismatches), "
          "%d hardware IRQ entr%s, %d display stores not compared (display rule as before)" %
          (len(journal), notes["annot_match"], len(notes["annot_bad"]), notes["irq"], "y" if notes["irq"] == 1 else "ies", notes["vjournal"]))
    print("declared DOS events in the chain:", [e for e in notes["dos"]])
    stops = {"RAM-10D2-13C6.BIN": s1, "RAM-2E60-0747.BIN": s2, "RAM-3582-3492.BIN": s3}
    account = {}
    for (label, a_name, b_name), (lo, hi) in zip(WINDOWS, ((0, s1), (s1, s2), (s2, s3))):
        a, b = imgs[a_name], imgs[b_name]
        pred = bytearray(base0)
        for x, v in snaps[hi].items():
            pred[x] = v
        delta = [x for x in range(RAM_SIZE) if a[x] != b[x]]
        wj = {}
        for i, addr, byte, cls, kind in journal:
            if lo < i <= hi or (lo == 0 and i <= hi):
                wj[addr] = (cls, kind, i)
        mism = [x for x in range(RAM_SIZE) if pred[x] != b[x]]
        explained = [x for x in delta if x in wj and pred[x] == b[x]]
        by_class = collections.Counter(wj[x][0] for x in explained)
        account[label] = dict(delta=delta, mism=mism, explained=explained, wj=wj)
        print("window %s: native delta %d bytes, explained %d, unattributed %d, replayed image vs native %d mismatch(es); explained by writer class %s" %
              (label, len(delta), len(explained), len([x for x in delta if x not in wj]), len(mism), dict(sorted(by_class.items()))))
    # the cumulative prediction at 3492 against this run's native image is the end-to-end statement
    check(all(not v["mism"] and len(v["explained"]) == len(v["delta"]) for v in account.values()),
          "a window is not fully reproduced by the store replay")
    e2e = [x for x in range(RAM_SIZE) if base0[x] != imgs["RAM-3582-3492.BIN"][x]]
    print("end to end 0165 -> 3492: %d delta bytes (the windows overlap: %d + %d + %d bytes)" %
          (len(e2e), len(account["0165->13C6"]["delta"]), len(account["13C6->0747"]["delta"]), len(account["0747->3492"]["delta"])))
    # B. C-model composition from this run's own 0165 image
    lib = build_library(build)
    ins = [i for i, r in enumerate(rows) if mnemonic(r[3]) == "in" and r[1] == 0x10D2]
    check(len(ins) == 1 and rows[ins[0]][3].split()[1].split(",")[-1] == "61", "IN sequence in the chain changed")
    input61 = rows[ins[0] + 1][5]["ax"] & 0xFF
    trace_outs = [(int(r[3].split()[1].split(",")[0], 16), r[5]["ax"] & 0xFF) for r in rows[s1:s2] if mnemonic(r[3]) == "out" and r[1] == 0x10D2]
    check(trace_outs == list(EXPECT_OUTS) and input61 == 0x30, "chain port events changed: %s IN=%02X" % (trace_outs, input61))
    if MUT == "comp-in61-03":
        input61 = 0x03
    img0, c0 = chain_to_13b1(lib, base0, cpus["CPU-10D2-0165.TXT"])
    pre = bytes(img0[:RAM_SIZE])
    stop_img, stop_cpu = bytearray(img0), Cpu.from_buffer_copy(c0)
    check(run13(lib, stop_img, stop_cpu, None) == 2, "13B1 NULL environment did not stop at the INT")
    frame_lo = (stop_cpu.ss << 4) + stop_cpu.sp
    frame = set(range(frame_lo, frame_lo + 6))
    pre_int = bytearray(stop_img[:RAM_SIZE])
    for x in frame:
        pre_int[x] = pre[x]
    b = Binding(input61)
    final_img, final_cpu = bytearray(img0), Cpu.from_buffer_copy(c0)
    check(run13(lib, final_img, final_cpu, b) == 0 and b.calls == 1 and len(b.outs) == 7 and final_cpu.ip == 0x0177, "13B1 with the environment did not return")
    check(call_plain(lib, "pr_r08ae_dispatch_00dc_cpu", final_img, final_cpu) == 0, "dispatcher did not return")
    n13, n747 = imgs["RAM-10D2-13C6.BIN"], imgs["RAM-2E60-0747.BIN"]
    m13 = [x for x in range(RAM_SIZE) if pre_int[x] != n13[x]]
    m747 = [x for x in range(RAM_SIZE) if final_img[x] != n747[x]]
    cpu_final = cpu_diff(final_cpu, cpus["CPU-2E60-0747.TXT"])
    print("C-model composition from this run's native 0165 image + CPU transcription (single run): composed image at the INT (frame restored) vs "
          "native 13C6: %d differing byte(s); composed RAM at 2E60:0747 vs native 0747: %d differing byte(s); final CPU vs transcription: %s; "
          "OUT sequence %s the trace, IN 61h = %02X (measured)" %
          (len(m13), len(m747), "equal in all 14 fields" if not cpu_final else cpu_final, "equals" if b.outs == trace_outs else "DIFFERS from", input61))
    check(not m13 and not m747 and not cpu_final and b.outs == trace_outs, "the C-model composition does not reproduce this run's native images")
    # C. each window against the separate-window run it repeats
    for (label, a_name, b_name), (run_name, fa, fb) in zip(WINDOWS, (("t4492-slot8-gap-037", "RAM-10D2-0165.BIN", "RAM-10D2-13C6.BIN"),
                                                                      ("t4492-slot4-logc-030", "RAM-10D2-13C6.BIN", "RAM-2E60-0747.BIN"),
                                                                      ("t4492-slot6-window-034", "RAM-2E60-0747.BIN", "RAM-3582-3492.BIN"))):
        fim_a, fim_b = (runs / run_name / fa).read_bytes(), (runs / run_name / fb).read_bytes()
        fd = {x for x in range(RAM_SIZE) if fim_a[x] != fim_b[x]}
        cur = set(account[label]["delta"])
        wj = account[label]["wj"]
        only_cur, only_filed, common = cur - fd, fd - cur, cur & fd
        diff_vals = sorted(x for x in common if imgs[b_name][x] != fim_b[x])
        # an address changed in the filed run but not in this one must still have a replayed writer here (value coincidence)
        coincide = [x for x in only_filed if x in wj]
        print("window %s vs %s: delta %d vs %d bytes; only here %d, only in the filed run %d (%d of them written by a replayed store here with an unchanged value), "
              "common %d with %d differing final value(s); differing values by region %s" %
              (label, run_name, len(cur), len(fd), len(only_cur), len(only_filed), len(coincide), len(common), len(diff_vals),
               dict(sorted(collections.Counter(region(x) for x in diff_vals).items()))))
        def who(x):
            """Name the register or event that wrote byte x here: the host-time candidates are SI (hour), the tick and the IRQ0 landing."""
            cls, kind, row = wj[x]
            r = rows[row][5]
            if kind == "dos-regsave":
                names = ("ax", "bx", "cx", "dx", "si", "di", "bp", "ds", "es")
                return "regsave-" + names[(x - ((r["ss"] << 4) + ((r["sp"] - 18) & 0xFFFF))) // 2]
            if kind == "push":
                return "push-%s/%s" % (rows[row][3].split()[1].rstrip(","), cls)
            return "%s/%s" % (kind, cls)
        tag = lambda x: ("%s%s" % (who(x) if x in wj else "no-writer", "=same-final" if imgs[b_name][x] == fim_b[x] else ""))
        got_cause = (dict(collections.Counter(tag(x) for x in only_filed)), dict(collections.Counter(tag(x) for x in only_cur)),
                     dict(collections.Counter(who(x) if x in wj else "no-writer" for x in diff_vals)))
        print("  causes: changed only in %s %s; changed only here %s; common bytes with a differing final value %s" %
              (run_name, got_cause[0], got_cause[1], got_cause[2]))
        expected_cause = EXPECT_CAUSES[label] if MUT != "xr-pin" else ({}, {}, {})
        check(got_cause == expected_cause, "window %s: the cross-run causes changed: %s" % (label, got_cause))
        check(len(coincide) == len(only_filed) and (MUT != "xr-expect-equal" or not only_filed),
              "window %s: a byte changed in %s has no replayed writer here" % (label, run_name))
        account[label]["xr"] = (len(only_cur), len(only_filed), len(common), len(diff_vals))
    print("single-invocation chain verified (models unverified beyond this chain)")


if __name__ == "__main__":
    main()
