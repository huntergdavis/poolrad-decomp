#!/usr/bin/env python3
"""Synthetic DOSBox-X far-call frames for original U38:+27D9."""

import ctypes
from hashlib import sha256
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile

import check_overlay_u38_roll_effect_27d9 as audit

ROOT = Path(__file__).resolve().parents[2]
MODES = ((0, 0), (0, 0xFF), (1, 7), (0x10, 7))
FRAME = 0xEFD0
FRAME_BYTES = 52
RECORD_SIZE = 28 + FRAME_BYTES + 2


def build(directory, wrapper):
    wrapper_file = directory / "wrapper.bin"
    wrapper_file.write_bytes(wrapper)
    asm = [
        ".intel_syntax noprefix", ".code16", ".global _start",
        "_start: jmp main", ".org 0x0048", "roll_stub:",
        "push bp", "mov bp,sp", "mov al,byte ptr [0x6900]",
        "mov sp,bp", "pop bp", "retf 4",
        ".org 0x0152", "effect_stub:",
        "push bp", "mov bp,sp", "mov byte ptr [0x6818],0x5a",
        "mov ax,0xbeef", "mov sp,bp", "pop bp", "retf 0xc",
        ".org 0x26d9", '.incbin "' + str(wrapper_file) + '"',
        ".org 0x3100", "main:",
        "mov ax,cs", "mov ds,ax", "mov es,ax", "mov ss,ax",
        "mov sp,0xf000", "mov bx,ax", "add bx,0x10",
        "mov word ptr [0x2800],bx", "add bx,0x10",
        "mov word ptr [0x280e],bx",
        "mov word ptr [outer_segment],ax",
        "mov word ptr [header],ax",
        "mov word ptr [header+2],offset returned",
        f"mov word ptr [header+4],{len(MODES)}",
        "mov dx,offset filename", "xor cx,cx", "mov ah,0x3c",
        "int 0x21", "jc failure", "mov word ptr [handle],ax",
        "mov bx,ax", "mov dx,offset header", "mov cx,6",
        "mov ah,0x40", "int 0x21", "again:",
        "cli", "mov sp,0xf000", f"mov di,{FRAME}",
        f"mov cx,{FRAME_BYTES}", "xor ax,ax", "rep stosb",
        "mov si,word ptr [index]",
        "mov byte ptr [0x6819],0", "mov byte ptr [0x6900],0",
        "cmp si,1", "jne not_ff", "mov byte ptr [0x6900],0xff",
        "not_ff:", "cmp si,2", "jne not_one",
        "mov byte ptr [0x6819],1", "not_one:",
        "cmp si,3", "jne ready",
        "mov byte ptr [0x6819],0x10", "ready:",
        "mov byte ptr [0x6818],0xb8",
        "push 0x0002", "popf",
        "mov ax,0xa5cd", "mov bx,0x3456", "mov cx,0x5678",
        "mov dx,0x789a", "mov si,0x9abc", "mov di,0xbcde",
        "mov bp,0x7777", "push 0xa104", "push 0xa103",
        "push 0xa102", "push 0xa101", "push 0xa100",
        ".byte 0x9a", ".word 0x27d9",
        "outer_segment: .word 0", "returned:",
    ]
    for i, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        asm.append(f"mov word ptr [output+{2*i}],{reg}")
    asm += ["mov word ptr [output+18],offset returned",
            "mov word ptr [output+20],ds",
            "mov word ptr [output+22],es",
            "mov word ptr [output+24],ss"]
    for i in range(FRAME_BYTES):
        asm += [f"mov al,byte ptr [{FRAME + i}]",
                f"mov byte ptr [output+{28 + i}],al"]
    for i, off in enumerate((0x6819, 0x6818)):
        asm += [f"mov al,byte ptr [{off}]",
                f"mov byte ptr [output+{28 + FRAME_BYTES + i}],al"]
    asm += ["pushf", "pop ax", "mov word ptr [output+26],ax",
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


def compare(data, com):
    assert len(data) == 6 + len(MODES) * RECORD_SIZE
    seg, ret_ip, count = struct.unpack_from("<3H", data)
    assert count == len(MODES)
    callback_type = ctypes.CFUNCTYPE(
        ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8),
        ctypes.c_size_t, ctypes.POINTER(audit.helper.Cpu))
    with tempfile.TemporaryDirectory(prefix="u38-27d9-native-") as tmp:
        library = Path(tmp) / "model.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/overlay_u38_roll_effect_27d9.c"),
                        "-o", str(library)], check=True)
        model = ctypes.CDLL(str(library)).pr_u38_roll_effect_27d9
        model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(audit.helper.Cpu), callback_type,
                          callback_type, ctypes.c_void_p]
        model.restype = ctypes.c_int
        for index, (flag, roll_value) in enumerate(MODES):
            expected = data[6 + index * RECORD_SIZE:6 + (index + 1) * RECORD_SIZE]
            memory = bytearray(audit.helper.RAM_SIZE)
            at = audit.helper.physical(seg, 0x100)
            memory[at:at + len(com)] = com
            memory[audit.helper.physical(seg, FRAME):
                   audit.helper.physical(seg, FRAME) + FRAME_BYTES] = bytes(FRAME_BYTES)
            loaded_roll = (seg + 0x10) & 0xFFFF
            loaded_effect = (seg + 0x20) & 0xFFFF
            audit.helper.put16(memory, seg, 0x2800, loaded_roll)
            audit.helper.put16(memory, seg, 0x280E, loaded_effect)
            memory[audit.helper.physical(seg, 0x6819)] = flag
            memory[audit.helper.physical(seg, 0x6818)] = 0xB8
            memory[audit.helper.physical(seg, 0x6900)] = roll_value
            for i, word in enumerate((ret_ip, seg, 0xA100, 0xA101, 0xA102,
                                      0xA103, 0xA104)):
                audit.helper.put16(memory, seg, 0xEFF2 + 2 * i, word)
            cpu = audit.helper.Cpu(0xA5CD, 0x3456, 0x5678, 0x789A,
                                   0x9ABC, 0xBCDE, 0x7777, 0xEFF2,
                                   seg, 0x27d9, seg, seg, seg, 0x0002)
            ram = (ctypes.c_uint8 * audit.helper.RAM_SIZE).from_buffer(memory)
            calls = [0, 0]

            @callback_type
            def roll(context, ptr, size, state_ptr):
                del context
                assert size == audit.helper.RAM_SIZE
                state = state_ptr.contents
                calls[0] += 1
                assert (state.cs, state.ip) == (loaded_roll, 0x0048)
                assert audit.helper.get16(ptr, state.ss, state.sp) == 0x2802
                assert audit.helper.get16(ptr, state.ss, state.sp + 2) == seg
                assert audit.helper.get16(ptr, state.ss, state.sp + 4) == 0xA506
                assert audit.helper.get16(ptr, state.ss, state.sp + 6) == 0xA503
                assert audit.helper.get16(ptr, state.ss, state.sp + 8) == 0xA566
                audit.roll_stub(ptr, state)
                return 1

            @callback_type
            def effect(context, ptr, size, state_ptr):
                del context
                assert size == audit.helper.RAM_SIZE
                state = state_ptr.contents
                calls[1] += 1
                assert (state.cs, state.ip) == (loaded_effect, 0x0052)
                assert audit.helper.get16(ptr, state.ss, state.sp) == 0x2810
                assert audit.helper.get16(ptr, state.ss, state.sp + 2) == seg
                assert audit.helper.get16(ptr, state.ss, state.sp + 4) == 1
                assert audit.helper.get16(ptr, state.ss, state.sp + 6) == 0x00FF
                assert audit.helper.get16(ptr, state.ss, state.sp + 8) == roll_value
                audit.effect_stub(ptr, state)
                return 1

            assert model(ram, audit.helper.RAM_SIZE, ctypes.byref(cpu),
                         roll, effect, None) == 0
            assert calls == ([1, 1] if flag == 0 else [0, 0])
            observed = struct.unpack_from("<14H", expected)
            for (name, _), word in zip(audit.helper.Cpu._fields_, observed):
                assert getattr(cpu, name) == word, (index, name,
                                                    hex(getattr(cpu, name)),
                                                    hex(word))
            start = audit.helper.physical(seg, FRAME)
            actual_frame = memory[start:start + FRAME_BYTES]
            assert actual_frame == expected[28:28 + FRAME_BYTES], (
                index, actual_frame.hex(), expected[28:28 + FRAME_BYTES].hex())
            for i, off in enumerate((0x6819, 0x6818)):
                assert memory[audit.helper.physical(seg, off)] == expected[-2 + i], index


def main():
    directory = ROOT / "resources/u38-27d9-parity/native"
    directory.mkdir(parents=True, exist_ok=True)
    wrapper = audit.source()
    com = build(directory, wrapper)
    data = run_native(directory)
    compare(data, com.read_bytes())
    report = dict(target="OVL_0038_27D9", original_body_sha256=audit.BODY_SHA,
                  native_cases=len(MODES), actual_far_entry_and_two_far_calls=True,
                  both_loaded_segments_relocated_to_distinct_synthetic_stubs=True,
                  reached_game_caller=False, compared_cpu_words=14,
                  compared_frame_bytes=FRAME_BYTES,
                  compared_ds_6819_6818=True,
                  result_sha256=sha256(data).hexdigest())
    (directory / "report.json").write_text(json.dumps(report, sort_keys=True,
                                                       indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
