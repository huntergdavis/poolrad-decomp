#!/usr/bin/env python3
"""Original U3B0 entry4 two-copy null path through RETF 0x28."""

import ctypes
from hashlib import sha256
import importlib.util
from pathlib import Path
import subprocess
import tempfile

from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_CS, UC_X86_REG_IP

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location(
    "node_gate", ROOT / "tests/recovered/check_overlay_u3b0_entry4_node_gate_0dc7.py")
gate = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gate)

NULL_SHA = "27af6cd147143a685677a7bc7e006ac323503866e5b51c2f131b25e9f19ee702"
EPILOGUE_SHA = "e48af3d400dcabb22010e7972ca8be680047359edd7df4b89cf89381c44466fd"
FULL_SHA = "f7da30b1ef0378e05b7543019da9e3b51ef699f8bc88178364750ee6f1de2963"
BASE = 0x2C8CF
PATH = (0x0DCF, 0x0DD2, 0x0DD4, 0x0DD7, 0x0DDA, 0x0DDC,
        0x0DDF, 0x0DE3, 0x0DE7, 0x10A1, 0x10A4, 0x10A6, 0x10A7)


def native_source():
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    assert len(ovr) == 206105 and sha256(ovr).hexdigest() == gate.OVR_SHA
    _, copy = gate.source()
    null = ovr[BASE + 0x0DCF:BASE + 0x0DEA]
    epilogue = ovr[BASE + 0x10A1:BASE + 0x10AA]
    full = ovr[BASE + 0x0D9A:BASE + 0x10AA]
    assert len(null) == 27 and sha256(null).hexdigest() == NULL_SHA
    assert len(epilogue) == 9 and sha256(epilogue).hexdigest() == EPILOGUE_SHA
    assert len(full) == 784 and sha256(full).hexdigest() == FULL_SHA
    decoder = Cs(CS_ARCH_X86, CS_MODE_16)
    assert [i.address for i in decoder.disasm(null, 0x0DCF)] == list(PATH[:9])
    assert [i.address for i in decoder.disasm(epilogue, 0x10A1)] == list(PATH[9:])
    return full, copy


def pointers(bp, pattern):
    if pattern == "normal":
        return (0x2000, 0x9000), (0x2100, 0x9000)
    if pattern == "same":
        return (0x2000, 0x9000), (0x2000, 0x9000)
    if pattern == "wrap":
        return (0xFFFE, 0x9000), (0xFFFF, 0x9000)
    if pattern == "alias_next":
        return ((bp + 6) & 0xFFFF, gate.SS), (0x2100, 0x9000)
    if pattern == "alias_saved_bp":
        return (0x2000, 0x9000), (bp, gate.SS)
    if pattern == "alias_return":
        return (0x2000, 0x9000), ((bp + 2) & 0xFFFF, gate.SS)
    raise AssertionError(pattern)


def original_frame(full, copy, case):
    sp, lengths, pattern, seed = case
    bp = (sp - 2) & 0xFFFF
    output_a, output_b = pointers(bp, pattern)
    ram = bytearray(gate.RAM_SIZE)
    start = gate.physical(gate.CS, 0x0D9A)
    ram[start:start + len(full)] = full
    start = gate.physical(0x0AF8, 0x0B25)
    ram[start:start + len(copy)] = copy
    cpu = gate.Cpu(0xABCD, 0x1357, 0x2468, 0x3579, 0x468A, 0x579B,
                   0x68AC, sp, gate.CS, 0x0D9A, gate.DS, 0x8000, gate.SS,
                   0x0202 if seed == 0 else 0x0AD7)
    for i in range(-0x1B0, 0x40):
        ram[gate.physical(gate.SS, sp + i)] = (i * 19 + seed * 23) & 255
    gate.put16(ram, gate.SS, sp, 0x1234)
    gate.put16(ram, gate.SS, sp + 2, 0xD000)
    gate.put16(ram, gate.SS, bp + 0x2A, 0x1000)
    gate.put16(ram, gate.SS, bp + 0x2C, gate.SRC)
    gate.put16(ram, gate.SS, bp + 0x26, 0x1100)
    gate.put16(ram, gate.SS, bp + 0x28, gate.SRC)
    gate.put16(ram, gate.SS, bp + 0x14, 0)
    gate.put16(ram, gate.SS, bp + 0x16, 0)
    gate.put16(ram, gate.SS, bp + 0x0A, output_a[0])
    gate.put16(ram, gate.SS, bp + 0x0C, output_a[1])
    gate.put16(ram, gate.SS, bp + 6, output_b[0])
    gate.put16(ram, gate.SS, bp + 8, output_b[1])
    gate.seed_string(ram, 0x1000, lengths[0], 0x21)
    gate.seed_string(ram, 0x1100, lengths[1], 0x62)
    if pattern in ("normal", "same", "wrap"):
        for off, seg in (output_a, output_b):
            for i in range(4):
                ram[gate.physical(seg, off + i)] = (0xA1 + i * 17) & 255
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, gate.RAM_SIZE)
    original.mem_write(0, bytes(ram))
    gate.load_cpu(original, cpu)
    trace = []

    def stop_at_null(emu, address, size, user):
        del address, size, user
        cs = emu.reg_read(UC_X86_REG_CS) & 0xFFFF
        ip = emu.reg_read(UC_X86_REG_IP) & 0xFFFF
        trace.append((cs, ip))
        if (cs, ip) == (gate.CS, 0x0DCF):
            emu.emu_stop()

    original.hook_add(UC_HOOK_CODE, stop_at_null)
    original.emu_start(gate.physical(gate.CS, 0x0D9A), gate.RAM_SIZE,
                       count=2000)
    assert trace[0] == (gate.CS, 0x0D9A)
    assert trace[-1] == (gate.CS, 0x0DCF)
    assert trace.count((0x0AF8, 0x0B25)) == 2
    assert (gate.CS, 0x0DC7) in trace
    frame = bytes(original.mem_read(0, gate.RAM_SIZE))
    at = gate.cpu_from_emu(original)
    assert at.ip == 0x0DCF and at.bp == bp
    assert at.sp == (bp - 0x018E) & 0xFFFF
    return frame, at


def compare_case(full, copy, model, case):
    frame, cpu = original_frame(full, copy, case)
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, gate.RAM_SIZE)
    original.mem_write(0, frame)
    gate.load_cpu(original, cpu)
    trace = []

    def record_path(emu, address, size, user):
        del address, size, user
        trace.append((emu.reg_read(UC_X86_REG_CS) & 0xFFFF,
                      emu.reg_read(UC_X86_REG_IP) & 0xFFFF))

    original.hook_add(UC_HOOK_CODE, record_path)
    original.emu_start(gate.physical(gate.CS, 0x0DCF), gate.RAM_SIZE,
                       count=len(PATH))
    assert trace == [(gate.CS, ip) for ip in PATH], (case, trace)
    expected_ram = bytes(original.mem_read(0, gate.RAM_SIZE))
    actual_ram = (ctypes.c_uint8 * gate.RAM_SIZE).from_buffer_copy(frame)
    actual_cpu = gate.Cpu.from_buffer_copy(cpu)
    assert model(actual_ram, gate.RAM_SIZE, ctypes.byref(actual_cpu)) == 0
    if bytes(actual_ram) != expected_ram:
        changed = [i for i, (a, b) in enumerate(zip(actual_ram, expected_ram))
                   if a != b]
        raise AssertionError((case, "RAM", [(hex(i), hex(actual_ram[i]),
                                             hex(expected_ram[i]))
                                            for i in changed[:12]]))
    for (name, _), reg in zip(gate.Cpu._fields_, gate.REGISTERS):
        expected = original.reg_read(reg) & 0xFFFF
        actual = getattr(actual_cpu, name)
        assert actual == expected, (case, name, hex(actual), hex(expected))


def main():
    full, copy = native_source()
    with tempfile.TemporaryDirectory(dir=ROOT / "resources") as temp:
        shared = Path(temp) / "u3b0_0dcf.so"
        subprocess.run([
            "cc", "-shared", "-fPIC", "-std=c11", "-Wall", "-Wextra",
            "-Wpedantic", "-Werror", "-I", str(ROOT / "include"),
            str(ROOT / "src/recovered/overlay_u3b0_entry4_null_return_0dcf.c"),
            "-o", str(shared)], check=True)
        model = ctypes.CDLL(str(shared)).pr_u3b0_entry4_null_return_0dcf
        model.argtypes = (ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(gate.Cpu))
        model.restype = ctypes.c_int
        cases = [(sp, lengths, pattern, seed)
                 for sp in (0x8000, 0x9001)
                 for lengths in ((0, 0), (44, 60))
                 for pattern in ("normal", "same", "wrap", "alias_next",
                                 "alias_saved_bp", "alias_return")
                 for seed in (0, 1)]
        for case in cases:
            compare_case(full, copy, model, case)
    print(f"U3B0 +0DCF null-return, two original copies, full RAM/14-register parity: {len(cases)} cases PASS")


if __name__ == "__main__":
    main()
