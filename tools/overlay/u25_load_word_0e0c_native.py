#!/usr/bin/env python3
"""Synthetic DOSBox-X original/C load-word oracle; all products are ignored.

This does not establish natural saved-game reachability. Reuses the native
oracle method from u5e_half_0e52_audit.py and the load-word pinned spans.
"""

import ctypes
from hashlib import sha256
import json
import os
from pathlib import Path
import struct
import subprocess

from u25_load_word_0e0c_audit import Cpu, ROOT, SIZE, library, physical, pointer, put16, source, word


ADDRESSES = [0xB1, 0xFB, 0xFC, 0x33D, 0x35F, 0x4900, 0x4CFF,
    0x6B00, 0x6B18, 0x6B9B, 0x6BA0, 0x6BA5, 0x6BAC, 0x6BB8, 0x6BBB,
    0x6BBD, 0x6BBF, 0x6BC1, 0x6BC3, 0x6BD8, 0x6BF7, 0x6BF9, 0x6C00,
    0x6C0C, 0x6C0D, 0x6C1B, 0x6DB1, 0x6DB4, 0x6E12, 0x6E3E, 0x6DD5,
    0x9700, 0x98FF, 0x9900, 0xB6FF, 0xC04A, 0xC04B, 0xC04C, 0xC04D,
    0xC04E, 0xC04F, 0xC050, 0xC059, 0xFFFF]
CASES = [(address, 26) for address in ADDRESSES] + [(0x6DCF, value) for value in (0, 3, 4, 7, 8, 12, 13, 16, 17, 18, 25, 26)]
FRAME_START, FRAME_SIZE, RECORD_SIZE = 0xEFBA, 80, 116


def native(directory, bodies, lib):
    directory.mkdir(parents=True, exist_ok=True)
    prefix, load = directory / "prefix.bin", directory / "load.bin"
    prefix.write_bytes(bodies[0x6FB] + bodies[0x756] + bodies[0x7BC])
    load.write_bytes(bodies[0xE0C])
    asm = [
        ".intel_syntax noprefix", ".code16", ".global _start", "_start: jmp main",
        ".org 0x5fb", '.incbin "' + str(prefix.resolve()) + '"',
        ".org 0xd0c", "load_entry:", '.incbin "' + str(load.resolve()) + '"',
        ".org 0x1f00", "main:", "mov ax,cs", "mov ds,ax", "mov es,ax", "mov ss,ax",
        "mov sp,0xf000", "cld", "mov di,0x7000", "mov cx,0x3200", "mov al,0xa5", "rep stosb",
    ]
    for off, target in ((0x49D2, 0x7000), (0x49D6, 0x7800), (0x49DA, 0x8000),
                        (0x49DE, 0x8400), (0x5D92, 0x9000), (0x5D96, 0x9000)):
        asm += [f"mov word ptr [{off}],{target}", f"mov word ptr [{off + 2}],cs"]
    for off, value in ((0x6F6E, 0xFEDC), (0x6F6A, 0x1234), (0x6F6C, 0xABCD),
                       (0x7830, 10), (0x9104, 0), (0x9106, 0)):
        asm.append(f"mov word ptr [{off}],{value}")
    for off, value in ((0x6AAD, 255), (0x6AAE, 128), (0x6AAF, 255), (0x6AB0, 163),
                       (0x6AB1, 7), (0x5376, 3), (0x9014, 7), (0x906E, 255), (0x9073, 255)):
        asm.append(f"mov byte ptr [{off}],{value}")
    asm += [
        "mov word ptr [header],cs", "mov word ptr [header+2],offset returned",
        "mov word ptr [header+4],offset load_entry", f"mov word ptr [header+6],{len(CASES)}",
        "mov dx,offset filename", "xor cx,cx", "mov ah,0x3c", "int 0x21", "jc failure",
        "mov word ptr [handle],ax", "mov bx,ax", "mov dx,offset header", "mov cx,8", "mov ah,0x40", "int 0x21",
        "again:", "mov sp,0xf000", "push cs", "pop es", "cld",
        f"mov di,{FRAME_START}", f"mov cx,{FRAME_SIZE}", "mov al,0xa5", "rep stosb",
        "mov word ptr [0xeff6],0xd35a", "mov word ptr [0xefe4],0xbeef",
        "mov si,word ptr [index]", "shl si,1", "shl si,1",
        "mov ax,word ptr [case_table+si]", "mov word ptr [address],ax",
        "mov ax,word ptr [case_table+si+2]", "mov byte ptr [0x9015],al",
        "push word ptr [address]", "mov ax,0x0202", "push ax", "popf",
        "mov ax,0x9234", "mov bx,0x1357", "mov cx,0x2468", "mov dx,0xabcd",
        "mov si,0x5678", "mov di,0x6789", "mov bp,0x4321", "push cs", "call load_entry", "returned:",
    ]
    for i, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        asm.append(f"mov word ptr [output+{2*i}],{reg}")
    asm += ["mov word ptr [output+18],offset returned", "mov word ptr [output+20],ds",
            "mov word ptr [output+22],es", "mov word ptr [output+24],ss"]
    for off in range(0, FRAME_SIZE, 2):
        asm += [f"mov ax,word ptr [{FRAME_START+off}]", f"mov word ptr [output+{28+off}],ax"]
    asm += [
        "pushf", "pop ax", "mov word ptr [output+26],ax",
        "mov ax,word ptr [address]", "mov word ptr [output+108],ax",
        "mov ax,word ptr [0x9014]", "mov word ptr [output+110],ax",
        "mov ax,word ptr [0x7830]", "mov word ptr [output+112],ax",
        "mov al,byte ptr [0x9015]", "xor ah,ah", "mov word ptr [output+114],ax",
        "mov bx,word ptr [handle]", "mov dx,offset output", f"mov cx,{RECORD_SIZE}", "mov ah,0x40", "int 0x21",
        "jc failure", f"cmp ax,{RECORD_SIZE}", "jne failure", "inc word ptr [index]",
        f"cmp word ptr [index],{len(CASES)}", "jb again",
        "mov bx,word ptr [handle]", "mov ah,0x3e", "int 0x21", "mov ax,0x4c00", "int 0x21",
        "failure:", "mov ax,0x4c01", "int 0x21", 'filename: .asciz "RESULT.BIN"',
        "header: .space 8", "handle: .word 0", "index: .word 0", "address: .word 0",
        f"output: .space {RECORD_SIZE}", "case_table:",
    ]
    asm += [f".word {address},{value}" for address, value in CASES]
    asm_path, obj, com = directory / "oracle.S", directory / "oracle.o", directory / "ORACLE.COM"
    asm_path.write_text("\n".join(asm) + "\n")
    subprocess.run(["as", "--32", str(asm_path), "-o", str(obj)], check=True, capture_output=True, text=True)
    subprocess.run(["ld", "-m", "elf_i386", "-Ttext", "0x100", "--oformat", "binary", str(obj), "-o", str(com)],
                   check=True, capture_output=True, text=True)
    config = directory / "dosbox.conf"
    config.write_text("[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n[cpu]\ncore=normal\ncputype=386\ncycles=max\n"
        "[mixer]\nnosound=true\n[midi]\nmididevice=none\n[autoexec]\nmount c " + str(directory.resolve()) +
        "\nc:\nORACLE.COM\nexit\n")
    result_path = directory / "RESULT.BIN"
    result_path.unlink(missing_ok=True)
    with (directory / "dosbox.log").open("w") as log:
        subprocess.run(["dosbox-x", "-conf", str(config), "-fastlaunch", "-exit", "-noconsole"],
            env=dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy"),
            stdout=log, stderr=subprocess.STDOUT, check=True, timeout=45)
    result = result_path.read_bytes()
    check_native(result, lib, com.read_bytes())
    return dict(native_cases=len(CASES), result_sha256=sha256(result).hexdigest())


def check_native(result, lib, code):
    if len(result) != 8 + len(CASES) * RECORD_SIZE:
        raise ValueError("synthetic native output incomplete")
    segment, return_ip, entry_ip, count = struct.unpack_from("<4H", result)
    if entry_ip != 0xE0C or count != len(CASES):
        raise ValueError("synthetic native header differs")
    for index, (address, value) in enumerate(CASES):
        observed = result[8 + index * RECORD_SIZE:8 + (index + 1) * RECORD_SIZE]
        ram = bytearray([0xA5]) * SIZE
        code_at = physical(segment, 0x100)
        ram[code_at:code_at + len(code)] = code
        for off, target in ((0x49D2, 0x7000), (0x49D6, 0x7800), (0x49DA, 0x8000),
                            (0x49DE, 0x8400), (0x5D92, 0x9000), (0x5D96, 0x9000)):
            pointer(ram, segment, off, segment, target)
        for off, data in ((0x6F6E, 0xFEDC), (0x6F6A, 0x1234), (0x6F6C, 0xABCD),
                          (0x7830, 10), (0x9104, 0), (0x9106, 0)):
            put16(ram, segment, off, data)
        for off, data in ((0x6AAD, 255), (0x6AAE, 128), (0x6AAF, 255), (0x6AB0, 163),
                          (0x6AB1, 7), (0x5376, 3), (0x9014, 7), (0x906E, 255), (0x9073, 255),
                          (0x9015, value)):
            ram[physical(segment, off)] = data
        for off, data in ((0xEFF6, 0xD35A), (0xEFE4, 0xBEEF), (0xEFFA, return_ip),
                          (0xEFFC, segment), (0xEFFE, address)):
            put16(ram, segment, off, data)
        cpu = Cpu(0x9234, 0x1357, 0x2468, 0xABCD, 0x5678, 0x6789,
                  0x4321, 0xEFFA, segment, entry_ip, segment, segment, segment, 0x0202)
        memory = (ctypes.c_uint8 * SIZE).from_buffer(ram)
        status = lib.pr_u25_load_word_0e0c(memory, SIZE, ctypes.byref(cpu))
        if status != 0:
            raise ValueError(f"C rejected native case {index}")
        for (name, _), expected in zip(Cpu._fields_, struct.unpack_from("<14H", observed)):
            undefined_af = address in (0x33D, 0xC04D, 0xC04E, 0xC04F) or 0x9900 <= address <= 0xB6FF
            mask = 0xFFEF if name == "flags" and undefined_af else 0xFFFF
            if (getattr(cpu, name) ^ expected) & mask:
                raise ValueError(f"native {name} mismatch {index}: {getattr(cpu, name):04X}/{expected:04X}")
        at = physical(segment, FRAME_START)
        if ram[at:at + FRAME_SIZE] != observed[28:108]:
            raise ValueError(f"native stack-frame mismatch {index}")
        if struct.unpack_from("<4H", observed, 108) != (
                address, word(ram, segment, 0x9014), word(ram, segment, 0x7830), value):
            raise ValueError(f"native input-preservation mismatch {index}")


def main():
    directory = ROOT / "resources/u25-load-word-parity/native"
    directory.mkdir(parents=True, exist_ok=True)
    bodies, _ = source(ROOT / "resources/DOS/POOLRAD/GAME.OVR")
    lib = library(directory)
    print(json.dumps(native(directory, bodies, lib), sort_keys=True))


if __name__ == "__main__":
    main()
