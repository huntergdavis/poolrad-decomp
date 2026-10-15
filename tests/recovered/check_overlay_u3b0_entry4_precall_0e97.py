#!/usr/bin/env python3
"""Original reached U3B0 +0E97 prefix parity to first near call."""

import ctypes
from hashlib import sha256
import importlib.util
from pathlib import Path
import subprocess
import tempfile

from capstone import Cs, CS_ARCH_X86, CS_MODE_16

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location(
    "node_count", ROOT / "tests/recovered/check_overlay_u3b0_entry4_node_count_0e5f.py")
node_count = importlib.util.module_from_spec(spec)
spec.loader.exec_module(node_count)
prior = node_count.prior

PREFIX_SHA = "cd205b64caeb9a17a2f23f4221798e28eb432a44420909d7db6fc28a521d146d"
PAIRS = ((0, 0), (5, 2), (2, 5), (0xFFFF, 0x8000),
         (0x8000, 0xFFFF), (0x7FFF, 0x7FFF), (0x8000, 0x7FFF),
         (0x7FFF, 0x8000))


def sources():
    parts, count_body = node_count.sources()
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    assert len(ovr) == 206105 and sha256(ovr).hexdigest() == prior.OVR_SHA
    body = ovr[node_count.flag_loop.setup.BASE + 0x0E97:
               node_count.flag_loop.setup.BASE + 0x0ED2]
    assert len(body) == 59 and sha256(body).hexdigest() == PREFIX_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x0E97))
    assert len(ins) == 22 and sum(i.size for i in ins) == 59
    assert ins[0].address == 0x0E97 and ins[-1].address == 0x0ED1
    assert not any(i.mnemonic in ("call", "lcall") for i in ins)
    return parts, count_body, body


def reached_frame(parts, count_body, prefix_body, case):
    frame, cpu = node_count.flag_loop.frame_after_setup(parts, case)
    ram = bytearray(frame)
    for ip, body in ((0x0E5F, count_body), (0x0E97, prefix_body)):
        at = prior.physical(prior.CS, ip)
        ram[at:at + len(body)] = body
    frame, cpu, _ = node_count.original_run(bytes(ram), cpu,
                                             0x0E30, 0x0E5F, 400)
    frame, cpu, _ = node_count.original_run(frame, cpu,
                                             0x0E5F, 0x0E97, 400)
    return frame, cpu


def read16(ram, segment, offset):
    at = prior.physical(segment, offset)
    return ram[at] | (ram[(at + 1) & prior.MASK] << 8)


def compare_case(parts, count_body, prefix_body, model, case):
    sp, lengths, pattern, coords, global_word, pointed_word = case
    frame, cpu = reached_frame(parts, count_body, prefix_body,
                               (sp, lengths, pattern, coords))
    ram = bytearray(frame)
    pointer_offset = read16(ram, cpu.ss, cpu.bp + 0x0A)
    pointer_segment = read16(ram, cpu.ss, cpu.bp + 0x0C)
    flag_offset = read16(ram, cpu.ss, cpu.bp + 0x0E)
    flag_segment = read16(ram, cpu.ss, cpu.bp + 0x10)
    assert prior.physical(pointer_segment, pointer_offset) != prior.physical(cpu.ds, 0x6B29)
    prior.put16(ram, cpu.ds, 0x6B29, global_word)
    prior.put16(ram, pointer_segment, pointer_offset, pointed_word)
    ram[prior.physical(flag_segment, flag_offset)] = 0
    frame = bytes(ram)

    expected_ram, original_cpu, trace = node_count.original_run(
        frame, cpu, 0x0E97, 0x0ED2, 80)
    branches = ((prior.CS, 0x0EA2) in trace,
                (prior.CS, 0x0EBB) in trace)
    actual_ram = (ctypes.c_uint8 * prior.RAM_SIZE).from_buffer_copy(frame)
    actual_cpu = prior.Cpu.from_buffer_copy(cpu)
    assert model(actual_ram, prior.RAM_SIZE, ctypes.byref(actual_cpu)) == 0
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
    assert actual_cpu.ip == 0x0ED2 and actual_cpu.sp == ((cpu.sp - 6) & 0xFFFF)
    return branches


def main():
    parts, count_body, prefix_body = sources()
    with tempfile.TemporaryDirectory(dir=ROOT / "resources") as temp:
        shared = Path(temp) / "u3b0_0e97.so"
        subprocess.run([
            "cc", "-shared", "-fPIC", "-std=c11", "-Wall", "-Wextra",
            "-Wpedantic", "-Werror", "-I", str(ROOT / "include"),
            str(ROOT / "src/recovered/overlay_u3b0_entry4_precall_0e97.c"),
            "-o", str(shared)], check=True)
        model = ctypes.CDLL(str(shared)).pr_u3b0_entry4_precall_0e97
        model.argtypes = (ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(prior.Cpu))
        model.restype = ctypes.c_int
        cases = [(sp, lengths, pattern, coords, global_word, pointed_word)
                 for sp in (0x8000, 0x9001)
                 for lengths in ((0, 0), (44, 60))
                 for pattern in ("zero", "three_one")
                 for coords in ((5, 2), (0xFE, 0x7F))
                 for global_word, pointed_word in PAIRS]
        coverage = set()
        for case in cases:
            coverage.add(compare_case(parts, count_body, prefix_body,
                                      model, case))
        assert coverage == {(False, False), (False, True),
                            (True, False), (True, True)}, coverage
    print(f"U3B0 +0E97 precall reached-original full RAM/14-register parity: {len(cases)} cases, all 4 branch pairs PASS")


if __name__ == "__main__":
    main()
