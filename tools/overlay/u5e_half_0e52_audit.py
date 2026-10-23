#!/usr/bin/env python3
"""Verify the complete u5E +0E52 leaf using ignored DOS originals."""

import argparse
import ctypes
from hashlib import sha256
import os
from pathlib import Path
import random
import struct
import subprocess
import tempfile

from capstone import CS_ARCH_X86, CS_MODE_16, Cs
from overlay_inventory import find_units, parse_mz
from u5e_party_value_2879_audit import Cpu, EXE_SHA, OVR_SHA, physical, put16
from pinned_shift_flags import LastFlagWriter, SPLIT_BY_RULE, declared_flags_split, split_used

RAM_SIZE = 1 << 20
ENTRY, END = 0x0E52, 0x0E7B
BODY_SHA = "c99969f9b7b28b6bdd15297e8b711a8491b605df8055135eb114b6215dfc82a2"


def check(condition, message):
    if not condition:
        raise ValueError(message)


def source_audit(exe_path, ovr_path):
    exe, ovr = exe_path.read_bytes(), ovr_path.read_bytes()
    check(sha256(exe).hexdigest() == EXE_SHA, "unsupported START.EXE")
    check(sha256(ovr).hexdigest() == OVR_SHA, "unsupported GAME.OVR")
    units = {u["stub_para"]: u for u in
             find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)}
    unit = units[0x5E]
    check(unit["ovr_offset"] == 0x127FB and unit["memory_segment"] == 0x68,
          "unit placement changed")
    body = ovr[unit["ovr_offset"] + ENTRY:unit["ovr_offset"] + END]
    check(len(body) == 41 and sha256(body).hexdigest() == BODY_SHA,
          "u5E +0E52 body changed")
    decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(len(decoded) == 17 and decoded[-1].address == 0x0E78 and
          decoded[-1].mnemonic == "retf" and decoded[-1].op_str == "2" and
          decoded[-1].address + decoded[-1].size == END,
          "routine extent changed")
    check(not any("call" in ins.mnemonic for ins in decoded),
          "leaf gained a call")
    branches = [(ins.address, ins.mnemonic, ins.op_str) for ins in decoded
                if ins.mnemonic.startswith("j")]
    check(branches == [(0x0E63, "jae", "0xe68")], "CFG changed")
    caller = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(
        ovr[unit["ovr_offset"] + 0x0D23:unit["ovr_offset"] + ENTRY], 0x0D23))
    sites = {ins.address: ins for ins in caller}
    check(unit["entries"][6] == 0x0D23 and
          sites[0x0DBE].mnemonic == "push" and sites[0x0DBE].op_str == "cs" and
          sites[0x0DBF].mnemonic == "call" and sites[0x0DBF].op_str == "0xe52",
          "instruction-confirmed caller changed")
    check(sites[0x0E47].mnemonic == "mov" and
          sites[0x0E47].op_str == "byte ptr es:[di + 0x113], al",
          "caller character-record store changed")
    callers = []
    for source in units.values():
        for fixup in source["fixups"]:
            at = source["ovr_offset"] + fixup - 3
            if fixup < 3 or ovr[at] != 0x9A:
                continue
            offset, segment = struct.unpack_from("<HH", ovr, at + 1)
            if (segment, offset) == (0x68, 0x3E):  # entry 6
                callers.append((source["stub_para"], fixup - 3))
    check(len(callers) == 3, "entry-6 fixup-confirmed caller count changed")
    print(f"START.EXE SHA-256 {EXE_SHA}; GAME.OVR SHA-256 {OVR_SHA}")
    print(f"u5E +0E52..+0E7A: 41 bytes, 17 instructions, 1 branch, "
          f"0 calls; SHA-256 {BODY_SHA}")
    print("caller u5E +0DBF (entry 6 +0D23), record store +0113; "
          f"entry-6 far caller sites: {[(hex(u), hex(s)) for u, s in callers]}")
    return body


def build_c(root, directory):
    library_path = directory / "u5e_0e52.so"
    subprocess.run([os.environ.get("CC", "cc"), "-std=c11", "-Wall", "-Wextra",
                    "-Wpedantic", "-Werror", "-shared", "-fPIC", "-I",
                    str(root / "include"),
                    str(root / "src/recovered/overlay_u5e_half_0e52.c"),
                    "-o", str(library_path)], check=True, capture_output=True,
                   text=True)
    library = ctypes.CDLL(str(library_path))
    library.pr_u5e_half_0e52.argtypes = [ctypes.POINTER(ctypes.c_uint8),
                                       ctypes.c_size_t, ctypes.POINTER(Cpu)]
    library.pr_u5e_half_0e52.restype = ctypes.c_int
    return library


def run_c(library, memory, cpu):
    ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
    check(library.pr_u5e_half_0e52(ram, RAM_SIZE, ctypes.byref(cpu)) == 0,
          "portable C rejected RAM")
    return bytes(ram), cpu


def synthetic_case(body, value, flag, *, ss=0x2000, sp=0x8000, ds=0x1000,
                   bp=0x7777, flags=0x0202):
    ram = bytearray([0xA5]) * RAM_SIZE
    cs = 0x3000
    ram[physical(cs, ENTRY):physical(cs, ENTRY) + len(body)] = body
    for offset, word in ((0, 0x7000), (2, cs), (4, 0x5A00 | value)):
        put16(ram, ss, (sp + offset) & 0xFFFF, word)
    ram[physical(ds, 0x6DE3)] = flag
    cpu = Cpu(0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE,
              bp, sp, cs, ENTRY, ds, 0x1111, ss, flags)
    return ram, cpu


def unicorn_audit(body, library):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16
    from unicorn import x86_const as regs
    register_ids = [getattr(regs, "UC_X86_REG_" +
                    ("EFLAGS" if name == "flags" else name.upper()))
                    for name, _ in Cpu._fields_]
    rng = random.Random(0x5E0E52)
    # Build one case at a time: keeping hundreds of full RAM copies costs
    # more than the emulator and obscures the test's small state footprint.
    specifications = [(f"value-{value}-bit-{flag}", value, flag, {})
                      for value in range(256) for flag in (0, 1)]
    specifications += [(f"flag-{flag}", rng.randrange(256), flag, {})
                       for flag in range(256)]
    specifications += [(f"stack-{sp:04x}", 255, 1, {"sp": sp})
                       for sp in (0, 1, 2, 3, 4, 0xFFFB, 0xFFFC, 0xFFFD,
                                  0xFFFE, 0xFFFF)]
    # DS6DE3 aliases each of the ordered writes, argument, and return word.
    specifications += [(f"alias-{sp:04x}", 255, 1,
                        {"ss": 0x1000, "ds": 0x1000, "sp": sp})
                       for sp in range(0x6DDF, 0x6DE8)]
    specifications += [("A20-stack", 255, 1, {"ss": 0xFFFF, "sp": 0x11}),
                       ("A20-global", 255, 1, {"ds": 0xFFFF}),
                       ("flags-DF", 255, 0, {"flags": 0x0603}),
                       ("flags-arithmetic", 254, 1, {"flags": 0x0ED7})]
    split_arrivals = 0
    for label, value, flag, options in specifications:
        memory, cpu = synthetic_case(body, value, flag, **options)
        original = Uc(UC_ARCH_X86, UC_MODE_16)
        original.mem_map(0, 2 * RAM_SIZE)
        original.mem_write(0, bytes(memory))
        original.mem_write(RAM_SIZE, bytes(memory))
        from unicorn import UC_HOOK_MEM_WRITE

        def mirror_a20(emulator, access, address, size, value, user):
            del access, user
            for byte in range(size):
                raw = address + byte
                at = raw & (RAM_SIZE - 1)
                mirror = at + RAM_SIZE if raw < RAM_SIZE else at
                emulator.mem_write(mirror, bytes([(value >> (8 * byte)) & 0xFF]))

        original.hook_add(UC_HOOK_MEM_WRITE, mirror_a20)
        for (name, _), register in zip(Cpu._fields_, register_ids):
            original.reg_write(register, getattr(cpu, name))
        # The 17-instruction span takes 16 instructions when JAE skips INC.
        # Stop immediately after RETF, before executing any return target.
        from unicorn import UC_HOOK_CODE
        returned = [False]
        writer = LastFlagWriter()  # declared splits for the pinned flag rules, see pinned_shift_flags

        def stop_after_return(emulator, address, size, user):
            del size, user
            writer.step(emulator, address)
            if address == physical(cpu.cs, 0x0E78):
                returned[0] = True

        original.hook_add(UC_HOOK_CODE, stop_after_return)
        # Count to RETF from the trace, avoiding executable-memory or
        # return-target assumptions even when arguments alias return words.
        for _ in range(17):
            start = physical(original.reg_read(regs.UC_X86_REG_CS),
                             original.reg_read(regs.UC_X86_REG_IP))
            original.emu_start(start, RAM_SIZE, count=1)
            if returned[0]:
                break
        check(returned[0], f"original did not return: {label}")
        writer.finish(original)
        expected = bytes(original.mem_read(0, RAM_SIZE))
        actual, cpu = run_c(library, memory, cpu)
        if actual != expected:
            differences = [i for i, (a, b) in enumerate(zip(actual, expected))
                           if a != b]
            raise ValueError(f"RAM mismatch {label}: {differences[:8]}")
        for (name, _), register in zip(Cpu._fields_, register_ids):
            expected_value = original.reg_read(register) & 0xFFFF
            if name == "flags":
                problem = declared_flags_split(cpu.flags, expected_value, writer)
                check(problem is None, f"register flags mismatch {label}: {problem}")
                split_arrivals += split_used(cpu.flags, expected_value, writer)
                continue
            check(getattr(cpu, name) == expected_value,
                  f"register {name} mismatch {label}: "
                  f"C={getattr(cpu, name):04x}, original={expected_value:04x}")
    print(f"{len(specifications)} original-byte/C cases: full 1 MiB RAM and 13 returned register words agree; FLAGS "
          f"agree except the declared pinned splits at {split_arrivals} returns {SPLIT_BY_RULE}")


def native_audit(body, library, directory):
    """Execute the hash-locked leaf in a synthetic DOS COM, not saved gameplay."""
    directory.mkdir(parents=True, exist_ok=True)
    source_path = directory / "leaf.bin"
    source_path.write_bytes(body)
    assembly = r'''.code16
.intel_syntax noprefix
.global _start
_start:
    mov ax, cs
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov sp, 0xf000
    mov word ptr [header], ax
    mov word ptr [header+2], offset returned
    mov word ptr [header+4], offset leaf
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
    mov sp, 0xf000
    mov ax, word ptr [index]
    mov byte ptr [0x6de3], ah
    mov ah, 0x5a
    push ax
    mov ax, 0x0202
    push ax
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
    mov word ptr [output], ax
    mov word ptr [output+2], bx
    mov word ptr [output+4], cx
    mov word ptr [output+6], dx
    mov word ptr [output+8], si
    mov word ptr [output+10], di
    mov word ptr [output+12], bp
    mov word ptr [output+14], sp
    mov word ptr [output+16], cs
    mov word ptr [output+18], offset returned
    mov word ptr [output+20], ds
    mov word ptr [output+22], es
    mov word ptr [output+24], ss
    mov ax, word ptr [0xeff6]
    mov word ptr [output+28], ax
    mov ax, word ptr [0xeff8]
    mov word ptr [output+30], ax
    mov ax, word ptr [0xeffa]
    mov word ptr [output+32], ax
    mov ax, word ptr [0xeffc]
    mov word ptr [output+34], ax
    mov ax, word ptr [0xeffe]
    mov word ptr [output+36], ax
    pushf
    pop ax
    mov word ptr [output+26], ax
    mov al, byte ptr [0x6de3]
    mov byte ptr [output+38], al
    mov ax, word ptr [index]
    mov byte ptr [output+39], al
    mov bx, word ptr [handle]
    mov dx, offset output
    mov cx, 40
    mov ah, 0x40
    int 0x21
    jc failure
    cmp ax, 40
    jne failure
    inc word ptr [index]
    cmp word ptr [index], 512
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
output: .space 40
leaf:
'''
    assembly += f'.incbin "{source_path.resolve()}"\n'
    asm_path, object_path = directory / "oracle.S", directory / "oracle.o"
    com_path = directory / "ORACLE.COM"
    asm_path.write_text(assembly)
    subprocess.run(["as", "--32", str(asm_path), "-o", str(object_path)], check=True)
    subprocess.run(["ld", "-m", "elf_i386", "-Ttext", "0x100",
                    "--oformat", "binary", str(object_path), "-o", str(com_path)],
                   check=True)
    # Keep emulator configuration, copied original instructions, and binary
    # results beneath ignored resources; publish only hashes and counts.
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
    check(len(result) == 6 + 512 * 40, "native oracle output is incomplete")
    segment, return_ip, leaf_ip = struct.unpack_from("<HHH", result)
    for index in range(512):
        record = result[6 + index * 40:6 + (index + 1) * 40]
        memory = bytearray([0xA5]) * RAM_SIZE
        cpu = Cpu(0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE,
                  0x7777, 0xEFFA, segment, leaf_ip, segment, segment,
                  segment, 0x0202)
        for displacement, value in ((0, return_ip), (2, segment),
                                    (4, 0x5A00 | (index & 0xFF))):
            put16(memory, segment, cpu.sp + displacement, value)
        memory[physical(segment, 0x6DE3)] = index >> 8
        actual, cpu = run_c(library, memory, cpu)
        native_words = struct.unpack_from("<14H", record)
        for (name, _), expected in zip(Cpu._fields_, native_words):
            # Full FLAGS, AF included: the model follows the pinned DOSBox-X
            # rule (AF set after a non-zero SHR), so nothing is masked here.
            check(getattr(cpu, name) == expected,
                  f"native {name} mismatch at {index}: C={getattr(cpu, name):04x}, DOSBox-X={expected:04x}")
        at = physical(segment, 0xEFF6)
        check(actual[at:at + 10] == record[28:38],
              f"native stack RAM mismatch at {index}")
        check(actual[physical(segment, 0x6DE3)] == record[38] and
              actual[physical(segment, 0xEFFE)] == record[39],
              f"native argument/global mismatch at {index}")
    print("512 DOSBox-X synthetic original-byte/C cases: all returned "
          "registers and full FLAGS (AF included, no mask), frame bytes, argument, and global agree")
    print("native result SHA-256 " + sha256(result).hexdigest())


def main():
    root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path,
                        default=root / "resources/DOS/POOLRAD/START.EXE")
    parser.add_argument("--ovr", type=Path,
                        default=root / "resources/DOS/POOLRAD/GAME.OVR")
    parser.add_argument("--unicorn", action="store_true")
    parser.add_argument("--native", action="store_true")
    args = parser.parse_args()
    body = source_audit(args.exe, args.ovr)
    with tempfile.TemporaryDirectory(prefix="poolrad-u5e-0e52-") as temp:
        library = build_c(root, Path(temp))
        if args.unicorn:
            unicorn_audit(body, library)
        if args.native:
            native_audit(body, library,
                         root / "resources/oracle-runs/u5e-half-0e52-synthetic")


if __name__ == "__main__":
    main()
