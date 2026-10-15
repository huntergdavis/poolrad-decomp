#!/usr/bin/env python3
"""Bounded DOSBox original/C parity; synthetic COM, NOT a reached game frame.

Capture pattern adapted from PR407's u6a_record_remaining_025c_audit.py.
No original bytes are embedded in this public generator.
"""
import ctypes
from hashlib import sha256
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests/recovered"))
import check_overlay_u6a_sort_0824 as base


def assembly(source):
    text = r'''.code16
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
    mov word ptr [0x5d92], 0x4000
    mov word ptr [0x5d94], ax
    mov dx, offset filename
    xor cx, cx
    mov ah, 0x3c
    int 0x21
    jc failure
    mov word ptr [handle], ax
    mov bx, ax
    mov dx, offset header
    mov cx, 4
    mov ah, 0x40
    int 0x21
again:
    xor si, si
fill:
    mov ax, si
    mov bx, 37
    mul bx
    mov cx, ax
    mov ax, word ptr [index]
    mov bx, 13
    mul bx
    add ax, cx
    xor ax, word ptr [index]
    test word ptr [index], 1
    jz store
    and al, 0x83
store:
    mov byte ptr [si+0x4017], al
    inc si
    cmp si, 21
    jb fill
    mov di, 0xeff6
    mov cx, 10
    mov al, 0xa5
    cld
    rep stosb
    mov sp, 0xf000
    mov ax, word ptr [index]
    and ax, 2
    shl ax, 9
    or ax, 0x0202
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
'''
    for i, (name, _) in enumerate(base.Cpu._fields_[:-1]):
        value = "offset returned" if name == "ip" else name
        text += f"    mov word ptr [output+{2*i}], {value}\n"
    # MOV preserves flags; capture frame before PUSHF reuses its return bytes.
    for i in range(5):
        text += (f"    mov ax, word ptr [{0xeff6 + 2*i}]\n"
                 f"    mov word ptr [output+{28 + 2*i}], ax\n")
    text += r'''
    pushf
    pop ax
    mov word ptr [output+26], ax
    mov ax, word ptr [0x5d92]
    mov word ptr [output+38], ax
    mov ax, word ptr [0x5d94]
    mov word ptr [output+40], ax
    mov si, 0x4017
    mov di, offset output+42
    mov cx, 21
    cld
    rep movsb
    mov ax, word ptr [index]
    mov word ptr [output+63], ax
    mov bx, word ptr [handle]
    mov dx, offset output
    mov cx, 65
    mov ah, 0x40
    int 0x21
    jc failure
    cmp ax, 65
    jne failure
    inc word ptr [index]
    cmp word ptr [index], 128
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
header: .space 4
handle: .word 0
index: .word 0
output: .space 65
.org 0x724
leaf:
'''
    return text + f'.incbin "{source.resolve()}"\n'


def main():
    body = base.load_source()
    directory = ROOT / "resources/oracle-runs/u6a-0824-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    source = directory / "leaf.bin"
    source.write_bytes(body)
    asm, obj, com = (directory / n for n in ("oracle.S", "oracle.o", "ORACLE.COM"))
    asm.write_text(assembly(source))
    subprocess.run(["as", "--32", str(asm), "-o", str(obj)], check=True)
    subprocess.run(["ld", "-m", "elf_i386", "-Ttext", "0x100", "--oformat",
                    "binary", str(obj), "-o", str(com)], check=True)
    base.check(com.read_bytes()[0x724:] == body, "COM original body mismatch")
    config = directory / "dosbox.conf"
    config.write_text("[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n"
                      "[cpu]\ncore=normal\ncputype=386\ncycles=max\n"
                      "[mixer]\nnosound=true\n[midi]\nmididevice=none\n"
                      "[autoexec]\nmount c " + str(directory.resolve()) +
                      "\nc:\nORACLE.COM\nexit\n")
    result_path = directory / "RESULT.BIN"
    if result_path.exists():
        result_path.unlink()  # Only this generator's ignored output.
    environment = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy")
    with (directory / "dosbox.log").open("w") as log:
        subprocess.run(["dosbox-x", "-conf", str(config), "-fastlaunch",
                        "-exit", "-noconsole"], env=environment, stdout=log,
                       stderr=subprocess.STDOUT, check=True, timeout=45)
    result = result_path.read_bytes()
    base.check(len(result) == 4 + 128 * 65, "incomplete native result")
    segment, return_ip = struct.unpack_from("<HH", result)
    with tempfile.TemporaryDirectory(prefix="u6a-0824-native-") as tmp:
        library = base.build_library(Path(tmp))
        for case in range(128):
            record = result[4 + 65*case:4 + 65*(case+1)]
            memory = bytearray([0xa5]) * base.RAM_SIZE
            cpu = base.Cpu(0x1234, 0x3456, 0x5678, 0x789a, 0x9abc, 0xbcde,
                           0x7777, 0xeffc, segment, 0x0824, segment, segment,
                           segment, 0x0202 | ((case & 2) << 9))
            base.put_word(memory, segment, 0xeffc, return_ip)
            base.put_word(memory, segment, 0xeffe, segment)
            base.put_word(memory, segment, 0x5d92, 0x4000)
            base.put_word(memory, segment, 0x5d94, segment)
            for i in range(21):
                value = ((i*37 + case*13) ^ case) & 255
                memory[base.physical(segment, 0x4017+i)] = value & 0x83 if case & 1 else value
            ram = (ctypes.c_uint8 * base.RAM_SIZE).from_buffer_copy(memory)
            base.check(library.pr_u6a_sort_0824(ram, len(ram), ctypes.byref(cpu)) == 0,
                       "C rejected native fixture")
            base.check(bytes(cpu) == record[:28], f"native CPU mismatch case {case}")
            for offset, begin, length in ((0xeff6, 28, 10), (0x5d92, 38, 4),
                                           (0x4017, 42, 21)):
                at = base.physical(segment, offset)
                base.check(bytes(ram[at:at+length]) == record[begin:begin+length],
                           f"native memory region mismatch case {case}")
            base.check(struct.unpack_from("<H", record, 63)[0] == case,
                       "native case order mismatch")
    print("128 synthetic DOSBox-X/C cases: all 14 CPU words, 10 frame bytes, "
          "pointer and 21 array bytes equal; no flag mask")
    print("result SHA-256 " + sha256(result).hexdigest())
    print("No reached saved-game frame; native full-RAM parity and strict credit remain open")


if __name__ == "__main__":
    main()
