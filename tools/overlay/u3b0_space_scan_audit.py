#!/usr/bin/env python3
"""Original-byte full-state parity for U3B0 +08D7 and +0923 with the SYSTEM 0AF8:0B25 string store composed.

The differential places the hash-checked expanded START.EXE image at load segment 0, so the
original string store runs live at 0AF8:0B25 from both routines' unrelocated, fixup-backed far
calls. It loads unit 0x3B0 and compares the entire 1 MiB RAM and all 14 CPU words between the
original under Unicorn and the composed C models, with no mask, including CODE stops at a
rewritten call site. Media stays private.
"""
import argparse
import ctypes
from hashlib import sha256
import json
from pathlib import Path
import random
import subprocess
import tempfile

from capstone import CS_ARCH_X86, CS_MODE_16, Cs
from overlay_inventory import find_units, parse_mz
from resident_loader_map import unpack
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, OVR_SHA, REGISTERS, physical, put16

ROUTINES = {
    0x08D7: dict(end=0x0923, site=0x08EB, size=76, count=31, name="pr_u3b0_lead_spaces_08d7_cpu", label="+08D7", key="lead"),
    0x0923: dict(end=0x0969, site=0x0937, size=70, count=29, name="pr_u3b0_trail_spaces_0923_cpu", label="+0923", key="trail"),
}
STORE_CS, STORE_IP, STORE_END = 0x0AF8, 0x0B25, 0x0B49
STORE_SHA = "009955722e8f73d0a0f7076aa418d141f642c5a729b2a58dfa7a7a3922989118"
CALLER_CS, SS, EDGE_SS = 0x3800, 0x2800, 0x1C00
RET_CS, RET_IP = 0x7001, 0x7000
OK, CODE = 0, 2
SPACE, CAP = 0x20, 0x28
NONSPACE = [b for b in range(256) if b != SPACE]


def check(condition, message):
    if not condition:
        raise ValueError(message)


def conditionals(decoded):
    return {i.address: (i.address + i.size, int(i.op_str, 16)) for i in decoded if i.mnemonic.startswith("j") and i.mnemonic != "jmp"}


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, "START.EXE provenance differs")
    check(len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA, "GAME.OVR provenance differs")
    image = bytes(unpack(exe)[0])
    store = image[STORE_CS * 16 + STORE_IP:STORE_CS * 16 + STORE_END]
    check(sha256(store).hexdigest() == STORE_SHA, "SYSTEM 0AF8:0B25 differs from the pinned string store")
    store_decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(store, STORE_IP))
    unit = {u["stub_para"]: u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)}[0x3B0]
    code = ovr[unit["ovr_offset"]:unit["ovr_offset"] + unit["code_size"]]
    starts = sorted(f["rel"] for f in unit["functions"])
    fixups = set(unit["fixups"])
    targets = {}
    for entry, spec in ROUTINES.items():
        check(spec["end"] == next(s for s in starts if s > entry), f"U3B0 {spec['label']} bounds differ")
        body = code[entry:spec["end"]]
        decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, entry))
        calls = [(i.address, i.op_str) for i in decoded if "call" in i.mnemonic]
        check(len(body) == spec["size"] and len(decoded) == spec["count"] and sum(i.size for i in decoded) == len(body)
              and decoded[-1].mnemonic == "retf" and decoded[-1].op_str == "6"
              and calls == [(spec["site"], "0xaf8, 0xb25")] and spec["site"] + 3 in fixups,
              f"U3B0 {spec['label']} extent or call differs: {calls}")
        conditional = conditionals(decoded)
        check(len(conditional) == 2, f"U3B0 {spec['label']} conditional count differs")
        targets[entry] = dict(spec, entry=entry, body=body, decoded=decoded, conditional=conditional, sha=sha256(body).hexdigest(),
                              file_offset=unit["ovr_offset"] + entry)
        print(f"U3B0 {spec['label']} ({len(body)}/{len(decoded)}, LCALL 0AF8:0B25 at +{spec['site']:04X}, RETF 6) "
              f"GAME file {unit['ovr_offset'] + entry:#x} sha {targets[entry]['sha']}")
    return targets, dict(decoded=store_decoded, conditional=conditionals(store_decoded)), code, image


def library(directory):
    path = directory / "u3b0s.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                    str(ROOT / "src/recovered/overlay_u3b0_space_scan_cpu.c"), str(ROOT / "src/recovered/system_strstore_fullstate_0b25.c"),
                    "-o", str(path)], check=True, capture_output=True, text=True)
    lib = ctypes.CDLL(str(path))
    functions = {}
    for entry, spec in ROUTINES.items():
        function = getattr(lib, spec["name"])
        function.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(ctypes.c_int)]
        function.restype = ctypes.c_int
        functions[entry] = function
    return functions


def characters(rng, n, lead=None, trail=None, density=None):
    """n string bytes; the first min(n, 28h) (the copied part) hold `lead` leading and `trail` trailing spaces."""
    k = min(n, CAP)
    if density is not None:
        head = [SPACE if rng.random() < density else rng.choice(NONSPACE) for _ in range(k)]
    else:
        lead = lead or 0
        trail = trail or 0
        head = [SPACE] * lead + [rng.choice(NONSPACE) for _ in range(k - lead - trail)] + [SPACE] * trail
    return head + [rng.choice((SPACE, rng.randrange(256))) for _ in range(n - k)]


def build(image, code, rng, entry, *, n, chars, sp=0x8000, ss=SS, src=None, link=None, flags=0x7202, background=None,
          patch=()):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    memory[0:len(image)] = image
    base = physical(CALLER_CS, 0)
    memory[base:base + len(code)] = code
    for offset, value in patch:
        memory[physical(CALLER_CS, offset)] = value
    src = src or (0x5000 + rng.randrange(0x100), rng.randrange(0x10000))
    memory[physical(src[0], src[1])] = n
    for i, value in enumerate(chars):
        memory[physical(src[0], (src[1] + 1 + i) & 0xFFFF)] = value
    words = [RET_IP, RET_CS, rng.randrange(0x10000) if link is None else link, src[1], src[0]]
    frame = physical(ss, sp)
    for i, value in enumerate(words):                   # both views agree at a segment end (Unicorn reads contiguously)
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
        put16(memory, 0, (frame + 2 * i) & (SIZE - 1), value)
    reg = tuple(rng.randrange(0x10000) for _ in range(7))
    high = any(s >= 0xF000 for s in (src[0], ss))
    cpu = Cpu(*reg, sp, CALLER_CS, entry, rng.randrange(0x10000), rng.randrange(0x10000), ss, flags)
    return memory, cpu, high


def compare(label, memory, cpu, high, target, store, function, status, budget=20000):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory))
    original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    start, body_end = physical(CALLER_CS, target["entry"]), physical(CALLER_CS, target["end"])
    store_start, store_end = physical(STORE_CS, STORE_IP), physical(STORE_CS, STORE_END)
    ret = physical(RET_CS, RET_IP)
    stop_at = physical(CALLER_CS, target["site"]) if status == CODE else None
    previous, store_previous, visited, edges, store_visited, store_edges, done = [None], [None], set(), set(), set(), set(), [False]

    def instruction(emulator, address, size, user):
        del size, user
        if address == stop_at:
            done[0] = True
            emulator.emu_stop()
            return
        here = address - start + target["entry"]
        visited.add(here)
        if previous[0] in target["conditional"] and previous[0] != here:
            edges.add((previous[0], here))
        previous[0] = here

    def library_instruction(emulator, address, size, user):
        del emulator, size, user
        here = address - store_start + STORE_IP
        store_visited.add(here)
        if store_previous[0] in store["conditional"] and store_previous[0] != here:
            store_edges.add((store_previous[0], here))
        store_previous[0] = here

    def outside(emulator, address, size, user):
        del size, user
        previous[0] = None
        if not store_start <= address < store_end:
            store_previous[0] = None
        if address == ret:
            done[0] = True
            emulator.emu_stop()

    def mirror(emulator, access, address, size, value, user):
        del access, user
        for byte in range(size):
            raw = address + byte
            at = raw & (SIZE - 1)
            emulator.mem_write(at + SIZE if raw < SIZE else at, bytes([(value >> (8 * byte)) & 0xFF]))

    original.hook_add(UC_HOOK_CODE, instruction, begin=start, end=body_end - 1)
    original.hook_add(UC_HOOK_CODE, library_instruction, begin=store_start, end=store_end - 1)
    original.hook_add(UC_HOOK_CODE, outside, begin=0, end=start - 1)
    original.hook_add(UC_HOOK_CODE, outside, begin=body_end, end=2 * SIZE - 1)
    if high:
        original.hook_add(UC_HOOK_MEM_WRITE, mirror)
    original.emu_start(start, 2 * SIZE, count=budget)
    check(done[0], f"original did not reach its {'site' if stop_at else 'return'}: {label}")
    expected = bytes(original.mem_read(0, SIZE))
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    callee = ctypes.c_int(99)
    got = function(ram, SIZE, ctypes.byref(cpu), ctypes.byref(callee))
    check(got == status and callee.value == (0 if status == OK else -1), f"C status {got}/{callee.value}: {label}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f"RAM mismatch {label}: {[(hex(d), actual[d], expected[d]) for d in differences[:6]]} ({len(differences)} bytes)")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        mine, want = getattr(cpu, name), original.reg_read(register) & 0xFFFF
        check(mine == want, f"{name} mismatch {label}: C={mine:04x}, original={want:04x}")
    return visited, edges, store_visited, store_edges


def cases(entry, rng):
    key = ROUTINES[entry]["key"]
    other = "trail" if key == "lead" else "lead"
    spec = []
    for k in range(CAP + 1):                                         # every copied length, spaces at the scanned end
        for count in sorted({0, 1, k // 2, max(k - 1, 0), k}):
            if count <= k:
                spare = rng.randrange(k - count + 1)
                spec.append((f"{key}-{k:02x}-{count:02x}", dict(n=k, chars=characters(rng, k, **{key: count, other: spare}))))
    for n in (0x29, 0x2A, 0x30, 0x80, 0xFF):                         # caller lengths above the 28h cap
        for count in (0, 1, CAP - 1, CAP):
            spec.append((f"long-{n:02x}-{count:02x}", dict(n=n, chars=characters(rng, n, **{key: count}))))
    for i in range(80):
        n = rng.choice((rng.randrange(CAP + 1), rng.randrange(CAP + 1), rng.randrange(256)))
        density = (0.0, 0.3, 0.7, 0.95, 1.0)[i % 5]
        spec.append((f"random-{i}", dict(n=n, chars=characters(rng, n, density=density), sp=rng.randrange(0x0100, 0xFF00) & ~1,
                                         flags=(0x7202, 0x0602, 0x0ED7, 0x0647, 0xF2D7)[i % 5])))
    for delta in (-0x3C, -0x3B, -0x3A, -0x38, -0x30, -0x2E, -0x2D, -0x2C, -0x2B, -0x2A, -0x20, -0x10, -0x04, -0x02, 0, 2, 4, 6, 8):
        n = rng.randrange(256)                                       # S inside the frame: pushes and the copy reshape it
        spec.append((f"frame-alias{delta:+x}", dict(n=n, chars=characters(rng, n, density=0.6), src=(SS, (0x8000 + delta) & 0xFFFF))))
    for sp in (0x0004, 0x0010, 0x002C, 0x002E, 0x0040, 0x8001, 0xFFF6, 0xFFF8, 0xFFFA, 0xFFFC, 0xFFFE):
        n = rng.randrange(CAP + 1)
        spec.append((f"sp-{sp:04x}", dict(n=n, chars=characters(rng, n, density=0.5), sp=sp, ss=EDGE_SS)))
    for ss, sp, src in ((0xFFFF, 0x0400, None), (0xFFF0, 0x0100, None), (SS, 0x8000, (0xFFFF, 0x0100)), (SS, 0x8000, (0xFFF8, 0xFFF0))):
        n = rng.randrange(CAP + 1)
        spec.append((f"a20-{ss:04x}-{sp:04x}", dict(n=n, chars=characters(rng, n, density=0.5), ss=ss, sp=sp, src=src)))
    spec.append(("random-background", dict(n=0x20, chars=characters(rng, 0x20, density=0.5), background=bytes(random.Random(entry).randbytes(SIZE)))))
    site = ROUTINES[entry]["site"]
    for offset, value in ((site, 0x90), (site + 1, 0x26), (site + 2, 0x0C)):
        spec.append((f"code-{offset - site}", dict(n=5, chars=characters(rng, 5, density=0.5), patch=((offset, value),), status=CODE)))
    return spec


def differential(targets, store, code, image, functions):
    rng = random.Random(0x3B008D7)
    report, store_visited, store_edges = {}, set(), set()
    for entry, target in targets.items():
        visited, edges, returned, stopped = set(), set(), 0, 0
        for label, options in cases(entry, rng):
            status = options.pop("status", OK)
            memory, cpu, high = build(image, code, rng, entry, **options)
            v, e, sv, se = compare(f"{target['label']} {label}", memory, cpu, high, target, store, functions[entry], status)
            visited |= v
            edges |= e
            store_visited |= sv
            store_edges |= se
            returned += status == OK
            stopped += status == CODE
        missing = sorted(hex(i.address) for i in target["decoded"] if i.address not in visited)
        missing_edges = sorted((hex(s), hex(o)) for s, outs in target["conditional"].items() for o in outs if (s, o) not in edges)
        check(not missing and not missing_edges, f"{target['label']} coverage incomplete: {missing} {missing_edges}")
        print(f"U3B0 {target['label']}: {returned} returning and {stopped} CODE-stop original-byte cases passed, full 1 MiB RAM and "
              f"all 14 CPU words equal; {len(visited)}/{len(target['decoded'])} instruction starts, "
              f"{len(edges)}/{2 * len(target['conditional'])} conditional outcomes")
        report[target["label"]] = dict(returning=returned, code_stops=stopped, starts=len(visited), outcomes=len(edges))
    missing = sorted(hex(i.address) for i in store["decoded"] if i.address not in store_visited)
    check(not missing and len(store_edges) == 2 * len(store["conditional"]), f"string store coverage incomplete: {missing}")
    print(f"composed string store: {len(store_visited)}/{len(store['decoded'])} instruction starts, "
          f"{len(store_edges)}/{2 * len(store['conditional'])} conditional outcomes")
    report["store"] = dict(starts=len(store_visited), outcomes=len(store_edges))
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unicorn", action="store_true")
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    targets, store, code, image = source()
    report = dict(targets={t["label"]: dict(sha256=t["sha"], bytes=len(t["body"]), instructions=len(t["decoded"]),
                                            file_offset=t["file_offset"]) for t in targets.values()})
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-u3b0s-") as temp:
            functions = library(Path(temp))
            if args.unicorn:
                report["unicorn"] = differential(targets, store, code, image, functions)
            if args.native:
                from u3b0_space_scan_native import native_audit
                report["native"] = native_audit(targets, code, image, functions)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
