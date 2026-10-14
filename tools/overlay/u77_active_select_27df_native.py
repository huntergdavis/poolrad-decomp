#!/usr/bin/env python3
"""Synthetic DOSBox-X U77 far-call frames; products stay in resources/.

Reuses the u25_load_word_0e0c_native.py COM oracle method. This executes
an actual far CALL, but does not establish a reached game caller or route.
"""

import ctypes
from hashlib import sha256
import json
import os
from pathlib import Path
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests/recovered"))
from check_overlay_u77_active_select_27df import (  # noqa: E402
    Cpu, RAM_SIZE, build_library, load_source, physical, put_far, put_word,
)

CASES = [(head, active, key) for head, active in
         ((0, 0), (0x9000, 0x9000), (0x9000, 0x9200), (0x9000, 0x9400))
         for key in (0x1247, 0xAB4F, 0xA558)]
FRAME, FRAME_SIZE = 0xEFD0, 48
RECORD_SIZE = 28 + FRAME_SIZE + 8 + 12


def main():
    directory = ROOT / "resources/u77-active-select-parity/native"
    directory.mkdir(parents=True, exist_ok=True)
    source = load_source()
    body = directory / "body.bin"
    body.write_bytes(source)
    asm = [".intel_syntax noprefix", ".code16", ".global _start",
           "_start: jmp main", ".org 0x26df", '.incbin "' + str(body) + '"',
           ".org 0x3000", "main:", "mov ax,cs", "mov ds,ax", "mov es,ax",
           "mov ss,ax", "mov sp,0xf000", "mov word ptr [call_segment],ax",
           "mov word ptr [header],ax", "mov word ptr [header+2],offset returned",
           f"mov word ptr [header+4],{len(CASES)}", "mov dx,offset filename",
           "xor cx,cx", "mov ah,0x3c", "int 0x21", "jc failure",
           "mov word ptr [handle],ax", "mov bx,ax", "mov dx,offset header",
           "mov cx,6", "mov ah,0x40", "int 0x21", "again:", "cli",
           "mov sp,0xf000", "push cs", "pop es", "cld",
           f"mov di,{FRAME}", f"mov cx,{FRAME_SIZE}", "mov al,0xa5", "rep stosb"]
    for off, target in ((0x9104, 0x9200), (0x9304, 0x9400), (0x9504, 0)):
        asm += [f"mov word ptr [{off}],{target}",
                f"mov word ptr [{off+2}]," + ("cs" if target else "0")]
    asm += ["mov si,word ptr [index]", "shl si,1", "mov bx,si", "shl si,1",
            "add si,bx", "mov ax,word ptr [cases+si]", "mov word ptr [0x5d96],ax",
            "mov dx,cs", "test ax,ax", "jnz head_segment", "xor dx,dx",
            "head_segment:", "mov word ptr [0x5d98],dx",
            "mov ax,word ptr [cases+si+2]", "mov word ptr [0x5d92],ax",
            "mov dx,cs", "test ax,ax", "jnz active_segment", "xor dx,dx",
            "active_segment:", "mov word ptr [0x5d94],dx",
            "push word ptr [cases+si+4]", "push 0x0002", "popf",
            "mov ax,0xabcd", "mov bx,0x3456", "mov cx,0x5678", "mov dx,0x789a",
            "mov si,0x9abc", "mov di,0xbcde", "mov bp,0x7777",
            ".byte 0x9a", ".word 0x27df", "call_segment: .word 0", "returned:"]
    for index, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        asm.append(f"mov word ptr [output+{2*index}],{reg}")
    asm += ["mov word ptr [output+18],offset returned", "mov word ptr [output+20],ds",
            "mov word ptr [output+22],es", "mov word ptr [output+24],ss"]
    offsets = list(range(FRAME, FRAME + FRAME_SIZE, 2)) + list(range(0x5D92, 0x5D9A, 2))
    offsets += [0x9104, 0x9106, 0x9304, 0x9306, 0x9504, 0x9506]
    for index, off in enumerate(offsets):
        asm += [f"mov ax,word ptr [{off}]", f"mov word ptr [output+{28+2*index}],ax"]
    asm += ["pushf", "pop ax", "mov word ptr [output+26],ax", "sti",
            "mov bx,word ptr [handle]", "mov dx,offset output",
            f"mov cx,{RECORD_SIZE}", "mov ah,0x40", "int 0x21", "jc failure",
            f"cmp ax,{RECORD_SIZE}", "jne failure", "inc word ptr [index]",
            f"cmp word ptr [index],{len(CASES)}", "jb again",
            "mov bx,word ptr [handle]", "mov ah,0x3e", "int 0x21",
            "mov ax,0x4c00", "int 0x21", "failure:", "mov ax,0x4c01", "int 0x21",
            'filename: .asciz "RESULT.BIN"', "header: .space 6", "handle: .word 0",
            "index: .word 0", f"output: .space {RECORD_SIZE}", "cases:"]
    asm += [f".word {head},{active},{key}" for head, active, key in CASES]
    assembly, obj, com = directory / "oracle.S", directory / "oracle.o", directory / "ORACLE.COM"
    assembly.write_text("\n".join(asm) + "\n")
    subprocess.run(["as", "--32", str(assembly), "-o", str(obj)], check=True)
    subprocess.run(["ld", "-m", "elf_i386", "-Ttext", "0x100", "--oformat", "binary",
                    str(obj), "-o", str(com)], check=True)
    config = directory / "dosbox.conf"
    config.write_text("[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n[cpu]\ncore=normal\n"
        "cputype=386\ncycles=max\n[mixer]\nnosound=true\n[midi]\nmididevice=none\n"
        "[autoexec]\nmount c " + str(directory) + "\nc:\nORACLE.COM\nexit\n")
    result_path = directory / "RESULT.BIN"
    result_path.unlink(missing_ok=True)
    with (directory / "dosbox.log").open("w") as log:
        subprocess.run(["dosbox-x", "-conf", str(config), "-fastlaunch", "-exit", "-noconsole"],
            env=dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy"),
            stdout=log, stderr=subprocess.STDOUT, timeout=45, check=True)
    result = result_path.read_bytes()
    if len(result) != 6 + len(CASES) * RECORD_SIZE:
        raise ValueError("native output incomplete")
    segment, return_ip, count = struct.unpack_from("<3H", result)
    if count != len(CASES):
        raise ValueError("native case count differs")
    lib = build_library(directory)
    for index, (head, active, key) in enumerate(CASES):
        observed = result[6 + index * RECORD_SIZE:6 + (index + 1) * RECORD_SIZE]
        memory = bytearray([0xA5]) * RAM_SIZE
        code = com.read_bytes()
        at = physical(segment, 0x100)
        memory[at:at + len(code)] = code
        put_far(memory, segment, 0x5D96, (segment if head else 0, head))
        put_far(memory, segment, 0x5D92, (segment if active else 0, active))
        for off, target in ((0x9104, 0x9200), (0x9304, 0x9400), (0x9504, 0)):
            put_far(memory, segment, off, (segment if target else 0, target))
        for off, value in ((0xEFFA, return_ip), (0xEFFC, segment), (0xEFFE, key)):
            put_word(memory, segment, off, value)
        cpu = Cpu(0xABCD, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE,
                  0x7777, 0xEFFA, segment, 0x27DF, segment, segment, segment, 0x0002)
        ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer(memory)
        if lib.pr_u77_active_select_27df(ram, RAM_SIZE, ctypes.byref(cpu)):
            raise ValueError("C rejected native state")
        for (name, _), expected in zip(Cpu._fields_, struct.unpack_from("<14H", observed)):
            undefined_af = not head or (key & 255) == 0x4F or ((key & 255) == 0x47 and head == active)
            mask = 0xFFEF if name == "flags" and undefined_af else 0xFFFF
            if (getattr(cpu, name) ^ expected) & mask:
                raise ValueError(f"native {index} {name} mismatch")
        actual = b"".join(bytes(memory[physical(segment, off):physical(segment, off)+2])
                          for off in offsets)
        if actual != observed[28:]:
            raise ValueError(f"native {index} frame/table/link mismatch")
    report = dict(native_cases=count, actual_far_call=True, reached_game_caller=False,
                  interrupts_isolated=True, source_sha256=sha256(source).hexdigest(),
                  compared_cpu_words=14, compared_frame_bytes=FRAME_SIZE,
                  result_sha256=sha256(result).hexdigest())
    (directory / "report.json").write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
