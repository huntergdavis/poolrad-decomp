#!/usr/bin/env python3
"""Original-byte/C state gate from eight callers into printer's first copy.

Original media and full RAM stay ignored. Public output is count/hash only.
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
    Cpu, RAM_SIZE, REGISTERS, SITES, cpu_values, make_case, physical,
    read16, source, uc_values,
)
from check_resident_1816_overlay_printer_composition import (  # noqa: E402
    Report, resident_prefix,
)
from resident_loader_map import unpack  # noqa: E402

START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
PRINTER_PREFIX_SHA = "372493c3602f9a20a394587515421dd79b7fdb8932a2c728bd4ec390d8647a4b"


def printer_prefix():
    raw = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    assert sha256(raw).hexdigest() == START_SHA
    image, *_ = unpack(raw)
    code = image[0x76B6:0x76D1]
    assert len(code) == 27 and sha256(code).hexdigest() == PRINTER_PREFIX_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(code, 0x0626))
    assert ins[0].address == 0x0626 and ins[-1].address == 0x063C
    assert ins[-1].mnemonic == "lcall" and ins[-1].op_str == "0xaf8, 0xb25"
    assert not any(x.mnemonic.startswith("j") for x in ins)
    return code


def compare_cpu(label, actual, expected):
    for (name, _), a, b in zip(Cpu._fields_, actual, expected):
        mask = 0x0FD5 if name == "flags" else 0xFFFF
        assert (a ^ b) & mask == 0, (label, name, hex(a), hex(b))


def check_case(index, site, overlay_code, resident_code,
               printer_code, composition, gate, color_case,
               sp_override=None, expected_flag=None):
    label = f"{site[0]:03X}:+{site[1]:04X}/color-{color_case}"
    if sp_override is not None:
        label += f"/sp-{sp_override:04X}"
    memory, initial, frame = make_case(site, overlay_code, color_case)
    if sp_override is not None:
        initial.sp = sp_override
    node_off, node_seg = frame[3:]
    memory[physical(0x00BA, 0x1816):physical(0x00BA, 0x1816) +
           len(resident_code)] = resident_code
    memory[physical(0x0709, 0x0626):physical(0x0709, 0x0626) +
           len(printer_code)] = printer_code
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
            snapshots.append(("copy", bytes(emu.mem_read(0, RAM_SIZE)),
                              uc_values(emu)))
            emu.emu_stop()

    original.hook_add(UC_HOOK_CODE, on_code)
    original.emu_start(physical(initial.cs, site[3]), 0, count=80)
    assert [x[0] for x in snapshots] == ["printer", "copy"], label
    ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
    cpu = Cpu(*cpu_values(initial))
    report = Report()
    assert composition(index, ram, RAM_SIZE, ctypes.byref(cpu),
                       ctypes.byref(report)) == 0, label
    assert report.event_count == 6 and list(report.order) == list(range(6))
    assert bytes(ram) == snapshots[0][1], (label, "printer-entry RAM")
    compare_cpu(label + "/printer", cpu_values(cpu), snapshots[0][2])

    assert gate(ram, RAM_SIZE, ctypes.byref(cpu)) == 0, label
    assert bytes(ram) == snapshots[1][1], (label, "copy-entry RAM")
    compare_cpu(label + "/copy", cpu_values(cpu), snapshots[1][2])
    if expected_flag is not None:
        assert cpu.flags & expected_flag, (label, hex(cpu.flags))
    sp = cpu.sp
    assert tuple(read16(ram, cpu.ss, sp + 2 * i) for i in range(7)) == (
        0x0641, 0x0709, 0x00FF, (cpu.bp - 0x100) & 0xFFFF, cpu.ss,
        node_off, node_seg), label


def main():
    overlays = source()
    resident_code = resident_prefix()
    printer_code = printer_prefix()
    with tempfile.TemporaryDirectory(prefix="poolrad-0709-copy-",
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
            "-o", str(library)], check=True)
        lib = ctypes.CDLL(str(library))
        composition = lib.pr_1816_overlay_to_first_printer
        composition.argtypes = [ctypes.c_uint, ctypes.POINTER(ctypes.c_uint8),
                                ctypes.c_size_t, ctypes.POINTER(Cpu),
                                ctypes.POINTER(Report)]
        composition.restype = ctypes.c_int
        gate = lib.pr_0709_0626_first_copy_gate
        gate.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                         ctypes.POINTER(Cpu)]
        gate.restype = ctypes.c_int
        for index, (site, (code, _)) in enumerate(zip(SITES, overlays)):
            for color_case in range(3):
                check_case(index, site, code, resident_code, printer_code,
                           composition, gate, color_case)
        # Three direct stack seeds make the printer's SUB SP,0104 set
        # CF, ZF, and OF respectively; all still begin at a real caller.
        for color_case, sp, flag in ((2, 0x0110, 0x0001),
                                     (1, 0x0128, 0x0040),
                                     (0, 0x8024, 0x0800)):
            check_case(0, SITES[0], overlays[0][0], resident_code,
                       printer_code, composition, gate, color_case,
                       sp_override=sp, expected_flag=flag)
    print("RES_0709:0626 -> 0AF8:0B25 copy-entry gate: 27 original-byte/C "
          "full-state composed cases passed (24 baseline, 3 stack flags)")
    print("Runtime copy, printer return, and 1816 suffix remain unresolved")


if __name__ == "__main__":
    main()
