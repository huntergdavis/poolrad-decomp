#!/usr/bin/env python3
"""Hash-lock u6A +025C and compare original bytes with portable C."""
import argparse
import ctypes
from collections import deque
from hashlib import sha256
from pathlib import Path
import os
import random
import struct
import subprocess
import tempfile

from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from overlay_inventory import find_units, parse_mz
from u5e_party_value_2879_audit import Cpu, EXE_SHA, OVR_SHA, physical, put16

RAM_SIZE = 1 << 20
ENTRY, END = 0x025C, 0x0302
BODY_SHA = "949abe77fa2823648d99cd3b76412560837646604ed9526f5f575dd8d3118b14"


def check(condition, message):
    if not condition:
        raise ValueError(message)


def source_audit(root):
    exe = (root / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (root / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    check(sha256(exe).hexdigest() == EXE_SHA, "START.EXE provenance changed")
    check(sha256(ovr).hexdigest() == OVR_SHA, "GAME.OVR provenance changed")
    unit = next(u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)
                if u["stub_para"] == 0x6A)
    check(unit["ovr_offset"] == 0x17831 and unit["memory_segment"] == 0x75,
          "unit placement changed")
    code = ovr[unit["ovr_offset"]:unit["ovr_offset"] + unit["code_size"]]
    body = code[ENTRY:END]
    check(len(body) == 166 and sha256(body).hexdigest() == BODY_SHA,
          "u6A +025C body changed")
    decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(len(decoded) == 64 and decoded[-1].address == 0x02FF and
          decoded[-1].mnemonic == "retf" and decoded[-1].op_str == "4" and
          decoded[-1].address + decoded[-1].size == END, "extent changed")
    starts = {ins.address for ins in decoded}
    branches = []
    for ins in decoded:
        check("call" not in ins.mnemonic, "leaf gained a call")
        if ins.mnemonic.startswith("j"):
            check(int(ins.op_str, 16) in starts, "branch leaves body")
            branches.append(ins.address)
    check(branches == [0x026A, 0x027F, 0x02A1, 0x02C3, 0x02CC], "CFG changed")
    check(body[0x02D8 - ENTRY] == body[0x02DE - ENTRY] == 0x98,
          "16-bit CBW selector sign extension changed")
    for start, end, site in ((0x05A4, 0x0824, 0x05DA),
                             (0x090A, 0x0B00, 0x0A12)):
        caller = {ins.address: ins for ins in
                  Cs(CS_ARCH_X86, CS_MODE_16).disasm(code[start:end], start)}
        check(caller[site - 1].mnemonic == "push" and
              caller[site - 1].op_str == "cs" and
              caller[site].mnemonic == "call" and
              caller[site].op_str == "0x25c", "aligned caller changed")
    print(f"u6A +025C..+0301: 166 bytes, 64 instructions, 5 branches, "
          f"0 calls; SHA-256 {BODY_SHA}")
    print("aligned near callers +05DA (body +05A4), +0A12 (body +090A)")
    check(unit["entries"][1] == 0x1E6F, "public entry 1 changed")
    pending, seen, reached = deque([(0x1E6F, [])]), set(), None
    decoder = Cs(CS_ARCH_X86, CS_MODE_16)
    while pending:
        at, path = pending.popleft()
        if at in seen or not 0 <= at < len(code):
            continue
        seen.add(at)
        if at == ENTRY:
            reached = path
            break
        ins = next(decoder.disasm(code[at:at + 15], at, count=1), None)
        if ins is None or ins.mnemonic.startswith("ret") or ins.mnemonic in ("int", "iret"):
            continue
        if ins.mnemonic == "call":
            pending.append((int(ins.op_str, 16), path + [(at, int(ins.op_str, 16))]))
        if ins.mnemonic.startswith("j") or ins.mnemonic.startswith("loop"):
            try:
                pending.append((int(ins.op_str, 16), path))
            except ValueError:
                pass
            if ins.mnemonic == "jmp":
                continue
        pending.append((at + ins.size, path))
    check(reached == [(0x1F93, 0x13F1), (0x147A, 0x090A),
                      (0x0991, 0x05A4), (0x05DA, ENTRY)],
          "public-entry near-call path changed")
    print("public entry 1 +1E6F CFG path: +1F93->13F1, +147A->090A, "
          "+0991->05A4, +05DA->025C (static path, not native reach)")
    return body


def build_c(root, directory):
    path = directory / "u6a_025c.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                    "-Werror", "-shared", "-fPIC", "-I", str(root / "include"),
                    str(root / "src/recovered/overlay_u6a_record_remaining_025c.c"),
                    "-o", str(path)], check=True, capture_output=True, text=True)
    library = ctypes.CDLL(str(path))
    library.pr_u6a_record_remaining_025c.argtypes = [
        ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu)]
    library.pr_u6a_record_remaining_025c.restype = ctypes.c_int
    return library


def fixture(body, rng, low, high, *, ss=0x2000, sp=0x8000, ds=0x1000,
            segment=0x4000, offset=0x100, pattern="random", flags=0x7202):
    ram = bytearray([0xA5]) * RAM_SIZE
    cs = 0x3000
    ram[physical(cs, ENTRY):physical(cs, ENTRY) + len(body)] = body
    for i in range(128):
        ram[physical(ds, 0x31FA + 16 * i)] = low if rng.randrange(2) else rng.randrange(256)
        ram[physical(ds, 0x31FB + 16 * i)] = high if rng.randrange(2) else rng.randrange(256)
    for i in range(21):
        value = rng.randrange(256)
        if pattern == "zero": value = 0
        if pattern == "match": value = 0x81
        ram[physical(segment, (offset + 0x17 + i) & 0xFFFF)] = value
    if pattern == "match":
        ram[physical(ds, 0x320A)] = low
        ram[physical(ds, 0x320B)] = high
    signed = lambda value: value if value < 128 else value - 256
    field = (offset + 0xB1 + 3 * signed(low) + signed(high)) & 0xFFFF
    ram[physical(segment, field)] = rng.randrange(256)
    put16(ram, ds, 0x5D92, offset)
    put16(ram, ds, 0x5D94, segment)
    for displacement, value in ((0, 0x7000), (2, cs),
                                (4, 0x5A00 | low), (6, 0xC300 | high)):
        put16(ram, ss, (sp + displacement) & 0xFFFF, value)
    cpu = Cpu(0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE,
              0x7777, sp, cs, ENTRY, ds, 0x1111, ss, flags)
    return ram, cpu


def run_c(library, memory, cpu):
    ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
    check(library.pr_u6a_record_remaining_025c(ram, RAM_SIZE,
                                             ctypes.byref(cpu)) == 0,
          "C rejected valid fixture")
    return bytes(ram), cpu


def differential(body, library):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    from unicorn import x86_const as regs
    register_ids = [getattr(regs, "UC_X86_REG_" +
                    ("EFLAGS" if name == "flags" else name.upper()))
                    for name, _ in Cpu._fields_]
    rng = random.Random(0x6A025C)
    specifications = [(f"random-{i}", rng.randrange(256), rng.randrange(256), {})
                      for i in range(256)]
    specifications += [(f"{pattern}-{a}-{b}", a, b, {"pattern": pattern})
                       for pattern in ("zero", "match")
                       for a in (0, 1, 127, 128, 255)
                       for b in (0, 1, 127, 128, 255)]
    specifications += [(f"stack-{sp:04x}", 1, 2, {"sp": sp, "pattern": "match"})
                       for sp in (0, 1, 2, 3, 4, 5, 0xFFFA, 0xFFFB,
                                  0xFFFC, 0xFFFD, 0xFFFE, 0xFFFF)]
    specifications += [(f"pointer-frame-alias-{sp:04x}", 1, 2,
                        {"ss": 0x1000, "sp": sp})
                       for sp in range(0x5D90, 0x5D9C)]
    specifications += [
        ("array-local-alias", 1, 2, {"segment": 0x2000, "offset": 0x7FE0}),
        ("array-pointer-alias", 1, 2, {"segment": 0x1000, "offset": 0x5D7B}),
        ("array-table-alias", 1, 2, {"segment": 0x1000, "offset": 0x31E3}),
        ("array-offset-wrap", 255, 128, {"offset": 0xFFF0}),
        ("A20-stack", 1, 2, {"ss": 0xFFFF, "sp": 0x11}),
        ("A20-record", 1, 2, {"segment": 0xFFFF}),
        ("A20-DS", 1, 2, {"ds": 0xFFFF}),
        ("preserved-DF", 1, 2, {"flags": 0x0603}),
        ("incoming-arithmetic-flags", 1, 2, {"flags": 0x0ED7}),
    ]
    visited, edges = set(), set()
    for label, low, high, options in specifications:
        memory, cpu = fixture(body, rng, low, high, **options)
        original = Uc(UC_ARCH_X86, UC_MODE_16)
        original.mem_map(0, 2 * RAM_SIZE)
        original.mem_write(0, bytes(memory))
        original.mem_write(RAM_SIZE, bytes(memory))
        for (name, _), register in zip(Cpu._fields_, register_ids):
            original.reg_write(register, getattr(cpu, name))
        returned = [False]
        previous = [None]
        code_start = physical(cpu.cs, ENTRY)
        code_return = physical(cpu.cs, 0x02FF)

        def on_instruction(emulator, address, size, user):
            del size, user
            if returned[0]:
                emulator.emu_stop()
                return
            check(code_start <= address < code_start + len(body),
                  f"execution left body before RETF: {label}")
            visited.add(address - code_start + ENTRY)
            here = address - code_start + ENTRY
            if previous[0] in (0x027F, 0x02A1, 0x02C3, 0x02CC):
                edges.add((previous[0], here))
            previous[0] = here
            if address == code_return:
                returned[0] = True

        def mirror_a20(emulator, access, address, size, value, user):
            del access, user
            for byte in range(size):
                raw = address + byte
                at = raw & (RAM_SIZE - 1)
                mirror = at + RAM_SIZE if raw < RAM_SIZE else at
                emulator.mem_write(mirror, bytes([(value >> (8 * byte)) & 0xFF]))

        original.hook_add(UC_HOOK_CODE, on_instruction)
        original.hook_add(UC_HOOK_MEM_WRITE, mirror_a20)
        original.emu_start(code_start, 2 * RAM_SIZE, count=10000)
        check(returned[0], f"original did not return: {label}")
        expected = bytes(original.mem_read(0, RAM_SIZE))
        actual, cpu = run_c(library, memory, cpu)
        if actual != expected:
            differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
            raise ValueError(f"RAM mismatch {label}: {differences[:8]}")
        for (name, _), register in zip(Cpu._fields_, register_ids):
            expected_word = original.reg_read(register) & 0xFFFF
            check(getattr(cpu, name) == expected_word,
                  f"{name} mismatch {label}: C={getattr(cpu, name):04x}, "
                  f"original={expected_word:04x}")
    check(len(visited) == 64, f"incomplete instruction coverage: {len(visited)}")
    expected_edges = {(0x027F, 0x0281), (0x027F, 0x02C8),
                      (0x02A1, 0x02A3), (0x02A1, 0x02C8),
                      (0x02C3, 0x02C5), (0x02C3, 0x02C8),
                      (0x02CC, 0x026C), (0x02CC, 0x02CE)}
    check(expected_edges <= edges, "conditional branch outcome coverage incomplete")
    print(f"{len(specifications)} original-byte/C cases passed: entire 1 MiB "
          "RAM, all 14 CPU words, all 64 instruction starts and 8 conditional edges reached")


def native_assembly(source_path):
    assembly = r'''.code16
.intel_syntax noprefix
.global _start
_start:
    mov ax, cs
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0xf000
    mov word ptr [header], ax
    mov word ptr [header+2], offset returned
    mov word ptr [header+4], offset leaf
    mov word ptr [0x5d92], 0x4000
    mov word ptr [0x5d94], ax
    mov byte ptr [0x320a], 1
    mov byte ptr [0x320b], 2
    mov dx, offset filename
    xor cx, cx
    mov ah, 0x3c
    int 0x21
    jc failure
    mov word ptr [handle], ax
    mov bx, ax
    mov dx, offset header
    mov cx, 6
    mov ah, 0x40
    int 0x21
again:
    mov ax, word ptr [index]
    mov byte ptr [0x40b6], al
    xor dx, dx
    mov cx, 22
    div cx
    mov di, 0x4017
    mov cx, 21
    xor ax, ax
    cld
    rep stosb
    mov di, 0x4017
    mov cx, dx
    mov al, 0x81
    rep stosb
    mov sp, 0xf000
    push 0xc302
    push 0x5a01
    push 0x0202
    popf
    mov ax, 0x1234
    mov bx, 0x3456
    mov cx, 0x5678
    mov dx, 0x789a
    mov si, 0x9abc
    mov di, 0xbcde
    mov bp, 0x7777
    push cs
    call leaf
returned:
'''
    for index, (name, _) in enumerate(Cpu._fields_[:-1]):
        value = "offset returned" if name == "ip" else name
        assembly += f"    mov word ptr [output+{2 * index}], {value}\n"
    # Save all 13 defined frame bytes before PUSHF can reuse argument RAM.
    for index in range(6):
        assembly += (f"    mov ax, word ptr [{0xEFF3 + 2 * index}]\n"
                     f"    mov word ptr [output+{28 + 2 * index}], ax\n")
    assembly += r'''
    mov al, byte ptr [0xefff]
    mov byte ptr [output+40], al
    pushf
    pop ax
    mov word ptr [output+26], ax
    mov ax, word ptr [0x5d92]
    mov word ptr [output+41], ax
    mov ax, word ptr [0x5d94]
    mov word ptr [output+43], ax
    mov si, 0x4017
    mov di, offset output+45
    mov cx, 21
    cld
    rep movsb
    mov al, byte ptr [0x40b6]
    mov byte ptr [output+66], al
    mov ax, word ptr [index]
    mov byte ptr [output+67], al
    mov bx, word ptr [handle]
    mov dx, offset output
    mov cx, 68
    mov ah, 0x40
    int 0x21
    jc failure
    cmp ax, 68
    jne failure
    inc word ptr [index]
    cmp word ptr [index], 256
    jb again
    mov bx, word ptr [handle]
    mov ah, 0x3e
    int 0x21
    mov ax, 0x4c00
    int 0x21
failure:
    mov ax, 0x4c01
    int 0x21
filename: .asciz "RESULT.BIN"
header: .space 6
handle: .word 0
index: .word 0
output: .space 68
leaf:
'''
    return assembly + f'.incbin "{source_path.resolve()}"\n'


def native_audit(root, body, library):
    directory = root / "resources/oracle-runs/u6a-record-025c-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    source = directory / "leaf.bin"
    source.write_bytes(body)
    assembly, obj, com = directory / "oracle.S", directory / "oracle.o", directory / "ORACLE.COM"
    assembly.write_text(native_assembly(source))
    subprocess.run(["as", "--32", str(assembly), "-o", str(obj)], check=True)
    subprocess.run(["ld", "-m", "elf_i386", "-Ttext", "0x100", "--oformat",
                    "binary", str(obj), "-o", str(com)], check=True)
    config = directory / "dosbox.conf"
    config.write_text("[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n"
                      "[cpu]\ncore=normal\ncputype=386\ncycles=max\n"
                      "[mixer]\nnosound=true\n[midi]\nmididevice=none\n"
                      "[autoexec]\nmount c " + str(directory.resolve()) +
                      "\nc:\nORACLE.COM\nexit\n")
    result_path = directory / "RESULT.BIN"
    if result_path.exists():
        result_path.unlink()
    environment = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy")
    with (directory / "dosbox.log").open("w") as log:
        subprocess.run(["dosbox-x", "-conf", str(config), "-fastlaunch",
                        "-exit", "-noconsole"], env=environment, stdout=log,
                       stderr=subprocess.STDOUT, check=True, timeout=45)
    result = result_path.read_bytes()
    check(len(result) == 6 + 256 * 68, "incomplete native output")
    segment, return_ip, leaf_ip = struct.unpack_from("<HHH", result)
    check(com.read_bytes()[leaf_ip - 0x100:leaf_ip - 0x100 + len(body)] == body,
          "native COM source differs from hash-locked leaf")
    for index in range(256):
        record = result[6 + index * 68:6 + (index + 1) * 68]
        memory = bytearray([0xA5]) * RAM_SIZE
        cpu = Cpu(0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE,
                  0x7777, 0xEFF8, segment, leaf_ip, segment, segment, segment, 0x0202)
        for displacement, value in ((0, return_ip), (2, segment),
                                    (4, 0x5A01), (6, 0xC302)):
            put16(memory, segment, cpu.sp + displacement, value)
        put16(memory, segment, 0x5D92, 0x4000)
        put16(memory, segment, 0x5D94, segment)
        memory[physical(segment, 0x320A)] = 1
        memory[physical(segment, 0x320B)] = 2
        for element in range(21):
            memory[physical(segment, 0x4017 + element)] = 0x81 if element < index % 22 else 0
        memory[physical(segment, 0x40B6)] = index
        actual, cpu = run_c(library, memory, cpu)
        expected_words = struct.unpack_from("<14H", record)
        for (name, _), expected in zip(Cpu._fields_, expected_words):
            check(getattr(cpu, name) == expected,
                  f"native {name} mismatch at {index}: "
                  f"C={getattr(cpu, name):04x}, native={expected:04x}")
        at = physical(segment, 0xEFF3)
        check(actual[at:at + 13] == record[28:41], f"native frame mismatch {index}")
        at = physical(segment, 0x5D92)
        check(actual[at:at + 4] == record[41:45], f"native pointer mismatch {index}")
        at = physical(segment, 0x4017)
        check(actual[at:at + 21] == record[45:66], f"native list mismatch {index}")
        check(actual[physical(segment, 0x40B6)] == record[66] == record[67],
              f"native capacity mismatch {index}")
    print("256 DOSBox-X synthetic cases passed: all 14 CPU words, 13 frame "
          "bytes, pointer, list and capacity; no flag mask")
    print("native result SHA-256 " + sha256(result).hexdigest())


def main():
    root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unicorn", action="store_true")
    parser.add_argument("--native", action="store_true")
    args = parser.parse_args()
    body = source_audit(root)
    with tempfile.TemporaryDirectory(prefix="poolrad-u6a-025c-") as temp:
        library = build_c(root, Path(temp))
        if args.unicorn:
            differential(body, library)
        if args.native:
            native_audit(root, body, library)


if __name__ == "__main__":
    main()
