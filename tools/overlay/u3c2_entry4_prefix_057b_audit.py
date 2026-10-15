#!/usr/bin/env python3
"""Four-instruction U3C2 prefix oracle; synthetic RAM/CPU, no branch/native claim."""
import argparse
import ctypes
from hashlib import sha256
import json
from pathlib import Path
import random
import subprocess
import tempfile

from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_IP
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, OVR_SHA, REGISTERS, physical

ENTRY, STOP = 0x057B, 0x0585
BODY_SHA = "59750d0bfa553507f37af613b31ff0f733f2c63c584d768ed42b180122d1e2ae"
PREFIX_SHA = "a736c9b7c7903f828f6e9650eb3764471652435344933d60895c15ba95005802"
SITES = (0x057B, 0x057C, 0x057E, 0x0581)


def source():
    media = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    if len(media) != 206105 or sha256(media).hexdigest() != OVR_SHA:
        raise ValueError("media provenance differs")
    body = media[0x300CD:0x3045B]
    if len(body) != 910 or sha256(body).hexdigest() != BODY_SHA:
        raise ValueError("assigned entry4 span differs")
    prefix = body[:STOP - ENTRY]
    if sha256(prefix).hexdigest() != PREFIX_SHA:
        raise ValueError("bounded prefix hash differs")
    code = body[:STOP - ENTRY + 2]
    instructions = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(code, ENTRY))
    actual = [(i.address, i.size, i.mnemonic, i.op_str) for i in instructions]
    expected = [
        (0x057B, 1, "push", "bp"),
        (0x057C, 2, "mov", "bp, sp"),
        (0x057E, 3, "sub", "sp, 4"),
        (0x0581, 4, "cmp", "byte ptr [bp + 0xe], 0"),
        (0x0585, 2, "jl", "0x5b1"),
    ]
    if actual != expected:
        raise ValueError("prefix/stop control-flow boundary differs")
    return code


def library(directory):
    output = directory / "prefix.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                    "-shared", "-fPIC", "-I", str(ROOT / "include"),
                    str(ROOT / "src/recovered/overlay_u3c2_entry4_prefix_057b.c"),
                    "-o", str(output)], check=True, capture_output=True, text=True)
    lib = ctypes.CDLL(str(output))
    lib.pr_u3c2_entry4_prefix_057b.argtypes = [
        ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu)]
    lib.pr_u3c2_entry4_prefix_057b.restype = ctypes.c_int
    return lib


def make_case(code, *, value=0, ss=0x2000, sp=0x8000, flags=0x0ED7,
              bp=0x7788, background=None):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    cpu = Cpu(0x1234, 0x2345, 0x3456, 0x4567, 0x5678, 0x6789,
              bp, sp, 0xF000, ENTRY, 0x1000, 0x3000, ss, flags)
    memory[physical(ss, (sp + 12) & 0xFFFF)] = value
    code_at = physical(cpu.cs, ENTRY)
    memory[code_at:code_at + len(code)] = code
    saved = physical(ss, (sp - 2) & 0xFFFF)
    if any(code_at <= at < code_at + len(code) for at in (saved, (saved + 1) & (SIZE - 1))):
        raise ValueError("case violates immutable-code precondition")
    return memory, cpu

def compare(label, memory, cpu, code, lib):
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    original.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing))
    original.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    code_at = physical(cpu.cs, ENTRY)
    visited = []
    reached_stop = [False]

    def hook(uc, address, size, user):
        del size, user
        ip = uc.reg_read(UC_X86_REG_IP)
        if address != physical(cpu.cs, ip) or ip not in (*SITES, STOP):
            raise ValueError(f"unexpected transfer in {label}")
        if ip == STOP:
            reached_stop[0] = True
            uc.emu_stop()
        else:
            visited.append(ip)

    original.hook_add(UC_HOOK_CODE, hook)
    original.emu_start(code_at, 2 * SIZE, count=5)
    if not reached_stop[0] or tuple(visited) != SITES:
        raise ValueError(f"finite prefix boundary not reached in {label}")
    if bytes(backing[code_at:code_at + len(code)]) != code:
        raise ValueError(f"immutable code changed in {label}")
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    if lib.pr_u3c2_entry4_prefix_057b(ram, SIZE, ctypes.byref(cpu)):
        raise ValueError(f"C rejected valid case {label}")
    if bytes(ram) != bytes(backing):
        different = [i for i in range(SIZE) if ram[i] != backing[i]]
        raise ValueError(f"RAM differs in {label}: {different[:8]}")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        expected = original.reg_read(register) & 0xFFFF
        if getattr(cpu, name) != expected:
            raise ValueError(f"{name} differs in {label}: {getattr(cpu, name):04X}/{expected:04X}")
    return set(visited), bool(cpu.flags & 0x80)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--quick", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    code = source()
    cases, signed_counts, visited = 0, [0, 0], set()
    with tempfile.TemporaryDirectory(prefix="poolrad-u3c2-057b-") as directory:
        lib = library(Path(directory))

        def run(label, **options):
            nonlocal cases, visited
            memory, cpu = make_case(code, **options)
            seen, negative = compare(label, memory, cpu, code, lib)
            visited |= seen
            signed_counts[int(negative)] += 1
            cases += 1

        for value in range(256):
            run(f"byte-{value}", value=value)
        values = (0, 1, 3, 0x7F, 0x80, 0x81, 0xFE, 0xFF)
        for flags in (0x0202, 0x0ED7, 0x0047, 0x0A93, 0x0602, 0x0002):
            for value in values:
                run(f"flags-{flags:X}-{value:X}", flags=flags, value=value)
        stacks = (0, 1, 2, 3, 4, 5, 0x0010, 0x0011, 0x0012, 0x0013, 0xFFF0, 0xFFF2, 0xFFF3,
                  0xFFFA, 0xFFFB, 0xFFFC, 0xFFFD, 0xFFFE, 0xFFFF)
        for ss in (0, 0x2000, 0xFFFF):
            for sp in stacks:
                for value in ((0, 0x80) if args.quick else values):
                    run(f"wrap-{ss:X}-{sp:X}-{value:X}", ss=ss, sp=sp, value=value)
        for offset in range(len(code)):
            run(f"immutable-code-read-{offset}", ss=0xF000, sp=ENTRY - 12 + offset)
        rng = random.Random(0x3C2057B)
        for index in range(4 if args.quick else 64):
            run(f"seeded-{index}", value=rng.randrange(256), bp=rng.randrange(65536),
                flags=rng.choice((0x0202, 0x0ED7, 0x0602)), background=rng.randbytes(SIZE))
    if visited != set(SITES) or not all(signed_counts):
        raise ValueError("prefix instruction/compare-state coverage incomplete")
    report = {
        "target": "OVL_03C2_057B", "bounded_prefix": True,
        "body_sha256": BODY_SHA, "prefix_sha256": PREFIX_SHA,
        "prefix_file_extent": [0x300CD, 0x300D7],
        "prefix_offset_extent": [ENTRY, STOP], "source_sites": len(SITES),
        "covered_sites": len(visited), "cases": cases,
        "prebranch_nonnegative_cases": signed_counts[0],
        "prebranch_negative_cases": signed_counts[1],
        "stop_before": "0585 JL 05B1", "conditional_branches_executed": 0,
        "compared_ram_bytes": SIZE, "compared_cpu_words": 14,
        "masked_flags": 0, "emulator_substitutions": 0,
        "natural_frame_measured": False, "native_frame_measured": False,
        "full_body_covered": False, "strict_full_c_credit": 0,
    }
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
