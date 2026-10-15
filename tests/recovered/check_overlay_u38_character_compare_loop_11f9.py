#!/usr/bin/env python3
"""Original-byte full-state parity for U38 +11F9 and SYSTEM AF8:0C10."""

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
SPAN_SHA = "ab4666d6d7720151a70c54792f777202f01c9803880ae3911e019affb2aeb76c"
CODE_SHA = "9f175ddec9eef17cf10700899a26277932c64394b7caef4abcf666eb3ce10438"
TRAILER_SHA = "eb19eb8681d04b84bfe569f341ba3b2de215ff19ee7c51282198074a48c57ae4"
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
    assert [i for i, off in enumerate(unit["entries"]) if off == 0x11F9] == [39]
    assert min(off for off in unit["entries"] if off > 0x11F9) == 0x1276
    assert [off for off in unit["fixups"] if 0x11F9 <= off < 0x1276] == [0x1253]
    span = ovr[0xFE54:0xFED1]
    code, trailer = span[:111], span[111:]
    assert len(span) == 125 and sha256(span).hexdigest() == SPAN_SHA
    assert len(code) == 111 and sha256(code).hexdigest() == CODE_SHA
    assert len(trailer) == 14 and sha256(trailer).hexdigest() == TRAILER_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(code, 0x11F9))
    assert sum(i.size for i in ins) == len(code)
    assert ins[-1].address == 0x1265 and ins[-1].mnemonic == "retf"
    assert [(i.address, i.op_str) for i in ins if i.mnemonic == "lcall"] == [
        (0x1250, "0xaf8, 0xc10")]
    image, _, _, _ = unpack(exe)
    system = bytes(image[0xBB90:0xBBBB])
    assert len(system) == 43 and sha256(system).hexdigest() == SYSTEM_SHA
    return span, system


def put16(ram, seg, off, value):
    helper.put16(ram, seg, off & 0xFFFF, value)


def make_case(source_bytes, label):
    span, system = source_bytes
    mode, ds, ss, sp, loaded = label
    ram = bytearray(helper.RAM_SIZE)
    at = helper.physical(0xF000, 0x11F9)
    ram[at:at + len(span)] = span
    at = helper.physical(loaded, 0x0C10)
    ram[at:at + len(system)] = system
    put16(ram, 0xF000, 0x1253, loaded)
    cpu = helper.Cpu(0xA5CD, 0x3456, 0x5678, 0x789A, 0x9ABC,
                     0xBCDE, 0x7777, sp, 0xF000, 0x11F9, ds, 0x1111,
                     ss, 0x76D3)
    put16(ram, ss, sp, 0x1234)
    put16(ram, ss, sp + 2, 0xD000)
    for i, word in enumerate((0xA100, 0xA101, 0xA102, 0x2100, 0x5000)):
        put16(ram, ss, sp + 4 + 2 * i, word)
    put16(ram, 0x5000, 0x2208, 0x2300)
    put16(ram, 0x5000, 0x220A, 0x5100)
    put16(ram, 0x5100, 0x230A, 0x2700)
    put16(ram, 0x5100, 0x230C, 0x5200)
    put16(ram, ds, 0x5D92, 0x2200)
    put16(ram, ds, 0x5D94, 0x4000)
    ram[helper.physical(0x4000, 0x229F)] = 2 if mode == 0 else 1
    ram[helper.physical(0x4000, 0x226C)] = 3 if mode == 1 else 0x82
    ram[helper.physical(ds, 0x6822)] = 0xA6
    string = b"\x03ABC" if mode != 5 else b"\x00"
    at = helper.physical(0x5200, 0x2700)
    ram[at:at + len(string)] = string
    for i in range(1, 9):
        off = 0x3F2 + (i << 4)
        if mode == 3 or (mode == 4 and i % 2 == 0):
            table = string
        elif mode == 5:
            table = b"\x01Z"
        else:
            table = b"\x03ABD"
        at = helper.physical(ds, off)
        ram[at:at + len(table)] = table
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
    runtime = helper.physical(label[4], 0x0C10)

    def on_code(emu, address, size, user):
        del size, user
        if address == runtime:
            calls.append(address)
        elif address == helper.physical(0xD000, 0x1234):
            reached[0] = True
            emu.emu_stop()
        else:
            assert (0xF11F9 <= address < 0xF1268 or
                    runtime <= address < runtime + len(source_bytes[1])), (
                        label, hex(address))

    original.hook_add(UC_HOOK_CODE, on_code)
    original.emu_start(helper.physical(0xF000, 0x11F9), helper.RAM_SIZE,
                       count=1200)
    assert reached[0] and len(calls) == (0 if label[0] in (0, 1) else 8), (
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
        library = Path(temp) / "u38_11f9.so"
        subprocess.run(["cc", "-shared", "-fPIC", "-std=c11", "-Wall",
                        "-Wextra", "-Wpedantic", "-Werror", "-Iinclude",
                        "src/recovered/overlay_u38_character_compare_loop_11f9.c",
                        "-o", str(library)], cwd=ROOT, check=True)
        model = ctypes.CDLL(str(library)).pr_u38_character_compare_loop_11f9
        model.argtypes = (ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(helper.Cpu))
        model.restype = ctypes.c_int
        cases = [(mode, ds, ss, sp, loaded)
                 for mode in range(6)
                 for ds, ss, sp, loaded in (
                     (0x3000, 0xE000, 0x8000, 0x7100),
                     (0x3000, 0x3000, 0x0010, 0x7200))]
        for label in cases:
            compare(label, source_bytes, model)
    print(f"U38 +11F9 plus SYSTEM 0C10: {len(cases)} full-RAM/14-CPU original-byte cases pass")


if __name__ == "__main__":
    main()
