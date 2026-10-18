"""DOSBox-X synthetic full-chain oracle for U6A +0E37.
Adapted from the merged GetMem native harness; private original bytes only.
"""
import ctypes
from hashlib import sha256
import os
from pathlib import Path
import random
import struct
import subprocess
from u6a_string_node_0e37_audit import Cpu, ROOT, SIZE, physical, put16, check
SYSTEM_CS=0x0AF8
ENTRY=0x0E37
OK, RUNERROR=0,2
NAMES={0:"return",2:"RunError203"}
CALLER_N=0x8000
SYS_N, DATA_DS, ARENA_N = 0x7000, 0x5000, 0x6000
MAIN_AT, SPAN_AT, CASES_AT = 0x0300, 0x0C00, 0x2000
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xD800, 0xF000
WINDOW_LOW, WINDOW_SIZE, ARENA_BYTES, SPAN_BYTES = STACK_TOP - 0x100, 0x100, 0x4000, 0x0C00
CASE_SIZE, NATIVE_CASES = 32, 64
RECORD_BYTES = 30 + WINDOW_SIZE + 0x10000 + ARENA_BYTES
NATIVE_FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)


def native_cases(rng):
    cases = []
    for n in range(NATIVE_CASES):
        blocks, segment = [], ARENA_N
        for _ in range(rng.choice((0, 1, 2, 3, 4))):
            segment += rng.randrange(1, 30)
            paragraphs, granule = rng.randrange(0, 20), rng.choice((0, 8))
            if paragraphs == 0 and granule == 0:
                granule = 8
            blocks.append((segment, 0, paragraphs, granule))
            segment += paragraphs + 1
        top = (segment + rng.randrange(1, 30), rng.choice((0, 8)))
        end = (min(ARENA_N + 0x3FF, top[0] + rng.choice((0, 2, 30, 300))), rng.choice((0, 8)))
        sizes = [0, 1, 8, 9, 16, 17, 100, rng.randrange(1, 600), rng.randrange(1, 0x3000)] + [16 * b[2] + b[3] for b in blocks]
        cases.append(dict(flags=n % len(NATIVE_FLAGS), size=(0,1,39,40,41,100,255,rng.randrange(256))[n%8], blocks=blocks, top=top, end=end))
    return cases


def case_bytes(case):
    data = struct.pack("<HBB4H", case["size"], case["flags"], len(case["blocks"]), case["top"][1], case["top"][0], case["end"][1], case["end"][0])
    for seg, off, paragraphs, granule in case["blocks"]:
        data += struct.pack("<HBB", seg, paragraphs, granule)
    return data + bytes(CASE_SIZE - len(data))


def native_audit(image, code, function):
    directory = ROOT / "resources/u6a-0e37/native"
    directory.mkdir(parents=True, exist_ok=True)
    span = image[physical(SYSTEM_CS, 0):physical(SYSTEM_CS, SPAN_BYTES)]
    caller=bytearray(code[0x0E37:0x0EBE])
    for site in (0x0E4B,0x0E54,0x0EA5):
        struct.pack_into("<H",caller,site+3-0x0E37,SYS_N)
    rng = random.Random(0x6A0E37)
    cases = native_cases(rng)
    check(SPAN_AT + len(span) <= CASES_AT and CASES_AT + CASE_SIZE * len(cases) <= OUTPUT, "COM layout overlaps")
    t = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main", f".org {MAIN_AT - 0x100:#x}", "main:",
         "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}", "    cld",
         f"    mov ax,{SYS_N:#x}", "    mov es,ax", "    mov si,offset span", "    xor di,di", f"    mov cx,{len(span):#x}", "    rep movsb",
         f"    mov ax,{CALLER_N:#x}", "    mov es,ax", "    mov si,offset caller", "    mov di,0x0e37",
         f"    mov cx,{len(caller)}", "    rep movsb", f"    mov ax,{SYS_N:#x}", "    mov es,ax",
         "    mov byte ptr es:[0xe2],0xea", "    mov word ptr es:[0xe3],offset trap", "    mov word ptr es:[0xe5],cs",
         "    push cs", "    pop es", f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
         f"    mov word ptr [{HEADER + 4:#x}],{len(cases)}", f"    mov word ptr [{HEADER + 6:#x}],0",
         "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
         "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8", "    mov ah,0x40", "    int 0x21", "    jc failure",
         "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    push cs", "    pop ds", "    push cs", "    pop es",
         "    mov al,0xa5", f"    mov di,{WINDOW_LOW:#x}", f"    mov cx,{WINDOW_SIZE:#x}", "    rep stosb",
         "    mov ax,word ptr [index]", f"    mov bx,{CASE_SIZE}", "    mul bx", "    add ax,offset cases", "    mov si,ax", "    mov word ptr [case_ptr],ax",
         f"    mov ax,{DATA_DS:#x}", "    mov es,ax", "    xor di,di", "    mov cx,0x8000", "    mov ax,0xa5a5", "    rep stosw",
         f"    mov ax,{ARENA_N:#x}", "    mov es,ax", "    xor di,di", f"    mov cx,{ARENA_BYTES // 2:#x}", "    mov ax,0xa5a5", "    rep stosw",
         f"    mov ax,{DATA_DS:#x}", "    mov es,ax",
         "    mov word ptr es:[0x4400],0", f"    mov word ptr es:[0x4402],{ARENA_N:#x}",
         "    mov ax,word ptr [si+4]", "    mov word ptr es:[0x4404],ax", "    mov ax,word ptr [si+6]", "    mov word ptr es:[0x4406],ax",
         "    mov ax,word ptr [si+8]", "    mov word ptr es:[0x4408],ax", "    mov ax,word ptr [si+10]", "    mov word ptr es:[0x440a],ax",
         "    mov word ptr es:[0x4414],0xa9", f"    mov word ptr es:[0x4416],{SYS_N:#x}",
         # FreeList: the first block, or HeapPtr when there is none
         "    mov ax,word ptr [si+4]", "    mov dx,word ptr [si+6]", "    cmp byte ptr [si+3],0", "    je list_head",
         "    xor ax,ax", "    mov dx,word ptr [si+12]", "list_head:", "    mov word ptr es:[0x440c],ax", "    mov word ptr es:[0x440e],dx",
         # each block (segment, paragraphs, granule) at offset 0, linked to the next block or to HeapPtr
         "    xor ch,ch", "    mov cl,byte ptr [si+3]", "    jcxz blocks_done", "    lea bx,[si+12]", "block_loop:",
         "    mov es,word ptr [bx]", "    xor ax,ax", "    mov dx,word ptr [bx+4]", "    cmp cx,1", "    jne block_link",
         "    mov ax,word ptr [si+4]", "    mov dx,word ptr [si+6]", "block_link:",
         "    mov word ptr es:[0],ax", "    mov word ptr es:[2],dx", "    mov al,byte ptr [bx+3]", "    xor ah,ah", "    mov word ptr es:[4],ax",
         "    mov al,byte ptr [bx+2]", "    xor ah,ah", "    mov word ptr es:[6],ax", "    add bx,4", "    loop block_loop", "blocks_done:",
         f"    mov ax,{DATA_DS:#x}", "    mov es,ax", "    mov al,byte ptr [si]", "    mov byte ptr es:[0x1000],al",
         "    mov word ptr cs:[0xef74],0x0100", f"    mov word ptr cs:[0xef76],{ARENA_N+0x380:#x}",
         "    push cs", "    pop es", "    push 0xa55a", f"    push {DATA_DS:#x}", "    push 0x1000", "    push 0xef80",
         "    mov bl,byte ptr [si+2]", "    xor bh,bh", "    shl bx,1",
         "    push word ptr [flags_table+bx]", f"    mov ax,{DATA_DS:#x}", "    mov ds,ax"]
    for name, value in ENTRY_REGS.items():
        t.append(f"    mov {name},{value:#x}")
    t += ["    popf", "    call dword ptr cs:[target_far]", "returned:", "    mov word ptr cs:[status],0", "    jmp record",
          "trap:", "    mov word ptr cs:[status],1", "record:"]
    for i, reg in enumerate("ax bx cx dx si di bp sp".split()):
        t.append(f"    mov word ptr cs:[{OUTPUT + 2 * i:#x}],{reg}")
    t += [f"    mov word ptr cs:[{OUTPUT + 20:#x}],ds", f"    mov word ptr cs:[{OUTPUT + 22:#x}],es", f"    mov word ptr cs:[{OUTPUT + 24:#x}],ss",
          f"    mov sp,{STACK_TOP + 0x800:#x}", "    pushf", "    pop ax", f"    mov word ptr cs:[{OUTPUT + 26:#x}],ax",
          "    cmp word ptr cs:[status],0", "    jne trapped",
          f"    mov word ptr cs:[{OUTPUT + 16:#x}],cs", f"    mov word ptr cs:[{OUTPUT + 18:#x}],offset returned", "    jmp segments",
          "trapped:", f"    mov word ptr cs:[{OUTPUT + 16:#x}],{SYS_N:#x}", f"    mov word ptr cs:[{OUTPUT + 18:#x}],0xe2", "segments:",
          "    mov ax,word ptr cs:[status]", f"    mov word ptr cs:[{OUTPUT + 28:#x}],ax",
          "    cld", "    push cs", "    pop ds", "    push cs", "    pop es", f"    mov di,{OUTPUT + 30:#x}",
          f"    mov si,{WINDOW_LOW:#x}", f"    mov cx,{WINDOW_SIZE:#x}", "    rep movsb", "    sti",
          "    mov bx,word ptr [handle]", f"    mov dx,{OUTPUT:#x}", f"    mov cx,{30 + WINDOW_SIZE}", "    mov ah,0x40", "    int 0x21", "    jc failure",
          f"    mov ax,{DATA_DS:#x}", "    mov ds,ax", "    xor dx,dx", "    mov cx,0x8000", "    mov ah,0x40", "    int 0x21", "    jc failure",
          "    mov dx,0x8000", "    mov cx,0x8000", "    mov ah,0x40", "    int 0x21", "    jc failure",
          f"    mov ax,{ARENA_N:#x}", "    mov ds,ax", "    xor dx,dx", f"    mov cx,{ARENA_BYTES:#x}", "    mov ah,0x40", "    int 0x21", "    jc failure",
          "    push cs", "    pop ds", "    inc word ptr [index]", "    mov ax,word ptr [index]", f"    cmp ax,{len(cases)}", "    jb again",
          "    mov bx,word ptr [handle]", "    mov ah,0x3e", "    int 0x21", "    mov ax,0x4c00", "    int 0x21",
          "failure:", "    mov ax,0x4c01", "    int 0x21",
          'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", "case_ptr: .word 0", "status: .word 0",
          f"target_far: .word {ENTRY:#x}, {CALLER_N:#x}", "flags_table: .word " + ",".join(f"{f:#x}" for f in NATIVE_FLAGS)]
    for label, blob, where in (("span", span, SPAN_AT), ("cases", b"".join(case_bytes(c) for c in cases), CASES_AT), ("caller", bytes(caller), 0x3000)):
        t += [f".org {where - 0x100:#x}", f"{label}:"]
        for i in range(0, len(blob), 16):
            t.append("    .byte " + ",".join(f"{b:#x}" for b in blob[i:i + 16]))
    asm, obj = directory / "oracle.S", directory / "oracle.o"
    asm.write_text("\n".join(t) + "\n")
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
                       env=environment, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=600)
    result = result_path.read_bytes()
    check(len(result) == 8 + len(cases) * RECORD_BYTES, f"native output incomplete: {len(result)} bytes")
    segment, returned, count, _ = struct.unpack_from("<4H", result)
    check(count == len(cases) and segment == 0x0814, f"native header differs or COM segment {segment:04x} is not 0814")
    statuses = {}
    for n, case in enumerate(cases):
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        memory = bytearray(SIZE)
        at=physical(CALLER_N,ENTRY);memory[at:at+len(caller)]=caller
        at = physical(SYS_N, 0); memory[at:at + len(span)] = span
        memory[physical(SYS_N, 0xE2):physical(SYS_N, 0xE2) + 5] = struct.pack("<BHH", 0xEA, 0, 0)
        at = physical(DATA_DS, 0); memory[at:at + 0x10000] = bytes([0xA5]) * 0x10000
        at = physical(ARENA_N, 0); memory[at:at + ARENA_BYTES] = bytes([0xA5]) * ARENA_BYTES
        put16(memory, DATA_DS, 0x4400, 0); put16(memory, DATA_DS, 0x4402, ARENA_N)
        put16(memory, DATA_DS, 0x4404, case["top"][1]); put16(memory, DATA_DS, 0x4406, case["top"][0])
        put16(memory, DATA_DS, 0x4408, case["end"][1]); put16(memory, DATA_DS, 0x440A, case["end"][0])
        put16(memory, DATA_DS, 0x4414, 0x00A9); put16(memory, DATA_DS, 0x4416, SYS_N)
        head = (case["blocks"][0][1], case["blocks"][0][0]) if case["blocks"] else (case["top"][1], case["top"][0])
        put16(memory, DATA_DS, 0x440C, head[0]); put16(memory, DATA_DS, 0x440E, head[1])
        for i, (seg, off, paragraphs, granule) in enumerate(case["blocks"]):
            nxt = (0, case["blocks"][i + 1][0]) if i + 1 < len(case["blocks"]) else (case["top"][1], case["top"][0])
            put16(memory, seg, 0, nxt[0]); put16(memory, seg, 2, nxt[1]); put16(memory, seg, 4, granule); put16(memory, seg, 6, paragraphs)
        for i in range(WINDOW_SIZE):
            memory[physical(segment, WINDOW_LOW + i)] = 0xA5
        memory[physical(DATA_DS,0x1000)]=case["size"]
        put16(memory,segment,0xEF74,0x0100);put16(memory,segment,0xEF76,ARENA_N+0x380)
        sp = STACK_TOP
        for value in (0xA55A, DATA_DS, 0x1000, 0xEF80, segment, returned):
            sp -= 2; put16(memory, segment, sp, value)
        cpu = Cpu(*ENTRY_REGS.values(), sp, CALLER_N, ENTRY, DATA_DS, segment, segment, NATIVE_FLAGS[case["flags"]])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        callee=ctypes.c_int(-1)
        status = function(ram, SIZE, ctypes.byref(cpu), 64, ctypes.byref(callee))
        check((status,callee.value) in ((0,0),(2,2)),f"unexpected native C stop {status}/{callee.value}")
        check(status in (OK, RUNERROR) and struct.unpack_from("<H", record, 28)[0] == (1 if status == RUNERROR else 0),
              f"native case {n}: C status {status}, DOS status {struct.unpack_from('<H', record, 28)[0]}")
        for (name, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, name) == want, f"native case {n} {name}: C={getattr(cpu, name):04x}, DOS={want:04x}")
        got = bytes(ram[physical(segment, WINDOW_LOW):physical(segment, WINDOW_LOW) + WINDOW_SIZE])
        got += bytes(ram[physical(DATA_DS, 0):physical(DATA_DS, 0) + 0x10000]) + bytes(ram[physical(ARENA_N, 0):physical(ARENA_N, 0) + ARENA_BYTES])
        if got != record[30:]:
            differences = [i for i, (a, b) in enumerate(zip(got, record[30:])) if a != b]
            raise ValueError(f"native case {n} bytes differ at {[hex(d) for d in differences[:8]]} ({len(differences)} bytes)")
        statuses[NAMES[status]] = statuses.get(NAMES[status], 0) + 1
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {len(cases)} ({statuses}); all 14 CPU words, the {WINDOW_SIZE}-byte stack window, "
          f"the whole 64 KiB DS segment and the {ARENA_BYTES:#x}-byte heap arena; no mask; RunError captured at {SYS_N:04X}:00E2")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}")
    return dict(cases=len(cases), statuses=statuses, result_sha256=digest)
