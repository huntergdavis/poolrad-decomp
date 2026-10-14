#!/usr/bin/env python3
"""Full in-body state parity for GAME.OVR u77:+189D with BA:1509 stubbed."""

import ctypes
from hashlib import sha256
from pathlib import Path
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
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
SPAN_SHA = "3b2597236166428adc977c31724e007e3942ac24a74e740a39d81ea174d10091"
CODE_SHA = "0a9c119606497ff144da91e7b4340f80842319c6ccc6ea719951cabb9a4d16d2"
RAM_SIZE = 1 << 20
REGISTERS = (UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX,
             UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI,
             UC_X86_REG_BP, UC_X86_REG_SP, UC_X86_REG_CS,
             UC_X86_REG_IP, UC_X86_REG_DS, UC_X86_REG_ES,
             UC_X86_REG_SS, UC_X86_REG_EFLAGS)
FIELDS = (0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x36, 0x37,
          0x38, 0x3C, 0x3D, 0x3E)


class Cpu(ctypes.Structure):
    _fields_ = [(name, ctypes.c_uint16) for name in
                "ax bx cx dx si di bp sp cs ip ds es ss flags".split()]


def physical(seg, off):
    return ((seg << 4) + (off & 0xFFFF)) & (RAM_SIZE - 1)


def word(memory, seg, off):
    a = physical(seg, off)
    return memory[a] | memory[(a + 1) & (RAM_SIZE - 1)] << 8


def put_word(memory, seg, off, value):
    a = physical(seg, off)
    memory[a] = value & 255
    memory[(a + 1) & (RAM_SIZE - 1)] = value >> 8 & 255


def put_far(memory, seg, off, pointer):
    put_word(memory, seg, off, pointer[0])
    put_word(memory, seg, (off + 2) & 0xFFFF, pointer[1])


def put_byte(memory, seg, off, value):
    memory[physical(seg, off)] = value


def source():
    data = OVR.read_bytes()
    assert len(data) == 206105 and sha256(data).hexdigest() == OVR_SHA
    span = data[141527:142020]
    assert len(span) == 493 and sha256(span).hexdigest() == SPAN_SHA
    code = span[:474]
    assert sha256(code).hexdigest() == CODE_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(code, 0x189D))
    assert len(ins) == 154 and ins[-1].address == 0x1A74
    assert ins[-1].mnemonic == "retf" and ins[-1].op_str == "4"
    assert ins[-1].address + ins[-1].size == 0x1A77
    assert [(x.address, x.op_str) for x in ins if x.mnemonic == "lcall"] == [
        (0x1A13, "0xba, 0x1509")]
    starts = {x.address for x in ins}
    assert all(int(x.op_str, 16) in starts for x in ins
               if x.mnemonic.startswith("j"))
    return code


def make_case(code, *, head=(), target_count=100, counts=(),
              mismatch=None, modes=(), target_offset=0x0100):
    memory = bytearray(RAM_SIZE)
    cs, ds, ss = 0xF000, 0x1000, 0xE000
    active = (0x0100, 0x3000)
    target = (target_offset, 0x4000)
    nodes = [target, (0x0100, 0x4100), (0x0100, 0x4200),
             (0x0100, 0x4300)]
    memory[physical(cs, 0x189D):physical(cs, 0x189D) + len(code)] = code
    put_far(memory, ds, 0x5D92, active)
    # Replace the active record's head at its +C8 far field.
    put_far(memory, active[1], active[0] + 0xC8,
            nodes[head[0]] if head else (0, 0))
    for index, node in enumerate(nodes):
        put_far(memory, node[1], node[0] + 0x2A, (0, 0))
        for field in FIELDS:
            put_byte(memory, node[1], node[0] + field,
                     1 if field == 0x3C else (field * 7 + 3) & 255)
        put_byte(memory, node[1], node[0] + 0x39,
                 target_count if index == 0 else (counts[index - 1]
                                                   if index - 1 < len(counts)
                                                   else 50))
    for position, index in enumerate(head):
        next_node = nodes[head[position + 1]] if position + 1 < len(head) else (0, 0)
        put_far(memory, nodes[index][1], nodes[index][0] + 0x2A, next_node)
    if mismatch is not None:
        node_index, field, value = mismatch
        put_byte(memory, nodes[node_index][1], nodes[node_index][0] + field,
                 value)
    if "current_3c_2" in modes:
        put_byte(memory, nodes[1][1], nodes[1][0] + 0x3C, 2)
    sp = 0x8000
    put_word(memory, ss, sp, 0x0712)
    put_word(memory, ss, sp + 2, cs)
    put_word(memory, ss, sp + 4, target[0])
    put_word(memory, ss, sp + 6, target[1])
    cpu = Cpu(0xABCD, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE,
              0x7777, sp, cs, 0x189D, ds, 0x1111, ss, 0x7202)
    return memory, cpu


def compare_case(label, memory, cpu, code, model, callback_type):
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, RAM_SIZE)
    original.mem_write(0, bytes(memory))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        original.reg_write(reg, getattr(cpu, name))
    code_start = physical(cpu.cs, 0x189D)
    expected_calls = []
    returned = [False]

    def on_code(emu, address, size, user):
        del size, user
        if address == physical(cpu.cs, 0x1A13):
            sp = emu.reg_read(UC_X86_REG_SP)
            args = tuple(word(emu.mem_read(0, RAM_SIZE), cpu.ss, sp + 2 * i)
                         for i in range(4))
            expected_calls.append(args)
            emu.reg_write(UC_X86_REG_SP, (sp + 8) & 0xFFFF)
            emu.reg_write(UC_X86_REG_IP, 0x1A18)
        elif not code_start <= address < code_start + len(code):
            returned[0] = True
            emu.emu_stop()

    original.hook_add(UC_HOOK_CODE, on_code)
    original.emu_start(code_start, RAM_SIZE, count=10000)
    assert returned[0], f"original did not return: {label}"
    expected_ram = bytes(original.mem_read(0, RAM_SIZE))
    ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
    actual_calls = []

    @callback_type
    def remove(context, ram_pointer, ram_size, state, noff, nseg, coff, cseg):
        del context, ram_pointer, ram_size, state
        actual_calls.append((noff, nseg, coff, cseg))
        return 1

    status = model(ram, RAM_SIZE, ctypes.byref(cpu), remove, None, 100)
    assert status == 0, (label, status)
    actual_ram = bytes(ram)
    if actual_ram != expected_ram:
        diff = [i for i, (a, b) in enumerate(zip(actual_ram, expected_ram))
                if a != b]
        raise AssertionError((label, "RAM", diff[:12],
                              [(i, actual_ram[i], expected_ram[i])
                               for i in diff[:12]]))
    assert actual_calls == expected_calls, (label, actual_calls, expected_calls)
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        observed = original.reg_read(reg) & 0xFFFF
        actual = getattr(cpu, name)
        mask = 0xFFEF if name == "flags" else 0xFFFF
        assert ((actual ^ observed) & mask) == 0, (
            label, name, hex(actual), hex(observed))


def main():
    code = source()
    callback_type = ctypes.CFUNCTYPE(
        ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8),
        ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_uint16,
        ctypes.c_uint16, ctypes.c_uint16, ctypes.c_uint16)
    with tempfile.TemporaryDirectory(prefix="poolrad-u77-189d-") as tmp:
        library = Path(tmp) / "model.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/overlay_u77_stack_189d.c"),
                        "-o", str(library)], check=True)
        model = ctypes.CDLL(str(library)).pr_u77_stack_189d
        model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(Cpu), callback_type,
                          ctypes.c_void_p, ctypes.c_size_t]
        model.restype = ctypes.c_int
        cases = [
            ("empty", dict(head=())),
            ("target-alone", dict(head=(0,))),
            ("merge-50", dict(head=(1,), target_count=100, counts=(50,))),
            ("merge-then-target", dict(head=(1, 0), target_count=100,
                                       counts=(50,))),
            ("overflow", dict(head=(1,), target_count=250, counts=(20,))),
            ("two-merges", dict(head=(1, 2), target_count=100,
                                counts=(50, 50))),
            ("zero-current", dict(head=(1,), target_count=100, counts=(0,))),
            ("field-mismatch", dict(head=(1,), mismatch=(1, 0x2F, 0xEE))),
            ("field-3e-mismatch", dict(head=(1,), mismatch=(1, 0x3E, 0xEE))),
            ("field-3c-gate", dict(head=(1,), modes=("current_3c_2",))),
            ("target-zero", dict(head=(1,), target_count=0, counts=(50,))),
            ("offset-wrap", dict(head=(1,), target_count=100,
                                 counts=(50,), target_offset=0xFF00)),
        ]
        for field in (0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x36,
                      0x37, 0x38):
            cases.append((f"mismatch-{field:02x}",
                          dict(head=(1,), mismatch=(1, field, 0xEE))))
        for field, value in ((0x3C, 0), (0x3D, 0xEE), (0x3E, 0xEE)):
            cases.append((f"self-compare-{field:02x}",
                          dict(head=(1,), mismatch=(1, field, value))))
        cases.extend([
            ("sum-exact-255", dict(head=(1,), target_count=200,
                                   counts=(55,))),
            ("sum-first-overflow", dict(head=(1,), target_count=200,
                                        counts=(56,))),
        ])
        for label, options in cases:
            memory, cpu = make_case(code, **options)
            compare_case(label, memory, cpu, code, model, callback_type)
    print(f"u77:+189D full-state original-byte parity: {len(cases)} cases")


if __name__ == "__main__":
    main()
