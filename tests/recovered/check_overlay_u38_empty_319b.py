#!/usr/bin/env python3
"""Original-byte full-state parity for U38 entry 116 (+319B)."""

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
BODY_SHA = "3d5d31028d8927cd81dffdf3a86f216fa4eb84d0874dc3daada06bd31188514d"
SEGMENTS = (0x0000, 0x3000, 0x7000, 0xB000, 0xE000, 0xEFFF)
STACKS = (0x0000, 0x0001, 0x317A, 0x8000,
          0xFFF0, 0xFFF8, 0xFFFA, 0xFFFC)
BPS = (0, 0x0123, 0x7777, 0xFFFF)
BACKGROUND = bytes(range(256)) * 4096


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
    assert len(unit["entries"]) == 117 and unit["entries"][116] == 0x319B
    assert not [off for off in unit["fixups"] if 0x319B <= off < 0x31A2]
    body = ovr[0x11DF6:0x11DFD]
    assert len(body) == 7 and sha256(body).hexdigest() == BODY_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x319B))
    assert [(i.address, i.mnemonic) for i in ins] == [
        (0x319B, "push"), (0x319C, "mov"),
        (0x319E, "pop"), (0x319F, "retf")]
    assert sum(i.size for i in ins) == len(body)
    assert ovr[0x11DFD:0x11E00] == b"\x55\x89\xe5"  # installer prologue
    return body


def make_case(body, ss, sp, bp):
    ram = bytearray(BACKGROUND)
    at = shared.physical(0xF000, 0x319B)
    ram[at:at + len(body)] = body
    flags = (0x0202, 0x72D3, 0x0AD7, 0x0ED7)[BPS.index(bp)]
    cpu = shared.Cpu(0xA5CD, 0x3456, 0x5678, 0x789A,
                     0x9ABC, 0xBCDE, bp, sp, 0xF000, 0x319B,
                     0x3000, 0x1111, ss, flags)
    shared.put16(ram, ss, sp, 0x1234)
    shared.put16(ram, ss, (sp + 2) & 0xFFFF, 0xD000)
    for i in range(5):
        shared.put16(ram, ss, (sp + 4 + 2 * i) & 0xFFFF, 0xA100 + i)
    return ram, cpu


def compare(model, body, ss, sp, bp):
    label = (ss, sp, bp)
    memory, cpu = make_case(body, ss, sp, bp)
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, shared.RAM_SIZE)
    original.mem_write(0, bytes(memory))
    for (name, _), reg in zip(shared.Cpu._fields_, shared.REGISTERS):
        original.reg_write(reg, getattr(cpu, name))
    reached = [False]

    def on_code(emu, address, size, user):
        del size, user
        if address == shared.physical(0xD000, 0x1234):
            reached[0] = True
            emu.emu_stop()
        else:
            assert shared.physical(0xF000, 0x319B) <= address < (
                shared.physical(0xF000, 0x31A2)), (label, hex(address))

    original.hook_add(UC_HOOK_CODE, on_code)
    try:
        original.emu_start(shared.physical(0xF000, 0x319B),
                           shared.RAM_SIZE, count=10)
    except Exception as exc:
        raise AssertionError((label, "original-byte exception", str(exc))) from exc
    assert reached[0], label
    expected_ram = bytes(original.mem_read(0, shared.RAM_SIZE))
    expected_cpu = tuple(original.reg_read(reg) & 0xFFFF
                         for reg in shared.REGISTERS)
    ram = (ctypes.c_uint8 * shared.RAM_SIZE).from_buffer_copy(memory)
    assert model(ram, shared.RAM_SIZE, ctypes.byref(cpu)) == 0
    if bytes(ram) != expected_ram:
        diff = [i for i, (a, b) in enumerate(zip(ram, expected_ram)) if a != b]
        raise AssertionError((label, "RAM", [hex(i) for i in diff[:12]]))
    assert shared.snap(cpu) == expected_cpu, (
        label, "CPU", shared.snap(cpu), expected_cpu)


def main():
    body = source()
    with tempfile.TemporaryDirectory(prefix="poolrad-u38-319b-") as temp:
        library = Path(temp) / "model.so"
        subprocess.run(["cc", "-shared", "-fPIC", "-std=c11", "-Wall",
                        "-Wextra", "-Wpedantic", "-Werror", "-I",
                        str(ROOT / "include"),
                        str(ROOT / "src/recovered/overlay_u38_empty_319b.c"),
                        "-o", str(library)], check=True)
        model = ctypes.CDLL(str(library)).pr_u38_empty_319b
        model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(shared.Cpu)]
        model.restype = ctypes.c_int
        for ss in SEGMENTS:
            for sp in STACKS:
                for bp in BPS:
                    compare(model, body, ss, sp, bp)
    print("192 original-byte U38 +319B cases passed: full 1 MiB RAM, "
          "14 CPU fields, near-1MiB stack and RETF 10 SP wraps")


if __name__ == "__main__":
    main()
