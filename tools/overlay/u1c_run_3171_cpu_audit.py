#!/usr/bin/env python3
"""Original-vs-C differential for the candidate-scout draft OVL_001C_3171 (ECL run loop).

The original body runs under Unicorn from the hash-checked GAME.OVR bytes. The C side
is src/recovered/overlay_u1c_run_3171_cpu.c, compiled here. Both sides get the same
deterministic dispatcher double at the near call (+2F2D): the double is the compiled C
function pr_u1c_3171_dispatch_double, called on the original side by a Unicorn code hook
that emulates the callee's RETF. The double is a test harness, not a claim about the real
61-case dispatcher; this audit checks the run loop up to that boundary.

Compared per case: all 1,048,576 RAM bytes, all 14 CPU words (unmasked), the number of
dispatches, and the final CS:IP:SP. A mutated C (fetch bias +1) must differ on at least
one case, so the differential is shown able to fail.

Proposals only: this audit makes no credit claim and touches no accounting file.
Media stays under ignored resources/.
"""
from __future__ import annotations

import argparse
import ctypes
from hashlib import sha256
from pathlib import Path
import subprocess
import sys
import tempfile

from capstone import CS_ARCH_X86, CS_MODE_16, Cs
from unicorn import UC_ARCH_X86, UC_HOOK_CODE, UC_MODE_16, Uc
from unicorn.x86_const import (
    UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_BP, UC_X86_REG_CS, UC_X86_REG_CX,
    UC_X86_REG_DI, UC_X86_REG_DS, UC_X86_REG_DX, UC_X86_REG_EFLAGS, UC_X86_REG_ES,
    UC_X86_REG_IP, UC_X86_REG_SI, UC_X86_REG_SP, UC_X86_REG_SS,
)

ROOT = Path(__file__).resolve().parents[2]
OVR = ROOT / "resources/DOS/POOLRAD/GAME.OVR"
EXE = ROOT / "resources/DOS/POOLRAD/START.EXE"
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
EXE_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
UNIT, ENTRY, END = 0x1C, 0x3171, 0x31B3
DISPATCH_REL = 0x2F2D
BODY_SHA = "4055cdf54a816923efbebe1bed325b67193264be3f6c20491f4f383d692aa489"
CHECKER_ROWS = ("OVL_001C_3171",)
CASE_KINDS = {}
SIZE = 1 << 20

UNIT_CS, STACK_SS, STACK_SP, DATA_DS = 0x2000, 0x3000, 0x1000, 0x4000
BUF_ES, BUF_OFF = 0x5000, 0x0000
CALLER_CS, CALLER_IP = 0x1000, 0x1234
REGISTERS = (UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX, UC_X86_REG_DX,
             UC_X86_REG_SI, UC_X86_REG_DI, UC_X86_REG_BP, UC_X86_REG_SP,
             UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_DS, UC_X86_REG_ES,
             UC_X86_REG_SS, UC_X86_REG_EFLAGS)
NAMES = ("ax", "bx", "cx", "dx", "si", "di", "bp", "sp", "cs", "ip",
         "ds", "es", "ss", "flags")


class Cpu(ctypes.Structure):
    _fields_ = [(n, ctypes.c_uint16) for n in NAMES]


def check(condition, message):
    if not condition:
        raise ValueError(message)


def source():
    exe, ovr = EXE.read_bytes(), OVR.read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, "START.EXE provenance")
    check(len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA, "GAME.OVR provenance")
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    from overlay_inventory import find_units, parse_mz
    units = {u["stub_para"]: u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)}
    unit = units[UNIT]
    at = unit["ovr_offset"] + ENTRY
    body = ovr[at:at + (END - ENTRY)]
    check(len(body) == 66 and sha256(body).hexdigest() == BODY_SHA, "OVL_001C_3171 span changed")
    decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(len(decoded) == 22 and decoded[-1].mnemonic == "retf" and decoded[-1].op_str == "2",
          "OVL_001C_3171 extent changed")
    call = [i for i in decoded if i.mnemonic == "call"]
    check(len(call) == 1 and int(call[0].op_str, 16) == DISPATCH_REL, "the near call changed")
    return ovr, at, body


MUTATIONS = {
    # the three defects a reviewer found, or that the old single mutant covered
    "bias": ("(uint16_t)(di + 0x6700u)", "(uint16_t)(di + 0x6701u)"),
    "ax_entry": ("    cpu->ax = entry;                                   /* 3174 mov ax,[bp+6] */\n", ""),
    "add_ax": ("    /* ADD DI,AX writes DI, not AX; this helper must only publish flags. */\n",
               "    cpu->ax = result;\n    /* ADD DI,AX writes DI, not AX; this helper must only publish flags. */\n"),
    "flags_xor": ("    /* ADD DI,AX writes DI, not AX; this helper must only publish flags. */\n",
                  "    flags ^= 0x08D5u;\n    /* ADD DI,AX writes DI, not AX; this helper must only publish flags. */\n"),
    # flips only when the ADD operands differ in bit 0, so a non-final dispatch can be
    # wrong while the final word still matches; only the rolling hash sees it.
    "flags_pcodd": ("    /* ADD DI,AX writes DI, not AX; this helper must only publish flags. */\n",
                    "    if (((left ^ right) & 1u) != 0u) flags ^= 0x08D5u;\n    /* ADD DI,AX writes DI, not AX; this helper must only publish flags. */\n"),
    # review-d's exact boundary-reproduction: flip only OF, immediately before the
    # dispatch, on nonterminal opcodes. The old rolling hash cancelled modulo 65536.
    "of_nonterminal": ("        push16(ram, ram_size, cpu, body_cs);                           /* 31A4 push cs */",
                       "        if (((uint8_t)cpu->ax != 0u) && ((uint8_t)cpu->ax != 0xFFu))\n"
                       "            cpu->flags ^= 0x0800u;\n"
                       "        push16(ram, ram_size, cpu, body_cs);                           /* 31A4 push cs */"),
}


def build(directory, mutate=None):
    source_path = ROOT / "src/recovered/overlay_u1c_run_3171_cpu.c"
    text = source_path.read_text()
    name = "u1c_3171" + (("_" + mutate) if mutate else "")
    if mutate:
        old, new = MUTATIONS[mutate]
        check(old in text, f"mutation {mutate} site absent")
        text = text.replace(old, new, 1)
    path = directory / (name + ".c")
    path.write_text(text)
    so = directory / (name + ".so")
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2",
                    "-fPIC", "-shared", "-I", str(ROOT / "include"), str(path), "-o", str(so)],
                   check=True, capture_output=True, text=True)
    lib = ctypes.CDLL(str(so))
    dispatch_t = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.POINTER(ctypes.c_uint8),
                                  ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_void_p)
    lib.pr_u1c_3171_dispatch_double.argtypes = [ctypes.POINTER(ctypes.c_uint8),
                                                ctypes.c_size_t, ctypes.POINTER(Cpu),
                                                ctypes.c_void_p]
    lib.pr_u1c_3171_dispatch_double.restype = ctypes.c_int
    lib.pr_u1c_run_3171.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                                    ctypes.POINTER(Cpu), ctypes.c_uint16, dispatch_t,
                                    ctypes.c_void_p]
    lib.pr_u1c_run_3171.restype = ctypes.c_int
    return lib, dispatch_t


def setup_ram(script, entry, stop, stop2, latch, buf_off=0, buf_es=0x5000, es=0, ds_edge=False):
    """Build the initial flat RAM, shared by both sides."""
    ram = bytearray(SIZE)
    base = UNIT_CS << 4
    ram[base + ENTRY:base + END] = source_bytes
    dbase = (DATA_DS << 4) & 0xFFFFF
    # engine globals (DS-relative)
    def put_ds(off, data):
        at = (dbase + off) & 0xFFFFF
        ram[at:at + len(data)] = data
    put_ds(0x49ED, entry.to_bytes(2, "little"))
    put_ds(0x49DE, buf_off.to_bytes(2, "little"))
    put_ds(0x49E0, buf_es.to_bytes(2, "little"))
    put_ds(0x442E, bytes([stop]))
    put_ds(0x49FF, bytes([stop2]))
    put_ds(0x6F7F, bytes([latch]))
    put_ds(0x6F7E, b"\xAA")
    for i, opcode in enumerate(script):
        at = ((buf_es << 4) + ((buf_off + entry + i + 0x6700) & 0xFFFF)) & 0xFFFFF
        ram[at] = opcode
    # caller stack: [SP]=return IP, [SP+2]=caller CS, [SP+4]=entry argument
    sbase = (STACK_SS << 4) & 0xFFFFF
    for off, value in ((0, CALLER_IP), (2, CALLER_CS), (4, entry)):
        at = (sbase + STACK_SP + off) & 0xFFFFF
        ram[at:at + 2] = value.to_bytes(2, "little")
    return ram


def run_original(ram, lib):
    uc = Uc(UC_ARCH_X86, UC_MODE_16)
    uc.mem_map(0, SIZE)
    uc.mem_write(0, bytes(ram))
    uc.reg_write(UC_X86_REG_CS, UNIT_CS)
    uc.reg_write(UC_X86_REG_IP, ENTRY)
    uc.reg_write(UC_X86_REG_DS, DATA_DS)
    uc.reg_write(UC_X86_REG_ES, 0)
    uc.reg_write(UC_X86_REG_SS, STACK_SS)
    uc.reg_write(UC_X86_REG_SP, STACK_SP)
    uc.reg_write(UC_X86_REG_BP, 0xDEAD)
    uc.reg_write(UC_X86_REG_EFLAGS, 0x0202)
    dispatch_phys = ((UNIT_CS << 4) + DISPATCH_REL) & 0xFFFFF
    caller_phys = ((CALLER_CS << 4) + CALLER_IP) & 0xFFFFF
    counts = [0]
    overflow = [False]

    def on_code(uc_, address, size_, _user):
        if address == dispatch_phys:
            ss, sp = uc_.reg_read(UC_X86_REG_SS), uc_.reg_read(UC_X86_REG_SP)
            sp &= 0xFFFF
            sbase = (ss << 4) & 0xFFFFF
            ret_ip = int.from_bytes(uc_.mem_read((sbase + sp) & 0xFFFFF, 2), "little")
            ret_cs = int.from_bytes(uc_.mem_read((sbase + sp + 2) & 0xFFFFF, 2), "little")
            uc_.reg_write(UC_X86_REG_SP, (sp + 4) & 0xFFFF)
            buf = (ctypes.c_uint8 * SIZE).from_buffer_copy(uc_.mem_read(0, SIZE))
            cpu = Cpu()
            for name, reg in zip(NAMES, REGISTERS):
                setattr(cpu, name, uc_.reg_read(reg) & 0xFFFF)
            rc = lib.pr_u1c_3171_dispatch_double(buf, SIZE, ctypes.byref(cpu), None)
            uc_.mem_write(0, bytes(buf))
            if rc != 0:
                overflow[0] = True
                uc_.emu_stop()
                return
            counts[0] += 1
            uc_.reg_write(UC_X86_REG_IP, ret_ip)
            uc_.reg_write(UC_X86_REG_CS, ret_cs)
        elif address == caller_phys:
            uc_.emu_stop()

    uc.hook_add(UC_HOOK_CODE, on_code)
    uc.emu_start(((UNIT_CS << 4) + ENTRY) & 0xFFFFF, 0, count=200000)
    if overflow[0]:
        raise ValueError("observation overflow: more than 64 dispatches")
    out = bytearray(uc.mem_read(0, SIZE))
    cpu = {name: uc.reg_read(reg) & 0xFFFF for name, reg in zip(NAMES, REGISTERS)}
    return out, cpu, counts[0]


def run_c(ram, lib, dispatch_t):
    buf = (ctypes.c_uint8 * SIZE).from_buffer_copy(bytes(ram))
    cpu = Cpu()
    cpu.bp, cpu.sp, cpu.cs, cpu.ip = 0xDEAD, STACK_SP, UNIT_CS, ENTRY
    cpu.ds, cpu.es, cpu.ss, cpu.flags = DATA_DS, 0, STACK_SS, 0x0202
    counts = [0]

    @dispatch_t
    def callback(ptr, size, cpu_ptr, _context):
        counts[0] += 1
        return lib.pr_u1c_3171_dispatch_double(ptr, size, cpu_ptr, None)

    status = lib.pr_u1c_run_3171(buf, SIZE, ctypes.byref(cpu), CALLER_CS, callback, None)
    return bytearray(buf), {n: getattr(cpu, n) for n in NAMES}, counts[0], status


def _case(label, spec, kind):
    """External callers (t135 companion) get (label, spec); the kind stays internal."""
    CASE_KINDS[label] = kind
    return label, spec


def cases():
    """Every case is labelled by how it was chosen.

    adversarial: hand-picked to exercise a distinct instruction-semantics shape
                 (both stop paths, latch carry, long run, 16-bit PC wrap).
    regression:  the two exact cases codex-review-d found failing at 72c1625a;
                 they are kept verbatim and each has a dedicated mutant.
    sweep:       the small cross-product entry x stop x stop2 x buffer offset,
                 chosen from the instruction semantics, not from passing cases.
    """
    adversarial = [
        ("advance-then-stop", dict(script=[2, 0], entry=0, stop=0, stop2=0, latch=0)),
        ("stop-immediately", dict(script=[0], entry=0, stop=0, stop2=0, latch=0)),
        ("second-stop-byte", dict(script=[1, 0xFF], entry=0, stop=0, stop2=0, latch=0)),
        ("stop-byte-preset", dict(script=[2], entry=0, stop=1, stop2=0, latch=0)),
        ("second-stop-preset", dict(script=[2], entry=0, stop=0, stop2=1, latch=0)),
        ("nonzero-entry", dict(script=[3, 4, 0], entry=7, stop=0, stop2=0, latch=0)),
        ("latch-carried", dict(script=[9, 0], entry=0, stop=0, stop2=0, latch=0x5A)),
        ("many-steps", dict(script=[1] * 12 + [0], entry=0, stop=0, stop2=0, latch=0)),
        ("wrap-pc", dict(script=[5, 0], entry=0xFF00, stop=0, stop2=0, latch=0)),
    ]
    CASE_KINDS.clear()
    for label, spec in adversarial:
        yield _case(label, spec, "adversarial")
    # the two exact reviewer regressions (named, inputs unchanged)
    yield _case("regress-entry7-stop2-1",
                   dict(script=[3, 4, 0], entry=7, stop=0, stop2=1, latch=0, buf_off=0), "regression")
    yield _case("regress-bufoff-advance",
                   dict(script=[2, 0], entry=0, stop=0, stop2=0, latch=0, buf_off=0x0100), "regression")
    # the same two defects with script [0] (the reviewers' other fixture shape)
    yield _case("regress-entry7-stop2-1-zero",
                   dict(script=[0], entry=7, stop=0, stop2=1, latch=0, buf_off=0), "regression")
    yield _case("regress-bufoff-advance-zero",
                   dict(script=[0], entry=0, stop=0, stop2=0, latch=0, buf_off=0x0100), "regression")
    # a non-default buffer SEGMENT, so BUF_ES is a per-case axis, not a constant
    yield _case("es-6000",
                   dict(script=[1, 0], entry=0, stop=0, stop2=0, latch=0,
                buf_off=0x0100, buf_es=0x6000), "adversarial")
    # first dispatch has odd PC, last has even: a non-final flags change must be caught
    yield _case("flags-nonfinal",
                   dict(script=[1, 0], entry=1, stop=0, stop2=0, latch=0, buf_off=0), "adversarial")
    # review-d's boundary controls around the 7FFF/8000 buffer-offset boundary
    for off in (0x7FFE, 0x7FFF, 0x8000):
        yield _case(f"of-collision-{off:04x}",
                    dict(script=[1, 1, 0], entry=0, stop=0, stop2=0, latch=0, buf_off=off),
                    "adversarial")
    # systematic sweep: 4 x 2 x 2 x 4 = 64 cases
    for entry in (0, 1, 7, 0xFFFF):
        for stop in (0, 1):
            for stop2 in (0, 1):
                for buf_off in (0, 1, 0x0100, 0xFF00):
                    label = f"sweep-e{entry:04X}-s{stop}-s2{stop2}-b{buf_off:04X}"
                    yield _case(label, dict(script=[1, 0], entry=entry, stop=stop,
                                            stop2=stop2, latch=0, buf_off=buf_off), "sweep")


def ram_mismatch(want_ram, got_ram, external):
    """Compare RAM, or in external-trace mode RAM EXCEPT the 130-byte window.

    CAVEAT: external mode compares RAM EXCEPT the 130-byte instrumentation region
    (DS:0F00h..0F82h). It must not replace normal full-RAM row acceptance. The mask
    does not distinguish intended instrumentation writes from model writes; a wrong
    value written inside that window is invisible to external final-state comparison.
    """
    obs_low = (DATA_DS << 4) + 0x0F00
    obs_high = obs_low + 0x82
    if external:
        return (bytes(want_ram[:obs_low]) != bytes(got_ram[:obs_low])
                or bytes(want_ram[obs_high:]) != bytes(got_ram[obs_high:]))
    return bytes(want_ram) != bytes(got_ram)


def compare(label, ram, lib, dispatch_t):
    # The t135 companion supplies a dispatch factory (external trace) and owns the
    # boundary comparison; external mode is DIAGNOSTIC ONLY (see ram_mismatch's caveat).
    external = not isinstance(dispatch_t, type)
    want_ram, want_cpu, want_calls = run_original(ram, lib)
    got_ram, got_cpu, got_calls, status = run_c(ram, lib, dispatch_t)
    check(status == 0, f"{label}: C returned {status}")
    obs_low = (DATA_DS << 4) + 0x0F00
    obs_high = obs_low + 0x82
    same_ram = not ram_mismatch(want_ram, got_ram, external)
    if not same_ram:
        for i, (a, b) in enumerate(zip(want_ram, got_ram)):
            if external and obs_low <= i < obs_high:
                continue
            if a != b:
                raise ValueError(f"{label}: RAM differs at {i:#07x}: original {a:02x}, C {b:02x}")
    if got_cpu != want_cpu:
        diff = {k: (want_cpu[k], got_cpu[k]) for k in NAMES if want_cpu[k] != got_cpu[k]}
        raise ValueError(f"{label}: CPU differs: {diff}")
    check(got_calls == want_calls, f"{label}: dispatch count {want_calls} vs {got_calls}")
    return want_calls


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--compile-only", action="store_true",
                        help="build and bind the C model only; no media, no differential")
    args = parser.parse_args(argv)
    if args.compile_only:  # same compiler command and symbol binding as the audit; no media
        try:
            with tempfile.TemporaryDirectory(prefix="poolrad-u1c3171-") as temp:
                build(Path(temp))
        except (OSError, ValueError, subprocess.CalledProcessError) as error:
            print(f"CHECKER_BUILD_FAILED: {error}", file=sys.stderr)
            return 1
        print("CHECKER_BUILD_OK")
        return 0
    ovr, at, body = source()
    global source_bytes
    source_bytes = body
    print(f"OVL_001C_3171 span {ENTRY:04X}..{END:04X} file {at} sha256 {BODY_SHA[:16]}...")
    print("span digest is a pre-filter: it proves DIFFER, never SAME; the differential below is the evidence.")
    with tempfile.TemporaryDirectory() as tmp:
        directory = Path(tmp)
        lib, dispatch_t = build(directory)
        mutants = {name: build(directory, name) for name in MUTATIONS}
        want = list(cases())
        total = 0
        for label, spec in want:
            kind = CASE_KINDS.get(label, "?")
            ram = setup_ram(**spec)
            total += compare(label, ram, lib, dispatch_t)
            print(f"  AGREE [{kind}] {label}")
        # each mutant must die, and the two regression cases must be the killers of
        # their own defect (not merely one of many incidental failures).
        required = {"bias": None, "ax_entry": "regress-entry7-stop2-1",
                    "add_ax": "regress-bufoff-advance", "flags_xor": "advance-then-stop",
                    "flags_pcodd": "flags-nonfinal",
                    "of_nonterminal": "of-collision-7fff"}
        for name, (mlib, mdispatch) in mutants.items():
            killed = []
            for label, spec in want:
                ram = setup_ram(**spec)
                try:
                    compare(label, ram, mlib, mdispatch)
                except ValueError:
                    killed.append(label)
            check(killed, f"mutant {name} survived every case")
            if required[name] is not None:
                check(required[name] in killed,
                      f"mutant {name} was not killed by its regression case {required[name]}")
            suffix = f" (incl. {required[name]})" if required[name] else ""
            print(f"  mutant {name}: killed on {len(killed)}/{len(want)} cases{suffix}")
        kinds = {kind: sum(1 for l, _s in want if CASE_KINDS.get(l) == kind) for kind in
                 ("adversarial", "regression", "sweep")}
        print(f"PASS: {len(want)} cases ({kinds['adversarial']} adversarial, "
              f"{kinds['regression']} regression, {kinds['sweep']} sweep), "
              f"{total} dispatches, full 1 MiB RAM + 14 CPU words")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (ImportError, OSError, ValueError, subprocess.CalledProcessError) as error:
        sys.exit(f"audit failed: {error}")
