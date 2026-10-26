#!/usr/bin/env python3
"""Original-vs-C differential for candidate-scout ECL handlers OVL_001C_0292 and OVL_001C_0EBC.

Each body is a raw-CPU C model generated from the original bytes
(tools/overlay/emit_u1c_save_0292.py, emit_u1c_save_table_0ebc.py). The original bytes
run under Unicorn 16-bit; the compiled C runs from the same state. The far calls to the
unit-0x25 runtime are routed to the SAME candidate helper doubles in
src/recovered/candidate_ecl_doubles.c on both sides (on the original side through a
Unicorn code hook that lets the doubles perform the callee's RETF).

Compared per case: all 1,048,576 RAM bytes, all 14 CPU words unmasked, dispatch count,
and the final CS:IP:SP. A mutated body (branch constant or add->sub) must differ on at
least one case, so the differential is shown able to fail.

Verification ceiling: the helper doubles stand in for the real unit-0x25 routines; this
checks the handler bodies' control flow and argument marshalling up to that boundary.
Proposals only: no credit, no accounting file touched. Media stays under resources/.
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
CHECKER_ROWS = ("OVL_001C_0292", "OVL_001C_0EBC")
OVR = ROOT / "resources/DOS/POOLRAD/GAME.OVR"
EXE = ROOT / "resources/DOS/POOLRAD/START.EXE"
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
EXE_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
SIZE = 1 << 20
UNIT_CS, STACK_SS, STACK_SP, DATA_DS = 0x2000, 0x3000, 0x1000, 0x4000
CALLER_CS, CALLER_IP = 0x1000, 0x1234
OPERAND_BASE = 0x1000
STRING_BUF = 0x70C8
REGISTERS = (UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX, UC_X86_REG_DX,
             UC_X86_REG_SI, UC_X86_REG_DI, UC_X86_REG_BP, UC_X86_REG_SP,
             UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_DS, UC_X86_REG_ES,
             UC_X86_REG_SS, UC_X86_REG_EFLAGS)
NAMES = ("ax", "bx", "cx", "dx", "si", "di", "bp", "sp", "cs", "ip",
         "ds", "es", "ss", "flags")

BODIES = {
    "0292": dict(
        stem="overlay_u1c_save_0292_cpu", fn="pr_u1c_save_0292",
        start=0x0292, end=0x02E0, calls=5,
        doubles={0x2A: "pr_candidate_load_operands",
                 0x4D: "pr_candidate_operand_word",
                 0x25: "pr_candidate_slot_value",
                 0x52: "pr_candidate_store_word",
                 0x5C: "pr_candidate_store_string"},
        mutate=("arith(c,rd8(b,c->ds,(uint16_t)(0x7006u)),0x80u,0u,8u,1u)",
                "arith(c,rd8(b,c->ds,(uint16_t)(0x7006u)),0x81u,0u,8u,1u)")),
    "0ebc": dict(
        stem="overlay_u1c_save_table_0ebc_cpu", fn="pr_u1c_save_table_0ebc",
        start=0x0EBC, end=0x0F08, calls=5,
        doubles={0x2A: "pr_candidate_load_operands",
                 0x4D: "pr_candidate_operand_word",
                 0x25: "pr_candidate_slot_value",
                 0x52: "pr_candidate_store_word"},
        mutate=("arith(c,c->ax,rd16(b,c->ss,(uint16_t)(c->bp+0xFFFEu)),0u,16u,0u)",
                "arith(c,c->ax,rd16(b,c->ss,(uint16_t)(c->bp+0xFFFEu)),0u,16u,1u)")),
}


class Cpu(ctypes.Structure):
    _fields_ = [(n, ctypes.c_uint16) for n in NAMES]


class Ctx(ctypes.Structure):
    _fields_ = [("operand_base", ctypes.c_uint16)]


def check(condition, message):
    if not condition:
        raise ValueError(message)


def media():
    exe, ovr = EXE.read_bytes(), OVR.read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, "START.EXE provenance")
    check(len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA, "GAME.OVR provenance")
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    from overlay_inventory import find_units, parse_mz
    units = {u["stub_para"]: u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)}
    return ovr, units


def body_bytes(ovr, units, cfg):
    unit = units[0x1C]
    at = unit["ovr_offset"] + cfg["start"]
    body = ovr[at:at + (cfg["end"] - cfg["start"])]
    decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, cfg["start"]))
    check(decoded and decoded[-1].mnemonic == "retf", "body does not end on RETF")
    far = [i for i in decoded if i.mnemonic == "lcall"]
    check(len(far) == cfg["calls"], f"{cfg['fn']}: far-call count changed")
    targets = {}
    for ins in far:
        seg, off = [int(x, 16) for x in ins.op_str.split(",")]
        targets[(seg << 4) + off] = cfg["doubles"][off]
    check(len(targets) == len(cfg["doubles"]), f"{cfg['fn']}: call target set changed")
    return at, body, targets


def build(directory, cfg, mutate=False):
    source = (ROOT / "src/recovered" / (cfg["stem"] + ".c")).read_text()
    if mutate:
        old, new = cfg["mutate"]
        check(old in source, f"{cfg['fn']}: mutation site absent")
        source = source.replace(old, new)
        check(new in source, f"{cfg['fn']}: mutation did not apply")
    body_c = directory / (cfg["stem"] + ("_mutant" if mutate else "") + ".c")
    body_c.write_text(source)
    doubles = ROOT / "src/recovered/candidate_ecl_doubles.c"
    so = directory / (cfg["stem"] + ("_mutant" if mutate else "") + ".so")
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2",
                    "-fPIC", "-shared", "-I", str(ROOT / "include"), str(body_c),
                    str(doubles), "-o", str(so)], check=True, capture_output=True, text=True)
    lib = ctypes.CDLL(str(so))
    fn = getattr(lib, cfg["fn"] + "_cpu")
    fn.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                   ctypes.POINTER(Cpu), ctypes.c_uint16, ctypes.c_void_p]
    fn.restype = ctypes.c_int
    for name in cfg["doubles"].values():
        f = getattr(lib, name)
        f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.POINTER(Cpu),
                      ctypes.POINTER(Ctx)]
        f.restype = ctypes.c_int
    return lib, fn


def setup(operands, string=b""):
    ram = bytearray(SIZE)
    base = UNIT_CS << 4
    ram[base + cfg_start:base + cfg_end] = source_body
    dbase = (DATA_DS << 4) & 0xFFFFF
    for i, (tag, low, high) in enumerate(operands):
        at = (dbase + OPERAND_BASE + i * 3) & 0xFFFFF
        ram[at:at + 3] = bytes([tag, low, high])
    if string:
        at = (dbase + STRING_BUF) & 0xFFFFF
        ram[at] = len(string)
        ram[at + 1:at + 1 + len(string)] = string
    sbase = (STACK_SS << 4) & 0xFFFFF
    for off, value in ((0, CALLER_IP), (2, CALLER_CS)):
        at = (sbase + STACK_SP + off) & 0xFFFFF
        ram[at:at + 2] = value.to_bytes(2, "little")
    return ram


def run_original(ram, lib, targets):
    uc = Uc(UC_ARCH_X86, UC_MODE_16)
    uc.mem_map(0, SIZE)
    uc.mem_write(0, bytes(ram))
    uc.reg_write(UC_X86_REG_CS, UNIT_CS)
    uc.reg_write(UC_X86_REG_IP, cfg_start)
    uc.reg_write(UC_X86_REG_DS, DATA_DS)
    uc.reg_write(UC_X86_REG_ES, 0)
    uc.reg_write(UC_X86_REG_SS, STACK_SS)
    uc.reg_write(UC_X86_REG_SP, STACK_SP)
    uc.reg_write(UC_X86_REG_BP, 0xDEAD)
    for reg in REGISTERS[0:6]:
        uc.reg_write(reg, 0x1111)
    uc.reg_write(UC_X86_REG_EFLAGS, 0x0202)
    caller_phys = ((CALLER_CS << 4) + CALLER_IP) & 0xFFFFF
    counts = [0]

    def on_code(uc_, address, size_, _user):
        if address == caller_phys:
            uc_.emu_stop()
            return
        name = targets.get(address)
        if name is None:
            return
        buf = (ctypes.c_uint8 * SIZE).from_buffer_copy(uc_.mem_read(0, SIZE))
        cpu = Cpu()
        for n, reg in zip(NAMES, REGISTERS):
            setattr(cpu, n, uc_.reg_read(reg) & 0xFFFF)
        ctx = Ctx(OPERAND_BASE)
        getattr(lib, name)(buf, ctypes.byref(cpu), ctypes.byref(ctx))
        uc_.mem_write(0, bytes(buf))
        for n, reg in zip(NAMES, REGISTERS):
            uc_.reg_write(reg, getattr(cpu, n))
        counts[0] += 1

    uc.hook_add(UC_HOOK_CODE, on_code)
    uc.emu_start(((UNIT_CS << 4) + cfg_start) & 0xFFFFF, 0, count=100000)
    out = bytearray(uc.mem_read(0, SIZE))
    cpu = {n: uc.reg_read(reg) & 0xFFFF for n, reg in zip(NAMES, REGISTERS)}
    return out, cpu, counts[0]


def run_c(ram, lib, fn):
    buf = (ctypes.c_uint8 * SIZE).from_buffer_copy(bytes(ram))
    cpu = Cpu()
    cpu.bp, cpu.sp, cpu.cs, cpu.ip = 0xDEAD, STACK_SP, UNIT_CS, cfg_start
    cpu.ds, cpu.es, cpu.ss, cpu.flags = DATA_DS, 0, STACK_SS, 0x0202
    cpu.ax = cpu.bx = cpu.cx = cpu.dx = cpu.si = cpu.di = 0x1111
    ctx = Ctx(OPERAND_BASE)
    status = fn(buf, SIZE, ctypes.byref(cpu), CALLER_CS, ctypes.byref(ctx))
    return bytearray(buf), {n: getattr(cpu, n) for n in NAMES}, status


def cases(bod):
    if bod == "0292":
        yield "save-word", dict(operands=[(0x00, 0x34, 0x12), (0x00, 0x00, 0x20)])
        yield "save-zero", dict(operands=[(0x00, 0x00, 0x00), (0x00, 0x00, 0x20)])
        yield "save-high-hi", dict(operands=[(0x00, 0x77, 0x00), (0x00, 0xF0, 0x3F)])
        yield "save-string", dict(operands=[(0x80, 0x34, 0x12), (0x00, 0x00, 0x20)],
                                  string=b"hello")
        yield "save-string-empty", dict(operands=[(0x80, 0x00, 0x00), (0x00, 0x00, 0x20)],
                                        string=b"")
        yield "save-string-long", dict(operands=[(0xFF, 0x01, 0x02), (0x00, 0x08, 0x30)],
                                       string=b"abcdefgh")
    else:
        yield "table-basic", dict(operands=[(0x00, 0x34, 0x12), (0x00, 0x00, 0x20),
                                            (0x00, 0x08, 0x00)])
        yield "table-zero-index", dict(operands=[(0x00, 0x34, 0x12), (0x00, 0x00, 0x20),
                                                 (0x00, 0x00, 0x00)])
        yield "table-wrap", dict(operands=[(0x00, 0x01, 0x00), (0x00, 0xF0, 0xFF),
                                           (0x00, 0x20, 0x00)])
        yield "table-high-value", dict(operands=[(0x00, 0xCD, 0xAB), (0x00, 0x00, 0x30),
                                                 (0x00, 0x10, 0x00)])
        yield "table-index-ff", dict(operands=[(0x00, 0x02, 0x00), (0x00, 0x00, 0x40),
                                               (0x00, 0xFF, 0x00)])


def compare(label, bod, ram, lib, fn, targets, counts):
    want_ram, want_cpu, want_calls = run_original(ram, lib, targets)
    got_ram, got_cpu, status = run_c(ram, lib, fn)
    check(status == 0 or status == 1, f"{label}: C returned {status}")
    if bytes(got_ram) != bytes(want_ram):
        for i, (a, b) in enumerate(zip(want_ram, got_ram)):
            if a != b:
                raise ValueError(f"{label}: RAM differs at {i:#07x}: original {a:02x}, C {b:02x}")
    if got_cpu != want_cpu:
        diff = {k: (want_cpu[k], got_cpu[k]) for k in NAMES if want_cpu[k] != got_cpu[k]}
        raise ValueError(f"{label}: CPU differs: {diff}")
    counts[0] += want_calls
    return want_calls


def audit(bod, ovr, units):
    global cfg_start, cfg_end, source_body
    cfg = BODIES[bod]
    at, source_body, targets = body_bytes(ovr, units, cfg)
    cfg_start, cfg_end = cfg["start"], cfg["end"]
    print(f"OVL_001C_{cfg['start']:04X} file {at} bytes {len(source_body)} "
          f"sha256 {sha256(source_body).hexdigest()[:16]}... calls {len(targets)}")
    with tempfile.TemporaryDirectory() as tmp:
        directory = Path(tmp)
        lib, fn = build(directory, cfg)
        mutant, mutant_fn = build(directory, cfg, mutate=True)
        want = list(cases(bod))
        counts = [0]
        for label, spec in want:
            ram = setup(**spec)
            compare(label, bod, ram, lib, fn, targets, counts)
            print(f"  AGREE {label}")
        killed = 0
        for label, spec in want:
            ram = setup(**spec)
            try:
                compare(label, bod, ram, mutant, mutant_fn, targets, [0])
            except ValueError:
                killed += 1
        check(killed >= 1, "the mutated body did not differ on any case")
        print(f"mutant killed on {killed}/{len(want)} cases")
        print(f"PASS: {len(want)} cases, {counts[0]} dispatches, "
              "full 1 MiB RAM + 14 CPU words")
        row = "OVL_001C_0292" if bod == "0292" else "OVL_001C_0EBC"
        print(f"ROW_EXECUTED {row} PASS")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("body", nargs="?", choices=sorted(BODIES))
    parser.add_argument("--compile-only", action="store_true",
                        help="build and bind both C models only; no media, no differential")
    args = parser.parse_args(argv)
    if args.compile_only:  # same compiler command and symbol binding as the audit; no media
        try:
            with tempfile.TemporaryDirectory(prefix="poolrad-u1c-save-") as temp:
                for cfg in BODIES.values():
                    build(Path(temp), cfg)
        except (OSError, ValueError, subprocess.CalledProcessError) as error:
            print(f"CHECKER_BUILD_FAILED: {error}", file=sys.stderr)
            return 1
        print("CHECKER_BUILD_OK")
        return 0
    try:
        ovr, units = media()
    except (ImportError, OSError, ValueError, subprocess.CalledProcessError) as error:
        sys.exit(f"audit failed: {error}")
    if not args.body:
        # All-bodies differential: the checker registers two rows, so one command must
        # execute BOTH and fail if either fails. No row can be AGREE unless its own
        # differential ran and passed.
        failed = 0
        for bod in sorted(BODIES):
            try:
                audit(bod, ovr, units)
            except (ImportError, OSError, ValueError, subprocess.CalledProcessError) as error:
                print(f"{bod}: audit failed: {error}", file=sys.stderr)
                failed = 1
        return failed
    try:
        audit(args.body, ovr, units)
    except (ImportError, OSError, ValueError, subprocess.CalledProcessError) as error:
        sys.exit(f"audit failed: {error}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
