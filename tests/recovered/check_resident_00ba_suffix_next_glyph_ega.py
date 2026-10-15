#!/usr/bin/env python3
"""Original-byte/C audit of the copied suffix's second glyph EGA boundary."""

import ctypes
from hashlib import sha256
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests/recovered"))
sys.path.insert(0, str(ROOT / "tools/overlay"))
import check_resident_0709_064f_first_ega as first  # noqa: E402
import check_resident_00ba_1887_copy_return as copy_gate  # noqa: E402
import check_resident_00ba_suffix_first_glyph as suffix  # noqa: E402
import check_resident_00ba_suffix_glyph_control as control  # noqa: E402
from check_resident_0709_0626_postcopy_control import compare_cpu  # noqa: E402
from resident_loader_map import unpack  # noqa: E402


def compare_state(label, ram, cpu, expected_ram, expected_cpu):
    assert bytes(ram) == expected_ram, (label, "RAM")
    compare_cpu(label, tuple(getattr(cpu, n) for n, _ in
                             first.Cpu._fields_), expected_cpu)


def check_case(stops, body, literal, helper, loop_model, glyph_model):
    return_ram, return_cpu = control.prepare_0694(stops, body, literal, helper)
    (entry_ram, entry_cpu), _ = copy_gate.run_to(
        return_ram, return_cpu, (0x0709, 0x01DF), 50)
    ram = (ctypes.c_uint8 * first.RAM_SIZE).from_buffer_copy(return_ram)
    cpu = first.Cpu(*return_cpu)
    loop_report = control.Report()
    assert loop_model(ram, first.RAM_SIZE, ctypes.byref(cpu),
                      ctypes.byref(loop_report)) == 0
    assert loop_report.stop == 1 and loop_report.glyph_index_after == 2
    compare_state("second glyph entry", ram, cpu, entry_ram, entry_cpu)

    ss, bp, ds, sp = entry_cpu[12], entry_cpu[6], entry_cpu[10], entry_cpu[7]
    assert entry_ram[first.physical(ss, bp - 0x102)] == 2
    assert entry_ram[first.physical(ss, bp - 0x103)] == 2
    ch = control.word(entry_ram, ss, sp + 6) & 0xFF
    upper = ch - 0x20 if 0x61 <= ch <= 0x7A else ch
    glyph = upper % 0x40
    font_off = control.word(entry_ram, ds, 0x5E20)
    font_seg = control.word(entry_ram, ds, 0x5E22)
    memory = bytearray(entry_ram)
    memory[first.physical(font_seg, font_off + glyph * 8)] = suffix.FONT_BYTE
    (expected_ram, expected_cpu), stores, changed = suffix.first_change(
        memory, entry_cpu)
    ram = (ctypes.c_uint8 * first.RAM_SIZE).from_buffer_copy(memory)
    cpu = first.Cpu(*entry_cpu)
    report = first.EgaReport()
    assert glyph_model(ram, first.RAM_SIZE, ctypes.byref(cpu),
                       ctypes.byref(report)) == 0
    compare_state("second glyph first EGA change", ram, cpu,
                  expected_ram, expected_cpu)
    assert expected_cpu[8:10] == (0x0709, 0x0415)
    assert (report.glyph_index, report.color, report.plane,
            report.stores, report.page_segment, report.page_offset) == (
                glyph, memory[first.physical(ss, sp + 10)] & 0xFF,
                (len(stores) - 1) // 2, len(stores), 0xA000,
                changed[0] - 0xA0000)
    assert changed[0] == first.physical(0xA000, 0x0143)
    assert [at for at in range(0xA0000, 0xB0000)
            if memory[at] != expected_ram[at]] == [changed[0]]
    return len(stores)


def main():
    raw = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    assert len(raw) == 65231 and sha256(raw).hexdigest() == copy_gate.EXE_SHA
    image, *_ = unpack(raw)
    body = image[0x23B6:0x2432]
    literal = image[0x23B3:0x23B6]
    helper = raw[0xB781:0xB79B]
    assert sha256(body).hexdigest() == suffix.BODY_SHA
    assert sha256(literal).hexdigest() == copy_gate.LITERAL_SHA
    assert sha256(helper).hexdigest() == copy_gate.HELPER_SHA
    assert sha256((ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
                  ).hexdigest() == first.OVR_SHA
    overlays = first.source()
    prefix = first.resident_prefix()
    printer, copy = first.original_bodies()
    drawer, upcase = first.original_extra()
    with tempfile.TemporaryDirectory(prefix="poolrad-next-glyph-",
                                     dir=ROOT / "resources") as tmp:
        library = Path(tmp) / "next_glyph.so"
        subprocess.run([
            "cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
            "-shared", "-fPIC", "-I", str(ROOT / "tools/overlay"),
            "-I", str(ROOT / "tools/replay"),
            str(ROOT / "tools/replay/resident_0709_064f_first_ega.c"),
            str(ROOT / "tools/replay/resident_00ba_suffix_glyph_control.c"),
            str(ROOT / "tools/replay/resident_00ba_suffix_next_glyph_ega.c"),
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
        loop_model = lib.pr_00ba_suffix_glyph_control
        loop_model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                               ctypes.POINTER(first.Cpu),
                               ctypes.POINTER(control.Report)]
        loop_model.restype = ctypes.c_int
        glyph_model = lib.pr_00ba_suffix_next_glyph_ega
        glyph_model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                                ctypes.POINTER(first.Cpu),
                                ctypes.POINTER(first.EgaReport)]
        glyph_model.restype = ctypes.c_int
        checked, stores = 0, set()
        for site, (code, _) in zip(first.SITES, overlays):
            if site[-1] == 0:
                continue
            for color in range(3):
                for ch in (0x54, 0x74):
                    _, _, stops = first.run_case(
                        site, code, ch, color, prefix, printer, copy,
                        drawer, upcase, glyph_entry, ega_change,
                        trace_return=True)
                    stores.add(check_case(stops, body, literal, helper,
                                          loop_model, glyph_model))
                    checked += 1
    assert checked == 12 and stores == {1, 3, 5}
    print("RES_00BA copied suffix second glyph: 12/12 original-byte/C "
          "first EGA changes; full 1MiB RAM and 14-field CPU parity; "
          "A000:0143, store counts", sorted(stores))


if __name__ == "__main__":
    main()
