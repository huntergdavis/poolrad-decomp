#!/usr/bin/env python3
"""Full-state original-byte parity for GAME.OVR U38 entry 103."""

import ctypes
from hashlib import sha256
from pathlib import Path
import subprocess
import sys
import tempfile

from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE

import check_overlay_u38_target_select_101d as helper

ROOT = Path(__file__).resolve().parents[2]
BODY_SHA = "8cdf5fe20286cd39394d2b2e5ed4dffa01a318fb10a62b35548bb19ff9a0d323"
PROFILES = ((1, 1, 4), (0, 1, 4), (0, 0, 3),
            (0, 0, 4), (0, 0x80, 0x7F), (0, 0x80, 0x80))


def source():
    ovr = helper.OVR.read_bytes()
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    assert len(ovr) == 206105 and sha256(ovr).hexdigest() == helper.OVR_SHA
    assert len(exe) == 65231 and sha256(exe).hexdigest() == helper.EXE_SHA
    sys.path.insert(0, str(ROOT / "tools/overlay"))
    from overlay_inventory import find_units, parse_mz
    unit = next(u for u in find_units(exe[parse_mz(exe)["header_bytes"]:],
                                    ovr) if u["stub_para"] == 0x38)
    assert unit["ovr_offset"] == 0xEC5B
    assert unit["entries"][103:105] == [0x2B7D, 0x2BC8]
    assert [i for i, off in enumerate(unit["entries"]) if off == 0x2B7D] == [103]
    assert not any(0x2B7D <= off < 0x2BC8 for off in unit["fixups"])
    body = ovr[0x117D8:0x11823]
    assert len(body) == 75 and sha256(body).hexdigest() == BODY_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x2B7D))
    assert len(ins) == 25 and sum(i.size for i in ins) == len(body)
    assert [(i.address, i.op_str) for i in ins if i.mnemonic == "call"] == [
        (0x2B8C, "0x101d")]
    assert ins[-1].address == 0x2BC5 and ins[-1].mnemonic == "retf"
    return body


def make_case(wrapper, callee, label):
    mode, profile, ds, ss, sp, loaded_segment = label
    target_byte, kind, value = PROFILES[profile]
    ram = bytearray(helper.RAM_SIZE)
    for off, body in ((0x101D, callee), (0x2B7D, wrapper)):
        at = helper.physical(0xF000, off)
        ram[at:at + len(body)] = body
    helper.put16(ram, 0xF000, 0x1048, loaded_segment)
    cpu = helper.Cpu(0xA5CD, 0x3456, 0x5678, 0x789A, 0x9ABC,
                     0xBCDE, 0x7777, sp, 0xF000, 0x2B7D, ds, 0x1111,
                     ss, 0x72D3)
    helper.put16(ram, ss, sp, 0x1234)
    helper.put16(ram, ss, sp + 2, 0xD000)
    for i in range(5):
        helper.put16(ram, ss, sp + 4 + 2 * i, 0xA100 + i)
    helper.put16(ram, ds, 0x5D92, 0x2300)
    helper.put16(ram, ds, 0x5D94, 0x4000)
    if mode != 0:
        helper.put16(ram, 0x4000, 0x23CC, 0x2200)
        helper.put16(ram, 0x4000, 0x23CE, 0x5000)
    for off, word in ((0x23F8, 0x3300), (0x23FA, 0x6000),
                      (0x23FC, 0x4400), (0x23FE, 0x7000)):
        helper.put16(ram, 0x4000, off, word)
    ram[helper.physical(0x5000, 0x222E)] = 7
    ram[helper.physical(ds, 0x5600)] = helper.FLAGS[mode]
    for seg, off in ((0x5000, 0x2200), (0x6000, 0x3300),
                     (0x7000, 0x4400)):
        ram[helper.physical(seg, off + 0x32)] = target_byte
    ram[helper.physical(0x4000, 0x232E)] = kind
    ram[helper.physical(0x4000, 0x2373)] = value
    ram[helper.physical(ds, 0x6818)] = 0xA7
    return ram, cpu


def compare(label, model, callback_type, wrapper, callee):
    memory, cpu = make_case(wrapper, callee, label)
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, helper.RAM_SIZE)
    original.mem_write(0, bytes(memory))
    for (name, _), reg in zip(helper.Cpu._fields_, helper.REGISTERS):
        original.reg_write(reg, getattr(cpu, name))
    expected_calls = []
    reached = [False]

    def on_code(emu, address, size, user):
        del size, user
        if address == helper.physical(label[5], 0x2E8B):
            state = helper.Cpu(*(emu.reg_read(reg) & 0xFFFF
                                 for reg in helper.REGISTERS))
            snapshot = bytearray(emu.mem_read(0, helper.RAM_SIZE))
            expected_calls.append((helper.snap(state), helper.frame(snapshot, state)))
            helper.select_python(snapshot, state)
            state.ip = helper.get16(snapshot, state.ss, state.sp)
            state.sp = (state.sp + 2) & 0xFFFF
            state.cs = helper.get16(snapshot, state.ss, state.sp)
            state.sp = (state.sp + 10) & 0xFFFF
            emu.mem_write(0, bytes(snapshot))
            for (name, _), reg in zip(helper.Cpu._fields_, helper.REGISTERS):
                emu.reg_write(reg, getattr(state, name))
        elif address == helper.physical(0xD000, 0x1234):
            reached[0] = True
            emu.emu_stop()
        else:
            assert (0xF101D <= address < 0xF1086 or
                    0xF2B7D <= address < 0xF2BC8), (label, hex(address))

    original.hook_add(UC_HOOK_CODE, on_code)
    original.emu_start(helper.physical(0xF000, 0x2B7D), helper.RAM_SIZE,
                       count=140)
    assert reached[0] and len(expected_calls) == (label[0] != 0), label
    expected_ram = bytes(original.mem_read(0, helper.RAM_SIZE))
    ram = (ctypes.c_uint8 * helper.RAM_SIZE).from_buffer_copy(memory)
    actual_calls = []

    @callback_type
    def external(context, ptr, size, state_ptr):
        del context
        assert size == helper.RAM_SIZE
        state = state_ptr.contents
        actual_calls.append((helper.snap(state), helper.frame(ptr, state)))
        helper.select_python(ptr, state)
        return 1

    assert model(ram, helper.RAM_SIZE, ctypes.byref(cpu), external, None) == 0
    if bytes(ram) != expected_ram:
        diff = [i for i, (a, b) in enumerate(zip(ram, expected_ram)) if a != b]
        raise AssertionError((label, "RAM", [hex(i) for i in diff[:12]]))
    assert actual_calls == expected_calls, (label, "calls", actual_calls,
                                            expected_calls)
    for (name, _), reg in zip(helper.Cpu._fields_, helper.REGISTERS):
        actual, expected = getattr(cpu, name), original.reg_read(reg) & 0xFFFF
        assert actual == expected, (label, name, hex(actual), hex(expected))


def main():
    wrapper, callee = source(), helper.source()
    callback_type = ctypes.CFUNCTYPE(
        ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8),
        ctypes.c_size_t, ctypes.POINTER(helper.Cpu))
    with tempfile.TemporaryDirectory(prefix="poolrad-u38-2b7d-") as tmp:
        library = Path(tmp) / "model.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/overlay_u38_target_select_101d.c"),
                        str(ROOT / "src/recovered/resident_character_target_select.c"),
                        str(ROOT / "src/recovered/overlay_u38_target_reset_2b7d.c"),
                        "-o", str(library)], check=True)
        model = ctypes.CDLL(str(library)).pr_u38_target_reset_2b7d
        model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(helper.Cpu), callback_type,
                          ctypes.c_void_p]
        model.restype = ctypes.c_int
        cases = [(mode, profile, *layout, segment)
                 for segment in (0x00BA, 0x3456)
                 for mode in range(8)
                 for profile in range(len(PROFILES))
                 for layout in ((0x3000, 0xE000, 0x8000),
                                (0x3000, 0x3000, 0x7000),
                                (0xF123, 0xE000, 0x0002))]
        for label in cases:
            compare(label, model, callback_type, wrapper, callee)
    print(f"u38:+2b7d composed original-byte full-state parity: {len(cases)} cases")


if __name__ == "__main__":
    main()
