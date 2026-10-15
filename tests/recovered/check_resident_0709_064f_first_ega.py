#!/usr/bin/env python3
"""Trace one synthetic glyph from the eight overlay routes to its first EGA write."""

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
from check_resident_1816_overlay_callers import (  # noqa: E402
    Cpu, RAM_SIZE, REGISTERS, SITES, make_case, physical, put16, source,
    uc_values,
)
from check_resident_1816_overlay_printer_composition import (  # noqa: E402
    resident_prefix,
)
from check_resident_0709_0626_postcopy_control import (  # noqa: E402
    compare_cpu, original_bodies,
)
from resident_loader_map import unpack  # noqa: E402

DRAWER_SHA = "8d71dd4c0162462f99aef0fbbd4cfee472702f330bca1e07b50f192ffd972aef"
UPCASE_SHA = "738c9a82c7faab6335f0952cc5d989d273cb85828a0735f34966220f370d012c"
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"


def original_extra():
    raw = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    image, *_ = unpack(raw)
    drawer = image[0x726F:0x767A]
    upcase = raw[0xC4A0:0xC4B3]
    assert len(drawer) == 1035 and sha256(drawer).hexdigest() == DRAWER_SHA
    assert len(upcase) == 19 and sha256(upcase).hexdigest() == UPCASE_SHA
    return drawer, upcase


class EgaReport(ctypes.Structure):
    _fields_ = [("glyph_index", ctypes.c_uint8),
                ("color", ctypes.c_uint8),
                ("plane", ctypes.c_uint8),
                ("stores", ctypes.c_uint8),
                ("page_offset", ctypes.c_uint16),
                ("page_segment", ctypes.c_uint16)]


def run_case(site, overlay_code, glyph, color_case, prefix, printer, copy,
             drawer, upcase, entry_step, change_step, trace_return=False):
    memory, initial, frame = make_case(site, overlay_code, 0)
    node_off, node_seg = frame[3:]
    for seg, off, code in ((0x00BA, 0x1816, prefix),
                           (0x0709, 0x0626, printer),
                           (0x0709, 0x01DF, drawer),
                           (0x0AF8, 0x0B25, copy),
                           (0x0AF8, 0x182A, upcase)):
        at = physical(seg, off)
        memory[at:at + len(code)] = code
    memory[physical(node_seg, node_off)] = 1
    memory[physical(node_seg, node_off + 1)] = glyph
    first, second, color, plane = ((0, 0xA5, 0x0C, 2),
                                    (7, 1, 0x0E, 1),
                                    (7, 2, 0x0B, 0))[color_case]
    memory[physical(node_seg, node_off + 0x10D)] = first
    memory[physical(node_seg, node_off + 0x10E)] = second
    ds = initial.ds
    put16(memory, ds, 0x5E20, 0x1000)
    put16(memory, ds, 0x5E22, 0x3000)
    put16(memory, ds, 0x4A16, 0xA000)
    put16(memory, ds, 0x4A18, 0xA000)
    memory[physical(ds, 0x4A1E)] = 1
    memory[physical(ds, 0x25C9):physical(ds, 0x25C9) + 4] = bytes((8, 4, 2, 1))
    memory[physical(0x3000, 0x1000 + 0x14 * 8)] = 0x3F

    emu = Uc(UC_ARCH_X86, UC_MODE_16)
    emu.mem_map(0, RAM_SIZE)
    emu.mem_write(0, bytes(memory))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        emu.reg_write(reg, getattr(initial, name))
    stops, writes, changes = [], [], []
    pending_before = []
    first_write_count = []

    def on_code(uc, address, size, user):
        del size, user
        for label, seg, off in (("guard", 0x0709, 0x064F),
                                ("drawer", 0x0709, 0x01DF)):
            if address == physical(seg, off):
                stops.append((label, bytes(uc.mem_read(0, RAM_SIZE)),
                              uc_values(uc)))
        if address == physical(0x0709, 0x0412):
            pending_before[:] = [("before", bytes(uc.mem_read(0, RAM_SIZE)),
                                  uc_values(uc))]
        if address == physical(0x0709, 0x0415) and changes and not first_write_count:
            stops.extend(pending_before)
            stops.append(("after", bytes(uc.mem_read(0, RAM_SIZE)),
                          uc_values(uc)))
            first_write_count.append(len(writes))
            if not trace_return:
                uc.emu_stop()
        if trace_return and address == physical(0x0709, 0x0694):
            stops.append(("glyph-return", bytes(uc.mem_read(0, RAM_SIZE)),
                          uc_values(uc)))
        if trace_return and address == physical(0x00BA, 0x1858):
            stops.append(("printer-return", bytes(uc.mem_read(0, RAM_SIZE)),
                          uc_values(uc)))
            uc.emu_stop()

    def on_write(uc, access, address, size, value, user):
        del access, user
        if 0xA0000 <= address < 0xB0000:
            writes.append((address, size, value))
            old = bytes(uc.mem_read(address, size))
            if old != value.to_bytes(size, "little"):
                changes.append((address, size, value, old,
                                uc.reg_read(REGISTERS[8]),
                                uc.reg_read(REGISTERS[9])))

    emu.hook_add(UC_HOOK_CODE, on_code)
    emu.hook_add(UC_HOOK_MEM_WRITE, on_write)
    emu.emu_start(physical(initial.cs, site[3]), 0, count=10000)
    names = [x[0] for x in stops]
    assert names[:4] == ["guard", "drawer", "before", "after"], names
    if trace_return:
        assert names[4:] == ["glyph-return", "printer-return"], names
    else:
        assert len(names) == 4, names
    assert changes and changes[0][1:3] == (1, 0x3F), changes[:2]
    assert changes[0][4:] == (0x0709, 0x0412), changes[0]
    changed = [at for at in range(0xA0000, 0xB0000)
               if stops[2][1][at] != stops[3][1][at]]
    assert changed == [changes[0][0]], (changed[:5], changes)
    assert first_write_count == [2 * plane + 1], first_write_count

    ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(stops[0][1])
    cpu = Cpu(*stops[0][2])
    assert entry_step(ram, RAM_SIZE, ctypes.byref(cpu)) == 0
    assert bytes(ram) == stops[1][1], (site[1], glyph, color, "entry RAM")
    compare_cpu("glyph entry", tuple(getattr(cpu, name) for name, _ in
                                     Cpu._fields_), stops[1][2])
    report = EgaReport()
    assert change_step(ram, RAM_SIZE, ctypes.byref(cpu),
                       ctypes.byref(report)) == 0
    assert bytes(ram) == stops[3][1], (site[1], glyph, color, "change RAM")
    compare_cpu("first change", tuple(getattr(cpu, name) for name, _ in
                                      Cpu._fields_), stops[3][2])
    assert (report.glyph_index, report.color, report.plane,
            report.stores, report.page_segment, report.page_offset) == (
                0x14, color, plane, first_write_count[0], 0xA000,
                changes[0][0] - 0xA0000)
    return changes[0][0], first_write_count[0], stops


def main():
    assert sha256((ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
                  ).hexdigest() == OVR_SHA
    overlays = source()
    prefix = resident_prefix()
    printer, copy = original_bodies()
    drawer, upcase = original_extra()
    with tempfile.TemporaryDirectory(prefix="poolrad-064f-ega-",
                                     dir=ROOT / "resources") as tmp:
        library = Path(tmp) / "first_ega.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC", "-I",
                        str(ROOT / "tools/overlay"),
                        str(ROOT / "tools/replay/resident_0709_064f_first_ega.c"),
                        "-o", str(library)], check=True)
        lib = ctypes.CDLL(str(library))
        entry_step = lib.pr_064f_first_glyph_entry
        entry_step.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                               ctypes.POINTER(Cpu)]
        entry_step.restype = ctypes.c_int
        change_step = lib.pr_064f_first_ega_change
        change_step.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                                ctypes.POINTER(Cpu), ctypes.POINTER(EgaReport)]
        change_step.restype = ctypes.c_int
        returned = 0
        first_addresses = {}
        for site, (code, _) in zip(SITES, overlays):
            for color_case in range(3):
                for glyph in (0x54, 0x74):
                    trace_return = site == SITES[0] and glyph == 0x54
                    address, stores, stops = run_case(
                        site, code, glyph, color_case, prefix, printer,
                        copy, drawer, upcase, entry_step, change_step,
                        trace_return)
                    if trace_return:
                        assert stops[-2][2][8:10] == (0x0709, 0x0694)
                        assert stops[-1][2][8:10] == (0x00BA, 0x1858)
                        returned += 1
                    key = (site[0], site[1])
                    assert first_addresses.setdefault(key, address) == address
                    assert stores == (5, 3, 1)[color_case]
        assert returned == 3
        assert len(first_addresses) == 8
        for (unit, off), address in first_addresses.items():
            print(f"{unit:03X}:+{off:04X} first mapped A000={address:05X}")
    print("RES_0709:064F to first EGA change: 48/48 original-byte/C "
          "full-state composed cases passed")
    print("Same-run original-byte glyph and printer return: 3/3 cases passed")


if __name__ == "__main__":
    main()
