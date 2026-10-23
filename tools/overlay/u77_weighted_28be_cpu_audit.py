#!/usr/bin/env python3
"""Original-byte full-state parity for raw-CPU U77 +28BE with the SYSTEM LongInt multiply and divide composed, plus DOSBox-X.

The differential places the hash-checked expanded START.EXE image at load segment 0, so the original LongInt
multiply (0AF8:0A5B) and divide (0AF8:0A76) run live from +28BE's two unrelocated, fixup-backed far calls. It
loads unit 0x77 and compares the entire 1 MiB RAM and all 14 CPU words between the original under Unicorn and
the C model, with no mask. The original decides each stop: its return (wherever RETF 4
goes), a rewritten call site (CODE, read live), a callee return elsewhere (TRANSFER) or the divide's jump to its
runtime-error target 0AF8:00E2 (CALLEE). Media stays private.
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

from pinned_shift_flags import LastFlagWriter, declared_flags_split, split_used

from capstone import CS_ARCH_X86, CS_MODE_16, Cs
from overlay_inventory import find_units, parse_mz
from resident_loader_map import unpack
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, OVR_SHA, REGISTERS, physical, put16

ENTRY, CODE_END, LISTED_END, BODY_RETF = 0x28BE, 0x2939, 0x299C, 0x2936
SITES = {0x28FD: "mul", 0x2922: "div"}
SYSTEM_CS, ERROR_TARGET = 0x0AF8, 0x00E2
LEAVES = dict(mul=(0x0A5B, 0x0A76, "74b9ab3b26ee7b54bf51f067f0825dea48772933b210dc5b38897464e5f65745"),
              div=(0x0A76, 0x0AE4, "131b97677c63a6daa216f4219868f045485e35f999a551feb16daf4ec50d69e5"))
CALLER_CS, SS, EDGE_SS, DS = 0x8800, 0x2800, 0x2000, 0x1800
RET_CS, RET_IP = 0x9001, 0x7000
OK, CODE, CALLEE, TRANSFER = 0, 2, 3, 4
NAMES = {OK: "return", CODE: "CODE", CALLEE: "CALLEE", TRANSFER: "TRANSFER"}


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
    leaves = {}
    for name, (start, end, sha) in LEAVES.items():
        body = image[SYSTEM_CS * 16 + start:SYSTEM_CS * 16 + end]
        check(sha256(body).hexdigest() == sha, f"SYSTEM {name} bytes differ")
        decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, start))
        check(sum(i.size for i in decoded) == len(body), f"SYSTEM {name} extent differs")
        retfs = [i.address for i in decoded if i.mnemonic == "retf"]
        leaves[name] = dict(start=start, end=end, retf=retfs[-1], body=body, decoded=decoded, conditional=conditionals(decoded))
    unit = {u["stub_para"]: u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)}[0x77]
    code = ovr[unit["ovr_offset"]:unit["ovr_offset"] + unit["code_size"]]
    starts = sorted(f["rel"] for f in unit["functions"])
    check(LISTED_END == next(s for s in starts if s > ENTRY), "U77 +28BE listed bounds differ")
    body = code[ENTRY:CODE_END]
    decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    calls = sorted((i.address, i.op_str) for i in decoded if "call" in i.mnemonic)
    fixups = set(unit["fixups"])
    check(sum(i.size for i in decoded) == len(body) == 123 and len(decoded) == 44 and decoded[-1].address == BODY_RETF
          and decoded[-1].op_str == "4" and calls == [(0x28FD, "0xaf8, 0xa5b"), (0x2922, "0xaf8, 0xa76")]
          and all(s + 3 in fixups for s, _ in calls), f"U77 +28BE extent or calls differ: {calls}")
    conditional = conditionals(decoded)
    target = dict(body=body, decoded=decoded, conditional=conditional, sha=sha256(body).hexdigest(), file_offset=unit["ovr_offset"] + ENTRY,
                  leaves=leaves)
    print(f"U77 +28BE ({len(body)}/{len(decoded)}; +2939..+299B are text data; LCALL 0AF8:0A5B, 0AF8:0A76, RETF 4) GAME file "
          f"{target['file_offset']:#x} sha {target['sha']}; {len(conditional)} conditionals")
    return target, code, image


CHECKER_ROWS = ('OVL_0077_28BE',)


def library(directory):
    path = directory / "u77w.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                    str(ROOT / "src/recovered/overlay_u77_weighted_28be_cpu.c"), str(ROOT / "src/recovered/system_longmul_0a5b_cpu.c"),
                    str(ROOT / "src/recovered/system_longdiv_0a76_cpu.c"), "-o", str(path)], check=True, capture_output=True, text=True)
    function = ctypes.CDLL(str(path)).pr_u77_weighted_28be_cpu
    function.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(ctypes.c_int)]
    function.restype = ctypes.c_int
    return function


def value(rng):
    return rng.choice((rng.randrange(1 << 12), (1 << 32) - rng.randrange(1, 1 << 10), rng.randrange(1 << 32), rng.randrange(1 << 20)))


def world_spec(rng, divisor=None):
    weights = [value(rng) for _ in range(5)]
    weights[3] = value(rng) if divisor is None else divisor                 # DS:0D30 is w[3]
    return dict(record=[rng.choice((rng.randrange(1 << 8), rng.randrange(1 << 16), 0xFFFF)) for _ in range(5)], weights=weights)


def build(image, code, rng, spec, *, sp=0x8000, ss=SS, ds=DS, record_at=None, flags=0x7202, background=None, patch=(), base=0x0000):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    memory[0:len(image)] = image
    start = physical(CALLER_CS, base)
    memory[start:start + len(code)] = code
    for offset, value_ in patch:
        memory[physical(CALLER_CS, (base + offset) & 0xFFFF)] = value_
    record_at = record_at or (0x4000 + rng.randrange(0x2000), rng.randrange(0x10000))
    for k, v in enumerate(spec["record"]):
        put16(memory, record_at[0], (record_at[1] + 0x88 + 2 * k) & 0xFFFF, v)
    for k, w in enumerate(spec["weights"]):
        put16(memory, ds, (0x0D24 + 4 * k) & 0xFFFF, w & 0xFFFF)
        put16(memory, ds, (0x0D26 + 4 * k) & 0xFFFF, w >> 16)
    frame = physical(ss, sp)
    for i, v in enumerate([RET_IP, RET_CS, record_at[1], record_at[0]]):  # both views agree at a segment end
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, v)
        put16(memory, 0, (frame + 2 * i) & (SIZE - 1), v)
    reg = tuple(rng.randrange(0x10000) for _ in range(7))
    high = any(seg >= 0xF000 for seg in (ss, record_at[0], ds))
    return memory, Cpu(*reg, sp, CALLER_CS, (base + ENTRY) & 0xFFFF, ds, rng.randrange(0x10000), ss, flags), high


def compare(label, memory, cpu, high, target, function, base=0x0000, budget=400000):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory))
    original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    origin = physical(cpu.cs, base)
    body = (origin + ENTRY, origin + CODE_END)
    ranges = {name: (physical(SYSTEM_CS, leaf["start"]), physical(SYSTEM_CS, leaf["end"])) for name, leaf in target["leaves"].items()}
    sites = {origin + s: (origin + s + 5, bytes([0x9A]) + struct.pack("<H", target["leaves"][name]["start"])) for s, name in SITES.items()}
    error = physical(SYSTEM_CS, ERROR_TARGET)
    state = dict(stop=None, expected=None, check=None, returning=False, previous=None)
    seen = {k: set() for k in ("body", *LEAVES)}
    edges = {k: set() for k in seen}

    writer = LastFlagWriter()  # declared split (pinned_shift_flags): AF after a non-zero shift, DIV's six bits, else exact

    def note(kind, here, conditional):
        seen[kind].add(here)
        if state["previous"] is not None and state["previous"][0] == kind and state["previous"][1] in conditional and state["previous"][1] != here:
            edges[kind].add((state["previous"][1], here))
        state["previous"] = (kind, here)

    def halt(emulator, stop):
        state["stop"] = stop
        emulator.emu_stop()

    def hook(emulator, address, size, user):
        del size, user
        if state["returning"]:
            return halt(emulator, OK)
        if state["check"] is not None:
            expected, state["check"] = state["check"], None
            if address != expected:
                return halt(emulator, TRANSFER)
        if address == error:
            return halt(emulator, CALLEE)
        if address in sites:
            if bytes(emulator.mem_read(address, 3)) != sites[address][1]:
                return halt(emulator, CODE)
            state["expected"] = sites[address][0]
        if body[0] <= address < body[1]:
            writer.step(emulator, address)
            note("body", address - origin, target["conditional"])
            if address - origin == BODY_RETF:
                state["returning"] = True
            return None
        for name, (start, end) in ranges.items():
            if start <= address < end:
                writer.step(emulator, address)
                leaf = target["leaves"][name]
                here = address - start + leaf["start"]
                note(name, here, leaf["conditional"])
                if here == leaf["retf"]:
                    state["check"] = state["expected"]
                return None
        raise ValueError(f"unexpected fetch at {address:#x}: {label}")

    def mirror(emulator, access, address, size, value_, user):
        del access, user
        for byte in range(size):
            raw = address + byte
            at = raw & (SIZE - 1)
            emulator.mem_write(at + SIZE if raw < SIZE else at, bytes([(value_ >> (8 * byte)) & 0xFF]))

    original.hook_add(UC_HOOK_CODE, hook)
    if high:
        original.hook_add(UC_HOOK_MEM_WRITE, mirror)
    original.emu_start(body[0], 2 * SIZE, count=budget)
    writer.finish(original)
    check(state["stop"] is not None, f"original did not stop: {label}")
    expected = bytes(original.mem_read(0, SIZE))
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    callee = ctypes.c_int(99)
    status = function(ram, SIZE, ctypes.byref(cpu), ctypes.byref(callee))
    check(status == state["stop"], f"C status {status}, original stopped with {NAMES[state['stop']]}: {label}")
    check(state["stop"] != CALLEE or callee.value == 2, f"C callee status {callee.value} at a divide error: {label}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f"RAM mismatch {label}: {[(hex(d), actual[d], expected[d]) for d in differences[:6]]} ({len(differences)} bytes)")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        mine, want = getattr(cpu, name), original.reg_read(register) & 0xFFFF
        if name == "flags":
            problem = declared_flags_split(mine, want, writer)
            check(problem is None, f"flags mismatch {label}: {problem}")
            SPLITS[0] += split_used(mine, want, writer)
            continue
        check(mine == want, f"{name} mismatch {label}: C={mine:04x}, original={want:04x}")
    return state["stop"], seen, edges


SPLITS = [0]   # arrivals where C's pinned flags differ from raw Unicorn under the declared split


def cases(rng):
    spec = []
    for i in range(200):
        spec.append((f"random-{i}", dict(spec=world_spec(rng), sp=rng.randrange(0x0400, 0xEE00) & ~1,
                                          flags=(0x7202, 0x0602, 0x0ED7, 0x72D7)[i % 4])))   # bit 15 stays clear, as on a 386
    for divisor in (1, 0xFFFFFFFF, 100, 0x80000000, 0x7FFFFFFF, 3, 0xFFFFFFFD):
        spec.append((f"divisor-{divisor:08x}", dict(spec=world_spec(rng, divisor))))
    spec.append(("min-by-minus-one", dict(spec=dict(record=[0x8000, 0, 0, 0, 0], weights=[0xFFFF, 0, 0, 0xFFFFFFFF, 0]))))
    for k in range(3):
        spec.append((f"zero-divisor-{k}", dict(spec=world_spec(rng, 0), flags=(0x7202, 0x0ED7, 0x0002)[k])))
    sp = 0x8000
    bp = sp - 2
    spec.append(("record-on-j", dict(spec=world_spec(rng), record_at=(SS, (bp - 9 - 0x88) & 0xFFFF))))   # the values are j and the sum
    spec.append(("record-on-sum", dict(spec=world_spec(rng), record_at=(SS, (bp - 8 - 0x88) & 0xFFFF))))
    spec.append(("record-on-stack", dict(spec=world_spec(rng), record_at=(SS, (bp - 0x18 - 0x88) & 0xFFFF))))
    spec.append(("weights-under-stack", dict(spec=world_spec(rng), ds=SS, sp=0x0D46)))   # pushes rewrite w[3] mid-loop
    spec.append(("weights-on-frame", dict(spec=world_spec(rng), ds=SS, sp=0x0D24 + 0x10)))
    for sp_ in (0x000E, 0x0010, 0x0040, 0xFFF0, 0xFFF8, 0xFFFE):          # the multiply's RETF frame at the segment end, and the entry frame
        spec.append((f"sp-{sp_:04x}", dict(spec=world_spec(rng), sp=sp_, ss=EDGE_SS)))
    spec.append(("ss-a20", dict(spec=world_spec(rng), sp=0x0400, ss=0xFFFF)))
    spec.append(("record-a20", dict(spec=world_spec(rng), record_at=(0xFFFF, 0x0100))))
    spec.append(("ds-a20", dict(spec=world_spec(rng), ds=0xFFF8)))
    spec.append(("random-background", dict(spec=world_spec(rng), background=bytes(random.Random(0x7728BE).randbytes(SIZE)))))
    spec.append(("relocated-entry", dict(spec=world_spec(rng), base=0x1000)))
    for k, (site, offset, v) in enumerate(((0x28FD, 0, 0x90), (0x28FD, 1, 0x5C), (0x2922, 2, 0x0B), (0x2922, 0, 0xCB))):
        spec.append((f"code-{site:04x}-{offset}", dict(spec=world_spec(rng), patch=((site + offset, v),), flags=(0x7202, 0x0ED7)[k & 1])))
    return spec


def differential(target, code, image, function):
    rng = random.Random(0x7728BE)
    seen = {k: set() for k in ("body", *LEAVES)}
    edges = {k: set() for k in seen}
    stops = {k: 0 for k in NAMES}
    for label, options in cases(rng):
        base = options.get("base", 0)
        memory, cpu, high = build(image, code, rng, options.pop("spec"), **options)
        stop, s, e = compare(f"+28BE {label}", memory, cpu, high, target, function, base)
        stops[stop] += 1
        for kind in seen:
            seen[kind] |= s[kind]
            edges[kind] |= e[kind]
    report = {}
    for kind in seen:
        decoded, conditional = (target["decoded"], target["conditional"]) if kind == "body" else \
            (target["leaves"][kind]["decoded"], target["leaves"][kind]["conditional"])
        missing = sorted(hex(i.address) for i in decoded if i.address not in seen[kind])
        reachable = {(s, o) for s, outs in conditional.items() for o in outs}
        missing_edges = sorted((hex(s), hex(o)) for s, o in reachable - edges[kind])
        check(not missing and not missing_edges, f"{kind} coverage incomplete: {missing} {missing_edges}")
        report[kind] = dict(starts=len(seen[kind]), instructions=len(decoded), outcomes=len(edges[kind]), reachable_outcomes=len(reachable))
        print(f"  {kind}: {len(seen[kind])}/{len(decoded)} instruction starts, {len(edges[kind])}/{len(reachable)} conditional outcomes")
    check(all(stops[k] for k in (CODE, CALLEE, TRANSFER)), f"a stop kind was not exercised: {stops}")
    print(f"U77 +28BE: declared flags split used at {SPLITS[0]} arrivals (pinned AF after the second SHL at a +28FD CODE stop, or pinned DIV flags)")
    print(f"U77 +28BE: {sum(stops.values())} original-byte cases passed ({stops[OK]} returns, {stops[CODE]} CODE, {stops[CALLEE]} CALLEE, "
          f"{stops[TRANSFER]} TRANSFER); full 1 MiB RAM and all 14 CPU words equal")
    report["stops"] = {NAMES[k]: v for k, v in stops.items()}
    return report


# ---- DOSBox-X synthetic oracle -------------------------------------------------------------------------
CALLER_N, SYS_N, REC_SEG, DATA_DS = 0x8800, 0x9000, 0x2000, 0x1900
MAIN_AT, MUL_BLOB, DIV_BLOB, UNIT_BLOB, CASES_AT = 0x0300, 0x0D40, 0x0D80, 0x1000, 0x5000
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xC800, 0xF000
FRAME_LOW, FRAME_SIZE = 0xEF80, 0x80
CASE_SIZE = 2 + 10 + 20
RECORD_BYTES = 28 + FRAME_SIZE
NATIVE_FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)


def native_cases():
    rng = random.Random(0x7728BED)
    cases = []
    for n in range(120):
        spec = world_spec(rng)
        while spec["weights"][3] == 0:
            spec = world_spec(rng)
        cases.append(dict(spec, flags=n % len(NATIVE_FLAGS)))
    return cases


def native_audit(target, code, function):
    directory = ROOT / "resources/oracle-runs/u77-28be-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    unit = bytearray(code)
    for site in SITES:
        struct.pack_into("<H", unit, site + 3, SYS_N)
    blobs = {name: target["leaves"][name]["body"] for name in LEAVES}
    cases = native_cases()
    text = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main", f".org {MAIN_AT - 0x100:#x}", "main:",
            "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}", "    cld"]
    for segment, offset, label, length in ((CALLER_N, 0, "unit", len(unit)), (SYS_N, LEAVES["mul"][0], "mul", len(blobs["mul"])),
                                           (SYS_N, LEAVES["div"][0], "div", len(blobs["div"]))):
        text += [f"    mov ax,{segment:#x}", "    mov es,ax", f"    mov si,offset {label}", f"    mov di,{offset:#x}", f"    mov cx,{length:#x}", "    rep movsb"]
    text += ["    mov ax,cs", "    mov es,ax",
             f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
             f"    mov word ptr [{HEADER + 4:#x}],{len(cases)}", f"    mov word ptr [{HEADER + 6:#x}],0",
             "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
             "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8", "    mov ah,0x40", "    int 0x21", "    jc failure",
             "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    mov ax,cs", "    mov ds,ax", "    mov es,ax",
             "    mov al,0xa5", f"    mov di,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE}", "    rep stosb",
             "    mov ax,word ptr [index]", f"    mov bx,{CASE_SIZE}", "    mul bx", "    add ax,offset cases", "    mov si,ax",
             f"    mov ax,{REC_SEG:#x}", "    mov es,ax", "    mov di,0x88", "    add si,2", "    mov cx,10", "    rep movsb",
             f"    mov ax,{DATA_DS:#x}", "    mov es,ax", "    mov di,0x0d24", "    mov cx,20", "    rep movsb",
             f"    sub si,{CASE_SIZE}", "    mov ax,cs", "    mov es,ax",
             f"    mov ax,{REC_SEG:#x}", "    push ax", "    xor ax,ax", "    push ax",
             "    mov bl,byte ptr [si]", "    xor bh,bh", "    shl bx,1", "    push word ptr [flags_table+bx]", "    popf",
             f"    mov ax,{DATA_DS:#x}", "    mov ds,ax"]
    for name, v in ENTRY_REGS.items():
        text.append(f"    mov {name},{v:#x}")
    text += ["    call dword ptr cs:[target_far]", "returned:"]
    for i, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        text.append(f"    mov word ptr cs:[{OUTPUT + 2 * i:#x}],{reg}")
    text += [f"    mov word ptr cs:[{OUTPUT + 18:#x}],offset returned", f"    mov word ptr cs:[{OUTPUT + 20:#x}],ds",
             f"    mov word ptr cs:[{OUTPUT + 22:#x}],es", f"    mov word ptr cs:[{OUTPUT + 24:#x}],ss",
             f"    mov sp,{STACK_TOP + 0x800:#x}", "    pushf", "    pop ax", f"    mov word ptr cs:[{OUTPUT + 26:#x}],ax",
             "    cld", "    mov ax,cs", "    mov es,ax", "    mov ds,ax", f"    mov di,{OUTPUT + 28:#x}",
             f"    mov si,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE}", "    rep movsb", "    sti",
             "    mov bx,word ptr [handle]", f"    mov dx,{OUTPUT:#x}", f"    mov cx,{RECORD_BYTES}", "    mov ah,0x40", "    int 0x21", "    jc failure",
             f"    cmp ax,{RECORD_BYTES}", "    jne failure",
             "    inc word ptr [index]", "    mov ax,word ptr [index]", f"    cmp ax,{len(cases)}", "    jb again",
             "    mov bx,word ptr [handle]", "    mov ah,0x3e", "    int 0x21", "    mov ax,0x4c00", "    int 0x21",
             "failure:", "    mov ax,0x4c01", "    int 0x21",
             'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", f"target_far: .word {ENTRY:#x}, {CALLER_N:#x}",
             "flags_table: .word " + ",".join(f"{f:#x}" for f in NATIVE_FLAGS)]
    for label, blob, at in (("mul", blobs["mul"], MUL_BLOB), ("div", blobs["div"], DIV_BLOB), ("unit", bytes(unit), UNIT_BLOB)):
        text += [f".org {at - 0x100:#x}", f"{label}:"]
        for i in range(0, len(blob), 16):
            text.append("    .byte " + ",".join(f"{b:#x}" for b in blob[i:i + 16]))
    text += [f".org {CASES_AT - 0x100:#x}", "cases:"]
    for case in cases:
        row = struct.pack("<BB", case["flags"], 0) + b"".join(struct.pack("<H", v) for v in case["record"]) \
            + b"".join(struct.pack("<I", w) for w in case["weights"])
        check(len(row) == CASE_SIZE, "case row size differs")
        text.append("    .byte " + ",".join(f"{b:#x}" for b in row))
    check(MUL_BLOB + len(blobs["mul"]) <= DIV_BLOB and DIV_BLOB + len(blobs["div"]) <= UNIT_BLOB
          and UNIT_BLOB + len(unit) <= CASES_AT and CASES_AT + CASE_SIZE * len(cases) <= OUTPUT, "COM layout overlaps")
    asm, obj = directory / "oracle.S", directory / "oracle.o"
    asm.write_text("\n".join(text) + "\n")
    subprocess.run(["as", "--32", str(asm), "-o", str(obj)], check=True)
    subprocess.run(["ld", "-m", "elf_i386", "-Ttext", "0x100", "--oformat", "binary", str(obj), "-o", str(directory / "ORACLE.COM")], check=True)
    config = directory / "dosbox.conf"
    config.write_text("[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n[cpu]\ncore=normal\ncputype=386\ncycles=max\n"
                      "[mixer]\nnosound=true\n[midi]\nmididevice=none\n"
                      "[autoexec]\nmount c " + str(directory.resolve()) + "\nc:\nORACLE.COM\nexit\n")
    result_path = directory / "RESULT.BIN"
    if result_path.exists():
        result_path.unlink()
    environment = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy")
    with (directory / "dosbox.log").open("w") as log:
        subprocess.run(["dosbox-x", "-conf", str(config), "-fastlaunch", "-exit", "-noconsole"],
                       env=environment, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=300)
    result = result_path.read_bytes()
    check(len(result) == 8 + len(cases) * RECORD_BYTES, f"native output incomplete: {len(result)} bytes")
    segment, returned, count, _ = struct.unpack_from("<4H", result)
    check(count == len(cases), "native header differs")
    negative = 0
    for n, case in enumerate(cases):
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        memory = bytearray(SIZE)
        for seg, off, blob in ((CALLER_N, 0, bytes(unit)), (SYS_N, LEAVES["mul"][0], blobs["mul"]), (SYS_N, LEAVES["div"][0], blobs["div"])):
            at = physical(seg, off)
            memory[at:at + len(blob)] = blob
        for i in range(FRAME_SIZE):
            memory[physical(segment, FRAME_LOW + i)] = 0xA5
        rec = b"".join(struct.pack("<H", v) for v in case["record"])
        memory[physical(REC_SEG, 0x88):physical(REC_SEG, 0x88) + 10] = rec
        weights = b"".join(struct.pack("<I", w) for w in case["weights"])
        memory[physical(DATA_DS, 0x0D24):physical(DATA_DS, 0x0D24) + 20] = weights
        sp = STACK_TOP
        for v in (REC_SEG, 0, segment, returned):
            sp -= 2
            put16(memory, segment, sp, v)
        cpu = Cpu(*ENTRY_REGS.values(), sp, CALLER_N, ENTRY, DATA_DS, segment, segment, NATIVE_FLAGS[case["flags"]])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        callee = ctypes.c_int(99)
        label = f"native case {n}"
        check(function(ram, SIZE, ctypes.byref(cpu), ctypes.byref(callee)) == 0 and callee.value == 0, f"C status on {label}")
        for (name, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, name) == want, f"{label} {name}: C={getattr(cpu, name):04x}, DOS={want:04x}")
        got = bytes(ram[physical(segment, FRAME_LOW):physical(segment, FRAME_LOW) + FRAME_SIZE])
        check(got == record[28:], f"{label} bytes differ")
        negative += cpu.dx >= 0x8000
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {len(cases)} ({negative} negative quotients); all 14 CPU words and the "
          f"{FRAME_SIZE:#x}-byte stack window (sum, j, result and every callee frame); no mask")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}; multiply and divide copied to {SYS_N:04X}")
    return dict(cases=len(cases), result_sha256=digest)


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
    report = dict(target=dict(sha256=target["sha"], bytes=len(target["body"]), instructions=len(target["decoded"]), file_offset=target["file_offset"]))
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-u77w-") as temp:
            function = library(Path(temp))
            if args.unicorn:
                report["unicorn"] = differential(target, code, image, function)
            if args.native:
                report["native"] = native_audit(target, code, function)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
