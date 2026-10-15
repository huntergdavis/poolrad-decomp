#!/usr/bin/env python3
"""Original-byte/C second EGA glyph drawer and final printer return."""

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
    Cpu, RAM_SIZE, REGISTERS, SITES, make_case, physical, put16, read16, source,
    uc_values,
)
from check_resident_1816_overlay_printer_composition import (  # noqa: E402
    resident_prefix,
)
from check_resident_0709_0626_postcopy_control import (  # noqa: E402
    compare_cpu, original_bodies,
)
from resident_loader_map import unpack  # noqa: E402

OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
DRAWER_SHA = "a9d1c72241b6810c3e0995b03cca98edc56cb76734e948333e360d82ef7e1840"
UPCASE_SHA = "738c9a82c7faab6335f0952cc5d989d273cb85828a0735f34966220f370d012c"


def glyph_bodies():
    raw = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    image, *_ = unpack(raw)
    drawer = image[0x726F:0x767B]
    upcase = raw[0xC4A0:0xC4B3]
    assert len(drawer) == 1036 and sha256(drawer).hexdigest() == DRAWER_SHA
    assert sha256(upcase).hexdigest() == UPCASE_SHA
    return drawer, upcase


def case(site, code, color_case, prefix, printer, copy, drawer, upcase,
         drawer_model, printer_model):
    memory, initial, frame = make_case(site, code, 0)
    node_off, node_seg = frame[3:]
    for seg, off, body in ((0x00BA, 0x1816, prefix),
                           (0x0709, 0x0626, printer),
                           (0x0709, 0x01DF, drawer),
                           (0x0AF8, 0x0B25, copy),
                           (0x0AF8, 0x182A, upcase)):
        at = physical(seg, off)
        memory[at:at + len(body)] = body
    memory[physical(node_seg, node_off)] = 2
    memory[physical(node_seg, node_off + 1)] = 0x54
    second_char = (0x74, 0x62, 0x61, 0x74, 0x74)[color_case]
    memory[physical(node_seg, node_off + 2)] = second_char
    first, second = ((0, 0xA5), (7, 1), (7, 2),
                     (7, 2), (7, 2))[color_case]
    memory[physical(node_seg, node_off + 0x10D)] = first
    memory[physical(node_seg, node_off + 0x10E)] = second
    ds = initial.ds
    put16(memory, ds, 0x5E20, 0x1000)
    put16(memory, ds, 0x5E22, 0x3000)
    put16(memory, ds, 0x4A16, 0xA000)
    put16(memory, ds, 0x4A18, 0xA000)
    memory[physical(ds, 0x4A1E)] = 1
    memory[physical(ds, 0x25C9):physical(ds, 0x25C9) + 4] = bytes((8, 4, 2, 1))
    for glyph, rows in ((0x14, bytes((0x3F, 0x80, 0x40, 0x20,
                                      0x10, 0x08, 0x04, 0x02))),
                        (0x02, bytes((0xC3, 0x3C, 0xA5, 0x5A,
                                      0xFF, 0x81, 0x24, 0x18))),
                        (0x01, bytes((0x01, 0x02, 0x04, 0x08,
                                      0x10, 0x20, 0x40, 0x80)))):
        at = physical(0x3000, 0x1000 + glyph * 8)
        memory[at:at + 8] = rows

    emu = Uc(UC_ARCH_X86, UC_MODE_16)
    emu.mem_map(0, RAM_SIZE)
    emu.mem_write(0, bytes(memory))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        emu.reg_write(reg, getattr(initial, name))
    entries, snapshots, ega_writes = [], [], []

    def on_code(uc, address, size, user):
        del size, user
        if address == physical(0x0709, 0x01DF):
            entries.append(address)
            if len(entries) == 2:
                if color_case >= 3:
                    ss = uc.reg_read(REGISTERS[12])
                    sp = uc.reg_read(REGISTERS[7])
                    uc.mem_write(physical(ss, sp + 0x0A),
                                 bytes((color_case - 3,)))
                snapshots.append(("second-entry", bytes(uc.mem_read(0, RAM_SIZE)),
                                  uc_values(uc)))
        elif address == physical(0x0709, 0x0694) and len(entries) == 1:
            snapshots.append(("first-return", bytes(uc.mem_read(0, RAM_SIZE)),
                              uc_values(uc)))
        elif address == physical(0x0709, 0x0694) and len(entries) == 2:
            snapshots.append(("second-return", bytes(uc.mem_read(0, RAM_SIZE)),
                              uc_values(uc)))
        elif address == physical(0x00BA, 0x1858):
            snapshots.append(("printer-return", bytes(uc.mem_read(0, RAM_SIZE)),
                              uc_values(uc)))
            uc.emu_stop()

    def on_write(uc, access, address, size, value, user):
        del uc, access, user
        if len(entries) == 2 and 0xA0000 <= address < 0xB0000:
            ega_writes.append((address, size, value))

    emu.hook_add(UC_HOOK_CODE, on_code)
    emu.hook_add(UC_HOOK_MEM_WRITE, on_write)
    emu.emu_start(physical(initial.cs, site[3]), 0, count=10000)
    assert len(entries) == 2
    assert [x[0] for x in snapshots] == ["first-return", "second-entry",
                                      "second-return", "printer-return"]
    before_ram, before_cpu = snapshots[1][1:]
    after_ram, after_cpu = snapshots[2][1:]
    ss, bp, sp = before_cpu[12], before_cpu[6], before_cpu[7]
    glyph = (second_char - 0x20) & 0x3F
    color = read16(before_ram, ss, sp + 0x0A) & 255
    row = read16(before_ram, ss, sp + 0x0C) & 255
    col = read16(before_ram, ss, sp + 0x0E) & 255
    expected_writes = []
    for scanline in range(8):
        offset = row * 320 + col + scanline * 40
        font_byte = before_ram[physical(0x3000, 0x1000 + glyph * 8 + scanline)]
        for plane in range(4):
            mask = before_ram[physical(before_cpu[10], 0x25C9 + 3 - plane)]
            value = font_byte if mask & color else 0
            expected_writes.extend(((physical(0xA000, offset), 1, value),
                                    (physical(0xA000, offset), 1, value)))
    assert ega_writes == expected_writes, (site[1], color_case, "EGA stores")
    assert tuple(read16(before_ram, ss, sp + 2 * n) for n in range(5)) == (
        0x0694, 0x0709, 1, second_char,
        (snapshots[0][2][0] & 0xFF00) | 1)
    assert after_cpu[8:10] == (0x0709, 0x0694)
    assert snapshots[3][2][8:10] == (0x00BA, 0x1858)
    assert after_ram[physical(ss, sp + 0x0E)] == (
        before_ram[physical(ss, sp + 0x0E)] + 1) & 255

    ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(before_ram)
    cpu = Cpu(*before_cpu)
    assert drawer_model(ram, RAM_SIZE, ctypes.byref(cpu)) == 0
    assert bytes(ram) == after_ram, (site[1], color_case, "RAM")
    compare_cpu("second glyph return", tuple(getattr(cpu, name) for name, _ in
                                    Cpu._fields_), after_cpu)
    assert printer_model(ram, RAM_SIZE, ctypes.byref(cpu)) == 0
    assert bytes(ram) == snapshots[3][1], (site[1], color_case, "printer RAM")
    compare_cpu("final printer return", tuple(getattr(cpu, name) for name, _ in
                                         Cpu._fields_), snapshots[3][2])
    return snapshots


def main():
    assert sha256((ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
                  ).hexdigest() == OVR_SHA
    overlays = source()
    prefix = resident_prefix()
    printer, copy = original_bodies()
    drawer, upcase = glyph_bodies()
    with tempfile.TemporaryDirectory(prefix="poolrad-second-glyph-",
                                     dir=ROOT / "resources") as tmp:
        library = Path(tmp) / "second_glyph.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC", "-I",
                        str(ROOT / "tools/overlay"),
                        str(ROOT / "tools/replay/resident_0709_second_glyph_return.c"),
                        "-o", str(library)], check=True)
        lib = ctypes.CDLL(str(library))
        drawer_model = lib.pr_01df_second_glyph_return
        printer_model = lib.pr_0694_final_printer_return
        for model in (drawer_model, printer_model):
            model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                              ctypes.POINTER(Cpu)]
            model.restype = ctypes.c_int
        for site, (code, _) in zip(SITES, overlays):
            for color_case in range(5):
                case(site, code, color_case, prefix, printer, copy,
                     drawer, upcase, drawer_model, printer_model)
    print("RES_0709:01DF second glyph -> 0694 -> 00BA:1858: "
          "40/40 original-byte/C full-state cases passed; "
          "2560/2560 EGA stores checked")


if __name__ == "__main__":
    main()
