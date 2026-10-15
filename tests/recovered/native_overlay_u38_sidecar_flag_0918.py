#!/usr/bin/env python3
"""Synthetic DOSBox-X far-call frames for original U38:+0918 wrapper.

The resident call segment is relocated to a small synthetic stub in the COM.
This validates native execution of the original wrapper, not natural game
reachability or the original resident routine. All products stay ignored.
"""

import ctypes
from hashlib import sha256
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile

import check_overlay_u38_sidecar_flag_0918 as audit

ROOT = Path(__file__).resolve().parents[2]
AMOUNTS = (0, 3, 4, 0xFF, 0, 3, 4, 0xFF)
RECORD_SIZE = 66
FRAME = 0xEFEC


def build(directory, body):
    body_file = directory / "body.bin"
    body_file.write_bytes(body)
    asm = [
        ".intel_syntax noprefix", ".code16", ".global _start",
        "_start: jmp main", ".org 0x818",
        '.incbin "' + str(body_file) + '"',
        ".org 0x2099", "stub:",
        "push bp", "mov bp,sp", "push ax", "push es", "push di",
        "les di,[bp+6]", "mov word ptr es:[di],0x2200",
        "mov word ptr es:[di+2],0x9000", "pop di", "pop es",
        "pop ax", "pop bp", "mov al,byte ptr cs:[native_result]",
        "retf 10", ".org 0x2300", "main:",
        "mov ax,cs", "mov ds,ax", "mov es,ax", "mov ss,ax",
        "mov sp,0xf000", "mov word ptr [0x931],ax",
        "mov word ptr [outer_call_segment],ax",
        "mov word ptr [header],ax",
        "mov word ptr [header+2],offset returned",
        f"mov word ptr [header+4],{len(AMOUNTS)}",
        "mov dx,offset filename", "xor cx,cx", "mov ah,0x3c",
        "int 0x21", "jc failure", "mov word ptr [handle],ax",
        "mov bx,ax", "mov dx,offset header", "mov cx,6",
        "mov ah,0x40", "int 0x21", "again:",
        "cli", "mov sp,0xf000", "mov si,word ptr [index]",
        "mov ax,si", "and ax,1", "mov byte ptr [native_result],al",
        "mov al,byte ptr [amounts+si]", "mov byte ptr [0x6822],al",
        "mov byte ptr [0x681e],0xa7",
        "mov word ptr [0x5d92],0x2300", "mov ax,cs",
        "mov word ptr [0x5d94],ax",
        "push 0xa004", "push 0xa003", "push 0xa002",
        "push 0xa001", "push 0xa000",
        "push 0x0002", "popf",
        "mov ax,0xa5cd", "mov bx,0x3456", "mov cx,0x5678",
        "mov dx,0x789a", "mov si,0x9abc", "mov di,0xbcde",
        "mov bp,0x7777", ".byte 0x9a", ".word 0x0918",
        "outer_call_segment: .word 0", "returned:",
    ]
    for i, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        asm.append(f"mov word ptr [output+{2*i}],{reg}" if reg != "cs"
                   else f"mov word ptr [output+{2*i}],cs")
    asm += ["mov word ptr [output+18],offset returned",
            "mov word ptr [output+20],ds",
            "mov word ptr [output+22],es",
            "mov word ptr [output+24],ss"]
    for i in range(16):
        asm += [f"mov ax,word ptr [{FRAME + 2*i}]",
                f"mov word ptr [output+{34 + 2*i}],ax"]
    asm += ["pushf", "pop ax", "mov word ptr [output+26],ax",
            f"mov ax,word ptr [{FRAME}]", "mov word ptr [output+28],ax",
            f"mov ax,word ptr [{FRAME+2}]", "mov word ptr [output+30],ax",
            "mov al,byte ptr [0x681e]", "mov byte ptr [output+32],al",
            "mov al,byte ptr [0x6822]", "mov byte ptr [output+33],al",
            "sti", "mov bx,word ptr [handle]", "mov dx,offset output",
            f"mov cx,{RECORD_SIZE}", "mov ah,0x40", "int 0x21",
            "jc failure", f"cmp ax,{RECORD_SIZE}", "jne failure",
            "inc word ptr [index]", f"cmp word ptr [index],{len(AMOUNTS)}",
            "jb again", "mov bx,word ptr [handle]", "mov ah,0x3e",
            "int 0x21", "mov ax,0x4c00", "int 0x21", "failure:",
            "mov ax,0x4c01", "int 0x21", 'filename: .asciz "RESULT.BIN"',
            "header: .space 6", "handle: .word 0", "index: .word 0",
            "native_result: .byte 0", "amounts: .byte 0,3,4,255,0,3,4,255",
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
    assert len(data) == 6 + len(AMOUNTS) * RECORD_SIZE
    seg, ret_ip, count = struct.unpack_from("<3H", data)
    assert count == len(AMOUNTS)
    callback_type = ctypes.CFUNCTYPE(
        ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8),
        ctypes.c_size_t, ctypes.POINTER(audit.Cpu))
    with tempfile.TemporaryDirectory(prefix="u38-native-") as tmp:
        library = Path(tmp) / "model.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/overlay_u38_sidecar_flag_0918.c"),
                        "-o", str(library)], check=True)
        model = ctypes.CDLL(str(library)).pr_u38_sidecar_flag_0918
        model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(audit.Cpu), callback_type, ctypes.c_void_p]
        model.restype = ctypes.c_int
        for index, amount in enumerate(AMOUNTS):
            expected = data[6 + index * RECORD_SIZE:6 + (index + 1) * RECORD_SIZE]
            memory = bytearray(audit.RAM_SIZE)
            at = audit.physical(seg, 0x100)
            memory[at:at + len(com)] = com
            audit.put16(memory, seg, 0x931, seg)
            audit.put16(memory, seg, 0x5D92, 0x2300)
            audit.put16(memory, seg, 0x5D94, seg)
            memory[audit.physical(seg, 0x681E)] = 0xA7
            memory[audit.physical(seg, 0x6822)] = amount
            for i, word in enumerate((ret_ip, seg, 0xA000, 0xA001,
                                      0xA002, 0xA003, 0xA004)):
                audit.put16(memory, seg, 0xEFF2 + 2 * i, word)
            cpu = audit.Cpu(0xA5CD, 0x3456, 0x5678, 0x789A, 0x9ABC,
                            0xBCDE, 0x7777, 0xEFF2, seg, 0x0918,
                            seg, seg, seg, 0x0002)
            ram = (ctypes.c_uint8 * audit.RAM_SIZE).from_buffer(memory)

            @callback_type
            def stub(context, ptr, size, state_ptr):
                del context
                assert size == audit.RAM_SIZE
                state = state_ptr.contents
                audit.put16(ptr, state.ss, state.bp - 4, 0x2200)
                audit.put16(ptr, state.ss, state.bp - 2, 0x9000)
                state.ax = (state.ax & 0xFF00) | (index & 1)
                return 1

            assert model(ram, audit.RAM_SIZE, ctypes.byref(cpu), stub, None) == 0
            observed = struct.unpack_from("<14H", expected)
            for (name, _), word in zip(audit.Cpu._fields_, observed):
                assert getattr(cpu, name) == word, (index, name,
                                                    hex(getattr(cpu, name)), hex(word))
            assert memory[audit.physical(seg, FRAME):audit.physical(seg, FRAME)+32] == expected[34:66]
            assert audit.get16(memory, seg, FRAME) == struct.unpack_from("<H", expected, 28)[0]
            assert audit.get16(memory, seg, FRAME + 2) == struct.unpack_from("<H", expected, 30)[0]
            assert memory[audit.physical(seg, 0x681E)] == expected[32]
            assert memory[audit.physical(seg, 0x6822)] == expected[33]


def main():
    directory = ROOT / "resources/u38-0918-parity/native"
    directory.mkdir(parents=True, exist_ok=True)
    body = audit.source()
    com = build(directory, body)
    data = run_native(directory)
    compare(data, com.read_bytes())
    report = dict(target="OVL_0038_0918", original_body_sha256=audit.BODY_SHA,
                  native_cases=len(AMOUNTS), actual_far_call=True,
                  resident_target_relocated_to_synthetic_stub=True,
                  reached_game_caller=False, compared_cpu_words=14,
                  compared_frame_bytes=32,
                  result_sha256=sha256(data).hexdigest())
    (directory / "report.json").write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
