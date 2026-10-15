#!/usr/bin/env python3
"""Synthetic DOSBox-X far-call frames for original U38:+1086."""

import ctypes
from hashlib import sha256
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile

import check_overlay_u38_target_dispatch_1086 as audit

ROOT = Path(__file__).resolve().parents[2]
MODES = (0, 1, 2, 3)  # null; marker 1, 0, 0x80
FRAME = 0xEFD0
FRAME_BYTES = 52
RECORD_SIZE = 29 + FRAME_BYTES


def build(directory, wrapper, callee):
    wrapper_file = directory / "wrapper.bin"
    callee_file = directory / "callee.bin"
    wrapper_file.write_bytes(wrapper)
    callee_file.write_bytes(callee)
    asm = [
        ".intel_syntax noprefix", ".code16", ".global _start",
        "_start: jmp main", ".org 0x0eac",
        "dispatch_stub:", "push bp", "mov bp,sp",
        "mov byte ptr [0x6818],0x5a", "mov ax,0xbeef",
        "mov sp,bp", "pop bp", "retf 6",
        ".org 0x0f1d",
        '.incbin "' + str(callee_file) + '"',
        ".org 0x0f86", '.incbin "' + str(wrapper_file) + '"',
        ".org 0x2d8b", "stub:",
        "push bp", "mov bp,sp", "les di,[bp+6]",
        "mov word ptr es:[di],0x2200", "mov ax,cs",
        "mov word ptr es:[di+2],ax", "mov al,1",
        "mov sp,bp", "pop bp", "retf 8",
        ".org 0x3100", "main:",
        "mov ax,cs", "mov ds,ax", "mov es,ax", "mov ss,ax",
        "mov sp,0xf000", "mov word ptr [0x1048],ax",
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
        "mov word ptr [0x40cc],0", "mov word ptr [0x40ce],0",
        "or si,si", "je no_link",
        "mov word ptr [0x40cc],0x2200", "mov ax,cs",
        "mov word ptr [0x40ce],ax", "no_link:",
        "mov byte ptr [0x222e],7", "mov byte ptr [0x5600],0",
        "mov byte ptr [0x2232],1",
        "cmp si,2", "jne not_zero", "mov byte ptr [0x2232],0",
        "not_zero:", "cmp si,3", "jne flag_ready",
        "mov byte ptr [0x2232],0x80", "flag_ready:",
        "mov byte ptr [0x6818],5",
        "mov word ptr [0x5d92],0x4000", "mov ax,cs",
        "mov word ptr [0x5d94],ax",
        "push 0x0002", "popf",
        "mov ax,0xa5cd", "mov bx,0x3456", "mov cx,0x5678",
        "mov dx,0x789a", "mov si,0x9abc", "mov di,0xbcde",
        "mov bp,0x7777", "push 0xa104", "push 0xa103",
        "push 0xa102", "push 0xa101", "push 0xa100",
        ".byte 0x9a", ".word 0x1086",
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
    asm += ["mov al,byte ptr [0x6818]",
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


def compare(data, com):
    assert len(data) == 6 + len(MODES) * RECORD_SIZE
    seg, ret_ip, count = struct.unpack_from("<3H", data)
    assert count == len(MODES)
    callback_type = ctypes.CFUNCTYPE(
        ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8),
        ctypes.c_size_t, ctypes.POINTER(audit.helper.Cpu))
    with tempfile.TemporaryDirectory(prefix="u38-1086-native-") as tmp:
        library = Path(tmp) / "model.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/overlay_u38_target_select_101d.c"),
                        str(ROOT / "src/recovered/overlay_u38_target_dispatch_1086.c"),
                        "-o", str(library)], check=True)
        model = ctypes.CDLL(str(library)).pr_u38_target_dispatch_1086
        model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(audit.helper.Cpu), callback_type,
                          callback_type, ctypes.c_void_p]
        model.restype = ctypes.c_int
        for index, mode in enumerate(MODES):
            expected = data[6 + index * RECORD_SIZE:6 + (index + 1) * RECORD_SIZE]
            memory = bytearray(audit.helper.RAM_SIZE)
            at = audit.helper.physical(seg, 0x100)
            memory[at:at + len(com)] = com
            memory[audit.helper.physical(seg, FRAME):
                   audit.helper.physical(seg, FRAME) + FRAME_BYTES] = bytes(FRAME_BYTES)
            audit.helper.put16(memory, seg, 0x1048, seg)
            audit.helper.put16(memory, seg, 0x5D92, 0x4000)
            audit.helper.put16(memory, seg, 0x5D94, seg)
            if index:
                audit.helper.put16(memory, seg, 0x40CC, 0x2200)
                audit.helper.put16(memory, seg, 0x40CE, seg)
            memory[audit.helper.physical(seg, 0x222E)] = 7
            memory[audit.helper.physical(seg, 0x5600)] = 0
            memory[audit.helper.physical(seg, 0x2232)] = (
                0 if mode == 2 else (0x80 if mode == 3 else 1))
            memory[audit.helper.physical(seg, 0x6818)] = 5
            for i, word in enumerate((ret_ip, seg, 0xA100, 0xA101, 0xA102,
                                      0xA103, 0xA104)):
                audit.helper.put16(memory, seg, 0xEFF2 + 2 * i, word)
            cpu = audit.helper.Cpu(0xA5CD, 0x3456, 0x5678, 0x789A,
                                   0x9ABC, 0xBCDE, 0x7777, 0xEFF2,
                                   seg, 0x1086, seg, seg, seg, 0x0002)
            ram = (ctypes.c_uint8 * audit.helper.RAM_SIZE).from_buffer(memory)
            calls = [0, 0]

            @callback_type
            def stub(context, ptr, size, state_ptr):
                del context
                assert size == audit.helper.RAM_SIZE
                state = state_ptr.contents
                calls[0] += 1
                assert state.cs == seg and state.ip == 0x2E8B
                assert audit.helper.get16(ptr, state.ss, state.sp) == 0x104A
                assert audit.helper.get16(ptr, state.ss, state.sp + 2) == seg
                output_off = audit.helper.get16(ptr, state.ss, state.sp + 4)
                output_seg = audit.helper.get16(ptr, state.ss, state.sp + 6)
                assert audit.helper.get16(ptr, state.ss, state.sp + 8) == 0x4000
                assert audit.helper.get16(ptr, state.ss, state.sp + 10) == seg
                audit.helper.put16(ptr, state.ss, state.sp - 2, state.bp)
                state.es, state.di = output_seg, output_off
                audit.helper.put16(ptr, output_seg, output_off, 0x2200)
                audit.helper.put16(ptr, output_seg, output_off + 2, seg)
                state.ax = (seg & 0xFF00) | 1
                return 1

            @callback_type
            def dispatch(context, ptr, size, state_ptr):
                del context
                assert size == audit.helper.RAM_SIZE
                state = state_ptr.contents
                calls[1] += 1
                assert state.cs == seg and state.ip == 0x0FAC
                assert audit.helper.get16(ptr, state.ss, state.sp) == 0x10BD
                assert audit.helper.get16(ptr, state.ss, state.sp + 2) == seg
                assert audit.helper.get16(ptr, state.ss, state.sp + 4) == 0x2264
                assert audit.helper.get16(ptr, state.ss, state.sp + 6) == 0xA103
                assert audit.helper.get16(ptr, state.ss, state.sp + 8) == 0xA104
                audit.helper.put16(ptr, state.ss, state.sp - 2, state.bp)
                ptr[audit.helper.physical(state.ds, 0x6818)] = 0x5A
                state.ax = 0xBEEF
                return 1

            assert model(ram, audit.helper.RAM_SIZE, ctypes.byref(cpu),
                         stub, dispatch, None) == 0
            assert calls == [2 * (index != 0), int(mode == 2)]
            observed = struct.unpack_from("<14H", expected)
            for (name, _), word in zip(audit.helper.Cpu._fields_, observed):
                assert getattr(cpu, name) == word, (index, name,
                                                    hex(getattr(cpu, name)),
                                                    hex(word))
            start = audit.helper.physical(seg, FRAME)
            actual_frame = memory[start:start + FRAME_BYTES]
            assert actual_frame == expected[28:28 + FRAME_BYTES], (
                index, actual_frame.hex(), expected[28:28 + FRAME_BYTES].hex())
            assert memory[audit.helper.physical(seg, 0x6818)] == expected[28 + FRAME_BYTES]


def main():
    directory = ROOT / "resources/u38-1086-parity/native"
    directory.mkdir(parents=True, exist_ok=True)
    wrapper, callee = audit.source(), audit.helper.source()
    com = build(directory, wrapper, callee)
    data = run_native(directory)
    compare(data, com.read_bytes())
    report = dict(target="OVL_0038_1086", original_body_sha256=audit.BODY_SHA,
                  native_cases=len(MODES), actual_far_entry_and_near_calls=True,
                  resident_target_relocated_to_synthetic_stub=True,
                  dependency_0fac_relocated_to_synthetic_stub=True,
                  reached_game_caller=False, compared_cpu_words=14,
                  compared_frame_bytes=FRAME_BYTES,
                  compared_effect_byte=True,
                  result_sha256=sha256(data).hexdigest())
    (directory / "report.json").write_text(json.dumps(report, sort_keys=True,
                                                       indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
