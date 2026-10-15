#!/usr/bin/env python3
"""Compare one JMP using frames produced by the pinned original entry prefix."""
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

ENTRY, JUMP, STOP = 0x0909, 0x0913, 0x0918
PREFIX_SITES = (0x0909, 0x090A, 0x090C, 0x090F)
BODY_SHA = "38a673986257d14dbe4da3ede438a98c0b516884c6fc92c79228088a68dba9ed"
PREFIX_SHA = "fc6542ae7bd0b6f694715bb0984ef7bf2118607c0dff04566c1352b189f8214b"
JUMP_SHA = "c3ce784032215a5bb8028beba705a6f0b97cfeb0ae5541802743e954da13169e"


def source():
    media = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    if len(media) != 206105 or sha256(media).hexdigest() != OVR_SHA:
        raise ValueError("media provenance differs")
    body = media[197723:198443]
    if len(body) != 720 or sha256(body).hexdigest() != BODY_SHA:
        raise ValueError("assigned caller body differs")
    code = body[:12]
    if sha256(code[:10]).hexdigest() != PREFIX_SHA or sha256(code[10:]).hexdigest() != JUMP_SHA:
        raise ValueError("original prefix/jump identity differs")
    decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(code, ENTRY))
    expected = [(0x0909, 1, "push", "bp"), (0x090A, 2, "mov", "bp, sp"),
                (0x090C, 3, "sub", "sp, 0x20"),
                (0x090F, 4, "mov", "byte ptr [bp - 3], 0"),
                (0x0913, 2, "jmp", "0x918")]
    if [(i.address, i.size, i.mnemonic, i.op_str) for i in decoded] != expected:
        raise ValueError("original frame producer or jump differs")
    stop_code = body[STOP - ENTRY:STOP - ENTRY + 3]
    stop_ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(stop_code, STOP))
    if [(i.address, i.size, i.mnemonic, i.op_str) for i in stop_ins] != [(STOP, 3, "mov", "al, byte ptr [bp + 0xa]")]:
        raise ValueError("successor stop instruction differs")
    return code, stop_code


def library(directory):
    output = directory / "jump.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                    "-shared", "-fPIC", "-I", str(ROOT / "include"),
                    str(ROOT / "src/recovered/overlay_u3c2_jump_successor_0913.c"),
                    "-o", str(output)], check=True, capture_output=True, text=True)
    lib = ctypes.CDLL(str(output))
    lib.pr_u3c2_jump_successor_0913.argtypes = [ctypes.POINTER(Cpu)]
    lib.pr_u3c2_jump_successor_0913.restype = ctypes.c_int
    return lib


def compare(label, code, stop_code, lib, *, ss=0x2000, sp=0x8000,
            bp=0x7788, flags=0x0ED7, background=None):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    cpu = Cpu(0x1234, 0x2345, 0x3456, 0x4567, 0x5678, 0x6789,
              bp, sp, 0xF000, ENTRY, 0x1000, 0x3000, ss, flags)
    code_at, jump_at, stop_at = (physical(cpu.cs, ip) for ip in (ENTRY, JUMP, STOP))
    memory[code_at:code_at + len(code)] = code
    memory[stop_at:stop_at + len(stop_code)] = stop_code
    saved = physical(ss, (sp - 2) & 0xFFFF)
    writes = (saved, (saved + 1) & (SIZE - 1), physical(ss, (sp - 5) & 0xFFFF))
    if any(code_at <= at < code_at + len(code) or stop_at <= at < stop_at + len(stop_code) for at in writes):
        raise ValueError("case violates immutable-code precondition")
    backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing))
    original.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    visited = []

    def prefix_hook(uc, address, size, user):
        del size, user
        ip = uc.reg_read(UC_X86_REG_IP)
        if address != physical(cpu.cs, ip) or ip not in (*PREFIX_SITES, JUMP):
            raise ValueError(f"unexpected prefix transfer in {label}")
        visited.append(ip)
        if ip == JUMP:
            uc.emu_stop()

    hook = original.hook_add(UC_HOOK_CODE, prefix_hook)
    original.emu_start(code_at, 2 * SIZE, count=5)
    original.hook_del(hook)
    if tuple(visited) != (*PREFIX_SITES, JUMP):
        raise ValueError(f"original entry did not produce a jump frame in {label}")
    frame_ram = bytes(backing)
    frame_cpu = Cpu(*(original.reg_read(reg) & 0xFFFF for reg in REGISTERS))
    candidate = Cpu.from_buffer_copy(frame_cpu)
    visited.clear()

    def jump_hook(uc, address, size, user):
        del size, user
        ip = uc.reg_read(UC_X86_REG_IP)
        if address != physical(cpu.cs, ip) or ip not in (JUMP, STOP):
            raise ValueError(f"unexpected jump transfer in {label}")
        visited.append(ip)
        if ip == STOP:
            uc.emu_stop()

    original.hook_add(UC_HOOK_CODE, jump_hook)
    # Resume the same original machine: no guest CPU/RAM edit after capture.
    original.emu_start(jump_at, 2 * SIZE, count=2)
    if visited != [JUMP, STOP] or bytes(backing) != frame_ram:
        raise ValueError(f"jump did not reach the finite stop unchanged in {label}")
    if bytes(backing[code_at:code_at + len(code)]) != code or bytes(backing[stop_at:stop_at + len(stop_code)]) != stop_code:
        raise ValueError(f"immutable code changed in {label}")
    if lib.pr_u3c2_jump_successor_0913(ctypes.byref(candidate)):
        raise ValueError(f"C rejected the original-produced frame in {label}")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        if getattr(candidate, name) != (original.reg_read(register) & 0xFFFF):
            raise ValueError(f"{name} differs in {label}")
    return frame_ram, frame_cpu


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--quick", action="store_true")
    parser.add_argument("--capture-frame", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    code, stop_code = source()
    cases, digest = 0, sha256()
    flag_counts = {name: [0, 0] for name in ("CF", "PF", "AF", "ZF", "SF", "OF")}
    with tempfile.TemporaryDirectory(prefix="poolrad-u3c2-jump-0913-") as directory:
        lib = library(Path(directory))

        def run(label, **options):
            nonlocal cases
            ram, cpu = compare(label, code, stop_code, lib, **options)
            digest.update(ram)
            digest.update(bytes(cpu))
            for name, bit in zip(flag_counts, (0, 2, 4, 6, 7, 11)):
                flag_counts[name][int(bool(cpu.flags & (1 << bit)))] += 1
            if not cases and args.capture_frame:
                target = ROOT / "resources/u3c2-jump-0913"
                target.mkdir(parents=True, exist_ok=True)
                (target / "first-original-frame.ram").write_bytes(ram)
                (target / "first-original-frame.json").write_text(json.dumps({
                    "provenance": "original four-instruction prefix from synthetic input",
                    "cpu": {name: getattr(cpu, name) for name, _ in Cpu._fields_},
                    "ram_sha256": sha256(ram).hexdigest(), "stop_before": "0913 JMP",
                    "native_frame": False,
                }, indent=2) + "\n")
            cases += 1

        stacks = (0, 1, 2, 3, 4, 5, 0x10, 0x11, 0x12, 0x13,
                  0x20, 0x21, 0x22, 0x7FFF, 0x8000, 0x8001, 0x8002,
                  0x8021, 0x8022, 0xFFF0, 0xFFFA, 0xFFFE, 0xFFFF)
        for ss in (0, 0x2000, 0xFFFF):
            for sp in stacks:
                for bp in (0, 0xFFFF):
                    run(f"frame-{ss:X}-{sp:X}-{bp:X}", ss=ss, sp=sp, bp=bp)
        for flags in (0x0202, 0x0ED7, 0x0047, 0x0A93, 0x0602, 0x0002):
            for sp in (0, 1, 2, 0x21, 0x22, 0x8002, 0x8021, 0xFFFF):
                run(f"flags-{flags:X}-{sp:X}", flags=flags, sp=sp)
        rng = random.Random(0x3C20913)
        for index in range(4 if args.quick else 64):
            run(f"seeded-{index}", bp=rng.randrange(65536), sp=rng.randrange(65536),
                flags=rng.choice((0x0202, 0x0ED7, 0x0602)), background=rng.randbytes(SIZE))
    if any(not all(counts) for name, counts in flag_counts.items() if name != "AF") or flag_counts["AF"] != [cases, 0]:
        raise ValueError("original-produced frame flag coverage incomplete")
    report = {
        "target": "OVL_03C2_0909", "bounded_jump": True,
        "jump_extent": [JUMP, 0x0915], "jump_file_extent": [197733, 197735],
        "jump_sha256": JUMP_SHA, "prefix_sha256": PREFIX_SHA, "body_sha256": BODY_SHA,
        "cases": cases, "original_frame_producer_sites": len(PREFIX_SITES),
        "source_sites": 1, "covered_sites": 1, "stop_before": "0918 first successor instruction",
        "original_frame_provenance": "original entry prefix from synthetic initial input",
        "original_frame_digest_sha256": digest.hexdigest(),
        "original_frame_flag_counts_clear_set": flag_counts,
        "guest_cpu_ram_edits_after_capture": 0, "prefix_c_executed": False,
        "successor_instructions_executed": 0, "compared_ram_bytes": SIZE,
        "compared_cpu_words": 14, "masked_flags": 0, "emulator_substitutions": 0,
        "native_frame_measured": False, "natural_frame_measured": False,
        "full_body_covered": False, "strict_full_c_credit": 0,
    }
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
