#!/usr/bin/env python3
"""C composition of the run-030 `10D2:13C6 -> 2E60:0747` window against native 0747 and the W3 trace.

Print no game or RAM payload.  Composes the existing C models `pr_r08ae_dispatch_00dc_cpu`,
`pr_r08ae_table_17ae_cpu`, `pr_r08ae_table_1742_cpu` and `pr_r08ae_vector_13b1_cpu` (ctypes, built from the
recovered sources, no new model code) under a declared environment, and checks them three ways:

  A. trace-free chain: from the native 0165 RAM image and the manual CPU transcription only, run the chain
     to the `13B1` model with the declared environment (the AH=25 vector service, `IN 61h` and the OUTs)
     and diff the composed RAM with native 0747 byte by byte, attributing every byte of the native
     13C6 -> 0747 delta to the C model (the INT frame push) or to the declared DOS binding.  `13B1` cannot be
     resumed at `13C6`, so the window is reached through the whole function from its entry.
  B. against the hash-locked W3 trace: the registers the model hands to the DOS hook, the INT frame, the ordered
     OUT/IN events, the model's return state at 10D2:0177 and the dispatcher's return span (trace entry state)
     to 2E60:0747, each compared with the trace rows.
  C. the native transcriptions: trace first/last lines vs CPU-10D2-13C6.TXT and CPU-2E60-0747.TXT.

The DOS AH=25 service (vector write, PSP stack pointer and the nine saved registers, as the run003 binding) is a
DECLARED HYPOTHESIS supported only by byte equality: the trace shows the callback returning straight to the stub's
iret and its internals are not instruction-traced.  The IN 61h value is a measured input read from the trace.
Needs a C compiler; run without `python3 -I`.  `POOLRAD_LINK_MUTANT=<name>` perturbs one branch (see the check script).
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
MUT = os.environ.get("POOLRAD_LINK_MUTANT", "")
RUN030 = {
    "LOGL-W3-13C6-to-0747.TXT": "4960c5b94fe3c09bf487a1e71069a1297c0648bab9927d07c9409835b1432fc0",
    "RAM-10D2-0165.BIN": "b227e2c1fadb3b1b8d3dd00a1020d83edddcdc193e16acbe26c93f181c5890c8",
    "RAM-10D2-13C6.BIN": "342882515c7dd4f852a17a15bc78c4d7bf147b2cb287e3e7169abbcaee02e270",
    "RAM-2E60-0747.BIN": "99d632bb8349102c6f41a495ce212367e07a4d35b32206b9654703e2e065e270",
    "CPU-10D2-0165.TXT": "be674fba164d5b5bde0407d34d6cfd3f6f4515a05247c80bd227a2c5d3f73218",
    "CPU-10D2-13C6.TXT": "c5403b94eb3b242d82c7646dff8c566bcd54412e02af6e3d4e0112c97bf77c35",
    "CPU-2E60-0747.TXT": "c4b279854701fd67463ae3f7cb58a1d9eb805c21891786d10175bddbcfa56396",
}
SOURCES = ("resident_08ae_dispatch_00dc_cpu", "resident_08ae_table_17ae_cpu", "resident_08ae_table_1742_cpu",
           "resident_08ae_vector_13b1_cpu")
RAM_SIZE = 655360
RAM_1M = 1 << 20
LINES = 26
DELTA_BYTES = 26
SS_BASE = 0x1D160
FIELDS = "ax bx cx dx si di bp sp cs ip ds es ss flags".split()
STATUS = {0: "RETURN", 1: "ARGUMENT", 2: "CALL_137C", 3: "CALL_17AE", 4: "CALL_1742", 5: "CALL_181A",
          6: "CALL_006C", 7: "CALL_13B1"}
EXPECT_OUTS = ((0x40, 0xFF), (0x40, 0xFF), (0x61, 0x30), (0xC0, 0x9F), (0xC0, 0xBF), (0xC0, 0xDF), (0xC0, 0xFF))
IVT_08 = 0x20


def check(condition, message):
    if not condition:
        raise SystemExit(message)


class Cpu(ctypes.Structure):
    _fields_ = [(n, ctypes.c_uint16) for n in FIELDS]


DOS_HOOK = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8), ctypes.POINTER(Cpu),
                            ctypes.POINTER(ctypes.c_uint16))
OUT_HOOK = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.c_uint16, ctypes.c_uint8)
IN_HOOK = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.c_uint16, ctypes.POINTER(ctypes.c_uint8))


class Env(ctypes.Structure):
    _fields_ = [("context", ctypes.c_void_p), ("dos", DOS_HOOK), ("port_out", OUT_HOOK), ("port_in", IN_HOOK)]


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
    path = build_dir / "link_compose_0747.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2", "-fPIC", "-shared",
                    "-I", str(ROOT / "include"), *[str(ROOT / "src/recovered" / (s + ".c")) for s in SOURCES],
                    "-o", str(path)], check=True)
    lib = ctypes.CDLL(str(path))
    u8, p = ctypes.POINTER(ctypes.c_uint8), ctypes.POINTER
    for name in ("pr_r08ae_dispatch_00dc_cpu", "pr_r08ae_table_17ae_cpu", "pr_r08ae_table_1742_cpu"):
        f = getattr(lib, name)
        f.argtypes, f.restype = [u8, ctypes.c_size_t, p(Cpu)], ctypes.c_int
    f = lib.pr_r08ae_vector_13b1_cpu
    f.argtypes, f.restype = [u8, ctypes.c_size_t, p(Cpu), p(Env)], ctypes.c_int
    return lib


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


def image(base):
    return bytearray(base) + bytearray(RAM_1M - len(base))


def call_plain(lib, name, img, cpu):
    ram = (ctypes.c_uint8 * RAM_1M).from_buffer(img)
    return getattr(lib, name)(ram, RAM_1M, ctypes.byref(cpu))


class Binding:
    """The declared environment of the 13B1 model: DOS AH=25 vector service, IN 61h and the OUT log."""

    def __init__(self, input61):
        self.input61, self.calls, self.outs, self.ins = input61, 0, [], []
        self.writes = []                   # (address, byte, label) of every byte the DOS binding stores
        self.cpu_at_dos = None

    def word(self, ram, addr, value, label):
        for k in (0, 1):
            ram[addr + k] = (value >> (8 * k)) & 255
            self.writes.append((addr + k, (value >> (8 * k)) & 255, label))

    def dos(self, ctx, ram, cpu, flags_out):
        c = cpu.contents
        rd = lambda a: ram[a] | (ram[a + 1] << 8)
        if (self.calls or c.ax != 0x2508 or c.dx != 0xFEA5 or c.ds != 0xF000 or c.ip != 0x13C6 or c.flags != 0x7046 or
                rd(IVT_08) != 0x13EA or rd(IVT_08 + 2) != 0x10D2 or ram[0x8140] != 0xCD or ram[0x8141] != 0x20):
            return 1
        self.cpu_at_dos = Cpu.from_buffer_copy(c)
        stack = (c.ss << 4) + c.sp
        if rd(stack) != 0x13C8 or rd(stack + 2) != c.cs:
            return 1
        if MUT != "bind-skip":
            regs = [c.ax, c.bx, c.cx, c.dx, c.si, c.di, c.bp, c.ds, c.es]
            if MUT == "dos-regorder":
                regs[0], regs[1] = regs[1], regs[0]
            for i, v in enumerate(regs):
                self.word(ram, stack - 18 + 2 * i, v, "dos-regsave")
            self.word(ram, 0x8140 + 0x2E, (c.sp - (16 if MUT == "dos-psp" else 18)) & 0xFFFF, "dos-psp")
            self.word(ram, 0x8140 + 0x30, c.ss, "dos-psp")
            self.word(ram, IVT_08, c.dx, "ivt")
            self.word(ram, IVT_08 + 2, (c.ds + (1 if MUT == "dos-vseg" else 0)) & 0xFFFF, "ivt")
        flags_out[0] = {"dos-flags-if": 0x7046, "dos-flags-zf": 0x7202}.get(MUT, 0x7246)
        self.calls += 1
        return 0

    def port_out(self, ctx, port, value):
        self.outs.append((port, value))
        return 0

    def port_in(self, ctx, port, value):
        self.ins.append(port)
        value[0] = self.input61
        return 0

    def env(self):
        self._h = (DOS_HOOK(self.dos), OUT_HOOK(self.port_out), IN_HOOK(self.port_in))
        return Env(None, *self._h)


def run13(lib, img, cpu, binding):
    ram = (ctypes.c_uint8 * RAM_1M).from_buffer(img)
    env = binding.env() if binding else None
    return lib.pr_r08ae_vector_13b1_cpu(ram, RAM_1M, ctypes.byref(cpu), ctypes.byref(env) if env else None)


def chain_to_13b1(lib, base, cpu0):
    img = image(base)
    c = Cpu.from_buffer_copy(cpu0)
    for name, want in (("pr_r08ae_dispatch_00dc_cpu", 3), ("pr_r08ae_table_17ae_cpu", 0), ("pr_r08ae_dispatch_00dc_cpu", 4),
                       ("pr_r08ae_table_1742_cpu", 0), ("pr_r08ae_dispatch_00dc_cpu", 7)):
        st = call_plain(lib, name, img, c)
        check(st == want, "%s returned %s, expected %s" % (name, STATUS.get(st, st), STATUS.get(want, want)))
    return img, c


def main():
    check(len(sys.argv) == 3, "usage: audit_slot_a_link_compose_13c6_0747.py RUN030_DIR BUILD_DIR")
    run, build = Path(sys.argv[1]), Path(sys.argv[2])
    for name, digest in RUN030.items():
        check(sha256((run / name).read_bytes()).hexdigest() == digest, "%s identity mismatch" % name)
    lib = build_library(build)
    rows = parse(run / "LOGL-W3-13C6-to-0747.TXT")
    check(len(rows) == LINES and (rows[0][1], rows[0][2]) == (0x10D2, 0x13C6) and rows[0][1:3] == rows[1][1:3] and
          (rows[-1][1], rows[-1][2]) == (0x2E60, 0x0747), "W3 start/stop rows mismatch")
    b0165 = (run / "RAM-10D2-0165.BIN").read_bytes()
    n13c6 = (run / "RAM-10D2-13C6.BIN").read_bytes()
    n0747 = (run / "RAM-2E60-0747.BIN").read_bytes()
    cpu13, cpu747 = cpu_from_file(run / "CPU-10D2-13C6.TXT"), cpu_from_file(run / "CPU-2E60-0747.TXT")
    # C. trace vs the manual transcriptions
    check(not cpu_diff(cpu_from_row(rows[1]), cpu13) and not cpu_diff(cpu_from_row(rows[-1]), cpu747),
          "trace first/last line disagrees with the CPU transcriptions")
    print("transcriptions: all 14 fields (flags included) of CPU-10D2-13C6.TXT and CPU-2E60-0747.TXT equal the trace's first and last lines")
    # measured inputs from the trace: the IN 61h value and the ordered OUTs
    ins = [i for i, r in enumerate(rows) if mnemonic(r[3]) == "in"]
    check(len(ins) == 1 and rows[ins[0]][3].split()[1].split(",")[-1] == "61", "W3 IN sequence mismatch")
    input61 = rows[ins[0] + 1][5]["ax"] & 0xFF
    trace_outs = [(int(r[3].split()[1].split(",")[0], 16), r[5]["ax"] & 0xFF) for r in rows[1:-1] if mnemonic(r[3]) == "out"]
    check(trace_outs == list(EXPECT_OUTS) and input61 == 0x30, "W3 port events changed: %s IN=%02X" % (trace_outs, input61))
    if MUT == "in61-03":
        input61 = 0x03                                  # the old synthetic value, refuted by the trace
    # A. trace-free chain to the 13B1 entry, then the model with and without the environment
    img0, c0 = chain_to_13b1(lib, b0165, cpu_from_file(run / "CPU-10D2-0165.TXT"))
    pre = bytes(img0[:RAM_SIZE])
    stop_img, stop_cpu = bytearray(img0), Cpu.from_buffer_copy(c0)
    check(run13(lib, stop_img, stop_cpu, None) == 2, "13B1 NULL environment did not stop at the INT")
    frame_lo = (stop_cpu.ss << 4) + stop_cpu.sp
    frame = set(range(frame_lo, frame_lo + 6))
    changed = [a for a in range(RAM_SIZE) if pre[a] != stop_img[a]]
    frame_changed = [a for a in changed if a in frame]
    pushes13 = [a for a in changed if a not in frame]
    # the 13B1 model's own pre-INT pushes belong to the 0165 -> 13C6 window; with the INT frame restored the
    # composed image at the INT must equal native 13C6 (the previous link)
    pre_int = bytearray(stop_img[:RAM_SIZE])
    for a in frame:
        pre_int[a] = pre[a]
    check(bytes(pre_int) == n13c6, "the composed pre-INT image differs from native 13C6")
    print("composed image at the INT (frame restored) equals native 13C6 in all %d bytes (%d pre-INT stack pushes by 13B1 belong to the previous window)" %
          (RAM_SIZE, len(pushes13)))
    b = Binding(input61)
    final_img, final_cpu = bytearray(img0), Cpu.from_buffer_copy(c0)
    st = run13(lib, final_img, final_cpu, b)
    check(st == 0 and b.calls == 1 and len(b.outs) == 7 and len(b.ins) == 1 and final_cpu.ip == 0x0177,
          "13B1 with the environment returned %d (calls %d, outs %d, ins %d, ip %04X)" % (st, b.calls, len(b.outs), len(b.ins), final_cpu.ip))
    tail = Cpu.from_buffer_copy(final_cpu)
    st = call_plain(lib, "pr_r08ae_dispatch_00dc_cpu", final_img, final_cpu)
    check(st == 0, "dispatcher did not return: %s" % STATUS.get(st, st))
    got = bytes(final_img[:RAM_SIZE])
    delta = [a for a in range(RAM_SIZE) if n13c6[a] != n0747[a]]
    check(len(delta) == DELTA_BYTES, "native 13C6->0747 delta size mismatch: %d" % len(delta))
    mism = [a for a in range(RAM_SIZE) if got[a] != n0747[a]]
    hook_bytes = {}
    for a, v, lab in b.writes:
        hook_bytes[a] = lab
    after_pre = [a for a in range(RAM_SIZE) if stop_img[a] != final_img[a] and a not in frame]
    writer = {}
    for a in frame_changed:
        writer[a] = "c-model INT frame"
    for a in hook_bytes:
        writer[a] = "declared DOS binding:" + hook_bytes[a]
    explained = [a for a in delta if got[a] == n0747[a]]
    by = collections.Counter(writer.get(a, "none") for a in delta)
    cpu_final = cpu_diff(final_cpu, cpu747)
    print("trace-free chain 0165 -> 13B1 (declared environment) -> dispatcher return: composed RAM vs native 0747: %d differing byte(s); "
          "native 13C6->0747 delta %d bytes, explained %d, unexplained %d" % (len(mism), len(delta), len(explained), len(delta) - len(explained)))
    print("  delta bytes by writer:", dict(sorted(by.items())))
    print("  INT frame SS:%04X-%04X written by the 13B1 model (%d bytes changed); DOS binding stores %d bytes (%d changed value)" %
          (frame_lo - SS_BASE, frame_lo - SS_BASE + 5, len(frame_changed), len(b.writes), len(set(after_pre))))
    ctrl = {}
    for lab in sorted({w.split(":")[0] if w.startswith("declared") else w for w in by if w != "none"}):
        alt = bytearray(got)
        for a in delta:
            if writer.get(a, "").startswith(lab):
                alt[a] = n13c6[a]
        ctrl[lab] = sum(1 for a in delta if alt[a] != n0747[a])
    print("  omission controls (delta bytes left unexplained):", ctrl)
    print("  final CPU at 2E60:0747 vs transcription:", "equal in all 14 fields" if not cpu_final else cpu_final)
    print("  events: OUT sequence %s; IN 61h = %02X (measured in the trace); OUT 61h value %02X" %
          ("equals the trace" if b.outs == trace_outs else "DIFFERS %s" % b.outs, input61, b.outs[2][1] if len(b.outs) > 2 else -1))
    # B. against the trace
    cd, row3, row5 = b.cpu_at_dos, rows[3], rows[5]
    check(cd is not None, "the DOS hook was not reached")
    regs_dos = cpu_diff(cd, cpu_from_row(row3), skip=("cs", "ip", "flags"))
    stack = (cd.ss << 4) + cd.sp
    frame_words = [final_img[stack + 2 * k] | (final_img[stack + 2 * k + 1] << 8) for k in range(3)]
    want_frame = [row5[2], row5[1], rows[0][6]]
    print("  registers handed to the DOS hook vs trace row 3 (callback 0038): %s; INT frame (IP, CS, FLAGS) %s the trace (%s)" %
          ("equal" if not regs_dos else "DIFFER %s" % regs_dos, "equals" if frame_words == want_frame else "DIFFERS from",
           " ".join("%04X" % w for w in frame_words)))
    ret = cpu_diff(tail, cpu_from_row(rows[23]))
    print("  13B1 return state at 10D2:0177 vs trace row 23: %s" % ("equal in all 14 fields" if not ret else "DIFFER %s" % ret))
    d_cpu = cpu_from_row(rows[23])
    if MUT == "span-entry-sp":
        d_cpu.sp = (d_cpu.sp + 2) & 0xFFFF
    if MUT == "tail-resume-ip":
        d_cpu.ip = (d_cpu.ip + 1) & 0xFFFF
    d_img = bytearray(final_img)
    st2 = call_plain(lib, "pr_r08ae_dispatch_00dc_cpu", d_img, d_cpu)
    span = cpu_diff(d_cpu, cpu_from_row(rows[25]))
    print("  dispatcher return span rows 23 -> 25 from the trace entry state: status %s, exit regs vs trace row 25 %s, RAM unchanged %s" %
          (STATUS.get(st2, st2), "equal" if not span else "DIFFER %s" % span, bytes(d_img) == bytes(final_img)))
    check(not mism, "composed RAM differs from native 0747 in %d byte(s)" % len(mism))
    check(len(explained) == len(delta), "%d delta byte(s) unexplained" % (len(delta) - len(explained)))
    check(not cpu_final, "final CPU differs from the transcription: %s" % cpu_final)
    check(b.outs == trace_outs, "OUT sequence differs from the trace")
    check(not regs_dos and frame_words == want_frame and not ret, "model events differ from the trace")
    check(st2 == 0 and not span and bytes(d_img) == bytes(final_img), "dispatcher return span differs from the trace")
    print("run-030 13C6->0747 C composition verified (model-only status unverified beyond this window)")


if __name__ == "__main__":
    main()
