#!/usr/bin/env python3
"""DOSBox-X synthetic far-frame parity for original U3B0 +0711 bytes."""

import argparse
import ctypes
from hashlib import sha256
import json
import os
from pathlib import Path
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "resources/u3b0-0711-parity/native"
RAM_SIZE = 1 << 20
MASK = RAM_SIZE - 1
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
BODY_SHA = "a3510458227e9373193717033475fa31fe5d1bf0ef01a33da6582726d6650092"
STACK_OFFSETS = tuple(range(0xFFE0, 0x10000)) + tuple(range(0x20))
CROSS_OFFSETS = (0,)
NODE_OFFSETS = (0x4029, 0x402A, 0x402B, 0x402C, 0x402D,
                0x4129, 0x412A, 0x412B, 0x412C, 0x412D,
                0x4229, 0x422A, 0x422B, 0x422C, 0x422D)
RESULT_SIZE = 28 + len(STACK_OFFSETS) + len(CROSS_OFFSETS) + len(NODE_OFFSETS)


class Cpu(ctypes.Structure):
    _fields_ = [(name, ctypes.c_uint16) for name in
                "ax bx cx dx si di bp sp cs ip ds es ss flags".split()]


def physical(seg, off):
    return ((seg << 4) + (off & 0xFFFF)) & MASK


def put16_at(ram, at, value):
    ram[at & MASK] = value & 255
    ram[(at + 1) & MASK] = value >> 8


def original_body(ovr_path=None):
    ovr = (ovr_path or ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    if len(ovr) != 206105 or sha256(ovr).hexdigest() != OVR_SHA:
        raise ValueError("original GAME.OVR identity differs")
    body = ovr[0x2CFE0:0x2D073]
    if len(body) != 147 or sha256(body).hexdigest() != BODY_SHA:
        raise ValueError("U3B0 +0711 body identity differs")
    return body


def assembly(body_path, sp, bound, first_flag, second_flag):
    assert sp in (0xFFFB, 0xFFFE, 0xFFFF)
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
        "mov es,ax", "mov si,offset body", "mov di,0x0711",
        "mov cx,147", "rep movsb",
        "mov ax,cs", "add ax,0x2000", "mov ss,ax", "mov es,ax",
        "mov sp,0x8000", "mov di,0xffe0", "mov cx,0x20",
        "mov al,0xa5", "rep stosb", "xor di,di", "mov cx,0x20",
        "rep stosb", "mov ax,ss", "add ax,0x1000", "mov es,ax",
        "xor di,di", "mov cx,1", "mov al,0xa5", "rep stosb",
        f"mov di,0x{sp:04x}",
        "mov word ptr ss:[di],offset returned",
        "mov ax,cs", f"mov word ptr ss:[{wrapped_cs}],ax",
        "mov word ptr ss:[di+4],0xa5a5",
        "mov word ptr ss:[di+6],0x4000",
        "mov word ptr ss:[di+8],ax",
        f"mov word ptr ss:[di+10],0x{bound:04x}",
        f"mov byte ptr cs:[0x4029],0x{first_flag:02x}",
        "mov word ptr cs:[0x402a],0x4100",
        "mov word ptr cs:[0x402c],ax",
        f"mov byte ptr cs:[0x4129],0x{second_flag:02x}",
        "mov word ptr cs:[0x412a],0x4200",
        "mov word ptr cs:[0x412c],ax",
        "mov byte ptr cs:[0x4229],0",
        "mov word ptr cs:[0x422a],0",
        "mov word ptr cs:[0x422c],0",
        "mov es,ax", "mov ax,0x0002", "push ax", "popf",
        "mov bx,0x1357", "mov cx,0x2468", "mov dx,0x3579",
        "mov si,0x468a", "mov di,0x579b", "mov bp,0x68ac",
        f"mov sp,0x{sp:04x}",
        "mov ax,word ptr cs:[code_segment]", "push ax",
        "mov ax,0x0711", "push ax", "mov ax,0xabcd", "retf",
        "returned:",
    ]
    for i, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        lines.append(f"mov word ptr cs:[output+{2*i}],{reg}")
    lines += [
        "mov word ptr cs:[output+18],offset returned",
        "mov word ptr cs:[output+20],ds",
        "mov word ptr cs:[output+22],es",
        "mov word ptr cs:[output+24],ss",
        "mov sp,0x8000",
        "pushf", "pop word ptr cs:[output+26]",
    ]
    for i, off in enumerate(STACK_OFFSETS):
        lines += [f"mov al,byte ptr ss:[0x{off:04x}]",
                  f"mov byte ptr cs:[output+{28+i}],al"]
    lines += ["mov ax,ss", "add ax,0x1000", "mov es,ax"]
    for i, off in enumerate(CROSS_OFFSETS):
        lines += [f"mov al,byte ptr es:[0x{off:04x}]",
                  f"mov byte ptr cs:[output+{28+len(STACK_OFFSETS)+i}],al"]
    for i, off in enumerate(NODE_OFFSETS):
        index = 28 + len(STACK_OFFSETS) + len(CROSS_OFFSETS) + i
        lines += [f"mov al,byte ptr cs:[0x{off:04x}]",
                  f"mov byte ptr cs:[output+{index}],al"]
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


def native_case(body, sp, bound, first_flag, second_flag):
    directory = OUT / f"sp{sp:04x}-b{bound:04x}-f{first_flag}{second_flag}"
    directory.mkdir(parents=True, exist_ok=True)
    body_path = directory / "body.bin"
    body_path.write_bytes(body)
    source = directory / "oracle.S"
    obj = directory / "oracle.o"
    com = directory / "ORACLE.COM"
    source.write_text(assembly(body_path, sp, bound, first_flag, second_flag))
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


def compare_case(result, sp, bound, first_flag, second_flag, method):
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
    frame = physical(ss, sp)
    put16_at(ram, frame, returned)
    put16_at(ram, physical(ss, sp + 2), cs)
    put16_at(ram, physical(ss, sp + 4), 0xA5A5)
    put16_at(ram, physical(ss, sp + 6), 0x4000)
    put16_at(ram, physical(ss, sp + 8), cs)
    put16_at(ram, physical(ss, sp + 10), bound)
    for off, flag, next_off, next_seg in (
        (0x4000, first_flag, 0x4100, cs),
        (0x4100, second_flag, 0x4200, cs),
        (0x4200, 0, 0, 0),
    ):
        ram[physical(cs, off + 0x29)] = flag
        put16_at(ram, physical(cs, off + 0x2A), next_off)
        put16_at(ram, physical(cs, off + 0x2C), next_seg)
    put16_at(ram, physical(ss, sp - 4), 0x0711)
    put16_at(ram, physical(ss, sp - 2), code_seg)
    cpu = Cpu(0xABCD, 0x1357, 0x2468, 0x3579, 0x468A, 0x579B,
              0x68AC, sp, code_seg, 0x0711, cs, cs, ss, 0x0002)
    memory = (ctypes.c_uint8 * RAM_SIZE).from_buffer(ram)
    if method(memory, RAM_SIZE, ctypes.byref(cpu)) != 0:
        raise ValueError("C rejected native frame")
    for (name, _), value in zip(Cpu._fields_, observed):
        if getattr(cpu, name) != value:
            raise ValueError(f"SP={sp:04X} {name}: "
                             f"C={getattr(cpu,name):04X} native={value:04X}")
    expected_stack = bytes(ram[physical(ss, off)] for off in STACK_OFFSETS)
    if expected_stack != result[28:28 + len(STACK_OFFSETS)]:
        native_stack = result[28:28 + len(STACK_OFFSETS)]
        different = [(f"{off:04X}", f"{expected_stack[i]:02X}",
                      f"{native_stack[i]:02X}")
                     for i, off in enumerate(STACK_OFFSETS)
                     if expected_stack[i] != native_stack[i]]
        raise ValueError(f"SP={sp:04X} stack bytes differ: {different}")
    cross_seg = (ss + 0x1000) & 0xFFFF
    expected_cross = bytes(ram[physical(cross_seg, off)] for off in CROSS_OFFSETS)
    cross_start = 28 + len(STACK_OFFSETS)
    if expected_cross != result[cross_start:cross_start + len(CROSS_OFFSETS)]:
        native_cross = result[cross_start:cross_start + len(CROSS_OFFSETS)]
        different = [(f"{off:04X}", f"{expected_cross[i]:02X}",
                      f"{native_cross[i]:02X}")
                     for i, off in enumerate(CROSS_OFFSETS)
                     if expected_cross[i] != native_cross[i]]
        raise ValueError(f"SP={sp:04X} linear cross bytes differ: {different}")
    expected_nodes = bytes(ram[physical(cs, off)] for off in NODE_OFFSETS)
    node_start = cross_start + len(CROSS_OFFSETS)
    if expected_nodes != result[node_start:]:
        native_nodes = result[node_start:]
        different = [(f"{off:04X}", f"{expected_nodes[i]:02X}",
                      f"{native_nodes[i]:02X}")
                     for i, off in enumerate(NODE_OFFSETS)
                     if expected_nodes[i] != native_nodes[i]]
        raise ValueError(f"SP={sp:04X} node bytes differ: {different}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ovr", type=Path)
    args = parser.parse_args()
    body = original_body(args.ovr)
    OUT.mkdir(parents=True, exist_ok=True)
    shared = OUT / "u3b0_0711.so"
    subprocess.run([
        "cc", "-shared", "-fPIC", "-std=c11", "-Wall", "-Wextra",
        "-Wpedantic", "-Werror", "-I", str(ROOT / "include"),
        str(ROOT / "src/recovered/overlay_u3b0_leaf_0711.c"),
        "-o", str(shared)], check=True)
    method = ctypes.CDLL(str(shared)).pr_u3b0_leaf_0711
    method.argtypes = (ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                      ctypes.POINTER(Cpu))
    method.restype = ctypes.c_int
    cases = []
    fixtures = [(0xFFFE, 0, 1, 0), (0xFFFF, 1, 0, 1), (0xFFFB, 2, 0, 1)]
    fixtures += [(sp, bound, first, second)
                 for sp in (0xFFFB, 0xFFFE, 0xFFFF)
                 for bound, first, second in ((0, 0, 0), (0x8000, 0x80, 0),
                                              (0x7FFF, 0, 0xFF))]
    for sp, bound, first_flag, second_flag in fixtures:
        result, com_sha = native_case(body, sp, bound, first_flag,
                                      second_flag)
        compare_case(result, sp, bound, first_flag, second_flag, method)
        cases.append({"sp": f"{sp:04X}", "bound": bound,
                      "first_flag": first_flag, "second_flag": second_flag,
                      "result_sha256": sha256(result).hexdigest(),
                      "com_sha256": com_sha,
                      "native_cpu_ram_parity": "PASS"})
    report = {"original_body_sha256": BODY_SHA, "core": "normal",
              "cpu": "386", "cases": cases}
    (OUT / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(f"U3B0 +0711 DOSBox-X synthetic native/C parity: {len(cases)} cases PASS")


if __name__ == "__main__":
    main()
