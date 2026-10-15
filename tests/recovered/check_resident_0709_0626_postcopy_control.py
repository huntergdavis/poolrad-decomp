#!/usr/bin/env python3
"""Original-byte/C full-state copy and first printer guard differential.

The original runtime copy executes. Supported stops are 0709:064F and the
printer's immediate return to 00BA:1858; glyph effects are not entered.
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
    Cpu, RAM_SIZE, REGISTERS, SITES, cpu_values, make_case, physical,
    source, uc_values,
)
from check_resident_1816_overlay_printer_composition import (  # noqa: E402
    Report, resident_prefix,
)
from resident_loader_map import unpack  # noqa: E402

START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
PRINTER_SHA = "b9cd1d6249e9f10945fcf39c09ae313ba672693230d20b29d342bc04bd9894ed"
COPY_SHA = "009955722e8f73d0a0f7076aa418d141f642c5a729b2a58dfa7a7a3922989118"


def original_bodies():
    raw = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    assert sha256(raw).hexdigest() == START_SHA
    image, *_ = unpack(raw)
    printer = image[0x76B6:0x7737]
    copy = raw[0xB79B:0xB7BF]
    assert len(printer) == 129 and sha256(printer).hexdigest() == PRINTER_SHA
    assert len(copy) == 36 and sha256(copy).hexdigest() == COPY_SHA
    return printer, copy


def compare_cpu(label, actual, expected):
    for (name, _), a, b in zip(Cpu._fields_, actual, expected):
        # XOR CH,CH makes AF undefined in the runtime copy.
        mask = 0x0FC5 if name == "flags" else 0xFFFF
        assert (a ^ b) & mask == 0, (label, name, hex(a), hex(b))


def put_emu16(emu, seg, off, value):
    at = physical(seg, off)
    emu.mem_write(at, bytes((value & 255, value >> 8 & 255)))


def check_case(index, site, overlay_code, resident_code, printer_code,
               copy_code, composition, first_gate, copy_step, guard_step,
               color_case, length=None, col_override=None, cap_override=None):
    label = f"{site[0]:03X}:+{site[1]:04X}/color-{color_case}"
    if col_override is not None:
        label += f"/column-{col_override}"
    if cap_override is not None:
        label += f"/cap-{cap_override}"
    memory, initial, frame = make_case(site, overlay_code, color_case)
    node_off, node_seg = frame[3:]
    length = (0, 1, 14)[color_case] if length is None else length
    memory[physical(0x00BA, 0x1816):physical(0x00BA, 0x1816) +
           len(resident_code)] = resident_code
    memory[physical(0x0709, 0x0626):physical(0x0709, 0x0626) +
           len(printer_code)] = printer_code
    memory[physical(0x0AF8, 0x0B25):physical(0x0AF8, 0x0B25) +
           len(copy_code)] = copy_code
    for offset in range(length + 1):
        memory[physical(node_seg, node_off + offset)] = (
            length if offset == 0 else (offset * 11 + 7) & 255)
    first, second = ((0, 0xA5), (7, 1), (0xFF, 2))[color_case]
    memory[physical(node_seg, node_off + 0x10D)] = first
    memory[physical(node_seg, node_off + 0x10E)] = second

    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, RAM_SIZE)
    original.mem_write(0, bytes(memory))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        original.reg_write(reg, getattr(initial, name))
    snapshots = []

    def on_code(emu, address, size, user):
        del size, user
        if address == physical(0x0709, 0x0626):
            snapshots.append(("printer", bytes(emu.mem_read(0, RAM_SIZE)),
                              uc_values(emu)))
        elif address == physical(0x0AF8, 0x0B25):
            ss, bp, sp = (emu.reg_read(REGISTERS[i]) for i in (12, 6, 7))
            if col_override is not None:
                emu.mem_write(physical(ss, bp + 0x0E), bytes((col_override,)))
            if cap_override is not None:
                put_emu16(emu, ss, sp + 4, cap_override)
            snapshots.append(("copy", bytes(emu.mem_read(0, RAM_SIZE)),
                              uc_values(emu)))
        elif address == physical(0x0709, 0x0641):
            snapshots.append(("returned", bytes(emu.mem_read(0, RAM_SIZE)),
                              uc_values(emu)))
        elif address == physical(0x0709, 0x064F):
            snapshots.append(("valid", bytes(emu.mem_read(0, RAM_SIZE)),
                              uc_values(emu)))
            emu.emu_stop()
        elif address == physical(0x00BA, 0x1858):
            snapshots.append(("rejected", bytes(emu.mem_read(0, RAM_SIZE)),
                              uc_values(emu)))
            emu.emu_stop()

    original.hook_add(UC_HOOK_CODE, on_code)
    original.emu_start(physical(initial.cs, site[3]), 0, count=500)
    names = [x[0] for x in snapshots]
    assert names[:3] == ["printer", "copy", "returned"], (label, names)
    assert len(names) == 4 and names[-1] in ("valid", "rejected"), (label, names)

    ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
    cpu = Cpu(*cpu_values(initial))
    report = Report()
    assert composition(index, ram, RAM_SIZE, ctypes.byref(cpu),
                       ctypes.byref(report)) == 0, label
    assert report.event_count == 6 and list(report.order) == list(range(6))
    assert bytes(ram) == snapshots[0][1], (label, "printer-entry RAM")
    compare_cpu(label + "/printer", cpu_values(cpu), snapshots[0][2])

    assert first_gate(ram, RAM_SIZE, ctypes.byref(cpu)) == 0, label
    if col_override is not None:
        ram[physical(cpu.ss, cpu.bp + 0x0E)] = col_override
    if cap_override is not None:
        at = physical(cpu.ss, cpu.sp + 4)
        ram[at:at + 2] = bytes((cap_override & 255,
                                cap_override >> 8 & 255))
    assert bytes(ram) == snapshots[1][1], (label, "copy-entry RAM")
    compare_cpu(label + "/copy", cpu_values(cpu), snapshots[1][2])

    assert copy_step(ram, RAM_SIZE, ctypes.byref(cpu)) == 0, label
    assert bytes(ram) == snapshots[2][1], (label, "copy-return RAM")
    compare_cpu(label + "/copy-return", cpu_values(cpu), snapshots[2][2])
    if cap_override is not None:
        assert ram[physical(cpu.ss, cpu.bp - 0x100)] == cap_override
    stop = ctypes.c_int()
    assert guard_step(ram, RAM_SIZE, ctypes.byref(cpu),
                      ctypes.byref(stop)) == 0, label
    assert stop.value == (1 if names[-1] == "valid" else 2), label
    assert bytes(ram) == snapshots[3][1], (label, "guard-stop RAM")
    compare_cpu(label + "/guard-stop", cpu_values(cpu), snapshots[3][2])
    return names[-1]


def main():
    overlays = source()
    resident_code = resident_prefix()
    printer_code, copy_code = original_bodies()
    with tempfile.TemporaryDirectory(prefix="poolrad-0626-postcopy-",
                                     dir=ROOT / "resources") as tmp:
        library = Path(tmp) / "gates.so"
        subprocess.run([
            "cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
            "-shared", "-fPIC", "-I", str(ROOT / "tools/overlay"),
            "-I", str(ROOT / "tools/replay"),
            str(ROOT / "tools/overlay/resident_1816_caller_routes.c"),
            str(ROOT / "tools/replay/resident_1816_printer_gate.c"),
            str(ROOT / "tools/replay/resident_1816_overlay_printer_composition.c"),
            str(ROOT / "tools/replay/resident_0709_printer_copy_gate.c"),
            str(ROOT / "tools/replay/resident_0709_0626_postcopy_control.c"),
            "-o", str(library)], check=True)
        lib = ctypes.CDLL(str(library))
        composition = lib.pr_1816_overlay_to_first_printer
        composition.argtypes = [ctypes.c_uint, ctypes.POINTER(ctypes.c_uint8),
                                ctypes.c_size_t, ctypes.POINTER(Cpu),
                                ctypes.POINTER(Report)]
        composition.restype = ctypes.c_int
        first_gate = lib.pr_0709_0626_first_copy_gate
        first_gate.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                               ctypes.POINTER(Cpu)]
        first_gate.restype = ctypes.c_int
        copy_step = lib.pr_0626_runtime_copy_return
        copy_step.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                              ctypes.POINTER(Cpu)]
        copy_step.restype = ctypes.c_int
        guard_step = lib.pr_0626_postcopy_guard
        guard_step.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                               ctypes.POINTER(Cpu), ctypes.POINTER(ctypes.c_int)]
        guard_step.restype = ctypes.c_int
        stops = []
        for index, (site, (code, _)) in enumerate(zip(SITES, overlays)):
            for color_case in range(3):
                stops.append(check_case(
                    index, site, code, resident_code, printer_code, copy_code,
                    composition, first_gate, copy_step, guard_step, color_case))
        stops.append(check_case(0, SITES[0], overlays[0][0], resident_code,
                                printer_code, copy_code, composition,
                                first_gate, copy_step, guard_step, 0,
                                col_override=40))
        stops.append(check_case(0, SITES[0], overlays[0][0], resident_code,
                                printer_code, copy_code, composition,
                                first_gate, copy_step, guard_step, 2,
                                length=14, cap_override=2))
        stops.append(check_case(0, SITES[0], overlays[0][0], resident_code,
                                printer_code, copy_code, composition,
                                first_gate, copy_step, guard_step, 2,
                                length=255))
    assert "valid" in stops and "rejected" in stops
    print("RES_0709:0626 actual runtime copy and guards: 27 original-byte/C "
          "full-state composed cases passed")
    print(f"supported stops: valid={stops.count('valid')} "
          f"printer-return={stops.count('rejected')}")


if __name__ == "__main__":
    main()
