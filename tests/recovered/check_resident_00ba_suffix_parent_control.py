#!/usr/bin/env python3
"""Original-byte/C audit from second glyph return into two overlay parents."""

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

PARENTS = (
    (0x11D1, 0x11DD, 0x2220B, 12,
     "432394564917b2a0f34b170334812b65edaa9f35e871ae83a8dcf7e204b8f1c4"),
    (0x2ABB, 0x2AC8, 0x23AF5, 13,
     "4320bd8f150aed52c5a3ce6156203bf146f7e23cde60100405b2e96eecfc9de4"),
)


class ParentReport(ctypes.Structure):
    _fields_ = [("node_length", ctypes.c_uint8),
                ("pushed_length", ctypes.c_uint16),
                ("parent_ip", ctypes.c_uint16)]


def compare_state(label, ram, cpu, expected_ram, expected_cpu):
    assert bytes(ram) == expected_ram, (label, "RAM")
    compare_cpu(label, tuple(getattr(cpu, n) for n, _ in
                             first.Cpu._fields_), expected_cpu)


def second_return(stops, body, literal, helper, loop_model):
    first_ram, first_cpu = control.prepare_0694(stops, body, literal, helper)
    (entry_ram, entry_cpu), _ = copy_gate.run_to(
        first_ram, first_cpu, (0x0709, 0x01DF), 50)
    ss, ds, sp = entry_cpu[12], entry_cpu[10], entry_cpu[7]
    ch = control.word(entry_ram, ss, sp + 6) & 0xFF
    upper = ch - 0x20 if 0x61 <= ch <= 0x7A else ch
    glyph = upper % 0x40
    font_off = control.word(entry_ram, ds, 0x5E20)
    font_seg = control.word(entry_ram, ds, 0x5E22)
    memory = bytearray(entry_ram)
    memory[first.physical(font_seg, font_off + glyph * 8)] = suffix.FONT_BYTE
    (changed_ram, changed_cpu), _, changed = suffix.first_change(
        memory, entry_cpu)
    assert changed[0] == first.physical(0xA000, 0x0143)
    (glyph_ram, glyph_cpu), _ = copy_gate.run_to(
        changed_ram, changed_cpu, (0x0709, 0x0694), 10000)
    assert glyph_ram[first.physical(ss, glyph_cpu[6] - 0x102)] == 2
    (returned_ram, returned_cpu), _ = copy_gate.run_to(
        glyph_ram, glyph_cpu, (0x00BA, 0x188C), 50)
    ram = (ctypes.c_uint8 * first.RAM_SIZE).from_buffer_copy(glyph_ram)
    cpu = first.Cpu(*glyph_cpu)
    report = control.Report()
    assert loop_model(ram, first.RAM_SIZE, ctypes.byref(cpu),
                      ctypes.byref(report)) == 0
    assert report.stop == 2 and report.glyph_index_before == 2
    compare_state("second glyph printer return", ram, cpu,
                  returned_ram, returned_cpu)
    return returned_ram, returned_cpu


def check_parent(base_ram, base_cpu, site, model, overlay, length_override=None):
    parent_ip, stop_ip, file_at, span_size, digest = PARENTS[site]
    ss, display_bp = base_cpu[12], base_cpu[6]
    assert control.word(base_ram, ss, display_bp + 2) == parent_ip
    parent_cs = control.word(base_ram, ss, display_bp + 4)
    code = overlay[file_at:file_at + span_size]
    assert len(code) == span_size and sha256(code).hexdigest() == digest
    memory = bytearray(base_ram)
    at = first.physical(parent_cs, parent_ip)
    memory[at:at + span_size] = code
    (overlay_ram, overlay_cpu), _ = copy_gate.run_to(
        memory, base_cpu, (parent_cs, parent_ip), 5)
    ptr_seg = ss if site == 0 else overlay_cpu[10]
    ptr_off = (overlay_cpu[6] - 0x3F) & 0xFFFF if site == 0 else 0x5D92
    node_off = control.word(overlay_ram, ptr_seg, ptr_off)
    node_seg = control.word(overlay_ram, ptr_seg, ptr_off + 2)
    node_at = first.physical(node_seg, node_off)
    if length_override is not None:
        memory[node_at] = length_override
    length = memory[node_at]
    (expected_ram, expected_cpu), _ = copy_gate.run_to(
        memory, base_cpu, (parent_cs, stop_ip), 12)
    ram = (ctypes.c_uint8 * first.RAM_SIZE).from_buffer_copy(memory)
    cpu = first.Cpu(*base_cpu)
    report = ParentReport()
    assert model(site, ram, first.RAM_SIZE, ctypes.byref(cpu),
                 ctypes.byref(report)) == 0
    compare_state("suffix parent control", ram, cpu,
                  expected_ram, expected_cpu)
    assert (report.node_length, report.pushed_length,
            report.parent_ip) == (length, length + 4, parent_ip)
    assert expected_cpu[8:10] == (parent_cs, stop_ip)
    assert control.word(expected_ram, ss, expected_cpu[7]) == length + 4
    assert expected_cpu[7] == (overlay_cpu[7] - 2) & 0xFFFF


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
    overlay = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    assert sha256(overlay).hexdigest() == first.OVR_SHA
    overlays = first.source()
    prefix = first.resident_prefix()
    printer, copy = first.original_bodies()
    drawer, upcase = first.original_extra()
    with tempfile.TemporaryDirectory(prefix="poolrad-suffix-parent-",
                                     dir=ROOT / "resources") as tmp:
        library = Path(tmp) / "suffix_parent.so"
        subprocess.run([
            "cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
            "-shared", "-fPIC", "-I", str(ROOT / "tools/overlay"),
            "-I", str(ROOT / "tools/replay"),
            str(ROOT / "tools/replay/resident_0709_064f_first_ega.c"),
            str(ROOT / "tools/replay/resident_00ba_suffix_glyph_control.c"),
            str(ROOT / "tools/replay/resident_00ba_second_printer_return.c"),
            str(ROOT / "tools/replay/resident_00ba_suffix_parent_control.c"),
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
        model = lib.pr_00ba_suffix_parent_control
        model.argtypes = [ctypes.c_uint, ctypes.POINTER(ctypes.c_uint8),
                          ctypes.c_size_t, ctypes.POINTER(first.Cpu),
                          ctypes.POINTER(ParentReport)]
        model.restype = ctypes.c_int
        natural = variants = 0
        for site_index, site in enumerate((first.SITES[4], first.SITES[5])):
            code = overlays[4 + site_index][0]
            for color in range(3):
                for ch in (0x54, 0x74):
                    _, _, stops = first.run_case(
                        site, code, ch, color, prefix, printer, copy,
                        drawer, upcase, glyph_entry, ega_change,
                        trace_return=True)
                    ram, cpu = second_return(stops, body, literal, helper,
                                             loop_model)
                    check_parent(ram, cpu, site_index, model, overlay)
                    natural += 1
                    if ch == 0x54:
                        for length in (0x0F, 0xFC):
                            check_parent(ram, cpu, site_index, model, overlay,
                                         length_override=length)
                            variants += 1
    assert (natural, variants) == (12, 12)
    print("RES_00BA suffix parent control: 12/12 reached and 12/12 "
          "late-length variants; full 1MiB RAM and 14-field CPU parity "
          "through second-printer return and first parent push")


if __name__ == "__main__":
    main()
