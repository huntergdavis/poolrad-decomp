#!/usr/bin/env python3
"""Validates the C EGA planar-mode model (docs/re/EGA_HW_MODEL.md) against DOSBox-X's EGA emulation.

A synthetic COM sets video mode 0Dh on machine=ega and interprets a random script of fixed 4-byte operations: byte OUTs
to the sequencer and graphics-controller ports, CPU byte writes and reads at A000:offset. It records every read and finally
dumps the four planes of the window through Read Map Select. The C model (pr_ega) replays the same script; every read and
every plane byte must be equal. The emulator runs as an external process; no emulator code is involved. Outputs stay
under ignored resources/.
"""
import argparse
import ctypes
from hashlib import sha256
import json
from pathlib import Path
import random
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
WINDOW = 0x100
OPS = 3000
SCRIPT_AT, READS_AT, DUMP_AT, STACK_TOP = 0x1000, 0x6000, 0x7000, 0xF000
OUT, WRITE, READ = 1, 2, 3
RESET = ((2, 0x0F),)  # sequencer
GC_RESET = ((0, 0), (1, 0), (2, 0), (3, 0), (4, 0), (5, 0), (7, 0x0F), (8, 0xFF))


def check(condition, message):
    if not condition:
        raise ValueError(message)


def script(rng):
    ops = []
    for index, value in RESET:
        ops += [(OUT, 0x3C4, index), (OUT, 0x3C5, value)]
    for index, value in GC_RESET:
        ops += [(OUT, 0x3CE, index), (OUT, 0x3CF, value)]
    ops += [(WRITE, offset, 0) for offset in range(WINDOW)] + [(READ, 0, 0)]
    while len(ops) < OPS:
        kind = rng.random()
        if kind < 0.25:
            register = rng.choice((0, 1, 2, 3, 4, 5, 7, 8, "seq"))
            if register == "seq":
                ops += [(OUT, 0x3C4, 2), (OUT, 0x3C5, rng.randrange(16))]
                continue
            value = {0: rng.randrange(16), 1: rng.randrange(16), 2: rng.randrange(16), 3: rng.randrange(32), 4: rng.randrange(4),
                     5: rng.randrange(3) | (rng.randrange(2) << 3), 7: rng.randrange(16), 8: rng.randrange(256)}[register]
            ops += [(OUT, 0x3CE, register), (OUT, 0x3CF, value)]
        elif kind < 0.7:
            ops.append((WRITE, rng.randrange(WINDOW), rng.randrange(256)))
        else:
            ops.append((READ, rng.randrange(WINDOW), 0))
    return ops


def library(directory):
    path = directory / "ega.so"
    built = subprocess.run(["cc", "-std=c11", "-O2", "-fPIC", "-shared", "-I", str(ROOT / "include"), str(ROOT / "src/recovered/ega_hw.c"),
                            "-o", str(path)], capture_output=True, text=True)
    check(built.returncode == 0, built.stderr[-2000:])
    lib = ctypes.CDLL(str(path))
    lib.pr_ega_reset.argtypes = [ctypes.c_void_p]
    lib.pr_ega_out.argtypes = [ctypes.c_void_p, ctypes.c_uint16, ctypes.c_uint16, ctypes.c_uint]
    lib.pr_ega_read.argtypes = [ctypes.c_void_p, ctypes.c_uint16]
    lib.pr_ega_read.restype = ctypes.c_uint8
    lib.pr_ega_write.argtypes = [ctypes.c_void_p, ctypes.c_uint16, ctypes.c_uint8]
    return lib


def replay(lib, ops):
    ega = ctypes.create_string_buffer(4 * 0x10000 + 64)
    lib.pr_ega_reset(ega)
    reads = []
    for kind, a, value in ops:
        if kind == OUT:
            check(lib.pr_ega_out(ega, a, value, 1) == 0, f"C model refused OUT {a:04X},{value:02X}")
        elif kind == WRITE:
            check(lib.pr_ega_write(ega, a, value) == 0, "C model refused a write")
        else:
            reads.append(lib.pr_ega_read(ega, a))
    raw = ega.raw
    return bytes(reads), b"".join(raw[p * 0x10000:p * 0x10000 + WINDOW] for p in range(4))


def native(ops):
    import os
    directory = ROOT / "resources/oracle-runs/ega-hw-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    blob = b"".join(struct.pack("<BHB", k, a, v) for k, a, v in ops) + b"\0"
    reads = sum(1 for k, _, _ in ops if k == READ)
    check(SCRIPT_AT + len(blob) <= READS_AT and READS_AT + reads <= DUMP_AT, "COM layout overlaps")
    t = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main", ".org 0x200", "main:",
         "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}", "    cld",
         "    mov ax,0x000d", "    int 0x10", "    cli", "    mov ax,0xa000", "    mov es,ax",
         "    mov si,offset script", f"    mov bp,{READS_AT:#x}",
         "next:", "    lodsb", "    or al,al", "    jz done", "    mov cl,al", "    lodsw", "    mov bx,ax", "    lodsb",
         "    cmp cl,1", "    je op_out", "    cmp cl,2", "    je op_write",
         "    mov al,byte ptr es:[bx]", "    mov byte ptr [bp],al", "    inc bp", "    jmp next",
         "op_out:", "    mov dx,bx", "    out dx,al", "    jmp next",
         "op_write:", "    mov byte ptr es:[bx],al", "    jmp next",
         "done:", "    sti"]
    for p in range(4):
        t += ["    mov dx,0x3ce", "    mov ax,0x0005", "    out dx,ax", f"    mov ax,{(p << 8) | 4:#x}", "    out dx,ax",
              "    push ds", "    mov ax,0xa000", "    mov ds,ax", "    push cs", "    pop es", "    xor si,si", f"    mov di,{DUMP_AT + p * WINDOW:#x}",
              f"    mov cx,{WINDOW:#x}", "    rep movsb", "    pop ds"]
    t += ["    push cs", "    pop ds", "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure", "    mov bx,ax",
          f"    mov dx,{READS_AT:#x}", f"    mov cx,{reads}", "    mov ah,0x40", "    int 0x21", "    jc failure",
          f"    mov dx,{DUMP_AT:#x}", f"    mov cx,{4 * WINDOW}", "    mov ah,0x40", "    int 0x21", "    jc failure",
          "    mov ah,0x3e", "    int 0x21", "    mov ax,0x0003", "    int 0x10", "    mov ax,0x4c00", "    int 0x21",
          "failure:", "    mov ax,0x4c01", "    int 0x21", 'filename: .asciz "RESULT.BIN"', f".org {SCRIPT_AT - 0x100:#x}", "script:"]
    for i in range(0, len(blob), 16):
        t.append("    .byte " + ",".join(f"{x:#x}" for x in blob[i:i + 16]))
    asm, obj = directory / "oracle.S", directory / "oracle.o"
    asm.write_text("\n".join(t) + "\n")
    subprocess.run(["as", "--32", str(asm), "-o", str(obj)], check=True)
    subprocess.run(["ld", "-m", "elf_i386", "-Ttext", "0x100", "--oformat", "binary", str(obj), "-o", str(directory / "ORACLE.COM")], check=True)
    config = directory / "dosbox.conf"
    config.write_text("[sdl]\noutput=surface\n[dosbox]\nmachine=ega\nmemsize=16\n[cpu]\ncore=normal\ncputype=386\ncycles=max\n"
                      "[mixer]\nnosound=true\n[midi]\nmididevice=none\n"
                      "[autoexec]\nmount c " + str(directory.resolve()) + "\nc:\nORACLE.COM\nexit\n")
    result_path = directory / "RESULT.BIN"
    if result_path.exists():
        result_path.unlink()
    environment = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy")
    with (directory / "dosbox.log").open("w") as log:
        subprocess.run(["dosbox-x", "-conf", str(config), "-fastlaunch", "-exit", "-noconsole"],
                       env=environment, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=600)
    result = result_path.read_bytes()
    check(len(result) == reads + 4 * WINDOW, f"native output incomplete: {len(result)} bytes")
    return result[:reads], result[reads:], sha256(result).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--seed", type=lambda s: int(s, 0), default=0xE6A0)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    ops = script(random.Random(args.seed))
    with tempfile.TemporaryDirectory(prefix="poolrad-egahw-") as temp:
        lib = library(Path(temp))
        mine_reads, mine_planes = replay(lib, ops)
    dos_reads, dos_planes, digest = native(ops)
    bad_reads = [i for i, (x, y) in enumerate(zip(mine_reads, dos_reads)) if x != y]
    bad_planes = [i for i, (x, y) in enumerate(zip(mine_planes, dos_planes)) if x != y]
    kinds = {k: sum(1 for op in ops if op[0] == k) for k in (OUT, WRITE, READ)}
    check(not bad_reads, f"reads differ at {bad_reads[:8]} (C {[mine_reads[i] for i in bad_reads[:4]]}, DOS {[dos_reads[i] for i in bad_reads[:4]]})")
    check(not bad_planes, f"plane bytes differ at {[(i // WINDOW, i % WINDOW) for i in bad_planes[:8]]}")
    print(f"EGA model vs DOSBox-X machine=ega mode 0Dh: {len(ops)} script operations ({kinds[OUT]} OUTs, {kinds[WRITE]} writes, "
          f"{kinds[READ]} reads); all {len(dos_reads)} reads and all {len(dos_planes)} plane bytes equal; result SHA-256 {digest}")
    report = dict(operations=len(ops), outs=kinds[OUT], writes=kinds[WRITE], reads=kinds[READ], plane_bytes=len(dos_planes), result_sha256=digest)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")


if __name__ == "__main__":
    main()
