#!/usr/bin/env python3
"""Synthetic DOSBox-X normal-core U38:+317A far-entry capture."""

from hashlib import sha256
import json
import os
from pathlib import Path
import struct
import subprocess

from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE

import check_overlay_u38_remove_gate_317a as audit

ROOT = Path(__file__).resolve().parents[2]
CASES = (0x00, 0x01, 0x80, 0xFF)
FRAME = 0xEFD0
FRAME_BYTES = 52
RECORD_SIZE = 28 + FRAME_BYTES


def build(directory, body):
    body_file = directory / "wrapper.bin"
    body_file.write_bytes(body)
    asm = [
        ".intel_syntax noprefix", ".code16", ".global _start",
        "_start: jmp main", ".org 0x307a",
        '.incbin "' + str(body_file) + '"',
        ".org 0x3100", "main:",
        "mov ax,cs", "mov ds,ax", "mov es,ax", "mov ss,ax",
        "mov sp,0xf000", "mov word ptr [outer_segment],ax",
        "mov word ptr [header],ax", "add ax,0x700",
        "mov word ptr [0x3195],ax", "mov word ptr [header+2],ax",
        "mov word ptr [header+4],offset returned",
        f"mov word ptr [header+6],{len(CASES)}",
        "mov dx,offset filename", "xor cx,cx",
        "mov ah,0x3c", "int 0x21", "jc failure",
        "mov word ptr [handle],ax", "mov bx,ax",
        "mov dx,offset header", "mov cx,8", "mov ah,0x40",
        "int 0x21", "again:",
        "cli", "mov sp,0xf000", f"mov di,{FRAME}",
        f"mov cx,{FRAME_BYTES}", "xor ax,ax", "rep stosb",
        "push 0x0002", "popf", "mov ax,0xa5cd",
        "mov bx,0x3456", "mov cx,0x5678", "mov dx,0x789a",
        "mov si,0x9abc", "mov di,0xbcde", "mov bp,0x7777",
        "push 0xa104", "push 0xa103", "push 0xa102",
        "push 0xa101", "mov si,word ptr [index]",
        "mov bx,offset gate_values", "mov bl,byte ptr [bx+si]",
        "mov bh,0xa1", "push bx", "mov bx,0x3456", "mov si,0x9abc",
        ".byte 0x9a", ".word 0x317a", "outer_segment: .word 0",
        "returned:",
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
            "gate_values: .byte " + ",".join(str(c) for c in CASES),
            f"output: .space {RECORD_SIZE}",
            ".org 0x6f2a", "remove_stub: .byte 0xb8,0xef,0xbe,0xca,0x0a,0x00"]
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


def expected_case(cs, returned, body, gate):
    ram = bytearray(audit.shared.RAM_SIZE)
    loaded = (cs + 0x700) & 0xFFFF
    for seg, off, code in ((cs, 0x317A, body),
                           (loaded, 0x002A,
                            bytes((0xB8, 0xEF, 0xBE, 0xCA, 0x0A, 0x00)))):
        at = audit.shared.physical(seg, off)
        ram[at:at + len(code)] = code
    audit.shared.put16(ram, cs, 0x3195, loaded)
    sp = 0xEFF2
    cpu = audit.shared.Cpu(0xA5CD, 0x3456, 0x5678, 0x789A,
                           0x9ABC, 0xBCDE, 0x7777, sp,
                           cs, 0x317A, cs, cs, cs, 2)
    audit.shared.put16(ram, cs, sp, returned)
    audit.shared.put16(ram, cs, sp + 2, cs)
    for i in range(5):
        audit.shared.put16(ram, cs, sp + 4 + 2 * i, 0xA100 + i)
    audit.shared.put16(ram, cs, sp + 4, 0xA100 | gate)
    emu = Uc(UC_ARCH_X86, UC_MODE_16)
    emu.mem_map(0, audit.shared.RAM_SIZE)
    emu.mem_write(0, bytes(ram))
    for (name, _), reg in zip(audit.shared.Cpu._fields_,
                               audit.shared.REGISTERS):
        emu.reg_write(reg, getattr(cpu, name))
    reached = [False]

    def stop(uc, address, size, user):
        del size, user
        if address == audit.shared.physical(cs, returned):
            reached[0] = True
            uc.emu_stop()

    emu.hook_add(UC_HOOK_CODE, stop)
    emu.emu_start(audit.shared.physical(cs, 0x317A),
                  audit.shared.RAM_SIZE, count=80)
    assert reached[0]
    regs = tuple(emu.reg_read(reg) & 0xFFFF for reg in audit.shared.REGISTERS)
    mem = bytes(emu.mem_read(audit.shared.physical(cs, FRAME), FRAME_BYTES))
    return regs, mem


def compare(data, body):
    assert len(data) == 8 + len(CASES) * RECORD_SIZE
    cs, loaded, returned, count = struct.unpack_from("<4H", data)
    assert loaded == (cs + 0x700) & 0xFFFF
    assert count == len(CASES)
    for i, gate in enumerate(CASES):
        record = data[8 + i * RECORD_SIZE:8 + (i + 1) * RECORD_SIZE]
        actual = (struct.unpack_from("<14H", record), record[28:])
        expected = expected_case(cs, returned, body, gate)
        assert actual == expected, (i, gate, actual, expected)
    return {"cases": len(CASES), "original_wrapper_sha256": audit.BODY_SHA,
            "capture_sha256": sha256(data).hexdigest()}


def main():
    body = audit.source()
    directory = ROOT / "resources/u38-317a-parity/native"
    directory.mkdir(parents=True, exist_ok=True)
    build(directory, body)
    report = compare(run_native(directory), body)
    (directory / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    print("4 DOSBox-X normal-core original-byte U38 +317A frames passed: "
          "14 CPU fields, 52 stack bytes")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
