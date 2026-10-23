#!/usr/bin/env python3
"""Original-byte full-state parity for u6A +0214 with +0159, +019C and u86 +323D composed.

+0214 walks the far record list headed at DS:5D96/5D98 (link at +104h/+106h)
and calls +0159 and +019C on each record; +019C reaches unit 0x86 public
entry 6 (+323D) through the overlay stub 0096:003E. The differential loads the
original unit 0x6A code, the u86 +323D body and the loaded-state stub into one
1 MiB image, runs the original under Unicorn and the composed C on a copy, and
compares the entire RAM and all 14 CPU words. Media stays private.
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

UNIT_BASE = 0x17831
STUB_LINEAR = (0x0096 << 4) + 0x003E
CALLEE = dict(entry=0x323D, end=0x328A, sha="2ec1f95931eeb474593d0733ece3c8d582ab3a2a4d1c23b9de59d8f0bb5e87cd")
TARGET = dict(entry=0x0214, end=0x025C, insns=27, retf=0x025B, conditionals=1,
              calls=[(0x0236, "0x159"), (0x0240, "0x19c")], callers={(0x1E6F, 0x1ECA), (0x1E6F, 0x1FB3)})
SOURCES = ("overlay_u6a_record_walk_0214", "overlay_u6a_record_clear_high_0159",
           "overlay_u6a_link_high_clear_019c", "overlay_u86_record_predicate_323d")
CS, CS2, DS, SS = 0x3000, 0x4000, 0x1000, 0x2000
CAP = 64


def check(condition, message):
    if not condition:
        raise ValueError(message)


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, "START.EXE provenance differs")
    check(len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA, "GAME.OVR provenance differs")
    units = {u["stub_para"]: u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)}
    unit, callee_unit = units[0x6A], units[0x86]
    check(unit["ovr_offset"] == UNIT_BASE and unit["memory_segment"] == 0x75, "unit 0x6A placement differs")
    check(callee_unit["memory_segment"] == 0x96 and callee_unit["entries"][6] == CALLEE["entry"], "unit 0x86 entry 6 differs")
    code = ovr[UNIT_BASE:UNIT_BASE + unit["code_size"]]
    callee = ovr[callee_unit["ovr_offset"] + CALLEE["entry"]:callee_unit["ovr_offset"] + CALLEE["end"]]
    check(sha256(callee).hexdigest() == CALLEE["sha"], "u86 +323D differs")
    starts = sorted(f["rel"] for f in unit["functions"])
    decoder = Cs(CS_ARCH_X86, CS_MODE_16)
    body = code[TARGET["entry"]:TARGET["end"]]
    decoded = list(decoder.disasm(body, TARGET["entry"]))
    check(len(decoded) == TARGET["insns"] and decoded[-1].address == TARGET["retf"] and decoded[-1].mnemonic == "retf"
          and decoded[-1].op_str == "" and TARGET["end"] in starts, "+0214 extent differs")
    calls = [(i.address, i.op_str) for i in decoded if "call" in i.mnemonic or i.mnemonic.startswith("int")]
    check(calls == TARGET["calls"], f"+0214 calls differ: {calls}")
    check(all(decoded[decoded.index(i) - 1].op_str == "cs" for i in decoded if i.mnemonic == "call"), "near call lacks PUSH CS")
    conditional = {i.address: (i.address + i.size, int(i.op_str, 16)) for i in decoded if i.mnemonic.startswith("j") and i.mnemonic != "jmp"}
    check(len(conditional) == TARGET["conditionals"], "+0214 conditional count differs")
    near = set()
    for function in starts:
        end = next((x for x in starts if x > function), len(code))
        aligned = {i.address: i for i in decoder.disasm(code[function:end], function)}
        for site, ins in aligned.items():
            if ins.mnemonic == "call" and ins.op_str.startswith("0x") and int(ins.op_str, 16) == TARGET["entry"]:
                check(aligned[site - 1].op_str == "cs", "+0214 caller lacks PUSH CS")
                near.add((function, site))
    check(near == TARGET["callers"], f"+0214 aligned near callers differ: {sorted(near)}")
    target = dict(body=body, decoded=decoded, conditional=conditional, sha=sha256(body).hexdigest(), **TARGET)
    print(f"u6A +0214 ({len(body)}/{len(decoded)}, PUSH CS/CALL +0159 and +019C per record) sha {target['sha'][:16]}...; "
          f"callers {sorted(f'+{f:04X}@+{s:04X}' for f, s in near)}; u86 +323D {CALLEE['sha'][:16]}...")
    return target, code, callee


CHECKER_ROWS = ('OVL_006A_0214',)


def library(directory):
    import subprocess
    path = directory / "u6a_0214.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2", "-fPIC", "-shared",
                    "-I", str(ROOT / "include"), *[str(ROOT / "src/recovered" / (s + ".c")) for s in SOURCES],
                    "-o", str(path)], check=True, capture_output=True, text=True)
    f = ctypes.CDLL(str(path)).pr_u6a_record_walk_0214
    f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_uint, ctypes.POINTER(ctypes.c_int)]
    f.restype = ctypes.c_int
    return f


def fixture(code, callee, *, records, ds=DS, ss=SS, sp=0x8000, es=0x1111, flags=0x7202, background=None,
            seed=1, record_base=(0x5000, 0x0100), record_stride=0x200, node_base=(0x6000, 0x0000), head=0,
            regs=None, table1=None, table2=None):
    """records: list of (items[22] incl. +2C, nodes[(index, bytes3, next)], next_record or None)."""
    rng = random.Random(seed)
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    at = physical(CS, 0); memory[at:at + len(code)] = code
    at = physical(CS2, CALLEE["entry"]); memory[at:at + len(callee)] = callee
    memory[STUB_LINEAR:STUB_LINEAR + 5] = struct.pack("<BHH", 0xEA, CALLEE["entry"], CS2)
    for i in range(128):
        memory[physical(ds, 0x31FB + 16 * i)] = table1[i] if table1 else rng.randrange(256)
    for i in range(256):
        memory[physical(ds, 0x5582 + 16 * i)] = table2[i] if table2 else rng.randrange(256)
    rseg, roff = record_base
    nseg, noff = node_base
    pointer = lambda k: (0, 0) if k is None else (rseg, (roff + k * record_stride) & 0xFFFF)
    node_count = 0
    for k, (items, nodes, nxt) in enumerate(records):
        seg, off = pointer(k)
        for i, value in enumerate(items):
            memory[physical(seg, (off + 0x17 + i) & 0xFFFF)] = value
        first = node_count
        for j, (index, bytes3, nnext) in enumerate(nodes):
            nseg_j, noff_j = nseg, (noff + (first + j) * 0x40) & 0xFFFF
            memory[physical(nseg_j, (noff_j + 0x2E) & 0xFFFF)] = index
            for b, value in enumerate(bytes3):
                memory[physical(nseg_j, (noff_j + 0x3C + b) & 0xFFFF)] = value
            link = (0, 0) if nnext is None else (nseg, (noff + (first + nnext) * 0x40) & 0xFFFF)
            put16(memory, nseg_j, (noff_j + 0x2A) & 0xFFFF, link[1]); put16(memory, nseg_j, (noff_j + 0x2C) & 0xFFFF, link[0])
        node_count += len(nodes)
        hd = (0, 0) if not nodes else (nseg, (noff + first * 0x40) & 0xFFFF)
        put16(memory, seg, (off + 0xC8) & 0xFFFF, hd[1]); put16(memory, seg, (off + 0xCA) & 0xFFFF, hd[0])
        link = pointer(nxt)
        put16(memory, seg, (off + 0x104) & 0xFFFF, link[1]); put16(memory, seg, (off + 0x106) & 0xFFFF, link[0])
    first_record = pointer(head if records else None)
    put16(memory, ds, 0x5D96, first_record[1]); put16(memory, ds, 0x5D98, first_record[0])
    frame = physical(ss, sp)
    for delta, value in ((0, 0x7000), (2, 0x7001)):
        put16(memory, ss, (sp + delta) & 0xFFFF, value)
        memory[(frame + delta) & (SIZE - 1)] = value & 0xFF; memory[(frame + delta + 1) & (SIZE - 1)] = value >> 8
    r = regs or (0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777)
    cpu = Cpu(*r, sp, CS, TARGET["entry"], ds, es, ss, flags)
    high = any(s >= 0xF000 for s in (ds, ss, rseg, nseg))
    return memory, cpu, high


def compare(label, memory, cpu, high, target, function, budget=3000000):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory)); original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    start = physical(cpu.cs, target["entry"]); body_end = start + len(target["body"]); last = physical(cpu.cs, target["retf"])
    back = physical(0x7001, 0x7000)
    returned, previous, visited, edges = [False], [None], set(), set()

    def instruction(emulator, address, size, user):
        del size, user
        here = address - start + target["entry"]
        visited.add(here)
        if previous[0] in target["conditional"]:
            edges.add((previous[0], here))
        previous[0] = here
        if address == last:
            returned[0] = True

    def stop(emulator, address, size, user):
        del address, size, user
        if returned[0]:
            emulator.emu_stop()

    def mirror(emulator, access, address, size, value, user):
        del access, user
        for byte in range(size):
            raw = address + byte; at = raw & (SIZE - 1)
            emulator.mem_write(at + SIZE if raw < SIZE else at, bytes([(value >> (8 * byte)) & 0xFF]))

    original.hook_add(UC_HOOK_CODE, instruction, begin=start, end=body_end - 1)
    original.hook_add(UC_HOOK_CODE, stop, begin=back, end=back)
    if high:
        original.hook_add(UC_HOOK_MEM_WRITE, mirror)
    original.emu_start(start, 2 * SIZE, count=budget)
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    chain = ctypes.c_int(-1)
    if not returned[0]:
        status = function(ram, SIZE, ctypes.byref(cpu), CAP, ctypes.byref(chain))
        check(status == 4, f"original did not return and the C cap gave status {status}: {label}")
        return set(), set(), False
    expected = bytes(original.mem_read(0, SIZE))
    status = function(ram, SIZE, ctypes.byref(cpu), CAP, ctypes.byref(chain))
    check(status == 0, f"C status {status} (callee {chain.value}): {label}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f"RAM mismatch {label}: {[(hex(d), actual[d], expected[d]) for d in differences[:6]]} ({len(differences)} bytes)")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        got, want = getattr(cpu, name), original.reg_read(register) & 0xFFFF
        check(got == want, f"{name} mismatch {label}: C={got:04x}, original={want:04x}")
    return visited, edges, True


def random_record(rng, k, count, node_max=3):
    items = tuple(rng.randrange(256) for _ in range(22))
    n = rng.randrange(node_max + 1)
    nodes = tuple((rng.randrange(256), tuple(rng.randrange(256) for _ in range(3)), j + 1 if j + 1 < n else None) for j in range(n))
    return (items, nodes, k + 1 if k + 1 < count else None)


def cases():
    rng = random.Random(0x6A0214)
    spec = [("empty", dict(records=[]))]
    for count in (1, 2, 3, 5):
        for i in range(8):
            spec.append((f"list-{count}-{i}", dict(seed=rng.randrange(1 << 30), records=[random_record(rng, k, count) for k in range(count)])))
    for i in range(80):
        count = rng.randrange(0, 6)
        spec.append((f"random-{i}", dict(seed=rng.randrange(1 << 30), records=[random_record(rng, k, count) for k in range(count)],
                                        flags=rng.choice((0x7202, 0x0202, 0x0ED7, 0x0647)), es=rng.randrange(0x10000),
                                        regs=tuple(rng.randrange(0x10000) for _ in range(7)))))
    base = [random_record(random.Random(5), k, 3) for k in range(3)]
    spec.append(("cyclic-records", dict(records=[base[0][:2] + (1,), base[1][:2] + (0,)])))      # never returns in DOS
    spec.append(("head-not-first", dict(records=[base[0][:2] + (None,), base[1][:2] + (0,)], head=1)))
    spec.append(("link-offset-wrap", dict(records=base, record_base=(0x5000, 0xFE80))))
    spec.append(("records-a20", dict(records=base, record_base=(0xFFFF, 0x0010))))
    spec.append(("nodes-a20", dict(records=base, node_base=(0xFFFF, 0x0010))))
    spec.append(("record-over-head", dict(records=base, record_base=(DS, 0x5D96 - 0x104))))     # a link word aliases the head
    for sp in (0x0100, 0x0200, 0xFFF0, 0xFFFC):
        spec.append((f"sp-{sp:04x}", dict(records=base, sp=sp, ss=0x1800, background=bytes(SIZE))))
    spec.append(("ss-a20", dict(records=base, ss=0xFFFF, sp=0x0400)))
    spec.append(("ds-a20", dict(records=base, ds=0xFFFF)))
    spec.append(("random-background", dict(records=base, background=bytes(random.Random(9).randbytes(SIZE)))))
    spec.append(("preserved-df", dict(records=base, flags=0x0603)))
    return spec


def differential(target, code, callee, function):
    visited, edges, count, nonret = set(), set(), 0, 0
    for label, options in cases():
        memory, cpu, high = fixture(code, callee, **options)
        v, e, returned = compare(f"+0214 {label}", memory, cpu, high, target, function)
        visited |= v; edges |= e; count += returned; nonret += not returned
    missing_sites = sorted(set(i.address for i in target["decoded"]) - visited)
    missing_edges = sorted({(s, o) for s, outs in target["conditional"].items() for o in outs} - edges)
    check(not missing_sites and not missing_edges, f"+0214 coverage incomplete: {missing_sites} {missing_edges}")
    print(f"+0214: {count} original-byte cases passed, full 1 MiB RAM and all 14 CPU words equal; {nonret} non-returning "
          f"original(s) stopped by the C cap; {len(visited)}/{len(target['decoded'])} instruction starts, "
          f"{len(edges)}/{2 * len(target['conditional'])} conditional outcomes")
    return dict(returning=count, nonreturning=nonret, starts=len(visited), outcomes=len(edges))


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
    target, code, callee = source()
    report = dict(target=dict(sha256=target["sha"], bytes=len(target["body"]), instructions=target["insns"]))
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-u6a-0214-") as temp:
            function = library(Path(temp))
            from loaded_stub_checks import verify_refusals
            memory, cpu, _ = fixture(code, callee, records=[])
            report["loaded_stub_guards"] = verify_refusals(
                memory, cpu, lambda ram, state: function(ram, SIZE, ctypes.byref(state), 10, None),
                [(cpu.cs, 0x01C3)], check_target_cs=False)
            if args.unicorn:
                report["unicorn_cases"] = differential(target, code, callee, function)
            if args.native:
                from u6a_record_walk_0214_native import native_audit
                report["native"] = native_audit(ROOT, target, code, callee, function)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
