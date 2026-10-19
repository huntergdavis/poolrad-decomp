#!/usr/bin/env python3
"""Synthetic DOSBox-X U38:+2B20 -> U9E entry-9 far-call frames."""

import ctypes
from hashlib import sha256
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile

import check_overlay_u38_link_roll_2b20 as audit

ROOT = Path(__file__).resolve().parents[2]
MODES = (0, 1, 2, 3, 4)
ROLL_VALUES = (0, 0, 0, 15, 255)
FRAME = 0xEFD0
FRAME_BYTES = 52
RECORD_SIZE = 28 + FRAME_BYTES + 2


def build(directory, wrapper, callee):
    wrapper_file = directory / "wrapper.bin"
    callee_file = directory / "callee.bin"
    wrapper_file.write_bytes(wrapper)
    callee_file.write_bytes(callee)
    asm = [
        ".intel_syntax noprefix", ".code16", ".global _start",
        "_start: jmp main", ".org 0x2a20",
        '.incbin "' + str(wrapper_file) + '"',
        ".org 0x3200", "main:",
        "mov ax,cs", "mov ds,ax", "mov es,ax", "mov ss,ax",
        "mov sp,0xf000", "mov word ptr [outer_segment],ax",
        "mov word ptr [header],ax", "add ax,0x700",
        "mov word ptr [0x2b55],ax", "mov word ptr [header+2],ax",
        "mov word ptr [header+4],offset returned",
        f"mov word ptr [header+6],{len(MODES)}",
        "mov dx,offset filename", "xor cx,cx", "mov ah,0x3c",
        "int 0x21", "jc failure", "mov word ptr [handle],ax",
        "mov bx,ax", "mov dx,offset header", "mov cx,8",
        "mov ah,0x40", "int 0x21", "again:",
        "cli", "mov sp,0xf000", f"mov di,{FRAME}",
        f"mov cx,{FRAME_BYTES}", "xor ax,ax", "rep stosb",
        "mov si,word ptr [index]",
        "mov word ptr [0x40cc],0", "mov word ptr [0x40ce],0",
        "cmp si,2", "jb no_link",
        "mov word ptr [0x40cc],0x2200", "mov ax,cs",
        "mov word ptr [0x40ce],ax", "no_link:",
        "mov byte ptr [0x222e],0x54", "cmp si,2", "jb wrong_kind",
        "mov byte ptr [0x222e],0x55", "wrong_kind:",
        "mov bx,offset roll_values", "mov al,byte ptr [bx+si]",
        "mov byte ptr [roll_value],al",
        "mov byte ptr [0x6818],0xa7",
        "mov byte ptr [0x681c],0xb8",
        "mov word ptr [0x5d92],0x4000", "mov ax,cs",
        "mov word ptr [0x5d94],ax",
        "push 0x0002", "popf",
        "mov ax,0xa5cd", "mov bx,0x3456", "mov cx,0x5678",
        "mov dx,0x789a", "mov si,0x9abc", "mov di,0xbcde",
        "mov bp,0x7777", "push 0xa104", "push 0xa103",
        "push 0xa102", "push 0xa101", "push 0xa100",
        ".byte 0x9a", ".word 0x2b20",
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
            "mov al,byte ptr [0x681c]",
            f"mov byte ptr [output+{29 + FRAME_BYTES}],al",
            "pushf", "pop ax", "mov word ptr [output+26],ax",
            "sti", "mov bx,word ptr [handle]", "mov dx,offset output",
            f"mov cx,{RECORD_SIZE}", "mov ah,0x40", "int 0x21",
            "jc failure", f"cmp ax,{RECORD_SIZE}", "jne failure",
            "inc word ptr [index]", f"cmp word ptr [index],{len(MODES)}",
            "jb again", "mov bx,word ptr [handle]", "mov ah,0x3e",
            "int 0x21", "mov ax,0x4c00", "int 0x21", "failure:",
            "mov ax,0x4c01", "int 0x21", 'filename: .asciz "RESULT.BIN"',
            "header: .space 8", "handle: .word 0", "index: .word 0",
            "roll_value: .byte 0",
            "roll_values: .byte " + ",".join(str(v) for v in ROLL_VALUES),
            f"output: .space {RECORD_SIZE}",
            ".org 0x6f4d", ".byte 0xe9,0xde,0x0d",
            ".org 0x7ce3", "roll_stub:",
            "push bp", "mov bp,sp", "mov al,byte ptr [roll_value]",
            "mov bx,0xbeef", "mov dx,0xcb67",
            "push 0x0247", "popf", "mov sp,bp", "pop bp", "retf 4",
            ".org 0x7d2e", '.incbin "' + str(callee_file) + '"']
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
    assert len(data) == 8 + len(MODES) * RECORD_SIZE
    seg, target_seg, ret_ip, count = struct.unpack_from("<4H", data)
    assert target_seg == (seg + 0x700) & 0xFFFF
    assert count == len(MODES)
    callback_type = ctypes.CFUNCTYPE(
        ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8),
        ctypes.c_size_t, ctypes.POINTER(audit.latch.Cpu))
    with tempfile.TemporaryDirectory(prefix="u38-2b20-native-") as tmp:
        library = Path(tmp) / "model.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/overlay_u9e_entry9_roll_latch_0e2e.c"),
                        str(ROOT / "src/recovered/overlay_u38_link_roll_2b20.c"),
                        str(ROOT / "src/recovered/system_random_fullstate_11c0.c"),
                        str(ROOT / "src/recovered/system_seed_fullstate_120f.c"),
                        str(ROOT / "src/recovered/overlay_u9e_entry8_fullstate_compose_0de3.c"),
                        str(ROOT / "src/recovered/overlay_u9e_entry8_fullstate_loop_0e05.c"),
                        str(ROOT / "src/recovered/overlay_u9e_entry8_system_call_0e0e.c"),
                        str(ROOT / "src/recovered/overlay_u9e_entry8_fullstate_tail_0e1f.c"),
                        str(ROOT / "src/recovered/overlay_u9e_entry8_roll.c"),
                        str(ROOT / "src/recovered/overlay_u9e_entry8_fullstate_postcall_0e13.c"),
                        str(ROOT / "src/recovered/overlay_u9e_entry8_fullstate_prefix_0de3.c"),
                        "-o", str(library)], check=True)
        so = ctypes.CDLL(str(library))
        model = so.pr_u38_link_roll_2b20
        callee_model = so.pr_u9e_entry9_roll_latch_0e2e
        for fn in (model, callee_model):
            fn.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                           ctypes.POINTER(audit.latch.Cpu), callback_type,
                           ctypes.c_void_p]
            fn.restype = ctypes.c_int
        for index, mode in enumerate(MODES):
            expected = data[8 + index * RECORD_SIZE:
                            8 + (index + 1) * RECORD_SIZE]
            memory = bytearray(audit.latch.RAM_SIZE)
            at = audit.latch.physical(seg, 0x100)
            memory[at:at + len(com)] = com
            start = audit.latch.physical(seg, FRAME)
            memory[start:start + FRAME_BYTES] = bytes(FRAME_BYTES)
            audit.latch.put16(memory, seg, 0x2B55, target_seg)
            audit.latch.put16(memory, seg, 0x5D92, 0x4000)
            audit.latch.put16(memory, seg, 0x5D94, seg)
            if mode >= 2:
                audit.latch.put16(memory, seg, 0x40CC, 0x2200)
                audit.latch.put16(memory, seg, 0x40CE, seg)
            memory[audit.latch.physical(seg, 0x222E)] = 0x55 if mode >= 2 else 0x54
            memory[audit.latch.physical(seg, 0x6818)] = 0xA7
            memory[audit.latch.physical(seg, 0x681C)] = 0xB8
            for i, word in enumerate((ret_ip, seg, 0xA100, 0xA101, 0xA102,
                                      0xA103, 0xA104)):
                audit.latch.put16(memory, seg, 0xEFF2 + 2 * i, word)
            cpu = audit.latch.Cpu(0xA5CD, 0x3456, 0x5678, 0x789A,
                                  0x9ABC, 0xBCDE, 0x7777, 0xEFF2,
                                  seg, 0x2B20, seg, seg, seg, 0x0002)
            ram = (ctypes.c_uint8 * audit.latch.RAM_SIZE).from_buffer(memory)
            calls = [0, 0]

            @callback_type
            def roll(context, ptr, size, state_ptr):
                del context
                assert size == audit.latch.RAM_SIZE
                state = state_ptr.contents
                calls[1] += 1
                assert state.cs == target_seg and state.ip == 0x0DE3
                assert audit.latch.get16(ptr, state.ss, state.sp) == 0x0E46
                assert audit.latch.get16(ptr, state.ss, state.sp + 2) == target_seg
                audit.latch.put16(ptr, state.ss, state.sp - 2, state.bp)
                audit.latch.put16(ptr, state.ss, state.sp - 4, 0x0247)
                state.ax = (state.ax & 0xFF00) | ROLL_VALUES[index]
                state.bx, state.dx, state.flags = 0xBEEF, 0xCB67, 0x0247
                return 1

            @callback_type
            def far(context, ptr, size, state_ptr):
                del context
                assert size == audit.latch.RAM_SIZE
                state = state_ptr.contents
                calls[0] += 1
                assert state.cs == target_seg and state.ip == 0x004D
                assert audit.latch.get16(ptr, state.ss, state.sp) == 0x2B57
                assert audit.latch.get16(ptr, state.ss, state.sp + 2) == seg
                state.ip = 0x0E2E
                assert callee_model(ptr, size, state_ptr, roll, None) == 0
                return 1

            assert model(ram, audit.latch.RAM_SIZE, ctypes.byref(cpu),
                         far, None) == 0
            assert calls == ([0, 0] if mode < 2 else [1, 1])
            observed = struct.unpack_from("<14H", expected)
            for (name, _), word in zip(audit.latch.Cpu._fields_, observed):
                assert getattr(cpu, name) == word, (index, name,
                                                    hex(getattr(cpu, name)),
                                                    hex(word))
            actual_frame = memory[start:start + FRAME_BYTES]
            expected_frame = expected[28:28 + FRAME_BYTES]
            assert actual_frame == expected_frame, (index, [
                (hex(FRAME + i), hex(a), hex(b))
                for i, (a, b) in enumerate(zip(actual_frame, expected_frame))
                if a != b][:12])
            assert (memory[audit.latch.physical(seg, 0x6818)],
                    memory[audit.latch.physical(seg, 0x681C)]) == tuple(expected[-2:])


def main():
    directory = ROOT / "resources/u38-2b20-parity/native"
    directory.mkdir(parents=True, exist_ok=True)
    wrapper, callee = audit.source(), audit.latch.source()
    com = build(directory, wrapper, callee)
    data = run_native(directory)
    compare(data, com.read_bytes())
    report = dict(target="OVL_0038_2B20", original_body_sha256=audit.BODY_SHA,
                  native_cases=len(MODES), actual_far_entry_and_u9e_wrapper=True,
                  u9e_roll_replaced_by_synthetic_stub=True,
                  reached_game_caller=False, compared_cpu_words=14,
                  compared_frame_bytes=FRAME_BYTES,
                  result_sha256=sha256(data).hexdigest())
    (directory / "report.json").write_text(json.dumps(report, sort_keys=True,
                                                       indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
