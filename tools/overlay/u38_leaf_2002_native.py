#!/usr/bin/env python3
"""DOSBox-X wrapped synthetic far-frame parity for U38 +2002."""

import argparse
import ctypes
from hashlib import sha256
import json
import os
from pathlib import Path
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "resources/u38-2002-parity/native"
RAM_SIZE = 1 << 20
MASK = RAM_SIZE - 1
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
BODY_SHA = "cdf3d522f6de4b8f37ccc47e3267e244f26cab4866883306b9dcfe1525dd2af4"
# The capture uses SS:7FFE/7FFF for PUSHF after recording CPU state.
STACK_OFFSETS = (tuple(range(0xFFE0, 0x10000)) + tuple(range(0x20)) +
                 tuple(range(0x7FE0, 0x7FFE)) + tuple(range(0x8000, 0x8020)))
CROSS_OFFSETS = (0, 1, 2)
CELL_OFFSETS = tuple(range(0x2096, 0x209E))
RESULT_SIZE = 28 + len(STACK_OFFSETS) + len(CROSS_OFFSETS) + 8 + len(CELL_OFFSETS)


def base_offsets(base):
    return tuple((base + i) & 0xFFFF for i in range(-0x31, -0x29))


class Cpu(ctypes.Structure):
    _fields_ = [(name, ctypes.c_uint16) for name in
                "ax bx cx dx si di bp sp cs ip ds es ss flags".split()]


def physical(seg, off):
    return ((seg << 4) + (off & 0xFFFF)) & MASK


def put16(ram, seg, off, value):
    at = physical(seg, off)
    ram[at] = value & 255
    ram[(at + 1) & MASK] = (value >> 8) & 255


def original_body(ovr_path=None):
    ovr = (ovr_path or ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    if len(ovr) != 206105 or sha256(ovr).hexdigest() != OVR_SHA:
        raise ValueError("original GAME.OVR identity differs")
    body = ovr[0x10C5D:0x10D3A]
    if len(body) != 221 or sha256(body).hexdigest() != BODY_SHA:
        raise ValueError("U38 +2002 body identity differs")
    return body


def assembly(body_path, sp, mode, base):
    assert sp in (0x8000, 0xFFFB, 0xFFFE, 0xFFFF)
    assert mode in (0, 1)
    wrapped_cs = (sp + 2) & 0xFFFF
    lines = [
        ".intel_syntax noprefix", ".code16", ".global _start",
        "_start: jmp main", ".org 0x01cc", "body:",
        f'.incbin "{body_path.resolve()}"',
        ".org 0x0300", "main:",
        "mov ax,cs", "mov ds,ax", "mov es,ax", "mov ss,ax",
        "mov sp,0xf000", "mov dx,offset filename", "xor cx,cx",
        "mov ah,0x3c", "int 0x21", "jc failure",
        "mov word ptr cs:[handle],ax", "cli",
        "mov ax,cs", "add ax,0x1000", "mov word ptr cs:[code_segment],ax",
        "mov es,ax", "mov si,offset body", "mov di,0x2002",
        "mov cx,221", "rep movsb",
        "mov ax,cs", "add ax,0x2000", "mov ss,ax", "mov es,ax",
        "mov sp,0x8000", "mov di,0xffe0", "mov cx,0x20",
        "mov al,0xa5", "rep stosb", "xor di,di", "mov cx,0x20",
        "rep stosb", "mov di,0x7fe0", "mov cx,0x40", "rep stosb",
        "mov ax,ss", "add ax,0x1000", "mov es,ax",
        "xor di,di", "mov cx,3", "mov al,0xa5", "rep stosb",
        f"mov di,0x{sp:04x}",
        "mov word ptr ss:[di],offset returned",
        "mov ax,cs", f"mov word ptr ss:[0x{wrapped_cs:04x}],ax",
        f"mov word ptr ss:[di+4],0x{base:04x}",
        f"mov word ptr ss:[0x{(base+8)&0xFFFF:04x}],0x2000",
        f"mov word ptr ss:[0x{(base+10)&0xFFFF:04x}],ax",
    ]
    for i in range(8):
        value = i + 2 if mode == 0 else 2
        score = i + 1 if mode == 0 else 8 - i
        lines += [
            f"mov byte ptr cs:[0x{0x2096 + i:04x}],0x{value:02x}",
            f"mov word ptr cs:[0x{0x409B + i * 0x38 + value * 4:04x}],0x{score:04x}",
            f"mov word ptr cs:[0x{0x409D + i * 0x38 + value * 4:04x}],0",
        ]
    lines += [
        "mov ax,cs", "mov es,ax", "mov ax,0x0002", "push ax", "popf",
        "mov bx,0x1357", "mov cx,0x2468", "mov dx,0x3579",
        "mov si,0x468a", "mov di,0x579b", "mov bp,0x68ac",
        f"mov sp,0x{sp:04x}",
        "mov ax,word ptr cs:[code_segment]", "push ax",
        "mov ax,0x2002", "push ax", "mov ax,0xabcd", "retf",
        "returned:",
    ]
    for i, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        lines.append(f"mov word ptr cs:[output+{2*i}],{reg}")
    lines += [
        "mov word ptr cs:[output+18],offset returned",
        "mov word ptr cs:[output+20],ds",
        "mov word ptr cs:[output+22],es",
        "mov word ptr cs:[output+24],ss",
        "mov sp,0x8000", "pushf", "pop word ptr cs:[output+26]",
    ]
    for i, off in enumerate(STACK_OFFSETS):
        lines += [f"mov al,byte ptr ss:[0x{off:04x}]",
                  f"mov byte ptr cs:[output+{28+i}],al"]
    lines += ["mov ax,ss", "add ax,0x1000", "mov es,ax"]
    pos = 28 + len(STACK_OFFSETS)
    for i, off in enumerate(CROSS_OFFSETS):
        lines += [f"mov al,byte ptr es:[0x{off:04x}]",
                  f"mov byte ptr cs:[output+{pos+i}],al"]
    pos += len(CROSS_OFFSETS)
    for i, off in enumerate(base_offsets(base)):
        lines += [f"mov al,byte ptr ss:[0x{off:04x}]",
                  f"mov byte ptr cs:[output+{pos+i}],al"]
    pos += 8
    for i, off in enumerate(CELL_OFFSETS):
        lines += [f"mov al,byte ptr cs:[0x{off:04x}]",
                  f"mov byte ptr cs:[output+{pos+i}],al"]
    lines += [
        "mov ax,cs", "mov ss,ax", "mov sp,0xf000", "sti",
        "mov bx,word ptr cs:[handle]", "mov dx,offset output",
        f"mov cx,{RESULT_SIZE}", "mov ah,0x40", "int 0x21",
        "jc failure", f"cmp ax,{RESULT_SIZE}", "jne failure",
        "mov bx,word ptr cs:[handle]", "mov ah,0x3e", "int 0x21",
        "mov ax,0x4c00", "int 0x21",
        "failure:", "mov ax,0x4c01", "int 0x21",
        'filename: .asciz "RESULT.BIN"', "handle: .word 0",
        "code_segment: .word 0", f"output: .space {RESULT_SIZE}",
    ]
    return "\n".join(lines) + "\n"


def native_case(body, sp, mode, base):
    directory = OUT / f"sp{sp:04x}-m{mode}-base{base:04x}"
    directory.mkdir(parents=True, exist_ok=True)
    body_path = directory / "body.bin"
    body_path.write_bytes(body)
    source = directory / "oracle.S"
    obj = directory / "oracle.o"
    com = directory / "ORACLE.COM"
    source.write_text(assembly(body_path, sp, mode, base))
    subprocess.run(["as", "--32", str(source), "-o", str(obj)],
                   check=True, capture_output=True, text=True)
    subprocess.run(["ld", "-m", "elf_i386", "-Ttext", "0x100",
                    "--oformat", "binary", str(obj), "-o", str(com)],
                   check=True, capture_output=True, text=True)
    if com.read_bytes()[0x01CC:0x01CC + len(body)] != body:
        raise ValueError("COM body differs from original")
    config = directory / "dosbox.conf"
    config.write_text(
        "[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n"
        "[cpu]\ncore=normal\ncputype=386\ncycles=max\n"
        "[mixer]\nnosound=true\n[midi]\nmididevice=none\n"
        "[autoexec]\nmount c " + str(directory.resolve()) +
        "\nc:\nORACLE.COM\nexit\n")
    result_path = directory / "RESULT.BIN"
    result_path.unlink(missing_ok=True)
    with (directory / "dosbox.log").open("w") as log:
        subprocess.run(["dosbox-x", "-conf", str(config), "-fastlaunch",
                        "-exit", "-noconsole"],
                       env=dict(os.environ, SDL_VIDEODRIVER="dummy",
                                SDL_AUDIODRIVER="dummy"),
                       stdout=log, stderr=subprocess.STDOUT,
                       check=True, timeout=45)
    result = result_path.read_bytes()
    if len(result) != RESULT_SIZE:
        raise ValueError(f"native capture incomplete for SP={sp:04X}")
    return result, sha256(com.read_bytes()).hexdigest()


def compare_case(result, sp, mode, base, method):
    observed = struct.unpack_from("<14H", result)
    cs, returned, ss = observed[8], observed[9], observed[12]
    code_seg = (cs + 0x1000) & 0xFFFF
    if ss != (cs + 0x2000) & 0xFFFF:
        raise ValueError("native stack segment differs")
    ram = bytearray(RAM_SIZE)
    for off in STACK_OFFSETS:
        ram[physical(ss, off)] = 0xA5
    for off in CROSS_OFFSETS:
        ram[physical((ss + 0x1000) & 0xFFFF, off)] = 0xA5
    put16(ram, ss, sp, returned)
    put16(ram, ss, sp + 2, cs)
    put16(ram, ss, sp + 4, base)
    put16(ram, ss, base + 8, 0x2000)
    put16(ram, ss, base + 10, cs)
    for i in range(8):
        value = i + 2 if mode == 0 else 2
        score = i + 1 if mode == 0 else 8 - i
        ram[physical(cs, 0x2096 + i)] = value
        put16(ram, cs, 0x409B + i * 0x38 + value * 4, score)
        put16(ram, cs, 0x409D + i * 0x38 + value * 4, 0)
    put16(ram, ss, sp - 4, 0x2002)
    put16(ram, ss, sp - 2, code_seg)
    cpu = Cpu(0xABCD, 0x1357, 0x2468, 0x3579, 0x468A, 0x579B,
              0x68AC, sp, code_seg, 0x2002, cs, cs, ss, 0x0002)
    memory = (ctypes.c_uint8 * RAM_SIZE).from_buffer(ram)
    if method(memory, RAM_SIZE, ctypes.byref(cpu)) != 0:
        raise ValueError("C rejected native frame")
    for (name, _), value in zip(Cpu._fields_, observed):
        if getattr(cpu, name) != value:
            raise ValueError(f"SP={sp:04X} {name}: "
                             f"C={getattr(cpu,name):04X} native={value:04X}")
    positions = ((STACK_OFFSETS, ss),
                 (CROSS_OFFSETS, (ss + 0x1000) & 0xFFFF),
                 (base_offsets(base), ss), (CELL_OFFSETS, cs))
    pos = 28
    for offsets, segment in positions:
        expected = bytes(ram[physical(segment, off)] for off in offsets)
        actual = result[pos:pos + len(offsets)]
        if expected != actual:
            diff = [(f"{off:04X}", f"{expected[i]:02X}", f"{actual[i]:02X}")
                    for i, off in enumerate(offsets) if expected[i] != actual[i]]
            raise ValueError(f"SP={sp:04X} segment {segment:04X} bytes differ: {diff}")
        pos += len(offsets)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ovr", type=Path)
    args = parser.parse_args()
    body = original_body(args.ovr)
    OUT.mkdir(parents=True, exist_ok=True)
    shared = OUT / "u38_2002.so"
    subprocess.run([
        "cc", "-shared", "-fPIC", "-std=c11", "-Wall", "-Wextra",
        "-Wpedantic", "-Werror", "-I", str(ROOT / "include"),
        str(ROOT / "src/recovered/overlay_u38_leaf_2002.c"),
        "-o", str(shared)], check=True)
    method = ctypes.CDLL(str(shared)).pr_u38_leaf_2002
    method.argtypes = (ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                       ctypes.POINTER(Cpu))
    method.restype = ctypes.c_int
    cases = []
    for sp, mode, base in ((0xFFFE, 0, 0x9000), (0xFFFF, 1, 0x9000),
                           (0xFFFB, 0, 0x9000), (0x8000, 0, 0xFFF4)):
        result, com_sha = native_case(body, sp, mode, base)
        compare_case(result, sp, mode, base, method)
        cases.append({"sp": f"{sp:04X}", "mode": mode, "base": f"{base:04X}",
                      "result_sha256": sha256(result).hexdigest(),
                      "com_sha256": com_sha,
                      "native_cpu_ram_parity": "PASS"})
    report = {"original_body_sha256": BODY_SHA, "core": "normal",
              "cpu": "386", "cases": cases}
    (OUT / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(f"U38 +2002 DOSBox-X synthetic native/C parity: {len(cases)} cases PASS")


if __name__ == "__main__":
    main()
