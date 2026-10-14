#!/usr/bin/env python3
"""Compare bounded C instruction effects with original bytes and native RAM."""

import ctypes as ct
from hashlib import sha256
from pathlib import Path
import sys

import unicorn
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16
from unicorn import x86_const as x86

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests" / "recovered"))
import check_slot_a_u6f_196a_resident_native as native  # noqa: E402

NAMES = "cs ip ds es fs gs ss sp bp ax bx cx dx si di flags".split()
REGS = [getattr(x86, "UC_X86_REG_" + ("EFLAGS" if n == "flags" else n.upper()))
        for n in NAMES]
RAM_SIZE = 655360


class CPU(ct.Structure):
    _fields_ = [(name, ct.c_uint16) for name in
                "ax bx cx dx si di bp sp cs ds es ss fs gs ip flags".split()]


class State(ct.Structure):
    _fields_ = [("cpu", CPU), ("ram", ct.POINTER(ct.c_uint8)), ("ram_size", ct.c_size_t)]


class Report(ct.Structure):
    _fields_ = [("dependency_valid", ct.c_uint), ("caller", ct.c_uint16),
                ("call_pc", ct.c_uint16), ("callee_segment", ct.c_uint16),
                ("callee_pc", ct.c_uint16), ("return_pc", ct.c_uint16),
                ("caller_is_overlay", ct.c_uint)]


def cpu_values(cpu):
    return [getattr(cpu, name) for name in NAMES]


def original_code(root, ram):
    native.check(root)
    start = (root / "POOLRAD/START.EXE").read_bytes()
    game = (root / "POOLRAD/GAME.OVR").read_bytes()
    for data, digest in (
        (game[0x1DFE6 + 0x196F:0x1DFE6 + 0x1974],
         "fe81385603a3fc55df47de14941cf1bdbf72d0bd7c936edaecb4ceda0c23d52e"),
        (ram[(0x2A8E << 4) + 0x196F:(0x2A8E << 4) + 0x1974],
         "78cec5fcdde872ab5e4c4add6d5debd1f4cf3d3430f3e12b1a051c4ce061d147"),
    ):
        if sha256(data).hexdigest() != digest:
            raise AssertionError("next original/loaded call identity mismatch")
    for offset, file_start, size, digest in (
        (0x0B25, 0x0B79B, 36, "009955722e8f73d0a0f7076aa418d141f642c5a729b2a58dfa7a7a3922989118"),
        (0x0B57, 0x0B7CD, 65, "18383182a06f0521f0873061e61004f0b1c6bec8f3b30b1158ec7ad7822940fa"),
        (0x0C10, 0x0B886, 43, "11e2860118cb471f9f1ecb1d0c44c060e0fbb83862f69c1cf39b5e0484298a58"),
    ):
        source = start[file_start:file_start + size]
        loaded = ram[(0x131C << 4) + offset:(0x131C << 4) + offset + size]
        if sha256(source).hexdigest() != digest or loaded != source:
            raise AssertionError("original/loaded SYSTEM body identity mismatch")


def compare_cpu(actual, expected, context):
    if actual != expected:
        fields = [name for name, a, b in zip(NAMES, actual, expected) if a != b]
        raise AssertionError(f"{context}: CPU mismatch in {','.join(fields)}")


def differential(lib, label, image, values, expected_status, checkpoints=None):
    memory = ct.create_string_buffer(image, len(image))
    cpu = CPU(**dict(zip(NAMES, values)))
    state = State(cpu, ct.cast(memory, ct.POINTER(ct.c_uint8)), len(image))
    report = Report()
    uc = Uc(UC_ARCH_X86, UC_MODE_16)
    uc.mem_map(0, 0x100000)
    uc.mem_write(0, image)
    for register, value in zip(REGS, values):
        uc.reg_write(register, value)
    seen = set()
    controlled = False
    for steps in range(20000):
        pc = (state.cpu.cs, state.cpu.ip)
        before_cpu, before_ram = cpu_values(state.cpu), memory.raw
        status = lib.pr_slot_a_string_0356_step(ct.byref(state), ct.byref(report))
        if status:
            if status != expected_status:
                raise AssertionError(f"{label}: unexpected status {status} at {pc[0]:04X}:{pc[1]:04X}")
            compare_cpu(cpu_values(state.cpu), before_cpu, "blocked state")
            if memory.raw != before_ram:
                raise AssertionError("blocked instruction mutated RAM")
            if not report.dependency_valid:
                raise AssertionError("missing typed call dependency")
            if expected_status == 1:
                assert (report.caller, report.call_pc, report.callee_segment,
                        report.callee_pc, report.return_pc, report.caller_is_overlay) == (
                            0x6F, 0x196F, 0x0802, 0x0425, 0x1974, 1)
            elif expected_status == 2:
                assert (report.caller, report.call_pc, report.callee_segment,
                        report.callee_pc, report.return_pc, report.caller_is_overlay) == (
                            0x0802, 0x03FC, 0x0AF8, 0x0CBC, 0x0401, 0)
            if checkpoints and seen != set(checkpoints):
                raise AssertionError("not all native checkpoints were reached")
            print(f"{label}: {steps} C instruction steps, CPU/full-RAM Unicorn differential passed; blocker={status}")
            return steps, len(seen)
        for repetition in range(65537):
            uc.emu_start((uc.reg_read(REGS[0]) << 4) + uc.reg_read(REGS[1]),
                         0x100000, count=1)
            if (uc.reg_read(REGS[0]), uc.reg_read(REGS[1])) != pc:
                break
        else:
            raise AssertionError("Unicorn instruction did not finish")
        context = f"{label} after {pc[0]:04X}:{pc[1]:04X}"
        compare_cpu(cpu_values(state.cpu), [uc.reg_read(r) & 0xFFFF for r in REGS], context)
        if memory.raw != bytes(uc.mem_read(0, len(image))):
            raise AssertionError(context + ": full RAM differs")
        if (state.cpu.cs, state.cpu.ip) == (0x1026, 0x041A):
            controlled = True
        if checkpoints:
            key = (state.cpu.cs, state.cpu.ip, controlled)
            for checkpoint, (target, expected_cpu, expected_ram) in checkpoints.items():
                if checkpoint not in seen and key == target:
                    compare_cpu(cpu_values(state.cpu), expected_cpu, checkpoint)
                    if memory.raw != expected_ram:
                        raise AssertionError(checkpoint + ": native full RAM differs")
                    seen.add(checkpoint)
    raise AssertionError("C instruction bound exhausted")


def main():
    if len(sys.argv) != 3:
        raise SystemExit("usage: audit_slot_a_string_0356_abi.py SHARED_C_LIBRARY IGNORED_NATIVE_ROOT")
    if unicorn.__version__ != "2.1.4":
        raise SystemExit("this differential is pinned to Unicorn 2.1.4")
    if not __debug__:
        raise SystemExit("native hash audit requires non-optimized Python assertions")
    library, root = (Path(arg).resolve(strict=True) for arg in sys.argv[1:])
    private = (ROOT / "resources").resolve()
    if not library.is_relative_to(private) or not root.is_relative_to(private):
        raise SystemExit("library and native inputs must stay in this clone's ignored resources/")
    lib = ct.CDLL(str(library))
    lib.pr_slot_a_string_0356_step.argtypes = [ct.POINTER(State), ct.POINTER(Report)]
    lib.pr_slot_a_string_0356_step.restype = ct.c_int
    image = (root / "ram-196a.bin").read_bytes()
    original_code(root, image)
    live_cpu = native.transcript(root, "ev-resident-live.txt", native.LIVE_EV_HASH,
                                 list(native.LIVE_HASHES))
    control_cpu = native.transcript(root, "ev-resident-controlled.txt", native.CONTROL_EV_HASH,
                                    list(native.CONTROL_HASHES) + ["196f"])
    checkpoints = {}
    for label in native.LIVE_HASHES:
        if label.startswith("196a"):
            continue
        values = live_cpu[label]
        checkpoints["live-" + label] = (
            (values[0], values[1], label == "196f"), values,
            (root / f"ram-{label}.bin").read_bytes())
    for label in native.CONTROL_HASHES:
        values = control_cpu[label]
        checkpoints["controlled-" + label] = (
            (values[0], values[1], True), values,
            (root / f"ram-controlled-{label}.bin").read_bytes())
    steps, seen = differential(lib, "native saved-A", image, live_cpu["196a"], 1, checkpoints)
    print(f"native: {seen} CPU/full-RAM checkpoints match, including return +196F")

    for name, source, needle, status in (
        ("synthetic-no-match", b"ABC", b"Z", 1),
        ("synthetic-long-needle", b"A", b"XY", 1),
        ("synthetic-empty-source", b"", b"Z", 1),
        ("synthetic-composed-delete", b"ABA", b"A", 1),
        ("synthetic-empty-needle", b"ABC", b"", 1),
    ):
        synthetic = bytearray(image)
        values = live_cpu["196a"].copy()
        values[7] = 0x8000  # synthetic caller SP; not a native-route input
        values[8] = 0x8100
        values[15] |= 0x0400  # SYSTEM CLD must clear incoming DF
        ss = values[6] << 4
        def put_word(offset, value):
            synthetic[ss + offset:ss + offset + 2] = value.to_bytes(2, "little")
        for offset, address in ((0x8000, 0x5100), (0x8004, 0x5000), (0x8008, 0x5200)):
            put_word(offset, address)
            put_word(offset + 2, values[6])
        synthetic[ss + 0x5000:ss + 0x5001 + len(source)] = bytes([len(source)]) + source
        synthetic[ss + 0x5100:ss + 0x5101 + len(needle)] = bytes([len(needle)]) + needle
        differential(lib, name, bytes(synthetic), values, status)
    print(f"original-byte/Unicorn/C differential complete; native instruction steps={steps}")


if __name__ == "__main__":
    main()
