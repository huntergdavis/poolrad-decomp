#!/usr/bin/env python3
"""C composition of the run-037 `10D2:0165 -> 10D2:13C6` window; print no game or RAM payload.

Composes the existing C models `pr_r08ae_dispatch_00dc_cpu`, `pr_r08ae_table_17ae_cpu`,
`pr_r08ae_table_1742_cpu` and `pr_r08ae_vector_13b1_cpu` (the order the 030 composition test uses) and
checks them three ways, building the models with ctypes from the recovered sources (no new model code):

  A. trace-free chain: start from the native 0165 RAM image and the manual CPU transcription only, run
     dispatch -> 17AE -> dispatch -> 1742 -> dispatch -> 13B1 (stopped at its INT 21h by a NULL
     environment) and diff the composed RAM with native 13C6 byte by byte, attributing every delta byte
     to the component that last changed it.  The same chain is run on run 030's own 0165/13C6 images.
  B. per-span verification against the hash-locked W2 LOGL trace: every span is run from the trace's
     entry state over base + store-replay shadow memory and its exit registers, flags and RAM are compared
     with the trace row (and the shadow) at the span's exit.
  C. the six bytes the model writes beyond the trace (the 13B1 model stops AFTER the INT 21h frame push,
     the trace stops before the INT executes) are checked against run 030's later native 0747 image.

Nothing here verifies the C models themselves beyond this window: their own audits are separate.  Needs
capstone (store replay shadow) and a C compiler; run without `python3 -I`.  `POOLRAD_GCOMP_MUTANT=<name>`
perturbs one branch (see check_slot_a_gap_compose_037.sh).
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
MUT = os.environ.get("POOLRAD_GCOMP_MUTANT", "")
RUN037 = {
    "LOGL-W2-0165-to-13C6.TXT": "9a18628141a799e7bbc519d7e66aeed46ee73f06eda2bdfe887f5eb03199000d",
    "RAM-10D2-0165.BIN": "0914b16f3278486195a111769095f97c55315330a12f278f2c3dc86a2d84b643",
    "RAM-10D2-13C6.BIN": "7f255423f12188d9d961c9bb8f041bb69f51b175466f188d3bb84427332c841e",
    "CPU-10D2-0165.TXT": "60cc866a7629f1a6e94dac7352e151bbf7547759796a7b0cd1c3d21cdf9fca85",
    "CPU-10D2-13C6.TXT": "653663076aa7e53dca5aa9e92a728b34d8aeac4e17c45f8aac2c60a65c8f3521",
}
RUN030 = {
    "RAM-10D2-0165.BIN": "b227e2c1fadb3b1b8d3dd00a1020d83edddcdc193e16acbe26c93f181c5890c8",
    "RAM-10D2-13C6.BIN": "342882515c7dd4f852a17a15bc78c4d7bf147b2cb287e3e7169abbcaee02e270",
    "RAM-2E60-0747.BIN": "99d632bb8349102c6f41a495ce212367e07a4d35b32206b9654703e2e065e270",
    "CPU-10D2-0165.TXT": "be674fba164d5b5bde0407d34d6cfd3f6f4515a05247c80bd227a2c5d3f73218",
    "CPU-10D2-13C6.TXT": "c5403b94eb3b242d82c7646dff8c566bcd54412e02af6e3d4e0112c97bf77c35",
}
SOURCES = ("resident_08ae_dispatch_00dc_cpu", "resident_08ae_table_17ae_cpu", "resident_08ae_table_1742_cpu",
           "resident_08ae_vector_13b1_cpu")
RAM_SIZE = 655360
RAM_1M = 1 << 20
VIDEO_BASE = 0xA0000
LINES = 766
DELTA_BYTES = 54
SS_BASE = 0x1D160
FRAME_SIZE = 6                    # FLAGS, CS, next IP pushed by the model's INT 21h
STATUS = {0: "RETURN", 1: "ARGUMENT", 2: "CALL_137C", 3: "CALL_17AE", 4: "CALL_1742", 5: "CALL_181A",
          6: "CALL_006C", 7: "CALL_13B1"}
FIELDS = "ax bx cx dx si di bp sp cs ip ds es ss flags".split()


def check(condition, message):
    if not condition:
        raise SystemExit(message)


class Cpu(ctypes.Structure):
    _fields_ = [(n, ctypes.c_uint16) for n in FIELDS]


def cpu_from_file(path):
    d = {}
    for part in Path(path).read_text().split():
        name, _, value = part.partition("=")
        check(len(value) == 4 and name.lower() not in d, "malformed CPU field in " + Path(path).name)
        d[name.lower()] = int(value, 16)
    check(len(d) == 14, "CPU file is not 14 fields")
    c = Cpu()
    for n in FIELDS:
        setattr(c, n, d[n])
    return c


def cpu_from_row(row):
    c = Cpu()
    for n in FIELDS:
        if n in row[5]:
            setattr(c, n, row[5][n])
    c.cs, c.ip, c.flags = row[1], row[2], row[6]
    return c


def cpu_diff(c, want, skip=()):
    return {n: (getattr(c, n), getattr(want, n)) for n in FIELDS if n not in skip and getattr(c, n) != getattr(want, n)}


def build_library(build_dir):
    build_dir.mkdir(parents=True, exist_ok=True)
    path = build_dir / "gap_compose_037.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2", "-fPIC", "-shared",
                    "-I", str(ROOT / "include"), *[str(ROOT / "src/recovered" / (s + ".c")) for s in SOURCES],
                    "-o", str(path)], check=True)
    lib = ctypes.CDLL(str(path))
    u8, p = ctypes.POINTER(ctypes.c_uint8), ctypes.POINTER
    for name in ("pr_r08ae_dispatch_00dc_cpu", "pr_r08ae_table_17ae_cpu", "pr_r08ae_table_1742_cpu"):
        f = getattr(lib, name)
        f.argtypes, f.restype = [u8, ctypes.c_size_t, p(Cpu)], ctypes.c_int
    f = lib.pr_r08ae_vector_13b1_cpu
    f.argtypes, f.restype = [u8, ctypes.c_size_t, p(Cpu), ctypes.c_void_p], ctypes.c_int
    return lib


def call_model(lib, name, img, cpu):
    ram = (ctypes.c_uint8 * RAM_1M).from_buffer(img)
    f = getattr(lib, name)
    if name == "pr_r08ae_vector_13b1_cpu":
        return f(ram, RAM_1M, ctypes.byref(cpu), None)      # NULL environment: stop at the INT 21h frame push
    return f(ram, RAM_1M, ctypes.byref(cpu))


# ---- trace parsing and the store-replay shadow (same method as the 037 store-replay audit) ----
LINE = re.compile(
    r"^(\d+\.\d+)\s+([0-9A-F]{4}):([0-9A-F]{8})\s+(.*?)\s{2,}((?:[0-9A-F]{2} )*[0-9A-F]{2})\s+"
    r"EAX:([0-9A-F]{8}) EBX:([0-9A-F]{8}) ECX:([0-9A-F]{8}) EDX:([0-9A-F]{8}) ESI:([0-9A-F]{8}) "
    r"EDI:([0-9A-F]{8}) EBP:([0-9A-F]{8}) ESP:([0-9A-F]{8}) DS:([0-9A-F]{4}) ES:([0-9A-F]{4}) "
    r"FS:([0-9A-F]{4}) GS:([0-9A-F]{4}) SS:([0-9A-F]{4}) CF:(\d) ZF:(\d) SF:(\d) OF:(\d) AF:(\d) "
    r"PF:(\d) IF:(\d) TF:(\d) VM:\d FLG:([0-9A-F]{8})")


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
            flags = ((int(g[26], 16) & 0xF000) | 0x0002 | cf | (pf << 2) | (af << 4) | (zf << 6) |
                     (sf << 7) | (tf << 8) | (iff << 9) | (of << 11))
            rows.append((float(g[0]), int(g[1], 16), int(g[2], 16) & 0xFFFF, g[3],
                         bytes(int(x, 16) for x in g[4].split()), regs, flags))
    return rows


def mnemonic(text):
    parts = text.split()
    return parts[0] if parts else ""


def replay(rows, base, snap_rows):
    """Store replay of the trace over `base`; returns {row: shadow dict before that row executes}."""
    import capstone
    from capstone import x86 as X
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_16)
    md.detail = True
    mem, snaps = {}, {}

    def rd(a):
        a &= 0xFFFFF
        check(a < VIDEO_BASE, "a replayed read depends on unobserved display memory")
        return mem.get(a, base[a])

    def rdw(a):
        return rd(a) | (rd(a + 1) << 8)

    def wr(a, n, v):
        a &= 0xFFFFF
        for k in range(n):
            b = (a + k) & 0xFFFFF
            check(b < VIDEO_BASE, "store at/above 0xA0000")
            mem[b] = (v >> (8 * k)) & 255

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

    alu = {"add": lambda o, s: o + s, "sub": lambda o, s: o - s, "and": lambda o, s: o & s,
           "or": lambda o, s: o | s, "xor": lambda o, s: o ^ s}
    last = len(rows) - 1
    for i, (ts, cs, ip, text, byt, r, flags) in enumerate(rows):
        if i in snap_rows:
            snaps[i] = dict(mem)
        if i == 0 or i == last:
            continue         # duplicated first line / the 13C6 stop state (not executed)
        check(cs == 0x10D2 and not text.startswith("callback") and mnemonic(text) not in ("in", "out"),
              "undeclared host effect in the window at row %d" % i)
        ins = next(md.disasm(byt, ip), None)
        check(ins is not None and ins.addr_size == 2, "undecoded instruction at row %d" % i)
        mn, ops = ins.mnemonic, ins.operands
        sspb, sp = r["ss"] << 4, r["sp"]
        if mn == "push":
            wr(sspb + ((sp - 2) & 0xFFFF), 2, val(r, ops[0], 2) & 0xFFFF)
        elif mn == "call":
            wr(sspb + ((sp - 2) & 0xFFFF), 2, (ip + ins.size) & 0xFFFF)
        elif mn == "lcall":
            wr(sspb + ((sp - 2) & 0xFFFF), 2, cs)
            wr(sspb + ((sp - 4) & 0xFFFF), 2, (ip + ins.size) & 0xFFFF)
        elif mn.split()[-1] in ("stosb", "stosw", "movsb", "movsw", "xchg", "int", "pushf", "pusha"):
            raise SystemExit("store form %s appears in the window" % mn)
        elif ops and ops[0].type == X.X86_OP_MEM and mn not in (
                "cmp", "test", "lea", "les", "lds", "mul", "imul", "div", "idiv", "jmp", "ljmp", "call", "lcall", "push") \
                and not mn.startswith("j"):
            a, size = ea(r, ops[0]), ops[0].size
            old = rd(a) if size == 1 else rdw(a)
            if mn == "mov":
                new = val(r, ops[1], size)
            elif mn in alu:
                new = alu[mn](old, val(r, ops[1], size))
            elif mn == "dec":
                new = old - 1
            elif mn == "inc":
                new = old + 1
            else:
                raise SystemExit("unsupported memory-destination form %s at row %d" % (mn, i))
            wr(a, size, new & ((1 << (8 * size)) - 1))
    return snaps


def image(base, overlay):
    m = bytearray(base) + bytearray(RAM_1M - len(base))
    for a, v in overlay.items():
        m[a] = v
    return m


def span_of(rows, ip, nth=0):
    """Entry row (first callee row) and exit row (row after the matching return) of the nth call to 10D2:ip."""
    hits = [i for i in range(1, len(rows) - 1) if (rows[i][1], rows[i][2]) == (0x10D2, ip) and
            mnemonic(rows[i - 1][3]) in ("call", "lcall")]
    check(len(hits) > nth, "no call to %04X" % ip)
    e = hits[nth]
    sp0 = rows[e][5]["sp"]
    x = next((i for i in range(e + 1, len(rows)) if mnemonic(rows[i][3]) in ("ret", "retf") and rows[i][5]["sp"] == sp0), None)
    return e, (x + 1 if x is not None else None)


def run_chain(lib, base, cpu0):
    """Trace-free chain from a native 0165 image and CPU; returns (image, final cpu, per-step records)."""
    img = image(base, {})
    c = Cpu.from_buffer_copy(cpu0)
    if MUT == "entry-ds":
        c.ds = (c.ds + 1) & 0xFFFF
    steps, pre = [], {}

    def run(label, name, expect_status):
        before = bytes(img[:RAM_SIZE])
        pre[label] = before
        if MUT == f"drop-{label}":
            scratch, c2 = bytearray(img), Cpu.from_buffer_copy(c)
            st = call_model(lib, name, scratch, c2)
            c.__init__(*[getattr(c2, n) for n in FIELDS])        # CPU advances, RAM effects are dropped
        else:
            st = call_model(lib, name, img, c)
        check(st == expect_status, "%s returned %s, expected %s" % (label, STATUS.get(st, st), STATUS.get(expect_status, expect_status)))
        after = bytes(img[:RAM_SIZE])
        changed = [a for a in range(RAM_SIZE) if before[a] != after[a]]
        steps.append((label, Cpu.from_buffer_copy(c), changed))

    d = "pr_r08ae_dispatch_00dc_cpu"
    run("dispatch-0", d, 3)
    run("17AE", "pr_r08ae_table_17ae_cpu", 0)
    if MUT == "glue-ip":
        c.ip = (c.ip + 1) & 0xFFFF
    run("dispatch-1", d, 4)
    run("1742", "pr_r08ae_table_1742_cpu", 0)
    run("dispatch-2", d, 7)
    run("13B1", "pr_r08ae_vector_13b1_cpu", 2)               # ENV_DOS: stopped after the INT 21h frame push
    return img, c, steps, pre["13B1"]


def account(label, lib, run_dir, base, native, cpu0, cpu_stop, frame_ref):
    """Part A/C for one run: trace-free chain vs native 13C6."""
    img, c, steps, pre13 = run_chain(lib, base, cpu0)
    delta = [a for a in range(RAM_SIZE) if base[a] != native[a]]
    check(len(delta) == DELTA_BYTES, "%s native delta size mismatch" % label)
    frame_lo = (c.ss << 4) + c.sp
    frame = set(range(frame_lo, frame_lo + FRAME_SIZE))
    pushes13 = {a for a in steps[-1][2] if a not in frame}
    if MUT == "frame-overlap":
        pushes13 = pushes13 | {frame_lo}        # mutant: pretend 13B1's own pushes touch the frame
    # the model stops AFTER the INT 21h frame push, the native stop is BEFORE it: restore the six frame bytes to
    # their values before the 13B1 model ran, exact only if 13B1's own pre-INT pushes do not touch them
    check(not (pushes13 & frame),
          "%s: 13B1's own pushes overlap the INT frame, the restore would be inexact" % label)
    got = bytearray(img[:RAM_SIZE])
    for a in frame:
        if MUT != "frame-keep":
            got[a] = pre13[a]
    mism = [a for a in range(RAM_SIZE) if got[a] != native[a]]
    check(not mism, "%s composed pre-INT image differs from native 13C6 in %d byte(s)" % (label, len(mism)))
    in_frame_delta = [a for a in delta if a in frame]
    # last changing component of every delta byte (the model-only frame write is not attributed)
    last = {}
    for lab, _, changed in steps:
        for a in changed:
            if lab == "13B1" and a in frame:
                continue
            last[a] = lab.split("-")[0]
    explained = [a for a in delta if got[a] == native[a]]
    unexplained = [a for a in delta if got[a] != native[a]]
    by = collections.Counter(last.get(a, "none") for a in delta)
    print("%s: composed from native 0165 + transcription only; delta %d bytes, explained %d, unexplained %d; "
          "model-only INT frame SS:%04X-%04X (%d bytes, %d of them overlap native delta bytes), composed pre-INT image "
          "equals native 13C6 in all %d bytes" %
          (label, len(delta), len(explained), len(unexplained), frame_lo - SS_BASE, frame_lo - SS_BASE + FRAME_SIZE - 1,
           FRAME_SIZE, len(in_frame_delta), RAM_SIZE))
    print("  delta bytes by last-changing component:", dict(sorted(by.items())))
    print("  bytes changed by each step:", [(lab, len(ch)) for lab, _, ch in steps])
    # omission controls: revert the bytes whose last change was component X to the 0165 value
    ctrl = {}
    for comp in sorted(set(last.get(a, "none") for a in delta)):
        alt = bytearray(got)
        for a in delta:
            if last.get(a) == comp:
                alt[a] = base[a]
        ctrl[comp] = sum(1 for a in delta if alt[a] != native[a])
    print("  omission controls (delta bytes left unexplained):", ctrl)
    want = cpu_stop
    d = cpu_diff(c, want)
    check(set(d) == {"sp", "flags"} and c.sp == (want.sp - FRAME_SIZE) & 0xFFFF and c.flags == want.flags & ~0x0200,
          "%s final CPU differs from the transcription beyond the INT effects: %s" % (label, d))
    print("  final CPU vs transcription: equal except the INT effects (SP %04X vs %04X, IF cleared: flags %04X vs %04X)" %
          (c.sp, want.sp, c.flags, want.flags))
    check(not unexplained, "%s: %d delta byte(s) unexplained by the composition" % (label, len(unexplained)))
    # frame cross-check: the later native image of run 030 holds the frame below its SP
    fv = bytes(img[frame_lo:frame_lo + FRAME_SIZE])
    shift = 1 if MUT == "frame-ref-shift" else 0     # mutant: compare against the wrong reference bytes
    ref_ok = frame_ref is not None and frame_ref[frame_lo + shift:frame_lo + shift + FRAME_SIZE] == fv
    if frame_ref is not None:
        print("  INT frame bytes (model-only, beyond the trace): equal to run 030's later native 0747 image: %s" % ref_ok)
        check(ref_ok, "%s: INT frame bytes differ from run 030's native 0747 image" % label)
    return bytes(got), frame, last, delta


def spans(lib, rows, base, snaps, e_x):
    """Part B: each span from the trace's entry state over base + shadow; exit vs the trace row and shadow."""
    names = ("dispatch-0", "17AE", "dispatch-1", "1742", "dispatch-2", "13B1")
    models = ("pr_r08ae_dispatch_00dc_cpu", "pr_r08ae_table_17ae_cpu", "pr_r08ae_dispatch_00dc_cpu",
              "pr_r08ae_table_1742_cpu", "pr_r08ae_dispatch_00dc_cpu", "pr_r08ae_vector_13b1_cpu")
    statuses = (3, 0, 4, 0, 7, 2)
    bounds = e_x
    ok_all = True
    print("Per-span verification from the trace's entry state (model exit vs trace row and replay shadow):")
    for k, (name, model, st) in enumerate(zip(names, models, statuses)):
        e, x = bounds[k]
        img = image(base, snaps[e])
        c = cpu_from_row(rows[e])
        if MUT == "span-entry-sp":
            c.sp = (c.sp + 2) & 0xFFFF
        img0 = bytes(img)
        got = call_model(lib, model, img, c)
        want = cpu_from_row(rows[x])
        shadow = image(base, snaps[x]) if x in snaps else None
        skip, frame = (), set()
        if name == "13B1":                          # the trace stops BEFORE the INT, the model after the frame push
            skip = ("sp", "flags")
            frame = set(range((c.ss << 4) + c.sp, (c.ss << 4) + c.sp + FRAME_SIZE))
            check(c.sp == (want.sp - FRAME_SIZE) & 0xFFFF and c.flags == want.flags & ~0x0200, "13B1 INT effects unexpected")
            for a in frame:
                img[a] = img0[a]         # undo the model-only frame push before comparing RAM
        regs = cpu_diff(c, want, skip)
        ram = [a for a in range(RAM_SIZE) if img[a] != shadow[a]]
        extra = len(frame)
        print("  %-11s rows %3d->%3d status %-9s exit-regs %s  RAM vs shadow %d diff(s)%s" %
              (name, e, x, "ENV_DOS" if name == "13B1" else STATUS.get(got, got), "equal" if not regs else "DIFFER %s" % regs, len(ram),
               "  (+%d INT frame bytes beyond the trace)" % extra if extra else ""))
        ok_all &= got == st and not regs and not ram
    check(ok_all, "a span does not reproduce the trace")


def main():
    check(len(sys.argv) == 4, "usage: audit_slot_a_gap_compose_037.py RUN037_DIR RUN030_DIR BUILD_DIR")
    run37, run30, build = Path(sys.argv[1]), Path(sys.argv[2]), Path(sys.argv[3])
    for d, locks in ((run37, RUN037), (run30, RUN030)):
        for name, digest in locks.items():
            check(sha256((d / name).read_bytes()).hexdigest() == digest, "%s/%s identity mismatch" % (d.name, name))
    lib = build_library(build)
    rows = parse(run37 / "LOGL-W2-0165-to-13C6.TXT")
    check(len(rows) == LINES and (rows[0][1], rows[0][2]) == (0x10D2, 0x0165) and rows[0][1:3] == rows[1][1:3] and
          (rows[-1][1], rows[-1][2]) == (0x10D2, 0x13C6), "window start/stop rows mismatch")
    b37, n37 = (run37 / "RAM-10D2-0165.BIN").read_bytes(), (run37 / "RAM-10D2-13C6.BIN").read_bytes()
    b30, n30 = (run30 / "RAM-10D2-0165.BIN").read_bytes(), (run30 / "RAM-10D2-13C6.BIN").read_bytes()
    r747 = (run30 / "RAM-2E60-0747.BIN").read_bytes()
    print("run 037 window: %d trace lines, native delta from the locked images; models built from %d recovered sources" %
          (len(rows), len(SOURCES)))
    # frame reference: run 030's native 0747 holds the INT frame below its SP; compared with each chain's frame
    got37, frame37, last37, delta37 = account("run 037", lib, run37, b37, n37, cpu_from_file(run37 / "CPU-10D2-0165.TXT"),
                                              cpu_from_file(run37 / "CPU-10D2-13C6.TXT"), r747)
    got30, frame30, last30, delta30 = account("run 030", lib, run30, b30, n30, cpu_from_file(run30 / "CPU-10D2-0165.TXT"),
                                              cpu_from_file(run30 / "CPU-10D2-13C6.TXT"), r747)
    print("same chain on two runs: composed images differ from each other at %d byte(s) (host-time noise in the "
          "inputs); delta address sets equal: %s" %
          (sum(1 for a in range(RAM_SIZE) if got37[a] != got30[a]), set(delta37) == set(delta30)))
    # B: spans against the trace
    e17, x17 = span_of(rows, 0x17AE)
    e42, x42 = span_of(rows, 0x1742)
    e13, _ = span_of(rows, 0x13B1)
    last_row = len(rows) - 1
    bounds = [(1, e17), (e17, x17), (x17, e42), (e42, x42), (x42, e13), (e13, last_row)]
    snap_rows = {r for pair in bounds for r in pair}
    snaps = replay(rows, b37, snap_rows)
    spans(lib, rows, b37, snaps, bounds)
    print("run-037 window C composition verified (model-only status unverified beyond this window)")


if __name__ == "__main__":
    main()
