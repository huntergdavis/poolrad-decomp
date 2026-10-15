#!/usr/bin/env python3
"""Original-byte/C full-state gate from 0709:0768 to its first copy call.

Overlay call operands are pinned, but callers are not executed here. Entry
frames are synthetic, including the documented U1C PRINT/PRINTCLEAR forms.
"""

import ctypes
from hashlib import sha256
from pathlib import Path
import subprocess
import sys
import tempfile

from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests/recovered"))
sys.path.insert(0, str(ROOT / "tools/overlay"))
from check_resident_1816_overlay_callers import (  # noqa: E402
    Cpu, RAM_SIZE, REGISTERS, cpu_values, physical, put16, read16, uc_values,
)
from resident_inventory import collect  # noqa: E402
from resident_loader_map import unpack  # noqa: E402

START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
PREFIX_SHA = "bd3a9fabc69274f2866c5d9ee448b805191310bf7fe81c9d693935d6c8b31765"
CALL_SHA = "505ad54ab9e5390450c330a9decab8bab5ee9f07d44dc475f2414fa8afc6f9ea"
TEMPLATE = bytes((i * 37 + 11) & 255 for i in range(RAM_SIZE))


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    assert sha256(exe).hexdigest() == START_SHA
    assert sha256(ovr).hexdigest() == OVR_SHA
    image, *_ = unpack(exe)
    code = image[0x77F8:0x7813]
    assert len(code) == 27 and sha256(code).hexdigest() == PREFIX_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(code, 0x0768))
    assert ins[0].address == 0x0768 and ins[-1].address == 0x077E
    assert ins[-1].mnemonic == "lcall" and ins[-1].op_str == "0xaf8, 0xb25"
    assert not any(x.mnemonic.startswith("j") for x in ins)
    sites = next(x for x in collect(exe, ovr)["resident_targets"]
                 if x["id"] == "RES_0709_0768")["sources"]
    assert len(sites) == 27
    for site in sites:
        call = ovr[site["ovr_file_offset"]:site["ovr_file_offset"] + 5]
        assert sha256(call).hexdigest() == CALL_SHA
    assert [(x["unit_offset"], x["ovr_file_offset"]) for x in sites
            if x["unit"] == 0x1C and x["unit_offset"] in (0x0A69, 0x0A91)] == [
                (0x0A69, 0x17AF), (0x0A91, 0x17D7)]
    return code, sites


def make_case(code, index, sp_override=None):
    seed = index % 3
    arm = index % 2
    cs, ss, ds = 0x0709, 0xE000, 0x1000
    sp = (0x8000, 0x0010, 0x9000)[seed] if sp_override is None else sp_override
    source_off = (0x70C8, 0xFFFE, 0x0000)[seed]
    source_seg = (ds, 0x2000, 0x7FFF)[seed]
    ax = (0xABCD, 0x12EF, 0xFF00)[seed]
    memory = bytearray(TEMPLATE)
    memory[physical(cs, 0x0768):physical(cs, 0x0768) + len(code)] = code
    # Far return, source pointer, then six words reversed from the U1C
    # push order (1, 0x11, 0x26, 0x16, 0x0A, PRINT/PRINTCLEAR flag).
    words = (0xA159, 0x1C00, source_off, source_seg,
             (ax & 0xFF00) | arm, (ax & 0xFF00) | 0x0A,
             (ax & 0xFF00) | 0x16, (ax & 0xFF00) | 0x26,
             (ax & 0xFF00) | 0x11, (ax & 0xFF00) | 1)
    for at, word in enumerate(words):
        put16(memory, ss, sp + 2 * at, word)
    cpu = Cpu(ax, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x3000, sp,
              cs, 0x0768, ds, 0x1111, ss,
              (0x3202, 0x3603, 0x3206)[seed])
    return memory, cpu, source_off, source_seg


def check_case(code, model, index, sp_override=None, expected_flag=None):
    label = f"case-{index}" if sp_override is None else f"sp-{sp_override:04X}"
    memory, initial, source_off, source_seg = make_case(
        code, index, sp_override)
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, RAM_SIZE)
    original.mem_write(0, bytes(memory))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        original.reg_write(reg, getattr(initial, name))
    snapshots = []

    def on_code(emu, address, size, user):
        del size, user
        if address == physical(0x0AF8, 0x0B25):
            snapshots.append((bytes(emu.mem_read(0, RAM_SIZE)), uc_values(emu)))
            emu.emu_stop()

    original.hook_add(UC_HOOK_CODE, on_code)
    original.emu_start(physical(0x0709, 0x0768), 0, count=20)
    assert len(snapshots) == 1, label
    expected_ram, expected_cpu = snapshots[0]
    ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
    cpu = Cpu(*cpu_values(initial))
    assert model(ram, RAM_SIZE, ctypes.byref(cpu)) == 0, label
    assert bytes(ram) == expected_ram, (label, "RAM mismatch")
    for (name, _), actual, expected in zip(Cpu._fields_, cpu_values(cpu),
                                           expected_cpu):
        mask = 0x0FD5 if name == "flags" else 0xFFFF
        assert (actual ^ expected) & mask == 0, (
            label, name, hex(actual), hex(expected))
    if expected_flag is not None:
        assert cpu.flags & expected_flag, (label, hex(cpu.flags))
    assert tuple(read16(ram, cpu.ss, cpu.sp + 2 * i) for i in range(7)) == (
        0x0783, 0x0709, 0x00FF, (cpu.bp - 0x100) & 0xFFFF, cpu.ss,
        source_off, source_seg), label


def main():
    code, sites = source()
    with tempfile.TemporaryDirectory(prefix="poolrad-0709-0768-",
                                     dir=ROOT / "resources") as tmp:
        library = Path(tmp) / "gate.so"
        subprocess.run([
            "cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
            "-shared", "-fPIC", "-I", str(ROOT / "tools/overlay"),
            str(ROOT / "tools/replay/resident_0709_printer_0768_copy_gate.c"),
            "-o", str(library)], check=True)
        model = ctypes.CDLL(str(library)).pr_0709_0768_first_copy_gate
        model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(Cpu)]
        model.restype = ctypes.c_int
        for index, _site in enumerate(sites):
            check_case(code, model, index)
        for index, sp, flag in ((2, 0x0110, 0x0001),
                                (1, 0x0128, 0x0040),
                                (0, 0x8002, 0x0800)):
            check_case(code, model, index, sp_override=sp,
                       expected_flag=flag)
    print("RES_0709:0768 -> 0AF8:0B25 first-copy gate: 30 original-byte/C "
          "full-state synthetic-entry cases passed")
    print("27 static overlay call operands pinned; caller paths not executed")


if __name__ == "__main__":
    main()
