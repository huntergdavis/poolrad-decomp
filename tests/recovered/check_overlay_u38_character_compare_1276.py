#!/usr/bin/env python3
"""Original-byte full-state parity for U38 +1276 plus SYSTEM AF8:0C10."""

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
BODY_SHA = "0598c5bb03bf995e3aeba41c11d749c629fa65cd273bd6a00892bf9a5eaca01a"
CONSTANT_SHA = "eb19eb8681d04b84bfe569f341ba3b2de215ff19ee7c51282198074a48c57ae4"
SYSTEM_SHA = "11e2860118cb471f9f1ecb1d0c44c060e0fbb83862f69c1cf39b5e0484298a58"


def source():
    ovr = helper.OVR.read_bytes()
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    assert len(ovr) == 206105 and sha256(ovr).hexdigest() == helper.OVR_SHA
    assert len(exe) == 65231 and sha256(exe).hexdigest() == helper.EXE_SHA
    sys.path.insert(0, str(ROOT / "tools/overlay"))
    from overlay_inventory import find_units, parse_mz
    from resident_loader_map import unpack
    unit = next(u for u in find_units(exe[parse_mz(exe)["header_bytes"]:],
                                    ovr) if u["stub_para"] == 0x38)
    assert unit["ovr_offset"] == 0xEC5B
    assert [i for i, off in enumerate(unit["entries"]) if off == 0x1276] == [40]
    assert min(off for off in unit["entries"] if off > 0x1276) == 0x12B2
    assert [off for off in unit["fixups"] if 0x1276 <= off < 0x12B2] == [0x1293, 0x12A5]
    body = ovr[0xFED1:0xFF0D]
    constants = ovr[0xFEC3:0xFED1]
    assert len(body) == 60 and sha256(body).hexdigest() == BODY_SHA
    assert len(constants) == 14 and sha256(constants).hexdigest() == CONSTANT_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x1276))
    assert len(ins) == 24 and sum(i.size for i in ins) == len(body)
    assert [(i.address, i.op_str) for i in ins if i.mnemonic == "lcall"] == [
        (0x1290, "0xaf8, 0xc10"), (0x12A2, "0xaf8, 0xc10")]
    image, _, _, _ = unpack(exe)
    system = bytes(image[0xBB90:0xBBBB])
    assert len(system) == 43 and sha256(system).hexdigest() == SYSTEM_SHA
    system_ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(system, 0x0C10))
    assert sum(i.size for i in system_ins) == len(system)
    assert system_ins[-1].address == 0x0C38 and system_ins[-1].mnemonic == "retf"
    return body, constants, system


def put16(ram, seg, off, value):
    helper.put16(ram, seg, off & 0xFFFF, value)


def make_case(source_bytes, label):
    body, constants, system = source_bytes
    mode, ds, ss, sp, first_segment, second_segment = label
    ram = bytearray(helper.RAM_SIZE)
    at = helper.physical(0xF000, 0x1276)
    ram[at:at + len(body)] = body
    at = helper.physical(0xF000, 0x1268)
    ram[at:at + len(constants)] = constants
    for seg in (first_segment, second_segment):
        at = helper.physical(seg, 0x0C10)
        ram[at:at + len(system)] = system
    put16(ram, 0xF000, 0x1293, first_segment)
    put16(ram, 0xF000, 0x12A5, second_segment)
    cpu = helper.Cpu(0xA5CD, 0x3456, 0x5678, 0x789A, 0x9ABC,
                     0xBCDE, 0x7777, sp, 0xF000, 0x1276, ds, 0x1111,
                     ss, 0x76D3)
    put16(ram, ss, sp, 0x1234)
    put16(ram, ss, sp + 2, 0xD000)
    for i in range(5):
        put16(ram, ss, sp + 4 + 2 * i, 0xA100 + i)
    put16(ram, ds, 0x5D92, 0x2200)
    put16(ram, ds, 0x5D94, 0x4000)
    ram[helper.physical(ds, 0x6822)] = 0xA6
    ram[helper.physical(0x4000, 0x229F)] = 2 if mode == 0 else 1
    first = constants[:1 + constants[0]]
    second = constants[8:8 + 1 + constants[8]]
    assert len(first) <= 8 and len(second) <= 6
    if mode == 1:
        string = first
    elif mode == 2:
        string = second
    elif mode == 3:
        string = bytearray(first)
        string[-1] ^= 0x20
    elif mode == 4:
        string = b"\x00"
    elif mode == 5:
        string = bytes([3]) + first[1:4]
    else:
        string = b"\x00"
    at = helper.physical(0x4000, 0x2200)
    ram[at:at + len(string)] = string
    return ram, cpu


def compare(label, source_bytes, model):
    memory, cpu = make_case(source_bytes, label)
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, helper.RAM_SIZE)
    original.mem_write(0, bytes(memory))
    for (name, _), reg in zip(helper.Cpu._fields_, helper.REGISTERS):
        original.reg_write(reg, getattr(cpu, name))
    reached = [False]
    calls = []
    call_at = {helper.physical(seg, 0x0C10)
               for seg in (label[4], label[5])}

    def on_code(emu, address, size, user):
        del size, user
        if address in call_at:
            calls.append(address)
        elif address == helper.physical(0xD000, 0x1234):
            reached[0] = True
            emu.emu_stop()
        else:
            assert (0xF1276 <= address < 0xF12B2 or
                    any(helper.physical(seg, 0x0C10) <= address <
                        helper.physical(seg, 0x0C3B)
                        for seg in (label[4], label[5]))), (label, hex(address))

    original.hook_add(UC_HOOK_CODE, on_code)
    original.emu_start(helper.physical(0xF000, 0x1276), helper.RAM_SIZE,
                       count=300)
    assert reached[0] and len(calls) == (0 if label[0] == 0 else
                                        (1 if label[0] == 1 else 2)), (
                                            label, reached[0], len(calls), calls)
    expected_ram = bytes(original.mem_read(0, helper.RAM_SIZE))
    ram = (ctypes.c_uint8 * helper.RAM_SIZE).from_buffer_copy(memory)
    assert model(ram, helper.RAM_SIZE, ctypes.byref(cpu)) == 0
    if bytes(ram) != expected_ram:
        diff = [i for i, (a, b) in enumerate(zip(ram, expected_ram)) if a != b]
        raise AssertionError((label, "RAM", diff[:20]))
    expected_cpu = helper.Cpu(*(original.reg_read(reg) & 0xFFFF
                                for reg in helper.REGISTERS))
    assert helper.snap(cpu) == helper.snap(expected_cpu), (
        label, helper.snap(cpu), helper.snap(expected_cpu))


def main():
    source_bytes = source()
    with tempfile.TemporaryDirectory() as temp:
        library = Path(temp) / "u38_1276.so"
        subprocess.run(["cc", "-shared", "-fPIC", "-std=c11", "-Wall",
                        "-Wextra", "-Wpedantic", "-Werror", "-Iinclude",
                        "src/recovered/overlay_u38_character_compare_1276.c",
                        "-o", str(library)], cwd=ROOT, check=True)
        model = ctypes.CDLL(str(library)).pr_u38_character_compare_1276
        model.argtypes = (ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(helper.Cpu))
        model.restype = ctypes.c_int
        cases = [(mode, ds, ss, sp, first, second)
                 for mode in range(6)
                 for ds, ss, sp, first, second in (
                     (0x3000, 0xE000, 0x8000, 0x7100, 0x7200),
                     (0x3000, 0x3000, 0x0010, 0x7300, 0x7400))]
        for label in cases:
            compare(label, source_bytes, model)
    print(f"U38 +1276 plus SYSTEM 0C10: {len(cases)} full-RAM/14-CPU original-byte cases pass")


if __name__ == "__main__":
    main()
