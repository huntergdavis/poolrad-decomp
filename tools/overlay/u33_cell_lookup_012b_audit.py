#!/usr/bin/env python3
"""Original-byte full-state parity for u33 +012B and +01AD with the u3BD lookups folded.

The +012B body reaches unit 0x3BD entries 3 (+0350) and 2 (+047B) through
the overlay stubs 03D5:002F and 03D5:002A; +01AD calls +012B twice. The
differential loads the original unit 0x33 span, the loaded-state stubs and
the original unit 0x3BD span (+031F..+0579) into one 1 MiB image, runs the
original under Unicorn and the composed C on a copy, and compares the entire
RAM and all 14 CPU words. The folded entry 3 and entry 2 models are also run
as direct targets so every folded instruction is covered. Media stays private.
"""
import argparse
import ctypes
from hashlib import sha256
import json
from pathlib import Path
import random
import struct
import tempfile

from capstone import CS_ARCH_X86, CS_MODE_16, Cs
from overlay_inventory import find_units, parse_mz
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, OVR_SHA, REGISTERS, physical, put16
from pinned_shift_flags import LastFlagWriter, SPLIT_BY_RULE, declared_flags_split, split_used

U33_BASE, U3BD_BASE = 0xC5D1, 0x2E8F7
U33_SPAN = (0x012B, 0x0221)
U3BD_SPAN = (0x031F, 0x057A)
STUB_PARAGRAPH = 0x03D5
STUBS = {0x2F: 0x0350, 0x2A: 0x047B}
CALLER_CS, LOOKUP_CS, DS, SS = 0x3000, 0x4000, 0x1000, 0x2000
TARGETS = {
    "012b": dict(unit=0x33, entry=0x012B, end=0x01AD, insns=49, retf=0x01AA, pops=8, conditionals=9, args=4,
                 sha="8a4956ef566e0c033171d771c19d0dc48aaf4b7d8e04e99be34c6fe3e3f276ae",
                 symbol="pr_u33_cell_lookup_012b", calls=[(0x0155, "lcall", "0x3d5, 0x2f"), (0x0170, "lcall", "0x3d5, 0x2a")],
                 callers={(0x01AD, 0x01D3), (0x01AD, 0x0209)}),
    "01ad": dict(unit=0x33, entry=0x01AD, end=0x0221, insns=54, retf=0x021E, pops=6, conditionals=0, args=3,
                 sha="6250ebfdce5344654824482798dfad959558058bf0dbb53d9de6e1852e4835db",
                 symbol="pr_u33_cell_pair_01ad", calls=[(0x01D3, "call", "0x12b"), (0x0209, "call", "0x12b")],
                 callers={(0x040F, 0x042B), (0x040F, 0x0448), (0x0615, 0x0631), (0x0615, 0x064D),
                          (0x0810, 0x0852), (0x0810, 0x0864), (0x0810, 0x0876), (0x1577, 0x1894), (0x1577, 0x1950)}),
    "0350": dict(unit=0x3BD, entry=0x0350, end=0x047B, insns=120, retf=0x0478, pops=6, conditionals=12, args=3,
                 sha="3b0a525351d0a12b3afeed4c9d9d39e28fdf3564166cdab0d16219fad1b60498",
                 symbol="pr_u3bd_field_0350_fullstate", calls=[(0x035F, "call", "0x31f"), (0x03B4, "call", "0x47b")], callers=None),
    "047b": dict(unit=0x3BD, entry=0x047B, end=0x057A, insns=103, retf=0x0577, pops=6, conditionals=11, args=3,
                 sha="92d75878d963a7f5766b1ea49e2f6c1fb51287775d06a5b6ca485fee8bc43658",
                 symbol="pr_u3bd_nibble_047b_fullstate", calls=[(0x048A, "call", "0x31f")], callers=None),
}
LEAF_SHA = "ab4d7f5e3d42b8d23dba77a563b5041f669850990444583890627278764f281b"


def check(condition, message):
    if not condition:
        raise ValueError(message)


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, "START.EXE provenance differs")
    check(len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA, "GAME.OVR provenance differs")
    units = find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)
    u33 = next(u for u in units if u["stub_para"] == 0x33)
    u3bd = next(u for u in units if u["stub_para"] == 0x3BD)
    check(u33["ovr_offset"] == U33_BASE and u33["memory_segment"] == 0x3B, "unit 0x33 placement differs")
    check(u3bd["ovr_offset"] == U3BD_BASE and u3bd["memory_segment"] == STUB_PARAGRAPH and
          u3bd["entries"][3] == 0x0350 and u3bd["entries"][2] == 0x047B, "unit 0x3BD entries 2/3 differ")
    decoder = Cs(CS_ARCH_X86, CS_MODE_16)
    spans = {0x33: ovr[U33_BASE + U33_SPAN[0]:U33_BASE + U33_SPAN[1]],
             0x3BD: ovr[U3BD_BASE + U3BD_SPAN[0]:U3BD_BASE + U3BD_SPAN[1]]}
    leaf = ovr[U3BD_BASE + 0x031F:U3BD_BASE + 0x0350]
    check(sha256(leaf).hexdigest() == LEAF_SHA, "+031F leaf differs")
    codes = {0x33: ovr[U33_BASE:U33_BASE + u33["code_size"]], 0x3BD: ovr[U3BD_BASE:U3BD_BASE + u3bd["code_size"]]}
    fixups = {0x33: set(u33["fixups"]), 0x3BD: set(u3bd["fixups"])}
    starts = {0x33: sorted(f["rel"] for f in u33["functions"]), 0x3BD: sorted(f["rel"] for f in u3bd["functions"])}
    bodies = {}
    for key, target in TARGETS.items():
        unit = target["unit"]
        body = codes[unit][target["entry"]:target["end"]]
        decoded = list(decoder.disasm(body, target["entry"]))
        check(sha256(body).hexdigest() == target["sha"], f"+{key} hash differs")
        check(len(decoded) == target["insns"] and decoded[-1].address == target["retf"] and
              decoded[-1].mnemonic == "retf" and int(decoded[-1].op_str, 16) == target["pops"] and
              decoded[-1].address + decoded[-1].size == target["end"], f"+{key} extent differs")
        sites = {i.address for i in decoded}
        calls = [(i.address, i.mnemonic, i.op_str) for i in decoded if "call" in i.mnemonic or i.mnemonic.startswith("int")]
        check(calls == target["calls"], f"+{key} calls differ: {calls}")
        for address, mnemonic, _ in calls:
            if mnemonic == "lcall":
                check((address + 3) in fixups[unit], f"+{key} far call segment word is not fixup-backed")
        conditional = {i.address: (i.address + i.size, int(i.op_str, 16)) for i in decoded
                       if i.mnemonic.startswith("j") and i.mnemonic != "jmp"}
        check(len(conditional) == target["conditionals"], f"+{key} conditional count differs")
        for i in decoded:
            if i.mnemonic.startswith("j"):
                check(int(i.op_str, 16) in sites, f"+{key} branch leaves the body")
        if target["callers"] is not None:
            near = set()
            for function in starts[unit]:
                end = next((x for x in starts[unit] if x > function), len(codes[unit]))
                aligned = {i.address: i for i in decoder.disasm(codes[unit][function:end], function)}
                for site, ins in aligned.items():
                    if ins.mnemonic != "call":
                        continue
                    try:
                        destination = int(ins.op_str, 16)
                    except ValueError:
                        continue
                    if destination == target["entry"]:
                        check(aligned[site - 1].mnemonic == "push" and aligned[site - 1].op_str == "cs",
                              f"+{key} caller lacks PUSH CS")
                        near.add((function, site))
            check(near == target["callers"], f"+{key} aligned near callers differ: {sorted(near)}")
        bodies[key] = dict(body=body, decoded=decoded, conditional=conditional, **target)
    print("u33 +012B (130 bytes, 49 instructions, LCALL 03D5:002F and 03D5:002A) and +01AD (116 bytes, 54 "
          "instructions, two PUSH CS/CALL +012B); u3BD +0350 (299/120) and +047B (255/103) folded; +031F leaf "
          f"{LEAF_SHA[:16]}... merged")
    print("aligned PUSH CS/CALL callers:", {k: sorted(f"+{f:04X}@+{s:04X}" for f, s in v["callers"])
                                             for k, v in bodies.items() if v["callers"]})
    return bodies, spans


CHECKER_ROWS = ('OVL_0033_012B', 'OVL_0033_01AD')


def library(directory):
    import subprocess
    path = directory / "u33.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-fPIC", "-shared",
                    "-I", str(ROOT / "include"),
                    str(ROOT / "src/recovered/overlay_u33_cell_lookup_012b.c"),
                    str(ROOT / "src/recovered/overlay_u33_cell_pair_01ad.c"),
                    str(ROOT / "src/recovered/overlay_u3bd_lookup_fullstate.c"),
                    str(ROOT / "src/recovered/overlay_u3bd_in_grid_031f.c"),
                    "-o", str(path)], check=True, capture_output=True, text=True)
    lib = ctypes.CDLL(str(path))
    functions = {}
    for key, target in TARGETS.items():
        f = getattr(lib, target["symbol"])
        f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu)]
        f.restype = ctypes.c_int
        functions[key] = f
    return functions


def fixture(spans, key, *, x=0, y=0, facing=0, map_id=5, party_y=0, buffer=None, tables=None,
            ss=SS, sp=0x8000, ds=DS, es=0x1111, buffer_at=(0x5000, 0x0100), flags=0x7202,
            background=None, link=0x7777, return_frame=(0x7000, 0x7001), frame_wrapped=False):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    target = TARGETS[key]
    cs = CALLER_CS if target["unit"] == 0x33 else LOOKUP_CS
    at = physical(CALLER_CS, U33_SPAN[0]); memory[at:at + len(spans[0x33])] = spans[0x33]
    at = physical(LOOKUP_CS, U3BD_SPAN[0]); memory[at:at + len(spans[0x3BD])] = spans[0x3BD]
    for offset, entry in STUBS.items():
        at = physical(STUB_PARAGRAPH, offset)
        memory[at:at + 5] = struct.pack("<BHH", 0xEA, entry, LOOKUP_CS)
    memory[physical(ds, 0x84DC)] = map_id & 0xFF
    memory[physical(ds, 0x6AAE)] = party_y & 0xFF
    put16(memory, ds, 0x6A5C, buffer_at[1]); put16(memory, ds, 0x6A5E, buffer_at[0])
    for i in range(0x400):
        memory[physical(buffer_at[0], (buffer_at[1] + i) & 0xFFFF)] = (buffer[i] if buffer else (i * 37 + 11) & 0xFF)
    for i in range(0x10A):
        memory[physical(ds, 0x2736 + i)] = (tables[i] if tables else (i * 5 + 3) & 0xFF)
    words = [return_frame[0], return_frame[1]]
    words += ([link, facing & 0xFF, y & 0xFF, x & 0xFF] if target["args"] == 4 else [facing & 0xFF, y & 0xFF, x & 0xFF])
    frame = physical(ss, sp)
    for i, value in enumerate(words):
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
        if not frame_wrapped:
            memory[(frame + 2 * i) & (SIZE - 1)] = value & 0xFF
            memory[(frame + 2 * i + 1) & (SIZE - 1)] = value >> 8
    cpu = Cpu(0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, sp, cs, target["entry"], ds, es, ss, flags)
    return memory, cpu


SPLIT_ARRIVALS = [0]


def compare(label, memory, cpu, bodies, key, function, budget=20000):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    target = bodies[key]
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory)); original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    start = physical(cpu.cs, target["entry"]); body_end = start + len(target["body"]); last = physical(cpu.cs, target["retf"])
    legal = [(physical(CALLER_CS, U33_SPAN[0]), physical(CALLER_CS, U33_SPAN[1])),
             (physical(LOOKUP_CS, U3BD_SPAN[0]), physical(LOOKUP_CS, U3BD_SPAN[1])),
             (physical(STUB_PARAGRAPH, 0x2A), physical(STUB_PARAGRAPH, 0x34))]
    returned, previous, visited, edges = [False], [None], set(), set()
    conditional = target["conditional"]
    writer = LastFlagWriter()  # declared splits for the pinned flag rules, see pinned_shift_flags

    def instruction(emulator, address, size, user):
        del size, user
        if returned[0]:
            emulator.emu_stop(); return
        writer.step(emulator, address)
        linear = address & (2 * SIZE - 1)
        if start <= linear < body_end:
            here = linear - start + target["entry"]
            visited.add(here)
            if previous[0] in conditional:
                edges.add((previous[0], here))
            previous[0] = here
            if linear == last:
                returned[0] = True
            return
        previous[0] = None
        check(any(lo <= linear < hi for lo, hi in legal), f"left the legal code: {label} at {linear:#x}")

    def mirror(emulator, access, address, size, value, user):
        del access, user
        for byte in range(size):
            raw = address + byte; at = raw & (SIZE - 1)
            emulator.mem_write(at + SIZE if raw < SIZE else at, bytes([(value >> (8 * byte)) & 0xFF]))

    original.hook_add(UC_HOOK_CODE, instruction); original.hook_add(UC_HOOK_MEM_WRITE, mirror)
    original.emu_start(start, 2 * SIZE, count=budget)
    check(returned[0], f"original did not return within {budget} instructions: {label}")
    writer.finish(original)
    expected = bytes(original.mem_read(0, SIZE))
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    status = function(ram, SIZE, ctypes.byref(cpu))
    check(status == 0, f"C status {status}: {label}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f"RAM mismatch {label}: {[hex(d) for d in differences[:8]]} ({len(differences)} bytes)")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        got, want = getattr(cpu, name), original.reg_read(register) & 0xFFFF
        if name == "flags":
            problem = declared_flags_split(got, want, writer)
            check(problem is None, f"flags mismatch {label}: {problem}")
            SPLIT_ARRIVALS[0] += split_used(got, want, writer)
            continue
        check(got == want, f"{name} mismatch {label}: C={got:04x}, original={want:04x}")
    return visited, edges


def cases(key):
    rng = random.Random({"012b": 0x33012B, "01ad": 0x3301AD, "0350": 0x3BD0350, "047b": 0x3BD047B}[key])
    spec = []
    coords = (-128, -16, -1, 0, 1, 7, 15, 16, 100, 127)
    if key in ("012b", "01ad"):
        for x in coords:
            for y in coords:
                for facing in (0, 1, 2, 3, 4, 5, 6, 7):
                    spec.append((f"grid-{x}-{y}-{facing}", dict(x=x, y=y, facing=facing, map_id=5, party_y=y if facing & 1 else 3)))
        for x in (-1, 16, 0):
            for y in (3, 4, -1, 127):
                for facing in (2, 6, 0, 1, 3, 5, 7, 0x82, 0x86, 0xFF, 8, 0x80):
                    spec.append((f"edge-{x}-{y}-{facing:02x}", dict(x=x, y=y, facing=facing, map_id=rng.choice((0, 5, 10)), party_y=3)))
    else:
        for x in coords:
            for y in coords:
                for selector in (0, 2, 4, 6, 1, 3, 5, 7, 0xFF, 0x80):
                    for map_id in (0, 10, 5):
                        spec.append((f"cell-{x}-{y}-{selector:02x}-{map_id}", dict(x=x, y=y, facing=selector, map_id=map_id)))
    for i in range(240):
        spec.append((f"random-{i}", dict(x=rng.randrange(256), y=rng.randrange(256), facing=rng.randrange(256),
                                        map_id=rng.choice((0, 10, 5, rng.randrange(256))), party_y=rng.randrange(256),
                                        buffer=[rng.randrange(256) for _ in range(0x400)],
                                        tables=[rng.randrange(256) for _ in range(0x10A)],
                                        flags=rng.choice((0x7202, 0x0202, 0x0ED7, 0x0647, 0x0A93)), es=rng.randrange(0x10000),
                                        link=rng.randrange(0x10000))))
    base = dict(x=3, y=4, facing=2, map_id=5, party_y=4)
    for sp in (0, 2, 4, 6, 8, 0xFFF0, 0xFFF2, 0xFFF4, 0xFFF6, 0xFFF8, 0xFFFA, 0xFFFC, 0xFFFE, 0xFFFF, 0x0001):
        spec.append((f"sp-{sp:04x}", dict(base, sp=sp, ss=0x1800, zero=True)))
    spec.append(("ss-a20", dict(base, ss=0xFFFF, sp=0x0040)))
    spec.append(("ds-a20", dict(base, ds=0xFFFF)))
    spec.append(("buffer-a20", dict(base, buffer_at=(0xFFFF, 0x0010))))
    spec.append(("buffer-offset-wrap", dict(base, buffer_at=(0x5000, 0xFFF0))))
    spec.append(("buffer-under-frame", dict(base, buffer_at=(SS, 0x8000 - 0x200))))
    spec.append(("ds-is-ss-pointer-in-frame", dict(base, ds=SS, ss=SS, sp=0x6A5C + 0x0C)))
    spec.append(("ds-is-ss-party-in-frame", dict(base, ds=SS, ss=SS, sp=0x6AAE + 0x06)))
    spec.append(("ds-is-ss-map-in-frame", dict(base, ds=SS, ss=SS, sp=0x84DC + 0x08, x=16)))
    spec.append(("ds-is-ss-tables-in-frame", dict(base, ds=SS, ss=SS, sp=0x2736 + 0x10)))
    spec.append(("random-background", dict(base, seed=11)))
    spec.append(("wrapped-frame-top", dict(base, sp=0xFFFA, ss=0x1800, frame_wrapped=True, zero=True)))
    spec.append(("preserved-df", dict(base, flags=0x0603)))
    spec.append(("incoming-arithmetic", dict(base, flags=0x0ED7)))
    return spec


def differential(bodies, spans, key, functions, quick=False):
    visited, edges, count = set(), set(), 0
    target = bodies[key]
    for label, options in cases(key):
        if quick and label.startswith("random-") and int(label.split("-")[1]) >= 24:
            continue
        options = dict(options)
        seed = options.pop("seed", None); zero = options.pop("zero", False)
        background = bytes(random.Random(seed).randbytes(SIZE)) if seed is not None else (bytes(SIZE) if zero else None)
        memory, cpu = fixture(spans, key, background=background, **options)
        v, e = compare(f"+{key} {label}", memory, cpu, bodies, key, functions[key])
        visited |= v; edges |= e; count += 1
    missing_sites = sorted(set(i.address for i in target["decoded"]) - visited)
    missing_edges = sorted({(s, o) for s, outs in target["conditional"].items() for o in outs} - edges)
    check(not missing_sites, f"+{key} unvisited instruction starts: {[hex(x) for x in missing_sites]}")
    check(not missing_edges, f"+{key} untaken conditional outcomes: {[(hex(a), hex(b)) for a, b in missing_edges]}")
    print(f"+{key}: {count} original-byte cases passed, full 1 MiB RAM and 13 CPU words equal, FLAGS through the declared "
          f"pinned splits at {SPLIT_ARRIVALS[0]} stops so far (div {SPLIT_BY_RULE['div']}, shift {SPLIT_BY_RULE['shift']}); "
          f"{len(visited)}/{len(target['decoded'])} instruction starts, {len(edges)}/{2 * len(target['conditional'])} conditional outcomes")
    return count


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compile-only", action="store_true",
                        help="build and bind the candidate library only; no media")
    parser.add_argument("--unicorn", action="store_true")
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--quick", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if args.compile_only:  # same compiler command and symbol binding as the audit; no media
        with tempfile.TemporaryDirectory(prefix='poolrad-checker-') as _t:
            library(Path(_t))
        print('CHECKER_BUILD_OK')
        return
    bodies, spans = source()
    report = dict(targets={k: dict(sha256=v["sha"], bytes=len(v["body"]), instructions=v["insns"]) for k, v in bodies.items()},
                  leaf_031f=LEAF_SHA)
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-u33-") as temp:
            functions = library(Path(temp))
            if args.unicorn:
                _cases = {}
                for k in bodies:
                    _cases[k] = differential(bodies, spans, k, functions, args.quick)
                    print(f"ROW_EXECUTED OVL_0033_{k.upper()} PASS")
                report["unicorn_cases"] = _cases
            if args.native:
                from u33_cell_lookup_012b_native import native_audit
                report["native"] = native_audit(ROOT, bodies, spans, functions)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
