#!/usr/bin/env python3
"""Original-byte/C audit of SYSTEM B0B on two reached parent frames."""

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
import check_resident_00ba_1887_copy_return as copy_gate  # noqa: E402
import check_resident_00ba_suffix_first_glyph as suffix  # noqa: E402
import check_resident_00ba_suffix_glyph_control as control  # noqa: E402
import check_resident_00ba_suffix_parent_control as parent  # noqa: E402
import check_resident_00ba_suffix_parent_copy_entry as entry  # noqa: E402
from check_resident_0709_0626_postcopy_control import compare_cpu  # noqa: E402
from resident_loader_map import unpack  # noqa: E402

SOURCE = (
    (0x21EEC, 0x0EB2, 5,
     "bfccb462af8b61ee8bf8229cfd4e656d3bd0de0494320f5878228356b8142e46"),
    (0x239CE, 0x2994, 7,
     "eb27091422364253f75ff44586270aa2f3b3683e3898409592d999a57243ac97"),
)
HELPER_IPS = (0x0B0B, 0x0B0C, 0x0B0E, 0x0B10, 0x0B14, 0x0B18,
              0x0B19, 0x0B1A, 0x0B1C, 0x0B1E, 0x0B20, 0x0B22)


class B0bReport(ctypes.Structure):
    _fields_ = [("before_ip", ctypes.c_uint16),
                ("after_ip", ctypes.c_uint16),
                ("source_length", ctypes.c_uint8),
                ("returned_to_parent", ctypes.c_uint8)]


def compare_state(label, ram, cpu, expected_ram, expected_cpu):
    assert bytes(ram) == expected_ram, (label, "RAM")
    compare_cpu(label, tuple(getattr(cpu, n) for n, _ in
                             first.Cpu._fields_), expected_cpu)


def trace_helper(memory, cpu, parent_cs, return_ip):
    emu = Uc(UC_ARCH_X86, UC_MODE_16)
    emu.mem_map(0, first.RAM_SIZE)
    emu.mem_write(0, bytes(memory))
    for value, reg in zip(cpu, first.REGISTERS):
        emu.reg_write(reg, value)
    stops = []
    helper_addresses = {first.physical(0x0AF8, ip) for ip in HELPER_IPS}
    target = first.physical(parent_cs, return_ip)

    def on_code(uc, address, size, user):
        del size, user
        repeat = (address == first.physical(0x0AF8, 0x0B1E) and stops and
                  stops[-1][1][9] == 0x0B1E)
        if (address in helper_addresses or address == target) and not repeat:
            stops.append((bytes(uc.mem_read(0, first.RAM_SIZE)),
                          first.uc_values(uc)))
        if address == target:
            uc.emu_stop()

    emu.hook_add(UC_HOOK_CODE, on_code)
    emu.emu_start(first.physical(cpu[8], cpu[9]), 0, count=100)
    assert len(stops) == 13, (len(stops), [hex(s[1][9]) for s in stops])
    assert tuple(s[1][9] for s in stops[:12]) == HELPER_IPS
    assert stops[-1][1][8:10] == (parent_cs, return_ip)
    return stops


def check_case(base_ram, base_cpu, site, overlay, parent_model,
               entry_model, b0b_model, zero_source=False):
    parent_ip, parent_stop, file_at, _, _ = parent.PARENTS[site]
    full_size, full_digest = entry.FULL_SPANS[site]
    source_file, source_off, source_len, source_digest = SOURCE[site]
    code = overlay[file_at:file_at + full_size]
    source = overlay[source_file:source_file + source_len + 1]
    assert len(code) == full_size and sha256(code).hexdigest() == full_digest
    assert len(source) == source_len + 1 and source[0] == source_len
    assert sha256(source).hexdigest() == source_digest
    ss, display_bp = base_cpu[12], base_cpu[6]
    parent_cs = control.word(base_ram, ss, display_bp + 4)
    assert control.word(base_ram, ss, display_bp + 2) == parent_ip
    memory = bytearray(base_ram)
    at = first.physical(parent_cs, parent_ip)
    memory[at:at + full_size] = code
    at = first.physical(parent_cs, source_off)
    memory[at:at + len(source)] = source
    (parent_ram, parent_cpu), _ = copy_gate.run_to(
        memory, base_cpu, (parent_cs, parent_stop), 12)
    ram = (ctypes.c_uint8 * first.RAM_SIZE).from_buffer_copy(memory)
    cpu = first.Cpu(*base_cpu)
    p_report = parent.ParentReport()
    assert parent_model(site, ram, first.RAM_SIZE, ctypes.byref(cpu),
                        ctypes.byref(p_report)) == 0
    compare_state("B0B parent length", ram, cpu, parent_ram, parent_cpu)

    (entry_ram, entry_cpu), _ = copy_gate.run_to(
        parent_ram, parent_cpu, (0x0AF8, 0x0B0B), 20)
    ram = (ctypes.c_uint8 * first.RAM_SIZE).from_buffer_copy(parent_ram)
    cpu = first.Cpu(*parent_cpu)
    frame = entry.CopyFrame()
    assert entry_model(site, ram, first.RAM_SIZE, ctypes.byref(cpu),
                       ctypes.byref(frame)) == 0
    compare_state("B0B actual nine-word entry", ram, cpu,
                  entry_ram, entry_cpu)
    words = tuple(control.word(entry_ram, ss, entry_cpu[7] + 2 * n)
                  for n in range(9))
    assert tuple(getattr(frame, n) for n, _ in entry.CopyFrame._fields_) == words

    memory = bytearray(entry_ram)
    if zero_source:
        memory[first.physical(parent_cs, source_off)] = 0
    actual_length = memory[first.physical(parent_cs, source_off)]
    return_ip = 0x11F2 if site == 0 else 0x2ADE
    stops = trace_helper(memory, entry_cpu, parent_cs, return_ip)
    assert stops[0][0] == bytes(memory)
    ram = (ctypes.c_uint8 * first.RAM_SIZE).from_buffer_copy(memory)
    cpu = first.Cpu(*entry_cpu)
    for step, expected in enumerate(stops[1:]):
        report = B0bReport()
        assert b0b_model(site, ram, first.RAM_SIZE, ctypes.byref(cpu),
                         ctypes.byref(report)) == 0
        compare_state(f"B0B instruction {step}", ram, cpu, *expected)
        assert report.before_ip == HELPER_IPS[step]
        assert report.after_ip == expected[1][9]
        assert report.source_length == actual_length
        assert report.returned_to_parent == (step == 11)
    assert bytes(ram)[first.physical(ss, words[4]):
                      first.physical(ss, words[4]) + actual_length + 1] == \
        bytes(ram)[first.physical(parent_cs, source_off):
                   first.physical(parent_cs, source_off) + actual_length + 1]
    assert cpu.sp == (entry_cpu[7] + 8) & 0xFFFF
    before = bytes(ram), tuple(getattr(cpu, n) for n, _ in first.Cpu._fields_)
    report = B0bReport(0xAA55, 0x55AA, 0xA5, 0x5A)
    assert b0b_model(site, ram, first.RAM_SIZE, ctypes.byref(cpu),
                     ctypes.byref(report)) == 2
    assert bytes(ram) == before[0]
    assert tuple(getattr(cpu, n) for n, _ in first.Cpu._fields_) == before[1]
    assert (report.before_ip, report.after_ip, report.source_length,
            report.returned_to_parent) == (0xAA55, 0x55AA, 0xA5, 0x5A)


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
    with tempfile.TemporaryDirectory(prefix="poolrad-parent-b0b-",
                                     dir=ROOT / "resources") as tmp:
        library = Path(tmp) / "parent_b0b.so"
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
        parent_model = lib.pr_00ba_suffix_parent_control
        parent_model.argtypes = [ctypes.c_uint, ctypes.POINTER(ctypes.c_uint8),
                                 ctypes.c_size_t, ctypes.POINTER(first.Cpu),
                                 ctypes.POINTER(parent.ParentReport)]
        parent_model.restype = ctypes.c_int
        entry_model = lib.pr_00ba_suffix_parent_copy_entry
        entry_model.argtypes = [ctypes.c_uint, ctypes.POINTER(ctypes.c_uint8),
                                ctypes.c_size_t, ctypes.POINTER(first.Cpu),
                                ctypes.POINTER(entry.CopyFrame)]
        entry_model.restype = ctypes.c_int
        b0b_model = lib.pr_00ba_suffix_parent_b0b_step
        b0b_model.argtypes = [ctypes.c_uint, ctypes.POINTER(ctypes.c_uint8),
                              ctypes.c_size_t, ctypes.POINTER(first.Cpu),
                              ctypes.POINTER(B0bReport)]
        b0b_model.restype = ctypes.c_int
        natural = zero = 0
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
                               parent_model, entry_model, b0b_model)
                    natural += 1
                    if ch == 0x54:
                        check_case(ram, cpu, site_index, overlay,
                                   parent_model, entry_model, b0b_model,
                                   zero_source=True)
                        zero += 1
    assert (natural, zero) == (12, 6)
    print("RES_00BA suffix parent SYSTEM B0B: 12/12 reached and 6/6 "
          "empty-source variants, 12 instruction checkpoints each; "
          "full 1MiB RAM and 14 CPU fields, preserved parent blocker")


if __name__ == "__main__":
    main()
