#!/usr/bin/env python3
"""Complete U38 installer source/caller and full-state parity audit."""

import argparse
from collections import deque
import ctypes
from hashlib import sha256
import json
from pathlib import Path
import random
import re
import subprocess
import tempfile

from capstone import CS_ARCH_X86, CS_MODE_16, Cs
from unicorn import Uc, UC_ARCH_X86, UC_HOOK_CODE, UC_MODE_16
from unicorn.x86_const import UC_X86_REG_IP

from overlay_inventory import find_units, parse_mz
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, OVR_SHA, REGISTERS, physical, put16

ENTRY, END = 0x31A2, 0x38A9
BODY_SHA = "4a60eda9584cb62276f52b14e2ae7af2077857c521f3d5a835a344f2f50b2519"
SEGMENT_SITES = [0x31A9 + 13 * i for i in range(138)]


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    if len(exe) != 65231 or sha256(exe).hexdigest() != EXE_SHA:
        raise ValueError("START.EXE provenance differs")
    if len(ovr) != 206105 or sha256(ovr).hexdigest() != OVR_SHA:
        raise ValueError("GAME.OVR provenance differs")
    units = find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)
    unit = next(u for u in units if u["stub_para"] == 0x38)
    body = ovr[unit["ovr_offset"] + ENTRY:unit["ovr_offset"] + END]
    dec = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    if len(body) != 1799 or sha256(body).hexdigest() != BODY_SHA or len(dec) != 556 or sum(i.size for i in dec) != 1799:
        raise ValueError("U38 installer extent differs")
    if [i.mnemonic for i in dec[:2]] != ["push", "mov"] or [i.mnemonic for i in dec[-2:]] != ["pop", "retf"]:
        raise ValueError("installer frame differs")
    if unit["entries"][1] != ENTRY or unit["memory_segment"] != 0x41:
        raise ValueError("installer entry/stub differs")
    targets, segments, destinations = [], [], []
    for index in range(138):
        group = dec[2 + 4 * index:6 + 4 * index]
        destination = 0x682C + 4 * index + (4 if index >= 87 else 0)
        if any(i.mnemonic != "mov" for i in group) or group[0].address != 0x31A5 + 13 * index:
            raise ValueError("installer operation order differs")
        if not group[0].op_str.startswith("ax, ") or not group[1].op_str.startswith("dx, "):
            raise ValueError("pointer immediate registers differ")
        if group[2].op_str != f"word ptr [{destination:#x}], ax" or group[3].op_str != f"word ptr [{destination+2:#x}], dx":
            raise ValueError("pointer destination or skipped slot differs")
        targets.append(int(group[0].op_str.split(", ")[1], 16))
        segments.append(int(group[1].op_str.split(", ")[1], 16))
        destinations.append(destination)
    if {p for p in unit["fixups"] if ENTRY <= p < END} != set(SEGMENT_SITES):
        raise ValueError("installer relocation set differs")
    if [i for i, seg in enumerate(segments) if seg == 0x68] != [72, 73] or any(seg not in (0x41, 0x68) for seg in segments):
        raise ValueError("mixed-unit pointer segments differ")
    by_segment = {u["memory_segment"]: u for u in units}
    for segment, offset in zip(segments, targets):
        target_unit = by_segment[segment]
        slot, remainder = divmod(offset - 0x20, 5)
        if remainder or not 0 <= slot < len(target_unit["entries"]):
            raise ValueError("installed pointer does not resolve to an overlay entry")
    if len(set(zip(segments, targets))) != 117:
        raise ValueError("distinct pointer-target set differs")
    caller = next(u for u in units if u["stub_para"] == 0x36)
    code = ovr[caller["ovr_offset"]:caller["ovr_offset"] + caller["code_size"]]
    decoded_caller = {i.address: i for i in Cs(CS_ARCH_X86, CS_MODE_16).disasm(code[0x2D:0x615], 0x2D)}
    if caller["entries"][1] != 0x82D or decoded_caller[0x60C].op_str != "0x41, 0x25" or 0x60F not in caller["fixups"]:
        raise ValueError("normalized caller entry or far-call fixup differs")
    if decoded_caller[0x607].op_str != "0x96, 0x52" or 0x60A not in caller["fixups"]:
        raise ValueError("preceding U86 initializer call differs")
    pending, seen, path = deque([(0x2D, [])]), set(), None
    while pending:
        ip, trace = pending.popleft()
        if ip in seen or ip not in decoded_caller:
            continue
        seen.add(ip)
        instruction = decoded_caller[ip]
        trace = trace + [ip]
        if ip == 0x60C:
            path = trace
            break
        if instruction.mnemonic.startswith("ret"):
            continue
        if instruction.mnemonic.startswith("j"):
            if not re.fullmatch(r"0x[0-9a-f]+", instruction.op_str):
                continue
            pending.append((int(instruction.op_str, 16), trace))
            if instruction.mnemonic == "jmp":
                continue
        pending.append((ip + instruction.size, trace))  # Calls assumed to return.
    if path is None or 0x607 not in path:
        raise ValueError("initializer call suffix not structurally reachable")
    return body, {i.address: i for i in dec}, dict(segments=segments, targets=targets,
        destinations=destinations, caller_suffix=code[0x607:0x611], caller_path=path)


CHECKER_ROWS = ('OVL_0038_31A2',)


def library(directory):
    path = directory / "u38_31a2.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
        "-fPIC", "-shared", "-I", str(ROOT / "include"),
        str(ROOT / "src/recovered/overlay_u38_effects_install_31a2.c"), "-o", str(path)],
        check=True, capture_output=True, text=True)
    lib = ctypes.CDLL(str(path))
    lib.pr_u38_effects_install_31a2.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu)]
    lib.pr_u38_effects_install_31a2.restype = ctypes.c_int
    return lib


def make_case(body, segments, *, ds=0x1000, ss=0x2000, sp=0x8000,
              base=0, operands=None, flags=0x0202, background=None):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    cpu = Cpu(0xABCD, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE,
              0x7777, sp, 0xF000, ENTRY, ds, 0x1111, ss, flags)
    at = physical(cpu.cs, ENTRY)
    memory[at:at + len(body)] = body
    values = operands if operands is not None else [(seg + base) & 0xFFFF for seg in segments]
    for ip, value in zip(SEGMENT_SITES, values):
        put16(memory, cpu.cs, ip, value)
    at = physical(ss, sp)
    for i, b in enumerate((0x12, 7, cpu.cs & 255, cpu.cs >> 8)):
        memory[(at + i) & (SIZE - 1)] = b
    return memory, cpu


def compare(label, memory, cpu, decoded, lib):
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    original.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing))
    original.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        original.reg_write(reg, getattr(cpu, name))
    entry = physical(cpu.cs, ENTRY)
    expected_code = memory[entry:entry + END - ENTRY]
    visited, at_return = set(), [False]

    def on_instruction(uc, address, size, user):
        del size, user
        ip = uc.reg_read(UC_X86_REG_IP)
        if not entry <= address < entry + END - ENTRY or ip not in decoded:
            raise ValueError(f"unexpected control transfer: {label}")
        visited.add(ip)
        if ip == END - 1:
            at_return[0] = True
            uc.emu_stop()

    hook = original.hook_add(UC_HOOK_CODE, on_instruction)
    original.emu_start(entry, 2 * SIZE, count=1000)
    if not at_return[0]:
        raise ValueError(f"original did not return: {label}")
    original.hook_del(hook)
    original.emu_start(physical(cpu.cs, END - 1), 2 * SIZE, count=1)
    if bytes(backing[entry:entry + END - ENTRY]) != expected_code:
        raise ValueError(f"immutable source changed: {label}")
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    if lib.pr_u38_effects_install_31a2(ram, SIZE, ctypes.byref(cpu)):
        raise ValueError(f"C rejected input: {label}")
    actual, expected = bytes(ram), bytes(backing)
    if actual != expected:
        diffs = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f"RAM differs {label}: {diffs[:8]}")
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        expected_word = original.reg_read(reg) & 0xFFFF
        if getattr(cpu, name) != expected_word:
            raise ValueError(f"{name} differs {label}: {getattr(cpu, name):04X}/{expected_word:04X}")
    return visited


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--quick", action="store_true")
    parser.add_argument("--output", type=Path)
    parser.add_argument("--compile-only", action="store_true",
                        help="build and bind the candidate library only; no media")
    args = parser.parse_args()
    if args.compile_only:  # same compiler command and symbol binding as the audit; no media
        with tempfile.TemporaryDirectory(prefix='poolrad-checker-') as _t:
            library(Path(_t))
        print('CHECKER_BUILD_OK')
        return
    body, decoded, metadata = source()
    visited, count = set(), 0
    with tempfile.TemporaryDirectory(prefix="poolrad-u38-31a2-") as tmp:
        lib = library(Path(tmp))

        def run(label, **options):
            nonlocal visited, count
            memory, cpu = make_case(body, metadata["segments"], **options)
            visited |= compare(label, memory, cpu, decoded, lib)
            count += 1

        for base in (0, 0x824, 0x14A0, 0xFFFF, 0xFFBF, 0xF000):
            run(f"loader-{base:04X}", base=base, flags=0x0ED7)
        for sp in range(0x6828, 0x6A5D, 8 if args.quick else 1):
            run(f"frame-table-{sp:04X}", ss=0x1000, sp=sp, flags=0x0ED7)
        for ds in (0, 0xF000, 0xFFFF):
            for ss in (0, 0xFFFF):
                for sp in (0, 1, 2, 3, 4, 0xFFFD, 0xFFFE, 0xFFFF):
                    run(f"wrap-{ds:04X}-{ss:04X}-{sp:04X}", ds=ds, ss=ss, sp=sp)
        rng = random.Random(0x3831A2)
        for index in range(8 if args.quick else 128):
            run(f"seeded-{index}", operands=[rng.randrange(0x10000) for _ in SEGMENT_SITES],
                flags=rng.choice((0x0202, 0x0ED7, 0x0047, 0x0A93)), background=rng.randbytes(SIZE))
    report = dict(target="OVL_0038_31A2", source_sha256=BODY_SHA, cases=count,
                  covered_sites=len(visited), source_sites=len(decoded),
                  unvisited_sites=[f"{ip:04X}" for ip in sorted(set(decoded) - visited)],
                  compared_ram_bytes=SIZE, compared_cpu_words=14, masked_flags=0,
                  pointer_installs=138, distinct_original_pointers=117, segment_fixups=138,
                  skipped_slot="DS:6988..698B", structural_caller_path_sites=len(metadata["caller_path"]),
                  caller_calls_assumed_to_return=True, natural_game_route_measured=False)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
