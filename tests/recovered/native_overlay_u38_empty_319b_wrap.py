#!/usr/bin/env python3
"""DOSBox-X original-byte RETF offset-wrap parity for U38:+319B."""

import ctypes
from hashlib import sha256
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile

import check_overlay_u38_empty_319b as audit

ROOT = Path(__file__).resolve().parents[2]
CASES = ((0xFFFE, 0xBEEF), (0xFFFF, 0x1234))
FRAME = 0xFFE8
FRAME_BYTES = 52
RECORD_SIZE = 28 + FRAME_BYTES


def build(directory, body):
    body_file = directory / "entry.bin"
    body_file.write_bytes(body)
    asm = [
        ".intel_syntax noprefix", ".code16", ".global _start",
        "_start: jmp main", ".org 0x309b",
        '.incbin "' + str(body_file) + '"',
        ".org 0x3100", "main:",
        "mov ax,cs", "mov ds,ax", "mov es,ax", "mov ss,ax",
        "mov sp,0x8000", "mov word ptr [header],ax",
        "mov word ptr [header+4],offset returned",
        f"mov word ptr [header+6],{len(CASES)}",
        "add ax,0x700", "mov word ptr [header+2],ax",
        "mov ss,ax", "mov sp,0x8000",
        "mov dx,offset filename", "xor cx,cx",
        "mov ah,0x3c", "int 0x21", "jc failure",
        "mov word ptr [handle],ax", "mov bx,ax",
        "mov dx,offset header", "mov cx,8", "mov ah,0x40",
        "int 0x21", "again:",
        "cli", "mov sp,0x8000",
        "mov ax,word ptr [header+2]", "mov es,ax",
        f"mov di,{FRAME}", f"mov cx,{FRAME_BYTES}",
        "xor ax,ax", "cld", "rep stosb", "mov ax,cs", "mov es,ax",
        "mov si,word ptr [index]", "shl si,1",
        "mov bx,offset frame_sps", "mov di,word ptr [bx+si]",
        "mov bx,offset bp_values", "mov bp,word ptr [bx+si]",
        "mov ax,offset returned", "mov word ptr ss:[di],ax",
        "mov bx,di", "add bx,2", "mov ax,cs",
        "mov word ptr ss:[bx],ax",
        "mov bx,di", "add bx,4",
        "mov word ptr ss:[bx],0xa100", "add bx,2",
        "mov word ptr ss:[bx],0xa101", "add bx,2",
        "mov word ptr ss:[bx],0xa102", "add bx,2",
        "mov word ptr ss:[bx],0xa103", "add bx,2",
        "mov word ptr ss:[bx],0xa104",
        "mov sp,di", "push 0x0002", "popf",
        "mov ax,0xa5cd", "mov bx,0x3456", "mov cx,0x5678",
        "mov dx,0x789a", "mov si,0x9abc", "mov di,0xbcde",
        "push cs", "push 0x319b", "retf",
        "returned:",
    ]
    for i, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        asm.append(f"mov word ptr [output+{2*i}],{reg}")
    asm += ["mov word ptr [output+18],offset returned",
            "mov word ptr [output+20],ds",
            "mov word ptr [output+22],es",
            "mov word ptr [output+24],ss"]
    for i in range(FRAME_BYTES):
        asm += [f"mov al,byte ptr ss:[{(FRAME + i) & 0xFFFF}]",
                f"mov byte ptr [output+{28 + i}],al"]
    asm += ["pushf", "pop ax", "mov word ptr [output+26],ax",
            "sti", "mov bx,word ptr [handle]", "mov dx,offset output",
            f"mov cx,{RECORD_SIZE}", "mov ah,0x40", "int 0x21",
            "jc failure", f"cmp ax,{RECORD_SIZE}", "jne failure",
            "inc word ptr [index]", f"cmp word ptr [index],{len(CASES)}",
            "jb again", "mov bx,word ptr [handle]",
            "mov ah,0x3e", "int 0x21", "mov ax,0x4c00", "int 0x21",
            "failure:", "mov ax,0x4c01", "int 0x21",
            'filename: .asciz "RESULT.BIN"', "header: .space 8",
            "handle: .word 0", "index: .word 0",
            "frame_sps: .word " + ",".join(str(x[0]) for x in CASES),
            "bp_values: .word " + ",".join(str(x[1]) for x in CASES),
            f"output: .space {RECORD_SIZE}"]
    assembly, obj, com = (directory / "oracle.S", directory / "oracle.o",
                          directory / "ORACLE.COM")
    assembly.write_text("\n".join(asm) + "\n")
    subprocess.run(["as", "--32", str(assembly), "-o", str(obj)], check=True)
    subprocess.run(["ld", "-m", "elf_i386", "-Ttext", "0x100",
                    "--oformat", "binary", str(obj), "-o", str(com)], check=True)


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


def expected_case(model, cs, ss, returned, body, sp, bp):
    ram = bytearray(audit.shared.RAM_SIZE)
    at = audit.shared.physical(cs, 0x319B)
    ram[at:at + len(body)] = body
    audit.shared.put16(ram, ss, sp, returned)
    audit.shared.put16(ram, ss, (sp + 2) & 0xFFFF, cs)
    for i in range(5):
        audit.shared.put16(ram, ss, (sp + 4 + 2 * i) & 0xFFFF,
                           0xA100 + i)
    audit.shared.put16(ram, ss, (sp - 2) & 0xFFFF, cs)
    audit.shared.put16(ram, ss, (sp - 4) & 0xFFFF, 0x319B)
    cpu = audit.shared.Cpu(0xA5CD, 0x3456, 0x5678, 0x789A,
                           0x9ABC, 0xBCDE, bp, sp, cs, 0x319B,
                           cs, cs, ss, 2)
    arr = (ctypes.c_uint8 * audit.shared.RAM_SIZE).from_buffer(ram)
    assert model(arr, audit.shared.RAM_SIZE, ctypes.byref(cpu)) == 0
    regs = audit.shared.snap(cpu)
    frame = bytes(ram[audit.shared.physical(ss, (FRAME + i) & 0xFFFF)]
                  for i in range(FRAME_BYTES))
    return regs, frame


def compare(data, body, model):
    assert len(data) == 8 + len(CASES) * RECORD_SIZE
    cs, ss, returned, count = struct.unpack_from("<4H", data)
    assert ss == (cs + 0x700) & 0xFFFF
    assert count == len(CASES)
    for i, (sp, bp) in enumerate(CASES):
        record = data[8 + i * RECORD_SIZE:8 + (i + 1) * RECORD_SIZE]
        actual = (struct.unpack_from("<14H", record), record[28:])
        expected = expected_case(model, cs, ss, returned, body, sp, bp)
        assert actual == expected, (i, sp, bp, actual, expected)
    return {"cases": len(CASES), "original_entry_sha256": audit.BODY_SHA,
            "capture_sha256": sha256(data).hexdigest()}


def main():
    body = audit.source()
    directory = ROOT / "resources/u38-319b-parity/native-wrap"
    directory.mkdir(parents=True, exist_ok=True)
    build(directory, body)
    with tempfile.TemporaryDirectory(prefix="poolrad-u38-319b-wrap-") as temp:
        library = Path(temp) / "model.so"
        subprocess.run(["cc", "-shared", "-fPIC", "-std=c11", "-Wall",
                        "-Wextra", "-Wpedantic", "-Werror", "-I",
                        str(ROOT / "include"),
                        str(ROOT / "src/recovered/overlay_u38_empty_319b.c"),
                        "-o", str(library)], check=True)
        model = ctypes.CDLL(str(library)).pr_u38_empty_319b
        model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(audit.shared.Cpu)]
        model.restype = ctypes.c_int
        report = compare(run_native(directory), body, model)
    (directory / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    print("2 DOSBox-X normal-core U38 +319B RETF offset-wrap frames passed: "
          "14 CPU fields, 52 stack bytes")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
