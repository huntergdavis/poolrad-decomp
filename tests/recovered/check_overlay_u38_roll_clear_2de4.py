#!/usr/bin/env python3
"""Original-byte full-state parity for U38:+2DE4 and private helper +0000."""

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
BODY_SHA = "7c0e900e0e89d330ef9f35ad0dcfa3550859a8eb46f4e1e311b40b585f591943"
HELPER_SHA = "f62ad6b7474059693853b623f564cfd5c9298cd50e4628732b49b97eb6265240"
RESULTS = (0, 1, 0x0B, 0x1E, 0x1F, 0x7F, 0x80, 0xFF)
LATCHES = (0, 0x0B, 0x35, 0x58, 0xFF, 0x1E)
LAYOUTS = ((0x3000, 0xE000, 0x8000, 0xB000, 0xA5CD),
           (0x4200, 0xD000, 0x7FEC, 0xA900, 0x1E62),
           (0x2400, 0xA000, 0x9000, 0xC100, 0xFF80),
           (0x3500, 0xB000, 0x7010, 0x9600, 0x00FE))


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
    assert unit["entries"][108:110] == [0x2DE4, 0x2E08]
    assert [off for off in unit["fixups"] if 0x2DE4 <= off < 0x2E08] == [0x2DF0]
    body = ovr[0x11A3F:0x11A63]
    helper = ovr[0xEC5B:0xEC7A]
    assert len(body) == 36 and sha256(body).hexdigest() == BODY_SHA
    assert len(helper) == 31 and sha256(helper).hexdigest() == HELPER_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x2DE4))
    assert len(ins) == 19 and sum(i.size for i in ins) == len(body)
    assert [(i.address, i.mnemonic) for i in ins if i.mnemonic in ("lcall", "call")] == [
        (0x2DED, "lcall"), (0x2DFA, "call"), (0x2E01, "call")]
    return body, helper


def make_case(body, private, result, latch, layout):
    ds, ss, sp, loaded, ax = layout
    ram = bytearray(shared.RAM_SIZE)
    for seg, off, code in ((0xF000, 0x2DE4, body),
                           (0xF000, 0x0000, private),
                           (loaded, 0x0048, bytes((0xB0, result, 0xCA, 4, 0)))):
        at = shared.physical(seg, off)
        ram[at:at + len(code)] = code
    shared.put16(ram, 0xF000, 0x2DF0, loaded)
    cpu = shared.Cpu(ax, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE,
                     0x7777, sp, 0xF000, 0x2DE4, ds, 0x1111, ss, 0x72D3)
    shared.put16(ram, ss, sp, 0x1234)
    shared.put16(ram, ss, (sp + 2) & 0xFFFF, 0xD000)
    for i in range(5):
        shared.put16(ram, ss, (sp + 4 + 2 * i) & 0xFFFF, 0xA100 + i)
    ram[shared.physical(ds, 0x6817)] = latch
    ram[shared.physical(ds, 0x6818)] = 0xA7
    return ram, cpu


def frame(ram, state, count):
    return tuple(shared.get16(ram, state.ss, (state.sp + 2 * i) & 0xFFFF)
                 for i in range(count))


def compare(model, callback_type, body, private, result, latch, layout):
    memory, cpu = make_case(body, private, result, latch, layout)
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, shared.RAM_SIZE)
    original.mem_write(0, bytes(memory))
    for (name, _), reg in zip(shared.Cpu._fields_, shared.REGISTERS):
        original.reg_write(reg, getattr(cpu, name))
    far, near, returned = [], [], [False]
    roll_at = shared.physical(layout[3], 0x0048)
    helper_at = shared.physical(0xF000, 0)

    def on_code(emu, address, size, user):
        del size, user
        if address in (roll_at, helper_at):
            state = shared.Cpu(*(emu.reg_read(reg) & 0xFFFF
                                 for reg in shared.REGISTERS))
            snapshot = bytearray(emu.mem_read(0, shared.RAM_SIZE))
            if address == roll_at:
                far.append((shared.snap(state), frame(snapshot, state, 4)))
            else:
                near.append((shared.snap(state), frame(snapshot, state, 4)))
        elif address == shared.physical(0xD000, 0x1234):
            returned[0] = True
            emu.emu_stop()
        else:
            assert (0xF2DE4 <= address < 0xF2E08 or
                    helper_at <= address < helper_at + len(private) or
                    roll_at <= address < roll_at + 5), hex(address)

    original.hook_add(UC_HOOK_CODE, on_code)
    original.emu_start(shared.physical(0xF000, 0x2DE4), shared.RAM_SIZE,
                       count=100)
    assert returned[0] and len(far) == 1
    assert len(near) == (2 if result <= 0x1E else 0)
    if near:
        assert [x[1][0] for x in near] == [0x2DFD, 0x2E04]
        assert [x[1][2] & 255 for x in near] == [0x0B, 0x35]
    expected_ram = bytes(original.mem_read(0, shared.RAM_SIZE))
    expected_cpu = tuple(emu_reg & 0xFFFF for emu_reg in
                         (original.reg_read(reg) for reg in shared.REGISTERS))
    ram = (ctypes.c_uint8 * shared.RAM_SIZE).from_buffer_copy(memory)
    actual_far = []

    @callback_type
    def roll_callback(context, ptr, size, state_ptr):
        del context
        assert size == shared.RAM_SIZE
        state = state_ptr.contents
        actual_far.append((shared.snap(state), frame(ptr, state, 4)))
        state.ax = (state.ax & 0xFF00) | result
        state.ip = shared.get16(ptr, state.ss, state.sp)
        state.cs = shared.get16(ptr, state.ss, (state.sp + 2) & 0xFFFF)
        state.sp = (state.sp + 8) & 0xFFFF
        return 1

    model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                      ctypes.POINTER(shared.Cpu), callback_type, ctypes.c_void_p]
    model.restype = ctypes.c_int
    assert model(ram, shared.RAM_SIZE, ctypes.byref(cpu), roll_callback, None) == 0
    assert actual_far == far, (result, latch, layout, "far frame")
    assert bytes(ram) == expected_ram, (result, latch, layout, "RAM")
    assert shared.snap(cpu) == expected_cpu, (
        result, latch, layout, "CPU", shared.snap(cpu), expected_cpu)


def main():
    body, private = source()
    with tempfile.TemporaryDirectory(prefix="poolrad-u38-2de4-") as temp:
        library = Path(temp) / "model.so"
        subprocess.run(["cc", "-shared", "-fPIC", "-std=c11", "-Wall",
                        "-Wextra", "-Wpedantic", "-Werror", "-I",
                        str(ROOT / "include"),
                        str(ROOT / "src/recovered/overlay_u38_roll_clear_2de4.c"),
                        "-o", str(library)], check=True)
        model = ctypes.CDLL(str(library)).pr_u38_roll_clear_2de4
        callback_type = ctypes.CFUNCTYPE(
            ctypes.c_int, ctypes.c_void_p,
            ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
            ctypes.POINTER(shared.Cpu))
        for result in RESULTS:
            for latch in LATCHES:
                for layout in LAYOUTS:
                    compare(model, callback_type, body, private,
                            result, latch, layout)
    print("192 original-byte U38 +2DE4/+0000 cases passed: full 1 MiB RAM, "
          "14 CPU fields, far frame and two near helper frames")


if __name__ == "__main__":
    main()
