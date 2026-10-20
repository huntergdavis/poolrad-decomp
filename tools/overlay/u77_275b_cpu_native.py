#!/usr/bin/env python3
"""U77 +275B DOSBox-X corroboration with DECLARED STUB substitutions.

DECLARATION (read this before reading the numbers):
1. Stubbed endpoints: **00BA:0BB8** and **00BA:13AB**, at their entry points,
   substituted with `RETF 4` stubs.
2. The stub does **not** execute their closure, so the residents' internal
   SYSTEM endpoints are **not exercised by this run** - only by the differential.
3. Division of labour: the differential (29 fixtures, 14 CPU words, all
   1,048,576 RAM bytes, successor accounting, asserted exclusions) carries the
   **composition** claim; this run corroborates the **caller's 132 bytes**
   against the original instruction stream with **synthetic entry frames**.
4. Coverage reduction: this set is **28 of 31** fixtures. The three non-default-SP
   cases are excluded **by construction** because this capture does not vary SP;
   closing them needs assembly that sets SP per case.
5. Frame: this run uses its **own declared frame** (return IP 0x0000, CS 0xDEAD,
   then the record and target far pointers at [bp+6]/[bp+0x0A]). It is **not
   asserted** to be byte-identical to the differential's fixture frame.

Compared: 12 CPU words (CS and IP are the harness's frame), the arithmetic flags
under mask 0x0FD5, and both 0x300-byte object blocks.
"""
import ctypes
import os
import struct
import subprocess
import sys
from hashlib import sha256
from pathlib import Path

import u77_275b_cpu_audit as audit

ROOT = Path(__file__).resolve().parents[2]
CS = 0x4000          # dedicated caller segment (the COM loads the body here)
ENTRY = 0x275B
RESIDENT = 0x00BA
STACK_SEGMENT = 0x2000
STACK_TOP = 0x8000
DEFAULT_SP = 0x7FF4      # post-CALL SP, matching the differential fixture
DECLARED_RETURN_CS = 0xDEAD   # declared, and identical on the C side
DECLARED_DS = 0x1000          # fixture DS; the body never reads it
COMPARED_FLAGS = 0x0FD5       # CF PF AF ZF SF TF IF DF OF; IOPL/NT (0x7000) are
                              # 386 CPU-mode state the body never computes
OBJECT_BYTES = 0x300   # covers +0x102 base, +0x37/+0x39 fields and +0xC7 type
TARGET_SEGMENT, TARGET_OFFSET = 0x3000, 0x0100
RECORD_SEGMENT, RECORD_OFFSET = 0x4000, 0x0200
HEADER = 8            # file header: case count word + pad
PER_CASE = 28 + 2 * OBJECT_BYTES
RECORD_BASE = 0x5000  # reserved in the COM's segment, clear of code and blobs
STUB_BYTES = bytes((0xCA, 0x04, 0x00))          # RETF 4


def cases():
    """27 of the differential's 29: the two SP-variant fixtures are excluded."""
    return [(name, spec) for name, spec in audit.cases()
            if spec.get("sp", DEFAULT_SP) == DEFAULT_SP]


def library(directory):
    sources = [str(ROOT / "src/recovered/overlay_0077_275b_cpu.c"),
               str(ROOT / "tools/overlay/u77_275b_native_stubs.c")]
    target = Path(directory) / "u77_275b_stub.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                    "-O2", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                    *sources, "-o", str(target)], check=True)
    lib = ctypes.CDLL(str(target))
    lib.pr_ovl0077_275b_cpu.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                                        ctypes.POINTER(audit.Cpu),
                                        ctypes.POINTER(ctypes.c_size_t),
                                        ctypes.POINTER(audit.Diagnostic)]
    lib.pr_ovl0077_275b_cpu.restype = ctypes.c_int
    return lib


def build_com(directory, body, specs):
    fixture_input = bytearray()
    for name, spec in specs:
        _, ram, _cpu = audit.case(name, **spec)
        frame = struct.pack("<4H",
                            TARGET_OFFSET, TARGET_SEGMENT,
                            RECORD_OFFSET, RECORD_SEGMENT)
        target = ram[audit.physical(TARGET_SEGMENT, 0):audit.physical(TARGET_SEGMENT, 0) + OBJECT_BYTES]
        record = ram[audit.physical(RECORD_SEGMENT, 0):audit.physical(RECORD_SEGMENT, 0) + OBJECT_BYTES]
        fixture_input += frame + struct.pack("<H", spec.get("flags", 0x0202)) \
            + bytes(target) + bytes(record)
    (directory / "INPUT.BIN").write_bytes(bytes(fixture_input))

    code = [".code16", ".intel_syntax noprefix", ".global _start", "_start:",
            "    jmp main", ".org 0x200", "main:",
            "    push cs", "    pop ds", "    push cs", "    pop es",
            f"    mov ax,{CS:#x}", "    mov es,ax", f"    mov di,{ENTRY:#x}",
            "    mov si,offset body", f"    mov cx,{len(body)}", "    rep movsb",
            "    push cs", "    pop ds", "    push cs", "    pop es",
            # declared stubs at both resident entry points
            f"    mov ax,{RESIDENT:#x}", "    mov es,ax"]
    for offset in (0x0BB8, 0x13AB):
        code += [f"    mov di,{offset:#x}", "    mov byte ptr es:[di],0xca",
                 "    mov byte ptr es:[di+1],0x04", "    mov byte ptr es:[di+2],0x00"]
    code += ["    mov dx,offset input_name", "    mov ax,0x3d00", "    int 0x21", "    jc failure",
             "    mov word ptr cs:[input_handle],ax",
             "    mov dx,offset output_name", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21",
             "    jc failure", "    mov word ptr cs:[output_handle],ax",
             f"    mov word ptr cs:[file_header],{len(specs)}",
             "    push cs", "    pop ds",
             "    mov bx,word ptr cs:[output_handle]", "    mov ah,0x40", "    mov cx,8",
             "    mov dx,offset file_header", "    int 0x21",
             f"    mov word ptr cs:[index],0",
             "again:",
             "    push cs", "    pop ds", "    push cs", "    pop es",
             "    mov dx,offset frame_words", "    mov cx,8", "    call read_input",
             "    mov dx,offset frame_flags", "    mov cx,2", "    call read_input",
             "    mov dx,offset object_target", f"    mov cx,{OBJECT_BYTES}", "    call read_input",
             "    mov dx,offset object_record", f"    mov cx,{OBJECT_BYTES}", "    call read_input",
             f"    mov ax,{TARGET_SEGMENT:#x}", "    mov es,ax", "    mov si,offset object_target",
             f"    mov di,0", f"    mov cx,{OBJECT_BYTES}", "    rep movsb",
             f"    mov ax,{RECORD_SEGMENT:#x}", "    mov es,ax", "    mov si,offset object_record",
             f"    mov di,0", f"    mov cx,{OBJECT_BYTES}", "    rep movsb",
             "    push cs", "    pop ds",
             # Switch to the declared stack FIRST, then push the arguments so they
             # land where the body reads them: [bp+6]=record offset/segment and
             # [bp+0x0A]=target offset/segment, i.e. target pushed first.
             f"    mov ax,{STACK_SEGMENT:#x}", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}",
             "    mov ax,word ptr cs:[frame_words+2]", "    push ax",   # target segment (first pushed)
             "    mov ax,word ptr cs:[frame_words+0]", "    push ax",   # target offset
             "    mov ax,word ptr cs:[frame_words+6]", "    push ax",   # record segment
             "    mov ax,word ptr cs:[frame_words+4]", "    push ax",   # record offset

             "    push cs", "    pop ds", "    push cs", "    pop es",
             f"    mov ax,{CS:#x}", "    mov ds,ax",
             # declared entry register file, mirroring the differential fixture
             "    xor ax,ax", "    xor bx,bx", "    xor cx,cx", "    xor dx,dx",
             "    xor si,si", "    xor di,di", f"    mov bp,{0x7F00:#x}",
             "    push word ptr cs:[frame_flags]", "    popf",   # declared entry flags
             f"    mov ax,{DECLARED_DS:#x}", "    mov ds,ax",   # declared entry DS, last moment
             "    call dword ptr cs:[target]",   # return CS/IP are the harness frame
             "returned:",]
    for index, register in enumerate("ax bx cx dx si di bp sp".split()):
        code.append(f"    mov word ptr cs:[{RECORD_BASE+2*index:#x}],{register}")
    for offset, register in ((16, "cs"), (20, "ds"), (22, "es"), (24, "ss")):
        code.append(f"    mov word ptr cs:[{RECORD_BASE+offset:#x}],{register}")
    # IP cannot be moved to memory; record the return label instead and declare
    # that IP is therefore NOT compared (the frames differ by construction).
    code.append(f"    mov word ptr cs:[{RECORD_BASE+18:#x}],offset returned")
    code += ["    pushf", "    pop ax", f"    mov word ptr cs:[{RECORD_BASE+26:#x}],ax",
             "    push cs", "    pop ds",        # restore DS AFTER the capture
             "    cld",                          # a declared entry DF would break our own copies
             "    push cs", "    pop ds", "    push cs", "    pop es",
             f"    mov ax,{TARGET_SEGMENT:#x}", "    mov ds,ax", "    mov si,0",
             f"    mov di,{RECORD_BASE+28:#x}", f"    mov cx,{OBJECT_BYTES}", "    rep movsb",
             f"    mov ax,{RECORD_SEGMENT:#x}", "    mov ds,ax", "    mov si,0",
             f"    mov cx,{OBJECT_BYTES}", "    rep movsb",
             "    push cs", "    pop ds", "    push cs", "    pop es",
             f"    mov dx,{RECORD_BASE:#x}", f"    mov cx,{PER_CASE}", "    call write_output",
             "    push cs", "    pop ds", "    inc word ptr cs:[index]",
             f"    cmp word ptr cs:[index],{len(specs)}", "    jb again",
             "    mov bx,word ptr cs:[output_handle]", "    mov ah,0x3e", "    int 0x21",
             "    mov ax,0x4c00", "    int 0x21",
             "read_input:", "    mov bx,word ptr cs:[input_handle]", "    mov ah,0x3f", "    int 0x21",
             "    jc failure", "    cmp ax,cx", "    jne failure", "    ret",
             "mark:", "    push ax", "    push bx", "    push cx", "    push dx", "    push ds",
             "    push cs", "    pop ds",
             "    mov byte ptr cs:[mark_byte],al",   # write the value BEFORE DX is reloaded
             "    mov bx,word ptr cs:[output_handle]", "    mov ah,0x40", "    mov cx,1",
             "    mov dx,offset mark_byte", "    int 0x21",
             "    pop ds", "    pop dx", "    pop cx", "    pop bx", "    pop ax", "    ret",
             "mark_byte: .byte 0",
             "write_output:", "    mov bx,word ptr cs:[output_handle]", "    mov ah,0x40", "    int 0x21",
             "    jc failure", "    cmp ax,cx", "    jne failure", "    ret",
             "failure:", "    mov ax,0x4c01", "    int 0x21",
             'input_name: .asciz "INPUT.BIN"', 'output_name: .asciz "RESULT.BIN"',
             "input_handle: .word 0", "output_handle: .word 0", "index: .word 0",
             "file_header: .word 0,0,0,0",
             "frame_words: .word 0,0,0,0", "frame_flags: .word 0",
             f"target: .word {ENTRY:#x},{CS:#x}"]
    at = 0x6000
    for label, blob in (("body", body), ("object_target", bytes(OBJECT_BYTES)),
                        ("object_record", bytes(OBJECT_BYTES))):
        code += [f".org {at-0x100:#x}", label + ":"]
        for index in range(0, len(blob), 16):
            code.append("    .byte " + ",".join(hex(x) for x in blob[index:index + 16]))
        at = (at + len(blob) + 15) & ~15
    audit.check(at < 0xD000, "COM code overlaps capture")
    asm = directory / "oracle.S"
    obj = directory / "oracle.o"
    asm.write_text("\n".join(code) + "\n")
    subprocess.run(["as", "--32", str(asm), "-o", str(obj)], check=True)
    subprocess.run(["ld", "-m", "elf_i386", "-Ttext", "0x100", "--oformat", "binary",
                    str(obj), "-o", str(directory / "ORACLE.COM")], check=True)


def run_dosbox(directory):
    config = directory / "dosbox.conf"
    config.write_text("[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n[cpu]\ncore=normal\n"
                      "cputype=386\ncycles=max\n[mixer]\nnosound=true\n[midi]\nmididevice=none\n"
                      "[autoexec]\nmount c " + str(directory.resolve()) + "\nc:\nORACLE.COM\nexit\n")
    output = directory / "RESULT.BIN"
    if output.exists():
        output.unlink()
    with (directory / "dosbox.log").open("w") as log:
        subprocess.run(["dosbox-x", "-conf", str(config), "-fastlaunch", "-exit", "-noconsole"],
                       env=dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy"),
                       stdout=log, stderr=subprocess.STDOUT, check=True, timeout=600)
    return output.read_bytes()


def native_audit(body):
    directory = ROOT / "resources/oracle-runs/u77-275b-stubbed"
    directory.mkdir(parents=True, exist_ok=True)
    specs = cases()
    build_com(directory, body, specs)
    result = run_dosbox(directory)
    audit.check(len(result) == HEADER + len(specs) * PER_CASE,
                f"incomplete native output {len(result)}")
    with tempfile_library(directory) as lib:
        for index, (name, spec) in enumerate(specs):
            record = result[HEADER + index * PER_CASE:HEADER + (index + 1) * PER_CASE]
            _, ram, cpu = audit.case(name, **spec)
            diagnostic = audit.Diagnostic()
            working = (ctypes.c_uint8 * audit.SIZE).from_buffer_copy(bytes(ram))
            budget = ctypes.c_size_t(1 << 20)
            status = lib.pr_ovl0077_275b_cpu(working, audit.SIZE, ctypes.byref(cpu),
                                             ctypes.byref(budget), ctypes.byref(diagnostic))
            audit.check(status == 0, f"{name}: stubbed C status {status}")
            for (_, field), want in zip(audit.REGISTERS, struct.unpack_from("<14H", record)):
                if field in ("cs", "ip"):
                    continue      # set by the harness's own return frame, not the body
                got = getattr(cpu, field)
                if field == "flags":
                    got &= COMPARED_FLAGS
                    want &= COMPARED_FLAGS
                audit.check(got == want, f"{name} {field}: C={got:04x} DOS={want:04x}")
            produced = (bytes(working[audit.physical(TARGET_SEGMENT, 0):
                                      audit.physical(TARGET_SEGMENT, 0) + OBJECT_BYTES])
                        + bytes(working[audit.physical(RECORD_SEGMENT, 0):
                                        audit.physical(RECORD_SEGMENT, 0) + OBJECT_BYTES]))
            audit.check(produced == record[28:],
                        f"{name}: object blocks differ from the native capture")
    return dict(cases=len(specs), cpu_words=12,
                frame_words_declared_not_compared=("cs", "ip"),
                flag_mask=f"{COMPARED_FLAGS:#06x}",
                object_bytes=2 * OBJECT_BYTES,
                stubbed_endpoints=("00BA:0BB8", "00BA:13AB"),
                frame="declared (synthetic): return IP 0000, CS DEAD",
                excluded_by_construction=("three non-default-SP fixtures",),
                result_sha256=sha256(result).hexdigest())


class tempfile_library:
    """Compile the stub-linked model in a temp dir and clean it up."""

    def __init__(self, directory):
        self.directory = directory

    def __enter__(self):
        import tempfile
        self.temp = tempfile.TemporaryDirectory(prefix="u77-275b-stub-")
        return library(self.temp.name)

    def __exit__(self, *exc):
        self.temp.cleanup()
        return False


def main(argv=None):
    body, _ = audit.source()
    report = native_audit(body)
    print("Native parity (declared stubs):", report)
    return 0


if __name__ == "__main__":
    sys.exit(main())
