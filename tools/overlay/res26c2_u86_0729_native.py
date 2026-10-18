"""DOSBox-X synthetic oracle for raw-CPU RES_00BA_26C2 and u86 +0729 (imported by res26c2_u86_0729_audit.py).

The COM copies the original unit 0x86 code to paragraph 8800 and RES_00BA_26C2's 103
bytes to 9000:26C2, with +0729's fixup-backed far-call segment word relocated
loader-style to 9000. Each case writes the k-th 16-byte table entry at 1900:31FA+16k,
the DS:6DBF flag and the two record bytes at 6000:0196/019B, pushes the argument
word and far-calls either routine with IRQs masked. The records hold all 14 CPU
words, 64 stack bytes and every byte the case wrote.
"""
import ctypes
from hashlib import sha256
import os
import random
import struct
import subprocess

from u86_procvars_3413_audit import Cpu, ROOT, SIZE, physical, put16

CALLER_N, RES_N, DATA_DS = 0x8800, 0x9000, 0x1900
MAIN_AT, CASES_AT, CODE_AT, RES_AT_COM = 0x0300, 0x0C00, 0x2000, 0x1E00
OUTPUT, HEADER, STACK_TOP = 0x9000, 0x9800, 0xF000
FRAME_LOW, FRAME_SIZE = 0xEFC0, 0x40
CASE_SIZE = 24
RECORD_BYTES = 28 + FRAME_SIZE + 16 + 3
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)


def check(condition, message):
    if not condition:
        raise ValueError(message)


def native_cases():
    rng = random.Random(0x26C2DF)
    cases = []
    for function in (0, 1):
        for i in range(80):
            k = rng.randrange(256)
            entry = [rng.randrange(256) for _ in range(16)]
            entry[0] = (0, 1, 2, 3, 0xFF)[i % 5]
            flag = (0, 1)[(i // 5) % 2]
            if function == 1 and i % 4 == 3:                         # aim [BP-2] at 0 or FFh
                target = (0, 0xFF)[(i // 4) % 2]
                entry[0] = 3
                if flag:
                    entry[2] = (target - 6 * entry[3]) & 0xFF
                else:
                    entry[3] = 0; entry[2] = target
                entry[6] = (0, 9)[(i // 8) % 2]
            cases.append(dict(function=function, k=k, flag=flag, entry=entry, rec96=rng.randrange(256), rec9b=rng.randrange(256),
                              high=rng.randrange(256)))
    return cases


def assembly(code, res, cases):
    text = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main",
            f".org {MAIN_AT - 0x100:#x}", "main:",
            "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}", "    cld",
            f"    mov ax,{CALLER_N:#x}", "    mov es,ax", "    mov si,offset unit", "    xor di,di", f"    mov cx,{len(code):#x}", "    rep movsb",
            f"    mov ax,{RES_N:#x}", "    mov es,ax", "    mov si,offset resident", "    mov di,0x26c2", f"    mov cx,{len(res):#x}", "    rep movsb",
            f"    mov ax,{DATA_DS:#x}", "    mov es,ax", "    mov word ptr es:[0x5d92],0x0100", "    mov word ptr es:[0x5d94],0x6000",
            "    push cs", "    pop es",
            f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
            f"    mov word ptr [{HEADER + 4:#x}],{len(cases)}", f"    mov word ptr [{HEADER + 6:#x}],0",
            "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
            "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8",
            "    mov ah,0x40", "    int 0x21", "    jc failure",
            "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    push cs", "    pop ds", "    push cs", "    pop es",
            "    mov al,0xa5", f"    mov di,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE}", "    rep stosb",
            "    mov ax,word ptr [index]", f"    mov bx,{CASE_SIZE}", "    mul bx", "    add ax,offset cases", "    mov si,ax",
            "    mov word ptr [case_ptr],si",
            # entry address: 31FA + 16k
            "    mov al,byte ptr [si+1]", "    xor ah,ah", "    shl ax,4", "    add ax,0x31fa", "    mov word ptr [entry_off],ax",
            f"    mov ax,{DATA_DS:#x}", "    mov es,ax", "    mov di,word ptr [entry_off]", "    push si", "    add si,3", "    mov cx,16", "    rep movsb", "    pop si",
            "    mov al,byte ptr [si+2]", "    mov byte ptr es:[0x6dbf],al",
            "    mov ax,0x6000", "    mov es,ax", "    mov al,byte ptr [si+19]", "    mov byte ptr es:[0x0196],al",
            "    mov al,byte ptr [si+20]", "    mov byte ptr es:[0x019b],al",
            "    push cs", "    pop es",
            "    mov al,byte ptr [si+1]", "    mov ah,byte ptr [si+21]", "    push ax",
            "    cmp byte ptr [si],0", "    jne to_entry",
            "    mov word ptr [target_far],0x26c2", f"    mov word ptr [target_far+2],{RES_N:#x}", "    jmp target_set",
            "to_entry:", "    mov word ptr [target_far],0x0729", f"    mov word ptr [target_far+2],{CALLER_N:#x}",
            "target_set:", "    mov bl,byte ptr [si+22]", "    xor bh,bh", "    shl bx,1", "    push word ptr [flags_table+bx]",
            f"    mov ax,{DATA_DS:#x}", "    mov ds,ax", "    popf"]
    for name, value in ENTRY_REGS.items():
        text.append(f"    mov {name},{value:#x}")
    text += ["    call dword ptr cs:[target_far]", "returned:"]
    for i, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        text.append(f"    mov word ptr cs:[{OUTPUT + 2 * i:#x}],{reg}")
    text += [f"    mov word ptr cs:[{OUTPUT + 18:#x}],offset returned", f"    mov word ptr cs:[{OUTPUT + 20:#x}],ds",
             f"    mov word ptr cs:[{OUTPUT + 22:#x}],es", f"    mov word ptr cs:[{OUTPUT + 24:#x}],ss",
             f"    mov sp,{STACK_TOP + 0x800:#x}", "    pushf", "    pop ax", f"    mov word ptr cs:[{OUTPUT + 26:#x}],ax",
             "    cld", "    push cs", "    pop ds", "    push cs", "    pop es", f"    mov di,{OUTPUT + 28:#x}",
             f"    mov si,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE}", "    rep movsb",
             f"    mov ax,{DATA_DS:#x}", "    mov ds,ax", "    mov si,word ptr cs:[entry_off]", "    mov cx,16", "    rep movsb",
             "    mov si,0x6dbf", "    movsb",
             "    mov ax,0x6000", "    mov ds,ax", "    mov si,0x0196", "    movsb", "    mov si,0x019b", "    movsb",
             "    push cs", "    pop ds", "    sti",
             "    mov bx,word ptr [handle]", f"    mov dx,{OUTPUT:#x}", f"    mov cx,{RECORD_BYTES}",
             "    mov ah,0x40", "    int 0x21", "    jc failure", f"    cmp ax,{RECORD_BYTES}", "    jne failure",
             "    inc word ptr [index]", "    mov ax,word ptr [index]", f"    cmp ax,{len(cases)}", "    jb again",
             "    mov bx,word ptr [handle]", "    mov ah,0x3e", "    int 0x21", "    mov ax,0x4c00", "    int 0x21",
             "failure:", "    mov ax,0x4c01", "    int 0x21",
             'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", "case_ptr: .word 0", "entry_off: .word 0",
             "target_far: .word 0, 0", "flags_table: .word " + ",".join(f"{f:#x}" for f in FLAGS),
             f".org {CASES_AT - 0x100:#x}", "cases:"]
    for n, case in enumerate(cases):
        row = [case["function"], case["k"], case["flag"]] + case["entry"] + [case["rec96"], case["rec9b"], case["high"], n % len(FLAGS), 0]
        text.append("    .byte " + ",".join(f"{b:#x}" for b in row))
    text += [f".org {RES_AT_COM - 0x100:#x}", "resident:"]
    for i in range(0, len(res), 16):
        text.append("    .byte " + ",".join(f"{b:#x}" for b in res[i:i + 16]))
    text += [f".org {CODE_AT - 0x100:#x}", "unit:"]
    for i in range(0, len(code), 16):
        text.append("    .byte " + ",".join(f"{b:#x}" for b in code[i:i + 16]))
    return "\n".join(text) + "\n"


def native_audit(targets, code, image, functions):
    directory = ROOT / "resources/oracle-runs/r26c2-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    res = targets["26C2"]["body"]
    unit = bytearray(code)
    struct.pack_into("<H", unit, 0x073A + 3, RES_N)
    check(len(unit) <= 16 * (RES_N - CALLER_N) and CASES_AT + CASE_SIZE * 160 <= RES_AT_COM and RES_AT_COM + len(res) <= CODE_AT
          and CODE_AT + len(unit) <= OUTPUT, "layout overlaps")
    cases = native_cases()
    asm, obj = directory / "oracle.S", directory / "oracle.o"
    asm.write_text(assembly(bytes(unit), res, cases))
    subprocess.run(["as", "--32", str(asm), "-o", str(obj)], check=True)
    subprocess.run(["ld", "-m", "elf_i386", "-Ttext", "0x100", "--oformat", "binary", str(obj), "-o", str(directory / "ORACLE.COM")], check=True)
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
    check(len(result) == 8 + len(cases) * RECORD_BYTES, f"native output incomplete: {len(result)} bytes")
    segment, returned, count, _ = struct.unpack_from("<4H", result)
    check(count == len(cases) and 0x10000 + 16 * segment <= 16 * DATA_DS, "native header or COM placement differs")
    per = [0, 0]
    for n, case in enumerate(cases):
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        memory = bytearray(SIZE)
        at = physical(CALLER_N, 0); memory[at:at + len(unit)] = unit
        at = physical(RES_N, 0x26C2); memory[at:at + len(res)] = res
        put16(memory, DATA_DS, 0x5D92, 0x0100); put16(memory, DATA_DS, 0x5D94, 0x6000)
        entry_off = (0x31FA + 16 * case["k"]) & 0xFFFF
        for i, b in enumerate(case["entry"]):
            memory[physical(DATA_DS, entry_off + i)] = b
        memory[physical(DATA_DS, 0x6DBF)] = case["flag"]
        memory[physical(0x6000, 0x0196)] = case["rec96"]; memory[physical(0x6000, 0x019B)] = case["rec9b"]
        for i in range(FRAME_SIZE):
            memory[physical(segment, FRAME_LOW + i)] = 0xA5
        sp = STACK_TOP
        for value in ((case["high"] << 8) | case["k"], segment, returned):
            sp -= 2; put16(memory, segment, sp, value)
        key = "0729" if case["function"] else "26C2"
        entry_cs, entry_ip = (CALLER_N, 0x0729) if case["function"] else (RES_N, 0x26C2)
        cpu = Cpu(*ENTRY_REGS.values(), sp, entry_cs, entry_ip, DATA_DS, segment, segment, FLAGS[n % len(FLAGS)])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        check(functions[key](ram, SIZE, ctypes.byref(cpu)) == 0, f"C status on native case {n}")
        for (name, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, name) == want, f"native case {n} ({key}) {name}: C={getattr(cpu, name):04x}, DOS={want:04x}")
        got = bytes(ram[physical(segment, FRAME_LOW):physical(segment, FRAME_LOW) + FRAME_SIZE])
        got += bytes(ram[physical(DATA_DS, entry_off):physical(DATA_DS, entry_off) + 16])
        got += bytes([ram[physical(DATA_DS, 0x6DBF)], ram[physical(0x6000, 0x0196)], ram[physical(0x6000, 0x019B)]])
        check(got == record[28:], f"native case {n} ({key}) bytes differ")
        per[case["function"]] += 1
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {len(cases)} (RES_00BA_26C2: {per[0]}, +0729: {per[1]}); all 14 CPU words, "
          f"{FRAME_SIZE} stack bytes and the 19 bytes each case wrote; no mask")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}")
    return dict(cases=len(cases), result_sha256=digest)
