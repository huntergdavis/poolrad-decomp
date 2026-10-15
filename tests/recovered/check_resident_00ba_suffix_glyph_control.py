#!/usr/bin/env python3
"""Original-byte/C audit of the copied suffix's post-glyph loop edge."""

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
from check_resident_0709_0626_postcopy_control import compare_cpu  # noqa: E402
from resident_loader_map import unpack  # noqa: E402

TAIL_SHA = "a99211136bc3e92f64ad18457e57c760368383435486747c3a6a6bc6c850de6c"
LOOP_SHA = "39e75f859dc7aa6ce2df5f776a95cc6420c610d39f11386d7fa8e094d9d92f2b"


class Report(ctypes.Structure):
    _fields_ = [("stop", ctypes.c_int),
                ("glyph_index_before", ctypes.c_uint8),
                ("glyph_index_after", ctypes.c_uint8),
                ("total", ctypes.c_uint8),
                ("column_after", ctypes.c_uint8)]


def word(ram, seg, off):
    at = first.physical(seg, off)
    return ram[at] | ram[(at + 1) & (first.RAM_SIZE - 1)] << 8


def prepare_0694(stops, body, literal, helper):
    memory = bytearray(stops[5][1])
    initial = stops[5][2]
    assert initial[8:10] == (0x00BA, 0x1858)
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
    ss, bp, ds = guard_cpu[12], guard_cpu[6], guard_cpu[10]
    assert guard_ram[first.physical(ss, bp - 0x100)] == 2
    ch = guard_ram[first.physical(ss, bp - 0xFF)]
    upper = ch - 0x20 if 0x61 <= ch <= 0x7A else ch
    glyph = upper % 0x40
    font_off = word(guard_ram, ds, 0x5E20)
    font_seg = word(guard_ram, ds, 0x5E22)
    memory = bytearray(guard_ram)
    memory[first.physical(font_seg, font_off + glyph * 8)] = suffix.FONT_BYTE
    (changed_ram, changed_cpu), _, _ = suffix.first_change(memory,
                                                            guard_cpu)
    (return_ram, return_cpu), _ = copy_gate.run_to(
        changed_ram, changed_cpu, (0x0709, 0x0694), 10000)
    assert return_cpu[8:10] == (0x0709, 0x0694)
    assert return_ram[first.physical(ss, bp - 0x102)] == 1
    assert return_ram[first.physical(ss, bp - 0x103)] == 2
    return return_ram, return_cpu


def check_edge(ram_before, cpu_before, model, terminal=False,
               wrap_column=False):
    memory = bytearray(ram_before)
    ss, bp = cpu_before[12], cpu_before[6]
    old_column = memory[first.physical(ss, bp + 0x0E)]
    if terminal:
        memory[first.physical(ss, bp - 0x103)] = 1
    if wrap_column:
        memory[first.physical(ss, bp + 0x0E)] = 0xFF
        old_column = 0xFF
    target = (0x00BA, 0x188C) if terminal else (0x0709, 0x01DF)
    (expected_ram, expected_cpu), _ = copy_gate.run_to(
        memory, cpu_before, target, 50)
    ram = (ctypes.c_uint8 * first.RAM_SIZE).from_buffer_copy(memory)
    cpu = first.Cpu(*cpu_before)
    report = Report()
    assert model(ram, first.RAM_SIZE, ctypes.byref(cpu),
                 ctypes.byref(report)) == 0
    assert bytes(ram) == expected_ram, (terminal, wrap_column, "RAM")
    compare_cpu("suffix glyph control", tuple(getattr(cpu, n) for n, _ in
                                              first.Cpu._fields_), expected_cpu)
    assert expected_cpu[8:10] == target
    assert (report.stop, report.glyph_index_before,
            report.glyph_index_after, report.total,
            report.column_after) == (
                2 if terminal else 1, 1, 1 if terminal else 2,
                1 if terminal else 2, (old_column + 1) & 0xFF)
    if terminal:
        assert expected_cpu[7] == (bp + 16) & 0xFFFF
    else:
        assert expected_ram[first.physical(ss, bp - 0x102)] == 2
        frame_words = tuple(word(expected_ram, ss, expected_cpu[7] + 2 * n)
                            for n in range(8))
        high = cpu_before[0] & 0xFF00
        expected_words = (
            0x0694, 0x0709, 1,
            expected_ram[first.physical(ss, bp - 0xFE)], high | 1,
            high | expected_ram[first.physical(ss, bp + 0x0A)],
            high | expected_ram[first.physical(ss, bp + 0x0C)],
            high | ((old_column + 1) & 0xFF))
        assert frame_words == expected_words, (frame_words, expected_words)


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
    printer = image[0x76B6:0x7737]
    assert sha256(printer[0x0694 - 0x0626:]).hexdigest() == TAIL_SHA
    assert sha256(printer[0x066D - 0x0626:0x0694 - 0x0626]
                  ).hexdigest() == LOOP_SHA
    assert sha256((ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
                  ).hexdigest() == first.OVR_SHA
    overlays = first.source()
    prefix = first.resident_prefix()
    printer, copy = first.original_bodies()
    drawer, upcase = first.original_extra()
    with tempfile.TemporaryDirectory(prefix="poolrad-suffix-control-",
                                     dir=ROOT / "resources") as tmp:
        library = Path(tmp) / "suffix_control.so"
        subprocess.run([
            "cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
            "-shared", "-fPIC", "-I", str(ROOT / "tools/overlay"),
            str(ROOT / "tools/replay/resident_0709_064f_first_ega.c"),
            str(ROOT / "tools/replay/resident_00ba_suffix_glyph_control.c"),
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
        model = lib.pr_00ba_suffix_glyph_control
        model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(first.Cpu), ctypes.POINTER(Report)]
        model.restype = ctypes.c_int
        cont = done = wrap = 0
        for site, (code, _) in zip(first.SITES, overlays):
            if site[-1] == 0:
                continue
            for color in range(3):
                for ch in (0x54, 0x74):
                    _, _, stops = first.run_case(
                        site, code, ch, color, prefix, printer, copy,
                        drawer, upcase, glyph_entry, ega_change,
                        trace_return=True)
                    ram, cpu = prepare_0694(stops, body, literal, helper)
                    check_edge(ram, cpu, model)
                    cont += 1
                    check_edge(ram, cpu, model, terminal=True)
                    done += 1
                    if ch == 0x54:
                        check_edge(ram, cpu, model, wrap_column=True)
                        wrap += 1
    assert (cont, done, wrap) == (12, 12, 6)
    print("RES_00BA suffix glyph loop: 12/12 second-drawer entries, "
          "12/12 completed-count returns, 6/6 column-wrap entries; "
          "full 1MiB RAM and 14-field CPU parity")


if __name__ == "__main__":
    main()
