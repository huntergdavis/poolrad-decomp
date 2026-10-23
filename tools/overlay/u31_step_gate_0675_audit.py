#!/usr/bin/env python3
"""Original-byte full-state parity for u31 +0675 with U3C6 +0CB9 and resident RES_00BA_2199 composed.

The differential places the hash-checked expanded START.EXE image at load segment 0,
so the unrelocated far calls to 00BA:2199 reach the original resident bytes, and
patches the 03E0:0070 stub to its loaded form (EA to unit 0x3C6 +0CB9). It loads unit
0x31 and the three unit 0x3C6 spans that +0CB9 runs, then runs the original under
Unicorn and the composed C model on a copy. Every case compares the entire 1 MiB RAM
and all 14 CPU words. The DS world is the +1577 audit's +0CB9 world plus the +0675
tables, objects and sidecar lists. Media stays private.
"""
import argparse
import ctypes
from hashlib import sha256
import json
from pathlib import Path
import random
import struct
import subprocess
import tempfile

from capstone import CS_ARCH_X86, CS_MODE_16, Cs
from overlay_inventory import find_units, parse_mz
from resident_loader_map import unpack
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, OVR_SHA, REGISTERS, physical, put16
from u33_place_search_1577_audit import world, U3C6_BASE, U3C6_SPANS, STUB_3C6

U31_BASE = 0xABC0
ENTRY, END, RETF_AT = 0x0675, 0x07D6, 0x07D3
CALLS = [(0x06DA, "lcall", "0x3e0, 0x70")] + [(s, "lcall", "0xba, 0x2199") for s in (0x0758, 0x076F, 0x0786, 0x079D)]
KEYS = (0x20, 0x1E, 0x6F, 0x7D)
CALLER_CS, SCAN_CS, DS, SS = 0x3800, 0x3A00, 0x1800, 0x2800


def check(condition, message):
    if not condition:
        raise ValueError(message)


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, "START.EXE provenance differs")
    check(len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA, "GAME.OVR provenance differs")
    units = {u["stub_para"]: u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)}
    u31, u3c6 = units[0x31], units[0x3C6]
    check(u31["ovr_offset"] == U31_BASE and u31["memory_segment"] == 0x38, "unit 0x31 placement differs")
    check(u3c6["ovr_offset"] == U3C6_BASE and u3c6["memory_segment"] == STUB_3C6 and u3c6["entries"][16] == 0x0CB9,
          "unit 0x3C6 entry 16 differs")
    code31 = ovr[U31_BASE:U31_BASE + u31["code_size"]]
    code3c6 = ovr[U3C6_BASE:U3C6_BASE + u3c6["code_size"]]
    starts = sorted(f["rel"] for f in u31["functions"])
    decoder = Cs(CS_ARCH_X86, CS_MODE_16)
    body = code31[ENTRY:END]
    decoded = list(decoder.disasm(body, ENTRY))
    check(len(decoded) == 137 and len(body) == 353 and decoded[-1].address == RETF_AT and decoded[-1].mnemonic == "retf"
          and decoded[-1].op_str == "0xc" and ENTRY in starts, "+0675 extent differs")
    calls = [(i.address, i.mnemonic, i.op_str) for i in decoded if "call" in i.mnemonic or i.mnemonic.startswith("int")]
    check(calls == CALLS, f"+0675 calls differ: {calls}")
    fixups = set(u31["fixups"])
    check(all(site + 3 in fixups for site, _, _ in CALLS), "a far call segment word is not fixup-backed")
    sites = {i.address for i in decoded}
    check(all(int(i.op_str, 16) in sites for i in decoded if i.mnemonic.startswith("j")), "+0675 branch leaves the body")
    conditional = {i.address: (i.address + i.size, int(i.op_str, 16)) for i in decoded if i.mnemonic.startswith("j") and i.mnemonic != "jmp"}
    check(len(conditional) == 10, "+0675 conditional count differs")
    near = set()
    for function in starts:
        end = next((x for x in starts if x > function), len(code31))
        aligned = {i.address: i for i in decoder.disasm(code31[function:end], function)}
        for site, ins in aligned.items():
            if ins.mnemonic == "call" and ins.op_str.startswith("0x") and int(ins.op_str, 16) == ENTRY:
                check(aligned[site - 1].op_str == "cs", "+0675 caller lacks PUSH CS")
                near.add((function, site))
    check(near == {(0x07F2, 0x094B)}, f"+0675 aligned near callers differ: {sorted(near)}")
    image = bytes(unpack(exe)[0])
    check(sha256(image[0x2D39:0x2DA8]).hexdigest() == "721e86627eac3e72deed41d2a26283e7c95444d99fc4f762a9714b11504dfa9c",
          "resident RES_00BA_2199 bytes differ")
    target = dict(body=body, decoded=decoded, conditional=conditional, sha=sha256(body).hexdigest())
    print(f"u31 +0675 ({len(body)}/{len(decoded)}, LCALL 03E0:0070 to u3C6 +0CB9, 4 x LCALL 00BA:2199) "
          f"sha {target['sha'][:16]}...; caller +07F2@+094B")
    return target, code31, code3c6, image


CHECKER_ROWS = ('OVL_0031_0675',)


def library(directory):
    path = directory / "u31.so"
    sources = ["overlay_u31_step_gate_0675.c", "overlay_u3c6_0cb9_cpu.c", "resident_00ba_sidecar_lookup_2199_cpu.c"]
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2", "-fPIC", "-shared",
                    "-I", str(ROOT / "include"), *[str(ROOT / "src/recovered" / s) for s in sources],
                    "-o", str(path)], check=True, capture_output=True, text=True)
    f = ctypes.CDLL(str(path)).pr_u31_step_gate_0675

    class Callee(ctypes.Structure):
        _fields_ = [("site", ctypes.c_uint16), ("status", ctypes.c_int)]
    f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_uint, ctypes.POINTER(Callee)]
    f.restype = ctypes.c_int
    return f, Callee


def put_word(memory, segment, offset, value):
    at = physical(segment, offset & 0xFFFF)
    memory[at] = value & 0xFF
    memory[(at + 1) & (SIZE - 1)] = (value >> 8) & 0xFF


def fixture(code31, code3c6, image, rng, *, b=None, c=None, y6=None, y15=None, keys=None, p6_force=None,
            sp=0x8000, flags=0x7202, regs=None, background=None, grid_fill=None, r_in_frame=False, open_cells=False,
            weight=None, steer=False):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    memory[0:len(image)] = image
    at = physical(CALLER_CS, 0); memory[at:at + len(code31)] = code31
    for lo, hi in U3C6_SPANS:
        at = physical(SCAN_CS, lo); memory[at:at + hi - lo] = code3c6[lo:hi]
    at = physical(STUB_3C6, 0x70); memory[at:at + 5] = struct.pack("<BHH", 0xEA, 0x0CB9, SCAN_CS)
    data, count = world(rng, realistic=True)
    for i in range(0x600):
        data.setdefault(0x02AE + i, rng.randrange(256))
    for j in range(0x100):
        data.setdefault(0x2744 + 4 * j, rng.choice((0, 1, 2, 3, 5, 9, 0x40, 0xFE)))
    count = data.get(0x5F2A, count) or 1
    for offset, value in data.items():
        memory[physical(DS, offset)] = value
    grid = (0x8000, 0x0000)
    put16(memory, DS, 0x6716, grid[1]); put16(memory, DS, 0x6718, grid[0])
    for i in range(0x4E9):
        memory[physical(grid[0], i)] = grid_fill if grid_fill is not None else rng.choice((0x00, 0x1E, 0x1E, 0x02, 0x05, 0x07, 0x80, 0x01, 0x03))
    put16(memory, DS, 0x6A5C, 0x0100); put16(memory, DS, 0x6A5E, 0x5000)
    for i in range(0x400):
        memory[physical(0x5000, 0x0100 + i)] = rng.randrange(256)
    slot = rng.randrange(1, count + 1)
    if steer:                                                   # +0CB9 steps all run in range: class 1, zero pairs, origin (10, 10)
        for offset, value in ((0x5F2A + 4 * slot, 1), (0x5F27 + 4 * slot, 10), (0x5F28 + 4 * slot, 10)):
            memory[physical(DS, offset)] = value
        for i in range(8):
            memory[physical(DS, 0x2844 + 8 + i)] = 0
    x = (0x1100 + slot, 0x2222)
    y = (0x0200 + 16 * rng.randrange(0x80), 0x6000)
    put_word(memory, x[1], x[0] + 0x108, y[0]); put_word(memory, x[1], x[0] + 0x10A, y[1])
    memory[physical(y[1], y[0] + 6)] = rng.randrange(256) if y6 is None else y6
    memory[physical(y[1], y[0] + 0x15)] = rng.randrange(256) if y15 is None else y15
    chosen = keys if keys is not None else [k for k in KEYS if rng.random() < 0.3] + rng.sample(range(256), rng.randrange(0, 4))
    rng.shuffle(chosen)
    head = (0, 0)
    for i, key in reversed(list(enumerate(chosen))):
        node = (0x0040 + 0x20 * i + rng.randrange(0x10), 0x7000)
        memory[physical(node[1], node[0])] = key
        put_word(memory, node[1], node[0] + 5, head[0]); put_word(memory, node[1], node[0] + 7, head[1])
        head = node
    put_word(memory, x[1], x[0] + 0x7F, head[0]); put_word(memory, x[1], x[0] + 0x81, head[1])
    r = (sp - 0x30, SS) if r_in_frame else (0x0100 + rng.randrange(0x100), 0x6800)
    words = [0x7000, 0x7001, r[0], r[1], (rng.randrange(256) << 8) | (rng.randrange(256) if b is None else b),
             (rng.randrange(256) << 8) | (rng.randrange(256) if c is None else c), x[0], x[1]]
    frame = physical(SS, sp)
    for i, value in enumerate(words):
        put16(memory, SS, (sp + 2 * i) & 0xFFFF, value)
        memory[(frame + 2 * i) & (SIZE - 1)] = value & 0xFF
        memory[(frame + 2 * i + 1) & (SIZE - 1)] = value >> 8
    if p6_force is not None:                                    # make every cell +0CB9 can report the same class
        for i in range(0x4E9):
            memory[physical(grid[0], i)] = p6_force
        if weight is not None:
            memory[physical(DS, 0x2744 + 4 * p6_force)] = weight
    if open_cells:                                              # no occupant anywhere: P10 = 0
        for j in range(50 * 25):
            memory[physical(DS, 0x60DB + j)] = 0
    reg = regs or tuple(rng.randrange(0x10000) for _ in range(7))
    cpu = Cpu(*reg, sp, CALLER_CS, ENTRY, DS, rng.randrange(0x10000), SS, flags)
    return memory, cpu, dict(words=words[2:], r_in_frame=r_in_frame)


def compare(label, memory, cpu, target, function, Callee, budget=3000000, stop_site=None):
    """stop_site: a call site whose first byte the caller made non-original in `memory`; the original is stopped on
    reaching it (before executing it) and the model must stop there with CODE, every register and byte equal."""
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory)); original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    start = physical(cpu.cs, ENTRY); body_end = start + len(target["body"])
    ret = physical(0x7001, 0x7000)
    previous, visited, edges, returned = [None], set(), set(), [False]

    def instruction(emulator, address, size, user):
        del emulator, size, user
        here = address - start + ENTRY
        visited.add(here)
        if previous[0] in target["conditional"]:
            edges.add((previous[0], here))
        previous[0] = here

    def leave(emulator, address, size, user):
        del address, size, user
        returned[0] = True
        emulator.emu_stop()

    def outside(emulator, address, size, user):
        del emulator, address, size, user
        previous[0] = None

    original.hook_add(UC_HOOK_CODE, instruction, begin=start, end=body_end - 1)
    original.hook_add(UC_HOOK_CODE, outside, begin=0, end=start - 1)
    original.hook_add(UC_HOOK_CODE, outside, begin=body_end, end=ret - 1)
    original.hook_add(UC_HOOK_CODE, leave, begin=ret, end=ret)
    if stop_site is not None:
        original.hook_add(UC_HOOK_CODE, leave, begin=start + stop_site - ENTRY, end=start + stop_site - ENTRY)
    original.emu_start(start, 2 * SIZE, count=budget)
    check(returned[0], f"original did not return or reach the stop: {label}")
    if stop_site is not None:
        check((original.reg_read(REGISTERS[9]) & 0xFFFF) == stop_site, f"original did not stop at +{stop_site:04X}: {label}")
    expected = bytes(original.mem_read(0, SIZE))
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    callee = Callee()
    status = function(ram, SIZE, ctypes.byref(cpu), 1000, ctypes.byref(callee))
    check(status == (0 if stop_site is None else 2), f"C status {status} (callee +{callee.site:04X} status {callee.status}): {label}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f"RAM mismatch {label}: {[(hex(d), actual[d], expected[d]) for d in differences[:6]]} ({len(differences)} bytes)")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        got, want = getattr(cpu, name), original.reg_read(register) & 0xFFFF
        check(got == want, f"{name} mismatch {label}: C={got:04x}, original={want:04x}")
    return visited, edges, callee.site


def cases():
    spec = []
    for i in range(300):
        spec.append((f"random-{i}", dict(flags=(0x7202, 0x0202, 0x0ED7, 0x0647)[i % 4])))
    for i in range(40):
        spec.append((f"class-1e-{i}", dict(p6_force=0x1E, y6=0xFF - i, keys=[k for n, k in enumerate(KEYS) if (i >> n) & 1],
                                           open_cells=i % 4 != 3, weight=1 + i % 3, steer=i % 5 != 4)))
    for i in range(12):
        spec.append((f"class-1e-none-{i}", dict(p6_force=0x1E, y6=[0, 1, 2, 0x7F, 0x80, 0xFE, 0xFF][i % 7], keys=[],
                                                open_cells=True, weight=i % 3, steer=True)))
    for i in range(40):                                         # only the fourth key present: +079D's hit
        spec.append((f"class-1e-7d-{i}", dict(p6_force=0x1E, y6=0xFF, keys=[0x7D, (0x31 + 7 * i) & 0xFF], open_cells=i % 2 == 0,
                                              weight=2, steer=i % 3 != 2)))
    for value in (0x00, 0x01, 0x02, 0x03, 0x05, 0x07, 0x80, 0x17):
        spec.append((f"class-{value:02x}", dict(p6_force=value)))
    for sp in (0x0040, 0x0100, 0xFFC0, 0xFFE8):
        spec.append((f"sp-{sp:04x}", dict(sp=sp)))
    spec.append(("result-in-frame", dict(r_in_frame=True)))
    spec.append(("random-background", dict(background=bytes(random.Random(9).randbytes(SIZE)))))
    spec.append(("preserved-df-if", dict(flags=0x0603)))
    return spec


def differential(target, code31, code3c6, image, function, Callee):
    rng = random.Random(0x310675)
    visited, edges, count, sites = set(), set(), 0, {}
    for label, options in cases():
        memory, cpu, _ = fixture(code31, code3c6, image, rng, **options)
        v, e, site = compare(f"+0675 {label}", memory, cpu, target, function, Callee)
        visited |= v; edges |= e; count += 1; sites[site] = sites.get(site, 0) + 1
    missing_sites = sorted(hex(s) for s in set(i.address for i in target["decoded"]) - visited)
    missing_edges = sorted((hex(s), hex(o)) for s, outs in target["conditional"].items() for o in outs if (s, o) not in edges)
    print(f"+0675: {count} original-byte cases passed, full 1 MiB RAM and all 14 CPU words equal; "
          f"{len(visited)}/{len(target['decoded'])} instruction starts, {len(edges)}/{2 * len(target['conditional'])} conditional outcomes; "
          f"last composed call per case: {dict(sorted((f'+{k:04X}', v) for k, v in sites.items()))}")
    check(not missing_sites and not missing_edges, f"+0675 coverage incomplete: {missing_sites} {missing_edges}")
    stops = 0
    for label, site, options in stop_cases():                   # t44: the flags at each call site's CODE stop
        memory, cpu, _ = fixture(code31, code3c6, image, rng, **options)
        memory[physical(cpu.cs, site)] = 0x90
        compare(f"+0675 {label}", memory, cpu, target, function, Callee, stop_site=site)
        stops += 1
    print(f"+0675: {stops} CODE-stop cases passed at the four lookup sites (the OR AL,AL flags reach +076F/+0786/+079D)")
    return dict(returning=count, code_stops=stops, starts=len(visited), outcomes=len(edges))


def stop_cases():
    """CODE stops at the lookup calls: +0758 after CMP, and +076F/+0786/+079D after OR AL,AL (every key missing), each
    under several entry-flag seeds. +06DA's flags are IDIV's, written to the pinned DIVW/IDIVW values, which Unicorn does
    not follow; that stop is checked by the DOSBox-X oracle (u31_step_gate_0675_native.py) instead."""
    out = []
    for n, flags in enumerate((0x7202, 0x0ED7, 0x0647, 0x0203)):
        for site in (0x0758, 0x076F, 0x0786, 0x079D):
            out.append((f"code-{site:04x}-{n}", site, dict(flags=flags, p6_force=0x1E, y6=0xFF - n, keys=[], open_cells=True,
                                                          weight=1 + n % 3, steer=True)))
    return out


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compile-only", action="store_true",
                        help="build and bind the candidate library only; no media")
    parser.add_argument("--unicorn", action="store_true")
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if args.compile_only:  # same compiler command and symbol binding as the audit; no media
        with tempfile.TemporaryDirectory(prefix='poolrad-checker-') as _t:
            library(Path(_t))
        print('CHECKER_BUILD_OK')
        return
    target, code31, code3c6, image = source()
    report = dict(target=dict(sha256=target["sha"], bytes=len(target["body"]), instructions=len(target["decoded"])))
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-u31-0675-") as temp:
            function, Callee = library(Path(temp))
            if args.unicorn:
                report["unicorn"] = differential(target, code31, code3c6, image, function, Callee)
            if args.native:
                from u31_step_gate_0675_native import native_audit
                report["native"] = native_audit(target, code31, code3c6, image, function, Callee)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
