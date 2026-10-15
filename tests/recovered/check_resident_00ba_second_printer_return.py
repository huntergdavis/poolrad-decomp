#!/usr/bin/env python3
"""Audit the 1887 second-printer call frame and 188C display return."""

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
from check_resident_0709_0626_postcopy_control import compare_cpu  # noqa: E402
from resident_loader_map import unpack  # noqa: E402

TAIL_SHA = "bef4975c763f243ac454d60a3875242fccb942dc5646440e9f47e6d9a2fb2920"


class Frame(ctypes.Structure):
    _fields_ = [(name, ctypes.c_uint16) for name in
                "destination_offset destination_segment color row column".split()]


def word(ram, seg, off):
    at = first.physical(seg, off)
    return ram[at] | ram[(at + 1) & (first.RAM_SIZE - 1)] << 8


def check_case(stops, body, literal, helper, continuation, copy_return,
               printer_entry, display_return, empty_source=False):
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
    c_ram, c_cpu = copy_gate.call(continuation, memory, initial)
    assert c_ram == copy_ram, "1858 to SYSTEM RAM"
    compare_cpu("1858 to SYSTEM", c_cpu, copy_cpu)
    if empty_source:
        copy_ram = bytearray(copy_ram)
        copy_ram[first.physical(0x00BA, 0x1813)] = 0
    (call_ram, call_cpu), _ = copy_gate.run_to(
        copy_ram, copy_cpu, (0x00BA, 0x1887), 30)
    c_ram, c_cpu = copy_gate.call(copy_return, copy_ram, copy_cpu)
    assert c_ram == call_ram, "SYSTEM to 1887 RAM"
    compare_cpu("SYSTEM to 1887", c_cpu, call_cpu)

    (printer_ram, printer_cpu), _ = copy_gate.run_to(
        call_ram, call_cpu, (0x0709, 0x0626), 5)
    frame = Frame()
    ram = (ctypes.c_uint8 * first.RAM_SIZE).from_buffer_copy(call_ram)
    cpu = first.Cpu(*call_cpu)
    assert printer_entry(ram, first.RAM_SIZE, ctypes.byref(cpu),
                         ctypes.byref(frame)) == 0
    assert bytes(ram) == printer_ram, "1887 call RAM"
    compare_cpu("1887 call", tuple(getattr(cpu, n) for n, _ in
                                   first.Cpu._fields_), printer_cpu)
    assert printer_cpu[8:10] == (0x0709, 0x0626)
    assert printer_cpu[7] == (call_cpu[7] - 4) & 0xFFFF
    assert (word(printer_ram, ss, printer_cpu[7]),
            word(printer_ram, ss, printer_cpu[7] + 2)) == (0x188C, 0x00BA)
    assert (frame.destination_offset, frame.destination_segment,
            frame.color, frame.row, frame.column) == tuple(
                word(call_ram, ss, call_cpu[7] + 2 * n) for n in range(5))
    assert frame.destination_offset == (bp - 4) & 0xFFFF
    assert frame.destination_segment == ss
    assert frame.color == call_ram[first.physical(ss, bp - 1)]
    assert frame.row == call_ram[first.physical(ss, bp + 8)]

    (returned_ram, returned_cpu), _ = copy_gate.run_to(
        printer_ram, printer_cpu, (0x00BA, 0x188C), 100000)
    assert returned_cpu[7] == (bp - 4) & 0xFFFF
    assert returned_ram[first.physical(ss, bp - 4)] == (0 if empty_source else 2)
    overlay = (word(returned_ram, ss, bp + 4),
               word(returned_ram, ss, bp + 2))
    (expected_ram, expected_cpu), _ = copy_gate.run_to(
        returned_ram, returned_cpu, overlay, 5)
    actual_ram, actual_cpu = copy_gate.call(
        display_return, returned_ram, returned_cpu)
    assert actual_ram == expected_ram, "188C return RAM"
    compare_cpu("188C return", actual_cpu, expected_cpu)
    assert actual_cpu[8:10] == overlay
    assert actual_cpu[7] == (bp + 16) & 0xFFFF


def main():
    raw = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    assert len(raw) == 65231 and sha256(raw).hexdigest() == copy_gate.EXE_SHA
    image, *_ = unpack(raw)
    body = image[0x23B6:0x2432]
    assert len(body) == 124 and sha256(body).hexdigest() == \
        "645caa0e81651dc9b2be1f156ea343d47f83a195f64f84a5c6fd828a89f50175"
    tail = image[0x23B6 + 0x1887 - 0x1816:0x2432]
    assert len(tail) == 11 and sha256(tail).hexdigest() == TAIL_SHA
    literal = image[0x23B3:0x23B6]
    helper = raw[0xB781:0xB79B]
    assert sha256(literal).hexdigest() == copy_gate.LITERAL_SHA
    assert sha256(helper).hexdigest() == copy_gate.HELPER_SHA
    assert sha256((ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
                  ).hexdigest() == first.OVR_SHA
    overlays = first.source()
    prefix = first.resident_prefix()
    printer, copy = first.original_bodies()
    drawer, upcase = first.original_extra()
    with tempfile.TemporaryDirectory(prefix="poolrad-second-printer-",
                                     dir=ROOT / "resources") as tmp:
        library = Path(tmp) / "second_printer.so"
        subprocess.run([
            "cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
            "-shared", "-fPIC", "-I", str(ROOT / "tools/overlay"),
            "-I", str(ROOT / "include"),
            str(ROOT / "tools/replay/resident_0709_064f_first_ega.c"),
            str(ROOT / "tools/replay/resident_00ba_1858_continuation.c"),
            str(ROOT / "tools/replay/resident_00ba_1887_copy_return.c"),
            str(ROOT / "tools/replay/resident_00ba_second_printer_return.c"),
            str(ROOT / "src/replay/slot_a_saved_copy_1965.c"),
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
        continuation = lib.pr_1858_display_continuation
        copy_return = lib.pr_1887_display_copy_return
        display_return = lib.pr_188c_display_return
        for model in (continuation, copy_return, display_return):
            model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                              ctypes.POINTER(first.Cpu)]
            model.restype = ctypes.c_int
        printer_entry = lib.pr_1887_second_printer_entry
        printer_entry.argtypes = [ctypes.POINTER(ctypes.c_uint8),
                                  ctypes.c_size_t, ctypes.POINTER(first.Cpu),
                                  ctypes.POINTER(Frame)]
        printer_entry.restype = ctypes.c_int
        baseline = empty = 0
        for site, (code, _) in zip(first.SITES, overlays):
            if site[-1] == 0:
                continue
            for color in range(3):
                for glyph in (0x54, 0x74):
                    _, _, stops = first.run_case(
                        site, code, glyph, color, prefix, printer, copy,
                        drawer, upcase, glyph_entry, ega_change,
                        trace_return=True)
                    check_case(stops, body, literal, helper, continuation,
                               copy_return, printer_entry, display_return)
                    baseline += 1
                    if glyph == 0x54:
                        check_case(stops, body, literal, helper, continuation,
                                   copy_return, printer_entry, display_return,
                                   empty_source=True)
                        empty += 1
    assert (baseline, empty) == (12, 6)
    print("RES_00BA second printer: 12/12 literal and 6/6 empty-source "
          "call/return gates; full 1MiB RAM and 14-field CPU parity at "
          "entry and overlay return")


if __name__ == "__main__":
    main()
