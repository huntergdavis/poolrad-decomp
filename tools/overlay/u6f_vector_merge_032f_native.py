#!/usr/bin/env python3
"""DOSBox far-call frames for U6F:+032F; synthetic inputs, not saved-game reach.

Adapts the capture-before-DOS-I/O pattern of u6a_0824_synthetic_native.py.
Original bytes, generated COMs, frames and results remain ignored resources/.
"""
import ctypes
from hashlib import sha256
import os
from pathlib import Path
import struct
import subprocess
import tempfile

import u6f_vector_merge_032f_audit as base

CASES, RECORD = 384, 231


def assembly(source):
    text = r'''.code16
.intel_syntax noprefix
.global _start
_start:
    mov ax, cs
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0xf000
    mov word ptr [header], ax
    mov word ptr [header+2], offset returned
    mov word ptr [header+4], offset leaf
    mov word ptr [0x4013], 0x4400
    mov word ptr [0x4015], ax
    mov word ptr [0x4213], 0x4600
    mov word ptr [0x4215], ax
    mov word ptr [0x5eb2], 0x5000
    mov word ptr [0x5eb4], ax
    mov dx, offset filename
    xor cx, cx
    mov ah, 0x3c
    int 0x21
    jc failure
    mov word ptr [handle], ax
    mov bx, ax
    mov dx, offset header
    mov cx, 6
    mov ah, 0x40
    int 0x21
again:
    cld
    mov di, 0x4000
    mov cx, 55
    mov al, 0xa5
    rep stosb
    mov di, 0x4200
    mov cx, 55
    rep stosb
    mov word ptr [0x4013], 0x4400
    mov ax, cs
    mov word ptr [0x4015], ax
    mov word ptr [0x4213], 0x4600
    mov word ptr [0x4215], ax
    mov ax, word ptr [index]
    cmp ax, 256
    jb mode_ready
    sub ax, 256
    xor dx, dx
    mov cx, 3
    div cx
    mov ax, dx
mode_ready:
    mov byte ptr [0x4a1e], al
    mov ax, word ptr [index]
    xor dx, dx
    mov cx, 21
    div cx
    inc dx
    mov word ptr [0x4211], dx
    mov ax, word ptr [index]
    xor dx, dx
    mov cx, 15
    div cx
    inc dx
    shl dx, 2
    mov ax, word ptr [index]
    and ax, 3
    add dx, ax
    mov word ptr [0x5011], dx
    xor si, si
fill:
    mov ax, si
    mov bx, 17
    mul bx
    mov dx, word ptr [index]
    mov cx, dx
    shl dx, 1
    add dx, cx
    add ax, dx
    xor al, 0x5a
    mov byte ptr [si+0x4017], al
    mov ax, si
    mov bx, 13
    mul bx
    add ax, word ptr [index]
    xor al, 0xa5
    mov byte ptr [si+0x4217], al
    mov ax, si
    mov bx, 7
    mul bx
    add ax, word ptr [index]
    xor al, 0xaa
    mov byte ptr [si+0x4400], al
    mov ax, word ptr [index]
    mov bx, 5
    mul bx
    mov cx, ax
    mov ax, si
    mov bx, 11
    mul bx
    add ax, cx
    xor al, 0xf0
    mov byte ptr [si+0x4600], al
    inc si
    cmp si, 32
    jb fill
    cli
    mov di, 0xefec
    mov cx, 20
    mov al, 0xa5
    rep stosb
    mov sp, 0xf000
    push cs
    push 0x4200
    push cs
    push 0x4000
    mov ax, word ptr [index]
    and ax, 1
    shl ax, 10
    or ax, 2
    push ax
    popf
    mov ax, 0x1234
    mov bx, 0x3456
    mov cx, 0x5678
    mov dx, 0x789a
    mov si, 0x9abc
    mov di, 0xbcde
    mov bp, 0x7777
    push cs
    call leaf
returned:
'''
    for i, name in enumerate(base.FIELDS[:-1]):
        value = "offset returned" if name == "ip" else name
        text += f"    mov word ptr [output+{2*i}], {value}\n"
    # Preserve the full twenty-byte caller/local frame before PUSHF can
    # overwrite the old source-segment argument. MOV leaves FLAGS unchanged.
    for i in range(10):
        text += (f"    mov ax, word ptr [{0xefec + 2*i}]\n"
                 f"    mov word ptr [output+{28 + 2*i}], ax\n")
    text += r'''
    pushf
    pop ax
    mov word ptr [output+26], ax
    cld
'''
    for source_offset, destination_offset, count in (
            (0x4000, 48, 55), (0x4200, 103, 55),
            (0x4400, 158, 32), (0x4600, 190, 32)):
        text += (f"    mov si, {source_offset}\n"
                 f"    mov di, offset output+{destination_offset}\n"
                 f"    mov cx, {count}\n    rep movsb\n")
    text += r'''
    mov al, byte ptr [0x4a1e]
    mov byte ptr [output+222], al
    mov ax, word ptr [0x5eb2]
    mov word ptr [output+223], ax
    mov ax, word ptr [0x5eb4]
    mov word ptr [output+225], ax
    mov ax, word ptr [0x5011]
    mov word ptr [output+227], ax
    mov ax, word ptr [index]
    mov word ptr [output+229], ax
    sti
    mov bx, word ptr [handle]
    mov dx, offset output
    mov cx, 231
    mov ah, 0x40
    int 0x21
    jc failure
    cmp ax, 231
    jne failure
    inc word ptr [index]
    cmp word ptr [index], 384
    jb again
    mov bx, word ptr [handle]
    mov ah, 0x3e
    int 0x21
    mov ax, 0x4c00
    int 0x21
failure:
    mov ax, 0x4c01
    int 0x21
filename: .asciz "RESULT.BIN"
header: .space 6
handle: .word 0
index: .word 0
output: .space 231
.org 0x1000
leaf:
'''
    return text + f'.incbin "{source.resolve()}"\n'


def main():
    parser = base.argparse.ArgumentParser(description=__doc__)
    base.add_media_arguments(parser)
    args = parser.parse_args()
    body, _ = base.source_audit(args.exe, args.ovr)
    directory = base.ROOT / "resources/oracle-runs/u6f-032f-synthetic-native"
    directory.mkdir(parents=True, exist_ok=True)
    source, asm, obj, com = (directory / n for n in
                             ("leaf.bin", "oracle.S", "oracle.o", "ORACLE.COM"))
    source.write_bytes(body)
    asm.write_text(assembly(source))
    subprocess.run(["as", "--32", str(asm), "-o", str(obj)], check=True)
    subprocess.run(["ld", "-m", "elf_i386", "-Ttext", "0x100", "--oformat",
                    "binary", str(obj), "-o", str(com)], check=True)
    base.check(com.read_bytes()[0x1000:] == body, "native copied body mismatch")
    config = directory / "dosbox.conf"
    config.write_text("[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n"
                      "[cpu]\ncore=normal\ncputype=386\ncycles=max\n"
                      "[mixer]\nnosound=true\n[midi]\nmididevice=none\n"
                      "[autoexec]\nmount c " + str(directory.resolve()) +
                      "\nc:\nORACLE.COM\nexit\n")
    result_path = directory / "RESULT.BIN"
    if result_path.exists():
        result_path.unlink()
    environment = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy")
    with (directory / "dosbox.log").open("w") as log:
        subprocess.run(["dosbox-x", "-conf", str(config), "-fastlaunch",
                        "-exit", "-noconsole"], env=environment, stdout=log,
                       stderr=subprocess.STDOUT, timeout=60, check=True)
    result = result_path.read_bytes()
    base.check(len(result) == 6 + CASES * RECORD, "incomplete native result")
    segment, return_ip, leaf_ip = struct.unpack_from("<HHH", result)
    base.check(leaf_ip == 0x1100, "native body position mismatch")
    with tempfile.TemporaryDirectory(prefix="u6f-native-032f-") as tmp:
        library = base.build_c(Path(tmp))
        for case in range(CASES):
            mode = case if case < 256 else (case - 256) % 3
            count, total = case % 21 + 1, (case % 15 + 1)*4 + case % 4
            memory = bytearray([0xa5]) * base.RAM_SIZE
            cpu = base.Cpu(0x1234, 0x3456, 0x5678, 0x789a, 0x9abc, 0xbcde,
                           0x7777, 0xeff4, segment, leaf_ip, segment, segment,
                           segment, 2 | ((case & 1) << 10))
            base.put_far(memory, segment, 0xeff4, (segment, return_ip))
            base.put_far(memory, segment, 0xeff8, (segment, 0x4000))
            base.put_far(memory, segment, 0xeffc, (segment, 0x4200))
            base.put_far(memory, segment, 0x4013, (segment, 0x4400))
            base.put_far(memory, segment, 0x4213, (segment, 0x4600))
            base.put_word(memory, segment, 0x4211, count)
            memory[base.physical(segment, 0x4a1e)] = mode
            base.put_far(memory, segment, 0x5eb2, (segment, 0x5000))
            base.put_word(memory, segment, 0x5011, total)
            for i in range(32):
                for offset, value in (
                        (0x4017, (case*3 + i*17) ^ 0x5a),
                        (0x4217, (case + i*13) ^ 0xa5),
                        (0x4400, (case + i*7) ^ 0xaa),
                        (0x4600, (case*5 + i*11) ^ 0xf0)):
                    memory[base.physical(segment, offset+i)] = value & 255
            ram = (ctypes.c_uint8 * base.RAM_SIZE).from_buffer_copy(memory)
            base.check(library.pr_u6f_vector_merge_032f(ram, len(ram), ctypes.byref(cpu)) == 0,
                       "C rejected native frame")
            record = result[6 + case*RECORD:6 + (case+1)*RECORD]
            base.check(bytes(cpu) == record[:28], f"native CPU mismatch case {case}")
            for offset, begin, size in ((0xefec, 28, 20), (0x4000, 48, 55),
                                        (0x4200, 103, 55), (0x4400, 158, 32),
                                        (0x4600, 190, 32), (0x4a1e, 222, 1),
                                        (0x5eb2, 223, 4), (0x5011, 227, 2)):
                at = base.physical(segment, offset)
                base.check(bytes(ram[at:at+size]) == record[begin:begin+size],
                           f"native region {offset:04x} mismatch case {case}")
            base.check(struct.unpack_from("<H", record, 229)[0] == case,
                       "native case order mismatch")
    print(f"{CASES} synthetic DOSBox/C far-call cases: all 14 CPU words, 20 frame bytes, "
          "records/vectors/globals equal; no flag mask")
    print("result SHA-256 " + sha256(result).hexdigest())
    if args.report:
        args.report.write_text(base.json.dumps(dict(cases=CASES, cpu_words=14,
            ram_bytes_per_case=201, flags_mask=0,
            result_sha256=sha256(result).hexdigest(),
            com_sha256=sha256(com.read_bytes()).hexdigest()), indent=2) + "\n")
    print("synthetic native frame evidence; natural game reach and ownership remain unmeasured")


if __name__ == "__main__":
    main()
