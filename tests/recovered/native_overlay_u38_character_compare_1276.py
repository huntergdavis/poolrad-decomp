#!/usr/bin/env python3
"""DOSBox-X frames for original U38:+1276 plus original SYSTEM :0C10."""

import ctypes
from hashlib import sha256
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile

import check_overlay_u38_character_compare_1276 as audit

ROOT = Path(__file__).resolve().parents[2]
MODES = range(4)  # no call, first equal, second equal, both unequal
FRAME = 0xEFD0
FRAME_BYTES = 52
RECORD_SIZE = 28 + FRAME_BYTES + 1


def build(directory, source_bytes):
    body, constants, system = source_bytes
    for name, content in (("wrapper.bin", body), ("constants.bin", constants),
                          ("system.bin", system)):
        (directory / name).write_bytes(content)
    asm = [
        ".intel_syntax noprefix", ".code16", ".global _start",
        "_start: jmp main", ".org 0x0b10",
        '.incbin "' + str(directory / "system.bin") + '"',
        ".org 0x1168",
        '.incbin "' + str(directory / "constants.bin") + '"',
        ".org 0x1176",
        '.incbin "' + str(directory / "wrapper.bin") + '"',
        ".org 0x3000", "main:",
        "mov ax,cs", "mov ds,ax", "mov es,ax", "mov ss,ax",
        "mov sp,0xf000", "mov word ptr [outer_segment],ax",
        "mov word ptr [0x1293],ax", "mov word ptr [0x12a5],ax",
        "mov word ptr [header],ax",
        "mov word ptr [header+2],offset returned",
        f"mov word ptr [header+4],{len(MODES)}",
        "mov dx,offset filename", "xor cx,cx", "mov ah,0x3c",
        "int 0x21", "jc failure", "mov word ptr [handle],ax",
        "mov bx,ax", "mov dx,offset header", "mov cx,6",
        "mov ah,0x40", "int 0x21", "again:",
        "cli", "cld", "mov sp,0xf000", f"mov di,{FRAME}",
        f"mov cx,{FRAME_BYTES}", "xor ax,ax", "rep stosb",
        "mov di,0x2200", "mov cx,8", "xor ax,ax", "rep stosb",
        "mov byte ptr [0x229f],1", "mov si,word ptr [index]",
        "or si,si", "jne choose_string",
        "mov byte ptr [0x229f],2", "jmp string_ready",
        "choose_string:", "cmp si,2", "je copy_second",
        "mov si,0x1268", "mov di,0x2200", "mov cx,8",
        "rep movsb", "cmp word ptr [index],3", "jne string_ready",
        "xor byte ptr [0x2207],0x20", "jmp string_ready",
        "copy_second:", "mov si,0x1270", "mov di,0x2200",
        "mov cx,6", "rep movsb", "string_ready:",
        "mov word ptr [0x5d92],0x2200", "mov ax,cs",
        "mov word ptr [0x5d94],ax", "mov byte ptr [0x6822],0xa6",
        "push 0x0402", "popf",
        "mov ax,0xa5cd", "mov bx,0x3456", "mov cx,0x5678",
        "mov dx,0x789a", "mov si,0x9abc", "mov di,0xbcde",
        "mov bp,0x7777", "push 0xa104", "push 0xa103",
        "push 0xa102", "push 0xa101", "push 0xa100",
        ".byte 0x9a", ".word 0x1276",
        "outer_segment: .word 0", "returned:",
    ]
    for i, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        asm.append(f"mov word ptr [output+{2*i}],{reg}")
    asm += ["mov word ptr [output+18],offset returned",
            "mov word ptr [output+20],ds", "mov word ptr [output+22],es",
            "mov word ptr [output+24],ss"]
    for i in range(FRAME_BYTES):
        asm += [f"mov al,byte ptr [{FRAME + i}]",
                f"mov byte ptr [output+{28 + i}],al"]
    asm += ["mov al,byte ptr [0x6822]",
            f"mov byte ptr [output+{28 + FRAME_BYTES}],al",
            "pushf", "pop ax", "mov word ptr [output+26],ax",
            "sti", "mov bx,word ptr [handle]", "mov dx,offset output",
            f"mov cx,{RECORD_SIZE}", "mov ah,0x40", "int 0x21",
            "jc failure", f"cmp ax,{RECORD_SIZE}", "jne failure",
            "inc word ptr [index]", f"cmp word ptr [index],{len(MODES)}",
            "jb again", "mov bx,word ptr [handle]", "mov ah,0x3e",
            "int 0x21", "mov ax,0x4c00", "int 0x21", "failure:",
            "mov ax,0x4c01", "int 0x21", 'filename: .asciz "RESULT.BIN"',
            "header: .space 6", "handle: .word 0", "index: .word 0",
            f"output: .space {RECORD_SIZE}"]
    assembly, obj, com = (directory / "oracle.S", directory / "oracle.o",
                          directory / "ORACLE.COM")
    assembly.write_text("\n".join(asm) + "\n")
    subprocess.run(["as", "--32", str(assembly), "-o", str(obj)], check=True)
    subprocess.run(["ld", "-m", "elf_i386", "-Ttext", "0x100",
                    "--oformat", "binary", str(obj), "-o", str(com)], check=True)
    return com


def run_native(directory):
    config = directory / "dosbox.conf"
    config.write_text("[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n"
        "[cpu]\ncore=normal\ncputype=386\ncycles=max\n[mixer]\n"
        "nosound=true\n[midi]\nmididevice=none\n[autoexec]\nmount c "
        + str(directory.resolve()) + "\nc:\nORACLE.COM\nexit\n")
    result = directory / "RESULT.BIN"
    result.unlink(missing_ok=True)
    with (directory / "dosbox.log").open("w") as log:
        subprocess.run(["dosbox-x", "-conf", str(config), "-fastlaunch",
                        "-exit", "-noconsole"],
                       env=dict(os.environ, SDL_VIDEODRIVER="dummy",
                                SDL_AUDIODRIVER="dummy"),
                       stdout=log, stderr=subprocess.STDOUT,
                       check=True, timeout=45)
    return result.read_bytes()


def compare(data, com, source_bytes):
    assert len(data) == 6 + len(MODES) * RECORD_SIZE
    seg, ret_ip, count = struct.unpack_from("<3H", data)
    assert count == len(MODES)
    with tempfile.TemporaryDirectory(prefix="u38-1276-native-") as tmp:
        library = Path(tmp) / "model.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/overlay_u38_character_compare_1276.c"),
                        "-o", str(library)], check=True)
        model = ctypes.CDLL(str(library)).pr_u38_character_compare_1276
        model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(audit.helper.Cpu)]
        model.restype = ctypes.c_int
        constants = source_bytes[1]
        for mode in MODES:
            expected = data[6 + mode * RECORD_SIZE:6 + (mode + 1) * RECORD_SIZE]
            memory = bytearray(audit.helper.RAM_SIZE)
            at = audit.helper.physical(seg, 0x100)
            memory[at:at + len(com)] = com
            audit.put16(memory, seg, 0x1293, seg)
            audit.put16(memory, seg, 0x12A5, seg)
            start = audit.helper.physical(seg, FRAME)
            memory[start:start + FRAME_BYTES] = bytes(FRAME_BYTES)
            for i, word in enumerate((ret_ip, seg, 0xA100, 0xA101, 0xA102,
                                      0xA103, 0xA104)):
                audit.put16(memory, seg, 0xEFF2 + 2 * i, word)
            audit.put16(memory, seg, 0x5D92, 0x2200)
            audit.put16(memory, seg, 0x5D94, seg)
            memory[audit.helper.physical(seg, 0x229F)] = 2 if mode == 0 else 1
            if mode == 1:
                string = constants[:8]
            elif mode == 2:
                string = constants[8:14]
            elif mode == 3:
                string = bytearray(constants[:8])
                string[-1] ^= 0x20
            else:
                string = b"\x00"
            at = audit.helper.physical(seg, 0x2200)
            memory[at:at + len(string)] = string
            memory[audit.helper.physical(seg, 0x6822)] = 0xA6
            cpu = audit.helper.Cpu(0xA5CD, 0x3456, 0x5678, 0x789A,
                                   0x9ABC, 0xBCDE, 0x7777, 0xEFF2,
                                   seg, 0x1276, seg, seg, seg, 0x0402)
            ram = (ctypes.c_uint8 * audit.helper.RAM_SIZE).from_buffer(memory)
            assert model(ram, audit.helper.RAM_SIZE, ctypes.byref(cpu)) == 0
            observed = struct.unpack_from("<14H", expected)
            for (name, _), word in zip(audit.helper.Cpu._fields_, observed):
                assert getattr(cpu, name) == word, (mode, name,
                                                    hex(getattr(cpu, name)),
                                                    hex(word))
            assert memory[start:start + FRAME_BYTES] == expected[28:28 + FRAME_BYTES], mode
            assert memory[audit.helper.physical(seg, 0x6822)] == expected[-1], mode


def main():
    directory = ROOT / "resources/u38-1276-parity/native"
    directory.mkdir(parents=True, exist_ok=True)
    source_bytes = audit.source()
    com = build(directory, source_bytes)
    data = run_native(directory)
    compare(data, com.read_bytes(), source_bytes)
    report = dict(target="OVL_0038_1276", original_sha256=audit.BODY_SHA,
                  system_sha256=audit.SYSTEM_SHA, native_cases=len(MODES),
                  actual_original_system_compare=True,
                  reached_game_caller=False, compared_cpu_words=14,
                  compared_frame_bytes=FRAME_BYTES, compared_ds_6822=True,
                  result_sha256=sha256(data).hexdigest())
    (directory / "report.json").write_text(json.dumps(report, sort_keys=True,
                                                       indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
