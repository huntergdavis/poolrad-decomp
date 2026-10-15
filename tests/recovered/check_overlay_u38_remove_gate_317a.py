#!/usr/bin/env python3
"""Original-byte, full-state parity for U38 public entry 115 (+317A)."""

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
BODY_SHA = "6fea300fcdcd481e8db60540c7d32abbbf2d0102b2c806648b05a086782788ad"
GATES = (0, 1, 2, 0x17, 0x7F, 0x80, 0xFE, 0xFF)
LAYOUTS = ((0x3000, 0xE000, 0x8000, 0xB000),
           (0x4200, 0xD000, 0x7FEC, 0xA900),
           (0x2400, 0xA000, 0x9000, 0xC100),
           (0x3500, 0xB000, 0x7010, 0x9600))
HIGH_BYTES = (0x00, 0x1E, 0x42, 0x7F, 0xA5, 0xFF)


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
    assert unit["entries"][115:117] == [0x317A, 0x319B]
    assert [off for off in unit["fixups"] if 0x317A <= off < 0x319B] == [0x3195]
    body = ovr[0x11DD5:0x11DF6]
    assert len(body) == 33 and sha256(body).hexdigest() == BODY_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x317A))
    assert len(ins) == 15 and sum(i.size for i in ins) == len(body)
    assert [(i.address, i.mnemonic) for i in ins if i.mnemonic == "lcall"] == [
        (0x3192, "lcall")]
    assert ins[-1].address == 0x3198 and ins[-1].mnemonic == "retf"
    return body


def frame(ram, cpu):
    return tuple(shared.get16(ram, cpu.ss, (cpu.sp + 2 * i) & 0xFFFF)
                 for i in range(7))


def make_case(body, gate, layout, high):
    ds, ss, sp, loaded = layout
    ram = bytearray(shared.RAM_SIZE)
    for seg, off, code in ((0xF000, 0x317A, body),
                           (loaded, 0x002A, bytes((0xB8, 0xEF, 0xBE,
                                                   0xCA, 0x0A, 0x00)))):
        at = shared.physical(seg, off)
        ram[at:at + len(code)] = code
    shared.put16(ram, 0xF000, 0x3195, loaded)
    cpu = shared.Cpu((high << 8) | 0xCD, 0x3456, 0x5678, 0x789A,
                     0x9ABC, 0xBCDE, 0x7777, sp, 0xF000, 0x317A,
                     ds, 0x1111, ss, 0x72D3)
    shared.put16(ram, ss, sp, 0x1234)
    shared.put16(ram, ss, (sp + 2) & 0xFFFF, 0xD000)
    for i in range(5):
        shared.put16(ram, ss, (sp + 4 + 2 * i) & 0xFFFF, 0xA100 + i)
    shared.put16(ram, ss, (sp + 4) & 0xFFFF, 0xA100 | gate)
    return ram, cpu


def compare(model, callback_type, body, gate, layout, high):
    memory, cpu = make_case(body, gate, layout, high)
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, shared.RAM_SIZE)
    original.mem_write(0, bytes(memory))
    for (name, _), reg in zip(shared.Cpu._fields_, shared.REGISTERS):
        original.reg_write(reg, getattr(cpu, name))
    far, returned = [], [False]
    loaded_at = shared.physical(layout[3], 0x002A)

    def on_code(emu, address, size, user):
        del size, user
        if address == loaded_at:
            state = shared.Cpu(*(emu.reg_read(reg) & 0xFFFF
                                 for reg in shared.REGISTERS))
            snapshot = bytearray(emu.mem_read(0, shared.RAM_SIZE))
            far.append((shared.snap(state), frame(snapshot, state)))
        elif address == shared.physical(0xD000, 0x1234):
            returned[0] = True
            emu.emu_stop()
        else:
            assert (shared.physical(0xF000, 0x317A) <= address <
                    shared.physical(0xF000, 0x319B) or
                    loaded_at <= address < loaded_at + 6), hex(address)

    original.hook_add(UC_HOOK_CODE, on_code)
    original.emu_start(shared.physical(0xF000, 0x317A),
                       shared.RAM_SIZE, count=80)
    label = (gate, layout, high)
    assert returned[0] and len(far) == (gate != 0), label
    if far:
        assert far[0][1] == (0x3197, 0xF000, 0, 0,
                             (high << 8) | 0x17, 0xA103, 0xA104), label
    expected_ram = bytes(original.mem_read(0, shared.RAM_SIZE))
    expected_cpu = tuple(original.reg_read(reg) & 0xFFFF
                         for reg in shared.REGISTERS)
    ram = (ctypes.c_uint8 * shared.RAM_SIZE).from_buffer_copy(memory)
    actual_far = []

    @callback_type
    def callback(context, ptr, size, state_ptr):
        del context
        assert size == shared.RAM_SIZE
        state = state_ptr.contents
        actual_far.append((shared.snap(state), frame(ptr, state)))
        state.ax = 0xBEEF
        state.ip = shared.get16(ptr, state.ss, state.sp)
        state.cs = shared.get16(ptr, state.ss, (state.sp + 2) & 0xFFFF)
        state.sp = (state.sp + 14) & 0xFFFF
        return 1

    assert model(ram, shared.RAM_SIZE, ctypes.byref(cpu), callback, None) == 0
    assert actual_far == far, (label, "far frame", actual_far, far)
    if bytes(ram) != expected_ram:
        diff = [i for i, (a, b) in enumerate(zip(ram, expected_ram)) if a != b]
        raise AssertionError((label, "RAM", [hex(i) for i in diff[:12]]))
    assert shared.snap(cpu) == expected_cpu, (
        label, "CPU", shared.snap(cpu), expected_cpu)


def main():
    body = source()
    callback_type = ctypes.CFUNCTYPE(
        ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8),
        ctypes.c_size_t, ctypes.POINTER(shared.Cpu))
    with tempfile.TemporaryDirectory(prefix="poolrad-u38-317a-") as temp:
        library = Path(temp) / "model.so"
        subprocess.run(["cc", "-shared", "-fPIC", "-std=c11", "-Wall",
                        "-Wextra", "-Wpedantic", "-Werror", "-I",
                        str(ROOT / "include"),
                        str(ROOT / "src/recovered/overlay_u38_remove_gate_317a.c"),
                        "-o", str(library)], check=True)
        model = ctypes.CDLL(str(library)).pr_u38_remove_gate_317a
        model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(shared.Cpu), callback_type,
                          ctypes.c_void_p]
        model.restype = ctypes.c_int
        for gate in GATES:
            for layout in LAYOUTS:
                for high in HIGH_BYTES:
                    compare(model, callback_type, body, gate, layout, high)
    print("192 original-byte U38 +317A cases passed: full 1 MiB RAM, "
          "14 CPU fields, loaded far-call frame")


if __name__ == "__main__":
    main()
