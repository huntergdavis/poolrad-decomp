#!/usr/bin/env python3
"""Original-byte/C full-state audit of eight 1816 overlay call prefixes.

The original executes only the straight-line argument setup. A synthetic
callee hook stands at CALL FAR 00BA:1816. This checker prints address, count,
and hash evidence only; original bytes and RAM remain private.
"""

import ctypes
from hashlib import sha256
from pathlib import Path
import subprocess
import sys
import tempfile

from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
from unicorn.x86_const import (
    UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX, UC_X86_REG_DX,
    UC_X86_REG_SI, UC_X86_REG_DI, UC_X86_REG_BP, UC_X86_REG_SP,
    UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_DS, UC_X86_REG_ES,
    UC_X86_REG_SS, UC_X86_REG_EFLAGS,
)

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools/overlay"))
from resident_inventory import collect  # noqa: E402

EXE = ROOT / "resources/DOS/POOLRAD/START.EXE"
OVR = ROOT / "resources/DOS/POOLRAD/GAME.OVR"
RAM_SIZE = 1 << 20
REGISTERS = (UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX,
             UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI,
             UC_X86_REG_BP, UC_X86_REG_SP, UC_X86_REG_CS,
             UC_X86_REG_IP, UC_X86_REG_DS, UC_X86_REG_ES,
             UC_X86_REG_SS, UC_X86_REG_EFLAGS)

# Unit, call offset, file offset, prefix start, prefix file offset, SHA-256,
# pointer source, segment/offset selectors, column, row source, flag.
SITES = (
    (0x05E, 0x0372, 0x12B6D, 0x0362, 0x12B5D,
     "705949170e64159e65308a093da662c7ccdba641c83063040916d35e150f0bdb",
     "bp", 0x10, 0x0E, 0x17, -1, 0),
    (0x06C, 0x2D4F, 0x1C6CE, 0x2D3D, 0x1C6BC,
     "9b6556299a20b24c01613153586505a8a80576c5d87bdb4e44609b31b8adc265",
     "ds", 0x5D94, 0x5D92, 0x04, -0x16, 0),
    (0x077, 0x0089, 0x210C3, 0x0078, 0x210B2,
     "53a090581a2123fbb23568b84dd9cd0c62c70caf1b3c427710bf0500f045a15e",
     "ds", 0x4718, 0x4716, 0x01, None, 0),
    (0x077, 0x0DE0, 0x21E1A, 0x0DCF, 0x21E09,
     "900299d7985f4aa6f992043c52cae50d0e4d06debc3c5ebd8f80f2caacd8755a",
     "ds", 0x5D94, 0x5D92, 0x01, None, 0),
    (0x077, 0x11CC, 0x22206, 0x11BD, 0x221F7,
     "8228ea1ac5f4f4b8516524c591b7c0d89fe51b97dd687c7a3ed13d632f3165de",
     "bp", -0x3D, -0x3F, 0x01, None, 1),
    (0x077, 0x2AB6, 0x23AF0, 0x2AA5, 0x23ADF,
     "85e5363baa285a6b29c6320f0cc19b640a55280dda3ab2cab72787322ddb8ac3",
     "ds", 0x5D94, 0x5D92, 0x01, None, 1),
    (0x07F, 0x1B50, 0x267D9, 0x1B3F, 0x267C8,
     "05564fba780ad047a86fc208ca800d9cd59c8bb945b426e8f2aa8850f3f2162e",
     "ds", 0x5D94, 0x5D92, 0x01, None, 0),
    (0x086, 0x33C3, 0x2A3DB, 0x33B4, 0x2A3CC,
     "b18961dbf181e11d63f1d5d26e609f157aad52c30bcd16aebcd8707f1cead347",
     "bp", 0x0E, 0x0C, 0x01, None, 0),
)


class Cpu(ctypes.Structure):
    _fields_ = [(name, ctypes.c_uint16) for name in
                "ax bx cx dx si di bp sp cs ip ds es ss flags".split()]


class Frame(ctypes.Structure):
    _fields_ = [(name, ctypes.c_uint16) for name in
                "flag row column node_offset node_segment".split()]


def physical(seg, off):
    return ((seg << 4) + (off & 0xFFFF)) & (RAM_SIZE - 1)


def put16(memory, seg, off, value):
    a = physical(seg, off)
    memory[a] = value & 255
    memory[(a + 1) & (RAM_SIZE - 1)] = value >> 8 & 255


def read16(memory, seg, off):
    a = physical(seg, off)
    return memory[a] | memory[(a + 1) & (RAM_SIZE - 1)] << 8


def cpu_values(cpu):
    return tuple(getattr(cpu, name) & 0xFFFF for name, _ in Cpu._fields_)


def uc_values(emu):
    return tuple(emu.reg_read(reg) & 0xFFFF for reg in REGISTERS)


def assert_cpu_equal(label, actual, expected):
    for (name, _), a, b in zip(Cpu._fields_, actual, expected):
        mask = 0xFFEF if name == "flags" else 0xFFFF
        if (a ^ b) & mask:
            raise AssertionError((label, name, hex(a), hex(b)))


def source():
    exe, ovr = EXE.read_bytes(), OVR.read_bytes()
    inventory = collect(exe, ovr)
    sources = next(x for x in inventory["resident_targets"]
                   if x["id"] == "RES_00BA_1816")["sources"]
    assert len(sources) == len(SITES) == 8
    result = []
    for site, item in zip(SITES, sources):
        unit, call, file_at, start, file_start, digest, _, _, _, _, _, flag = site
        assert (item["unit"], item["unit_offset"], item["ovr_file_offset"]) == (
            unit, call, file_at)
        code = ovr[file_start:file_at + 5]
        assert len(code) == call - start + 5
        assert sha256(code).hexdigest() == digest
        assert code[-8:-5] == bytes((0xB0, flag, 0x50))
        assert code[-5] == 0x9A and int.from_bytes(code[-4:-2], "little") == 0x1816
        assert int.from_bytes(code[-2:], "little") == 0x00BA
        ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(code, start))
        assert len(ins) == 9 and ins[0].address == start
        assert ins[-1].address == call and ins[-1].mnemonic == "lcall"
        assert [x.mnemonic for x in ins].count("push") == 5
        assert ins[-1].address + ins[-1].size == call + 5
        result.append((code, tuple(x.address for x in ins)))
    return result


def make_case(site, code, seed):
    unit, call, _, start, _, _, kind, seg_at, off_at, col, row_at, flag = site
    del unit, call, flag
    memory = bytearray(RAM_SIZE)
    cs, ds, ss, bp = 0xF000, 0x1000, 0xE000, 0x3000
    ax = (0xABCD, 0x12EF, 0xFF00)[seed]
    sp = (0x8000, 0x0008, 0x9000)[seed]
    off = (0x2345, 0xFFFE, 0x0000)[seed]
    seg = (0x4567, 0x2000, 0x7FFF)[seed]
    row_value = (7, 0x82, 0x00)[seed]
    memory[physical(cs, start):physical(cs, start) + len(code)] = code
    if kind == "bp":
        put16(memory, ss, bp + seg_at, seg)
        put16(memory, ss, bp + off_at, off)
    else:
        put16(memory, ds, seg_at, seg)
        put16(memory, ds, off_at, off)
    if row_at is not None:
        memory[physical(ss, bp + row_at)] = row_value
    row_low = row_value if row_at is not None else (
        21 if site[1] == 0x0DE0 else 19 if site[1] == 0x33C3 else 1)
    frame = ((ax & 0xFF00) | site[-1], (ax & 0xFF00) | row_low,
             (ax & 0xFF00) | col, off, seg)
    cpu = Cpu(ax, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE,
              bp, sp, cs, start, ds, 0x1111, ss, 0x7202)
    return memory, cpu, frame


def compare_case(label, site_index, site, code, addresses, model, callback_type,
                 seed):
    memory, start_cpu, expected_frame = make_case(site, code, seed)
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, RAM_SIZE)
    original.mem_write(0, bytes(memory))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        original.reg_write(reg, getattr(start_cpu, name))
    call = site[1]
    executed = []
    before = []
    marker = physical(start_cpu.ds, 0x6000 + site_index)
    result_ax = 0x5A00 | (site_index << 4) | seed

    def on_code(emu, address, size, user):
        del size, user
        executed.append(emu.reg_read(UC_X86_REG_IP))
        if address != physical(start_cpu.cs, call):
            return
        snap = bytes(emu.mem_read(0, RAM_SIZE))
        cpu_snap = uc_values(emu)
        sp = emu.reg_read(UC_X86_REG_SP)
        frame = tuple(read16(snap, start_cpu.ss, sp + 2 * i)
                      for i in range(5))
        assert frame == expected_frame, (label, frame, expected_frame)
        before.append((snap, cpu_snap, frame))
        emu.mem_write(marker, bytes((0xA0 + site_index,)))
        emu.reg_write(UC_X86_REG_AX, result_ax)
        emu.reg_write(UC_X86_REG_SP, (sp + 10) & 0xFFFF)
        emu.reg_write(UC_X86_REG_IP, call + 5)
        emu.emu_stop()

    original.hook_add(UC_HOOK_CODE, on_code)
    original.emu_start(physical(start_cpu.cs, site[3]),
                       physical(start_cpu.cs, call + 5), count=16)
    assert len(before) == 1 and tuple(executed) == addresses, (label, executed)
    expected_ram = bytes(original.mem_read(0, RAM_SIZE))
    expected_cpu = uc_values(original)
    ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
    cpu = Cpu(*cpu_values(start_cpu))
    events, errors = [], []

    @callback_type
    def callback(ctx, ram_ptr, ram_size, cpu_ptr, event_site, event, value,
                 frame_ptr):
        del ctx
        try:
            assert ram_size == RAM_SIZE and event_site == site_index
            events.append((event, value))
            if event == 5:
                assert frame_ptr
                observed_frame = tuple(getattr(frame_ptr.contents, name)
                                       for name, _ in Frame._fields_)
                assert observed_frame == expected_frame
                assert bytes(ctypes.string_at(ram_ptr, RAM_SIZE)) == before[0][0]
                assert_cpu_equal(label + " pre-call", cpu_values(cpu_ptr.contents),
                                 before[0][1])
                ram_ptr[marker] = 0xA0 + site_index
                cpu_ptr.contents.ax = result_ax
            else:
                assert not frame_ptr
            return 0
        except Exception as exc:
            errors.append(exc)
            return 1

    status = model(site_index, ram, RAM_SIZE, ctypes.byref(cpu), callback, None)
    if errors:
        raise errors[0]
    assert status == 0, (label, status)
    assert [e for e, _ in events] == list(range(6)), (label, events)
    assert [v for _, v in events[:5]] == list(reversed(expected_frame)), (
        label, events, expected_frame)
    assert events[5][1] == 0
    assert bytes(ram) == expected_ram, (label, "RAM mismatch")
    assert_cpu_equal(label + " return", cpu_values(cpu), expected_cpu)


def main():
    sources = source()
    callback_type = ctypes.CFUNCTYPE(
        ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8),
        ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_uint, ctypes.c_int,
        ctypes.c_uint16, ctypes.POINTER(Frame))
    with tempfile.TemporaryDirectory(prefix="poolrad-1816-callers-",
                                     dir=ROOT / "resources") as tmp:
        library = Path(tmp) / "model.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC",
                        str(ROOT / "tools/overlay/resident_1816_caller_routes.c"),
                        "-o", str(library)], check=True)
        model = ctypes.CDLL(str(library)).pr_1816_caller_route_step
        model.argtypes = [ctypes.c_uint, ctypes.POINTER(ctypes.c_uint8),
                          ctypes.c_size_t, ctypes.POINTER(Cpu), callback_type,
                          ctypes.c_void_p]
        model.restype = ctypes.c_int
        for index, (site, (code, addresses)) in enumerate(zip(SITES, sources)):
            for seed in range(3):
                label = f"{site[0]:03X}:+{site[1]:04X}/case-{seed}"
                compare_case(label, index, site, code, addresses, model,
                             callback_type, seed)
    print("RES_00BA:1816 overlay caller prefixes: 8 sites, 24 full-state "
          "original/C synthetic-callee cases passed")


if __name__ == "__main__":
    main()
