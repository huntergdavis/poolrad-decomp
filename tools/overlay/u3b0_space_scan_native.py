"""DOSBox-X synthetic oracle for U3B0 +08D7 and +0923 (imported by u3b0_space_scan_audit.py).

At start-up the COM copies the unit 0x3B0 code to paragraph 8800, with both routines'
fixup-backed far-call segment words relocated loader-style to 9000, and the 36 original
SYSTEM string-store bytes to 9000:0B25. Each case fills a 128-byte stack window and a
256-byte source window, writes a length byte and 40 characters either in the source
window or inside the stack frame itself, pushes S and a static-link word and far-calls
the routine with IRQs masked. The C replay rebuilds the same memory and compares all 14
CPU words and both windows, with no mask.
"""
import ctypes
from hashlib import sha256
import os
import random
import struct
import subprocess

from u3b0_space_scan_audit import ROUTINES, STORE_CS, STORE_IP, STORE_END, SPACE, CAP, NONSPACE, check
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, physical, put16

CALLER_N, STORE_N, SRC_SEG, DATA_DS = 0x8800, 0x9000, 0x2000, 0x1900
MAIN_AT, STORE_BLOB, UNIT_BLOB, CASES_AT = 0x0300, 0x0F00, 0x1000, 0x2400
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xC800, 0xF000
FRAME_LOW, FRAME_SIZE, SRC_REGION = 0xEFB0, 0x80, 0x100
ENTRY_SP = STACK_TOP - 10
CASE_SIZE = 50
RECORD_BYTES = 28 + FRAME_SIZE + SRC_REGION
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)
ALIAS = (-0x3C, -0x3A, -0x2D, -0x2C, -0x2B, -0x20, -0x10, 0, 6)


def native_cases():
    rng = random.Random(0x3B0D05)
    cases = []
    for entry, spec in ROUTINES.items():
        key = spec["key"]
        rows = []
        for n in (0, 1, 2, CAP):                                    # all spaces, then no spaces
            rows += [(n, [SPACE] * CAP), (n, [rng.choice(NONSPACE) for _ in range(CAP)])]
        for _ in range(32):
            n = rng.randrange(CAP + 1)
            count = rng.choice((0, 1, n // 2, max(n - 1, 0), n))
            spare = rng.randrange(n - count + 1)
            head = {"lead": count, "trail": spare} if key == "lead" else {"lead": spare, "trail": count}
            chars = [SPACE] * head["lead"] + [rng.choice(NONSPACE) for _ in range(n - count - spare)] + [SPACE] * head["trail"]
            rows.append((n, chars + [rng.choice((SPACE, 0x41)) for _ in range(CAP - n)]))
        for n in (0x29, 0x30, 0x80, 0xC0, 0xFF, 0x29, 0x40, 0xFE, 0x2A, 0x50, 0xE0, 0x35):
            count = rng.choice((0, 1, CAP - 1, CAP, rng.randrange(CAP + 1)))
            rows.append((n, ([SPACE] * count + [rng.choice(NONSPACE) for _ in range(CAP - count)]) if key == "lead"
                         else ([rng.choice(NONSPACE) for _ in range(CAP - count)] + [SPACE] * count)))
        for i, (n, chars) in enumerate(rows):
            cases.append(dict(entry=entry, n=n, chars=chars, stack=False, offset=rng.randrange(0x00, SRC_REGION - 0x29),
                              link=rng.randrange(0x10000), flags=len(cases) % len(FLAGS)))
        for delta in ALIAS:
            n = rng.randrange(256)
            cases.append(dict(entry=entry, n=n, chars=[rng.choice((SPACE, SPACE, rng.randrange(256))) for _ in range(CAP)], stack=True,
                              offset=(ENTRY_SP + delta) & 0xFFFF, link=rng.randrange(0x10000), flags=len(cases) % len(FLAGS)))
    return cases


def assembly(unit, store, cases):
    text = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main",
            f".org {MAIN_AT - 0x100:#x}", "main:",
            "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}", "    cld"]
    for segment, offset, label, length in ((CALLER_N, 0, "unit", len(unit)), (STORE_N, STORE_IP, "store", len(store))):
        text += [f"    mov ax,{segment:#x}", "    mov es,ax", f"    mov si,offset {label}", f"    mov di,{offset:#x}",
                 f"    mov cx,{length:#x}", "    rep movsb"]
    text += ["    push cs", "    pop es",
             f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
             f"    mov word ptr [{HEADER + 4:#x}],{len(cases)}", f"    mov word ptr [{HEADER + 6:#x}],0",
             "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
             "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8",
             "    mov ah,0x40", "    int 0x21", "    jc failure",
             "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    push cs", "    pop ds", "    push cs", "    pop es",
             "    mov al,0xa5", f"    mov di,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE}", "    rep stosb",
             f"    mov ax,{SRC_SEG:#x}", "    mov es,ax", "    xor di,di", "    mov al,0x5a", f"    mov cx,{SRC_REGION}", "    rep stosb",
             "    mov ax,word ptr [index]", f"    mov bx,{CASE_SIZE}", "    mul bx", "    add ax,offset cases", "    mov si,ax",
             "    cmp byte ptr [si+3],0", "    je string_in_source", "    push cs", "    pop es",
             "string_in_source:", "    mov di,word ptr [si+4]", "    add si,8", "    mov cx,41", "    rep movsb", "    sub si,49",
             "    push cs", "    pop es",
             "    cmp byte ptr [si+3],0", "    je push_source", "    push cs", "    jmp pushed_segment",
             "push_source:", f"    mov ax,{SRC_SEG:#x}", "    push ax", "pushed_segment:",
             "    push word ptr [si+4]", "    push word ptr [si+6]",
             "    mov ax,word ptr [si]", "    mov word ptr [target_far],ax",
             "    mov bl,byte ptr [si+2]", "    xor bh,bh", "    shl bx,1", "    push word ptr [flags_table+bx]", "    popf",
             f"    mov ax,{DATA_DS:#x}", "    mov ds,ax"]
    for name, value in ENTRY_REGS.items():
        text.append(f"    mov {name},{value:#x}")
    text += ["    call dword ptr cs:[target_far]", "returned:"]
    for i, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        text.append(f"    mov word ptr cs:[{OUTPUT + 2 * i:#x}],{reg}")
    text += [f"    mov word ptr cs:[{OUTPUT + 18:#x}],offset returned", f"    mov word ptr cs:[{OUTPUT + 20:#x}],ds",
             f"    mov word ptr cs:[{OUTPUT + 22:#x}],es", f"    mov word ptr cs:[{OUTPUT + 24:#x}],ss",
             f"    mov sp,{STACK_TOP + 0x800:#x}", "    pushf", "    pop ax", f"    mov word ptr cs:[{OUTPUT + 26:#x}],ax",
             "    cld", "    push cs", "    pop es", "    push cs", "    pop ds", f"    mov di,{OUTPUT + 28:#x}",
             f"    mov si,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE}", "    rep movsb",
             f"    mov ax,{SRC_SEG:#x}", "    mov ds,ax", "    xor si,si", f"    mov cx,{SRC_REGION}", "    rep movsb",
             "    push cs", "    pop ds", "    sti",
             "    mov bx,word ptr [handle]", f"    mov dx,{OUTPUT:#x}", f"    mov cx,{RECORD_BYTES}",
             "    mov ah,0x40", "    int 0x21", "    jc failure", f"    cmp ax,{RECORD_BYTES}", "    jne failure",
             "    inc word ptr [index]", "    mov ax,word ptr [index]", f"    cmp ax,{len(cases)}", "    jb again",
             "    mov bx,word ptr [handle]", "    mov ah,0x3e", "    int 0x21", "    mov ax,0x4c00", "    int 0x21",
             "failure:", "    mov ax,0x4c01", "    int 0x21",
             'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0",
             f"target_far: .word 0, {CALLER_N:#x}", "flags_table: .word " + ",".join(f"{f:#x}" for f in FLAGS)]
    for label, blob, at in (("store", store, STORE_BLOB), ("unit", unit, UNIT_BLOB)):
        text += [f".org {at - 0x100:#x}", f"{label}:"]
        for i in range(0, len(blob), 16):
            text.append("    .byte " + ",".join(f"{b:#x}" for b in blob[i:i + 16]))
    text += [f".org {CASES_AT - 0x100:#x}", "cases:"]
    for case in cases:
        row = list(struct.pack("<HBBHH", case["entry"], case["flags"], int(case["stack"]), case["offset"], case["link"]))
        row += [case["n"]] + case["chars"] + [0]
        check(len(row) == CASE_SIZE, "case row size differs")
        text.append("    .byte " + ",".join(f"{b:#x}" for b in row))
    return "\n".join(text) + "\n"


def native_audit(targets, code, image, functions):
    directory = ROOT / "resources/oracle-runs/u3b0-space-scan-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    store = image[STORE_CS * 16 + STORE_IP:STORE_CS * 16 + STORE_END]
    unit = bytearray(code)
    for target in targets.values():
        struct.pack_into("<H", unit, target["site"] + 3, STORE_N)
    cases = native_cases()
    check(MAIN_AT < STORE_BLOB and STORE_BLOB + len(store) <= UNIT_BLOB and UNIT_BLOB + len(unit) <= CASES_AT
          and CASES_AT + CASE_SIZE * len(cases) <= OUTPUT and len(unit) <= 16 * (STORE_N - CALLER_N), "COM layout overlaps")
    asm, obj = directory / "oracle.S", directory / "oracle.o"
    asm.write_text(assembly(bytes(unit), store, cases))
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
    check(count == len(cases), "native header differs")
    per_routine = {}
    for n, case in enumerate(cases):
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        memory = bytearray(SIZE)
        for seg, off, blob in ((CALLER_N, 0, bytes(unit)), (STORE_N, STORE_IP, store)):
            at = physical(seg, off)
            memory[at:at + len(blob)] = blob
        for i in range(FRAME_SIZE):
            memory[physical(segment, FRAME_LOW + i)] = 0xA5
        for i in range(SRC_REGION):
            memory[physical(SRC_SEG, i)] = 0x5A
        string_segment = segment if case["stack"] else SRC_SEG
        for i, value in enumerate([case["n"]] + case["chars"]):
            memory[physical(string_segment, case["offset"] + i)] = value
        sp = STACK_TOP
        for value in (string_segment, case["offset"], case["link"], segment, returned):
            sp -= 2
            put16(memory, segment, sp, value)
        cpu = Cpu(*ENTRY_REGS.values(), sp, CALLER_N, case["entry"], DATA_DS, segment, segment, FLAGS[case["flags"]])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        callee = ctypes.c_int(99)
        label = f"native case {n} ({targets[case['entry']]['label']}, n={case['n']:02x}, {'stack' if case['stack'] else 'source'})"
        check(functions[case["entry"]](ram, SIZE, ctypes.byref(cpu), ctypes.byref(callee)) == 0 and callee.value == 0, f"C status on {label}")
        for (name, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, name) == want, f"{label} {name}: C={getattr(cpu, name):04x}, DOS={want:04x}")
        got = bytes(ram[physical(segment, FRAME_LOW):physical(segment, FRAME_LOW) + FRAME_SIZE])
        got += bytes(ram[physical(SRC_SEG, 0):physical(SRC_SEG, 0) + SRC_REGION])
        check(got == record[28:], f"{label} bytes differ")
        per_routine[targets[case["entry"]]["label"]] = per_routine.get(targets[case["entry"]]["label"], 0) + 1
    digest = sha256(result).hexdigest()
    aliased = sum(case["stack"] for case in cases)
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {len(cases)} ({per_routine}; {aliased} with S inside the stack frame); "
          f"all 14 CPU words, the {FRAME_SIZE}-byte stack window and the {SRC_REGION}-byte source window; no mask")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}; string store copied to {STORE_N:04X}:0B25")
    return dict(cases=len(cases), per_routine=per_routine, stack_aliased=aliased, result_sha256=digest)
