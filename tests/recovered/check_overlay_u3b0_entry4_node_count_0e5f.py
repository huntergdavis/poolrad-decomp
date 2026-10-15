#!/usr/bin/env python3
"""Original two-copy/caller U3B0 +0E5F finite node-count parity."""

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
    "flag_loop", ROOT / "tests/recovered/check_overlay_u3b0_entry4_node_loop_0e30.py")
flag_loop = importlib.util.module_from_spec(spec)
spec.loader.exec_module(flag_loop)
prior = flag_loop.prior

COUNT_SHA = "aec72078f9e6e042dbc99d93858663fdfc84b48c034d699740a02e7102c6e863"


def sources():
    parts = flag_loop.sources()
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    assert len(ovr) == 206105 and sha256(ovr).hexdigest() == prior.OVR_SHA
    body = ovr[flag_loop.setup.BASE + 0x0E5F:flag_loop.setup.BASE + 0x0E97]
    assert len(body) == 56 and sha256(body).hexdigest() == COUNT_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x0E5F))
    assert len(ins) == 16 and sum(i.size for i in ins) == 56
    assert ins[0].address == 0x0E5F and ins[-1].address == 0x0E95
    assert not any(i.mnemonic in ("call", "lcall") for i in ins)
    return parts, body


def original_run(ram, cpu, start, stop, count):
    emu = Uc(UC_ARCH_X86, UC_MODE_16)
    emu.mem_map(0, prior.RAM_SIZE)
    emu.mem_write(0, ram)
    prior.load_cpu(emu, cpu)
    trace = []

    def stop_at_boundary(uc, address, size, user):
        del address, size, user
        cs = uc.reg_read(UC_X86_REG_CS) & 0xFFFF
        ip = uc.reg_read(UC_X86_REG_IP) & 0xFFFF
        trace.append((cs, ip))
        if (cs, ip) == (prior.CS, stop):
            uc.emu_stop()

    emu.hook_add(UC_HOOK_CODE, stop_at_boundary)
    emu.emu_start(prior.physical(prior.CS, start), prior.RAM_SIZE,
                  count=count)
    assert trace[0] == (prior.CS, start) and trace[-1] == (prior.CS, stop), trace[-10:]
    return bytes(emu.mem_read(0, prior.RAM_SIZE)), prior.cpu_from_emu(emu), trace


def compare_case(parts, body, model, case):
    frame, cpu = flag_loop.frame_after_setup(parts, case)
    ram = bytearray(frame)
    at = prior.physical(prior.CS, 0x0E5F)
    ram[at:at + len(body)] = body
    frame, cpu, prior_trace = original_run(bytes(ram), cpu, 0x0E30, 0x0E5F, 400)
    assert (prior.CS, 0x0E30) in prior_trace
    expected_ram, original_cpu, trace = original_run(frame, cpu, 0x0E5F, 0x0E97, 400)
    assert (prior.CS, 0x0E91) in trace
    traversals = sum(ip == 0x0E91 for _, ip in trace)
    assert traversals == len(flag_loop.PATTERNS[case[2]])

    actual_ram = (ctypes.c_uint8 * prior.RAM_SIZE).from_buffer_copy(frame)
    actual_cpu = prior.Cpu.from_buffer_copy(cpu)
    assert model(actual_ram, prior.RAM_SIZE, ctypes.byref(actual_cpu), 8) == 0
    if bytes(actual_ram) != expected_ram:
        diff = [i for i, (a, b) in enumerate(zip(actual_ram, expected_ram))
                if a != b]
        raise AssertionError((case, "RAM", [(hex(i), hex(actual_ram[i]),
                                             hex(expected_ram[i]))
                                            for i in diff[:12]]))
    for name, _ in prior.Cpu._fields_:
        actual = getattr(actual_cpu, name)
        expected = getattr(original_cpu, name)
        assert actual == expected, (case, name, hex(actual), hex(expected))
    count_at = prior.physical(actual_cpu.ss, (actual_cpu.bp - 0x8D) & 0xFFFF)
    count = actual_ram[count_at] | (actual_ram[(count_at + 1) & prior.MASK] << 8)
    assert count == traversals


def main():
    parts, body = sources()
    with tempfile.TemporaryDirectory(dir=ROOT / "resources") as temp:
        shared = Path(temp) / "u3b0_0e5f.so"
        subprocess.run([
            "cc", "-shared", "-fPIC", "-std=c11", "-Wall", "-Wextra",
            "-Wpedantic", "-Werror", "-I", str(ROOT / "include"),
            str(ROOT / "src/recovered/overlay_u3b0_entry4_node_count_0e5f.c"),
            "-o", str(shared)], check=True)
        model = ctypes.CDLL(str(shared)).pr_u3b0_entry4_node_count_0e5f
        model.argtypes = (ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(prior.Cpu), ctypes.c_size_t)
        model.restype = ctypes.c_int
        cases = [(sp, lengths, pattern, coords)
                 for sp in (0x8000, 0x9001)
                 for lengths in ((0, 0), (44, 60))
                 for pattern in flag_loop.PATTERNS
                 for coords in ((5, 2), (0xFE, 0x7F))]
        for case in cases:
            compare_case(parts, body, model, case)
    print(f"U3B0 +0E5F node count, two original copies/caller full RAM/14-register parity: {len(cases)} cases PASS")


if __name__ == "__main__":
    main()
