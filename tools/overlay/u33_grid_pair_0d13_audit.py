#!/usr/bin/env python3
"""Original-byte full-state parity for u33 +0D13 and +0E9E composed with +08A4 and the dice chain.

Both bodies call the merged unit 0x33 +08A4 byte-flags leaf with PUSH CS/CALL
and unit 0x9E entry 8 (+0DE3) through stub 00B0:0048; entry 8 calls SYSTEM
0AF8:11C0. The differential loads the original unit 0x33 code at its own
offsets, the loaded-state stub, the original unit 0x9E span and the original
SYSTEM bytes into one 1 MiB image at base 0, runs the original under Unicorn
and the composed C on a copy, and compares the entire RAM and all 14 CPU
words. Media stays private.
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
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, OVR_SHA, REGISTERS, physical, put16

U33_BASE, U9E_BASE = 0xC5D1, 0x2AEEA
U9E_SPAN = (0x0DE3, 0x0E52)
U9E_SHA = "9a2681e8b24a9a95838a11f2d7cbddea0c253d29e631db37d25fae017a14693b"
FLAGS_SPAN = (0x08A4, 0x0B97)
FLAGS_SHA = "c721e542769e5b590a6ef00ee92ffa9b4a9b7a5a76a9c4563eda25b3a4b703bc"
SYSTEM_FILE = (0xBE36, 0xBEBD)
SYSTEM_CS, SYSTEM_OFFSET = 0x0AF8, 0x11C0
STUB_PARAGRAPH, STUB_OFFSET, ENTRY8 = 0x00B0, 0x48, 0x0DE3
CALLER_CS, DICE_CS, DS, SS = 0x3000, 0x4000, 0x1000, 0x2000
TARGETS = {
    "0d13": dict(entry=0x0D13, end=0x0E9E, insns=163, retf=0x0E9D, conditionals=13, sha="",
                 symbol="pr_u33_grid_pair_0d13", sites=[0x0DD6, 0x0DE9, 0x0DFD, 0x0E27],
                 flag_calls=[0x0D1E, 0x0D2C, 0x0D3A, 0x0D48, 0x0D56, 0x0E18], callers={(0x11B4, 0x121D)}, infeasible=set()),
    "0e9e": dict(entry=0x0E9E, end=0x11B4, insns=323, retf=0x11B3, conditionals=18, sha="",
                 symbol="pr_u33_grid_bands_0e9e", sites=[0x0F45, 0x0F5E, 0x1026, 0x107B, 0x10D9, 0x1150, 0x116E],
                 flag_calls=[0x0EB9, 0x0ECB, 0x0ED9, 0x0EEB, 0x115F], callers={(0x11B4, 0x1221)}, infeasible=set()),
}


def check(condition, message):
    if not condition:
        raise ValueError(message)


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, "START.EXE provenance differs")
    check(len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA, "GAME.OVR provenance differs")
    units = {u["stub_para"]: u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)}
    check(units[0x33]["ovr_offset"] == U33_BASE and units[0x9E]["ovr_offset"] == U9E_BASE, "unit placement differs")
    check(units[0x9E]["memory_segment"] == STUB_PARAGRAPH and units[0x9E]["entries"][8] == ENTRY8, "unit 0x9E entry 8 differs")
    decoder = Cs(CS_ARCH_X86, CS_MODE_16)
    code = ovr[U33_BASE:U33_BASE + units[0x33]["code_size"]]
    flags = code[FLAGS_SPAN[0]:FLAGS_SPAN[1]]
    check(sha256(flags).hexdigest() == FLAGS_SHA, "unit 0x33 +08A4 differs from the merged evidence")
    dice = ovr[U9E_BASE + U9E_SPAN[0]:U9E_BASE + U9E_SPAN[1]]
    check(sha256(dice).hexdigest() == U9E_SHA, "unit 0x9E +0DE3..+0E51 differs")
    system = exe[SYSTEM_FILE[0]:SYSTEM_FILE[1]]
    check(sha256(system[:22]).hexdigest() == "e4337c9ed94c3d0a72864fcf23dd6e8fb30693d25680a1d52401c919fcbbfeb9" and
          sha256(system[0x4F:0x4F + 54]).hexdigest() == "f5b67b3eec594ce9fa18e9d78e0bb2eec93c9475a758f2c7ae9988ad00ce6242",
          "SYSTEM bytes differ from the merged evidence")
    fixups = set(units[0x33]["fixups"])
    starts = sorted(f["rel"] for f in units[0x33]["functions"])
    bodies = {}
    for key, target in TARGETS.items():
        body = code[target["entry"]:target["end"]]
        decoded = list(decoder.disasm(body, target["entry"]))
        target["sha"] = sha256(body).hexdigest()
        check(len(decoded) == target["insns"] and decoded[-1].address == target["retf"] and decoded[-1].mnemonic == "retf"
              and decoded[-1].op_str == "" and decoded[-1].address + decoded[-1].size == target["end"], f"+{key} extent differs")
        sites = {i.address for i in decoded}
        calls = [(i.address, i.mnemonic, i.op_str) for i in decoded if "call" in i.mnemonic or i.mnemonic.startswith("int")]
        expected = sorted([(s, "lcall", "0xb0, 0x48") for s in target["sites"]] + [(s, "call", "0x8a4") for s in target["flag_calls"]])
        check(sorted(calls) == expected, f"+{key} calls differ: {calls}")
        for s in target["sites"]:
            check((s + 3) in fixups, f"+{key} far call segment word at +{s:04X} is not fixup-backed")
        by_address = {i.address: i for i in decoded}
        for s in target["flag_calls"]:
            check(by_address[s - 1].mnemonic == "push" and by_address[s - 1].op_str == "cs", f"+{key} +08A4 call at +{s:04X} lacks PUSH CS")
        conditional = {i.address: (i.address + i.size, int(i.op_str, 16)) for i in decoded
                       if i.mnemonic.startswith("j") and i.mnemonic != "jmp"}
        check(len(conditional) == target["conditionals"], f"+{key} conditional count differs: {len(conditional)}")
        for i in decoded:
            if i.mnemonic.startswith("j"):
                check(int(i.op_str, 16) in sites, f"+{key} branch leaves the body")
        near = set()
        for function in starts:
            end = next((x for x in starts if x > function), len(code))
            aligned = {i.address: i for i in decoder.disasm(code[function:end], function)}
            for site, ins in aligned.items():
                if ins.mnemonic != "call":
                    continue
                try:
                    destination = int(ins.op_str, 16)
                except ValueError:
                    continue
                if destination == target["entry"]:
                    check(aligned[site - 1].mnemonic == "push" and aligned[site - 1].op_str == "cs", f"+{key} caller lacks PUSH CS")
                    near.add((function, site))
        check(near == target["callers"], f"+{key} aligned near callers differ: {sorted(near)}")
        bodies[key] = dict(body=body, decoded=decoded, conditional=conditional, **target)
    print(f"u33 +0D13 (395/163, four LCALL 00B0:0048, six +08A4 calls) and +0E9E (790/323, seven LCALL, five +08A4 calls); "
          f"+08A4 {FLAGS_SHA[:16]}..., u9E span {U9E_SHA[:16]}..., SYSTEM {len(system)} bytes original")
    print("aligned PUSH CS/CALL callers:", {k: sorted(f"+{f:04X}@+{s:04X}" for f, s in v["callers"]) for k, v in bodies.items()})
    return bodies, dict(code=code, dice=dice, system=system)


CHECKER_ROWS = ('OVL_0033_0D13',)


def library(directory):
    path = directory / "u33_grid.so"
    sources = ["overlay_u33_grid_pair_0d13.c", "overlay_u33_grid_bands_0e9e.c", "overlay_u33_byte_flags_08a4.c",
               "overlay_u9e_entry8_fullstate_compose_0de3.c", "overlay_u9e_entry8_fullstate_prefix_0de3.c",
               "overlay_u9e_entry8_fullstate_loop_0e05.c", "overlay_u9e_entry8_fullstate_postcall_0e13.c",
               "overlay_u9e_entry8_fullstate_tail_0e1f.c", "overlay_u9e_entry8_system_call_0e0e.c",
               "system_random_fullstate_11c0.c", "system_seed_fullstate_120f.c"]
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-fPIC", "-shared", "-I", str(ROOT / "include")]
                   + [str(ROOT / "src/recovered" / s) for s in sources] + ["-o", str(path)], check=True, capture_output=True, text=True)
    lib = ctypes.CDLL(str(path))
    functions = {}
    for key, target in TARGETS.items():
        f = getattr(lib, target["symbol"])
        f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(ctypes.c_int)]
        f.restype = ctypes.c_int
        functions[key] = f
    return functions


def fixture(bodies, code, key, *, seed=0x12345678, class_byte=0, window_word=0x00FF, window_at=(0x5000, 0x0000),
            grid=None, table=None, grid_at=(0x8000, 0x0000), ss=SS, sp=0x8000, ds=DS, es=0x1111, flags=0x7202,
            background=None, return_frame=(0x7000, 0x7001), frame_wrapped=False):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    target = TARGETS[key]
    at = physical(CALLER_CS, 0); memory[at:at + len(code["code"])] = code["code"]
    at = physical(DICE_CS, U9E_SPAN[0]); memory[at:at + len(code["dice"])] = code["dice"]
    at = physical(SYSTEM_CS, SYSTEM_OFFSET); memory[at:at + len(code["system"])] = code["system"]
    at = physical(STUB_PARAGRAPH, STUB_OFFSET); memory[at:at + 5] = struct.pack("<BHH", 0xEA, ENTRY8, DICE_CS)
    put16(memory, ds, 0x4428, seed & 0xFFFF); put16(memory, ds, 0x442A, seed >> 16)
    memory[physical(ds, 0x465A)] = class_byte & 0xFF
    put16(memory, ds, 0x49D2, window_at[1]); put16(memory, ds, 0x49D4, window_at[0])
    put16(memory, window_at[0], (window_at[1] + 0x366) & 0xFFFF, window_word)
    gseg, goff = grid_at
    put16(memory, ds, 0x6716, goff); put16(memory, ds, 0x6718, gseg)
    for i in range(0x4E9):
        memory[physical(gseg, (goff + i) & 0xFFFF)] = (grid[i] if grid else (i * 7 + 3) & 0xFF)
    for i in range(0x400):
        memory[physical(ds, 0x2747 + i)] = (table[i] if table else (0x16 if i % 4 == 0 else 0x33))
    frame = physical(ss, sp)
    for i, value in enumerate(return_frame):
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
        if not frame_wrapped:
            memory[(frame + 2 * i) & (SIZE - 1)] = value & 0xFF
            memory[(frame + 2 * i + 1) & (SIZE - 1)] = value >> 8
    cpu = Cpu(0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, sp, CALLER_CS, target["entry"], ds, es, ss, flags)
    return memory, cpu


def compare(label, memory, cpu, bodies, key, function, budget=6000000):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    target = bodies[key]
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory)); original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    start = physical(cpu.cs, target["entry"]); body_end = start + len(target["body"]); last = physical(cpu.cs, target["retf"])
    legal = [(physical(CALLER_CS, FLAGS_SPAN[0]), physical(CALLER_CS, FLAGS_SPAN[1])),
             (physical(DICE_CS, U9E_SPAN[0]), physical(DICE_CS, U9E_SPAN[1])),
             (physical(SYSTEM_CS, SYSTEM_OFFSET), physical(SYSTEM_CS, SYSTEM_OFFSET + 0x85)),
             (physical(STUB_PARAGRAPH, STUB_OFFSET), physical(STUB_PARAGRAPH, STUB_OFFSET + 5))]
    returned, previous, visited, edges = [False], [None], set(), set()
    conditional = target["conditional"]

    def instruction(emulator, address, size, user):
        del size, user
        if returned[0]:
            emulator.emu_stop(); return
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
    expected = bytes(original.mem_read(0, SIZE))
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    chain = ctypes.c_int(-1)
    status = function(ram, SIZE, ctypes.byref(cpu), ctypes.byref(chain))
    check(status == 0, f"C status {status} (chain {chain.value}): {label}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f"RAM mismatch {label}: {[hex(d) for d in differences[:8]]} ({len(differences)} bytes)")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        got, want = getattr(cpu, name), original.reg_read(register) & 0xFFFF
        check(got == want, f"{name} mismatch {label}: C={got:04x}, original={want:04x}")
    return visited, edges


def cases(key):
    rng = random.Random({"0d13": 0x330D13, "0e9e": 0x330E9E}[key])
    spec = []
    for i in range(72):
        eligible = rng.choice((0.3, 0.6, 0.9, 1.0))
        spec.append((f"random-{i}", dict(seed=rng.randrange(1 << 32), class_byte=rng.randrange(256),
                                        window_word=rng.choice((0x00FF, 0x00FF, 0x0000, rng.randrange(0x10000))),
                                        grid=[rng.randrange(256) for _ in range(0x4E9)],
                                        table=[(0x16 if (j % 4 == 0 and rng.random() < eligible) else rng.randrange(256)) for j in range(0x400)],
                                        flags=rng.choice((0x7202, 0x0202, 0x0ED7)), es=rng.randrange(0x10000))))
    for cb in range(0, 256, 5):
        spec.append((f"class-{cb:02x}", dict(seed=rng.randrange(1 << 32), class_byte=cb, window_word=rng.choice((0x00FF, 0x1234)))))
    spec.append(("none-eligible", dict(table=[0x17] * 0x400)))
    spec.append(("all-eligible-default-class", dict(class_byte=0x00)))
    spec.append(("grid-a20", dict(grid_at=(0xFFFF, 0x0010))))
    spec.append(("grid-offset-wrap", dict(grid_at=(0x8000, 0xFE00))))
    spec.append(("grid-under-stack", dict(grid_at=(SS, 0x8000 - 0x600))))
    spec.append(("grid-over-seed", dict(grid_at=(DS, 0x4428 - 0x200))))
    spec.append(("window-under-stack", dict(window_at=(SS, 0x8000 - 0x400), class_byte=0xF5)))
    spec.append(("window-a20", dict(window_at=(0xFFFF, 0x0010), class_byte=0xF5)))
    for sp in (0, 2, 4, 6, 8, 0xFFF0, 0xFFF4, 0xFFF8, 0xFFFC, 0xFFFE, 0xFFFF):
        spec.append((f"sp-{sp:04x}", dict(sp=sp, ss=0x1800, zero=True)))
    spec.append(("ss-a20", dict(ss=0xFFFF, sp=0x0040)))
    spec.append(("ds-a20", dict(ds=0xFFFF)))
    spec.append(("random-background", dict(seed_bg=3)))
    spec.append(("preserved-df", dict(flags=0x0603)))
    return spec


def differential(bodies, code, key, functions, quick=False):
    visited, edges, count = set(), set(), 0
    target = bodies[key]
    for label, options in cases(key):
        if quick and label.startswith("random-") and int(label.split("-")[1]) >= 12:
            continue
        options = dict(options)
        bg = options.pop("seed_bg", None); zero = options.pop("zero", False)
        background = bytes(random.Random(bg).randbytes(SIZE)) if bg is not None else (bytes(SIZE) if zero else None)
        memory, cpu = fixture(bodies, code, key, background=background, **options)
        v, e = compare(f"+{key} {label}", memory, cpu, bodies, key, functions[key])
        visited |= v; edges |= e; count += 1
    missing_sites = sorted(set(i.address for i in target["decoded"]) - visited)
    missing_edges = {(s, o) for s, outs in target["conditional"].items() for o in outs} - edges
    check(not missing_sites, f"+{key} unvisited instruction starts: {[hex(x) for x in missing_sites]}")
    check(missing_edges == target["infeasible"], f"+{key} untaken conditional outcomes: {sorted((hex(a), hex(b)) for a, b in missing_edges)}")
    print(f"+{key}: {count} original-byte cases passed, full 1 MiB RAM and all 14 CPU words equal; "
          f"{len(visited)}/{len(target['decoded'])} instruction starts, {len(edges)}/{2 * len(target['conditional'])} conditional outcomes "
          f"({len(target['infeasible'])} declared infeasible)")
    return dict(returning=count, infeasible_edges=sorted(f"{a:04X}->{b:04X}" for a, b in target["infeasible"]))


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
    bodies, code = source()
    report = dict(targets={k: dict(sha256=v["sha"], bytes=len(v["body"]), instructions=v["insns"]) for k, v in bodies.items()},
                  flags_08a4=FLAGS_SHA, dice_span=U9E_SHA)
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-u33-grid-") as temp:
            functions = library(Path(temp))
            if args.unicorn:
                report["unicorn_cases"] = {k: differential(bodies, code, k, functions, args.quick) for k in bodies}
            if args.native:
                from u33_grid_pair_0d13_native import native_audit
                report["native"] = native_audit(ROOT, bodies, code, functions)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
