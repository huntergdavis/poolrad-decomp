#!/usr/bin/env python3
"""DOSBox-X normal-core far-entry capture for original U38:+319B."""

from hashlib import sha256
import json
import os
from pathlib import Path
import struct
import subprocess

from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE

import check_overlay_u38_empty_319b as audit

ROOT = Path(__file__).resolve().parents[2]
BPS = (0, 0x0123, 0x7777, 0xFFFF)
FRAME = 0xEFD0
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
        "mov sp,0xf000", "mov word ptr [outer_segment],ax",
        "mov word ptr [header],ax",
        "mov word ptr [header+2],offset returned",
        f"mov word ptr [header+4],{len(BPS)}",
        "mov dx,offset filename", "xor cx,cx",
        "mov ah,0x3c", "int 0x21", "jc failure",
        "mov word ptr [handle],ax", "mov bx,ax",
        "mov dx,offset header", "mov cx,6", "mov ah,0x40",
        "int 0x21", "again:",
        "cli", "mov sp,0xf000", f"mov di,{FRAME}",
        f"mov cx,{FRAME_BYTES}", "xor ax,ax", "rep stosb",
        "mov si,word ptr [index]", "shl si,1",
        "mov bx,offset bp_values", "mov bp,word ptr [bx+si]",
        "push 0x0002", "popf", "mov ax,0xa5cd",
        "mov bx,0x3456", "mov cx,0x5678", "mov dx,0x789a",
        "mov si,0x9abc", "mov di,0xbcde",
        "push 0xa104", "push 0xa103", "push 0xa102",
        "push 0xa101", "push 0xa100",
        ".byte 0x9a", ".word 0x319b", "outer_segment: .word 0",
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
            "inc word ptr [index]", f"cmp word ptr [index],{len(BPS)}",
            "jb again", "mov bx,word ptr [handle]",
            "mov ah,0x3e", "int 0x21", "mov ax,0x4c00", "int 0x21",
            "failure:", "mov ax,0x4c01", "int 0x21",
            'filename: .asciz "RESULT.BIN"', "header: .space 6",
            "handle: .word 0", "index: .word 0",
            "bp_values: .word " + ",".join(str(bp) for bp in BPS),
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


def expected_case(cs, returned, body, bp):
    ram = bytearray(audit.shared.RAM_SIZE)
    at = audit.shared.physical(cs, 0x319B)
    ram[at:at + len(body)] = body
    sp = 0xEFF2
    cpu = audit.shared.Cpu(0xA5CD, 0x3456, 0x5678, 0x789A,
                           0x9ABC, 0xBCDE, bp, sp, cs, 0x319B,
                           cs, cs, cs, 2)
    audit.shared.put16(ram, cs, sp, returned)
    audit.shared.put16(ram, cs, sp + 2, cs)
    for i in range(5):
        audit.shared.put16(ram, cs, sp + 4 + 2 * i, 0xA100 + i)
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
    emu.emu_start(audit.shared.physical(cs, 0x319B),
                  audit.shared.RAM_SIZE, count=10)
    assert reached[0]
    regs = tuple(emu.reg_read(reg) & 0xFFFF for reg in audit.shared.REGISTERS)
    mem = bytes(emu.mem_read(audit.shared.physical(cs, FRAME), FRAME_BYTES))
    return regs, mem


def compare(data, body):
    assert len(data) == 6 + len(BPS) * RECORD_SIZE
    cs, returned, count = struct.unpack_from("<3H", data)
    assert count == len(BPS)
    for i, bp in enumerate(BPS):
        record = data[6 + i * RECORD_SIZE:6 + (i + 1) * RECORD_SIZE]
        actual = (struct.unpack_from("<14H", record), record[28:])
        expected = expected_case(cs, returned, body, bp)
        assert actual == expected, (i, bp, actual, expected)
    return {"cases": len(BPS), "original_entry_sha256": audit.BODY_SHA,
            "capture_sha256": sha256(data).hexdigest()}


def main():
    body = audit.source()
    directory = ROOT / "resources/u38-319b-parity/native"
    directory.mkdir(parents=True, exist_ok=True)
    build(directory, body)
    report = compare(run_native(directory), body)
    (directory / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    print("4 DOSBox-X normal-core original-byte U38 +319B frames passed: "
          "14 CPU fields, 52 stack bytes")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
