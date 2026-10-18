#!/usr/bin/env python3
"""Designed original-byte full-state sweep for the raw-CPU U7F +05E0 party count, plus a DOSBox-X oracle.

The differential loads unit 0x7F and compares the entire 1 MiB RAM and all 14 CPU words between the original under
Unicorn and pr_u7f_party_count_05e0, with no mask: no AF masking (OR leaves AF clear in both oracles). Party lists vary
in length, status bytes (around 0 and B3h), placement, entry flags (AF set included) and stack position; one node sits
so that its status byte is the routine's own count byte. Cyclic lists loop forever in the original and in the API (no
budget), so they are not run. Media stays private.
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

ENTRY, END, BODY_RETF = 0x05E0, 0x063A, 0x0639
CALLER_CS, PARTY_SEG, DS, SS, EDGE_SS = 0x8800, 0x5000, 0x1800, 0x2800, 0x2000
RET_CS, RET_IP = 0x9001, 0x7000
STATUSES = (0x00, 0x00, 0xB3, 0xB3, 0x01, 0xB2, 0xB4, 0xFF, 0x80, 0x33)


def check(condition, message):
    if not condition:
        raise ValueError(message)


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, "START.EXE provenance differs")
    check(len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA, "GAME.OVR provenance differs")
    unit = {u["stub_para"]: u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)}[0x7F]
    code = ovr[unit["ovr_offset"]:unit["ovr_offset"] + unit["code_size"]]
    starts = sorted(f["rel"] for f in unit["functions"])
    check(END == next(s for s in starts if s > ENTRY) and ENTRY not in unit["entries"], "U7F +05E0 bounds differ")
    body = code[ENTRY:END]
    decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    conditional = {i.address: (i.address + i.size, int(i.op_str, 16)) for i in decoded if i.mnemonic.startswith("j") and i.mnemonic != "jmp"}
    check(len(body) == 90 and len(decoded) == 30 and sum(i.size for i in decoded) == 90 and decoded[-1].address == BODY_RETF
          and decoded[-1].mnemonic == "retf" and not any("call" in i.mnemonic for i in decoded) and len(conditional) == 3,
          "U7F +05E0 extent differs")
    target = dict(body=body, decoded=decoded, conditional=conditional, sha=sha256(body).hexdigest(), file_offset=unit["ovr_offset"] + ENTRY)
    print(f"U7F +05E0 (90/30, call-free, RETF; near-called) GAME file {target['file_offset']:#x} sha {target['sha']}")
    return target, code


def library(directory):
    path = directory / "u7f05e0.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                    str(ROOT / "src/recovered/overlay_u7f_party_count_05e0.c"), "-o", str(path)], check=True, capture_output=True, text=True)
    function = ctypes.CDLL(str(path)).pr_u7f_party_count_05e0
    function.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu)]
    function.restype = ctypes.c_int
    return function


def node_offset(k):
    return 0x0100 + 0x200 * k


def build(code, rng, statuses, *, sp=0x8000, ss=SS, party=PARTY_SEG, flags=0x7202, background=None, alias_count=False, base=0):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    start = physical(CALLER_CS, base)
    memory[start:start + len(code)] = code
    nodes = [(party, node_offset(k)) for k in range(len(statuses))]
    if alias_count and nodes:                                        # the last node's status byte is the count byte [BP-6]
        bp = (sp - 2) & 0xFFFF
        nodes[-1] = (ss, (bp - 6 - 0x84) & 0xFFFF)
    for k, ((seg, off), status) in enumerate(zip(nodes, statuses)):
        if not (alias_count and k == len(nodes) - 1):
            memory[physical(seg, off + 0x84)] = status
        nxt = nodes[k + 1] if k + 1 < len(nodes) else (0, 0)
        put16(memory, seg, (off + 0x104) & 0xFFFF, nxt[1]); put16(memory, seg, (off + 0x106) & 0xFFFF, nxt[0])
    head = nodes[0] if nodes else (0, 0)
    put16(memory, DS, 0x5D96, head[1]); put16(memory, DS, 0x5D98, head[0])
    frame = physical(ss, sp)
    for i, value in enumerate([RET_IP, RET_CS]):                     # both views agree at a segment end
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
        put16(memory, 0, (frame + 2 * i) & (SIZE - 1), value)
    reg = tuple(rng.randrange(0x10000) for _ in range(7))
    high = ss >= 0xF000 or party >= 0xF000
    return memory, Cpu(*reg, sp, CALLER_CS, (base + ENTRY) & 0xFFFF, DS, rng.randrange(0x10000), ss, flags), high


def compare(label, memory, cpu, high, target, function, base=0, budget=200000):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory)); original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    origin = physical(CALLER_CS, base)
    body = (origin + ENTRY, origin + END)
    state = dict(previous=None, returning=False, done=False)
    visited, edges = set(), set()

    def hook(emulator, address, size, user):
        del size, user
        if state["returning"]:
            state["done"] = True
            return emulator.emu_stop()
        check(body[0] <= address < body[1], f"fetch outside the body at {address:#x}: {label}")
        here = address - origin
        visited.add(here)
        if state["previous"] in target["conditional"] and state["previous"] != here:
            edges.add((state["previous"], here))
        state["previous"] = here
        if here == BODY_RETF:
            state["returning"] = True
        return None

    def mirror(emulator, access, address, size, value, user):
        del access, user
        for byte in range(size):
            raw = address + byte; at = raw & (SIZE - 1)
            emulator.mem_write(at + SIZE if raw < SIZE else at, bytes([(value >> (8 * byte)) & 0xFF]))

    original.hook_add(UC_HOOK_CODE, hook)
    if high:
        original.hook_add(UC_HOOK_MEM_WRITE, mirror)
    original.emu_start(body[0], 2 * SIZE, count=budget)
    check(state["done"], f"original did not return: {label}")
    expected = bytes(original.mem_read(0, SIZE))
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    check(function(ram, SIZE, ctypes.byref(cpu)) == 0, f"C status: {label}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f"RAM mismatch {label}: {[(hex(d), actual[d], expected[d]) for d in differences[:6]]} ({len(differences)} bytes)")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        mine, want = getattr(cpu, name), original.reg_read(register) & 0xFFFF
        check(mine == want, f"{name} mismatch {label}: C={mine:04x}, original={want:04x}")
    return visited, edges, cpu.ax & 0xFF


def cases(rng):
    spec = []
    for i in range(240):
        statuses = [rng.choice(STATUSES) for _ in range(rng.randrange(0, 13))]
        spec.append((f"random-{i}", dict(statuses=statuses, flags=(0x7202, 0x0202, 0x0ED7, 0x7213, 0x0013, 0x0647)[i % 6],
                                          sp=rng.randrange(0x0400, 0xF000) & ~1)))
    spec.append(("nil-head-af-set", dict(statuses=[], flags=0x0ED7)))
    spec.append(("all-counted", dict(statuses=[0, 0xB3] * 6, flags=0x0013)))
    spec.append(("none-counted", dict(statuses=[1, 0xB2, 0xB4, 0xFF] * 3)))
    spec.append(("255-plus", dict(statuses=[0] * 12)))
    for k, status in enumerate((0, 0xB3, 1)):
        spec.append((f"count-byte-node-{k}", dict(statuses=[status, 0, 0xB3, 5], alias_count=True)))
    for sp in (0x0010, 0x0040, 0xFFF0, 0xFFFC):
        spec.append((f"sp-{sp:04x}", dict(statuses=[0, 0xB3, 7], sp=sp, ss=EDGE_SS)))
    spec.append(("ss-a20", dict(statuses=[0, 0xB3, 7], sp=0x0400, ss=0xFFFF)))
    spec.append(("party-a20", dict(statuses=[0, 0xB3, 7, 0], party=0xFFF0)))
    spec.append(("party-segment-end", dict(statuses=[0, 0xB3], party=0x4F80)))
    spec.append(("random-background", dict(statuses=[0, 1, 0xB3, 0], background=bytes(random.Random(0x7F05E0).randbytes(SIZE)))))
    spec.append(("relocated-entry", dict(statuses=[0, 0xB3, 2], base=0x1000)))
    return spec


def differential(target, code, function):
    rng = random.Random(0x7F05E0)
    visited, edges, counts = set(), set(), {}
    total = 0
    for label, options in cases(rng):
        base = options.get("base", 0)
        memory, cpu, high = build(code, rng, options.pop("statuses"), **options)
        v, e, count = compare(f"+05E0 {label}", memory, cpu, high, target, function, base)
        visited |= v; edges |= e; total += 1
        counts[count] = counts.get(count, 0) + 1
    missing = sorted(hex(i.address) for i in target["decoded"] if i.address not in visited)
    reachable = {(s, o) for s, outs in target["conditional"].items() for o in outs}
    missing_edges = sorted((hex(s), hex(o)) for s, o in reachable - edges)
    check(not missing and not missing_edges, f"coverage incomplete: {missing} {missing_edges}")
    print(f"U7F +05E0: {total} original-byte cases passed (counts {dict(sorted(counts.items()))}); full 1 MiB RAM and all 14 CPU words equal, "
          f"no mask; {len(visited)}/{len(target['decoded'])} instruction starts, {len(edges)}/{len(reachable)} conditional outcomes")
    return dict(returning=total, starts=len(visited), outcomes=len(edges))


# ---- DOSBox-X synthetic oracle -------------------------------------------------------------------------
CALLER_N, PARTY_N, DATA_DS = 0x8800, 0x2000, 0x1900
MAIN_AT, CASES_AT, CODE_AT = 0x0300, 0x0E00, 0x4100
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xC800, 0xF000
FRAME_LOW, FRAME_SIZE = 0xEFC0, 0x40
MAX_NODES = 12
CASE_SIZE = 2 + MAX_NODES
RECORD_BYTES = 28 + FRAME_SIZE
NATIVE_FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0412, 0x0893, 0x0013)


def native_cases():
    rng = random.Random(0x7F05E0D)
    cases = [dict(statuses=[rng.choice(STATUSES) for _ in range(rng.randrange(0, MAX_NODES + 1))], flags=n % len(NATIVE_FLAGS)) for n in range(120)]
    cases[0]["statuses"] = []
    return cases


def native_audit(target, code, function):
    del target
    directory = ROOT / "resources/oracle-runs/u7f-05e0-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    cases = native_cases()
    check(CASES_AT + CASE_SIZE * len(cases) <= CODE_AT and CODE_AT + len(code) <= OUTPUT, "COM layout overlaps")
    text = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main", f".org {MAIN_AT - 0x100:#x}", "main:",
            "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}", "    cld",
            f"    mov ax,{CALLER_N:#x}", "    mov es,ax", "    mov si,offset unit", "    xor di,di", f"    mov cx,{len(code):#x}", "    rep movsb",
            "    push cs", "    pop es",
            f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
            f"    mov word ptr [{HEADER + 4:#x}],{len(cases)}", f"    mov word ptr [{HEADER + 6:#x}],0",
            "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
            "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8", "    mov ah,0x40", "    int 0x21", "    jc failure",
            "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    push cs", "    pop ds", "    push cs", "    pop es",
            "    mov al,0xa5", f"    mov di,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE}", "    rep stosb",
            "    mov ax,word ptr [index]", f"    mov bx,{CASE_SIZE}", "    mul bx", "    add ax,offset cases", "    mov si,ax",
            # n = [si], then MAX_NODES status bytes; node k at PARTY_N:0100 + 200h k, linked in order, the last nil
            f"    mov ax,{PARTY_N:#x}", "    mov es,ax", "    xor cx,cx", "    mov cl,byte ptr [si]", "    xor bx,bx", "    mov di,0x0100",
            "link_loop:", "    cmp bx,cx", "    jae link_done", "    mov al,byte ptr [si+bx+2]", "    mov byte ptr es:[di+0x84],al",
            "    lea ax,[di+0x200]", "    mov dx,es", "    inc bx", "    cmp bx,cx", "    jb link_next", "    xor ax,ax", "    xor dx,dx",
            "link_next:", "    mov word ptr es:[di+0x104],ax", "    mov word ptr es:[di+0x106],dx", "    add di,0x200", "    jmp link_loop",
            "link_done:", f"    mov ax,{DATA_DS:#x}", "    mov es,ax", "    xor ax,ax", "    xor dx,dx", "    or cx,cx", "    jz head_nil",
            "    mov ax,0x0100", f"    mov dx,{PARTY_N:#x}", "head_nil:", "    mov word ptr es:[0x5d96],ax", "    mov word ptr es:[0x5d98],dx",
            "    push cs", "    pop es",
            "    mov bl,byte ptr [si+1]", "    xor bh,bh", "    shl bx,1", "    push word ptr [flags_table+bx]",
            f"    mov ax,{DATA_DS:#x}", "    mov ds,ax", "    popf",
            "    mov ax,0x1234", "    mov bx,0x3456", "    mov cx,0x5678", "    mov dx,0x789a", "    mov si,0x9abc", "    mov di,0xbcde", "    mov bp,0x7777",
            "    call dword ptr cs:[target_far]", "returned:"]
    for i, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        text.append(f"    mov word ptr cs:[{OUTPUT + 2 * i:#x}],{reg}")
    text += [f"    mov word ptr cs:[{OUTPUT + 18:#x}],offset returned", f"    mov word ptr cs:[{OUTPUT + 20:#x}],ds",
             f"    mov word ptr cs:[{OUTPUT + 22:#x}],es", f"    mov word ptr cs:[{OUTPUT + 24:#x}],ss",
             f"    mov sp,{STACK_TOP + 0x800:#x}", "    pushf", "    pop ax", f"    mov word ptr cs:[{OUTPUT + 26:#x}],ax",
             "    cld", "    push cs", "    pop ds", "    push cs", "    pop es", f"    mov di,{OUTPUT + 28:#x}",
             f"    mov si,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE}", "    rep movsb", "    sti",
             "    mov bx,word ptr [handle]", f"    mov dx,{OUTPUT:#x}", f"    mov cx,{RECORD_BYTES}", "    mov ah,0x40", "    int 0x21", "    jc failure",
             f"    cmp ax,{RECORD_BYTES}", "    jne failure",
             "    inc word ptr [index]", "    mov ax,word ptr [index]", f"    cmp ax,{len(cases)}", "    jb again",
             "    mov bx,word ptr [handle]", "    mov ah,0x3e", "    int 0x21", "    mov ax,0x4c00", "    int 0x21",
             "failure:", "    mov ax,0x4c01", "    int 0x21",
             'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", f"target_far: .word {ENTRY:#x}, {CALLER_N:#x}",
             "flags_table: .word " + ",".join(f"{f:#x}" for f in NATIVE_FLAGS)]
    text += [f".org {CASES_AT - 0x100:#x}", "cases:"]
    for case in cases:
        row = [len(case["statuses"]), case["flags"]] + case["statuses"] + [0] * (MAX_NODES - len(case["statuses"]))
        text.append("    .byte " + ",".join(f"{b:#x}" for b in row))
    text += [f".org {CODE_AT - 0x100:#x}", "unit:"]
    for i in range(0, len(code), 16):
        text.append("    .byte " + ",".join(f"{b:#x}" for b in code[i:i + 16]))
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
    check(count == len(cases) and segment == 0x0814, f"native header differs or COM segment {segment:04x} is not 0814")
    af_cleared = 0
    for n, case in enumerate(cases):
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        memory = bytearray(SIZE)
        at = physical(CALLER_N, 0); memory[at:at + len(code)] = code
        nodes = [node_offset(k) for k in range(len(case["statuses"]))]
        for k, (off, status) in enumerate(zip(nodes, case["statuses"])):
            memory[physical(PARTY_N, off + 0x84)] = status
            nxt = nodes[k + 1] if k + 1 < len(nodes) else 0
            put16(memory, PARTY_N, off + 0x104, nxt); put16(memory, PARTY_N, off + 0x106, PARTY_N if nxt else 0)
        put16(memory, DATA_DS, 0x5D96, nodes[0] if nodes else 0); put16(memory, DATA_DS, 0x5D98, PARTY_N if nodes else 0)
        for i in range(FRAME_SIZE):
            memory[physical(segment, FRAME_LOW + i)] = 0xA5
        sp = STACK_TOP
        for value in (segment, returned):
            sp -= 2; put16(memory, segment, sp, value)
        cpu = Cpu(0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, sp, CALLER_N, ENTRY, DATA_DS, segment, segment,
                  NATIVE_FLAGS[case["flags"]])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        check(function(ram, SIZE, ctypes.byref(cpu)) == 0, f"C status on native case {n}")
        for (name, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, name) == want, f"native case {n} {name}: C={getattr(cpu, name):04x}, DOS={want:04x}")
        got = bytes(ram[physical(segment, FRAME_LOW):physical(segment, FRAME_LOW) + FRAME_SIZE])
        check(got == record[28:], f"native case {n} bytes differ")
        af_cleared += bool(NATIVE_FLAGS[case["flags"]] & 0x10)
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {len(cases)} ({af_cleared} entered with AF set, all returned with AF clear); "
          f"all 14 CPU words and the {FRAME_SIZE}-byte stack window; no mask")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}")
    return dict(cases=len(cases), result_sha256=digest)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unicorn", action="store_true")
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    target, code = source()
    report = dict(target=dict(sha256=target["sha"], bytes=90, instructions=30, file_offset=target["file_offset"]))
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-u7f05e0-") as temp:
            function = library(Path(temp))
            if args.unicorn:
                report["unicorn"] = differential(target, code, function)
            if args.native:
                report["native"] = native_audit(target, code, function)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
