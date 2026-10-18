#!/usr/bin/env python3
"""Original-byte full-state parity for the raw-CPU SYSTEM FillChar, Move, set IN and UpCase leaves, plus a DOSBox-X oracle.

Each leaf's bytes come from the hash-checked expanded START.EXE image. The Unicorn differential
places the whole image at load segment 0 and runs the original at 0AF8:<entry> against the C
model, stopping at the first fetch after its RETF wherever that goes, and compares the entire
1 MiB RAM and all 14 CPU words with no mask. The DOSBox-X oracle runs the original bytes at
9000:<entry>. Media stays private.
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
from resident_loader_map import unpack
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, REGISTERS, physical, put16

SYSTEM_CS = 0x0AF8
LEAVES = {
    "FillChar": dict(start=0x16A8, end=0x16BC, count=7, conditionals=0, retf="8", function="pr_system_fillchar_16a8",
                     sha="f1c5fade96970aa1dd1a9ffc2d876c7aea57bef8680d929d9f65d6c5190e17f7"),
    "Move": dict(start=0x0A3F, end=0x0A57, count=9, conditionals=0, retf="0xa", function="pr_system_move_0a3f",
                 sha="5bf8af52bdcd4af1b7e73b160b3ee02e6de7812b7e686aae02f73d9ccbc061aa"),
    "set IN": dict(start=0x0DC0, end=0x0DE1, count=13, conditionals=0, retf="6", function="pr_system_set_in_0dc0",
                   sha="753159dded0f3f45bb0352cb9483a15271d85888b6c1b127f8631879473ec62c"),
    "UpCase": dict(start=0x182A, end=0x183D, count=8, conditionals=2, retf="2", function="pr_system_upcase_182a",
                   sha="738c9a82c7faab6335f0952cc5d989d273cb85828a0735f34966220f370d012c"),
}
SS, EDGE_SS = 0x2800, 0x1C00
RET_CS, RET_IP = 0x9001, 0x7000


def check(condition, message):
    if not condition:
        raise ValueError(message)


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, "START.EXE provenance differs")
    image = bytes(unpack(exe)[0])
    targets = {}
    for name, leaf in LEAVES.items():
        body = image[SYSTEM_CS * 16 + leaf["start"]:SYSTEM_CS * 16 + leaf["end"]]
        decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, leaf["start"]))
        conditional = {i.address: (i.address + i.size, int(i.op_str, 16)) for i in decoded if i.mnemonic.startswith("j") and i.mnemonic != "jmp"}
        check(sha256(body).hexdigest() == leaf["sha"] and len(decoded) == leaf["count"] and sum(i.size for i in decoded) == len(body)
              and decoded[-1].mnemonic == "retf" and decoded[-1].op_str == leaf["retf"] and len(conditional) == leaf["conditionals"]
              and exe.count(body) == 1, f"SYSTEM {name} extent differs")
        targets[name] = dict(leaf, name=name, body=body, decoded=decoded, conditional=conditional, file_offset=exe.find(body))
        print(f"SYSTEM {name} 0AF8:{leaf['start']:04X} ({len(body)}/{len(decoded)}, RETF {leaf['retf']}) file {exe.find(body):#x} sha {leaf['sha'][:16]}...")
    return targets, image


def library(directory):
    path = directory / "leaves.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                    str(ROOT / "src/recovered/system_runtime_leaves_cpu.c"), "-o", str(path)], check=True, capture_output=True, text=True)
    lib = ctypes.CDLL(str(path))
    functions = {}
    for name, leaf in LEAVES.items():
        function = getattr(lib, leaf["function"])
        function.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu)]
        function.restype = ctypes.c_int
        functions[name] = function
    return functions


def build(image, rng, leaf, words, *, sp=0x8000, ss=SS, flags=0x7202, background=None, data=()):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    memory[0:len(image)] = image
    for seg, off, blob in data:
        for i, value in enumerate(blob):
            memory[physical(seg, (off + i) & 0xFFFF)] = value
    frame = physical(ss, sp)
    for i, value in enumerate([RET_IP, RET_CS] + list(words)):   # both views agree at a segment end
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
        put16(memory, 0, (frame + 2 * i) & (SIZE - 1), value)
    reg = tuple(rng.randrange(0x10000) for _ in range(7))
    cpu = Cpu(*reg, sp, SYSTEM_CS, leaf["start"], rng.randrange(0x10000), rng.randrange(0x10000), ss, flags)
    return memory, cpu


def compare(label, memory, cpu, target, function, high, budget=300000):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory))
    original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    start, end = physical(SYSTEM_CS, target["start"]), physical(SYSTEM_CS, target["end"])
    state = dict(previous=None, done=False)
    visited, edges = set(), set()

    def hook(emulator, address, size, user):
        del size, user
        if start <= address < end:
            here = address - start + target["start"]
            visited.add(here)
            if state["previous"] in target["conditional"] and state["previous"] != here:
                edges.add((state["previous"], here))
            state["previous"] = here
        else:
            state["done"] = True
            emulator.emu_stop()

    def mirror(emulator, access, address, size, value, user):
        del access, user
        for byte in range(size):
            raw = address + byte
            at = raw & (SIZE - 1)
            emulator.mem_write(at + SIZE if raw < SIZE else at, bytes([(value >> (8 * byte)) & 0xFF]))

    original.hook_add(UC_HOOK_CODE, hook)
    if high:
        original.hook_add(UC_HOOK_MEM_WRITE, mirror)
    original.emu_start(start, 2 * SIZE, count=budget)
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
    return visited, edges


FLAG_SET = (0x7202, 0x0602, 0x0ED7, 0x0647, 0xF2D7, 0x0002)


def cases(rng):
    """(leaf, label, words, options, high) in push order: words[0] is [SP+4]."""
    spec = []
    rand_seg = lambda: 0x4000 + rng.randrange(0x3000)
    # FillChar: [SP+4] value, [SP+6] count, [SP+8]/[SP+0Ah] X
    for i, count in enumerate((0, 1, 2, 3, 15, 16, 255, 256, 0x1234, 0xFFFF)):
        spec.append(("FillChar", f"count-{count:04x}", [rng.randrange(256), count, rng.randrange(0x10000), rand_seg()], dict(flags=FLAG_SET[i % 6])))
    for i in range(40):
        spec.append(("FillChar", f"random-{i}", [rng.randrange(0x10000), rng.randrange(0x400), rng.randrange(0x10000), rand_seg()],
                     dict(flags=FLAG_SET[i % 6], sp=rng.randrange(0x0100, 0xFF00) & ~1)))
    spec.append(("FillChar", "wrap", [0x3C, 0x20, 0xFFF0, 0x5000], {}))
    spec.append(("FillChar", "a20", [0x3C, 0x100, 0x0000, 0xFFFF], dict(high=True)))
    for delta in (-8, -2, 0, 4, 9):                                   # fill over its own frame: RETF goes elsewhere
        spec.append(("FillChar", f"frame{delta:+d}", [0x0B, 0x20, (0x8000 + delta) & 0xFFFF, SS], {}))
    for sp in (0x0004, 0xFFF4, 0xFFF6, 0xFFF8, 0xFFFE):
        spec.append(("FillChar", f"sp-{sp:04x}", [0x5A, 0x40, 0x0100, 0x5000], dict(sp=sp, ss=EDGE_SS)))
    spec.append(("FillChar", "ss-a20", [0x5A, 0x40, 0x0100, 0x5000], dict(sp=0x0400, ss=0xFFFF, high=True)))
    # Move: [SP+4] count, [SP+6]/[SP+8] Dest, [SP+0Ah]/[SP+0Ch] Source
    def move(label, count, src, dst, **options):
        data = [(src[0], src[1], bytes(rng.randrange(256) for _ in range(min(count, 0x2000) + 8)))]
        spec.append(("Move", label, [count, dst[1], dst[0], src[1], src[0]], dict(options, data=data)))
    for i, count in enumerate((0, 1, 2, 7, 255, 256, 0x1000, 0xFFFF)):
        move(f"count-{count:04x}", count, (rand_seg(), rng.randrange(0x10000)), (rand_seg(), rng.randrange(0x10000)), flags=FLAG_SET[i % 6])
    for d in (1, 2, 7, 0x10, -1, -2, -7, -0x10, 0):
        move(f"overlap{d:+d}", 0x40, (0x5000, 0x0400), (0x5000, (0x0400 + d) & 0xFFFF))
    for i in range(30):
        move(f"random-{i}", rng.randrange(0x300), (rand_seg(), rng.randrange(0x10000)), (rand_seg(), rng.randrange(0x10000)),
             flags=FLAG_SET[i % 6], sp=rng.randrange(0x0100, 0xFF00) & ~1)
    move("wrap", 0x20, (0x5000, 0xFFF0), (0x6000, 0xFFF8))
    move("src-a20", 0x20, (0xFFFF, 0x0100), (0x6000, 0x0100), high=True)
    move("dst-a20", 0x20, (0x6000, 0x0100), (0xFFFF, 0x0100), high=True)
    move("dst-frame", 0x20, (0x6000, 0x0100), (SS, 0x7FF8))           # RETF goes elsewhere
    move("src-frame", 0x20, (SS, 0x7FF8), (0x6000, 0x0100))
    for sp in (0x0004, 0xFFF2, 0xFFF6, 0xFFFE):
        move(f"sp-{sp:04x}", 0x10, (0x5000, 0x0100), (0x6000, 0x0200), sp=sp, ss=EDGE_SS)
    # set IN: [SP+4]/[SP+6] the set, [SP+8] the element
    for e in range(256):
        set_seg, set_off = rand_seg(), rng.randrange(0x10000)
        data = [(set_seg, set_off, bytes(rng.randrange(256) for _ in range(32)))]
        spec.append(("set IN", f"element-{e:02x}", [set_off, set_seg, e | (rng.randrange(256) << 8)], dict(data=data, flags=FLAG_SET[e % 6])))
    spec.append(("set IN", "wrap", [0xFFF0, 0x5000, 0xFF], dict(data=[(0x5000, 0xFFF0, bytes(range(0x80, 0xA0)))])))
    spec.append(("set IN", "a20", [0x0100, 0xFFFF, 0x77], dict(data=[(0xFFFF, 0x0100, bytes(range(32)))], high=True)))
    for sp in (0x0004, 0xFFF8, 0xFFFE):
        spec.append(("set IN", f"sp-{sp:04x}", [0x0100, 0x5000, 0x2A], dict(sp=sp, ss=EDGE_SS, data=[(0x5000, 0x0100, bytes(range(32)))])))
    # UpCase: [SP+4] the character
    for c in range(256):
        for k in range(2):
            spec.append(("UpCase", f"char-{c:02x}-{k}", [c | (rng.randrange(256) << 8)], dict(flags=FLAG_SET[(c + k) % 6])))
    for sp in (0x0004, 0xFFFC, 0xFFFE):
        spec.append(("UpCase", f"sp-{sp:04x}", [0x61], dict(sp=sp, ss=EDGE_SS)))
    spec.append(("UpCase", "ss-a20", [0x7A], dict(sp=0x0400, ss=0xFFFF, high=True)))
    spec.append(("UpCase", "random-background", [0x6D], dict(background=bytes(random.Random(0x182A).randbytes(SIZE)))))
    return spec


def differential(targets, image, functions):
    rng = random.Random(0x5A57)
    report = {}
    coverage = {name: (set(), set()) for name in targets}
    counts = {name: 0 for name in targets}
    for name, label, words, options in cases(rng):
        high = options.pop("high", False)
        memory, cpu = build(image, rng, targets[name], words, **options)
        v, e = compare(f"{name} {label}", memory, cpu, targets[name], functions[name], high)
        coverage[name][0].update(v)
        coverage[name][1].update(e)
        counts[name] += 1
    for name, target in targets.items():
        visited, edges = coverage[name]
        missing = sorted(hex(i.address) for i in target["decoded"] if i.address not in visited)
        missing_edges = sorted((hex(s), hex(o)) for s, outs in target["conditional"].items() for o in outs if (s, o) not in edges)
        check(not missing and not missing_edges, f"{name} coverage incomplete: {missing} {missing_edges}")
        print(f"{name}: {counts[name]} original-byte cases passed, full 1 MiB RAM and all 14 CPU words equal; "
              f"{len(visited)}/{len(target['decoded'])} instruction starts, {len(edges)}/{2 * len(target['conditional'])} conditional outcomes")
        report[name] = dict(cases=counts[name], starts=len(visited), outcomes=len(edges))
    return report


# ---- DOSBox-X synthetic oracle -------------------------------------------------------------------------
MAIN_AT, BLOB_AT, CASES_AT = 0x0300, 0x0F00, 0x1000
LEAF_N, DATA_SEG, DATA_DS = 0x9000, 0x2000, 0x1900
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xC800, 0xF000
FRAME_LOW, FRAME_SIZE, WINDOW = 0xEFC0, 0x40, 0x400
CASE_SIZE = 16
RECORD_BYTES = 28 + FRAME_SIZE + WINDOW
NATIVE_FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)
ORDER = ("FillChar", "Move", "set IN", "UpCase")


def native_cases():
    rng = random.Random(0x5A57D05)
    rows = []
    for i in range(48):
        count = (0, 1, 2, 3, 16, 255, 256, 0x3FF)[i] if i < 8 else rng.randrange(0x400)
        rows.append(("FillChar", [rng.randrange(0x10000), count, rng.randrange(0, WINDOW - count + 1), DATA_SEG]))
    for i in range(48):
        count = (0, 1, 2, 7, 0x40, 0x100, 0x1FF, 0x80)[i] if i < 8 else rng.randrange(0x200)
        src = rng.randrange(0, WINDOW - count + 1)
        dst = src + rng.choice((1, 2, 7, -1, -2, -7, 0)) if i % 3 == 0 else rng.randrange(0, WINDOW - count + 1)
        dst = min(max(dst, 0), WINDOW - count)
        rows.append(("Move", [count, dst, DATA_SEG, src, DATA_SEG]))
    for i in range(64):
        rows.append(("set IN", [rng.randrange(0, WINDOW - 32), DATA_SEG, (i * 37 + rng.randrange(8)) & 0xFF | (rng.randrange(256) << 8)]))
    for c in range(256):
        rows.append(("UpCase", [c | (rng.randrange(256) << 8)]))
    return [dict(leaf=leaf, words=words, flags=n % len(NATIVE_FLAGS), fill=rng.randrange(256)) for n, (leaf, words) in enumerate(rows)]


def native_audit(targets, functions):
    directory = ROOT / "resources/oracle-runs/system-runtime-leaves-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    cases = native_cases()
    blobs, at = [], BLOB_AT
    for name in ORDER:
        blobs.append((name, at, targets[name]["body"]))
        at += len(targets[name]["body"])
    text = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main", f".org {MAIN_AT - 0x100:#x}", "main:",
            "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}", "    cld",
            f"    mov ax,{LEAF_N:#x}", "    mov es,ax"]
    for name, blob_at, body in blobs:
        text += [f"    mov si,{blob_at:#x}", f"    mov di,{targets[name]['start']:#x}", f"    mov cx,{len(body)}", "    rep movsb"]
    text += ["    mov ax,cs", "    mov es,ax",
             f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
             f"    mov word ptr [{HEADER + 4:#x}],{len(cases)}", f"    mov word ptr [{HEADER + 6:#x}],0",
             "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
             "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8",
             "    mov ah,0x40", "    int 0x21", "    jc failure",
             "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    mov ax,cs", "    mov ds,ax", "    mov es,ax",
             "    mov al,0xa5", f"    mov di,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE}", "    rep stosb",
             "    mov ax,word ptr [index]", f"    mov bx,{CASE_SIZE}", "    mul bx", "    add ax,offset cases", "    mov si,ax",
             # data window: byte i = fill + 7i
             f"    mov ax,{DATA_SEG:#x}", "    mov es,ax", "    xor di,di", "    mov al,byte ptr [si+3]", f"    mov cx,{WINDOW}",
             "fill_window:", "    stosb", "    add al,7", "    loop fill_window",
             "    mov ax,cs", "    mov es,ax",
             # push words[0..n-1], last first so words[0] ends at [SP+4]
             "    mov cl,byte ptr [si+2]", "    xor ch,ch", "    mov bx,cx", "    shl bx,1", "    add bx,si", "    add bx,4",
             "push_words:", "    jcxz pushed", "    sub bx,2", "    push word ptr [bx]", "    dec cx", "    jmp push_words", "pushed:",
             "    mov bl,byte ptr [si]", "    xor bh,bh", "    shl bx,1", "    mov ax,word ptr [entries+bx]", "    mov word ptr [target_far],ax",
             "    mov bl,byte ptr [si+1]", "    xor bh,bh", "    shl bx,1", "    push word ptr [flags_table+bx]", "    popf",
             f"    mov ax,{DATA_DS:#x}", "    mov ds,ax", f"    mov ax,{DATA_SEG:#x}", "    mov es,ax"]
    for name, value in ENTRY_REGS.items():
        text.append(f"    mov {name},{value:#x}")
    text += ["    call dword ptr cs:[target_far]", "returned:"]
    for i, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        text.append(f"    mov word ptr cs:[{OUTPUT + 2 * i:#x}],{reg}")
    text += [f"    mov word ptr cs:[{OUTPUT + 18:#x}],offset returned", f"    mov word ptr cs:[{OUTPUT + 20:#x}],ds",
             f"    mov word ptr cs:[{OUTPUT + 22:#x}],es", f"    mov word ptr cs:[{OUTPUT + 24:#x}],ss",
             f"    mov sp,{STACK_TOP + 0x800:#x}", "    pushf", "    pop ax", f"    mov word ptr cs:[{OUTPUT + 26:#x}],ax",
             "    cld", "    mov ax,cs", "    mov es,ax", "    mov ds,ax", f"    mov di,{OUTPUT + 28:#x}",
             f"    mov si,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE}", "    rep movsb",
             f"    mov ax,{DATA_SEG:#x}", "    mov ds,ax", "    xor si,si", f"    mov cx,{WINDOW}", "    rep movsb",
             "    mov ax,cs", "    mov ds,ax", "    sti",
             "    mov bx,word ptr [handle]", f"    mov dx,{OUTPUT:#x}", f"    mov cx,{RECORD_BYTES}", "    mov ah,0x40", "    int 0x21",
             "    jc failure", f"    cmp ax,{RECORD_BYTES}", "    jne failure",
             "    inc word ptr [index]", "    mov ax,word ptr [index]", f"    cmp ax,{len(cases)}", "    jb again",
             "    mov bx,word ptr [handle]", "    mov ah,0x3e", "    int 0x21", "    mov ax,0x4c00", "    int 0x21",
             "failure:", "    mov ax,0x4c01", "    int 0x21",
             'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", f"target_far: .word 0, {LEAF_N:#x}",
             "entries: .word " + ",".join(f"{targets[name]['start']:#x}" for name in ORDER),
             "flags_table: .word " + ",".join(f"{f:#x}" for f in NATIVE_FLAGS)]
    for name, blob_at, body in blobs:
        text += [f".org {blob_at - 0x100:#x}", "    .byte " + ",".join(f"{b:#x}" for b in body)]
    text += [f".org {CASES_AT - 0x100:#x}", "cases:"]
    for case in cases:
        words = case["words"] + [0] * (5 - len(case["words"]))
        row = struct.pack("<BBBB5HH", ORDER.index(case["leaf"]), case["flags"], len(case["words"]), case["fill"], *words, 0)
        check(len(row) == CASE_SIZE, "case row size differs")
        text.append("    .byte " + ",".join(f"{b:#x}" for b in row))
    check(CASES_AT + CASE_SIZE * len(cases) <= OUTPUT and at <= CASES_AT, "COM layout overlaps")
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
    per_leaf = {name: 0 for name in ORDER}
    for n, case in enumerate(cases):
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        memory = bytearray(SIZE)
        for name, _, body in blobs:
            at = physical(LEAF_N, targets[name]["start"])
            memory[at:at + len(body)] = body
        for i in range(FRAME_SIZE):
            memory[physical(segment, FRAME_LOW + i)] = 0xA5
        for i in range(WINDOW):
            memory[physical(DATA_SEG, i)] = (case["fill"] + 7 * i) & 0xFF
        sp = STACK_TOP
        for value in list(reversed(case["words"])) + [segment, returned]:
            sp -= 2
            put16(memory, segment, sp, value)
        target = targets[case["leaf"]]
        cpu = Cpu(*ENTRY_REGS.values(), sp, LEAF_N, target["start"], DATA_DS, DATA_SEG, segment, NATIVE_FLAGS[case["flags"]])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        label = f"native case {n} ({case['leaf']}, words {[hex(w) for w in case['words']]})"
        check(functions[case["leaf"]](ram, SIZE, ctypes.byref(cpu)) == 0, f"C status on {label}")
        for (name, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, name) == want, f"{label} {name}: C={getattr(cpu, name):04x}, DOS={want:04x}")
        got = bytes(ram[physical(segment, FRAME_LOW):physical(segment, FRAME_LOW) + FRAME_SIZE])
        got += bytes(ram[physical(DATA_SEG, 0):physical(DATA_SEG, 0) + WINDOW])
        check(got == record[28:], f"{label} bytes differ")
        per_leaf[case["leaf"]] += 1
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {len(cases)} ({per_leaf}); all 14 CPU words, the {FRAME_SIZE}-byte stack "
          f"window and the {WINDOW}-byte data window; no mask")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}; leaves copied to {LEAF_N:04X}:<entry>")
    return dict(cases=len(cases), per_leaf=per_leaf, result_sha256=digest)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unicorn", action="store_true")
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    targets, image = source()
    report = dict(targets={name: dict(sha256=t["sha"], bytes=len(t["body"]), instructions=len(t["decoded"]), file_offset=t["file_offset"])
                           for name, t in targets.items()})
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-leaves-") as temp:
            functions = library(Path(temp))
            if args.unicorn:
                report["unicorn"] = differential(targets, image, functions)
            if args.native:
                report["native"] = native_audit(targets, functions)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
