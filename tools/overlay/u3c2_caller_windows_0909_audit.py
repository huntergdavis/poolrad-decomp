#!/usr/bin/env python3
"""Synthetic U3C2 argument-setup/CALL windows; stop before callee execution."""
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

BODY_SHA = "38a673986257d14dbe4da3ede438a98c0b516884c6fc92c79228088a68dba9ed"
WINDOWS = {
    0x0A71: (0x0AA5, "8f2e8a7f8d31d6e257b3be0471fcbf140fcd6a0c98ff455925750cb73cb2b344"),
    0x0B73: (0x0BA7, "0baa523f4b2a1dd5d0c083b9e623f446fffae163f6e9f424e949302a23ff4898"),
}
STOP = 0x057B


def source():
    media = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    if len(media) != 206105 or sha256(media).hexdigest() != OVR_SHA:
        raise ValueError("media provenance differs")
    base = 197723 - 0x0909
    body = media[base + 0x0909:base + 0x0BD9]
    decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x0909))
    if sha256(body).hexdigest() != BODY_SHA or len(decoded) != 297 or sum(i.size for i in decoded) != 720:
        raise ValueError("assigned caller body differs")
    windows = {}
    for entry, (end, digest) in WINDOWS.items():
        code = media[base + entry:base + end]
        sites = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(code, entry))
        if sha256(code).hexdigest() != digest or len(sites) != 24 or sum(i.size for i in sites) != 52:
            raise ValueError("caller window differs")
        if (sites[-1].address, sites[-1].mnemonic, sites[-1].op_str) != (end - 3, "call", "0x57b"):
            raise ValueError("near-call boundary differs")
        if any(i.mnemonic not in ("mov", "xor", "push", "call") for i in sites):
            raise ValueError("window gained a branch or dependency")
        windows[entry] = (code, tuple(i.address for i in sites))
    return windows, media[base + STOP:base + STOP + 1]


def library(directory):
    output = directory / "caller.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                    "-shared", "-fPIC", "-I", str(ROOT / "include"),
                    str(ROOT / "src/recovered/overlay_u3c2_caller_windows_0909.c"),
                    "-o", str(output)], check=True, capture_output=True, text=True)
    lib = ctypes.CDLL(str(output))
    lib.pr_u3c2_caller_windows_0909.argtypes = [
        ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu)]
    lib.pr_u3c2_caller_windows_0909.restype = ctypes.c_int
    return lib


def make_case(entry, code, stop_byte, *, index=0, ss=0x2000, sp=0x8000,
              bp=0x7000, flags=0x0ED7, background=None):
    memory = bytearray(bytes(range(256)) * (SIZE // 256)) if background is None else bytearray(background)
    cpu = Cpu(0xABCD, 0x2345, 0x3456, 0x4567, 0x5678, 0x6789,
              bp, sp, 0xF000, entry, 0x1000, 0x3000, ss, flags)
    first, last = (-3, 12) if entry == 0x0A71 else (-5, -2)
    memory[physical(ss, (bp + first) & 0xFFFF)] = index
    memory[physical(ss, (bp + first - 1) & 0xFFFF)] = (index + 1) & 255
    memory[physical(ss, (bp + last) & 0xFFFF)] = (index ^ 0xA5)
    at, stop_at = physical(cpu.cs, entry), physical(cpu.cs, STOP)
    memory[at:at + len(code)] = code
    memory[stop_at:stop_at + 1] = stop_byte
    for i in range(1, 8):
        dest = physical(ss, (sp - 2 * i) & 0xFFFF)
        for byte in (dest, (dest + 1) & (SIZE - 1)):
            if at <= byte < at + len(code) or byte == stop_at:
                raise ValueError("case violates immutable-code precondition")
    return memory, cpu


def compare(label, memory, cpu, code, sites, stop_byte, lib):
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    original.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing))
    original.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    entry = cpu.ip
    code_at, stop_at = physical(cpu.cs, entry), physical(cpu.cs, STOP)
    visited, reached_stop = [], [False]

    def hook(uc, address, size, user):
        del size, user
        ip = uc.reg_read(UC_X86_REG_IP)
        if address != physical(cpu.cs, ip) or ip not in (*sites, STOP):
            raise ValueError(f"unexpected transfer in {label}")
        if ip == STOP:
            reached_stop[0] = True
            uc.emu_stop()
        else:
            visited.append(ip)

    original.hook_add(UC_HOOK_CODE, hook)
    original.emu_start(code_at, 2 * SIZE, count=25)
    if not reached_stop[0] or tuple(visited) != sites:
        raise ValueError(f"finite caller boundary not reached in {label}")
    if bytes(backing[code_at:code_at + len(code)]) != code or bytes(backing[stop_at:stop_at + 1]) != stop_byte:
        raise ValueError(f"immutable code changed in {label}")
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    if lib.pr_u3c2_caller_windows_0909(ram, SIZE, ctypes.byref(cpu)):
        raise ValueError(f"C rejected valid case {label}")
    if bytes(ram) != bytes(backing):
        different = [i for i in range(SIZE) if ram[i] != backing[i]]
        raise ValueError(f"RAM differs in {label}: {different[:8]}")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        expected = original.reg_read(register) & 0xFFFF
        if getattr(cpu, name) != expected:
            raise ValueError(f"{name} differs in {label}: {getattr(cpu, name):04X}/{expected:04X}")
    return set(visited)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--quick", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    windows, stop_byte = source()
    counts, covered = {}, set()
    with tempfile.TemporaryDirectory(prefix="poolrad-u3c2-0909-") as directory:
        lib = library(Path(directory))
        for entry, (code, sites) in windows.items():
            count = 0

            def run(label, **options):
                nonlocal count, covered
                memory, cpu = make_case(entry, code, stop_byte, **options)
                covered |= compare(f"{entry:X}-{label}", memory, cpu, code, sites, stop_byte, lib)
                count += 1

            for index in range(256):
                run(f"index-{index}", index=index)
            stacks = (0, 1, 2, 3, 0x10, 0x11, 0x12, 0x13, 0xFFF0, 0xFFFA, 0xFFFE, 0xFFFF)
            for ss in (0, 0x2000, 0xFFFF):
                for sp in stacks:
                    for bp in (0, 0xFFF8):
                        run(f"wrap-{ss:X}-{sp:X}-{bp:X}", ss=ss, sp=sp, bp=bp, index=255)
                for bp in (0, 1, 0x7000, 0xFFF8):
                    for delta in (-24, -8, 0, 2, 8, 16):
                        run(f"alias-{ss:X}-{bp:X}-{delta}", ss=ss, bp=bp,
                            sp=(bp + delta) & 0xFFFF, index=0)
            for flags in (0x0202, 0x0ED7, 0x0047, 0x0A93, 0x0602, 0x0002):
                run(f"flags-{flags:X}", flags=flags)
            first = -3 if entry == 0x0A71 else -5
            for offset in range(len(code)):
                run(f"immutable-code-read-{offset}", ss=0xF000, bp=entry - first + offset, sp=0x7000)
            rng = random.Random(entry)
            for index in range(4 if args.quick else 64):
                run(f"seeded-{index}", index=rng.randrange(256),
                    bp=rng.randrange(65536), sp=rng.randrange(65536),
                    flags=rng.choice((0x0202, 0x0ED7, 0x0602)), background=rng.randbytes(SIZE))
            counts[f"{entry:04X}"] = count
    expected_sites = {site for _, sites in windows.values() for site in sites}
    if covered != expected_sites:
        raise ValueError("caller window site coverage incomplete")
    report = {
        "target": "OVL_03C2_0909", "bounded_windows": True,
        "body_sha256": BODY_SHA,
        "windows": [{"entry": entry, "end_exclusive": end, "sha256": digest,
                     "bytes": 52, "instructions": 24} for entry, (end, digest) in WINDOWS.items()],
        "cases": sum(counts.values()), "cases_by_entry": counts,
        "source_sites": len(expected_sites), "covered_sites": len(covered),
        "stop_before": "057B first callee instruction", "callee_instructions_executed": 0,
        "external_calls_executed": 0, "compared_ram_bytes": SIZE, "compared_cpu_words": 14,
        "masked_flags": 0, "emulator_substitutions": 0,
        "undefined_af_convention": "zero after XOR, matching Unicorn 2.1.4; native AF unclaimed",
        "entry_to_windows_reachability_measured": False,
        "natural_frame_measured": False, "native_frame_measured": False,
        "full_body_covered": False, "strict_full_c_credit": 0,
    }
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
