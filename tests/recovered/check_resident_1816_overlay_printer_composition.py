#!/usr/bin/env python3
"""Eight original overlay callers composed with 1816 to first printer entry.

The original printer body is never executed; output is address/count/hash only.
"""

import ctypes
from hashlib import sha256
from pathlib import Path
import subprocess
import sys
import tempfile

from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests/recovered"))
sys.path.insert(0, str(ROOT / "tools/overlay"))
from check_resident_1816_overlay_callers import (  # noqa: E402
    Cpu, Frame, RAM_SIZE, REGISTERS, SITES, cpu_values, make_case,
    physical, read16, source, uc_values,
)
from resident_loader_map import unpack  # noqa: E402

START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
PREFIX_SHA = "7b68709c7c03334a3b277bf7e2b100849407e2bf09c1f17b1e34f03e578c7d99"


class Report(ctypes.Structure):
    _fields_ = [("event_count", ctypes.c_uint),
                ("order", ctypes.c_int * 6),
                ("pushed_words", ctypes.c_uint16 * 5),
                ("frame", Frame)]


def resident_prefix():
    raw = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    assert sha256(raw).hexdigest() == START_SHA
    image, *_ = unpack(raw)
    code = image[0x23B6:0x23F8]
    assert len(code) == 0x42 and sha256(code).hexdigest() == PREFIX_SHA
    return code


def check_case(index, site, overlay_code, prefix, model, color_case):
    label = f"{site[0]:03X}:+{site[1]:04X}/color-{color_case}"
    memory, start_cpu, caller_frame = make_case(site, overlay_code,
                                                 color_case)
    memory[physical(0x00BA, 0x1816):physical(0x00BA, 0x1816) + len(prefix)] = (
        prefix)
    node_off, node_seg = caller_frame[3:]
    first, second, color = ((0, 0xA5, 0x0C),
                            (7, 1, 0x0E),
                            (0xFF, 2, 0x0B))[color_case]
    memory[physical(node_seg, node_off + 0x10D)] = first
    memory[physical(node_seg, node_off + 0x10E)] = second

    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, RAM_SIZE)
    original.mem_write(0, bytes(memory))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        original.reg_write(reg, getattr(start_cpu, name))
    seen, snapshots = [], []

    def on_code(emu, address, size, user):
        del size, user
        seen.append((emu.reg_read(REGISTERS[8]), emu.reg_read(REGISTERS[9])))
        if address == physical(0x0709, 0x0626):
            snapshots.append((bytes(emu.mem_read(0, RAM_SIZE)), uc_values(emu)))
            emu.emu_stop()

    original.hook_add(UC_HOOK_CODE, on_code)
    original.emu_start(physical(start_cpu.cs, site[3]), 0, count=80)
    assert len(snapshots) == 1 and seen[0] == (start_cpu.cs, site[3])
    assert seen[-1] == (0x0709, 0x0626), (label, seen[-1])
    expected_ram, expected_cpu = snapshots[0]
    sp = expected_cpu[7]
    expected_stack = (0x1858, 0x00BA, node_off, node_seg,
                      (caller_frame[0] & 0xFF00) | color,
                      caller_frame[1], caller_frame[2])
    assert tuple(read16(expected_ram, start_cpu.ss, sp + 2 * i)
                 for i in range(7)) == expected_stack, label

    ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
    cpu = Cpu(*cpu_values(start_cpu))
    report = Report()
    assert model(index, ram, RAM_SIZE, ctypes.byref(cpu),
                 ctypes.byref(report)) == 0, label
    assert report.event_count == 6 and list(report.order) == list(range(6))
    assert list(report.pushed_words) == list(reversed(caller_frame))
    assert tuple(getattr(report.frame, name) for name, _ in Frame._fields_) == (
        caller_frame)
    assert bytes(ram) == expected_ram, (label, "RAM mismatch")
    for (name, _), actual, expected in zip(Cpu._fields_, cpu_values(cpu),
                                           expected_cpu):
        mask = 0x0FD5 if name == "flags" else 0xFFFF
        assert (actual ^ expected) & mask == 0, (
            label, name, hex(actual), hex(expected))


def main():
    overlays = source()
    prefix = resident_prefix()
    with tempfile.TemporaryDirectory(prefix="poolrad-1816-compose-",
                                     dir=ROOT / "resources") as tmp:
        library = Path(tmp) / "composition.so"
        subprocess.run([
            "cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
            "-shared", "-fPIC", "-I", str(ROOT / "tools/overlay"),
            "-I", str(ROOT / "tools/replay"),
            str(ROOT / "tools/overlay/resident_1816_caller_routes.c"),
            str(ROOT / "tools/replay/resident_1816_printer_gate.c"),
            str(ROOT / "tools/replay/resident_1816_overlay_printer_composition.c"),
            "-o", str(library)], check=True)
        model = ctypes.CDLL(str(library)).pr_1816_overlay_to_first_printer
        model.argtypes = [ctypes.c_uint, ctypes.POINTER(ctypes.c_uint8),
                          ctypes.c_size_t, ctypes.POINTER(Cpu),
                          ctypes.POINTER(Report)]
        model.restype = ctypes.c_int
        for index, (site, (code, _)) in enumerate(zip(SITES, overlays)):
            for color_case in range(3):
                check_case(index, site, code, prefix, model, color_case)
    print("Eight overlay callers -> RES_00BA:1816 -> RES_0709:0626: "
          "24 original-byte/C full-state first-entry cases passed")
    print("Printer body, return effects, and optional suffix remain unresolved")


if __name__ == "__main__":
    main()
