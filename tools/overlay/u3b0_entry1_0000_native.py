#!/usr/bin/env python3
"""DOSBox-X original-byte/C parity for U3B0 entry 1 at offset zero.

Generated COM files and native captures stay under ignored resources/.
Frames are synthetic; no saved-game reach is claimed. The capture pattern
reuses tools/overlay/u9e_entry17_110d_native.py at commit 4b2c882.
"""

import ctypes
from hashlib import sha256
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[2]
RAM_SIZE = 1 << 20
MASK = RAM_SIZE - 1
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
LEAF_SHA = "a7d25485edf131a638e320475f22544c82fe81967478369e17556db2509b2a3c"
OFFSETS = tuple(range(0xFFE0, 0x10000)) + tuple(range(0x0000, 0x0020))
RESULT_SIZE = 28 + len(OFFSETS) + 1 + 12
CASES = ((0xFFFE, 0, 0), (0xFFFF, 1, 0),
         (0xFFFB, 2, 0), (0xFFFB, 3, 1))


class Cpu(ctypes.Structure):
    _fields_ = [(name, ctypes.c_uint16) for name in
                "ax bx cx dx si di bp sp cs ip ds es ss flags".split()]


def physical(seg, off):
    return ((seg << 4) + (off & 0xFFFF)) & MASK


def put16(ram, seg, off, value):
    at = physical(seg, off)
    ram[at] = value & 255
    ram[(at + 1) & MASK] = (value >> 8) & 255


def body_bytes():
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    if len(ovr) != 206105 or sha256(ovr).hexdigest() != OVR_SHA:
        raise ValueError("original GAME.OVR identity differs")
    body = ovr[0x2C8CF:0x2C936]
    if len(body) != 103 or sha256(body).hexdigest() != LEAF_SHA:
        raise ValueError("U3B0 entry 1 original slice differs")
    return body


def model_library(directory):
    shared = directory / "u3b0_entry1.so"
    subprocess.run([
        "cc", "-shared", "-fPIC", "-std=c11", "-Wall", "-Wextra", "-Werror",
        "-I", str(ROOT / "include"),
        str(ROOT / "src/recovered/overlay_u3b0_entry1_0000.c"),
        "-o", str(shared),
    ], check=True)
    method = ctypes.CDLL(str(shared)).pr_u3b0_entry1_0000
    method.argtypes = (ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                       ctypes.POINTER(Cpu))
    method.restype = ctypes.c_int
    return method


def assembly(body_path, sp, count, cycle):
    lines = [
        ".intel_syntax noprefix", ".code16", ".global _start",
        "_start: jmp main", ".org 0x01cc", "body:",
        f'.incbin "{body_path.resolve()}"',
        ".org 0x1f00", "main:",
        "mov ax,cs", "mov ds,ax", "mov es,ax", "mov ss,ax",
        "mov sp,0xf000", "mov dx,offset filename", "xor cx,cx",
        "mov ah,0x3c", "int 0x21", "jc failure",
        "mov word ptr cs:[handle],ax",
        "cli", "mov ax,cs", "add ax,0x1000",
        "mov word ptr cs:[code_segment],ax", "mov es,ax",
        "mov si,offset body", "xor di,di", "mov cx,103", "rep movsb",
        "mov ax,cs", "add ax,0x2000", "mov ss,ax", "mov es,ax",
        "mov sp,0x8000",
        "mov di,0xffe0", "mov cx,0x20", "mov al,0xa5",
        "rep stosb", "xor di,di", "mov cx,0x20", "rep stosb",
        "mov ax,ss", "add ax,0x1000", "mov es,ax",
        "mov byte ptr es:[0],0xa5",
        f"mov di,0x{sp:04x}",
        "mov word ptr ss:[di],offset returned",
        "mov ax,cs", "mov word ptr ss:[di+2],ax",
        "mov word ptr ss:[di+4],0x4000",
        "mov word ptr ss:[di+6],ax",
        f"mov word ptr ss:[di+8],0x{count:04x}",
        "mov word ptr cs:[0x402a],0x4100",
        "mov word ptr cs:[0x402c],ax",
        "mov word ptr cs:[0x412a],0x4200",
        "mov word ptr cs:[0x412c],ax",
        f"mov word ptr cs:[0x422a],0x{0x4100 if cycle else 0:04x}",
        "mov word ptr cs:[0x422c],ax" if cycle else
        "mov word ptr cs:[0x422c],0",
        "mov es,ax", "mov ax,0x0002", "push ax", "popf",
        "mov bx,0x1357", "mov cx,0x2468",
        "mov dx,0x3579", "mov si,0x468a", "mov di,0x579b",
        "mov bp,0x68ac", f"mov sp,0x{sp:04x}",
        "mov ax,word ptr cs:[code_segment]", "push ax",
        "mov ax,0", "push ax", "mov ax,0xabcd", "retf",
        "returned:",
    ]
    for i, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        lines.append(f"mov word ptr cs:[output+{2*i}],{reg}")
    lines += [
        "mov word ptr cs:[output+18],offset returned",
        "mov word ptr cs:[output+20],ds",
        "mov word ptr cs:[output+22],es",
        "mov word ptr cs:[output+24],ss",
    ]
    for i, off in enumerate(OFFSETS):
        lines += [f"mov al,byte ptr ss:[0x{off:04x}]",
                  f"mov byte ptr cs:[output+{28+i}],al"]
    cross_at = 28 + len(OFFSETS)
    lines += [
        "pushf", "pop word ptr cs:[output+26]",
        "mov ax,ss", "add ax,0x1000", "mov es,ax",
        "mov al,byte ptr es:[0]",
        f"mov byte ptr cs:[output+{cross_at}],al",
    ]
    for i, off in enumerate((0x402A, 0x402B, 0x402C, 0x402D,
                             0x412A, 0x412B, 0x412C, 0x412D,
                             0x422A, 0x422B, 0x422C, 0x422D)):
        lines += [f"mov al,byte ptr cs:[0x{off:04x}]",
                  f"mov byte ptr cs:[output+{cross_at+1+i}],al"]
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


def native_case(directory, body, sp, count, cycle):
    directory.mkdir(parents=True, exist_ok=True)
    body_path = directory / "body.bin"
    body_path.write_bytes(body)
    source = directory / "oracle.S"
    obj = directory / "oracle.o"
    com = directory / "ORACLE.COM"
    source.write_text(assembly(body_path, sp, count, cycle))
    subprocess.run(["as", "--32", str(source), "-o", str(obj)],
                   check=True, capture_output=True, text=True)
    subprocess.run(["ld", "-m", "elf_i386", "-Ttext", "0x100",
                    "--oformat", "binary", str(obj), "-o", str(com)],
                   check=True, capture_output=True, text=True)
    if com.read_bytes()[0x01CC:0x01CC + len(body)] != body:
        raise ValueError("COM body differs from original bytes")
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
        raise ValueError(f"native output incomplete for SP={sp:04X}")
    return result


def compare_case(result, sp, count, cycle, method):
    observed = struct.unpack_from("<14H", result)
    cs, return_ip, ss = observed[8], observed[9], observed[12]
    code_seg = (cs + 0x1000) & 0xFFFF
    if ss != (cs + 0x2000) & 0xFFFF:
        raise ValueError("native stack segment differs")
    ram = bytearray(RAM_SIZE)
    for off in OFFSETS:
        ram[physical(ss, off)] = 0xA5
    ram[(physical(ss, 0xFFFF) + 1) & MASK] = 0xA5
    put16(ram, ss, sp, return_ip)
    put16(ram, ss, sp + 2, cs)
    put16(ram, ss, sp + 4, 0x4000)
    put16(ram, ss, sp + 6, cs)
    put16(ram, ss, sp + 8, count)
    for off, next_off, next_seg in (
        (0x4000, 0x4100, cs),
        (0x4100, 0x4200, cs),
        (0x4200, 0x4100 if cycle else 0, cs if cycle else 0),
    ):
        put16(ram, cs, off + 0x2A, next_off)
        put16(ram, cs, off + 0x2C, next_seg)
    put16(ram, ss, sp - 2, code_seg)
    put16(ram, ss, sp - 4, 0)
    cpu = Cpu(0xABCD, 0x1357, 0x2468, 0x3579, 0x468A, 0x579B,
              0x68AC, sp, code_seg, 0, cs, cs, ss, 0x0002)
    memory = (ctypes.c_uint8 * RAM_SIZE).from_buffer(ram)
    if method(memory, RAM_SIZE, ctypes.byref(cpu)) != 0:
        raise ValueError("C rejected native entry frame")
    for (name, _), value in zip(Cpu._fields_, observed):
        if getattr(cpu, name) != value:
            raise ValueError(f"SP={sp:04X} count={count} {name}: "
                             f"C={getattr(cpu,name):04X} native={value:04X}")
    expected_stack = bytes(ram[physical(ss, off)] for off in OFFSETS)
    if expected_stack != result[28:28 + len(OFFSETS)]:
        raise ValueError(f"SP={sp:04X} count={count} stack differs")
    cross = ram[(physical(ss, 0xFFFF) + 1) & MASK]
    if cross != result[28 + len(OFFSETS)]:
        raise ValueError(f"SP={sp:04X} count={count} cross byte differs")
    expected_links = bytes(ram[physical(cs, off)] for off in
                           (0x402A, 0x402B, 0x402C, 0x402D,
                            0x412A, 0x412B, 0x412C, 0x412D,
                            0x422A, 0x422B, 0x422C, 0x422D))
    if expected_links != result[-12:]:
        raise ValueError(f"SP={sp:04X} count={count} links differ")


def main():
    directory = ROOT / "resources/u3b0-0000-parity/native"
    directory.mkdir(parents=True, exist_ok=True)
    emulator_path = Path(shutil.which("dosbox-x") or "")
    if not emulator_path.is_file():
        raise FileNotFoundError("dosbox-x executable not found")
    emulator = emulator_path.read_bytes()
    body = body_bytes()
    method = model_library(directory)
    cases = []
    for sp, count, cycle in CASES:
        case_dir = directory / f"sp{sp:04x}-n{count}-c{cycle}"
        result = native_case(case_dir, body, sp, count, cycle)
        compare_case(result, sp, count, cycle, method)
        cases.append({"sp": f"{sp:04X}", "count": count, "cycle": cycle,
                      "result_sha256": sha256(result).hexdigest()})
    report = {"target": "OVL_03B0_0000", "original_body_sha256": LEAF_SHA,
              "dosbox_core": "normal", "dosbox_cpu": "386",
              "dosbox_executable_bytes": len(emulator),
              "dosbox_executable_sha256": sha256(emulator).hexdigest(),
              "synthetic_native_cases": len(cases),
              "compared_cpu_words": 14,
              "compared_stack_bytes": len(OFFSETS) + 1,
              "compared_link_bytes": 12,
              "reached_game_caller": False, "cases": cases}
    (directory / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
