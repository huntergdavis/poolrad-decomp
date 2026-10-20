#!/usr/bin/env python3
"""DOSBox-X frames for original U38:+12B2 and its +0C06 callee."""

import argparse
import ctypes
from hashlib import sha256
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile

import check_overlay_u38_effect_match_12b2 as audit

CHECKER_ROWS = ("OVL_0038_12B2",)

ROOT = Path(__file__).resolve().parents[2]
FLAG_PROFILES = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
COUNTER_EDGES = (0x00, 0x01, 0x7F, 0x80, 0xFE, 0xFF)


def case_table():
    """(B[3], A[10Eh], [6816], [6822], FLAGS): both branches over B[3]/A[10Eh] pairs, counter edges and flag profiles."""
    out = []
    for n, (b3, a) in enumerate((b, a) for b in (0x00, 0x10, 0xEF, 0xFF, 0x0F, 0x1F) for a in (0, 1, 0x10, 0xFF)):
        out.append((b3, a, 0x7F, 0xFF, FLAG_PROFILES[n % 6]))
    for n, c in enumerate(COUNTER_EDGES):
        out += [(0x10, 1, c, COUNTER_EDGES[-1 - n], 0x0002), (0x10, 0, c, COUNTER_EDGES[-1 - n], 0x0002)]
    for f in FLAG_PROFILES:
        out += [(0x00, 0, 0x80, 0x00, f), (0x00, 1, 0x00, 0x80, f)]
    return out


CASES = case_table()
# Compared per case: CPU14, then three whole regions refilled with 5Ah before each case (interrupts off from the fill
# until the record is taken). The harness's own code and buffers (0100..05FF wrapper/leaf images, 3100.. main) are not in them.
REGIONS = ((0x2000, 0x1000), (0x6000, 0x1000), (0xE800, 0x0800))
RECORD_SIZE = 28 + sum(n for _, n in REGIONS)


def build(directory, source_bytes):
    body, leaf = source_bytes
    for name, content in (("wrapper.bin", body), ("leaf.bin", leaf)):
        (directory / name).write_bytes(content)
    asm = [
        ".intel_syntax noprefix", ".code16", ".global _start",
        "_start: jmp main", ".org 0x0b06",
        '.incbin "' + str(directory / "leaf.bin") + '"',
        ".org 0x11b2",
        '.incbin "' + str(directory / "wrapper.bin") + '"',
        ".org 0x3000", "main:",
        "mov ax,cs", "mov ds,ax", "mov es,ax", "mov ss,ax",
        "mov sp,0xf800", "mov word ptr [outer_segment],ax",
        "mov word ptr [header],ax",
        "mov word ptr [header+2],offset returned",
        f"mov word ptr [header+4],{len(CASES)}",
        "mov dx,offset filename", "xor cx,cx", "mov ah,0x3c",
        "int 0x21", "jc failure", "mov word ptr [handle],ax",
        "mov bx,ax", "mov dx,offset header", "mov cx,6",
        "mov ah,0x40", "int 0x21", "again:",
        "cli", "cld", "mov ax,cs", "mov es,ax", "mov al,0x5a"]
    for base, n in REGIONS:
        asm += [f"mov di,{base}", f"mov cx,{n}", "rep stosb"]
    asm += ["mov si,word ptr [index]", "mov ax,si", "shl si,1", "shl si,1", "shl si,1", "add si,ax", "add si,ax",
            "add si,offset table",  # 10 bytes per case
            "mov al,byte ptr [si]", "mov byte ptr [0x2203],al", "mov al,byte ptr [si+2]", "mov byte ptr [0x220e],al",
            "mov al,byte ptr [si+4]", "mov byte ptr [0x6816],al", "mov al,byte ptr [si+6]", "mov byte ptr [0x6822],al",
            "mov sp,0xf000", "push word ptr [si+8]", "popf",
            "mov ax,0xa5cd", "mov bx,0x3456", "mov cx,0x5678",
            "mov dx,0x789a", "mov si,0x9abc", "mov di,0xbcde",
            "mov bp,0x7777", "push cs", "push 0x2100",
            "push cs", "push 0x2200", "push 0xa145",
            ".byte 0x9a", ".word 0x12b2",
            "outer_segment: .word 0", "returned:"]
    for i, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        asm.append(f"mov word ptr cs:[output+{2*i}],{reg}")
    asm += ["mov word ptr cs:[output+18],offset returned",
            "mov word ptr cs:[output+20],ds", "mov word ptr cs:[output+22],es",
            "mov word ptr cs:[output+24],ss", "mov sp,0xf800",
            "pushf", "pop ax", "mov word ptr cs:[output+26],ax",
            "mov ax,cs", "mov ds,ax", "cld", "sti",
            "mov bx,word ptr [handle]", "mov dx,offset output", "mov cx,28", "mov ah,0x40", "int 0x21",
            "jc failure", "cmp ax,28", "jne failure"]
    for base, n in REGIONS:
        asm += ["mov bx,word ptr [handle]", f"mov dx,{base}", f"mov cx,{n}", "mov ah,0x40", "int 0x21",
                "jc failure", f"cmp ax,{n}", "jne failure"]
    asm += ["inc word ptr [index]", f"cmp word ptr [index],{len(CASES)}",
            "jae done", "jmp again", "done:", "mov bx,word ptr [handle]", "mov ah,0x3e",
            "int 0x21", "mov ax,0x4c00", "int 0x21", "failure:",
            "mov ax,0x4c01", "int 0x21", 'filename: .asciz "RESULT.BIN"',
            "header: .space 6", "handle: .word 0", "index: .word 0",
            "output: .space 28", "table:"]
    for row in CASES:
        asm.append(".word " + ",".join(str(v) for v in row))
    assembly, obj, com = (directory / "oracle.S", directory / "oracle.o",
                          directory / "ORACLE.COM")
    assembly.write_text("\n".join(asm) + "\n")
    subprocess.run(["as", "--32", str(assembly), "-o", str(obj)], check=True)
    subprocess.run(["ld", "-m", "elf_i386", "-Ttext", "0x100",
                    "--oformat", "binary", str(obj), "-o", str(com)], check=True)
    assert com.stat().st_size + 0x100 < 0x6000, "COM reaches the compared counter region"
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
        subprocess.run(["/usr/bin/dosbox-x", "-conf", str(config), "-fastlaunch",
                        "-exit", "-noconsole"],
                       env=dict(os.environ, SDL_VIDEODRIVER="dummy",
                                SDL_AUDIODRIVER="dummy"),
                       stdout=log, stderr=subprocess.STDOUT,
                       check=True, timeout=300)
    return result.read_bytes()


def build_model(directory):
    library = directory / "model.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                    "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                    str(ROOT / "src/recovered/overlay_u38_effect_match_12b2.c"),
                    "-o", str(library)], check=True)
    model = ctypes.CDLL(str(library)).pr_u38_effect_match_12b2
    model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                      ctypes.POINTER(audit.helper.Cpu)]
    model.restype = ctypes.c_int
    return model


def compare(data, com):
    assert len(data) == 6 + len(CASES) * RECORD_SIZE
    seg, ret_ip, count = struct.unpack_from("<3H", data)
    assert count == len(CASES)
    equal = 0
    with tempfile.TemporaryDirectory(prefix="u38-12b2-native-") as tmp:
        model = build_model(Path(tmp))
        for n, (b3, a10e, c1, c2, flags) in enumerate(CASES):
            expected = data[6 + n * RECORD_SIZE:6 + (n + 1) * RECORD_SIZE]
            memory = bytearray(audit.helper.RAM_SIZE)
            at = audit.helper.physical(seg, 0x100)
            memory[at:at + len(com)] = com
            for base, size in REGIONS:
                start = audit.helper.physical(seg, base)
                memory[start:start + size] = b"\x5a" * size
            for i, word in enumerate((ret_ip, seg, 0xA145, 0x2200, seg, 0x2100, seg)):
                audit.put16(memory, seg, 0xEFF2 + 2 * i, word)
            for off, value in ((0x2203, b3), (0x220E, a10e), (0x6816, c1), (0x6822, c2)):
                memory[audit.helper.physical(seg, off)] = value
            cpu = audit.helper.Cpu(0xA5CD, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, 0xEFF2,
                                   seg, 0x12B2, seg, seg, seg, flags)
            ram = (ctypes.c_uint8 * audit.helper.RAM_SIZE).from_buffer(memory)
            assert model(ram, audit.helper.RAM_SIZE, ctypes.byref(cpu)) == 0, n
            observed = struct.unpack_from("<14H", expected)
            for (name, _), word in zip(audit.helper.Cpu._fields_, observed):
                assert getattr(cpu, name) == word, (n, name, hex(getattr(cpu, name)), hex(word))
            offset = 28
            for base, size in REGIONS:
                start = audit.helper.physical(seg, base)
                mine = bytes(memory[start:start + size])
                if mine != expected[offset:offset + size]:
                    diff = [hex(base + i) for i, (x, y) in enumerate(zip(mine, expected[offset:offset + size])) if x != y]
                    raise AssertionError((n, "region", hex(base), diff[:8], len(diff)))
                offset += size
            equal += int(a10e == (b3 & 0x10) >> 4)
    return equal


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compile-only", action="store_true",
                        help="compile, load and bind the actual C model without media")
    args = parser.parse_args()
    if args.compile_only:
        with tempfile.TemporaryDirectory() as temp:
            build_model(Path(temp))
        print("CHECKER_BUILD_OK")
        return
    directory = ROOT / "resources/u38-12b2-parity/native"
    directory.mkdir(parents=True, exist_ok=True)
    source_bytes = audit.source()
    com = build(directory, source_bytes)
    data = run_native(directory)
    equal = compare(data, com.read_bytes())
    binary = "/usr/bin/dosbox-x"
    probe = subprocess.run([binary, "--version"], capture_output=True, text=True,
                           env=dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy"))
    version = next(line.split(",")[0].strip() for line in (probe.stdout + probe.stderr).splitlines() if "DOSBox-X version" in line)
    report = dict(target="OVL_0038_12B2", original_sha256=audit.BODY_SHA,
                  callee_sha256=audit.LEAF_SHA, native_cases=len(CASES), equal_branch_cases=equal,
                  original_callee_composed=True, compared_cpu_words=14,
                  compared_region_bytes=sum(n for _, n in REGIONS), compared_regions=[f"{b:04X}+{n:X}" for b, n in REGIONS],
                  flag_masks=0, result_sha256=sha256(data).hexdigest(),
                  dosbox_x=dict(path=binary, sha256=sha256(open(binary, "rb").read()).hexdigest(), version=version,
                                pinned_source="5a9c7c53481c2e933948039c33256776653f4faf", pinned_build_used=False))
    (directory / "report.json").write_text(json.dumps(report, sort_keys=True,
                                                       indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
