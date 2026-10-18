#!/usr/bin/env python3
"""Original-byte full-state parity for the raw-CPU U38 helper cluster, plus a DOSBox-X synthetic oracle.

Bodies: the private helper +0000 and public entries 50 (+1490), 92 (+29B6), 93 (+29CB),
94 (+29D9), 95 (+29F0) and 96 (+2A11), which compose it through PUSH AX; PUSH CS;
CALL +0000. The Unicorn differential loads the original unit 0x38 code into one 1 MiB
image and compares the entire RAM and all 14 CPU words for random and directed DS
bytes, keys, frame aliasing (with the host cap compared at its stop) and segment-end
stacks. Media stays private.
"""
import argparse
import ctypes
from hashlib import sha256
import json
import os
from pathlib import Path
import random
import struct
import subprocess
import tempfile

from capstone import CS_ARCH_X86, CS_MODE_16, Cs
from overlay_inventory import find_units, parse_mz
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, OVR_SHA, REGISTERS, physical, put16

U38_BASE = 0x0EC5B
TARGETS = {
    0x0000: dict(symbol="pr_u38_helper_0000_cpu", insns=11, retf=0x001C, pops=2, conditionals=2, entry_slot=None, args=1),
    0x1490: dict(symbol="pr_u38_entry50_1490_cpu", insns=36, retf=0x14E8, pops=0x0A, conditionals=5, entry_slot=50, args=5, capped=True),
    0x29B6: dict(symbol="pr_u38_entry92_29b6_cpu", insns=12, retf=0x29C8, pops=0x0A, conditionals=0, entry_slot=92, args=5),
    0x29CB: dict(symbol="pr_u38_entry93_29cb_cpu", insns=8, retf=0x29D6, pops=0x0A, conditionals=0, entry_slot=93, args=5),
    0x29D9: dict(symbol="pr_u38_entry94_29d9_cpu", insns=12, retf=0x29ED, pops=0x0A, conditionals=1, entry_slot=94, args=5),
    0x29F0: dict(symbol="pr_u38_entry95_29f0_cpu", insns=15, retf=0x2A0E, pops=0x0A, conditionals=1, entry_slot=95, args=5),
    0x2A11: dict(symbol="pr_u38_entry96_2a11_cpu", insns=12, retf=0x2A25, pops=0x0A, conditionals=1, entry_slot=96, args=5),
}
DS_BYTES = (0x6816, 0x6817, 0x6818, 0x6819, 0x681C, 0x682A)
CS, DS, SS = 0x3000, 0x1000, 0x2000
CAP = 400


def check(condition, message):
    if not condition:
        raise ValueError(message)


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, "START.EXE provenance differs")
    check(len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA, "GAME.OVR provenance differs")
    unit = {u["stub_para"]: u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)}[0x38]
    check(unit["ovr_offset"] == U38_BASE, "unit 0x38 placement differs")
    code = ovr[U38_BASE:U38_BASE + unit["code_size"]]
    starts = sorted(f["rel"] for f in unit["functions"])
    decoder = Cs(CS_ARCH_X86, CS_MODE_16)
    near_to_helper = []
    for function in starts:
        end = next((x for x in starts if x > function), len(code))
        aligned = {i.address: i for i in decoder.disasm(code[function:end], function)}
        for site, ins in aligned.items():
            if ins.mnemonic == "call" and ins.op_str == "0":
                check(aligned[site - 1].op_str == "cs", f"+{site:04X} lacks PUSH CS")
                near_to_helper.append((function, site))
    check(len(near_to_helper) == 20, f"helper callers differ: {len(near_to_helper)}")
    targets = {}
    for entry, spec in TARGETS.items():
        end = next(x for x in starts if x > entry)
        body = code[entry:end]
        decoded = list(decoder.disasm(body, entry))
        check(len(decoded) == spec["insns"] and decoded[-1].address == spec["retf"] and decoded[-1].mnemonic == "retf"
              and int(decoded[-1].op_str, 16) == spec["pops"] and decoded[-1].address + decoded[-1].size == end,
              f"+{entry:04X} extent differs")
        calls = [i.address for i in decoded if "call" in i.mnemonic or i.mnemonic.startswith("int")]
        check(all(decoded[[i.address for i in decoded].index(a)].op_str == "0" for a in calls), f"+{entry:04X} calls something else")
        check(sorted(s for f, s in near_to_helper if f == entry) == sorted(calls), f"+{entry:04X} helper sites differ")
        conditional = {i.address: (i.address + i.size, int(i.op_str, 16)) for i in decoded if i.mnemonic.startswith("j") and i.mnemonic != "jmp"}
        check(len(conditional) == spec["conditionals"], f"+{entry:04X} conditional count differs")
        if spec["entry_slot"] is not None:
            check(unit["entries"][spec["entry_slot"]] == entry, f"+{entry:04X} is not public entry {spec['entry_slot']}")
        else:
            check(entry not in unit["entries"], "+0000 is unexpectedly public")
        targets[entry] = dict(spec, entry=entry, end=end, body=body, decoded=decoded, conditional=conditional,
                              sha=sha256(body).hexdigest(), calls=calls)
        print(f"u38 +{entry:04X} ({len(body)}/{len(decoded)}, {len(calls)} helper call(s), RETF {spec['pops']:X}h) sha {targets[entry]['sha'][:16]}...")
    return targets, code


def library(directory):
    path = directory / "u38c.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2", "-fPIC", "-shared",
                    "-I", str(ROOT / "include"), str(ROOT / "src/recovered/overlay_u38_helper_cluster_cpu.c"),
                    "-o", str(path)], check=True, capture_output=True, text=True)
    lib = ctypes.CDLL(str(path))
    functions = {}
    for entry, spec in TARGETS.items():
        f = getattr(lib, spec["symbol"])
        if spec.get("capped"):
            f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_uint]
        else:
            f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu)]
        f.restype = ctypes.c_int
        functions[entry] = f
    return functions


def call(functions, entry, ram, cpu, cap=CAP):
    if TARGETS[entry].get("capped"):
        return functions[entry](ram, SIZE, ctypes.byref(cpu), cap)
    return functions[entry](ram, SIZE, ctypes.byref(cpu))


def fixture(code, entry, rng, *, ds_values=None, key=None, ss=SS, sp=0x8000, ds=DS, flags=0x7202, regs=None, background=None):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    at = physical(CS, 0); memory[at:at + len(code)] = code
    for offset in DS_BYTES:
        memory[physical(ds, offset)] = rng.randrange(256)
    for offset, value in (ds_values or {}).items():
        memory[physical(ds, offset)] = value & 0xFF
    words = [0x7000, 0x7001]
    if TARGETS[entry]["args"] == 1:
        words.append((rng.randrange(256) << 8) | (rng.randrange(256) if key is None else key))
    else:
        words += [rng.randrange(0x10000) for _ in range(5)]
    frame = physical(ss, sp)
    for i, value in enumerate(words):
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
        if i < 2:
            memory[(frame + 2 * i) & (SIZE - 1)] = value & 0xFF
            memory[(frame + 2 * i + 1) & (SIZE - 1)] = value >> 8
    reg = regs or tuple(rng.randrange(0x10000) for _ in range(7))
    cpu = Cpu(*reg, sp, CS, entry, ds, rng.randrange(0x10000), ss, flags)
    return memory, cpu, ss >= 0xF000 or ds >= 0xF000


def compare(label, memory, cpu, high, target, functions, code_size, cap=CAP, budget=200000):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory)); original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    base = physical(cpu.cs, 0)
    start, body_end, ret = base + target["entry"], base + target["end"], physical(0x7001, 0x7000)
    loop_back = base + 0x14B1
    previous, visited, edges, returned, stopped, arrivals = [None], set(), set(), [False], [False], [0]
    transferred, pending = [False], [False]
    helper_retf = base + 0x001C
    expected_returns = {base + site + 3 for site in target["calls"]}

    def transfer_check(emulator, address):
        """Stop at the first fetch after the helper's RETF when it is not the call's next instruction."""
        if pending[0]:
            pending[0] = False
            if address not in expected_returns:
                transferred[0] = True; emulator.emu_stop(); return True
        if address == helper_retf and target["entry"] != 0x0000:
            pending[0] = True
        return False

    def instruction(emulator, address, size, user):
        del size, user
        if transfer_check(emulator, address):
            return
        here = address - base
        if target.get("capped") and address == loop_back:
            arrivals[0] += 1
            if arrivals[0] == cap + 1:
                stopped[0] = True; emulator.emu_stop(); return
        visited.add(here)
        if previous[0] in target["conditional"]:
            edges.add((previous[0], here))
        previous[0] = here

    def outside(emulator, address, size, user):
        del size, user
        if transfer_check(emulator, address):
            return
        previous[0] = None

    def away(emulator, address, size, user):
        del size, user
        if transfer_check(emulator, address):
            return
        if address == ret:
            returned[0] = True
        else:
            transferred[0] = True                               # a RETF left for somewhere else: the C TRANSFER point
        emulator.emu_stop()

    def mirror(emulator, access, address, size, value, user):
        del access, user
        for byte in range(size):
            raw = address + byte; at = raw & (SIZE - 1)
            emulator.mem_write(at + SIZE if raw < SIZE else at, bytes([(value >> (8 * byte)) & 0xFF]))

    original.hook_add(UC_HOOK_CODE, instruction, begin=start, end=body_end - 1)
    if start > base:                                            # begin > end would hook every address
        original.hook_add(UC_HOOK_CODE, outside, begin=base, end=start - 1)
    original.hook_add(UC_HOOK_CODE, outside, begin=body_end, end=base + code_size - 1)
    original.hook_add(UC_HOOK_CODE, away, begin=0, end=base - 1)
    original.hook_add(UC_HOOK_CODE, away, begin=base + code_size, end=2 * SIZE - 1)
    if high:
        original.hook_add(UC_HOOK_MEM_WRITE, mirror)
    original.emu_start(start, 2 * SIZE, count=budget)
    check(returned[0] or stopped[0] or transferred[0], f"original neither returned, transferred nor reached the cap: {label}")
    expected = bytes(original.mem_read(0, SIZE))
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    status = call(functions, target["entry"], ram, cpu, cap)
    check(status == (0 if returned[0] else 3 if stopped[0] else 2), f"C status {status}: {label}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f"RAM mismatch {label}: {[(hex(d), actual[d], expected[d]) for d in differences[:6]]} ({len(differences)} bytes)")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        got, want = getattr(cpu, name), original.reg_read(register) & 0xFFFF
        check(got == want, f"{name} mismatch {label}: C={got:04x}, original={want:04x}")
    if stopped[0]:
        return None, f"cap#{arrivals[0]}"
    if transferred[0]:
        return None, f"transfer {cpu.cs:04x}:{cpu.ip:04x}"
    return visited, edges


def cases(entry):
    rng = random.Random(0x380000 + entry)
    spec = []
    for i in range(160):
        spec.append((f"random-{i}", dict(flags=(0x7202, 0x0202, 0x0ED7, 0x0647, 0x0002)[i % 5])))
    if entry == 0x0000:
        for k in range(256):
            spec.append((f"key-{k:02x}", dict(key=k, ds_values={0x6817: (k if k % 3 == 0 else (k + 1) & 0xFF)})))
    else:
        for bits in range(16):
            spec.append((f"6819-{bits:x}", dict(ds_values={0x6819: bits | (rng.randrange(16) << 4), 0x6817: rng.choice((0, 0x0B, 0x34, 0x35, 0x37))})))
        for v in (0x00, 0x01, 0x34, 0x35, 0x37, 0x0B):
            spec.append((f"6817-{v:02x}", dict(ds_values={0x6817: v, 0x6819: 0x03})))
        spec.append(("682a-zero", dict(ds_values={0x682A: 0})))
    if entry == 0x1490:
        for n in (0, 1, 2, 3, 0x10, 0x80, 0xFF):
            for b18 in (0, 1, 2, 0x7F, 0xFE):
                spec.append((f"loop-n{n:02x}-v{b18:02x}", dict(ds_values={0x6819: 0x01, 0x681C: n, 0x6818: b18})))
        # SS = DS with the frame bytes on DS:6816..681C, so the loop's DS stores rewrite the counter or the count.
        for sp in (0x681A, 0x681B, 0x681C, 0x681D, 0x681E, 0x6820):
            for n in (3, 0x40, 0xC0):
                spec.append((f"alias-sp{sp:04x}-n{n:02x}", dict(ss=DS, sp=sp, ds_values={0x6819: 0x01, 0x681C: n, 0x6818: 5})))
        # [BP-1] on DS:6818 (SS = DS, entry SP 681B; BP's low byte 01h lands on DS:6819): each pass nets -1 on the
        # counter until it drops below 80h and is reset to it, about 128 loop-backs; a cap of 50 stops it first.
        counter = dict(ss=DS, sp=0x681B, regs=(0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7701), ds_values={0x681C: 0x80})
        spec.append(("alias-counter-returns", dict(counter)))
        spec.append(("alias-counter-cap50", dict(counter, cap=50)))
    for sp in (0x0002, 0x0004, 0x0010, 0xFFF0, 0xFFF4, 0xFFF8, 0xFFFC, 0xFFFE):
        spec.append((f"sp-{sp:04x}", dict(ss=0x1800, sp=sp, background=bytes(SIZE))))
    spec.append(("ss-a20", dict(ss=0xFFFF, sp=0x0400)))
    spec.append(("ds-a20", dict(ds=0xFFFF)))
    spec.append(("random-background", dict(background=bytes(random.Random(9).randbytes(SIZE)))))
    spec.append(("preserved-df-if", dict(flags=0x0603)))
    return spec


def differential(targets, code, functions):
    report = {}
    for entry, target in targets.items():
        rng = random.Random(0x38C0DE + entry)
        visited, edges, count, limited = set(), set(), 0, []
        for label, options in cases(entry):
            options = dict(options); cap = options.pop("cap", CAP)
            memory, cpu, high = fixture(code, entry, rng, **options)
            v, e = compare(f"+{entry:04X} {label}", memory, cpu, high, target, functions, len(code), cap)
            if v is None:
                limited.append(f"{label}@{e}")
                continue
            visited |= v; edges |= e; count += 1
        missing_sites = sorted(hex(s) for s in set(i.address for i in target["decoded"]) - visited)
        missing_edges = sorted((hex(s), hex(o)) for s, outs in target["conditional"].items() for o in outs if (s, o) not in edges)
        check(not missing_sites and not missing_edges, f"+{entry:04X} coverage incomplete: {missing_sites} {missing_edges}")
        print(f"+{entry:04X}: {count} returning original-byte cases passed, full 1 MiB RAM and all 14 CPU words equal; "
              f"{len(visited & set(i.address for i in target['decoded']))}/{len(target['decoded'])} instruction starts, "
              f"{len(edges)}/{2 * len(target['conditional'])} conditional outcomes" + (f"; {len(limited)} stops: {', '.join(limited)}" if limited else ""))
        report[f"{entry:04X}"] = dict(returning=count, limit_stops=len(limited), starts=len(target["decoded"]), outcomes=len(edges),
                                      sha256=target["sha"], bytes=len(target["body"]), instructions=len(target["decoded"]))
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unicorn", action="store_true")
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    targets, code = source()
    report = dict(targets={f"{e:04X}": dict(sha256=t["sha"], bytes=len(t["body"]), instructions=len(t["decoded"])) for e, t in targets.items()})
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-u38c-") as temp:
            functions = library(Path(temp))
            if args.unicorn:
                report["unicorn"] = differential(targets, code, functions)
            if args.native:
                from u38_helper_cluster_native import native_audit
                report["native"] = native_audit(targets, code, functions)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
