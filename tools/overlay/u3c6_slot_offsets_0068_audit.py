#!/usr/bin/env python3
"""Original-byte full-state parity for u3C6 +0068, plus a DOSBox-X synthetic oracle.

The Unicorn differential loads the original unit 0x3C6 code into one 1 MiB image,
runs the original and the C model on a copy, and compares the entire RAM and all
14 CPU words. For counts above 0x48 the slot tables at DS:5F27/5F28 + 4s run into
the stored bytes at DS:604A/6092 + s, so later passes read earlier stores; the
cases cover that overlap. With SS = DS the stores can also rewrite the frame's
count and counter bytes; one such world never returns, and the C host cap is
compared against the original stopped at the same arrival at +0080. Media stays
private.
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

UNIT_BASE = 0x3073A
TARGET = dict(entry=0x0068, end=0x00E3, insns=51, retf=0x00E2, conditionals=2, callers={(0x07D4, 0x0986)})
CS, DS, SS = 0x3000, 0x1000, 0x2000


def check(condition, message):
    if not condition:
        raise ValueError(message)


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, "START.EXE provenance differs")
    check(len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA, "GAME.OVR provenance differs")
    units = {u["stub_para"]: u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)}
    unit = units[0x3C6]
    check(unit["ovr_offset"] == UNIT_BASE and unit["memory_segment"] == 0x03E0, "unit 0x3C6 placement differs")
    code = ovr[UNIT_BASE:UNIT_BASE + unit["code_size"]]
    starts = sorted(f["rel"] for f in unit["functions"])
    decoder = Cs(CS_ARCH_X86, CS_MODE_16)
    body = code[TARGET["entry"]:TARGET["end"]]
    decoded = list(decoder.disasm(body, TARGET["entry"]))
    check(len(decoded) == TARGET["insns"] and decoded[-1].address == TARGET["retf"] and decoded[-1].mnemonic == "retf"
          and decoded[-1].op_str == "" and TARGET["end"] in starts and TARGET["entry"] in starts, "+0068 extent differs")
    check(not any("call" in i.mnemonic or i.mnemonic.startswith("int") for i in decoded), "+0068 is not a leaf")
    conditional = {i.address: (i.address + i.size, int(i.op_str, 16)) for i in decoded if i.mnemonic.startswith("j") and i.mnemonic != "jmp"}
    check(len(conditional) == TARGET["conditionals"], "+0068 conditional count differs")
    near = set()
    for function in starts:
        end = next((x for x in starts if x > function), len(code))
        aligned = {i.address: i for i in decoder.disasm(code[function:end], function)}
        for site, ins in aligned.items():
            if ins.mnemonic == "call" and ins.op_str.startswith("0x") and int(ins.op_str, 16) == TARGET["entry"]:
                check(aligned[site - 1].op_str == "cs", "+0068 caller lacks PUSH CS")
                near.add((function, site))
    check(near == TARGET["callers"], f"+0068 aligned near callers differ: {sorted(near)}")
    check(TARGET["entry"] not in unit["entries"], "+0068 is unexpectedly a public entry")
    target = dict(body=body, decoded=decoded, conditional=conditional, sha=sha256(body).hexdigest(), **TARGET)
    print(f"u3C6 +0068 ({len(body)}/{len(decoded)}, leaf) sha {target['sha'][:16]}...; callers {sorted(f'+{f:04X}@+{s:04X}' for f, s in near)}")
    return target, code


CHECKER_ROWS = ('OVL_03C6_0068',)


def library(directory):
    path = directory / "u3c6_0068.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2", "-fPIC", "-shared",
                    "-I", str(ROOT / "include"), str(ROOT / "src/recovered/overlay_u3c6_slot_offsets_0068.c"),
                    "-o", str(path)], check=True, capture_output=True, text=True)
    f = ctypes.CDLL(str(path)).pr_u3c6_slot_offsets_0068
    f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_uint]
    f.restype = ctypes.c_int
    return f


def fixture(code, *, seed=1, count=None, ds=DS, ss=SS, sp=0x8000, es=0x1111, flags=0x7202, grid=(0x5000, 0x0100),
            header=None, background=None, regs=None, pokes=None):
    rng = random.Random(seed)
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    at = physical(CS, 0); memory[at:at + len(code)] = code
    for i in range(0x5F20, 0x6200):
        memory[physical(ds, i)] = rng.randrange(256)
    memory[physical(ds, 0x5F2A)] = rng.randrange(256) if count is None else count
    put16(memory, ds, 0x6716, grid[1]); put16(memory, ds, 0x6718, grid[0])
    h = header if header is not None else (rng.randrange(256), rng.randrange(256))
    memory[physical(grid[0], (grid[1] + 2) & 0xFFFF)] = h[0] & 0xFF
    memory[physical(grid[0], (grid[1] + 3) & 0xFFFF)] = h[1] & 0xFF
    for offset, value in (pokes or {}).items():
        memory[physical(ds, offset)] = value
    frame = physical(ss, sp)
    for delta, value in ((0, 0x7000), (2, 0x7001)):
        put16(memory, ss, (sp + delta) & 0xFFFF, value)
        memory[(frame + delta) & (SIZE - 1)] = value & 0xFF; memory[(frame + delta + 1) & (SIZE - 1)] = value >> 8
    r = regs or (0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777)
    cpu = Cpu(*r, sp, CS, TARGET["entry"], ds, es, ss, flags)
    return memory, cpu, any(s >= 0xF000 for s in (ds, ss, grid[0]))


LOOP_INC = 0x0080          # JNE +00DD's target; the C cap stops here


def compare(label, memory, cpu, high, target, function, cap=1000, budget=400000):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory)); original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    start = physical(cpu.cs, target["entry"]); body_end = start + len(target["body"]); last = physical(cpu.cs, target["retf"])
    returned, stopped, arrivals, previous, visited, edges = [False], [False], [0], [None], set(), set()

    def instruction(emulator, address, size, user):
        del size, user
        if returned[0] or stopped[0]:
            emulator.emu_stop(); return
        here = address - start + target["entry"]
        check(start <= address < body_end, f"left the body: {label}")
        if here == LOOP_INC:
            arrivals[0] += 1
            if arrivals[0] == cap + 1:
                stopped[0] = True; emulator.emu_stop(); return
        visited.add(here)
        if previous[0] in target["conditional"]:
            edges.add((previous[0], here))
        previous[0] = here
        if address == last:
            returned[0] = True

    def mirror(emulator, access, address, size, value, user):
        del access, user
        for byte in range(size):
            raw = address + byte; at = raw & (SIZE - 1)
            emulator.mem_write(at + SIZE if raw < SIZE else at, bytes([(value >> (8 * byte)) & 0xFF]))

    original.hook_add(UC_HOOK_CODE, instruction)
    if high:
        original.hook_add(UC_HOOK_MEM_WRITE, mirror)
    original.emu_start(start, 2 * SIZE, count=budget)
    check(returned[0] or stopped[0], f"original neither returned nor reached the cap: {label}")
    expected = bytes(original.mem_read(0, SIZE))
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    status = function(ram, SIZE, ctypes.byref(cpu), cap)
    check(status == (0 if returned[0] else 2), f"C status {status}: {label}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f"RAM mismatch {label}: {[(hex(d), actual[d], expected[d]) for d in differences[:6]]} ({len(differences)} bytes)")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        got, want = getattr(cpu, name), original.reg_read(register) & 0xFFFF
        check(got == want, f"{name} mismatch {label}: C={got:04x}, original={want:04x}")
    if stopped[0]:
        return None, arrivals[0], None
    return visited, edges, arrivals[0]


# Original loop-backs (taken JNE +00DD) per returning case: n - 1 without aliasing; with SS = DS, k = 20h:
# count := 1F at pass 1F stops after passes 1..1F; count := 12 and counter := 10 add passes 11 and 12.
LOOPBACKS = {"count-00": 0, "count-01": 0, "count-ff": 0xFE, "cap-2f-exact": 0x2F,
             "alias-count-stops": 0x1E, "alias-counter-reset": 0x21}


def alias(k, v1, v2, g2=0x03):
    """SS = DS with [BP-2] (the count) at DS:604A+k-1 and [BP-1] (the counter) at DS:604A+k: pass k-1's
    first store rewrites the count to v1, and pass k's rewrites the counter to v2."""
    pokes = {0x5F27 + 4 * (k - 1): (v1 + g2) & 0xFF, 0x5F27 + 4 * k: (v2 + g2) & 0xFF}
    return dict(seed=k, count=0x40, ss=DS, sp=0x604D + k, header=(g2, 0x11), pokes=pokes)


def cases():
    rng = random.Random(0x3C60068)
    spec = []
    for count in (0, 1, 2, 7, 0x48, 0x49, 0x4A, 0x80, 0xC0, 0xFE, 0xFF):
        spec.append((f"count-{count:02x}", dict(seed=count + 1, count=count)))
    for i in range(160):
        spec.append((f"random-{i}", dict(seed=rng.randrange(1 << 30), flags=rng.choice((0x7202, 0x0202, 0x0ED7, 0x0647)),
                                        es=rng.randrange(0x10000), regs=tuple(rng.randrange(0x10000) for _ in range(7)))))
    base = dict(seed=7, count=0x30)
    spec.append(("grid-in-ds-over-targets", dict(base, grid=(DS, 0x604A + 4 - 2))))     # header +2 is a stored byte
    spec.append(("grid-in-ds-over-table", dict(base, grid=(DS, 0x5F27 + 8 - 2))))
    spec.append(("grid-a20", dict(base, grid=(0xFFFF, 0x0010))))
    spec.append(("grid-offset-wrap", dict(base, grid=(0x5000, 0xFFFE))))                # header bytes wrap to 0000/0001
    for sp in (0x0002, 0x0004, 0x0010, 0xFFF0, 0xFFFC, 0xFFFE):
        spec.append((f"sp-{sp:04x}", dict(base, sp=sp, ss=0x1800, background=bytes(SIZE))))
    spec.append(("ss-a20", dict(base, ss=0xFFFF, sp=0x0400)))
    spec.append(("ds-a20", dict(base, ds=0xFFFF)))
    spec.append(("random-background", dict(base, background=bytes(random.Random(9).randbytes(SIZE)))))
    spec.append(("preserved-df", dict(base, flags=0x0603)))
    spec.append(("alias-count-stops", alias(0x20, 0x1F, 0x00)))            # count := 1F at pass 1F: stops there
    spec.append(("alias-counter-reset", alias(0x20, 0x12, 0x10)))          # count := 12, counter := 10: passes 11, 12
    spec.append(("alias-cycle-limit", alias(0x20, 0x60, 0x05), 600))      # counter cycles 6..20, never 60: endless
    for cap in (0, 1, 7, 0x2E):
        spec.append((f"cap-{cap:02x}-limit", dict(base), cap))              # count 30h needs 2Fh loop-backs
    spec.append(("cap-2f-exact", dict(base), 0x2F))
    return [entry if len(entry) == 3 else entry + (1000,) for entry in spec]


def differential(target, code, function):
    visited, edges, count, limited, pinned = set(), set(), 0, [], 0
    for label, options, cap in cases():
        memory, cpu, high = fixture(code, **options)
        v, e, loopbacks = compare(f"+0068 {label}", memory, cpu, high, target, function, cap)
        if v is None:
            limited.append(f"{label}@{e}")
            continue
        if label in LOOPBACKS:
            check(loopbacks == LOOPBACKS[label], f"{label}: original took {loopbacks} loop-backs")
            pinned += 1
        visited |= v; edges |= e; count += 1
    check(len(limited) == 5 and pinned == len(LOOPBACKS), f"unexpected LIMIT set {limited} or pins {pinned}")
    missing_sites = sorted(set(i.address for i in target["decoded"]) - visited)
    missing_edges = sorted({(s, o) for s, outs in target["conditional"].items() for o in outs} - edges)
    check(not missing_sites and not missing_edges, f"+0068 coverage incomplete: {missing_sites} {missing_edges}")
    print(f"+0068: {count} returning original-byte cases passed, full 1 MiB RAM and all 14 CPU words equal; "
          f"{len(visited)}/{len(target['decoded'])} instruction starts, {len(edges)}/{2 * len(target['conditional'])} conditional outcomes")
    print(f"+0068: {len(limited)} host-cap stops equal the original stopped at the same +0080 arrival: {', '.join(limited)}")
    return dict(returning=count, limit_stops=len(limited), starts=len(visited), outcomes=len(edges))


# ---- DOSBox-X synthetic oracle -------------------------------------------------------------------------
MAIN_AT, FIXTURES_AT, U3C6_AT = 0x0300, 0x0900, 0x1000
GRID, OUTPUT, HEADER, STACK_TOP = 0x8000, 0x9000, 0x9800, 0xF000
FRAME_LOW, FRAME_SIZE = 0xEFC0, 0x40
DS_AREA = (0x5F20, 0x0410)            # every slot read for counts 0..0xFF ends at DS:6324
FIXTURE_SIZE = 6
RECORD_BYTES = 28 + FRAME_SIZE + DS_AREA[1]
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)


def native_cases():
    rng = random.Random(0x3C60069)
    return [dict(count=c, seed=rng.randrange(256), h2=rng.randrange(256), h3=rng.randrange(256), flags=i % len(FLAGS))
            for i, c in enumerate([0, 1, 2, 0x10, 0x48, 0x49, 0x60, 0x80, 0xFF] + [rng.randrange(256) for _ in range(27)])]


def assembly(cases_):
    text = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main",
            f".org {MAIN_AT - 0x100:#x}", "main:",
            "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}",
            "    mov ax,cs", f"    add ax,{U3C6_AT // 16:#x}", "    mov word ptr [target_far+2],ax",
            f"    mov word ptr [0x6716],{GRID:#x}", "    mov word ptr [0x6718],cs",
            f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
            f"    mov word ptr [{HEADER + 4:#x}],{len(cases_)}", f"    mov word ptr [{HEADER + 6:#x}],0",
            "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
            "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8",
            "    mov ah,0x40", "    int 0x21", "    jc failure",
            "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    push cs", "    pop es",
            "    mov al,0xa5", f"    mov di,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE}", "    rep stosb",
            "    mov ax,word ptr [index]", f"    mov bx,{FIXTURE_SIZE}", "    mul bx", "    add ax,offset fixtures", "    mov si,ax",
            # DS area: seed + 11*i
            "    mov al,byte ptr [si+1]", f"    mov di,{DS_AREA[0]:#x}", f"    mov cx,{DS_AREA[1]:#x}", "area_loop:",
            "    mov byte ptr [di],al", "    add al,11", "    inc di", "    loop area_loop",
            "    mov al,byte ptr [si]", "    mov byte ptr [0x5f2a],al",
            "    mov al,byte ptr [si+2]", f"    mov byte ptr [{GRID + 2:#x}],al",
            "    mov al,byte ptr [si+3]", f"    mov byte ptr [{GRID + 3:#x}],al",
            "    mov bl,byte ptr [si+4]", "    xor bh,bh", "    shl bx,1", "    push word ptr [flags_table+bx]", "    popf"]
    for name, value in ENTRY_REGS.items():
        text.append(f"    mov {name},{value:#x}")
    text += ["    call dword ptr [target_far]", "returned:"]
    for i, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        text.append(f"    mov word ptr [{OUTPUT + 2 * i:#x}],{reg}")
    text += [f"    mov word ptr [{OUTPUT + 18:#x}],offset returned", f"    mov word ptr [{OUTPUT + 20:#x}],ds",
             f"    mov word ptr [{OUTPUT + 22:#x}],es", f"    mov word ptr [{OUTPUT + 24:#x}],ss",
             f"    mov sp,{STACK_TOP + 0x800:#x}", "    pushf", "    pop ax", f"    mov word ptr [{OUTPUT + 26:#x}],ax",
             "    cld", "    push cs", "    pop ds", "    push cs", "    pop es", f"    mov di,{OUTPUT + 28:#x}",
             f"    mov si,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE}", "    rep movsb",
             f"    mov si,{DS_AREA[0]:#x}", f"    mov cx,{DS_AREA[1]:#x}", "    rep movsb",
             "    sti", "    mov bx,word ptr [handle]", f"    mov dx,{OUTPUT:#x}", f"    mov cx,{RECORD_BYTES}",
             "    mov ah,0x40", "    int 0x21", "    jc failure", f"    cmp ax,{RECORD_BYTES}", "    jne failure",
             "    inc word ptr [index]", "    mov ax,word ptr [index]", f"    cmp ax,{len(cases_)}", "    jb again",
             "    mov bx,word ptr [handle]", "    mov ah,0x3e", "    int 0x21", "    mov ax,0x4c00", "    int 0x21",
             "failure:", "    mov ax,0x4c01", "    int 0x21",
             'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", "target_far: .word 0x0068, 0",
             "flags_table: .word " + ",".join(f"{f:#x}" for f in FLAGS),
             f".org {FIXTURES_AT - 0x100:#x}", "fixtures:"]
    for case in cases_:
        text.append(f"    .byte {case['count']},{case['seed']},{case['h2']},{case['h3']},{case['flags']},0")
    return "\n".join(text) + "\n"


def native_audit(code, function):
    directory = ROOT / "resources/oracle-runs/u3c6-0068-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    cases_ = native_cases()
    asm, obj = directory / "oracle.S", directory / "oracle.o"
    asm.write_text(assembly(cases_))
    subprocess.run(["as", "--32", str(asm), "-o", str(obj)], check=True)
    subprocess.run(["ld", "-m", "elf_i386", "-Ttext", "0x100", "--oformat", "binary", str(obj), "-o", str(directory / "main.bin")], check=True)
    assembled = (directory / "main.bin").read_bytes()
    check(0x100 + len(assembled) <= U3C6_AT, "harness reaches the unit 0x3C6 copy")
    image = bytearray(U3C6_AT + 0x00E3 - 0x100)
    image[:len(assembled)] = assembled
    image[U3C6_AT - 0x100:U3C6_AT - 0x100 + 0x00E3] = code[0:0x00E3]
    (directory / "ORACLE.COM").write_bytes(bytes(image))
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
                       env=environment, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=180)
    result = result_path.read_bytes()
    check(len(result) == 8 + len(cases_) * RECORD_BYTES, f"native output incomplete: {len(result)} bytes")
    segment, returned, count, _ = struct.unpack_from("<4H", result)
    check(count == len(cases_), "native header differs")
    for n, case in enumerate(cases_):
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        memory = bytearray([0xA5]) * SIZE
        base = physical(segment, 0x100)
        memory[base:base + len(image)] = image
        put16(memory, segment, 0x6716, GRID); put16(memory, segment, 0x6718, segment)
        for i in range(DS_AREA[1]):
            memory[physical(segment, DS_AREA[0] + i)] = (case["seed"] + 11 * i) & 0xFF
        memory[physical(segment, 0x5F2A)] = case["count"]
        memory[physical(segment, GRID + 2)] = case["h2"]; memory[physical(segment, GRID + 3)] = case["h3"]
        sp = STACK_TOP
        for value in (segment, returned):
            sp -= 2; put16(memory, segment, sp, value)
        cpu = Cpu(ENTRY_REGS["ax"], ENTRY_REGS["bx"], ENTRY_REGS["cx"], ENTRY_REGS["dx"], ENTRY_REGS["si"], ENTRY_REGS["di"],
                  ENTRY_REGS["bp"], sp, (segment + U3C6_AT // 16) & 0xFFFF, 0x0068, segment, segment, segment, FLAGS[case["flags"]])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        check(function(ram, SIZE, ctypes.byref(cpu), 1000) == 0, f"C status on native case {n}")
        actual = bytes(ram)
        for (name, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, name) == want, f"native case {n} {name}: C={getattr(cpu, name):04x}, DOS={want:04x}")
        offset = 28
        for start, length in ((FRAME_LOW, FRAME_SIZE), DS_AREA):
            at = physical(segment, start)
            if actual[at:at + length] != record[offset:offset + length]:
                diff = [i for i in range(length) if actual[at + i] != record[offset + i]]
                raise ValueError(f"native case {n} bytes at {start:#x}+{diff[:6]} differ")
            offset += length
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {len(cases_)}; all 14 CPU words, {FRAME_SIZE} frame bytes and "
          f"the {DS_AREA[1]:#x}-byte slot/target area; no mask")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}")
    return dict(cases=len(cases_), result_sha256=digest)


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
    target, code = source()
    report = dict(target=dict(sha256=target["sha"], bytes=len(target["body"]), instructions=target["insns"]))
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-u3c6-0068-") as temp:
            function = library(Path(temp))
            if args.unicorn:
                report["unicorn_cases"] = differential(target, code, function)
            if args.native:
                report["native"] = native_audit(code, function)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
