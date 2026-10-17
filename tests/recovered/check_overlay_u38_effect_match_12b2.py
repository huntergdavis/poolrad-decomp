#!/usr/bin/env python3
"""Original-byte full-state parity for U38 +12B2 calling U38 +0C06."""

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
BODY_SHA = "fdc58872484dc92452c0eb042ec51727f3543e989c6fed48c30876899ddc061b"
LEAF_SHA = "435bd9fac807d24f064b6bf223817b188e737a5b6ddb9a8b37d5635842a52fe8"


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
    assert [i for i, off in enumerate(unit["entries"]) if off == 0x12B2] == [41]
    assert [i for i, off in enumerate(unit["entries"]) if off == 0x0C06] == [30]
    assert min(off for off in unit["entries"] if off > 0x12B2) == 0x12FB
    assert [off for off in unit["fixups"] if 0x12B2 <= off < 0x12FB] == []
    body = ovr[0xFF0D:0xFF56]
    leaf = ovr[0xF861:0xF870]
    assert len(body) == 73 and sha256(body).hexdigest() == BODY_SHA
    assert len(leaf) == 15 and sha256(leaf).hexdigest() == LEAF_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x12B2))
    assert sum(i.size for i in ins) == len(body)
    assert [(i.address, i.op_str) for i in ins if i.mnemonic == "call"] == [
        (0x12E8, "0xc06")]
    assert ins[-1].address == 0x12F8 and ins[-1].mnemonic == "retf"
    leaf_ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(leaf, 0x0C06))
    assert sum(i.size for i in leaf_ins) == len(leaf)
    assert leaf_ins[-1].address == 0x0C12 and leaf_ins[-1].mnemonic == "retf"
    return body, leaf


def put16(ram, seg, off, value):
    helper.put16(ram, seg, off & 0xFFFF, value)


def make_case(source_bytes, label):
    body, leaf = source_bytes
    mode, ds, ss, sp, b3, a10e, old16, old22 = label
    ram = bytearray(helper.RAM_SIZE)
    at = helper.physical(0xF000, 0x12B2)
    ram[at:at + len(body)] = body
    at = helper.physical(0xF000, 0x0C06)
    ram[at:at + len(leaf)] = leaf
    cpu = helper.Cpu(0xA5CD, 0x3456, 0x5678, 0x789A, 0x9ABC,
                     0xBCDE, 0x7777, sp, 0xF000, 0x12B2, ds, 0x1111,
                     ss, 0x76D3)
    for i, word in enumerate((0x1234, 0xD000, 0xA145, 0x2200,
                              0x4000, 0x2100, 0x5000)):
        put16(ram, ss, sp + 2 * i, word)
    ram[helper.physical(0x4000, 0x2203)] = b3
    ram[helper.physical(0x5000, 0x220E)] = a10e
    ram[helper.physical(ds, 0x6816)] = old16
    ram[helper.physical(ds, 0x6822)] = old22
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

    def on_code(emu, address, size, user):
        del size, user
        if address == 0xF0C06:
            calls.append(address)
        elif address == helper.physical(0xD000, 0x1234):
            reached[0] = True
            emu.emu_stop()
        else:
            assert (0xF12B2 <= address < 0xF12FB or
                    0xF0C06 <= address < 0xF0C15), (label, hex(address))

    original.hook_add(UC_HOOK_CODE, on_code)
    original.emu_start(0xF12B2, helper.RAM_SIZE, count=200)
    expected_calls = int((label[4] & 0x10) >> 4 == label[5])
    assert reached[0] and len(calls) == expected_calls, (
        label, reached[0], len(calls))
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
        library = Path(temp) / "u38_12b2.so"
        subprocess.run(["cc", "-shared", "-fPIC", "-std=c11", "-Wall",
                        "-Wextra", "-Wpedantic", "-Werror", "-Iinclude",
                        "src/recovered/overlay_u38_effect_match_12b2.c",
                        "-o", str(library)], cwd=ROOT, check=True)
        model = ctypes.CDLL(str(library)).pr_u38_effect_match_12b2
        model.argtypes = (ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(helper.Cpu))
        model.restype = ctypes.c_int
        modes = ((0x00, 0, 0x7F, 0xFF),
                 (0x10, 1, 0xFF, 0x7F),
                 (0x00, 1, 0x00, 0x80),
                 (0x10, 0, 0x80, 0x00))
        layouts = ((0x3000, 0xE000, 0x8000),
                   (0x3000, 0x3000, 0x6830),
                   (0x3000, 0x3000, 0x0013))
        cases = [(mode, ds, ss, sp, b3, a10e, old16, old22)
                 for mode, (b3, a10e, old16, old22) in enumerate(modes)
                 for ds, ss, sp in layouts]
        for label in cases:
            compare(label, source_bytes, model)
    print(f"U38 +12B2 plus +0C06: {len(cases)} full-RAM/14-CPU original-byte cases pass")


if __name__ == "__main__":
    main()
