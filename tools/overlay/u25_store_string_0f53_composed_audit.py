#!/usr/bin/env python3
"""Original-byte full-state parity for the composed U25 +0F53 store string with SYSTEM 0AF8:0B25 live.

The differential places the hash-checked expanded START.EXE image at load segment 0, so the
original string store runs at 0AF8:0B25 from both of +0F53's unrelocated, fixup-backed far calls.
It loads unit 0x25 (the body and its +06FB classifier) and compares the entire 1 MiB RAM and all
14 CPU words between the original under Unicorn and pr_u25_store_string_0f53_composed, with no
mask. The original decides each case's stop: its return (wherever RETF 6 goes), a rewritten call
site (CODE), a string-store return elsewhere (TRANSFER) or the host loop-back cap (LIMIT); the C
status must match. Media stays private.
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

ENTRY, END, CLASSIFIER, CLASSIFIER_END = 0x0F53, 0x1153, 0x06FB, 0x0756
BODY_SHA = "fb04baf47e5abfffab480e8494d662d5c49762e67ca4269b12128cc9e9fd3815"
CLASSIFIER_SHA = "e41b64e0284b3dff9398b1d8fe08f2c8c773f8521975021c8d10734118d39347"
STORE_CS, STORE_IP, STORE_END, STORE_RETF = 0x0AF8, 0x0B25, 0x0B49, 0x0B46
STORE_SHA = "009955722e8f73d0a0f7076aa418d141f642c5a729b2a58dfa7a7a3922989118"
SITES, NEAR_CALL, BODY_RETF = (0x0F69, 0x1010), 0x0F72, 0x1150
UNREACHABLE = {(0x0F9A, 0x0FD7), (0x1028, 0x1065), (0x1095, 0x10D2), (0x1101, 0x1138)}
LOOP_INC = (0x0FA2, 0x1030, 0x109D, 0x1109)
POINTERS, DISPLACEMENTS = (0x49D2, 0x49D6, 0x49DA, 0x49DE), (0x6E00, 0x2A00, 0xD200, 0x6700)
RANGES = ((0x4900, 0x4CFF), (0x6B00, 0x6EFF), (0x9700, 0x98FF), (0x9900, 0xB6FF))
CALLER_CS, DS, SS, EDGE_SS = 0x8800, 0x1800, 0x2800, 0x2000
RET_CS, RET_IP = 0x9001, 0x7000
OK, CODE, TRANSFER, LIMIT = 0, 3, 4, 5
NAMES = {OK: "return", CODE: "CODE", TRANSFER: "TRANSFER", LIMIT: "LIMIT"}


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
    unit = {u["stub_para"]: u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)}[0x25]
    code = ovr[unit["ovr_offset"]:unit["ovr_offset"] + unit["code_size"]]
    check(unit["entries"][12] == ENTRY, "unit 0x25 entry 12 differs")
    body, classifier = code[ENTRY:END], code[CLASSIFIER:CLASSIFIER_END]
    check(sha256(body).hexdigest() == BODY_SHA and sha256(classifier).hexdigest() == CLASSIFIER_SHA, "U25 +0F53 or +06FB bytes differ")
    decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    classifier_decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(classifier, CLASSIFIER))
    calls = [(i.address, i.mnemonic, i.op_str) for i in decoded if "call" in i.mnemonic]
    fixups = set(unit["fixups"])
    check(len(decoded) == 181 and sum(i.size for i in decoded) == 512 and decoded[-1].op_str == "6"
          and calls == [(SITES[0], "lcall", "0xaf8, 0xb25"), (NEAR_CALL, "call", "0x6fb"), (SITES[1], "lcall", "0xaf8, 0xb25")]
          and all(s + 3 in fixups for s in SITES), f"U25 +0F53 extent or calls differ: {calls}")
    check(len(classifier_decoded) == 28 and classifier_decoded[-1].op_str == "2", "U25 +06FB extent differs")
    store_decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(store, STORE_IP))
    target = dict(decoded=decoded, conditional=conditionals(decoded), classifier=classifier_decoded,
                  classifier_conditional=conditionals(classifier_decoded), store=store_decoded,
                  store_conditional=conditionals(store_decoded), sha=BODY_SHA, file_offset=unit["ovr_offset"] + ENTRY)
    check(len(target["conditional"]) == 13 and len(target["classifier_conditional"]) == 8, "conditional counts differ")
    print(f"U25 +0F53 (512/181, LCALL 0AF8:0B25 at +0F69 and +1010, CALL +06FB, RETF 6) GAME file {target['file_offset']:#x}; "
          f"image {len(image):#x} bytes")
    return target, code, image


CHECKER_ROWS = ('OVL_0025_0F53',)


def library(directory):
    path = directory / "u25s.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                    str(ROOT / "src/recovered/overlay_u25_store_string_0f53.c"), str(ROOT / "src/recovered/system_strstore_fullstate_0b25.c"),
                    "-o", str(path)], check=True, capture_output=True, text=True)
    function = ctypes.CDLL(str(path)).pr_u25_store_string_0f53_composed
    function.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_uint, ctypes.POINTER(ctypes.c_int)]
    function.restype = ctypes.c_int
    return function


def class_of(address):
    return next((c for c, (low, high) in enumerate(RANGES) if low <= address <= high), 4)


def build(image, code, rng, *, address, n, chars=None, sp=0x8000, ss=SS, src=None, windows=None, record=None, flags=0x7202,
          background=None, patch=(), extra=()):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    memory[0:len(image)] = image
    base = physical(CALLER_CS, 0)
    memory[base:base + len(code)] = code
    for offset, value in patch:
        memory[physical(CALLER_CS, offset)] = value
    src = src or (0x6000 + rng.randrange(0x1000), rng.randrange(0x10000))
    chars = [rng.randrange(256) for _ in range(255)] if chars is None else chars
    memory[physical(src[0], src[1])] = n
    for i, value in enumerate(chars):
        memory[physical(src[0], (src[1] + 1 + i) & 0xFFFF)] = value
    windows = windows or [(0x4000 + rng.randrange(0xF00), rng.randrange(0x10000)) for _ in range(4)]
    for pointer, (seg, off) in zip(POINTERS, windows):
        put16(memory, DS, pointer, off)
        put16(memory, DS, pointer + 2, seg)
    record = record or (0x7000 + rng.randrange(0x700), rng.randrange(0x8000))
    put16(memory, DS, 0x5D92, record[1])
    put16(memory, DS, 0x5D94, record[0])
    words = [RET_IP, RET_CS, src[1], src[0], address]
    frame = physical(ss, sp)
    for i, value in enumerate(words):                   # both views agree at a segment end (Unicorn reads contiguously)
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
        put16(memory, 0, (frame + 2 * i) & (SIZE - 1), value)
    for seg, off, value in extra:
        memory[physical(seg, off)] = value
    reg = tuple(rng.randrange(0x10000) for _ in range(7))
    high = any(s >= 0xF000 for s in (src[0], ss, record[0])) or any(seg >= 0xF000 for seg, _ in windows)
    cpu = Cpu(*reg, sp, CALLER_CS, ENTRY, DS, rng.randrange(0x10000), ss, flags)
    return memory, cpu, high


def compare(label, memory, cpu, high, target, function, cap, budget=400000):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory))
    original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    base = physical(CALLER_CS, 0)
    body = (base + ENTRY, base + END)
    classifier = (base + CLASSIFIER, base + CLASSIFIER_END)
    store = (physical(STORE_CS, STORE_IP), physical(STORE_CS, STORE_END))
    sites = {base + s: base + s + 5 for s in SITES}
    incs = {base + s for s in LOOP_INC}
    site_bytes = {s: bytes(memory[base + s:base + s + 3]) for s in SITES}
    state = dict(stop=None, expected=None, check_return=None, returning=False, arrivals=0, previous=None)
    seen = dict(body=set(), classifier=set(), store=set())
    edges = dict(body=set(), classifier=set(), store=set())

    def note(kind, here, conditional):
        seen[kind].add(here)
        if state["previous"] is not None and state["previous"][0] == kind and state["previous"][1] in conditional \
                and state["previous"][1] != here:
            edges[kind].add((state["previous"][1], here))
        state["previous"] = (kind, here)

    def hook(emulator, address, size, user):
        del size, user
        if state["returning"]:
            state["stop"] = OK
            emulator.emu_stop()
            return
        if state["check_return"] is not None:
            expected, state["check_return"] = state["check_return"], None
            if address != expected:
                state["stop"] = TRANSFER
                emulator.emu_stop()
                return
        if address in sites:
            offset = address - base
            if site_bytes[offset] != bytes([0x9A, 0x25, 0x0B]):
                state["stop"] = CODE
                emulator.emu_stop()
                return
            state["expected"] = sites[address]
        if address in incs:
            state["arrivals"] += 1
            if state["arrivals"] > cap:
                state["stop"] = LIMIT
                emulator.emu_stop()
                return
        if body[0] <= address < body[1]:
            note("body", address - base, target["conditional"])
            if address - base == BODY_RETF:
                state["returning"] = True
        elif classifier[0] <= address < classifier[1]:
            note("classifier", address - base, target["classifier_conditional"])
        elif store[0] <= address < store[1]:
            here = address - store[0] + STORE_IP
            note("store", here, target["store_conditional"])
            if here == STORE_RETF:
                state["check_return"] = state["expected"]
        else:
            raise ValueError(f"unexpected fetch at {address:#x}: {label}")

    def mirror(emulator, access, address, size, value, user):
        del access, user
        for byte in range(size):
            raw = address + byte
            at = raw & (SIZE - 1)
            emulator.mem_write(at + SIZE if raw < SIZE else at, bytes([(value >> (8 * byte)) & 0xFF]))

    original.hook_add(UC_HOOK_CODE, hook)
    if high:
        original.hook_add(UC_HOOK_MEM_WRITE, mirror)
    original.emu_start(body[0], 2 * SIZE, count=budget)
    check(state["stop"] is not None, f"original did not stop: {label}")
    expected = bytes(original.mem_read(0, SIZE))
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    callee = ctypes.c_int(99)
    status = function(ram, SIZE, ctypes.byref(cpu), cap, ctypes.byref(callee))
    check(status == state["stop"], f"C status {status}, original stopped with {NAMES[state['stop']]}: {label}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f"RAM mismatch {label}: {[(hex(d), actual[d], expected[d]) for d in differences[:6]]} ({len(differences)} bytes)")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        mine, want = getattr(cpu, name), original.reg_read(register) & 0xFFFF
        check(mine == want, f"{name} mismatch {label}: C={mine:04x}, original={want:04x}")
    return state["stop"], seen, edges, callee.value


def cases(rng):
    spec = []
    for c, (low, high) in enumerate(RANGES):                       # every class at its edges and middle
        for address in (low, low + 1, (low + high) // 2, high - 1, high):
            for n in (0, 1, 3, 20, 255):
                spec.append((f"class{c}-{address:04x}-n{n}", dict(address=address, n=n)))
    for address in (0x0000, 0x48FF, 0x4D00, 0x6AFF, 0x6F00, 0x96FF, 0xB700, 0xFFFF):
        for n in (0, 5):
            spec.append((f"class4-{address:04x}-n{n}", dict(address=address, n=n)))
    for n in (0, 1, 14, 15, 16, 255):                                # the 6B00 name copy, capped at 15
        spec.append((f"name-n{n}", dict(address=0x6B00, n=n)))
    bp = 0x8000 - 2
    spec.append(("name-onto-local", dict(address=0x6B00, n=40, record=(SS, (bp - 0x100) & 0xFFFF))))
    spec.append(("name-above-local", dict(address=0x6B00, n=40, record=(SS, (bp - 0x100 + 1) & 0xFFFF))))
    spec.append(("name-below-local", dict(address=0x6B00, n=40, record=(SS, (bp - 0x100 - 1) & 0xFFFF))))
    spec.append(("name-over-store-frame", dict(address=0x6B00, n=40, record=(SS, (bp - 0x112) & 0xFFFF))))      # TRANSFER
    spec.append(("name-over-own-frame", dict(address=0x6B00, n=40, record=(SS, (bp + 2) & 0xFFFF))))            # returns elsewhere
    for i in range(120):
        c = rng.randrange(5)
        address = rng.randrange(*((RANGES[c][0], RANGES[c][1] + 1) if c < 4 else (0, 0x10000)))
        n = rng.choice((0, rng.randrange(1, 40), rng.randrange(256)))
        spec.append((f"random-{i}", dict(address=address, n=n, sp=rng.randrange(0x0200, 0xFE00) & ~1,
                                         flags=(0x7202, 0x0602, 0x0ED7, 0x0647, 0xF2D7)[i % 5])))
    # element stores aliasing the frame, the classifier table and the pointer table (class 0, A = 4900h unless noted)
    def window_for(c, address, index, seg, target):
        width = 2 if c < 3 else 1
        return (seg, (target - (width * ((address + index) & 0xFFFF)) - DISPLACEMENTS[c]) & 0xFFFF)
    for name, c, address, index, target_offset, n in (
            ("ahead-in-local", 0, 0x4900, 1, bp - 0x100 + 5, 20), ("bound", 0, 0x4900, 2, bp - 0x104, 20),
            ("index", 0, 0x4900, 2, bp - 0x103, 20), ("length", 1, 0x6B01, 3, bp - 0x102, 20),
            ("address-arg", 2, 0x9700, 1, bp + 0x0A, 20), ("saved-bp", 3, 0x9900, 4, bp, 20),
            ("own-return", 0, 0x4CFF, 5, bp + 2, 20), ("byte-bound", 3, 0xB6FF, 2, bp - 0x104, 30)):
        windows = [(0x4000 + 0x100 * k, 0x1000 * k) for k in range(4)]
        windows[c] = window_for(c, address, index, SS, target_offset & 0xFFFF)
        for cap in (300, 2):
            spec.append((f"alias-{name}-cap{cap}", dict(address=address, n=n, windows=windows, cap=cap)))
    windows = [(0x4000 + 0x100 * k, 0x1000 * k) for k in range(4)]
    windows[0] = window_for(0, 0x4900, 3, DS, POINTERS[0])               # rewrites its own pointer mid-loop
    spec.append(("alias-pointer-table", dict(address=0x4900, n=12, windows=windows)))
    for sp in (0x0050, 0x0110, 0x0120, 0xFFF6, 0xFFF8, 0xFFFA, 0xFFFC, 0xFFFE):
        spec.append((f"sp-{sp:04x}", dict(address=rng.choice((0x4A00, 0x6B00, 0x9750, 0xA000, 0x0001)), n=rng.randrange(256), sp=sp, ss=EDGE_SS)))
    spec.append(("ss-a20", dict(address=0x4A00, n=30, ss=0xFFFF, sp=0x0400)))
    spec.append(("src-a20", dict(address=0x9720, n=30, src=(0xFFFF, 0x0100))))
    spec.append(("window-a20", dict(address=0x9900, n=30, windows=[(0xFFFF, 0x0000)] * 4)))
    spec.append(("random-background", dict(address=0x6C00, n=200, background=bytes(random.Random(0x0F53).randbytes(SIZE)))))
    for c in range(4):                                               # caps below the loop length
        spec.append((f"cap-class{c}", dict(address=RANGES[c][0] + (1 if c == 1 else 0), n=40, cap=c * 7)))
    spec.append(("code-site1-opcode", dict(address=0x4900, n=5, patch=((SITES[0], 0x90),))))
    spec.append(("code-site1-offset", dict(address=0x9900, n=5, patch=((SITES[0] + 2, 0x0C),))))
    spec.append(("code-site2-offset", dict(address=0x6B00, n=5, patch=((SITES[1] + 1, 0x26),))))
    return spec


def differential(target, code, image, function):
    rng = random.Random(0x25F53)
    counts = {kind: 0 for kind in NAMES}
    seen = dict(body=set(), classifier=set(), store=set())
    edges = dict(body=set(), classifier=set(), store=set())
    for label, options in cases(rng):
        cap = options.pop("cap", 300)
        memory, cpu, high = build(image, code, rng, **options)
        stop, s, e, callee = compare(f"+0F53 {label}", memory, cpu, high, target, function, cap)
        counts[stop] += 1
        for kind in seen:
            seen[kind] |= s[kind]
            edges[kind] |= e[kind]
    check(not (edges["body"] & UNREACHABLE), "an unreachable JA edge was taken")
    report = {}
    for kind, decoded, conditional, skip in (("body", target["decoded"], target["conditional"], UNREACHABLE),
                                              ("classifier", target["classifier"], target["classifier_conditional"], set()),
                                              ("store", target["store"], target["store_conditional"], set())):
        missing = sorted(hex(i.address) for i in decoded if i.address not in seen[kind])
        reachable = {(s, o) for s, outs in conditional.items() for o in outs} - skip
        missing_edges = sorted((hex(s), hex(o)) for s, o in reachable - edges[kind])
        check(not missing and not missing_edges, f"{kind} coverage incomplete: {missing} {missing_edges}")
        report[kind] = dict(starts=len(seen[kind]), outcomes=len(edges[kind]), reachable_outcomes=len(reachable))
        print(f"  {kind}: {len(seen[kind])}/{len(decoded)} instruction starts, {len(edges[kind])}/{len(reachable)} reachable conditional outcomes")
    check(all(counts[k] for k in (CODE, TRANSFER, LIMIT)), f"a stop kind was not exercised: {counts}")
    print(f"U25 +0F53 composed: {sum(counts.values())} original-byte cases passed ({counts[OK]} returns, {counts[CODE]} CODE, "
          f"{counts[TRANSFER]} TRANSFER, {counts[LIMIT]} LIMIT); full 1 MiB RAM and all 14 CPU words equal; the four never-taken JA edges absent")
    report["stops"] = {NAMES[k]: v for k, v in counts.items()}
    return report


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
    target, code, image = source()
    report = dict(target=dict(sha256=BODY_SHA, bytes=512, instructions=181, file_offset=target["file_offset"]))
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-u25s-") as temp:
            function = library(Path(temp))
            if args.unicorn:
                report["unicorn"] = differential(target, code, image, function)
            if args.native:
                from u25_store_string_0f53_composed_native import native_audit
                report["native"] = native_audit(target, code, image, function)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
