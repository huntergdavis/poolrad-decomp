#!/usr/bin/env python3
"""Original-byte/C continuation from first changed EGA store to glyph return."""

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


def run_patterned_tail(stops, model, draw_segment):
    """Replay later nonzero scanlines from the already reached 0415 stop."""
    memory = bytearray(stops[3][1])
    initial = stops[3][2]
    font = first.physical(0x3000, 0x1000 + 0x14 * 8)
    memory[font + 1:font + 8] = bytes((0x81, 0x42, 0x24, 0x18,
                                       0xA5, 0x5A, 0xFF))
    first.put16(memory, initial[10], 0x4A18, draw_segment)
    emu = Uc(UC_ARCH_X86, UC_MODE_16)
    emu.mem_map(0, first.RAM_SIZE)
    emu.mem_write(0, bytes(memory))
    for value, reg in zip(initial, first.REGISTERS):
        emu.reg_write(reg, value)
    expected = []

    def on_code(uc, address, size, user):
        del size, user
        if address == first.physical(0x0709, 0x0694):
            expected.append((bytes(uc.mem_read(0, first.RAM_SIZE)),
                             first.uc_values(uc)))
            uc.emu_stop()

    emu.hook_add(UC_HOOK_CODE, on_code)
    emu.emu_start(first.physical(0x0709, 0x0415), 0, count=10000)
    assert len(expected) == 1
    ram = (ctypes.c_uint8 * first.RAM_SIZE).from_buffer_copy(memory)
    cpu = first.Cpu(*initial)
    assert model(ram, first.RAM_SIZE, ctypes.byref(cpu)) == 0
    assert bytes(ram) == expected[0][0], "patterned tail RAM"
    compare_cpu("patterned first return",
                tuple(getattr(cpu, name) for name, _ in first.Cpu._fields_),
                expected[0][1])


def main():
    assert sha256((ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
                  ).hexdigest() == first.OVR_SHA
    overlays = first.source()
    prefix = first.resident_prefix()
    printer, copy = first.original_bodies()
    drawer, upcase = first.original_extra()
    with tempfile.TemporaryDirectory(prefix="poolrad-first-return-",
                                     dir=ROOT / "resources") as tmp:
        library = Path(tmp) / "first_return.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC", "-I",
                        str(ROOT / "tools/overlay"),
                        str(ROOT / "tools/replay/resident_0709_064f_first_ega.c"),
                        str(ROOT / "tools/replay/resident_0709_first_glyph_return.c"),
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
        model = lib.pr_0415_first_glyph_return
        model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(first.Cpu)]
        model.restype = ctypes.c_int
        count = patterned = 0
        for site, (code, _) in zip(first.SITES, overlays):
            for color in range(3):
                for glyph in (0x54, 0x74):
                    _, _, stops = first.run_case(
                        site, code, glyph, color, prefix, printer, copy,
                        drawer, upcase, entry, change, trace_return=True)
                    assert [x[0] for x in stops] == [
                        "guard", "drawer", "before", "after",
                        "glyph-return", "printer-return"]
                    ram = (ctypes.c_uint8 * first.RAM_SIZE).from_buffer_copy(
                        stops[3][1])
                    cpu = first.Cpu(*stops[3][2])
                    assert model(ram, first.RAM_SIZE, ctypes.byref(cpu)) == 0
                    assert bytes(ram) == stops[4][1], (site[1], color, glyph,
                                                       "return RAM")
                    compare_cpu("first glyph return",
                                tuple(getattr(cpu, name) for name, _ in
                                      first.Cpu._fields_), stops[4][2])
                    assert stops[5][2][8:10] == (0x00BA, 0x1858)
                    count += 1
                    if site == first.SITES[0]:
                        for draw_segment in (0xA000, 0xB000):
                            run_patterned_tail(stops, model, draw_segment)
                            patterned += 1
    assert (count, patterned) == (48, 12)
    print("RES_0709:0415 -> first glyph 0694: 48/48 original-byte/C "
          "full-state cases and 12/12 patterned-font tails passed")


if __name__ == "__main__":
    main()
