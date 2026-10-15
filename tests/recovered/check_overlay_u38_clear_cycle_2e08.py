#!/usr/bin/env python3
"""Original-byte full-state parity for U38:+2E08 and private helper +0000."""

import ctypes
from hashlib import sha256
from pathlib import Path
import subprocess
import sys
import tempfile

from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE

import check_overlay_u38_target_select_101d as shared

ROOT = Path(__file__).resolve().parents[2]
BODY_SHA = "7142253f1af3c81015b05a5e1743c3cf19ecf776ec43da551cc00b4e07d374a1"
HELPER_SHA = "f62ad6b7474059693853b623f564cfd5c9298cd50e4628732b49b97eb6265240"
LATCHES = (0, 0x0B, 0x35, 0x34, 0x37, 0x58, 0xFF, 0x1E,
           1, 2, 0x80, 0x7F)
GATES = (0, 1, 0x80, 0xFF)
LAYOUTS = ((0x3000, 0xE000, 0x8000, 0xA5CD),
           (0x4200, 0xD000, 0x7FEC, 0x1E62),
           (0x2400, 0xA000, 0x9000, 0xFF80),
           (0x3500, 0xB000, 0x7010, 0x00FE))


def source():
    ovr = shared.OVR.read_bytes()
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    assert len(ovr) == 206105 and sha256(ovr).hexdigest() == shared.OVR_SHA
    assert len(exe) == 65231 and sha256(exe).hexdigest() == shared.EXE_SHA
    sys.path.insert(0, str(ROOT / "tools/overlay"))
    from overlay_inventory import find_units, parse_mz
    unit = next(u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)
                if u["stub_para"] == 0x38)
    assert unit["ovr_offset"] == 0xEC5B
    assert unit["entries"][109:111] == [0x2E08, 0x2E37]
    assert [off for off in unit["fixups"] if 0x2E08 <= off < 0x2E37] == []
    body = ovr[0x11A63:0x11A92]
    private = ovr[0xEC5B:0xEC7A]
    assert len(body) == 47 and sha256(body).hexdigest() == BODY_SHA
    assert len(private) == 31 and sha256(private).hexdigest() == HELPER_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x2E08))
    assert len(ins) == 23 and sum(i.size for i in ins) == len(body)
    assert [i.address for i in ins if i.mnemonic == "call"] == [
        0x2E0F, 0x2E16, 0x2E1D, 0x2E24]
    assert ins[-1].address == 0x2E34 and ins[-1].mnemonic == "retf"
    return body, private


def make_case(body, private, latch, gate, layout):
    ds, ss, sp, ax = layout
    ram = bytearray(shared.RAM_SIZE)
    for off, code in ((0x2E08, body), (0, private)):
        at = shared.physical(0xF000, off)
        ram[at:at + len(code)] = code
    cpu = shared.Cpu(ax, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE,
                     0x7777, sp, 0xF000, 0x2E08, ds, 0x1111, ss, 0x72D3)
    shared.put16(ram, ss, sp, 0x1234)
    shared.put16(ram, ss, (sp + 2) & 0xFFFF, 0xD000)
    for i in range(5):
        shared.put16(ram, ss, (sp + 4 + 2 * i) & 0xFFFF, 0xA100 + i)
    for off, value in ((0x6816, 0x12), (0x6817, latch),
                       (0x6818, 0xA7), (0x682A, gate)):
        ram[shared.physical(ds, off)] = value
    return ram, cpu


def compare(model, body, private, latch, gate, layout):
    memory, cpu = make_case(body, private, latch, gate, layout)
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, shared.RAM_SIZE)
    original.mem_write(0, bytes(memory))
    for (name, _), reg in zip(shared.Cpu._fields_, shared.REGISTERS):
        original.reg_write(reg, getattr(cpu, name))
    frames, returned = [], [False]
    helper_at = shared.physical(0xF000, 0)

    def on_code(emu, address, size, user):
        del size, user
        if address == helper_at:
            state = shared.Cpu(*(emu.reg_read(reg) & 0xFFFF
                                 for reg in shared.REGISTERS))
            snap = bytearray(emu.mem_read(0, shared.RAM_SIZE))
            frames.append(tuple(shared.get16(
                snap, state.ss, (state.sp + 2 * i) & 0xFFFF)
                for i in range(4)))
        elif address == shared.physical(0xD000, 0x1234):
            returned[0] = True
            emu.emu_stop()
        else:
            assert (0xF2E08 <= address < 0xF2E37 or
                    helper_at <= address < helper_at + len(private)), hex(address)

    original.hook_add(UC_HOOK_CODE, on_code)
    original.emu_start(shared.physical(0xF000, 0x2E08), shared.RAM_SIZE,
                       count=120)
    assert returned[0] and len(frames) == 4
    assert [x[0] for x in frames] == [0x2E12, 0x2E19, 0x2E20, 0x2E27]
    assert [x[2] & 255 for x in frames] == [0x0B, 0x35, 0x34, 0x37]
    expected_ram = bytes(original.mem_read(0, shared.RAM_SIZE))
    expected_cpu = tuple(original.reg_read(reg) & 0xFFFF
                         for reg in shared.REGISTERS)
    ram = (ctypes.c_uint8 * shared.RAM_SIZE).from_buffer_copy(memory)
    model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                      ctypes.POINTER(shared.Cpu)]
    model.restype = ctypes.c_int
    assert model(ram, shared.RAM_SIZE, ctypes.byref(cpu)) == 0
    assert bytes(ram) == expected_ram, (latch, gate, layout, "RAM")
    assert shared.snap(cpu) == expected_cpu, (
        latch, gate, layout, "CPU", shared.snap(cpu), expected_cpu)


def main():
    body, private = source()
    with tempfile.TemporaryDirectory(prefix="poolrad-u38-2e08-") as temp:
        library = Path(temp) / "model.so"
        subprocess.run(["cc", "-shared", "-fPIC", "-std=c11", "-Wall",
                        "-Wextra", "-Wpedantic", "-Werror", "-I",
                        str(ROOT / "include"),
                        str(ROOT / "src/recovered/overlay_u38_clear_cycle_2e08.c"),
                        "-o", str(library)], check=True)
        model = ctypes.CDLL(str(library)).pr_u38_clear_cycle_2e08
        for latch in LATCHES:
            for gate in GATES:
                for layout in LAYOUTS:
                    compare(model, body, private, latch, gate, layout)
    print("192 original-byte U38 +2E08/+0000 cases passed: full 1 MiB RAM, "
          "14 CPU fields, all four helper frames")


if __name__ == "__main__":
    main()
