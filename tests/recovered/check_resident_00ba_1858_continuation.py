#!/usr/bin/env python3
"""Original-byte/C display suffix after first printer return at 00BA:1858."""

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
from check_resident_0709_0626_postcopy_control import compare_cpu  # noqa: E402
from resident_loader_map import unpack  # noqa: E402

BODY_SHA = "645caa0e81651dc9b2be1f156ea343d47f83a195f64f84a5c6fd828a89f50175"


def word(ram, seg, off):
    at = first.physical(seg, off)
    return ram[at] | ram[at + 1] << 8


def full_body():
    raw = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    image, *_ = unpack(raw)
    body = image[0x23B6:0x2432]
    assert len(body) == 124 and sha256(body).hexdigest() == BODY_SHA
    return body


def check_tail(stops, body, model, late_length=False):
    memory = bytearray(stops[5][1])
    initial = stops[5][2]
    assert initial[8:10] == (0x00BA, 0x1858)
    at = first.physical(0x00BA, 0x1816)
    memory[at:at + len(body)] = body
    ss, bp = initial[12], initial[6]
    flag = memory[first.physical(ss, bp + 6)]
    if late_length:
        assert flag != 0
        node_off = word(memory, ss, bp + 0x0C)
        node_seg = word(memory, ss, bp + 0x0E)
        memory[first.physical(node_seg, node_off)] = 2
        memory[first.physical(ss, bp + 0x0A)] = 0xFF
    if flag == 0:
        target = first.physical(word(memory, ss, bp + 4),
                                word(memory, ss, bp + 2))
    else:
        target = first.physical(0x0AF8, 0x0B0B)

    emu = Uc(UC_ARCH_X86, UC_MODE_16)
    emu.mem_map(0, first.RAM_SIZE)
    emu.mem_write(0, bytes(memory))
    for value, reg in zip(initial, first.REGISTERS):
        emu.reg_write(reg, value)
    expected = []

    def on_code(uc, address, size, user):
        del size, user
        if address == target:
            expected.append((bytes(uc.mem_read(0, first.RAM_SIZE)),
                             first.uc_values(uc)))
            uc.emu_stop()

    emu.hook_add(UC_HOOK_CODE, on_code)
    emu.emu_start(first.physical(0x00BA, 0x1858), 0, count=100)
    assert len(expected) == 1, (flag, "missing original stop")
    ram = (ctypes.c_uint8 * first.RAM_SIZE).from_buffer_copy(memory)
    cpu = first.Cpu(*initial)
    assert model(ram, first.RAM_SIZE, ctypes.byref(cpu)) == 0
    assert bytes(ram) == expected[0][0], (flag, late_length, "RAM")
    compare_cpu("display 1858 continuation",
                tuple(getattr(cpu, name) for name, _ in first.Cpu._fields_),
                expected[0][1])
    if flag == 0:
        assert first.physical(cpu.cs, cpu.ip) == target
    else:
        assert (cpu.cs, cpu.ip) == (0x0AF8, 0x0B0B)
        assert tuple(word(ram, ss, cpu.sp + 2 * n) for n in range(6)) == (
            0x1887, 0x00BA, 0x1813, 0x00BA, (bp - 4) & 0xFFFF, ss)
        if late_length:
            assert word(ram, ss, cpu.sp + 16) == 0x0101
            assert cpu.dx == 2
    return flag


def main():
    assert sha256((ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
                  ).hexdigest() == first.OVR_SHA
    overlays = first.source()
    prefix = first.resident_prefix()
    printer, copy = first.original_bodies()
    drawer, upcase = first.original_extra()
    body = full_body()
    with tempfile.TemporaryDirectory(prefix="poolrad-1858-",
                                     dir=ROOT / "resources") as tmp:
        library = Path(tmp) / "display_tail.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC", "-I",
                        str(ROOT / "tools/overlay"),
                        str(ROOT / "tools/replay/resident_0709_064f_first_ega.c"),
                        str(ROOT / "tools/replay/resident_00ba_1858_continuation.c"),
                        "-o", str(library)], check=True)
        lib = ctypes.CDLL(str(library))
        entry = lib.pr_064f_first_glyph_entry
        entry.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(first.Cpu)]
        entry.restype = ctypes.c_int
        change = lib.pr_064f_first_ega_change
        change.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                           ctypes.POINTER(first.Cpu),
                           ctypes.POINTER(first.EgaReport)]
        change.restype = ctypes.c_int
        model = lib.pr_1858_display_continuation
        model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(first.Cpu)]
        model.restype = ctypes.c_int
        zero = nonzero = late = 0
        for site, (code, _) in zip(first.SITES, overlays):
            for color in range(3):
                for glyph in (0x54, 0x74):
                    _, _, stops = first.run_case(
                        site, code, glyph, color, prefix, printer, copy,
                        drawer, upcase, entry, change, trace_return=True)
                    flag = check_tail(stops, body, model)
                    zero += flag == 0
                    nonzero += flag != 0
                    if flag != 0 and glyph == 0x54:
                        check_tail(stops, body, model, late_length=True)
                        late += 1
    assert (zero, nonzero, late) == (36, 12, 6)
    print("RES_00BA:1858 flag split: 36/36 overlay returns, "
          "12/12 SYSTEM copy entries, 6/6 late-read variants; "
          "full original-byte/C RAM and CPU parity")


if __name__ == "__main__":
    main()
