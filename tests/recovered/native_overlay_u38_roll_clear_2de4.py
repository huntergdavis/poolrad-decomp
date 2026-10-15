#!/usr/bin/env python3
"""Synthetic DOSBox-X U38:+2DE4/+0000 original-byte far-entry capture."""

from hashlib import sha256
import json
import os
from pathlib import Path
import struct
import subprocess

from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE

import check_overlay_u38_roll_clear_2de4 as audit

ROOT = Path(__file__).resolve().parents[2]
CASES = ((0x00, 0x0B), (0x1E, 0x35), (0x1F, 0x35), (0xFF, 0x58))
FRAME = 0xEFD0
FRAME_BYTES = 52
RECORD_SIZE = 28 + FRAME_BYTES + 2
STUB = bytes((0xB0, 0, 0xCA, 4, 0))


def build(directory, body, private):
    body_file = directory / "wrapper.bin"
    private_file = directory / "helper.bin"
    body_file.write_bytes(body)
    private_file.write_bytes(private)
    asm = [
        ".intel_syntax noprefix", ".code16", ".global _start",
        "_start: jmp main", ".org 0x2ce4",
        '.incbin "' + str(body_file) + '"',
        ".org 0x3100", "main:",
        "mov ax,cs", "mov ds,ax", "mov es,ax", "mov ss,ax",
        "mov sp,0xf000", "mov word ptr [outer_segment],ax",
        "mov word ptr [header],ax", "add ax,0x700",
        "mov word ptr [0x2df0],ax", "mov word ptr [header+2],ax",
        "mov word ptr [header+4],offset returned",
        f"mov word ptr [header+6],{len(CASES)}",
        "xor si,si", "mov di,offset psp_saved", "mov cx,31",
        "rep movsb", "mov si,offset helper_source", "xor di,di",
        "mov cx,31", "rep movsb", "mov dx,offset filename", "xor cx,cx",
        "mov ah,0x3c", "int 0x21", "jc failure",
        "mov word ptr [handle],ax", "mov bx,ax",
        "mov dx,offset header", "mov cx,8", "mov ah,0x40",
        "int 0x21", "again:",
        "cli", "mov sp,0xf000", f"mov di,{FRAME}",
        f"mov cx,{FRAME_BYTES}", "xor ax,ax", "rep stosb",
        "mov si,word ptr [index]", "mov bx,offset roll_values",
        "mov al,byte ptr [bx+si]", "mov byte ptr [0x7049],al",
        "mov bx,offset latch_values", "mov al,byte ptr [bx+si]",
        "mov byte ptr [0x6817],al", "mov byte ptr [0x6818],0xa7",
        "push 0x0002", "popf", "mov ax,0xa5cd",
        "mov bx,0x3456", "mov cx,0x5678", "mov dx,0x789a",
        "mov si,0x9abc", "mov di,0xbcde", "mov bp,0x7777",
        "push 0xa104", "push 0xa103", "push 0xa102",
        "push 0xa101", "push 0xa100",
        ".byte 0x9a", ".word 0x2de4", "outer_segment: .word 0",
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
    asm += ["mov al,byte ptr [0x6817]",
            f"mov byte ptr [output+{28 + FRAME_BYTES}],al",
            "mov al,byte ptr [0x6818]",
            f"mov byte ptr [output+{29 + FRAME_BYTES}],al",
            "pushf", "pop ax", "mov word ptr [output+26],ax",
            "sti", "mov bx,word ptr [handle]", "mov dx,offset output",
            f"mov cx,{RECORD_SIZE}", "mov ah,0x40", "int 0x21",
            "jc failure", f"cmp ax,{RECORD_SIZE}", "jne failure",
            "inc word ptr [index]", f"cmp word ptr [index],{len(CASES)}",
            "jb again", "mov si,offset psp_saved", "xor di,di",
            "mov cx,31", "rep movsb",
            "mov bx,word ptr [handle]", "mov ah,0x3e",
            "int 0x21", "mov ax,0x4c00", "int 0x21", "failure:",
            "mov ax,0x4c01", "int 0x21", 'filename: .asciz "RESULT.BIN"',
            "header: .space 8", "handle: .word 0", "index: .word 0",
            "roll_values: .byte " + ",".join(str(c[0]) for c in CASES),
            "latch_values: .byte " + ",".join(str(c[1]) for c in CASES),
            f"output: .space {RECORD_SIZE}", "psp_saved: .space 31",
            "helper_source:", '.incbin "' + str(private_file) + '"',
            ".org 0x6f48", "roll_stub: .byte 0xb0,0,0xca,4,0"]
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


def expected_case(cs, returned, body, private, result, latch):
    ram = bytearray(audit.shared.RAM_SIZE)
    loaded = (cs + 0x700) & 0xFFFF
    for seg, off, code in ((cs, 0x2DE4, body), (cs, 0, private),
                           (loaded, 0x0048,
                            bytes((0xB0, result, 0xCA, 4, 0)))):
        at = audit.shared.physical(seg, off)
        ram[at:at + len(code)] = code
    audit.shared.put16(ram, cs, 0x2DF0, loaded)
    sp = 0xEFF2
    cpu = audit.shared.Cpu(0xA5CD, 0x3456, 0x5678, 0x789A,
                           0x9ABC, 0xBCDE, 0x7777, sp,
                           cs, 0x2DE4, cs, cs, cs, 2)
    audit.shared.put16(ram, cs, sp, returned)
    audit.shared.put16(ram, cs, sp + 2, cs)
    for i in range(5):
        audit.shared.put16(ram, cs, sp + 4 + 2 * i, 0xA100 + i)
    ram[audit.shared.physical(cs, 0x6817)] = latch
    ram[audit.shared.physical(cs, 0x6818)] = 0xA7
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
    emu.emu_start(audit.shared.physical(cs, 0x2DE4),
                  audit.shared.RAM_SIZE, count=100)
    assert reached[0]
    regs = tuple(emu.reg_read(reg) & 0xFFFF for reg in audit.shared.REGISTERS)
    mem = bytes(emu.mem_read(audit.shared.physical(cs, FRAME), FRAME_BYTES))
    globals_ = bytes(emu.mem_read(audit.shared.physical(cs, 0x6817), 2))
    return regs, mem, globals_


def compare(data, body, private):
    assert len(data) == 8 + len(CASES) * RECORD_SIZE
    cs, loaded, returned, count = struct.unpack_from("<4H", data)
    assert loaded == (cs + 0x700) & 0xFFFF
    assert count == len(CASES)
    for i, (result, latch) in enumerate(CASES):
        record = data[8 + i * RECORD_SIZE:8 + (i + 1) * RECORD_SIZE]
        actual_regs = struct.unpack_from("<14H", record)
        actual_frame = record[28:28 + FRAME_BYTES]
        actual_globals = record[28 + FRAME_BYTES:]
        expected = expected_case(cs, returned, body, private, result, latch)
        assert (actual_regs, actual_frame, actual_globals) == expected, (
            i, (actual_regs, actual_frame.hex(), actual_globals.hex()),
            (expected[0], expected[1].hex(), expected[2].hex()))
    return {"cases": len(CASES), "original_wrapper_sha256": audit.BODY_SHA,
            "original_helper_sha256": audit.HELPER_SHA,
            "capture_sha256": sha256(data).hexdigest()}


def main():
    body, private = audit.source()
    directory = ROOT / "resources/u38-2de4-parity/native"
    directory.mkdir(parents=True, exist_ok=True)
    build(directory, body, private)
    report = compare(run_native(directory), body, private)
    (directory / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    print("4 DOSBox-X normal-core original-byte U38 +2DE4/+0000 frames "
          "passed: 14 CPU fields, 52 stack bytes, DS:6817/6818")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
