#!/usr/bin/env python3
"""Original-byte/full-state parity for U25:+0F53 with its +06FB classifier."""

import ctypes
from hashlib import sha256
from pathlib import Path
import random
import subprocess
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
OVR = ROOT / "resources/DOS/POOLRAD/GAME.OVR"
EXE = ROOT / "resources/DOS/POOLRAD/START.EXE"
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
BODY_SHA = "fb04baf47e5abfffab480e8494d662d5c49762e67ca4269b12128cc9e9fd3815"
CLASSIFY_SHA = "e41b64e0284b3dff9398b1d8fe08f2c8c773f8521975021c8d10734118d39347"
STRSTORE_SHA = "009955722e8f73d0a0f7076aa418d141f642c5a729b2a58dfa7a7a3922989118"
UNIT = 0x74D3
BODY = (0x0F53, 0x1153)
CLASSIFY = (0x06FB, 0x0756)
SYSTEM_FILE = 0xB79B - 0x0B25   # START.EXE file offset of SYSTEM AF8:0000
RAM_SIZE = 1 << 20
STRSTORE = 0
CLEANUP = (10,)
ARG_WORDS = (5,)
TARGET = {0x0B25: STRSTORE}
SYSTEM_RETURNS = {0x0B25: (0x0B46, "retf", "0xa")}
# JA after CMP 0,[len-1] can never be taken (0 is never above a byte).
UNREACHABLE = {(0x0F9A, 0x0FD7), (0x1028, 0x1065), (0x1095, 0x10D2), (0x1101, 0x1138)}
REGISTERS = (UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX,
             UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI,
             UC_X86_REG_BP, UC_X86_REG_SP, UC_X86_REG_CS,
             UC_X86_REG_IP, UC_X86_REG_DS, UC_X86_REG_ES,
             UC_X86_REG_SS, UC_X86_REG_EFLAGS)
CS = 0xF000
WINDOWS = ((0x49D2, 0x4000, 0x0100), (0x49D6, 0x5000, 0x0200),
           (0x49DA, 0x6000, 0x0300), (0x49DE, 0x7000, 0x0400))
DISP = (0x6E00, 0x2A00, 0xD200, 0x6700)
RANGES = ((0x4900, 0x4CFF), (0x6B00, 0x6EFF), (0x9700, 0x98FF), (0x9900, 0xB6FF))
LOCAL_BYTES = 0x104


class Cpu(ctypes.Structure):
    _fields_ = [(name, ctypes.c_uint16) for name in
                "ax bx cx dx si di bp sp cs ip ds es ss flags".split()]


def physical(seg, off):
    return ((seg << 4) + (off & 0xFFFF)) & (RAM_SIZE - 1)


class Bytes:
    """Byte access over a bytearray, ctypes buffer or Unicorn engine."""

    def __init__(self, target):
        self.target = target

    def get(self, a):
        if isinstance(self.target, Uc):
            return self.target.mem_read(a, 1)[0]
        return self.target[a]

    def set(self, a, value):
        if isinstance(self.target, Uc):
            self.target.mem_write(a, bytes([value & 255]))
        else:
            self.target[a] = value & 255

    def get8(self, seg, off):
        return self.get(physical(seg, off))

    def set8(self, seg, off, value):
        self.set(physical(seg, off), value)

    def get16(self, seg, off):
        a = physical(seg, off)
        return self.get(a) | self.get((a + 1) & (RAM_SIZE - 1)) << 8

    def set16(self, seg, off, value):
        a = physical(seg, off)
        self.set(a, value)
        self.set((a + 1) & (RAM_SIZE - 1), value >> 8)


def instructions(data, span):
    body = data[UNIT + span[0]:UNIT + span[1]]
    return body, list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, span[0]))


def source():
    data = OVR.read_bytes()
    assert len(data) == 206105 and sha256(data).hexdigest() == OVR_SHA
    body, ins = instructions(data, BODY)
    assert len(body) == 512 and sha256(body).hexdigest() == BODY_SHA
    assert len(ins) == 181 and sum(x.size for x in ins) == 512
    assert [x.address for x in ins if x.mnemonic.startswith("ret")] == [0x1150]
    assert ins[-1].mnemonic == "retf" and ins[-1].op_str == "6"
    near = [(x.address, x.op_str) for x in ins if x.mnemonic == "call"]
    assert near == [(0x0F72, "0x6fb")]
    sites = {}
    for x in ins:
        if x.mnemonic == "lcall":
            seg, off = (int(v, 16) for v in x.op_str.split(", "))
            assert seg == 0xAF8 and off in TARGET, hex(x.address)
            sites[x.address] = TARGET[off]
        if x.mnemonic.startswith("j"):
            assert BODY[0] <= int(x.op_str, 16) < BODY[1], hex(x.address)
    assert sorted(sites) == [0x0F69, 0x1010]
    assert data[UNIT + BODY[0] - 3:UNIT + BODY[0]] == bytes.fromhex("ca0200")
    assert data[UNIT + BODY[1]:UNIT + BODY[1] + 3] == bytes.fromhex("5589e5")
    for site, target in UNREACHABLE:
        x = next(i for i in ins if i.address == site)
        assert x.mnemonic == "ja" and int(x.op_str, 16) == target
    classify, cins = instructions(data, CLASSIFY)
    assert len(classify) == 91 and sha256(classify).hexdigest() == CLASSIFY_SHA
    assert len(cins) == 28 and cins[-1].mnemonic == "retf" and cins[-1].op_str == "2"
    exe = EXE.read_bytes()
    assert sha256(exe[0xB79B:0xB7BF]).hexdigest() == STRSTORE_SHA
    for off, (ret, mnemonic, operand) in SYSTEM_RETURNS.items():
        tail = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(
            exe[SYSTEM_FILE + ret:SYSTEM_FILE + ret + 3], ret))[0]
        assert (tail.mnemonic, tail.op_str) == (mnemonic, operand), hex(off)
    return body, ins, classify, cins, sites


def args(mem, state, event):
    return tuple(mem.get16(state.ss, state.sp + 2 * i)
                 for i in range(ARG_WORDS[event]))


def copy_pascal(mem, dseg, doff, sseg, soff, cap):
    n = min(mem.get8(sseg, soff), cap)
    text = [mem.get8(sseg, soff + 1 + i) for i in range(n)]
    mem.set8(dseg, doff, n)
    for i, ch in enumerate(text):
        mem.set8(dseg, doff + 1 + i, ch)


def apply_call(mem, state, event, case):
    """SYSTEM StrStore, as decoded from START.EXE."""
    words = args(mem, state, event)
    assert event == STRSTORE
    copy_pascal(mem, words[2], words[1], words[4], words[3], words[0] & 255)
    state.cx = (state.cx & 0xFF00) | (case.rng_word(event) & 0xFF)
    state.bx = (state.bx + 0x0101) & 0xFFFF
    state.flags = (0x0A53, 0x0457)[case.seed & 1]


class Case:
    def __init__(self, address, text, stale, ah, ss, sp, seed):
        self.address, self.text, self.stale = address, text, stale
        self.ah, self.ss, self.sp, self.seed = ah, ss, sp, seed

    def rng_word(self, event):
        return (self.seed * 7 + event * 13) & 0xFFFF

    def label(self):
        return (hex(self.address), len(self.text), hex(self.ss), hex(self.sp),
                self.seed)


def klass(address):
    for i, (low, high) in enumerate(RANGES):
        if low <= address <= high:
            return i
    return 4


def make_case(body, classify, case):
    memory = bytearray(RAM_SIZE)
    mem = Bytes(memory)
    ds = 0x3000
    for span, code in ((BODY, body), (CLASSIFY, classify)):
        a = physical(CS, span[0])
        memory[a:a + len(code)] = code
    for pointer, seg, base in WINDOWS:
        mem.set16(ds, pointer, base)
        mem.set16(ds, pointer + 2, seg)
    mem.set16(ds, 0x5D92, 0x0040)
    mem.set16(ds, 0x5D94, 0x2100)
    mem.set8(0x2000, 0x0080, len(case.text))
    for i, ch in enumerate(case.text):
        mem.set8(0x2000, 0x0081 + i, ch)
    rng = random.Random(case.seed)
    bp = (case.sp - 2) & 0xFFFF
    for off in range(LOCAL_BYTES + 0x20):
        mem.set8(case.ss, bp - LOCAL_BYTES - 0x20 + off,
                 rng.randrange(256) if case.stale else 0x5A)
    mem.set16(case.ss, case.sp, 0x0A05)
    mem.set16(case.ss, case.sp + 2, 0xD000)
    mem.set16(case.ss, case.sp + 4, 0x0080)
    mem.set16(case.ss, case.sp + 6, 0x2000)
    mem.set16(case.ss, case.sp + 8, case.address)
    cpu = Cpu((case.ah << 8) | 0xCD, 0x3456, 0x5678, 0x789A, 0x9ABC,
              0xBCDE, 0x7777, case.sp, CS, BODY[0], ds, 0x1111,
              case.ss, 0x72D3 if case.seed % 2 else 0x0202)
    return memory, cpu


def snap(state):
    return tuple(getattr(state, name) for name, _ in Cpu._fields_)


def run_original(memory, cpu, sites, case):
    emu = Uc(UC_ARCH_X86, UC_MODE_16)
    emu.mem_map(0, RAM_SIZE)
    emu.mem_write(0, bytes(memory))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        emu.reg_write(reg, getattr(cpu, name))
    mem = Bytes(emu)
    spans = [(physical(CS, a), physical(CS, b)) for a, b in (BODY, CLASSIFY)]
    calls, visited, edges = [], set(), set()
    returned, last = [False], [None]

    def on_code(engine, address, size, user):
        del size, user
        if not any(a <= address < b for a, b in spans):
            returned[0] = True
            engine.emu_stop()
            return
        off = address - (CS << 4)
        if last[0] is not None:
            edges.add((last[0], off))
        last[0] = off
        visited.add(off)
        if off in sites:
            event = sites[off]
            state = Cpu(*(engine.reg_read(reg) & 0xFFFF for reg in REGISTERS))
            calls.append((event, snap(state), args(mem, state, event)))
            apply_call(mem, state, event, case)
            for (name, _), reg in zip(Cpu._fields_, REGISTERS):
                engine.reg_write(reg, getattr(state, name))
            engine.reg_write(UC_X86_REG_SP, (state.sp + CLEANUP[event]) & 0xFFFF)
            engine.reg_write(UC_X86_REG_IP, off + 5)
            last[0] = None

    emu.hook_add(UC_HOOK_CODE, on_code)
    emu.emu_start(physical(CS, BODY[0]), RAM_SIZE, count=100000)
    assert returned[0], case.label()
    state = tuple(emu.reg_read(reg) & 0xFFFF for reg in REGISTERS)
    return bytes(emu.mem_read(0, RAM_SIZE)), state, calls, visited, edges


def compare_case(body, classify, sites, case, model, callback_type):
    memory, cpu = make_case(body, classify, case)
    expected_ram, expected_cpu, expected_calls, visited, edges = \
        run_original(memory, cpu, sites, case)
    ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
    actual_calls = []

    @callback_type
    def external(context, ram_pointer, ram_size, state_pointer, event):
        del context
        assert ram_size == RAM_SIZE
        state = state_pointer.contents
        mem = Bytes(ram_pointer)
        actual_calls.append((event, snap(state), args(mem, state, event)))
        apply_call(mem, state, event, case)
        return 1

    status = model(ram, RAM_SIZE, ctypes.byref(cpu), external, None)
    assert status == 0, (case.label(), status)
    if bytes(ram) != expected_ram:
        diff = [i for i, (a, b) in enumerate(zip(ram, expected_ram)) if a != b]
        raise AssertionError((case.label(), "RAM", [hex(i) for i in diff[:12]]))
    assert actual_calls == expected_calls, (case.label(), "calls")
    for (name, _), observed in zip(Cpu._fields_, expected_cpu):
        actual = getattr(cpu, name)
        if name == "flags":
            observed &= 0x0FD5
            actual &= 0x0FD5
        assert actual == observed, (case.label(), name, hex(actual), hex(observed))
    return visited, edges


def cases():
    out = []
    addresses = [0x4900, 0x4CFF, 0x6B00, 0x6B01, 0x6EFF, 0x9700, 0x98FF,
                 0x9900, 0xB6FF, 0x48FF, 0x4D00, 0x6AFF, 0x6F00, 0x96FF,
                 0xB700, 0x0000, 0xFFFF]
    texts = (b"", b"A", b"abc", bytes(range(1, 21)), bytes(range(1, 256)))
    for address in addresses:
        for text in texts:
            out.append(Case(address, text, len(out) % 3 == 0, 0xA5,
                            0xE000, 0x8000, len(out)))
    # SS/DS alias and stack-offset wrap of the local buffer.
    for ss, sp in ((0x3000, 0x7400), (0xE000, 0x0010), (0xE000, 0xFFF0)):
        for address in (0x4900, 0x9900, 0x6B00):
            for text in (b"", b"ab"):
                out.append(Case(address, text, True, 0x11, ss, sp, len(out)))
    rng = random.Random(0x0F53)
    for _ in range(40):
        address = rng.choice([rng.randrange(low, high + 1) for low, high in RANGES]
                             + [rng.randrange(0x10000)])
        out.append(Case(address,
                        bytes(rng.randrange(256) for _ in range(rng.randrange(0, 40))),
                        True, rng.randrange(256), rng.choice((0xE000, 0x3000)),
                        rng.choice((0x8000, 0x0300, 0xFFF8)), len(out)))
    return out


def main():
    body, ins, classify, cins, sites = source()
    starts = {x.address for x in ins + cins}
    branches = {x.address: (int(x.op_str, 16), x.address + x.size)
                for x in ins + cins
                if x.mnemonic.startswith("j") and x.mnemonic != "jmp"}
    callback_type = ctypes.CFUNCTYPE(
        ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8),
        ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_int)
    visited, edges = set(), set()
    with tempfile.TemporaryDirectory(prefix="poolrad-u25-0f53-") as tmp:
        library = Path(tmp) / "model.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/overlay_u25_store_string_0f53.c"),
                        str(ROOT / "src/recovered/system_strstore_fullstate_0b25.c"),
                        "-o", str(library)], check=True)
        model = ctypes.CDLL(str(library)).pr_u25_store_string_0f53
        model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(Cpu), callback_type, ctypes.c_void_p]
        model.restype = ctypes.c_int
        all_cases = cases()
        for case in all_cases:
            v, e = compare_case(body, classify, sites, case, model, callback_type)
            visited |= v
            edges |= e
    outcomes = sum((site, t) in edges for site, pair in branches.items()
                   for t in pair if (site, t) not in UNREACHABLE)
    print(f"u25:+0F53 full-state original-byte parity: {len(all_cases)} cases, "
          f"{len(visited & starts)}/{len(starts)} instruction starts, "
          f"{outcomes}/{2 * len(branches) - len(UNREACHABLE)} reachable conditional outcomes")
    assert visited == starts and outcomes == 2 * len(branches) - len(UNREACHABLE)
    assert not any(edge in edges for edge in UNREACHABLE)


if __name__ == "__main__":
    main()
