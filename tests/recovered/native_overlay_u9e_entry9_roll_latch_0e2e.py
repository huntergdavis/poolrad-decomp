#!/usr/bin/env python3
"""Synthetic DOSBox-X near-call frames for original U9E:+0E2E wrapper."""

import ctypes
from hashlib import sha256
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile

import check_overlay_u9e_entry9_roll_latch_0e2e as audit

ROOT = Path(__file__).resolve().parents[2]
PAIRS = ((0, 0), (1, 6), (3, 0), (3, 6),
         (6, 255), (255, 0), (255, 1), (255, 6))
FRAME = 0xEFEA
FRAME_BYTES = 22
RECORD_SIZE = 28 + 1 + FRAME_BYTES


def build(directory, body):
    body_file = directory / "body.bin"
    body_file.write_bytes(body)
    asm = [
        ".intel_syntax noprefix", ".code16", ".global _start",
        "_start: jmp main", ".org 0x0ce3", "stub:",
        "push bp", "mov bp,sp", "mov al,byte ptr [bp+8]",
        "mov sp,bp", "pop bp", "retf 4",
        ".org 0x0d2e", '.incbin "' + str(body_file) + '"',
        ".org 0x2300", "main:",
        "mov ax,cs", "mov ds,ax", "mov es,ax", "mov ss,ax",
        "mov sp,0xf000", "mov word ptr [outer_segment],ax",
        "mov word ptr [header],ax",
        "mov word ptr [header+2],offset returned",
        f"mov word ptr [header+4],{len(PAIRS)}",
        "mov dx,offset filename", "xor cx,cx", "mov ah,0x3c",
        "int 0x21", "jc failure", "mov word ptr [handle],ax",
        "mov bx,ax", "mov dx,offset header", "mov cx,6",
        "mov ah,0x40", "int 0x21", "again:",
        "cli", "mov sp,0xf000", "mov di,0xefea", "mov cx,22",
        "xor ax,ax", "rep stosb", "mov byte ptr [0x681c],0xa7",
        "mov bx,word ptr [index]", "shl bx,1",
        "mov al,byte ptr [pairs+bx]", "xor ah,ah", "push ax",
        "mov al,byte ptr [pairs+bx+1]", "xor ah,ah", "push ax",
        "push 0x0002", "popf",
        "mov ax,0xa5cd", "mov bx,0x3456", "mov cx,0x5678",
        "mov dx,0x789a", "mov si,0x9abc", "mov di,0xbcde",
        "mov bp,0x7777", ".byte 0x9a", ".word 0x0e2e",
        "outer_segment: .word 0", "returned:",
    ]
    for i, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        asm.append(f"mov word ptr [output+{2*i}],{reg}")
    asm += ["mov word ptr [output+18],offset returned",
            "mov word ptr [output+20],ds",
            "mov word ptr [output+22],es",
            "mov word ptr [output+24],ss",
            "mov al,byte ptr [0x681c]", "mov byte ptr [output+28],al"]
    for i in range(FRAME_BYTES):
        asm += [f"mov al,byte ptr [{FRAME + i}]",
                f"mov byte ptr [output+{29 + i}],al"]
    asm += ["pushf", "pop ax", "mov word ptr [output+26],ax",
            "sti", "mov bx,word ptr [handle]", "mov dx,offset output",
            f"mov cx,{RECORD_SIZE}", "mov ah,0x40", "int 0x21",
            "jc failure", f"cmp ax,{RECORD_SIZE}", "jne failure",
            "inc word ptr [index]", f"cmp word ptr [index],{len(PAIRS)}",
            "jb again", "mov bx,word ptr [handle]", "mov ah,0x3e",
            "int 0x21", "mov ax,0x4c00", "int 0x21", "failure:",
            "mov ax,0x4c01", "int 0x21", 'filename: .asciz "RESULT.BIN"',
            "header: .space 6", "handle: .word 0", "index: .word 0",
            "pairs: .byte " + ",".join(str(v) for pair in PAIRS for v in pair),
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


def compare(data, com):
    assert len(data) == 6 + len(PAIRS) * RECORD_SIZE
    seg, ret_ip, count = struct.unpack_from("<3H", data)
    assert count == len(PAIRS)
    callback_type = ctypes.CFUNCTYPE(
        ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8),
        ctypes.c_size_t, ctypes.POINTER(audit.Cpu))
    with tempfile.TemporaryDirectory(prefix="u9e-native-") as tmp:
        library = Path(tmp) / "model.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/overlay_u9e_entry9_roll_latch_0e2e.c"),
                        "-o", str(library)], check=True)
        model = ctypes.CDLL(str(library)).pr_u9e_entry9_roll_latch_0e2e
        model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(audit.Cpu), callback_type, ctypes.c_void_p]
        model.restype = ctypes.c_int
        for index, (count_word, range_word) in enumerate(PAIRS):
            expected = data[6 + index * RECORD_SIZE:6 + (index + 1) * RECORD_SIZE]
            memory = bytearray(audit.RAM_SIZE)
            at = audit.physical(seg, 0x100)
            memory[at:at + len(com)] = com
            memory[audit.physical(seg, 0x681C)] = 0xA7
            for i, word in enumerate((ret_ip, seg, range_word, count_word)):
                audit.put16(memory, seg, 0xEFF8 + 2 * i, word)
            cpu = audit.Cpu(0xA5CD, 0x3456, 0x5678, 0x789A, 0x9ABC,
                            0xBCDE, 0x7777, 0xEFF8, seg, 0x0E2E,
                            seg, seg, seg, 0x0002)
            ram = (ctypes.c_uint8 * audit.RAM_SIZE).from_buffer(memory)

            @callback_type
            def stub(context, ptr, size, state_ptr):
                del context
                assert size == audit.RAM_SIZE
                state = state_ptr.contents
                assert audit.get16(ptr, state.ss, state.sp) == 0x0E46
                assert audit.get16(ptr, state.ss, state.sp + 2) == seg
                assert audit.get16(ptr, state.ss, state.sp + 4) == (0xA500 | range_word)
                assert audit.get16(ptr, state.ss, state.sp + 6) == (0xA500 | count_word)
                audit.put16(ptr, state.ss, state.sp - 2, state.bp)
                state.ax = (state.ax & 0xFF00) | count_word
                return 1

            assert model(ram, audit.RAM_SIZE, ctypes.byref(cpu), stub, None) == 0
            observed = struct.unpack_from("<14H", expected)
            for (name, _), word in zip(audit.Cpu._fields_, observed):
                assert getattr(cpu, name) == word, (index, name,
                                                    hex(getattr(cpu, name)), hex(word))
            assert memory[audit.physical(seg, 0x681C)] == expected[28]
            start = audit.physical(seg, FRAME)
            actual_frame = memory[start:start + FRAME_BYTES]
            expected_frame = expected[29:]
            assert actual_frame == expected_frame, (index, actual_frame.hex(),
                                                    expected_frame.hex())


def main():
    directory = ROOT / "resources/u9e-0e2e-parity/native"
    directory.mkdir(parents=True, exist_ok=True)
    body = audit.source()
    com = build(directory, body)
    data = run_native(directory)
    compare(data, com.read_bytes())
    report = dict(target="OVL_009E_0E2E", original_body_sha256=audit.BODY_SHA,
                  native_cases=len(PAIRS), actual_far_entry_and_near_call=True,
                  callee_replaced_by_synthetic_stub=True,
                  reached_game_caller=False, compared_cpu_words=14,
                  compared_frame_bytes=FRAME_BYTES,
                  result_sha256=sha256(data).hexdigest())
    (directory / "report.json").write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
