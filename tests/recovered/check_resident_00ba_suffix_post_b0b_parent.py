#!/usr/bin/env python3
"""Original-byte/C audit of two parents immediately after SYSTEM B0B."""

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
import check_resident_00ba_suffix_parent_copy_entry as entry  # noqa: E402
import check_resident_00ba_suffix_parent_b0b as b0b  # noqa: E402
from check_resident_0709_0626_postcopy_control import compare_cpu  # noqa: E402
from resident_loader_map import unpack  # noqa: E402

NEXT_SPANS = (
    (0x2222C, 5,
     "04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23"),
    (0x23B18, 10,
     "daedd70c0a61144728ecef03d9240f2c546f0a0fae1451c338417f078b51cde7"),
)


class NextReport(ctypes.Structure):
    _fields_ = [(name, ctypes.c_uint16) for name in
                "retained_offset retained_segment next_offset next_segment "
                "return_ip callee_ip callee_cs".split()]


def compare_state(label, ram, cpu, expected_ram, expected_cpu):
    assert bytes(ram) == expected_ram, (label, "RAM")
    compare_cpu(label, tuple(getattr(cpu, n) for n, _ in
                             first.Cpu._fields_), expected_cpu)


def check_case(base_ram, base_cpu, site, overlay, b0b_model, next_model):
    parent_ip, _, file_at, _, _ = parent.PARENTS[site]
    full_size, full_digest = entry.FULL_SPANS[site]
    source_file, source_off, source_len, source_digest = b0b.SOURCE[site]
    next_file, next_size, next_digest = NEXT_SPANS[site]
    code = overlay[file_at:file_at + full_size]
    source = overlay[source_file:source_file + source_len + 1]
    successor = overlay[next_file:next_file + next_size]
    assert len(code) == full_size and sha256(code).hexdigest() == full_digest
    assert len(source) == source_len + 1 and source[0] == source_len
    assert sha256(source).hexdigest() == source_digest
    assert len(successor) == next_size and \
        sha256(successor).hexdigest() == next_digest
    ss, display_bp = base_cpu[12], base_cpu[6]
    parent_cs = control.word(base_ram, ss, display_bp + 4)
    assert control.word(base_ram, ss, display_bp + 2) == parent_ip
    memory = bytearray(base_ram)
    at = first.physical(parent_cs, parent_ip)
    memory[at:at + full_size] = code
    at = first.physical(parent_cs, source_off)
    memory[at:at + len(source)] = source
    at = first.physical(parent_cs, 0x11F2 if site == 0 else 0x2ADE)
    memory[at:at + next_size] = successor
    (entry_ram, entry_cpu), _ = copy_gate.run_to(
        memory, base_cpu, (0x0AF8, 0x0B0B), 30)
    return_ip = 0x11F2 if site == 0 else 0x2ADE
    (returned_ram, returned_cpu), _ = copy_gate.run_to(
        entry_ram, entry_cpu, (parent_cs, return_ip), 100)
    ram = (ctypes.c_uint8 * first.RAM_SIZE).from_buffer_copy(entry_ram)
    cpu = first.Cpu(*entry_cpu)
    for _ in range(12):
        report = b0b.B0bReport()
        assert b0b_model(site, ram, first.RAM_SIZE, ctypes.byref(cpu),
                         ctypes.byref(report)) == 0
    compare_state("natural B0B return", ram, cpu, returned_ram,
                  returned_cpu)
    assert returned_cpu[8:10] == (parent_cs, return_ip)
    retained = (returned_cpu[6] - (0x47 if site == 0 else 0x12A)) & 0xFFFF
    assert (control.word(returned_ram, ss, returned_cpu[7]),
            control.word(returned_ram, ss, returned_cpu[7] + 2)) == (
                retained, ss)
    assert returned_ram[first.physical(ss, retained):
                        first.physical(ss, retained) + source_len + 1] == source

    target = (0x0709, 0x0626) if site == 0 else (0x0AF8, 0x0B98)
    (expected_ram, expected_cpu), _ = copy_gate.run_to(
        returned_ram, returned_cpu, target, 10)
    ram = (ctypes.c_uint8 * first.RAM_SIZE).from_buffer_copy(returned_ram)
    cpu = first.Cpu(*returned_cpu)
    report = NextReport()
    assert next_model(site, ram, first.RAM_SIZE, ctypes.byref(cpu),
                      ctypes.byref(report)) == 0
    compare_state("post-B0B parent call entry", ram, cpu,
                  expected_ram, expected_cpu)
    expected_return = 0x11F7 if site == 0 else 0x2AE8
    next_off = retained if site == 0 else (returned_cpu[6] - 0x2A) & 0xFFFF
    assert (report.retained_offset, report.retained_segment,
            report.next_offset, report.next_segment,
            report.return_ip, report.callee_ip, report.callee_cs) == (
                retained, ss, next_off, ss, expected_return, target[1], target[0])
    words = tuple(control.word(expected_ram, ss, expected_cpu[7] + 2 * n)
                  for n in range(7 if site == 0 else 9))
    old_args = tuple(control.word(returned_ram, ss, returned_cpu[7] + 2 * n)
                     for n in range(5))
    expected_words = ((expected_return, parent_cs) + old_args if site == 0
                      else (expected_return, parent_cs, next_off, ss) +
                      old_args)
    assert words == expected_words
    assert expected_cpu[7] == (returned_cpu[7] - (4 if site == 0 else 8)) & 0xFFFF
    assert expected_cpu[13] == returned_cpu[13]


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
    with tempfile.TemporaryDirectory(prefix="poolrad-post-b0b-",
                                     dir=ROOT / "resources") as tmp:
        library = Path(tmp) / "post_b0b.so"
        subprocess.run([
            "cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
            "-shared", "-fPIC", "-I", str(ROOT / "tools/overlay"),
            "-I", str(ROOT / "tools/replay"),
            "-I", str(ROOT / "include"),
            str(ROOT / "tools/replay/resident_0709_064f_first_ega.c"),
            str(ROOT / "tools/replay/resident_00ba_suffix_glyph_control.c"),
            str(ROOT / "tools/replay/resident_00ba_second_printer_return.c"),
            str(ROOT / "tools/replay/resident_00ba_suffix_parent_control.c"),
            str(ROOT / "tools/replay/resident_00ba_suffix_parent_copy_entry.c"),
            str(ROOT / "tools/replay/resident_00ba_suffix_parent_b0b.c"),
            str(ROOT / "tools/replay/resident_00ba_suffix_post_b0b_parent.c"),
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
        loop_model = lib.pr_00ba_suffix_glyph_control
        loop_model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                               ctypes.POINTER(first.Cpu),
                               ctypes.POINTER(control.Report)]
        loop_model.restype = ctypes.c_int
        b0b_model = lib.pr_00ba_suffix_parent_b0b_step
        b0b_model.argtypes = [ctypes.c_uint, ctypes.POINTER(ctypes.c_uint8),
                              ctypes.c_size_t, ctypes.POINTER(first.Cpu),
                              ctypes.POINTER(b0b.B0bReport)]
        b0b_model.restype = ctypes.c_int
        next_model = lib.pr_00ba_suffix_post_b0b_parent
        next_model.argtypes = [ctypes.c_uint, ctypes.POINTER(ctypes.c_uint8),
                               ctypes.c_size_t, ctypes.POINTER(first.Cpu),
                               ctypes.POINTER(NextReport)]
        next_model.restype = ctypes.c_int
        checked = 0
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
                               b0b_model, next_model)
                    checked += 1
    assert checked == 12
    print("RES_00BA post-B0B parents: 12/12 natural helper-return frames "
          "and immediate next-call entries; full 1MiB RAM and 14 CPU "
          "fields, retained destination and next call frame")


if __name__ == "__main__":
    main()
