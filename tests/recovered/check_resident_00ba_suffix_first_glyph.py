#!/usr/bin/env python3
"""Compare the copied display suffix's first glyph at its first EGA change."""

import ctypes
from hashlib import sha256
from pathlib import Path
import subprocess
import sys
import tempfile

from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests/recovered"))
sys.path.insert(0, str(ROOT / "tools/overlay"))
import check_resident_0709_064f_first_ega as first  # noqa: E402
import check_resident_00ba_1887_copy_return as copy_gate  # noqa: E402
from check_resident_0709_0626_postcopy_control import compare_cpu  # noqa: E402
from resident_loader_map import unpack  # noqa: E402

BODY_SHA = "645caa0e81651dc9b2be1f156ea343d47f83a195f64f84a5c6fd828a89f50175"
FONT_BYTE = 0x3F


def word(ram, seg, off):
    at = first.physical(seg, off)
    return ram[at] | ram[(at + 1) & (first.RAM_SIZE - 1)] << 8


def first_change(memory, cpu):
    emu = Uc(UC_ARCH_X86, UC_MODE_16)
    emu.mem_map(0, first.RAM_SIZE)
    emu.mem_write(0, bytes(memory))
    for value, reg in zip(cpu, first.REGISTERS):
        emu.reg_write(reg, value)
    changed, stores, result = [], [], []

    def on_code(uc, address, size, user):
        del size, user
        if address == first.physical(0x0709, 0x0415) and changed:
            result.append((bytes(uc.mem_read(0, first.RAM_SIZE)),
                           first.uc_values(uc)))
            uc.emu_stop()

    def on_write(uc, access, address, size, value, user):
        del access, user
        if 0xA0000 <= address < 0xB0000:
            stores.append((address, size, value))
            old = bytes(uc.mem_read(address, size))
            if old != value.to_bytes(size, "little"):
                changed.append((address, size, value, old,
                                uc.reg_read(first.REGISTERS[8]),
                                uc.reg_read(first.REGISTERS[9])))

    emu.hook_add(UC_HOOK_CODE, on_code)
    emu.hook_add(UC_HOOK_MEM_WRITE, on_write)
    emu.emu_start(first.physical(cpu[8], cpu[9]), 0, count=10000)
    assert len(result) == 1, "missing first changed EGA stop"
    assert len(changed) == 1 and changed[0][1:3] == (1, FONT_BYTE)
    assert changed[0][4:] == (0x0709, 0x0412)
    return result[0], stores, changed[0]


def check_case(stops, body, literal, helper, model):
    memory = bytearray(stops[5][1])
    initial = stops[5][2]
    assert initial[8:10] == (0x00BA, 0x1858)
    ss, bp = initial[12], initial[6]
    assert memory[first.physical(ss, bp + 6)] != 0
    for seg, off, code in ((0x00BA, 0x1813, literal),
                           (0x00BA, 0x1816, body),
                           (0x0AF8, 0x0B0B, helper)):
        at = first.physical(seg, off)
        memory[at:at + len(code)] = code
    (copy_ram, copy_cpu), _ = copy_gate.run_to(
        memory, initial, (0x0AF8, 0x0B0B), 100)
    (display_ram, display_cpu), _ = copy_gate.run_to(
        copy_ram, copy_cpu, (0x00BA, 0x1887), 30)
    (printer_ram, printer_cpu), _ = copy_gate.run_to(
        display_ram, display_cpu, (0x0709, 0x0626), 5)
    (guard_ram, guard_cpu), _ = copy_gate.run_to(
        printer_ram, printer_cpu, (0x0709, 0x064F), 10000)
    assert guard_cpu[8:10] == (0x0709, 0x064F)
    printer_ss, printer_bp, printer_ds = guard_cpu[12], guard_cpu[6], guard_cpu[10]
    assert guard_ram[first.physical(printer_ss, printer_bp - 0x100)] == 2
    ch = guard_ram[first.physical(printer_ss, printer_bp - 0xFF)]
    upper = ch - 0x20 if 0x61 <= ch <= 0x7A else ch
    glyph = upper % 0x40
    font_off = word(guard_ram, printer_ds, 0x5E20)
    font_seg = word(guard_ram, printer_ds, 0x5E22)
    memory = bytearray(guard_ram)
    memory[first.physical(font_seg, font_off + glyph * 8)] = FONT_BYTE
    (expected_ram, expected_cpu), stores, changed = first_change(memory,
                                                                  guard_cpu)
    ram = (ctypes.c_uint8 * first.RAM_SIZE).from_buffer_copy(memory)
    cpu = first.Cpu(*guard_cpu)
    report = first.EgaReport()
    assert model(ram, first.RAM_SIZE, ctypes.byref(cpu),
                 ctypes.byref(report)) == 0
    assert bytes(ram) == expected_ram, "first suffix EGA RAM"
    compare_cpu("first suffix EGA", tuple(getattr(cpu, n) for n, _ in
                                            first.Cpu._fields_), expected_cpu)
    page_off = changed[0] - 0xA0000
    reported = (report.glyph_index, report.color, report.plane,
                report.stores, report.page_segment, report.page_offset)
    expected_report = (
        glyph, memory[first.physical(printer_ss, printer_bp + 0x0A)],
        (len(stores) - 1) // 2, len(stores), 0xA000, page_off)
    assert reported == expected_report, (reported, expected_report)
    assert expected_ram[changed[0]] == FONT_BYTE
    assert [at for at in range(0xA0000, 0xB0000)
            if memory[at] != expected_ram[at]] == [changed[0]]
    return page_off, len(stores)


def main():
    raw = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    assert len(raw) == 65231 and sha256(raw).hexdigest() == copy_gate.EXE_SHA
    image, *_ = unpack(raw)
    body = image[0x23B6:0x2432]
    literal = image[0x23B3:0x23B6]
    helper = raw[0xB781:0xB79B]
    assert len(body) == 124 and sha256(body).hexdigest() == BODY_SHA
    assert len(literal) == 3 and sha256(literal).hexdigest() == copy_gate.LITERAL_SHA
    assert len(helper) == 26 and sha256(helper).hexdigest() == copy_gate.HELPER_SHA
    assert sha256((ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
                  ).hexdigest() == first.OVR_SHA
    overlays = first.source()
    prefix = first.resident_prefix()
    printer, copy = first.original_bodies()
    drawer, upcase = first.original_extra()
    with tempfile.TemporaryDirectory(prefix="poolrad-suffix-glyph-",
                                     dir=ROOT / "resources") as tmp:
        library = Path(tmp) / "suffix_glyph.so"
        subprocess.run([
            "cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
            "-shared", "-fPIC", "-I", str(ROOT / "tools/overlay"),
            "-I", str(ROOT / "tools/replay"),
            str(ROOT / "tools/replay/resident_0709_064f_first_ega.c"),
            str(ROOT / "tools/replay/resident_00ba_suffix_first_glyph.c"),
            "-o", str(library)], check=True)
        lib = ctypes.CDLL(str(library))
        glyph_entry = lib.pr_064f_first_glyph_entry
        glyph_entry.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                                ctypes.POINTER(first.Cpu)]
        glyph_entry.restype = ctypes.c_int
        ega_change = lib.pr_064f_first_ega_change
        ega_change.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                               ctypes.POINTER(first.Cpu),
                               ctypes.POINTER(first.EgaReport)]
        ega_change.restype = ctypes.c_int
        model = lib.pr_00ba_suffix_first_glyph
        model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(first.Cpu),
                          ctypes.POINTER(first.EgaReport)]
        model.restype = ctypes.c_int
        checked, page_offsets, stores = 0, set(), set()
        for site, (code, _) in zip(first.SITES, overlays):
            if site[-1] == 0:
                continue
            for color in range(3):
                for ch in (0x54, 0x74):
                    _, _, stops = first.run_case(
                        site, code, ch, color, prefix, printer, copy,
                        drawer, upcase, glyph_entry, ega_change,
                        trace_return=True)
                    page_off, count = check_case(stops, body, literal,
                                                 helper, model)
                    checked += 1
                    page_offsets.add(page_off)
                    stores.add(count)
    assert checked == 12 and page_offsets and stores
    print("RES_00BA copied suffix first glyph: 12/12 original-byte/C "
          "first EGA changes; full 1MiB RAM and 14-field CPU parity; "
          "page offsets", sorted(page_offsets), "store counts", sorted(stores))


if __name__ == "__main__":
    main()
