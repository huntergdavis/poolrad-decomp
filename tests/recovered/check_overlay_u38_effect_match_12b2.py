#!/usr/bin/env python3
"""Original-byte full-state parity for U38 +12B2 calling U38 +0C06."""

import argparse
import ctypes
import json
import random
from hashlib import sha256
from pathlib import Path
import subprocess
import sys
import tempfile

from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE

import check_overlay_u38_target_select_101d as helper

CHECKER_ROWS = ("OVL_0038_12B2",)

ROOT = Path(__file__).resolve().parents[2]
BODY_SHA = "fdc58872484dc92452c0eb042ec51727f3543e989c6fed48c30876899ddc061b"
LEAF_SHA = "435bd9fac807d24f064b6bf223817b188e737a5b6ddb9a8b37d5635842a52fe8"


def source():
    ovr = helper.OVR.read_bytes()
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    assert len(ovr) == 206105 and sha256(ovr).hexdigest() == helper.OVR_SHA
    assert len(exe) == 65231 and sha256(exe).hexdigest() == helper.EXE_SHA
    sys.path.insert(0, str(ROOT / "tools/overlay"))
    from overlay_inventory import find_units, parse_mz
    unit = next(u for u in find_units(exe[parse_mz(exe)["header_bytes"]:],
                                    ovr) if u["stub_para"] == 0x38)
    assert unit["ovr_offset"] == 0xEC5B
    assert [i for i, off in enumerate(unit["entries"]) if off == 0x12B2] == [41]
    assert [i for i, off in enumerate(unit["entries"]) if off == 0x0C06] == [30]
    assert min(off for off in unit["entries"] if off > 0x12B2) == 0x12FB
    assert [off for off in unit["fixups"] if 0x12B2 <= off < 0x12FB] == []
    body = ovr[0xFF0D:0xFF56]
    leaf = ovr[0xF861:0xF870]
    assert len(body) == 73 and sha256(body).hexdigest() == BODY_SHA
    assert len(leaf) == 15 and sha256(leaf).hexdigest() == LEAF_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x12B2))
    assert sum(i.size for i in ins) == len(body)
    assert [(i.address, i.op_str) for i in ins if i.mnemonic == "call"] == [
        (0x12E8, "0xc06")]
    assert ins[-1].address == 0x12F8 and ins[-1].mnemonic == "retf"
    leaf_ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(leaf, 0x0C06))
    assert sum(i.size for i in leaf_ins) == len(leaf)
    assert leaf_ins[-1].address == 0x0C12 and leaf_ins[-1].mnemonic == "retf"
    return body, leaf


def put16(ram, seg, off, value):
    helper.put16(ram, seg, off & 0xFFFF, value)


def make_case(source_bytes, label, extra=None):
    body, leaf = source_bytes
    mode, ds, ss, sp, b3, a10e, old16, old22 = label
    ram = bytearray(helper.RAM_SIZE)
    at = helper.physical(0xF000, 0x12B2)
    ram[at:at + len(body)] = body
    at = helper.physical(0xF000, 0x0C06)
    ram[at:at + len(leaf)] = leaf
    cpu = helper.Cpu(0xA5CD, 0x3456, 0x5678, 0x789A, 0x9ABC,
                     0xBCDE, 0x7777, sp, 0xF000, 0x12B2, ds, 0x1111,
                     ss, 0x76D3)
    for i, word in enumerate((0x1234, 0xD000, 0xA145, 0x2200,
                              0x4000, 0x2100, 0x5000)):
        put16(ram, ss, sp + 2 * i, word)
    ram[helper.physical(0x4000, 0x2203)] = b3
    ram[helper.physical(0x5000, 0x220E)] = a10e
    ram[helper.physical(ds, 0x6816)] = old16
    ram[helper.physical(ds, 0x6822)] = old22
    extra = extra or {}
    if "flags" in extra:
        cpu.flags = extra["flags"]
    for name, value in extra.get("regs", {}).items():
        setattr(cpu, name, value)
    if "b" in extra:  # far pointer B at [BP+8]; byte +3 is read
        put16(ram, ss, sp + 6, extra["b"][1])
        put16(ram, ss, sp + 8, extra["b"][0])
    if "a" in extra:  # far pointer A at [BP+0xC]; byte +0x10E is read
        put16(ram, ss, sp + 10, extra["a"][1])
        put16(ram, ss, sp + 12, extra["a"][0])
    for seg, off, value in extra.get("poke", ()):
        ram[helper.physical(seg, off)] = value
    return ram, cpu


def compare(label, source_bytes, model, extra=None):
    memory, cpu = make_case(source_bytes, label, extra)
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    # Two mappings of one buffer reproduce A20-off physical wrap exactly as the C model's 20-bit mask does.
    backing = (ctypes.c_uint8 * helper.RAM_SIZE).from_buffer_copy(memory)
    original.mem_map_ptr(0, helper.RAM_SIZE, 7, ctypes.addressof(backing))
    original.mem_map_ptr(helper.RAM_SIZE, helper.RAM_SIZE, 7, ctypes.addressof(backing))
    for (name, _), reg in zip(helper.Cpu._fields_, helper.REGISTERS):
        original.reg_write(reg, getattr(cpu, name))
    reached = [False]
    calls = []
    seen, transitions = set(), set()
    previous = [None]

    def on_code(emu, address, size, user):
        del size, user
        seen.add(address)
        if previous[0] is not None:
            transitions.add((previous[0], address))
        previous[0] = address
        if address == 0xF0C06:
            calls.append(address)
        elif address == helper.physical(0xD000, 0x1234):
            reached[0] = True
            emu.emu_stop()
        else:
            assert (0xF12B2 <= address < 0xF12FB or
                    0xF0C06 <= address < 0xF0C15), (label, hex(address))

    original.hook_add(UC_HOOK_CODE, on_code)
    original.emu_start(0xF12B2, helper.RAM_SIZE, count=200)
    if extra and extra.get("alias"):
        # A pointer aliases state the wrapper itself writes or the counters, so the branch is data-dependent on
        # the run; C-vs-original parity below is the check, and the observed branch is returned for accounting.
        assert reached[0] and len(calls) in (0, 1), (label, reached[0], len(calls))
    else:
        expected_calls = int((label[4] & 0x10) >> 4 == label[5])
        assert reached[0] and len(calls) == expected_calls, (
            label, reached[0], len(calls))
    expected_ram = bytes(backing)
    ram = (ctypes.c_uint8 * helper.RAM_SIZE).from_buffer_copy(memory)
    assert model(ram, helper.RAM_SIZE, ctypes.byref(cpu)) == 0
    if bytes(ram) != expected_ram:
        diff = [i for i, (a, b) in enumerate(zip(ram, expected_ram)) if a != b]
        raise AssertionError((label, "RAM", diff[:20]))
    expected_cpu = helper.Cpu(*(original.reg_read(reg) & 0xFFFF
                                for reg in helper.REGISTERS))
    assert helper.snap(cpu) == helper.snap(expected_cpu), (
        label, helper.snap(cpu), helper.snap(expected_cpu))
    return seen, transitions, len(calls)


def build_model(directory):
    library = directory / "u38_12b2.so"
    subprocess.run(["cc", "-shared", "-fPIC", "-std=c11", "-Wall",
                    "-Wextra", "-Wpedantic", "-Werror", "-Iinclude",
                    "src/recovered/overlay_u38_effect_match_12b2.c",
                    "-o", str(library)], cwd=ROOT, check=True)
    model = ctypes.CDLL(str(library)).pr_u38_effect_match_12b2
    model.argtypes = (ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                      ctypes.POINTER(helper.Cpu))
    model.restype = ctypes.c_int
    return model


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compile-only", action="store_true",
                        help="compile, load and bind the actual C model without media")
    parser.add_argument("--exhaustive", action="store_true",
                        help="add the exhaustive data-byte, counter, flag, register and alias sweeps")
    parser.add_argument("--coverage", action="store_true",
                        help="report observed starts/outcomes for the unchanged 12 cases")
    args = parser.parse_args()
    if args.compile_only:
        with tempfile.TemporaryDirectory() as temp:
            build_model(Path(temp))
        print("CHECKER_BUILD_OK")
        return
    source_bytes = source()
    with tempfile.TemporaryDirectory() as temp:
        model = build_model(Path(temp))
        modes = ((0x00, 0, 0x7F, 0xFF),
                 (0x10, 1, 0xFF, 0x7F),
                 (0x00, 1, 0x00, 0x80),
                 (0x10, 0, 0x80, 0x00))
        layouts = ((0x3000, 0xE000, 0x8000),
                   (0x3000, 0x3000, 0x6830),
                   (0x3000, 0x3000, 0x0013))
        cases = [(mode, ds, ss, sp, b3, a10e, old16, old22)
                 for mode, (b3, a10e, old16, old22) in enumerate(modes)
                 for ds, ss, sp in layouts]
        seen, transitions = set(), set()
        for label in cases:
            starts, edges, _ = compare(label, source_bytes, model)
            seen.update(starts)
            transitions.update(edges)
        sweeps = {}
        if args.exhaustive:
            base = (0x00, 0x3000, 0xE000, 0x8000)
            ds, ss, sp = 0x3000, 0xE000, 0x8000
            allowed = 0x0ED5
            rng = random.Random(0x12B2)

            def run(group, label, extra=None):
                starts, edges, called = compare(label, source_bytes, model, extra)
                seen.update(starts)
                transitions.update(edges)
                entry = sweeps.setdefault(group, dict(cases=0, equal_branch=0))
                entry["cases"] += 1
                entry["equal_branch"] += called
            # Every value of both bytes the body reads: 256 x 256, all RAM and 14 CPU words compared each time.
            for b3 in range(256):
                for a10e in range(256):
                    run("data_bytes_256x256", (0, ds, ss, sp, b3, a10e, b3 ^ 0x55, a10e ^ 0xAA))
            # Counter edge values on both branches (INC/DEC overflow, zero, sign boundary).
            edge = (0, 1, 0x7F, 0x80, 0xFE, 0xFF)
            for old16 in edge:
                for old22 in edge:
                    run("counter_edges", (0, ds, ss, sp, 0x10, 1, old16, old22))
                    run("counter_edges", (0, ds, ss, sp, 0x00, 1, old16, old22))
            # Initial flags: each defined flag alone and cleared, plus random patterns (TF excluded: single-step trap).
            for flag in (0x0001, 0x0004, 0x0010, 0x0040, 0x0080, 0x0200, 0x0400, 0x0800):
                for b3 in (0x10, 0x00):
                    run("flags", (0, ds, ss, sp, b3, 1, 0x7F, 0x80), dict(flags=flag | 0x0002))
                    run("flags", (0, ds, ss, sp, b3, 1, 0x7F, 0x80), dict(flags=(allowed & ~flag) | 0x0002))
            for n in range(64):
                run("flags", (0, ds, ss, sp, rng.choice((0x10, 0x00)), 1, rng.randrange(256), rng.randrange(256)),
                    dict(flags=(rng.randrange(65536) & allowed) | 0x0002))
            # Other registers (the wrapper saves/restores BP and overwrites AX, DI, ES, CX, DX per path).
            for n in range(64):
                regs = {r: rng.randrange(65536) for r in ("ax", "bx", "cx", "dx", "si", "di", "es", "bp")}
                run("registers", (0, ds, ss, sp, rng.choice((0x10, 0x00)), 1, rng.randrange(256), rng.randrange(256)), dict(regs=regs))
            # Aliased and wrapped far pointers. The counters live at DS:6816 and DS:6822.
            def alias(name, a=None, b=None, poke=(), layout=(ds, ss, sp), b3=0x10, a10e=1, old=(1, 0)):
                extra = dict(alias=True, poke=poke)
                if a is not None:
                    extra["a"] = a
                if b is not None:
                    extra["b"] = b
                run("aliases", (0, layout[0], layout[1], layout[2], b3, a10e, old[0], old[1]), extra)
            off = lambda target, add: (target - add) & 0xFFFF
            for old in ((0, 0), (1, 0), (0, 1), (1, 1), (0xFF, 0xFF)):
                alias("A+10E == counter 6816", a=(ds, off(0x6816, 0x10E)), old=old)
                alias("A+10E == counter 6822", a=(ds, off(0x6822, 0x10E)), old=old)
                alias("B+3 == counter 6816", b=(ds, off(0x6816, 3)), old=old)
                alias("B+3 == counter 6822", b=(ds, off(0x6822, 3)), old=old)
                alias("A+10E and B+3 same cell", a=(ds, off(0x6816, 0x10E)), b=(ds, off(0x6816, 3)), old=old)
            alias("A == B", a=(0x4000, 0x2203 - 0x10E), b=(0x4000, 0x2200), poke=((0x4000, 0x2203, 0x10),))
            alias("A off wraps 16-bit", a=(0x5000, 0xFF00), poke=((0x5000, 0x000E, 1),))
            alias("B off wraps 16-bit", b=(0x4000, 0xFFFE), poke=((0x4000, 0x0001, 0x10),))
            # FFFF:0000 + 10Eh is physical 1000FE, which wraps to 0000FE; FFFF:0010 + 3 is 100003 -> 000003.
            alias("A20 wrap A", a=(0xFFFF, 0x0000), poke=((0x0000, 0x00FE, 1),))
            alias("A20 wrap B", b=(0xFFFF, 0x0010), poke=((0x0000, 0x0003, 0x10),))
            alias("A and B read the wrapper code", a=(0xF000, 0x12B2), b=(0xF000, 0x12B2))
            alias("B reads the return frame", b=(ss, sp - 3 + 0x10))
            alias("A reads the argument frame", a=(ss, (sp + 4 - 0x10E) & 0xFFFF))
            alias("DS == SS aliasing", layout=(0x3000, 0x3000, 0x6830), a=(0x3000, off(0x6816, 0x10E)))
    total = len(cases) + sum(g["cases"] for g in sweeps.values())
    print(f"U38 +12B2 plus +0C06: {len(cases)} full-RAM/14-CPU original-byte cases pass")
    if sweeps:
        print(json.dumps(dict(exhaustive_total_cases=total, groups=sweeps), sort_keys=True))
    if args.coverage:
        md = Cs(CS_ARCH_X86, CS_MODE_16)
        wrapper = list(md.disasm(source_bytes[0], 0xF12B2))
        leaf = list(md.disasm(source_bytes[1], 0xF0C06))
        outcomes = {(i.address, target)
                    for i in wrapper + leaf
                    if i.mnemonic.startswith("j") and i.mnemonic != "jmp"
                    for target in (i.address + i.size, int(i.op_str, 16))}
        wrapper_starts = {i.address for i in wrapper}
        leaf_starts = {i.address for i in leaf}
        print(json.dumps(dict(cases=total, ram_bytes=helper.RAM_SIZE,
            cpu_fields=len(helper.Cpu._fields_),
            wrapper_starts=len(seen & wrapper_starts), wrapper_total=len(wrapper),
            callee_starts=len(seen & leaf_starts), callee_total=len(leaf),
            conditional_outcomes=len(transitions & outcomes),
            conditional_total=len(outcomes),
            missing_starts=sorted((wrapper_starts | leaf_starts) - seen),
            missing_outcomes=sorted(outcomes - transitions)), sort_keys=True))


if __name__ == "__main__":
    main()
