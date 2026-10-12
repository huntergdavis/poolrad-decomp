#!/usr/bin/env python3
"""Check u5E +2879 original bytes against portable C on synthetic RAM."""

import argparse
import ctypes
from hashlib import sha256
from pathlib import Path
import random
import subprocess
import tempfile

from capstone import CS_ARCH_X86, CS_MODE_16, Cs
from overlay_inventory import find_units, parse_mz


RAM_SIZE = 1 << 20
EXE_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
BODY_SHA = "a474c72a13a1d9fde8a5b64f7d57ca2393afdd8a5e1861038dbf725454d3d4ec"
FILE_START = 0x15074
ENTRY = 0x2879
END = 0x2908


class Cpu(ctypes.Structure):
    _fields_ = [(name, ctypes.c_uint16) for name in
                "ax bx cx dx si di bp sp cs ip ds es ss flags".split()]


def check(condition, message):
    if not condition:
        raise ValueError(message)


def physical(segment, offset):
    return ((segment << 4) + offset) & (RAM_SIZE - 1)


def put16(memory, segment, offset, value):
    at = physical(segment, offset)
    memory[at] = value & 0xFF
    memory[(at + 1) & (RAM_SIZE - 1)] = value >> 8


def source_audit(exe_path, ovr_path):
    exe, ovr = exe_path.read_bytes(), ovr_path.read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA,
          "unsupported START.EXE")
    check(len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA,
          "unsupported GAME.OVR")
    body = ovr[FILE_START:FILE_START + END - ENTRY]
    check(len(body) == 143 and sha256(body).hexdigest() == BODY_SHA,
          "u5E +2879 body changed")
    decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(len(decoded) == 50 and decoded[-1].address == 0x2907 and
          decoded[-1].mnemonic == "retf" and
          decoded[-1].address + decoded[-1].size == END,
          "u5E extent or decode changed")
    starts = {item.address for item in decoded}
    for item in decoded:
        check("call" not in item.mnemonic, "u5E body gained a call")
        if item.mnemonic.startswith("j"):
            check(int(item.op_str, 16) in starts,
                  f"u5E branch leaves span at {item.address:#x}")

    units = {unit["stub_para"]: unit for unit in
             find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)}
    target = units[0x5E]
    check(target["memory_segment"] == 0x68 and
          target["entries"][19] == ENTRY and
          target["ovr_offset"] + ENTRY == FILE_START,
          "u5E entry descriptor changed")
    # Check actual relocation sites as well as the far-call instructions.
    for unit, file_base, site in ((0x2F, 0x9807, 0x0895),
                                  (0x33, 0xC5D1, 0x1F7F)):
        source = units[unit]
        check(source["ovr_offset"] == file_base and
              site + 3 in source["fixups"],
              f"u{unit:02X} caller lacks its segment fixup")
        at = source["ovr_offset"] + site
        call = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(
            ovr[at:at + 5], site))
        check(len(call) == 1 and call[0].mnemonic == "lcall" and
              call[0].op_str == "0x68, 0x7f", f"u{unit:02X} caller changed")
    print(f"u5E +2879..+2907: 143 bytes, 50 instructions, "
          f"SHA-256 {BODY_SHA}; zero calls, internal branches")
    print("two far callers: u2F +0895 and u33 +1F7F -> 0068:007F")
    return body


def build_c(root, directory):
    library_path = directory / "u5e_2879.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                    "-Werror", "-shared", "-fPIC", "-I", str(root / "include"),
                    str(root / "src/recovered/overlay_u5e_party_value_2879.c"),
                    "-o", str(library_path)], check=True, capture_output=True,
                   text=True)
    library = ctypes.CDLL(str(library_path))
    library.pr_u5e_party_value_2879.argtypes = [
        ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
        ctypes.POINTER(Cpu), ctypes.c_size_t]
    library.pr_u5e_party_value_2879.restype = ctypes.c_int
    return library


def compare_case(label, memory, cpu, body, library, max_nodes):
    from unicorn import Uc, UC_ARCH_X86, UC_HOOK_CODE, UC_MODE_16
    from unicorn.x86_const import (
        UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX, UC_X86_REG_DX,
        UC_X86_REG_SI, UC_X86_REG_DI, UC_X86_REG_BP, UC_X86_REG_SP,
        UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_DS, UC_X86_REG_ES,
        UC_X86_REG_SS, UC_X86_REG_EFLAGS)

    registers = (UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX,
                 UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI,
                 UC_X86_REG_BP, UC_X86_REG_SP, UC_X86_REG_CS,
                 UC_X86_REG_IP, UC_X86_REG_DS, UC_X86_REG_ES,
                 UC_X86_REG_SS, UC_X86_REG_EFLAGS)
    start = physical(cpu.cs, ENTRY)
    memory[start:start + len(body)] = body
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    # The FFFF:0100 read-only record case crosses physical 1 MiB. Mirror
    # the synthetic lower RAM there to model A20-off reads; this routine
    # writes only its low SS frame and DS result in that case.
    original.mem_map(0, 2 * RAM_SIZE)
    original.mem_write(0, bytes(memory))
    original.mem_write(RAM_SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, registers):
        original.reg_write(register, getattr(cpu, name))
    returned = [False]

    def on_instruction(emulator, address, size, user):
        del size, user
        if not start <= address < start + len(body):
            returned[0] = True
            emulator.emu_stop()

    original.hook_add(UC_HOOK_CODE, on_instruction)
    original.emu_start(start, RAM_SIZE, count=100000)
    check(returned[0], f"original did not return: {label}")
    expected = bytes(original.mem_read(0, RAM_SIZE))
    c_ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
    c_cpu = Cpu.from_buffer_copy(cpu)
    status = library.pr_u5e_party_value_2879(
        c_ram, RAM_SIZE, ctypes.byref(c_cpu), max_nodes)
    check(status == 0, f"C status {status}: {label}")
    actual = bytes(c_ram)
    if actual != expected:
        offsets = [at for at, (a, b) in enumerate(zip(actual, expected))
                   if a != b]
        raise ValueError(f"{label}: RAM differs at {offsets[:12]}")
    for (name, _), register in zip(Cpu._fields_, registers):
        dos = original.reg_read(register) & 0xFFFF
        got = getattr(c_cpu, name)
        check(dos == got, f"{label}: {name} DOS={dos:04x} C={got:04x}")


def synthetic_case(body, library, label, nodes, *, seed=0,
                   ds=0x1000, ss=0x2000, sp=0x8000,
                   record_segment=0x4000, first_offset=0x0100):
    rng = random.Random(seed)
    memory = bytearray(RAM_SIZE)
    cpu = Cpu(0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE,
              0x7777, sp, 0x3000, ENTRY, ds, 0x1111, ss, 0x7202)
    put16(memory, ss, sp, 0x7000)
    put16(memory, ss, (sp + 2) & 0xFFFF, cpu.cs)
    offsets = [first_offset + 0x200 * index for index in range(len(nodes))]
    put16(memory, ds, 0x5D96, offsets[0] if offsets else 0)
    put16(memory, ds, 0x5D98, record_segment if offsets else 0)
    for index, (eligible, gate, numerator, denominator) in enumerate(nodes):
        offset = offsets[index]
        next_offset = offsets[index + 1] if index + 1 < len(nodes) else 0
        next_segment = record_segment if next_offset else 0
        put16(memory, record_segment, (offset + 0x104) & 0xFFFF, next_offset)
        put16(memory, record_segment, (offset + 0x106) & 0xFFFF, next_segment)
        for field, value in ((0x10E, eligible), (0x10D, gate),
                             (0x11B, numerator), (0x32, denominator)):
            memory[physical(record_segment, (offset + field) & 0xFFFF)] = value
    compare_case(label, memory, cpu, body, library, max(1, len(nodes)))


def synthetic_audit(body, library):
    cases = (
        ("empty", []),
        ("ineligible", [(0, 1, 99, 99)]),
        ("zero-gate", [(1, 0, 99, 5)]),
        ("eligible", [(1, 1, 3, 4)]),
        ("mixed", [(0, 1, 99, 99), (1, 1, 3, 4)]),
        ("mul-low-word", [(1, 1, 255, 1)] * 20),
    )
    for label, nodes in cases:
        synthetic_case(body, library, label, nodes)
    synthetic_case(body, library, "return-frame-alias", [(1, 1, 3, 4)],
                   ss=0x0EE3, sp=0x7FFF)
    synthetic_case(body, library, "head-SS-local-alias", [(1, 1, 3, 4)],
                   ds=0x2000, ss=0x2000, sp=0x5DA0)
    synthetic_case(body, library, "node-11B-SS-sum-alias", [(1, 1, 7, 3)],
                   record_segment=0x2000, first_offset=0x7EDF)
    synthetic_case(body, library, "node-32-SS-sum-alias", [(1, 1, 7, 3)],
                   record_segment=0x2000, first_offset=0x7FCA)
    synthetic_case(body, library, "record-offset-wrap", [(1, 1, 7, 3)],
                   first_offset=0xFFF0)
    synthetic_case(body, library, "A20-wrap", [(1, 1, 7, 3)],
                   record_segment=0xFFFF)
    synthetic_case(body, library, "SS-offset-wrap", [(1, 1, 7, 3)],
                   sp=0x0001)
    rng = random.Random(0x5E2879)
    for index in range(100):
        nodes = [(rng.choice((0, 1, 2)), rng.randrange(2),
                  rng.randrange(256), rng.randrange(256))
                 for _ in range(rng.randrange(6))]
        synthetic_case(body, library, f"random-{index}", nodes, seed=index)
    print("113 finite original-byte/C cases: full 1 MiB RAM and all "
          "returned registers equal")


def self_cycle_audit(body, library):
    from unicorn import Uc, UC_ARCH_X86, UC_HOOK_CODE, UC_MODE_16
    from unicorn.x86_const import (
        UC_X86_REG_BP, UC_X86_REG_CS, UC_X86_REG_DS, UC_X86_REG_IP,
        UC_X86_REG_SP, UC_X86_REG_SS)

    memory = bytearray(RAM_SIZE)
    cs, ds, ss, sp = 0x3000, 0x1000, 0x2000, 0x8000
    start = physical(cs, ENTRY)
    memory[start:start + len(body)] = body
    put16(memory, ss, sp, 0x7000)
    put16(memory, ss, sp + 2, cs)
    put16(memory, ds, 0x5D96, 0x0100)
    put16(memory, ds, 0x5D98, 0x4000)
    put16(memory, 0x4000, 0x0204, 0x0100)
    put16(memory, 0x4000, 0x0206, 0x4000)
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, RAM_SIZE)
    original.mem_write(0, bytes(memory))
    for register, value in ((UC_X86_REG_CS, cs), (UC_X86_REG_IP, ENTRY),
                            (UC_X86_REG_DS, ds), (UC_X86_REG_SS, ss),
                            (UC_X86_REG_SP, sp), (UC_X86_REG_BP, 0x7777)):
        original.reg_write(register, value)
    visits = [0]
    returned = [False]

    def on_instruction(emulator, address, size, user):
        del size, user
        if address == start + 0x2896 - ENTRY:
            visits[0] += 1
        if not start <= address < start + len(body):
            returned[0] = True
            emulator.emu_stop()

    original.hook_add(UC_HOOK_CODE, on_instruction)
    original.emu_start(start, 0, count=1000)
    check(visits[0] > 10 and not returned[0],
          "self-cycle did not repeatedly revisit +2896")
    # The disjoint record is never written by this routine. Its next
    # pointer remains itself, so the source loop cannot reach the null exit.
    check(bytes(original.mem_read(physical(0x4000, 0x0204), 4)) ==
          bytes(memory[physical(0x4000, 0x0204):physical(0x4000, 0x0204) + 4]),
          "self-cycle edge changed")
    c_ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
    c_cpu = Cpu(0, 0, 0, 0, 0, 0, 0x7777, sp, cs, ENTRY,
                ds, 0, ss, 0x7202)
    status = library.pr_u5e_party_value_2879(
        c_ram, RAM_SIZE, ctypes.byref(c_cpu), 3)
    check(status == 2 and c_cpu.ip == 0x289E,
          "C self-cycle did not report host traversal limit")
    print(f"immutable self-cycle: original revisited +2896 {visits[0]} "
          "times within 1000 instructions; C limit 3 reported separately")


def main():
    root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path,
                        default=root / "resources/DOS/POOLRAD/START.EXE")
    parser.add_argument("--ovr", type=Path,
                        default=root / "resources/DOS/POOLRAD/GAME.OVR")
    parser.add_argument("--unicorn", action="store_true")
    args = parser.parse_args()
    body = source_audit(args.exe, args.ovr)
    if args.unicorn:
        with tempfile.TemporaryDirectory(prefix="poolrad-u5e-2879-") as name:
            library = build_c(root, Path(name))
            synthetic_audit(body, library)
            self_cycle_audit(body, library)


if __name__ == "__main__":
    main()
