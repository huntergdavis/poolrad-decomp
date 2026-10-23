#!/usr/bin/env python3
"""Original-byte full-state parity for u33 +0BDE (with +0B97 folded) and the +11B4 driver.

+0BDE calls the merged +08A4 leaf, the dice entry 8 through stub 00B0:0048 and
the +0B97 pair writer; +11B4 calls SYSTEM FillChar 0AF8:16A8 and then +0BDE,
+0D13, +0E9E and +0000. The differential loads the original unit 0x33 code at
its own offsets, the loaded-state stub, the original unit 0x9E span, the SYSTEM
generator and FillChar bytes into one 1 MiB image at base 0, runs the original
under Unicorn and the composed C on a copy, and compares the entire RAM and all
14 CPU words. Media stays private.
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
from pinned_shift_flags import LastFlagWriter, SPLIT_BY_RULE, declared_flags_split, split_used

U33_BASE, U9E_BASE = 0xC5D1, 0x2AEEA
U9E_SPAN = (0x0DE3, 0x0E52)
U9E_SHA = "9a2681e8b24a9a95838a11f2d7cbddea0c253d29e631db37d25fae017a14693b"
FLAGS_SPAN, FLAGS_SHA = (0x08A4, 0x0B97), "c721e542769e5b590a6ef00ee92ffa9b4a9b7a5a76a9c4563eda25b3a4b703bc"
PAIR_SPAN, PAIR_SHA = (0x0B97, 0x0BDE), "75c464f3df7839fc6a44ab167138b0addb4a329758a775b3255c50a35108b52a"
SYSTEM_FILE, FILL_FILE = (0xBE36, 0xBEBD), (0xC31E, 0xC332)
FILL_SHA = "f1c5fade96970aa1dd1a9ffc2d876c7aea57bef8680d929d9f65d6c5190e17f7"
SYSTEM_CS, SYSTEM_OFFSET, FILL_OFFSET = 0x0AF8, 0x11C0, 0x16A8
STUB_PARAGRAPH, STUB_OFFSET, ENTRY8 = 0x00B0, 0x48, 0x0DE3
CALLER_CS, DICE_CS, DS, SS = 0x3000, 0x4000, 0x1000, 0x2000
TARGETS = {
    "0bde": dict(entry=0x0BDE, end=0x0D13, insns=138, retf=0x0D12, conditionals=9, sha="",
                 symbol="pr_u33_grid_diagonal_0bde", sites=[0x0C0E, 0x0C21, 0x0C9C, 0x0CCE], runtime=[],
                 near={0x0BED: 0x08A4, 0x0BFB: 0x08A4, 0x0CB6: 0x0B97, 0x0CFB: 0x0B97, 0x0D0C: 0x0B97},
                 callers={(0x11B4, 0x1219)}, infeasible=set()),
    "11b4": dict(entry=0x11B4, end=0x122C, insns=46, retf=0x122B, conditionals=2, sha="",
                 symbol="pr_u33_grid_driver_11b4", sites=[], runtime=[(0x11CB, 0x16A8)],
                 near={0x1219: 0x0BDE, 0x121D: 0x0D13, 0x1221: 0x0E9E, 0x1225: 0x0000},
                 callers={(0x1244, 0x12DA)}, infeasible=set()),
}
LEGAL_U33 = [(0x0000, 0x012B), (0x08A4, 0x0B97), (0x0B97, 0x0BDE), (0x0BDE, 0x0D13), (0x0D13, 0x0E9E), (0x0E9E, 0x11B4), (0x11B4, 0x122C)]


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
    check(sha256(code[FLAGS_SPAN[0]:FLAGS_SPAN[1]]).hexdigest() == FLAGS_SHA, "unit 0x33 +08A4 differs")
    pair = code[PAIR_SPAN[0]:PAIR_SPAN[1]]
    pair_decoded = list(decoder.disasm(pair, PAIR_SPAN[0]))
    check(len(pair) == 71 and len(pair_decoded) == 29 and sha256(pair).hexdigest() == PAIR_SHA and
          pair_decoded[-1].mnemonic == "retf" and pair_decoded[-1].op_str == "6" and
          not any("call" in i.mnemonic for i in pair_decoded), "+0B97 pair writer differs")
    dice = ovr[U9E_BASE + U9E_SPAN[0]:U9E_BASE + U9E_SPAN[1]]
    check(sha256(dice).hexdigest() == U9E_SHA, "unit 0x9E +0DE3..+0E51 differs")
    system = exe[SYSTEM_FILE[0]:SYSTEM_FILE[1]]
    check(sha256(system[:22]).hexdigest() == "e4337c9ed94c3d0a72864fcf23dd6e8fb30693d25680a1d52401c919fcbbfeb9", "SYSTEM bytes differ")
    fill = exe[FILL_FILE[0]:FILL_FILE[1]]
    check(len(fill) == 20 and sha256(fill).hexdigest() == FILL_SHA, "SYSTEM FillChar bytes differ")
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
        calls = sorted((i.address, i.mnemonic, i.op_str) for i in decoded if "call" in i.mnemonic or i.mnemonic.startswith("int"))
        expected = sorted([(s, "lcall", "0xb0, 0x48") for s in target["sites"]] + [(s, "lcall", "0xaf8, 0x16a8") for s, _ in target["runtime"]]
                          + [(s, "call", f"{t:#x}" if t else "0") for s, t in target["near"].items()])
        check(calls == expected, f"+{key} calls differ: {calls}")
        for s in target["sites"] + [s for s, _ in target["runtime"]]:
            check((s + 3) in fixups, f"+{key} far call segment word at +{s:04X} is not fixup-backed")
        by_address = {i.address: i for i in decoded}
        for s in target["near"]:
            check(by_address[s - 1].mnemonic == "push" and by_address[s - 1].op_str == "cs", f"+{key} near call at +{s:04X} lacks PUSH CS")
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
    far = []
    for u in units.values():
        ucode = ovr[u["ovr_offset"]:u["ovr_offset"] + u["code_size"]]
        for fixup in u["fixups"]:
            site = fixup - 3
            if site >= 0 and ucode[site:site + 5] == struct.pack("<BHH", 0x9A, 0x20 + 5 * 1, 0x3B):
                far.append((u["stub_para"], site))
    print(f"u33 +0BDE (309/138: two +08A4 calls, four LCALL 00B0:0048, three +0B97 calls) and +11B4 (120/46: LCALL 0AF8:16A8, four "
          f"near calls); +0B97 71 bytes, FillChar {FILL_SHA[:16]}..., +08A4 {FLAGS_SHA[:16]}..., u9E {U9E_SHA[:16]}...")
    print("aligned PUSH CS/CALL callers:", {k: sorted(f"+{f:04X}@+{s:04X}" for f, s in v["callers"]) for k, v in bodies.items()},
          "entry 1 (+1E31) far callers:", len(far))
    return bodies, dict(code=code, dice=dice, system=system, fill=fill)


CHECKER_ROWS = ('OVL_0033_11B4',)


def library(directory):
    path = directory / "u33_driver.so"
    sources = ["overlay_u33_grid_driver_11b4.c", "overlay_u33_grid_diagonal_0bde.c", "overlay_u33_grid_pair_0d13.c",
               "overlay_u33_grid_bands_0e9e.c", "overlay_u33_grid_roll_0000.c", "overlay_u33_byte_flags_08a4.c",
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


def fixture(bodies, code, key, *, seed=0x12345678, class_byte=0, window_word=0x00FF, window_bytes=(1, 2), mode=0,
            window_at=(0x5000, 0x0000), grid=None, table=None, class_table=None, grid_at=(0x8000, 0x0000),
            ss=SS, sp=0x8000, ds=DS, es=0x1111, flags=0x7202, background=None, return_frame=(0x7000, 0x7001), frame_wrapped=False):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    target = TARGETS[key]
    at = physical(CALLER_CS, 0); memory[at:at + len(code["code"])] = code["code"]
    at = physical(DICE_CS, U9E_SPAN[0]); memory[at:at + len(code["dice"])] = code["dice"]
    at = physical(SYSTEM_CS, SYSTEM_OFFSET); memory[at:at + len(code["system"])] = code["system"]
    at = physical(SYSTEM_CS, FILL_OFFSET); memory[at:at + len(code["fill"])] = code["fill"]
    at = physical(STUB_PARAGRAPH, STUB_OFFSET); memory[at:at + 5] = struct.pack("<BHH", 0xEA, ENTRY8, DICE_CS)
    put16(memory, ds, 0x4428, seed & 0xFFFF); put16(memory, ds, 0x442A, seed >> 16)
    memory[physical(ds, 0x465A)] = class_byte & 0xFF
    memory[physical(ds, 0x49FA)] = mode & 0xFF
    put16(memory, ds, 0x49D2, window_at[1]); put16(memory, ds, 0x49D4, window_at[0])
    put16(memory, window_at[0], (window_at[1] + 0x366) & 0xFFFF, window_word)
    memory[physical(window_at[0], (window_at[1] + 0x186) & 0xFFFF)] = window_bytes[0] & 0xFF
    memory[physical(window_at[0], (window_at[1] + 0x188) & 0xFFFF)] = window_bytes[1] & 0xFF
    for i in range(0x100):
        memory[physical(ds, 0x3648 + i)] = (class_table[i] if class_table else (i * 11 + 5) & 0xFF)
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


SPLIT_ARRIVALS = [0]


def compare(label, memory, cpu, bodies, key, function, budget=12000000):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    target = bodies[key]
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory)); original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    start = physical(cpu.cs, target["entry"]); body_end = start + len(target["body"]); last = physical(cpu.cs, target["retf"])
    legal = [(physical(CALLER_CS, lo), physical(CALLER_CS, hi)) for lo, hi in LEGAL_U33] + [
        (physical(DICE_CS, U9E_SPAN[0]), physical(DICE_CS, U9E_SPAN[1])),
        (physical(SYSTEM_CS, SYSTEM_OFFSET), physical(SYSTEM_CS, SYSTEM_OFFSET + 0x85)),
        (physical(SYSTEM_CS, FILL_OFFSET), physical(SYSTEM_CS, FILL_OFFSET + 20)),
        (physical(STUB_PARAGRAPH, STUB_OFFSET), physical(STUB_PARAGRAPH, STUB_OFFSET + 5))]
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
    writer.finish(original)
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
        if name == "flags":
            problem = declared_flags_split(got, want, writer)
            check(problem is None, f"flags mismatch {label}: {problem}")
            SPLIT_ARRIVALS[0] += split_used(got, want, writer)
            continue
        check(got == want, f"{name} mismatch {label}: C={got:04x}, original={want:04x}")
    return visited, edges


def lcg_rolls(seed, dice):
    """Entry-8 results for consecutive (count, range) calls from one seed: seed := seed * 8088405h + 1 per die."""
    out = []
    for count, range_ in dice:
        total = 0
        for _ in range(count):
            seed = (seed * 0x08088405 + 1) & 0xFFFFFFFF
            total += (seed >> 16) % range_ + 1
        out.append(total)
    return out


def cases(key):
    rng = random.Random({"0bde": 0x330BDE, "11b4": 0x3311B4}[key])
    spec = []
    n = 60 if key == "0bde" else 36
    for i in range(n):
        spec.append((f"random-{i}", dict(seed=rng.randrange(1 << 32), class_byte=rng.randrange(256),
                                        window_word=rng.choice((0x00FF, 0x00FF, 0x0000, rng.randrange(0x10000))),
                                        window_bytes=(rng.randrange(0x2C), rng.randrange(4)), mode=rng.choice((0, 3, 4, rng.randrange(256))),
                                        grid=[rng.randrange(256) for _ in range(0x4E9)],
                                        table=[(0x16 if (j % 4 == 0 and rng.random() < 0.7) else rng.randrange(256)) for j in range(0x400)],
                                        class_table=[rng.randrange(256) for _ in range(0x100)],
                                        flags=rng.choice((0x7202, 0x0202, 0x0ED7)), es=rng.randrange(0x10000))))
    for cb in range(0, 256, 7):
        spec.append((f"class-{cb:02x}", dict(seed=rng.randrange(1 << 32), class_byte=cb, window_word=rng.choice((0x00FF, 0x1234)))))
    spec.append(("none-eligible", dict(table=[0x17] * 0x400)))
    spec.append(("grid-a20", dict(grid_at=(0xFFFF, 0x0010))))
    spec.append(("grid-offset-wrap", dict(grid_at=(0x8000, 0xFE00))))
    spec.append(("grid-under-stack", dict(grid_at=(SS, 0x8000 - 0x600))))
    spec.append(("grid-over-seed", dict(grid_at=(DS, 0x4428 - 0x200))))
    spec.append(("window-under-stack", dict(window_at=(SS, 0x8000 - 0x400), class_byte=0xF5)))
    spec.append(("window-over-class-byte", dict(window_at=(DS, 0x465A - 0x186))))
    for sp in (0, 2, 4, 6, 8, 0xFFF0, 0xFFF4, 0xFFF8, 0xFFFC, 0xFFFE, 0xFFFF):
        if key == "11b4" and sp == 8:
            # At 8 the driver's first generator call pushes CS at SS:0000 and the return IP at SS:FFFE, so the
            # callee's RETF reads CS physically beyond the segment end (the contiguous-frame contract shared by the
            # emulator and the model) and the original leaves the code; 0Ah still wraps FillChar's argument reads.
            sp = 0x0A
        spec.append((f"sp-{sp:04x}", dict(sp=sp, ss=0x1800, zero=True)))
    spec.append(("ss-a20", dict(ss=0xFFFF, sp=0x0040)))
    spec.append(("ds-a20", dict(ds=0xFFFF)))
    spec.append(("random-background", dict(seed_bg=3)))
    spec.append(("preserved-df", dict(flags=0x0603)))
    if key == "0bde":
        # The row loop's column limit (+0C5B to +0CBC) needs a start column of 1Ah, i.e. a 5d4 of 5..8
        # after a passing gate roll, so 25 rows push the column past 31h.
        found, seed = 0, 0x0BDE0001
        while found < 3:
            seed = (seed + 0x9E3779B1) & 0xFFFFFFFF
            gate, start = lcg_rolls(seed, ((1, 0x64), (5, 4)))
            if gate <= 0x23 and start <= 8:
                spec.append((f"diagonal-edge-{found}", dict(seed=seed, class_byte=0xB6, window_word=0x00FF)))
                found += 1
    return spec


def differential(bodies, code, key, functions, quick=False):
    visited, edges, count = set(), set(), 0
    target = bodies[key]
    for label, options in cases(key):
        if quick and label.startswith("random-") and int(label.split("-")[1]) >= 8:
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
    print(f"+{key}: {count} original-byte cases passed, full 1 MiB RAM and 13 CPU words equal, FLAGS through the declared "
          f"pinned splits at {SPLIT_ARRIVALS[0]} stops so far {SPLIT_BY_RULE}; "
          f"{len(visited)}/{len(target['decoded'])} instruction starts, {len(edges)}/{2 * len(target['conditional'])} conditional outcomes")
    return dict(returning=count)


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
                  flags_08a4=FLAGS_SHA, dice_span=U9E_SHA, fill_char=FILL_SHA)
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-u33-driver-") as temp:
            functions = library(Path(temp))
            if args.unicorn:
                report["unicorn_cases"] = {k: differential(bodies, code, k, functions, args.quick) for k in bodies}
            if args.native:
                from u33_grid_driver_11b4_native import native_audit
                report["native"] = native_audit(ROOT, bodies, code, functions)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
