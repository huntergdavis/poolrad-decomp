#!/usr/bin/env python3
"""Original-byte/C single-glyph printer exit after the first EGA drawer."""

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
import check_resident_0709_064f_first_ega as first  # noqa: E402
from check_resident_0709_0626_postcopy_control import compare_cpu  # noqa: E402


def run_wrap_case(stops, model):
    """Mutate only the reached 0694 boundary to check low-byte wrap and AH."""
    memory = bytearray(stops[4][1])
    initial = list(stops[4][2])
    ss, bp = initial[12], initial[6]
    memory[first.physical(ss, bp + 0x0E)] = 0xFF
    initial[0] = 0x5A01
    emu = Uc(UC_ARCH_X86, UC_MODE_16)
    emu.mem_map(0, first.RAM_SIZE)
    emu.mem_write(0, bytes(memory))
    for value, reg in zip(initial, first.REGISTERS):
        emu.reg_write(reg, value)
    expected = []

    def on_code(uc, address, size, user):
        del size, user
        if address == first.physical(0x00BA, 0x1858):
            expected.append((bytes(uc.mem_read(0, first.RAM_SIZE)),
                             first.uc_values(uc)))
            uc.emu_stop()

    emu.hook_add(UC_HOOK_CODE, on_code)
    emu.emu_start(first.physical(0x0709, 0x0694), 0, count=100)
    assert len(expected) == 1
    ram = (ctypes.c_uint8 * first.RAM_SIZE).from_buffer_copy(memory)
    cpu = first.Cpu(*initial)
    assert model(ram, first.RAM_SIZE, ctypes.byref(cpu)) == 0
    assert bytes(ram) == expected[0][0], "wrapped column RAM"
    compare_cpu("wrapped column return",
                tuple(getattr(cpu, name) for name, _ in first.Cpu._fields_),
                expected[0][1])
    assert ram[first.physical(ss, bp + 0x0E)] == 0
    assert cpu.ax == 0x5A01


def main():
    assert sha256((ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
                  ).hexdigest() == first.OVR_SHA
    overlays = first.source()
    prefix = first.resident_prefix()
    printer, copy = first.original_bodies()
    drawer, upcase = first.original_extra()
    with tempfile.TemporaryDirectory(prefix="poolrad-single-exit-",
                                     dir=ROOT / "resources") as tmp:
        library = Path(tmp) / "single_exit.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC", "-I",
                        str(ROOT / "tools/overlay"),
                        str(ROOT / "tools/replay/resident_0709_064f_first_ega.c"),
                        str(ROOT / "tools/replay/resident_0709_single_glyph_printer_exit.c"),
                        "-o", str(library)], check=True)
        lib = ctypes.CDLL(str(library))
        entry = lib.pr_064f_first_glyph_entry
        entry.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(first.Cpu)]
        entry.restype = ctypes.c_int
        change = lib.pr_064f_first_ega_change
        change.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                           ctypes.POINTER(first.Cpu),
                           ctypes.POINTER(first.EgaReport)]
        change.restype = ctypes.c_int
        model = lib.pr_0694_single_glyph_printer_exit
        model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(first.Cpu)]
        model.restype = ctypes.c_int
        count = wrapped = 0
        for site, (code, _) in zip(first.SITES, overlays):
            for color in range(3):
                for glyph in (0x54, 0x74):
                    _, _, stops = first.run_case(
                        site, code, glyph, color, prefix, printer, copy,
                        drawer, upcase, entry, change, trace_return=True)
                    assert [x[0] for x in stops] == [
                        "guard", "drawer", "before", "after",
                        "glyph-return", "printer-return"]
                    before_ram, before_cpu = stops[4][1:]
                    ss, bp = before_cpu[12], before_cpu[6]
                    assert before_ram[first.physical(ss, bp - 0x102)] == 1
                    assert before_ram[first.physical(ss, bp - 0x103)] == 1
                    ram = (ctypes.c_uint8 * first.RAM_SIZE).from_buffer_copy(
                        before_ram)
                    cpu = first.Cpu(*before_cpu)
                    assert model(ram, first.RAM_SIZE, ctypes.byref(cpu)) == 0
                    assert bytes(ram) == stops[5][1], (site[1], color, glyph,
                                                       "printer RAM")
                    compare_cpu("single-glyph printer return",
                                tuple(getattr(cpu, name) for name, _ in
                                      first.Cpu._fields_), stops[5][2])
                    count += 1
                    if site == first.SITES[0]:
                        run_wrap_case(stops, model)
                        wrapped += 1
    assert (count, wrapped) == (48, 6)
    print("RES_0709:0694 single-glyph printer -> 00BA:1858: "
          "48/48 original-byte/C full-state cases and "
          "6/6 column-wrap/AH cases passed")


if __name__ == "__main__":
    main()
