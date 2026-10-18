#!/usr/bin/env python3
"""DOSBox-X original-byte U9E entry 17/C parity at RETF stack wrap.

All generated COM files, original-byte slices and native results stay under
ignored resources/. These are synthetic entry frames, not saved-game reach.
The harness structure follows u9e_entry16_10e4_native.py.
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
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
LEAF_SHA = "63aaf31a1bb13ac3a652db1b1fef79afa6ddf2aee869fe426bc4bda7e5395830"
OFFSETS = tuple(range(0xFFF6, 0x10000)) + tuple(range(0x0000, 0x000E))
RESULT_SIZE = 28 + len(OFFSETS) + 1 + 3
CASES = ((0xFFFE, 0x65), (0xFFFE, 0x66),
         (0xFFFF, 0x65), (0xFFFF, 0x66))


class Cpu(ctypes.Structure):
    _fields_ = [(name, ctypes.c_uint16) for name in
                "ax bx cx dx si di bp sp cs ip ds es ss flags".split()]


def physical(seg, off):
    return ((seg << 4) + (off & 0xFFFF)) & (RAM_SIZE - 1)


def put16(ram, seg, off, value):
    at = physical(seg, off)
    ram[at] = value & 255
    ram[(at + 1) & (RAM_SIZE - 1)] = (value >> 8) & 255


def original_body():
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    if len(ovr) != 206105 or sha256(ovr).hexdigest() != OVR_SHA:
        raise ValueError("original GAME.OVR identity differs")
    body = ovr[0x2BFF7:0x2C046]
    if len(body) != 79 or sha256(body).hexdigest() != LEAF_SHA:
        raise ValueError("U9E entry 17 original slice differs")
    return body


def model_library(directory):
    shared = directory / "u9e_entry17.so"
    subprocess.run([
        "cc", "-shared", "-fPIC", "-std=c11", "-Wall", "-Wextra", "-Werror",
        "-I", str(ROOT / "include"),
        str(ROOT / "src/recovered/overlay_u9e_entry17_110d.c"),
        "-o", str(shared),
    ], check=True)
    method = ctypes.CDLL(str(shared)).pr_u9e_entry17_110d
    method.argtypes = (ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                       ctypes.POINTER(Cpu))
    method.restype = ctypes.c_int
    return method


def assembly(body_path, sp, source_byte):
    lines = [
        ".intel_syntax noprefix", ".code16", ".global _start",
        "_start: jmp main", ".org 0x100d", "leaf:",
        f'.incbin "{body_path.resolve()}"',
        ".org 0x1f00", "main:",
        "mov ax,cs", "mov ds,ax", "mov es,ax", "mov ss,ax", "mov sp,0xf000",
        "mov dx,offset filename", "xor cx,cx", "mov ah,0x3c", "int 0x21",
        "jc failure", "mov word ptr cs:[handle],ax",
        "cli", "mov ax,cs", "add ax,0x1000", "mov ss,ax", "mov es,ax",
        "mov sp,0x8000", "xor di,di", "mov cx,0x0010", "mov al,0xa5",
        "rep stosb", "mov di,0xfff0", "mov cx,0x0010", "rep stosb",
        "mov ax,ss", "add ax,0x1000", "mov es,ax",
        "mov byte ptr es:[0],0xa5",
        f"mov di,0x{sp:04x}",
        "mov word ptr ss:[di],offset returned",
        "mov ax,cs", "mov word ptr ss:[di+2],ax",
        "mov word ptr ss:[di+4],0x4000",
        "mov word ptr ss:[di+6],ax",
        "mov word ptr ss:[di+8],0x4100",
        "mov word ptr ss:[di+10],ax",
        "mov word ptr ss:[di+12],0x4200",
        "mov word ptr ss:[di+14],ax",
        "mov byte ptr cs:[0x4000],0xaa",
        "mov byte ptr cs:[0x4100],0xbb",
        f"mov byte ptr cs:[0x4203],0x{source_byte:02x}",
        "mov es,ax", "mov ax,0x0002", "push ax", "popf",
        "mov ax,0xabcd", "mov bx,0x1357", "mov cx,0x2468",
        "mov dx,0x3579", "mov si,0x468a", "mov di,0x579b",
        "mov bp,0x68ac", f"mov sp,0x{sp:04x}", "jmp leaf", "returned:",
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
    lines += [
        "pushf", "pop word ptr cs:[output+26]",
        "mov ax,ss", "add ax,0x1000", "mov es,ax",
        f"mov al,byte ptr es:[0]",
        f"mov byte ptr cs:[output+{28+len(OFFSETS)}],al",
        "mov al,byte ptr cs:[0x4000]",
        f"mov byte ptr cs:[output+{29+len(OFFSETS)}],al",
        "mov al,byte ptr cs:[0x4100]",
        f"mov byte ptr cs:[output+{30+len(OFFSETS)}],al",
        "mov al,byte ptr cs:[0x4203]",
        f"mov byte ptr cs:[output+{31+len(OFFSETS)}],al",
        "mov ax,cs", "mov ss,ax", "mov sp,0xf000", "sti",
        "mov bx,word ptr cs:[handle]", "mov dx,offset output",
        f"mov cx,{RESULT_SIZE}", "mov ah,0x40", "int 0x21",
        "jc failure", f"cmp ax,{RESULT_SIZE}", "jne failure",
        "mov bx,word ptr cs:[handle]", "mov ah,0x3e", "int 0x21",
        "mov ax,0x4c00", "int 0x21",
        "failure:", "mov ax,0x4c01", "int 0x21",
        'filename: .asciz "RESULT.BIN"', "handle: .word 0",
        f"output: .space {RESULT_SIZE}",
    ]
    return "\n".join(lines) + "\n"


def native_case(directory, body, sp, source_byte):
    directory.mkdir(parents=True, exist_ok=True)
    body_path = directory / "body.bin"
    body_path.write_bytes(body)
    source = directory / "oracle.S"
    obj = directory / "oracle.o"
    com = directory / "ORACLE.COM"
    source.write_text(assembly(body_path, sp, source_byte))
    subprocess.run(["as", "--32", str(source), "-o", str(obj)],
                   check=True, capture_output=True, text=True)
    subprocess.run(["ld", "-m", "elf_i386", "-Ttext", "0x100",
                    "--oformat", "binary", str(obj), "-o", str(com)],
                   check=True, capture_output=True, text=True)
    code = com.read_bytes()
    if code[0x100D:0x100D + 79] != body:
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


def compare_case(result, sp, source_byte, method):
    observed = struct.unpack_from("<14H", result)
    cs, return_ip, ss = observed[8], observed[9], observed[12]
    if ss != (cs + 0x1000) & 0xFFFF:
        raise ValueError("native stack segment differs")
    ram = bytearray(RAM_SIZE)
    for off in OFFSETS:
        ram[physical(ss, off)] = 0xA5
    ram[(physical(ss, 0xFFFF) + 1) & (RAM_SIZE - 1)] = 0xA5
    for i, value in enumerate((return_ip, cs, 0x4000, cs,
                               0x4100, cs, 0x4200, cs)):
        put16(ram, ss, (sp + 2 * i) & 0xFFFF, value)
    ram[physical(cs, 0x4000)] = 0xAA
    ram[physical(cs, 0x4100)] = 0xBB
    ram[physical(cs, 0x4203)] = source_byte
    cpu = Cpu(0xABCD, 0x1357, 0x2468, 0x3579, 0x468A, 0x579B,
              0x68AC, sp, cs, 0x110D, cs, cs, ss, 0x0002)
    memory = (ctypes.c_uint8 * RAM_SIZE).from_buffer(ram)
    if method(memory, RAM_SIZE, ctypes.byref(cpu)) != 0:
        raise ValueError("C rejected native entry frame")
    for (name, _), value in zip(Cpu._fields_, observed):
        if getattr(cpu, name) != value:
            raise ValueError(f"SP={sp:04X} source={source_byte:02X} {name}: "
                             f"C={getattr(cpu,name):04X} native={value:04X}")
    expected_stack = bytes(ram[physical(ss, off)] for off in OFFSETS)
    if expected_stack != result[28:28 + len(OFFSETS)]:
        raise ValueError(f"SP={sp:04X} source={source_byte:02X} stack bytes differ")
    cross = ram[(physical(ss, 0xFFFF) + 1) & (RAM_SIZE - 1)]
    if cross != result[28 + len(OFFSETS)]:
        raise ValueError(f"SP={sp:04X} source={source_byte:02X} cross byte differs")
    data = bytes(ram[physical(cs, off)] for off in (0x4000, 0x4100, 0x4203))
    if data != result[-3:]:
        raise ValueError(f"SP={sp:04X} source={source_byte:02X} data bytes differ")


def main():
    directory = ROOT / "resources/u9e-110d-parity/native"
    directory.mkdir(parents=True, exist_ok=True)
    emulator_path = Path(shutil.which("dosbox-x") or "")
    if not emulator_path.is_file():
        raise FileNotFoundError("dosbox-x executable not found")
    emulator = emulator_path.read_bytes()
    body = original_body()
    method = model_library(directory)
    cases = []
    for sp, source_byte in CASES:
        case_dir = directory / f"sp{sp:04x}-v{source_byte:02x}"
        result = native_case(case_dir, body, sp, source_byte)
        compare_case(result, sp, source_byte, method)
        cases.append({"sp": f"{sp:04X}", "source": f"{source_byte:02X}",
                      "result_sha256": sha256(result).hexdigest()})
    report = {"target": "OVL_009E_110D", "original_body_sha256": LEAF_SHA,
              "dosbox_core": "normal", "dosbox_cpu": "386",
              "dosbox_executable_bytes": len(emulator),
              "dosbox_executable_sha256": sha256(emulator).hexdigest(),
              "synthetic_native_cases": len(cases),
              "compared_cpu_words": 14,
              "compared_stack_bytes": len(OFFSETS) + 1,
              "compared_data_bytes": 3,
              "reached_game_caller": False, "cases": cases}
    (directory / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
