"""DOSBox-X synthetic oracle for U1C +266A (imported by u1c_party_item_scan_266a_audit.py).

At start-up the COM copies unit 0x1C to paragraph 8800 and unit 0x25 to 7800, writes the loaded overlay stub
EA jumps at 7F00:0025 (U25 +0136) and 7F00:002A (U25 +0000), copies SYSTEM FillChar to 9000:16A8, and
relocates +266A's three fixup-backed segment words loader-style (both stub calls to 7F00, FillChar to 9000).
Each case writes one ECL operand (tags without SYSTEM string calls), resets the operand cells, the compare
flags and the script PC in data segment 1900, builds a party list at 2000 with item lists at 2800, and far-calls
+266A with IRQs masked. The C replay rebuilds the same memory and compares all 14 CPU words, a 64-byte stack
window and the data bytes the routine and its callees write, with no mask.
"""
import ctypes
from hashlib import sha256
import os
import random
import struct
import subprocess

from u1c_party_item_scan_266a_audit import ENTRY, SITES, check, world_spec
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, physical, put16

CALLER_N, U25_N, STUB_N, SYS_N, DATA_DS, SCRIPT_SEG, PARTY_SEG, ITEM_SEG = 0x8800, 0x7800, 0x7F00, 0x9000, 0x1900, 0x2C00, 0x2000, 0x2800
MAIN_AT, TABLES_AT = 0x0300, 0x0F00
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xC800, 0xF000
FRAME_LOW, FRAME_SIZE = 0xEFC0, 0x40
DS_BYTES = [0x6F78 + i for i in range(6)] + [0x49ED, 0x49EE, 0x7006, 0x7046, 0x7086]
CASE_SIZE = 4 + 4 + 16
RECORD_BYTES = 28 + FRAME_SIZE + len(DS_BYTES)
NATIVE_FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)


def native_cases():
    rng = random.Random(0x1C266AD)
    cases = []
    for i in range(48):
        spec = world_spec(rng, tag=rng.choice((0, 0, 2, 0x10, 0x55)), members=rng.randrange(0, 4), match=i % 2 == 0)
        lists = [l[:3] for l in spec["lists"][:3]]
        cases.append(dict(spec, lists=lists, flags=i % len(NATIVE_FLAGS)))
    return cases


def layout(memory, case):
    """Write the party list and item lists for a case into `memory` (also what the COM harness writes)."""
    members = []
    for k, items in enumerate(case["lists"]):
        member = 0x0100 + 0x200 * k
        members.append(member)
        offsets = [0x0100 + 0x100 * k + 0x40 * j for j in range(len(items))]
        for j, (off, kind) in enumerate(zip(offsets, items)):
            memory[physical(ITEM_SEG, off + 0x2E)] = kind
            nxt = offsets[j + 1] if j + 1 < len(offsets) else 0
            put16(memory, ITEM_SEG, off + 0x2A, nxt)
            put16(memory, ITEM_SEG, off + 0x2C, ITEM_SEG if nxt else 0)
        put16(memory, PARTY_SEG, member + 0xC8, offsets[0] if offsets else 0)
        put16(memory, PARTY_SEG, member + 0xCA, ITEM_SEG if offsets else 0)
    for k, member in enumerate(members):
        nxt = members[k + 1] if k + 1 < len(members) else 0
        put16(memory, PARTY_SEG, member + 0x104, nxt)
        put16(memory, PARTY_SEG, member + 0x106, PARTY_SEG if nxt else 0)
    put16(memory, DATA_DS, 0x5D96, members[0] if members else 0)
    put16(memory, DATA_DS, 0x5D98, PARTY_SEG if members else 0)


def harness_writes(case):
    """(segment, offset, byte) writes the COM makes per case, from a zeroed scratch image."""
    scratch = bytearray(SIZE)
    layout(scratch, case)
    writes = []
    for seg in (PARTY_SEG, ITEM_SEG):
        base = physical(seg, 0)
        for off in range(0x1000):
            if scratch[base + off]:
                writes.append((seg, off, scratch[base + off]))
    for off in (0x5D96, 0x5D97, 0x5D98, 0x5D99):
        writes.append((DATA_DS, off, scratch[physical(DATA_DS, off)]))
    return writes


def native_audit(target, code1c, code25, image, function):
    del target
    directory = ROOT / "resources/oracle-runs/u1c-266a-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    unit = bytearray(code1c)
    for site, (offset, ip) in SITES.items():
        struct.pack_into("<H", unit, site + 3, STUB_N if ip is not None else SYS_N)
    fill = image[0x0AF8 * 16 + 0x16A8:0x0AF8 * 16 + 0x16BC]
    cases = native_cases()
    text = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main", f".org {MAIN_AT - 0x100:#x}", "main:",
            "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}", "    cld"]
    for segment, offset, label, length in ((CALLER_N, 0, "unit1c", len(unit)), (U25_N, 0, "unit25", len(code25)), (SYS_N, 0x16A8, "fill", len(fill))):
        text += [f"    mov ax,{segment:#x}", "    mov es,ax", f"    mov si,offset {label}", f"    mov di,{offset:#x}", f"    mov cx,{length:#x}", "    rep movsb"]
    text += [f"    mov ax,{STUB_N:#x}", "    mov es,ax",
             "    mov byte ptr es:[0x25],0xea", "    mov word ptr es:[0x26],0x0136", f"    mov word ptr es:[0x28],{U25_N:#x}",
             "    mov byte ptr es:[0x2a],0xea", "    mov word ptr es:[0x2b],0x0000", f"    mov word ptr es:[0x2d],{U25_N:#x}",
             f"    mov ax,{DATA_DS:#x}", "    mov es,ax", "    mov word ptr es:[0x49de],0x1000", f"    mov word ptr es:[0x49e0],{SCRIPT_SEG:#x}",
             "    mov word ptr es:[0x49d2],0x1000", "    mov word ptr es:[0x49d4],0x6000",
             "    mov ax,cs", "    mov es,ax",
             f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
             f"    mov word ptr [{HEADER + 4:#x}],{len(cases)}", f"    mov word ptr [{HEADER + 6:#x}],0",
             "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
             "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8", "    mov ah,0x40", "    int 0x21", "    jc failure",
             "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    mov ax,cs", "    mov ds,ax", "    mov es,ax",
             "    mov al,0xa5", f"    mov di,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE}", "    rep stosb",
             # clear the party and item segments' first 4 KiB, then replay this case's list writes
             f"    mov ax,{PARTY_SEG:#x}", "    mov es,ax", "    xor di,di", "    xor ax,ax", "    mov cx,0x800", "    rep stosw",
             f"    mov ax,{ITEM_SEG:#x}", "    mov es,ax", "    xor di,di", "    xor ax,ax", "    mov cx,0x800", "    rep stosw",
             "    mov ax,word ptr [index]", "    shl ax,1", "    mov bx,ax", "    mov si,word ptr [write_table+bx]",
             "write_loop:", "    lodsw", "    or ax,ax", "    jz writes_done", "    mov es,ax", "    lodsw", "    mov di,ax", "    lodsb", "    stosb",
             "    jmp write_loop", "writes_done:",
             "    mov ax,word ptr [index]", f"    mov bx,{CASE_SIZE}", "    mul bx", "    add ax,offset cases", "    mov si,ax",
             f"    mov ax,{DATA_DS:#x}", "    mov es,ax", "    mov word ptr es:[0x49ed],0x9900",
             "    mov al,0x5a", "    mov di,0x6f78", "    mov cx,6", "    rep stosb",
             "    mov byte ptr es:[0x7006],0x5a", "    mov byte ptr es:[0x7046],0x5a", "    mov byte ptr es:[0x7086],0x5a",
             f"    mov ax,{SCRIPT_SEG:#x}", "    mov es,ax", "    mov di,0x1001", "    mov al,byte ptr [si]", "    stosb", "    mov al,byte ptr [si+1]", "    stosb",
             "    mov al,byte ptr [si+2]", "    stosb",
             "    mov ax,cs", "    mov es,ax",
             "    mov bl,byte ptr [si+3]", "    xor bh,bh", "    shl bx,1", "    push word ptr [flags_table+bx]", "    popf",
             f"    mov ax,{DATA_DS:#x}", "    mov ds,ax"]
    for name, value in ENTRY_REGS.items():
        text.append(f"    mov {name},{value:#x}")
    text += ["    call dword ptr cs:[target_far]", "returned:"]
    for i, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        text.append(f"    mov word ptr cs:[{OUTPUT + 2 * i:#x}],{reg}")
    text += [f"    mov word ptr cs:[{OUTPUT + 18:#x}],offset returned", f"    mov word ptr cs:[{OUTPUT + 20:#x}],ds",
             f"    mov word ptr cs:[{OUTPUT + 22:#x}],es", f"    mov word ptr cs:[{OUTPUT + 24:#x}],ss",
             f"    mov sp,{STACK_TOP + 0x800:#x}", "    pushf", "    pop ax", f"    mov word ptr cs:[{OUTPUT + 26:#x}],ax",
             "    cld", "    mov ax,cs", "    mov es,ax", "    mov ds,ax", f"    mov di,{OUTPUT + 28:#x}",
             f"    mov si,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE}", "    rep movsb",
             f"    mov ax,{DATA_DS:#x}", "    mov ds,ax"]
    for off in DS_BYTES:
        text += [f"    mov al,byte ptr [{off:#x}]", "    stosb"]
    text += ["    mov ax,cs", "    mov ds,ax", "    sti",
             "    mov bx,word ptr [handle]", f"    mov dx,{OUTPUT:#x}", f"    mov cx,{RECORD_BYTES}", "    mov ah,0x40", "    int 0x21", "    jc failure",
             f"    cmp ax,{RECORD_BYTES}", "    jne failure",
             "    inc word ptr [index]", "    mov ax,word ptr [index]", f"    cmp ax,{len(cases)}", "    jb again",
             "    mov bx,word ptr [handle]", "    mov ah,0x3e", "    int 0x21", "    mov ax,0x4c00", "    int 0x21",
             "failure:", "    mov ax,0x4c01", "    int 0x21",
             'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", f"target_far: .word {ENTRY:#x}, {CALLER_N:#x}",
             "flags_table: .word " + ",".join(f"{f:#x}" for f in NATIVE_FLAGS),
             "write_table: .word " + ",".join(f"writes_{n}" for n in range(len(cases)))]
    text += [f".org {TABLES_AT - 0x100:#x}"]
    table_bytes = 0
    for n, case in enumerate(cases):
        text.append(f"writes_{n}:")
        for seg, off, value in harness_writes(case):
            text.append(f"    .word {seg:#x}, {off:#x}")
            text.append(f"    .byte {value:#x}")
            table_bytes += 5
        text.append("    .word 0")
        table_bytes += 2
    fill_blob = (TABLES_AT + table_bytes + 0x10F) & ~0xF
    u25_blob = (fill_blob + len(fill) + 0x1F) & ~0xF
    u1c_blob = (u25_blob + len(code25) + 0x1F) & ~0xF
    for label, blob, at in (("fill", fill, fill_blob), ("unit25", code25, u25_blob), ("unit1c", bytes(unit), u1c_blob)):
        text += [f".org {at - 0x100:#x}", f"{label}:"]
        for i in range(0, len(blob), 16):
            text.append("    .byte " + ",".join(f"{b:#x}" for b in blob[i:i + 16]))
    cases_at = u1c_blob + len(unit) + 0x10
    text += [f".org {cases_at - 0x100:#x}", "cases:"]
    for case in cases:
        row = bytes([case["tag"], case["value"], case["third"], case["flags"]]) + bytes(4) + bytes(16)
        check(len(row) == CASE_SIZE, "case row size differs")
        text.append("    .byte " + ",".join(f"{b:#x}" for b in row))
    check(cases_at + CASE_SIZE * len(cases) <= OUTPUT, f"COM layout overlaps: cases end {cases_at + CASE_SIZE * len(cases):#x}")
    asm, obj = directory / "oracle.S", directory / "oracle.o"
    asm.write_text("\n".join(text) + "\n")
    subprocess.run(["as", "--32", str(asm), "-o", str(obj)], check=True)
    subprocess.run(["ld", "-m", "elf_i386", "-Ttext", "0x100", "--oformat", "binary", str(obj), "-o", str(directory / "ORACLE.COM")], check=True)
    check((directory / "ORACLE.COM").stat().st_size <= 0xBF00, "COM image too large")
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
    found = 0
    for n, case in enumerate(cases):
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        memory = bytearray(SIZE)
        for seg, off, blob in ((CALLER_N, 0, bytes(unit)), (U25_N, 0, code25), (SYS_N, 0x16A8, fill)):
            at = physical(seg, off)
            memory[at:at + len(blob)] = blob
        memory[physical(STUB_N, 0x25):physical(STUB_N, 0x25) + 5] = struct.pack("<BHH", 0xEA, 0x0136, U25_N)
        memory[physical(STUB_N, 0x2A):physical(STUB_N, 0x2A) + 5] = struct.pack("<BHH", 0xEA, 0x0000, U25_N)
        put16(memory, DATA_DS, 0x49DE, 0x1000); put16(memory, DATA_DS, 0x49E0, SCRIPT_SEG)
        put16(memory, DATA_DS, 0x49D2, 0x1000); put16(memory, DATA_DS, 0x49D4, 0x6000)
        for i in range(FRAME_SIZE):
            memory[physical(segment, FRAME_LOW + i)] = 0xA5
        layout(memory, case)
        put16(memory, DATA_DS, 0x49ED, 0x9900)
        for off in (0x6F78, 0x6F79, 0x6F7A, 0x6F7B, 0x6F7C, 0x6F7D, 0x7006, 0x7046, 0x7086):
            memory[physical(DATA_DS, off)] = 0x5A
        for i, b in enumerate((case["tag"], case["value"], case["third"])):
            memory[physical(SCRIPT_SEG, 0x1001 + i)] = b
        sp = STACK_TOP
        for value in (segment, returned):
            sp -= 2
            put16(memory, segment, sp, value)
        cpu = Cpu(*ENTRY_REGS.values(), sp, CALLER_N, ENTRY, DATA_DS, segment, segment, NATIVE_FLAGS[case["flags"]])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        callee, callee_status = ctypes.c_int(99), ctypes.c_int(99)
        limits = function.argtypes[3]._type_(SYS_N, 4, 400, 1000)
        label = f"native case {n} (tag {case['tag']:02x}, value {case['value']:02x})"
        status = function(ram, SIZE, ctypes.byref(cpu), ctypes.byref(limits), ctypes.byref(callee), ctypes.byref(callee_status))
        check(status == 0, f"C status {status} ({callee.value}/{callee_status.value}) on {label}")
        for (name, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, name) == want, f"{label} {name}: C={getattr(cpu, name):04x}, DOS={want:04x}")
        got = bytes(ram[physical(segment, FRAME_LOW):physical(segment, FRAME_LOW) + FRAME_SIZE])
        got += bytes(ram[physical(DATA_DS, off)] for off in DS_BYTES)
        check(got == record[28:], f"{label} bytes differ")
        found += ram[physical(DATA_DS, 0x6F78)] == 1
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {len(cases)} ({found} found the operand among the items); all 14 CPU words, "
          f"the {FRAME_SIZE}-byte stack window and {len(DS_BYTES)} data bytes; no mask")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}; U25 at {U25_N:04X} behind stub {STUB_N:04X}, FillChar at {SYS_N:04X}:16A8")
    return dict(cases=len(cases), found=found, result_sha256=digest)
