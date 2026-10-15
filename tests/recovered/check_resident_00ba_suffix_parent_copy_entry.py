#!/usr/bin/env python3
"""Original-byte/C audit of two copied-suffix parent SYSTEM call frames."""

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
import check_resident_00ba_suffix_parent_control as parent  # noqa: E402
from check_resident_0709_0626_postcopy_control import compare_cpu  # noqa: E402
from resident_loader_map import unpack  # noqa: E402

FULL_SPANS = (
    (33, "6891457405da398cb810c30bf810ad0036545e2691a5de51cb31ad7328560a34"),
    (35, "bce60c40e40aaa7fd51cc52052b9c27040a3e1ccc91b8ddb813f38d91a80712b"),
)


class CopyFrame(ctypes.Structure):
    _fields_ = [(name, ctypes.c_uint16) for name in
                "return_ip return_cs source_offset source_segment "
                "destination_offset destination_segment color row column".split()]


def compare_state(label, ram, cpu, expected_ram, expected_cpu):
    assert bytes(ram) == expected_ram, (label, "RAM")
    compare_cpu(label, tuple(getattr(cpu, n) for n, _ in
                             first.Cpu._fields_), expected_cpu)


def check_case(base_ram, base_cpu, site, overlay, parent_model,
               copy_model, length_override=None):
    parent_ip, stop_ip, file_at, _, _ = parent.PARENTS[site]
    size, digest = FULL_SPANS[site]
    code = overlay[file_at:file_at + size]
    assert len(code) == size and sha256(code).hexdigest() == digest
    ss, display_bp, ds = base_cpu[12], base_cpu[6], base_cpu[10]
    parent_cs = control.word(base_ram, ss, display_bp + 4)
    assert control.word(base_ram, ss, display_bp + 2) == parent_ip
    memory = bytearray(base_ram)
    at = first.physical(parent_cs, parent_ip)
    memory[at:at + size] = code
    if length_override is not None:
        overlay_bp = control.word(memory, ss, display_bp)
        ptr_seg = ss if site == 0 else ds
        ptr_off = (overlay_bp - 0x3F) & 0xFFFF if site == 0 else 0x5D92
        node_off = control.word(memory, ptr_seg, ptr_off)
        node_seg = control.word(memory, ptr_seg, ptr_off + 2)
        memory[first.physical(node_seg, node_off)] = length_override

    (parent_ram, parent_cpu), _ = copy_gate.run_to(
        memory, base_cpu, (parent_cs, stop_ip), 12)
    ram = (ctypes.c_uint8 * first.RAM_SIZE).from_buffer_copy(memory)
    cpu = first.Cpu(*base_cpu)
    parent_report = parent.ParentReport()
    assert parent_model(site, ram, first.RAM_SIZE, ctypes.byref(cpu),
                        ctypes.byref(parent_report)) == 0
    compare_state("parent length push", ram, cpu, parent_ram, parent_cpu)

    (expected_ram, expected_cpu), _ = copy_gate.run_to(
        parent_ram, parent_cpu, (0x0AF8, 0x0B0B), 20)
    ram = (ctypes.c_uint8 * first.RAM_SIZE).from_buffer_copy(parent_ram)
    cpu = first.Cpu(*parent_cpu)
    frame = CopyFrame()
    assert copy_model(site, ram, first.RAM_SIZE, ctypes.byref(cpu),
                      ctypes.byref(frame)) == 0
    compare_state("parent SYSTEM copy entry", ram, cpu,
                  expected_ram, expected_cpu)
    words = tuple(control.word(expected_ram, ss, expected_cpu[7] + 2 * n)
                  for n in range(9))
    reported = tuple(getattr(frame, name) for name, _ in CopyFrame._fields_)
    assert reported == words
    ret = 0x11F2 if site == 0 else 0x2ADE
    source = 0x0EB2 if site == 0 else 0x2994
    destination = (parent_cpu[6] - (0x47 if site == 0 else 0x12A)) & 0xFFFF
    high = parent_cpu[0] & 0xFF00
    assert words == (ret, parent_cs, source, parent_cs, destination,
                     ss, high | 0x0A, high | 1,
                     parent_report.pushed_length)
    assert expected_cpu[7] == (parent_cpu[7] - 16) & 0xFFFF
    assert expected_cpu[13] == parent_cpu[13]


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
    with tempfile.TemporaryDirectory(prefix="poolrad-parent-copy-",
                                     dir=ROOT / "resources") as tmp:
        library = Path(tmp) / "parent_copy.so"
        subprocess.run([
            "cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
            "-shared", "-fPIC", "-I", str(ROOT / "tools/overlay"),
            "-I", str(ROOT / "tools/replay"),
            str(ROOT / "tools/replay/resident_0709_064f_first_ega.c"),
            str(ROOT / "tools/replay/resident_00ba_suffix_glyph_control.c"),
            str(ROOT / "tools/replay/resident_00ba_second_printer_return.c"),
            str(ROOT / "tools/replay/resident_00ba_suffix_parent_control.c"),
            str(ROOT / "tools/replay/resident_00ba_suffix_parent_copy_entry.c"),
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
        parent_model = lib.pr_00ba_suffix_parent_control
        parent_model.argtypes = [ctypes.c_uint, ctypes.POINTER(ctypes.c_uint8),
                                 ctypes.c_size_t, ctypes.POINTER(first.Cpu),
                                 ctypes.POINTER(parent.ParentReport)]
        parent_model.restype = ctypes.c_int
        copy_model = lib.pr_00ba_suffix_parent_copy_entry
        copy_model.argtypes = [ctypes.c_uint, ctypes.POINTER(ctypes.c_uint8),
                               ctypes.c_size_t, ctypes.POINTER(first.Cpu),
                               ctypes.POINTER(CopyFrame)]
        copy_model.restype = ctypes.c_int
        natural = variants = 0
        for site_index, site in enumerate((first.SITES[4], first.SITES[5])):
            code = overlays[4 + site_index][0]
            for color in range(3):
                for ch in (0x54, 0x74):
                    _, _, stops = first.run_case(
                        site, code, ch, color, prefix, printer, copy,
                        drawer, upcase, glyph_entry, ega_change,
                        trace_return=True)
                    ram, cpu = parent.second_return(
                        stops, body, literal, helper, loop_model)
                    check_case(ram, cpu, site_index, overlay,
                               parent_model, copy_model)
                    natural += 1
                    if ch == 0x54:
                        for length in (0x0F, 0xFC):
                            check_case(ram, cpu, site_index, overlay,
                                       parent_model, copy_model,
                                       length_override=length)
                            variants += 1
    assert (natural, variants) == (12, 12)
    print("RES_00BA suffix parent SYSTEM entry: 12/12 reached and 12/12 "
          "late-length variants; full 1MiB RAM and 14-field CPU parity; "
          "nine-word copy frame")


if __name__ == "__main__":
    main()
