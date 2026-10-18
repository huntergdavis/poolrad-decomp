"""DOSBox-X synthetic oracle for the raw-CPU RES_00BA_2199 model (imported by resident_2199_cpu_audit.py).

The COM holds the routine's original 111 bytes at offset 2199h of its own segment.
Each case restores three baked worlds at absolute segments, pushes the Pascal
arguments, and far-calls the routine with IRQs masked. The records hold all 14 CPU
words, the 32 stack bytes around the frame and every world byte. Node B2 sits at
6000:FFFA, so its +5 link word straddles physical 6FFFF/70000; a CPU that wrapped
that word inside the segment would take the high byte 5Ah from 6000:0000 instead.
"""
import ctypes
from hashlib import sha256
import os
import struct
import subprocess

from u86_procvars_3413_audit import Cpu, ROOT, SIZE, physical, put16

MAIN_AT, CASES_AT, WORLD_AT, BODY_AT = 0x0300, 0x0C00, 0x1000, 0x2199
OUTPUT, HEADER, STACK_TOP = 0x9000, 0x9800, 0xF000
FRAME_LOW, FRAME_SIZE = 0xEFE0, 0x20
REGIONS = ((0x5000, 0x0000, 0x400), (0x6FFF, 0x0000, 0x20), (0x6000, 0x0000, 0x10))
RECORD_BYTES = 28 + FRAME_SIZE + sum(r[2] for r in REGIONS)
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)
CASE_SIZE = 12

X1, X2, X3, X4 = (0x0000, 0x5000), (0x0010, 0x5000), (0x0020, 0x5000), (0x0030, 0x5000)
CUR1, CUR3, FRAME = (0x03F0, 0x5000), (0x007F, 0x5000), (0xEFEE, 0)      # segment 0: the runtime SS
BASE_CASES = [
    (X2, 0x0011, CUR1), (X1, 0x0011, CUR1), (X1, 0xA522, CUR1), (X1, 0xFF33, CUR1), (X1, 0x0144, CUR1),
    (X1, 0x5555, CUR1), (X1, 0x0066, CUR1), (X3, 0x0066, CUR1), (X3, 0x0077, CUR1), (X3, 0x00AB, CUR1),
    (X4, 0x0099, CUR1), (X4, 0x0088, CUR1), (X1, 0x0044, CUR3), (X1, 0x00EE, CUR3), (X1, 0x0033, FRAME),
    (X3, 0x0077, FRAME), (X2, 0x0000, FRAME), (X1, 0x7711, FRAME),
]
CASES = BASE_CASES + BASE_CASES


def check(condition, message):
    if not condition:
        raise ValueError(message)


def worlds():
    a = bytearray([0x5C]) * 0x400

    def word(buffer, offset, value):
        buffer[offset] = value & 0xFF; buffer[offset + 1] = value >> 8
    word(a, 0x007F, 0x0200); word(a, 0x0081, 0x5000)          # X1 -> list A
    word(a, 0x008F, 0x0000); word(a, 0x0091, 0x0000)          # X2 -> empty
    word(a, 0x009F, 0x0300); word(a, 0x00A1, 0x5000)          # X3 -> list B
    word(a, 0x00AF, 0x0380); word(a, 0x00B1, 0x5000)          # X4 -> list C, a cycle
    for i, key in enumerate((0x11, 0x22, 0x33, 0x44, 0x55)):
        off = 0x0200 + 0x10 * i
        a[off] = key; a[off + 1:off + 5] = bytes((0xC0 + i,)) * 4
        nxt = (0x0210 + 0x10 * i, 0x5000) if i < 4 else (0, 0)
        word(a, off + 5, nxt[0]); word(a, off + 7, nxt[1])
    a[0x0300] = 0x66; word(a, 0x0305, 0xFFFA); word(a, 0x0307, 0x6000)      # B1 -> B2 at 6000:FFFA
    a[0x0380] = 0x88; word(a, 0x0385, 0x0390); word(a, 0x0387, 0x5000)      # C1 -> C2
    a[0x0390] = 0x99; word(a, 0x0395, 0x0380); word(a, 0x0397, 0x5000)      # C2 -> C1
    word(a, 0x03F0, 0xBEEF); word(a, 0x03F2, 0xDEAD)                         # cursor cell CUR1
    p = bytearray([0x6C]) * 0x20                                              # physical 6FFF0..7000F
    p[0x0A] = 0x77; p[0x0B:0x0F] = b"\x01\x02\x03\x04"                       # B2 key and bytes 1..4
    p[0x0F] = 0x00; p[0x10] = 0x00                                            # +5 word at EA FFFF, read contiguous
    q = bytearray([0x6D]) * 0x10                                              # physical 60000..6000F
    q[0x00] = 0x5A                                                            # a wrapping CPU's +5 high byte
    q[0x01] = 0x00; q[0x02] = 0x00                                            # +7 word at EA 0001: segment 0
    return a, p, q


def assembly(a, p, q):
    text = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main",
            f".org {MAIN_AT - 0x100:#x}", "main:",
            "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}",
            "    mov word ptr [target_far+2],cs",
            f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
            f"    mov word ptr [{HEADER + 4:#x}],{len(CASES)}", f"    mov word ptr [{HEADER + 6:#x}],0",
            "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
            "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8",
            "    mov ah,0x40", "    int 0x21", "    jc failure",
            "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    push cs", "    pop ds"]
    for (segment, offset, length), label in zip(REGIONS, ("world_a", "world_p", "world_q")):
        text += [f"    mov ax,{segment:#x}", "    mov es,ax", f"    mov si,offset {label}", f"    mov di,{offset:#x}",
                 f"    mov cx,{length:#x}", "    rep movsb"]
    text += ["    push cs", "    pop es", "    mov al,0xa5", f"    mov di,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE}", "    rep stosb",
             "    mov ax,word ptr [index]", f"    mov bx,{CASE_SIZE}", "    mul bx", "    add ax,offset cases", "    mov si,ax",
             "    push word ptr [si+2]", "    push word ptr [si]", "    push word ptr [si+4]",
             "    mov ax,word ptr [si+8]", "    or ax,ax", "    jnz have_segment", "    mov ax,cs", "have_segment:",
             "    push ax", "    push word ptr [si+6]",
             "    mov bx,word ptr [si+10]", "    shl bx,1", "    push word ptr [flags_table+bx]", "    popf"]
    for name, value in ENTRY_REGS.items():
        text.append(f"    mov {name},{value:#x}")
    text += ["    call dword ptr [target_far]", "returned:"]
    for i, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        text.append(f"    mov word ptr [{OUTPUT + 2 * i:#x}],{reg}")
    text += [f"    mov word ptr [{OUTPUT + 18:#x}],offset returned", f"    mov word ptr [{OUTPUT + 20:#x}],ds",
             f"    mov word ptr [{OUTPUT + 22:#x}],es", f"    mov word ptr [{OUTPUT + 24:#x}],ss",
             f"    mov sp,{STACK_TOP + 0x800:#x}", "    pushf", "    pop ax", f"    mov word ptr [{OUTPUT + 26:#x}],ax",
             "    cld", "    push cs", "    pop es", f"    mov di,{OUTPUT + 28:#x}",
             "    push cs", "    pop ds", f"    mov si,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE}", "    rep movsb"]
    for segment, offset, length in REGIONS:
        text += [f"    mov ax,{segment:#x}", "    mov ds,ax", f"    mov si,{offset:#x}", f"    mov cx,{length:#x}", "    rep movsb"]
    text += ["    push cs", "    pop ds",
             "    sti", "    mov bx,word ptr [handle]", f"    mov dx,{OUTPUT:#x}", f"    mov cx,{RECORD_BYTES}",
             "    mov ah,0x40", "    int 0x21", "    jc failure", f"    cmp ax,{RECORD_BYTES}", "    jne failure",
             "    inc word ptr [index]", "    mov ax,word ptr [index]", f"    cmp ax,{len(CASES)}", "    jb again",
             "    mov bx,word ptr [handle]", "    mov ah,0x3e", "    int 0x21", "    mov ax,0x4c00", "    int 0x21",
             "failure:", "    mov ax,0x4c01", "    int 0x21",
             'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0",
             f"target_far: .word {BODY_AT:#x}, 0",
             "flags_table: .word " + ",".join(f"{f:#x}" for f in FLAGS),
             f".org {CASES_AT - 0x100:#x}", "cases:"]
    for n, ((x_off, x_seg), key, (c_off, c_seg)) in enumerate(CASES):
        text.append(f"    .word {x_off:#x},{x_seg:#x},{key:#x},{c_off:#x},{c_seg:#x},{n % len(FLAGS)}")
    for label, blob, at in (("world_a", a, WORLD_AT), ("world_p", p, WORLD_AT + 0x400), ("world_q", q, WORLD_AT + 0x420)):
        text += [f".org {at - 0x100:#x}", f"{label}:"]
        for i in range(0, len(blob), 16):
            text.append("    .byte " + ",".join(f"{b:#x}" for b in blob[i:i + 16]))
    return "\n".join(text) + "\n"


def native_audit(target, function):
    directory = ROOT / "resources/oracle-runs/res-2199-cpu-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    a, p, q = worlds()
    asm, obj = directory / "oracle.S", directory / "oracle.o"
    asm.write_text(assembly(a, p, q))
    subprocess.run(["as", "--32", str(asm), "-o", str(obj)], check=True)
    subprocess.run(["ld", "-m", "elf_i386", "-Ttext", "0x100", "--oformat", "binary", str(obj), "-o", str(directory / "main.bin")], check=True)
    assembled = (directory / "main.bin").read_bytes()
    check(0x100 + len(assembled) <= BODY_AT, "harness reaches the routine copy")
    image = bytearray(BODY_AT + len(target["body"]) - 0x100)
    image[:len(assembled)] = assembled
    image[BODY_AT - 0x100:] = target["body"]
    (directory / "ORACLE.COM").write_bytes(bytes(image))
    config = directory / "dosbox.conf"
    config.write_text("[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n[cpu]\ncore=normal\ncputype=386\ncycles=max\n"
                      "[mixer]\nnosound=true\n[midi]\nmididevice=none\n"
                      "[autoexec]\nmount c " + str(directory.resolve()) + "\nc:\nORACLE.COM\nexit\n")
    result_path = directory / "RESULT.BIN"
    if result_path.exists():
        result_path.unlink()
    environment = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy")
    with (directory / "dosbox.log").open("w") as log:
        subprocess.run(["dosbox-x", "-conf", str(config), "-fastlaunch", "-exit", "-noconsole"],
                       env=environment, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=300)
    result = result_path.read_bytes()
    check(len(result) == 8 + len(CASES) * RECORD_BYTES, f"native output incomplete: {len(result)} bytes")
    segment, returned, count, _ = struct.unpack_from("<4H", result)
    check(count == len(CASES), "native header differs")
    found = 0
    for n, ((x_off, x_seg), key, (c_off, c_seg)) in enumerate(CASES):
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        memory = bytearray(SIZE)
        for (seg, off, length), blob in zip(REGIONS, (a, p, q)):
            at = physical(seg, off); memory[at:at + length] = blob
        for i in range(FRAME_SIZE):
            memory[physical(segment, FRAME_LOW + i)] = 0xA5
        sp = STACK_TOP
        for value in (x_seg, x_off, key, c_seg or segment, c_off, segment, returned):
            sp -= 2; put16(memory, segment, sp, value)
        cpu = Cpu(*ENTRY_REGS.values(), sp, segment, BODY_AT, segment, segment, segment, FLAGS[n % len(FLAGS)])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        check(function(ram, SIZE, ctypes.byref(cpu), 1000) == 0, f"C status on native case {n}")
        for (name, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, name) == want, f"native case {n} {name}: C={getattr(cpu, name):04x}, DOS={want:04x}")
        offset = 28
        for seg, off, length in ((segment, FRAME_LOW, FRAME_SIZE),) + REGIONS:
            at = physical(seg, off)
            if bytes(ram[at:at + length]) != record[offset:offset + length]:
                diff = [i for i in range(length) if ram[at + i] != record[offset + i]]
                raise ValueError(f"native case {n} bytes at {seg:04x}:{off + diff[0]:04x} differ ({len(diff)} bytes)")
            offset += length
        found += cpu.ax & 0xFF == 1
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {len(CASES)} ({found} with AL = 1); all 14 CPU words, "
          f"{FRAME_SIZE} frame bytes and {sum(r[2] for r in REGIONS)} world bytes; no mask")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}")
    return dict(cases=len(CASES), result_sha256=digest)
