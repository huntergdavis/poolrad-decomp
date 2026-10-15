#!/usr/bin/env python3
"""Original two-copy/caller frame to U3B0 +0E0C setup parity."""

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
    "nonnull", ROOT / "tests/recovered/check_overlay_u3b0_entry4_nonnull_0dea.py")
prior = importlib.util.module_from_spec(spec)
spec.loader.exec_module(prior)

SETUP_SHA = "68b2baacca017ff0e375559e8ee1bebf3cb0c51b2e9e31fd83238685b35f6409"
CALLER_SHA = "d23c4f4e4838df0b499375696ce48731a564594277605903bee06c3ee1b1fdf8"
LEAF_SHA = "965144eab404415b7ce5d3f6e22d1c6991af4725dac559160302d89305468693"
BASE = 0x2C8CF


def sources():
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    assert len(ovr) == 206105 and sha256(ovr).hexdigest() == prior.OVR_SHA
    code, copy = prior.source()
    setup = ovr[BASE + 0x0E0C:BASE + 0x0E30]
    caller = ovr[BASE + 0x0DFF:BASE + 0x0E0C]
    leaf = ovr[BASE + 0x0067:BASE + 0x00AB]
    assert len(setup) == 36 and sha256(setup).hexdigest() == SETUP_SHA
    assert len(caller) == 13 and sha256(caller).hexdigest() == CALLER_SHA
    assert len(leaf) == 68 and sha256(leaf).hexdigest() == LEAF_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(setup, 0x0E0C))
    assert len(ins) == 14 and ins[0].address == 0x0E0C
    assert ins[-1].address == 0x0E2C and sum(i.size for i in ins) == 36
    return code, copy, caller, leaf, setup


def frame_after_caller(code, copy, caller, leaf, setup, case):
    ram, cpu = prior.two_copy_frame(code, copy, case)
    ram = bytearray(ram)
    at = prior.physical(prior.CS, 0x0DFF)
    ram[at:at + len(caller) + len(setup)] = caller + setup
    at = prior.physical(prior.CS, 0x0067)
    ram[at:at + len(leaf)] = leaf
    emu = Uc(UC_ARCH_X86, UC_MODE_16)
    emu.mem_map(0, prior.RAM_SIZE)
    emu.mem_write(0, bytes(ram))
    prior.load_cpu(emu, cpu)
    trace = []

    def stop_at_setup(uc, address, size, user):
        del address, size, user
        cs = uc.reg_read(UC_X86_REG_CS) & 0xFFFF
        ip = uc.reg_read(UC_X86_REG_IP) & 0xFFFF
        trace.append((cs, ip))
        if (cs, ip) == (prior.CS, 0x0E0C):
            uc.emu_stop()

    emu.hook_add(UC_HOOK_CODE, stop_at_setup)
    emu.emu_start(prior.physical(prior.CS, 0x0DEA), prior.RAM_SIZE,
                  count=300)
    assert trace[0] == (prior.CS, 0x0DEA)
    assert trace[-1] == (prior.CS, 0x0E0C)
    for ip in (0x0DFF, 0x0E06, 0x0067, 0x00A8, 0x0E09):
        assert (prior.CS, ip) in trace
    assert trace.count((prior.CS, 0x0067)) == 1
    returned = prior.cpu_from_emu(emu)
    assert returned.sp == (returned.bp - 0x018E) & 0xFFFF
    return bytes(emu.mem_read(0, prior.RAM_SIZE)), returned


def compare_case(code, copy, caller, leaf, setup, model, case):
    frame, cpu = frame_after_caller(code, copy, caller, leaf, setup, case)
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, prior.RAM_SIZE)
    original.mem_write(0, frame)
    prior.load_cpu(original, cpu)
    trace = []

    def record(uc, address, size, user):
        del address, size, user
        trace.append((uc.reg_read(UC_X86_REG_CS) & 0xFFFF,
                      uc.reg_read(UC_X86_REG_IP) & 0xFFFF))

    original.hook_add(UC_HOOK_CODE, record)
    original.emu_start(prior.physical(prior.CS, 0x0E0C), prior.RAM_SIZE,
                       count=14)
    assert len(trace) == 14 and trace[0] == (prior.CS, 0x0E0C), (case, trace)
    assert original.reg_read(UC_X86_REG_IP) & 0xFFFF == 0x0E30
    expected_ram = bytes(original.mem_read(0, prior.RAM_SIZE))
    actual_ram = (ctypes.c_uint8 * prior.RAM_SIZE).from_buffer_copy(frame)
    actual_cpu = prior.Cpu.from_buffer_copy(cpu)
    assert model(actual_ram, prior.RAM_SIZE, ctypes.byref(actual_cpu)) == 0
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
    code, copy, caller, leaf, setup = sources()
    with tempfile.TemporaryDirectory(dir=ROOT / "resources") as temp:
        shared = Path(temp) / "u3b0_0e0c.so"
        subprocess.run([
            "cc", "-shared", "-fPIC", "-std=c11", "-Wall", "-Wextra",
            "-Wpedantic", "-Werror", "-I", str(ROOT / "include"),
            str(ROOT / "src/recovered/overlay_u3b0_entry4_loop_setup_0e0c.c"),
            "-o", str(shared)], check=True)
        model = ctypes.CDLL(str(shared)).pr_u3b0_entry4_loop_setup_0e0c
        model.argtypes = (ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(prior.Cpu))
        model.restype = ctypes.c_int
        cases = [(sp, lengths, head, coordinates)
                 for sp in (0x8000, 0x9001)
                 for lengths in ((0, 0), (44, 60))
                 for head in ((0x2200, 0x5000), (0, 0x5000), (0x2200, 0))
                 for coordinates in ((5, 2), (0, 0), (0xFE, 0x7F),
                                     (0x80, 0x7F))]
        for case in cases:
            compare_case(code, copy, caller, leaf, setup, model, case)
    print(f"U3B0 +0E0C setup, original two-copy/caller full RAM/14-register parity: {len(cases)} cases PASS")


if __name__ == "__main__":
    main()
