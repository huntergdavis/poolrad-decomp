#!/usr/bin/env python3
"""Original two-copy/caller U3B0 +0E30 finite node-loop parity."""

import ctypes
from hashlib import sha256
import importlib.util
from pathlib import Path
import subprocess
import tempfile

from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_CS, UC_X86_REG_IP

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location(
    "setup", ROOT / "tests/recovered/check_overlay_u3b0_entry4_loop_setup_0e0c.py")
setup = importlib.util.module_from_spec(spec)
spec.loader.exec_module(setup)
prior = setup.prior

LOOP_SHA = "1f47b45e511eb13f59a8c73c7ef6f16c956975def923bbbd56eaaf89d7023407"
PATTERNS = {
    "zero": (0,),
    "one": (1,),
    "tail_zero": (1, 0),
    "tail_one": (1, 1),
    "three_zero": (1, 1, 0),
    "three_one": (1, 1, 1),
    "negative_flag": (0x80, 0),
}


def sources():
    code, copy, caller, leaf, setup_bytes = setup.sources()
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    assert len(ovr) == 206105 and sha256(ovr).hexdigest() == prior.OVR_SHA
    loop = ovr[setup.BASE + 0x0E30:setup.BASE + 0x0E5F]
    assert len(loop) == 47 and sha256(loop).hexdigest() == LOOP_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(loop, 0x0E30))
    assert len(ins) == 13 and sum(i.size for i in ins) == 47
    assert ins[0].address == 0x0E30 and ins[-1].address == 0x0E5D
    return code, copy, caller, leaf, setup_bytes, loop


def frame_after_setup(parts, case):
    code, copy, caller, leaf, setup_bytes, loop = parts
    sp, lengths, pattern, coords = case
    flags = PATTERNS[pattern]
    ram, cpu = prior.two_copy_frame(
        code, copy, (sp, lengths, (0x2200, 0x5000), coords))
    ram = bytearray(ram)
    for index, flag in enumerate(flags):
        offset = 0x2200 + index * 0x100
        address = prior.physical(0x5000, offset + 0x29)
        assert ram[address] == 0
        ram[address] = flag
        next_offset = offset + 0x100 if index + 1 < len(flags) else 0
        next_segment = 0x5000 if next_offset else 0
        prior.put16(ram, 0x5000, offset + 0x2A, next_offset)
        prior.put16(ram, 0x5000, offset + 0x2C, next_segment)
    at = prior.physical(prior.CS, 0x0DFF)
    ram[at:at + len(caller) + len(setup_bytes) + len(loop)] = (
        caller + setup_bytes + loop)
    at = prior.physical(prior.CS, 0x0067)
    ram[at:at + len(leaf)] = leaf

    emu = Uc(UC_ARCH_X86, UC_MODE_16)
    emu.mem_map(0, prior.RAM_SIZE)
    emu.mem_write(0, bytes(ram))
    prior.load_cpu(emu, cpu)
    trace = []

    def stop_at_loop(uc, address, size, user):
        del address, size, user
        cs = uc.reg_read(UC_X86_REG_CS) & 0xFFFF
        ip = uc.reg_read(UC_X86_REG_IP) & 0xFFFF
        trace.append((cs, ip))
        if (cs, ip) == (prior.CS, 0x0E30):
            uc.emu_stop()

    emu.hook_add(UC_HOOK_CODE, stop_at_loop)
    emu.emu_start(prior.physical(prior.CS, 0x0DEA), prior.RAM_SIZE,
                  count=400)
    assert trace[0] == (prior.CS, 0x0DEA)
    assert trace[-1] == (prior.CS, 0x0E30)
    for ip in (0x0DFF, 0x0E06, 0x0067, 0x00A8, 0x0E09, 0x0E0C):
        assert (prior.CS, ip) in trace
    frame = bytes(emu.mem_read(0, prior.RAM_SIZE))
    at = prior.cpu_from_emu(emu)
    count_at = prior.physical(at.ss, (at.bp - 0x56) & 0xFFFF)
    count = frame[count_at] | (frame[(count_at + 1) & prior.MASK] << 8)
    assert count == len(flags), (case, count)
    assert at.ip == 0x0E30
    return frame, at


def compare_case(parts, model, case):
    frame, cpu = frame_after_setup(parts, case)
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, prior.RAM_SIZE)
    original.mem_write(0, frame)
    prior.load_cpu(original, cpu)
    trace = []

    def stop_at_exit(uc, address, size, user):
        del address, size, user
        cs = uc.reg_read(UC_X86_REG_CS) & 0xFFFF
        ip = uc.reg_read(UC_X86_REG_IP) & 0xFFFF
        trace.append((cs, ip))
        if (cs, ip) == (prior.CS, 0x0E5F):
            uc.emu_stop()

    original.hook_add(UC_HOOK_CODE, stop_at_exit)
    original.emu_start(prior.physical(prior.CS, 0x0E30), prior.RAM_SIZE,
                       count=400)
    assert trace[0] == (prior.CS, 0x0E30)
    assert trace[-1] == (prior.CS, 0x0E5F), (case, trace[-10:])
    traversals = sum(ip == 0x0E59 for _, ip in trace)
    assert traversals == sum(bool(flag) for flag in PATTERNS[case[2]])
    expected_ram = bytes(original.mem_read(0, prior.RAM_SIZE))
    actual_ram = (ctypes.c_uint8 * prior.RAM_SIZE).from_buffer_copy(frame)
    actual_cpu = prior.Cpu.from_buffer_copy(cpu)
    assert model(actual_ram, prior.RAM_SIZE, ctypes.byref(actual_cpu),
                 8) == 0
    if bytes(actual_ram) != expected_ram:
        diff = [i for i, (a, b) in enumerate(zip(actual_ram, expected_ram))
                if a != b]
        raise AssertionError((case, "RAM", [(hex(i), hex(actual_ram[i]),
                                             hex(expected_ram[i]))
                                            for i in diff[:12]]))
    for (name, _), reg in zip(prior.Cpu._fields_, prior.REGISTERS):
        expected = original.reg_read(reg) & 0xFFFF
        actual = getattr(actual_cpu, name)
        assert actual == expected, (case, name, hex(actual), hex(expected))


def main():
    parts = sources()
    with tempfile.TemporaryDirectory(dir=ROOT / "resources") as temp:
        shared = Path(temp) / "u3b0_0e30.so"
        subprocess.run([
            "cc", "-shared", "-fPIC", "-std=c11", "-Wall", "-Wextra",
            "-Wpedantic", "-Werror", "-I", str(ROOT / "include"),
            str(ROOT / "src/recovered/overlay_u3b0_entry4_node_loop_0e30.c"),
            "-o", str(shared)], check=True)
        model = ctypes.CDLL(str(shared)).pr_u3b0_entry4_node_loop_0e30
        model.argtypes = (ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(prior.Cpu), ctypes.c_size_t)
        model.restype = ctypes.c_int
        cases = [(sp, lengths, pattern, coords)
                 for sp in (0x8000, 0x9001)
                 for lengths in ((0, 0), (44, 60))
                 for pattern in PATTERNS
                 for coords in ((5, 2), (0xFE, 0x7F))]
        for case in cases:
            compare_case(parts, model, case)
    print(f"U3B0 +0E30 node loop, two original copies/caller full RAM/14-register parity: {len(cases)} cases PASS")


if __name__ == "__main__":
    main()
