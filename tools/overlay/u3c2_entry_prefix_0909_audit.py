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

ENTRY, STOP = 0x0909, 0x0913
BODY_SHA = "38a673986257d14dbe4da3ede438a98c0b516884c6fc92c79228088a68dba9ed"
PREFIX_SHA = "fc6542ae7bd0b6f694715bb0984ef7bf2118607c0dff04566c1352b189f8214b"
SITES = (0x0909, 0x090A, 0x090C, 0x090F)


def source():
    media = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    if len(media) != 206105 or sha256(media).hexdigest() != OVR_SHA:
        raise ValueError("media provenance differs")
    body = media[197723:198443]
    if len(body) != 720 or sha256(body).hexdigest() != BODY_SHA:
        raise ValueError("assigned caller body differs")
    prefix = body[:STOP - ENTRY]
    if sha256(prefix).hexdigest() != PREFIX_SHA:
        raise ValueError("bounded prefix hash differs")
    code = body[:STOP - ENTRY + 2]
    instructions = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(code, ENTRY))
    actual = [(i.address, i.size, i.mnemonic, i.op_str) for i in instructions]
    expected = [
        (0x0909, 1, "push", "bp"),
        (0x090A, 2, "mov", "bp, sp"),
        (0x090C, 3, "sub", "sp, 0x20"),
        (0x090F, 4, "mov", "byte ptr [bp - 3], 0"),
        (0x0913, 2, "jmp", "0x918"),
    ]
    if actual != expected:
        raise ValueError("prefix/stop control-flow boundary differs")
    return code


def library(directory):
    output = directory / "prefix.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                    "-shared", "-fPIC", "-I", str(ROOT / "include"),
                    str(ROOT / "src/recovered/overlay_u3c2_entry_prefix_0909.c"),
                    "-o", str(output)], check=True, capture_output=True, text=True)
    lib = ctypes.CDLL(str(output))
    lib.pr_u3c2_entry_prefix_0909.argtypes = [
        ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu)]
    lib.pr_u3c2_entry_prefix_0909.restype = ctypes.c_int
    return lib


def make_case(code, *, value=0xA5, ss=0x2000, sp=0x8000, flags=0x0ED7,
              bp=0x7788, background=None):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    cpu = Cpu(0x1234, 0x2345, 0x3456, 0x4567, 0x5678, 0x6789,
              bp, sp, 0xF000, ENTRY, 0x1000, 0x3000, ss, flags)
    local = physical(ss, (sp - 5) & 0xFFFF)
    memory[local] = value
    code_at = physical(cpu.cs, ENTRY)
    memory[code_at:code_at + len(code)] = code
    saved = physical(ss, (sp - 2) & 0xFFFF)
    if any(code_at <= at < code_at + len(code) for at in (saved, (saved + 1) & (SIZE - 1), local)):
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
    if lib.pr_u3c2_entry_prefix_0909(ram, SIZE, ctypes.byref(cpu)):
        raise ValueError(f"C rejected valid case {label}")
    if bytes(ram) != bytes(backing):
        different = [i for i in range(SIZE) if ram[i] != backing[i]]
        raise ValueError(f"RAM differs in {label}: {different[:8]}")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        expected = original.reg_read(register) & 0xFFFF
        if getattr(cpu, name) != expected:
            raise ValueError(f"{name} differs in {label}: {getattr(cpu, name):04X}/{expected:04X}")
    return set(visited), cpu.flags


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--quick", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    code = source()
    cases, visited = 0, set()
    flag_counts = {name: [0, 0] for name in ("CF", "PF", "AF", "ZF", "SF", "OF")}
    with tempfile.TemporaryDirectory(prefix="poolrad-u3c2-entry-0909-") as directory:
        lib = library(Path(directory))

        def run(label, **options):
            nonlocal cases, visited
            memory, cpu = make_case(code, **options)
            seen, flags = compare(label, memory, cpu, code, lib)
            visited |= seen
            for name, bit in zip(flag_counts, (0, 2, 4, 6, 7, 11)):
                flag_counts[name][int(bool(flags & (1 << bit)))] += 1
            cases += 1

        for sp in range(256):
            run(f"low-sp-{sp}", sp=sp, value=sp)
        for sp in range(0x8002, 0x8022):
            run(f"overflow-{sp:X}", sp=sp)
        for flags in (0x0202, 0x0ED7, 0x0047, 0x0A93, 0x0602, 0x0002):
            for sp in (0, 1, 2, 0x21, 0x22, 0x8002, 0x8021, 0xFFFF):
                run(f"flags-{flags:X}-{sp:X}", sp=sp, flags=flags)
        stacks = (0, 1, 2, 3, 4, 5, 0x10, 0x11, 0x12, 0x13,
                  0x20, 0x21, 0x22, 0x7FFF, 0x8000, 0x8001, 0x8002,
                  0x8021, 0x8022, 0xFFF0, 0xFFFA, 0xFFFE, 0xFFFF)
        for ss in (0, 0x2000, 0xFFFF):
            for sp in stacks:
                for value in (0, 0xFF):
                    run(f"wrap-{ss:X}-{sp:X}-{value:X}", ss=ss, sp=sp, value=value)
        rng = random.Random(0x3C20909)
        for index in range(4 if args.quick else 64):
            run(f"seeded-{index}", value=rng.randrange(256), bp=rng.randrange(65536),
                sp=rng.randrange(65536), flags=rng.choice((0x0202, 0x0ED7, 0x0602)),
                background=rng.randbytes(SIZE))
    if visited != set(SITES) or any(not all(counts) for name, counts in flag_counts.items() if name != "AF"):
        raise ValueError("prefix site or defined-flag coverage incomplete")
    if flag_counts["AF"] != [cases, 0]:
        raise ValueError("subtracting 32 must leave defined AF zero")
    report = {
        "target": "OVL_03C2_0909", "bounded_prefix": True,
        "body_sha256": BODY_SHA, "prefix_sha256": PREFIX_SHA,
        "prefix_file_extent": [197723, 197733], "prefix_offset_extent": [ENTRY, STOP],
        "source_sites": len(SITES), "covered_sites": len(visited), "cases": cases,
        "defined_flag_counts_clear_set": flag_counts,
        "stop_before": "0913 JMP 0918", "branches_executed": 0,
        "compared_ram_bytes": SIZE, "compared_cpu_words": 14,
        "masked_flags": 0, "emulator_substitutions": 0,
        "undefined_flags": 0, "natural_frame_measured": False,
        "native_frame_measured": False, "full_body_covered": False,
        "strict_full_c_credit": 0,
    }
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
