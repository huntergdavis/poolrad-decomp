#!/usr/bin/env python3
"""Original-byte full-state parity for the raw-CPU SYSTEM string leaves 0AF8:0B0B (load) and 0AF8:0B98 (concatenation),
plus a DOSBox-X oracle.

The differential places the hash-checked expanded START.EXE image at load segment 0, so each leaf runs from its original
bytes, and compares the entire 1 MiB RAM and all 14 CPU words between the original under Unicorn and the C model, with no
mask, after the leaf's RETF 4. Strings of every length are placed apart, overlapping in both directions, in the same
place, and across segment ends, so REP MOVSB rereads bytes it has written and SI/DI wrap. Media stays private.
"""
import argparse
import ctypes
from hashlib import sha256
import json
import os
from pathlib import Path
import random
import struct
import subprocess
import tempfile

from capstone import CS_ARCH_X86, CS_MODE_16, Cs
from resident_loader_map import unpack
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, REGISTERS, physical, put16

SYSTEM_CS = 0x0AF8
LEAVES = {"load": (0x0B0B, 0x0B25, 12, 0x0B22), "concat": (0x0B98, 0x0BC4, 19, 0x0BC1)}
SS = 0x2800
RET_CS, RET_IP = 0x9001, 0x7000
FLAGS = (0x0602, 0x0202, 0x0ED7, 0x0647, 0x72D7, 0x0403, 0x0213)


def check(condition, message):
    if not condition:
        raise ValueError(message)


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, "START.EXE provenance differs")
    image = bytes(unpack(exe)[0])
    md = Cs(CS_ARCH_X86, CS_MODE_16)
    spans = {}
    for name, (start, end, count, retf) in LEAVES.items():
        span = image[physical(SYSTEM_CS, start):physical(SYSTEM_CS, end)]
        insns = list(md.disasm(span, start))
        check(len(insns) == count and sum(i.size for i in insns) == len(span) and insns[-1].address == retf and insns[-1].op_str == "4"
              and sum("movsb" in i.mnemonic for i in insns) == 1, f"SYSTEM {name} differs")
        spans[name] = dict(bytes=len(span), instructions=count, sha256=sha256(span).hexdigest(), decoded={i.address: i for i in insns})
        print(f"SYSTEM {name} {start:04X}..{end - 1:04X}: {len(span)} B, {count} insns, sha {spans[name]['sha256']}")
    return image, spans


def library(directory):
    path = directory / "strings.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                    str(ROOT / "src/recovered/system_string_leaves_cpu.c"), "-o", str(path)], check=True, capture_output=True, text=True)
    lib = ctypes.CDLL(str(path))
    functions = {}
    for name, symbol in (("load", "pr_system_strload_0b0b"), ("concat", "pr_system_strcat_0b98")):
        f = getattr(lib, symbol)
        f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu)]
        f.restype = ctypes.c_int
        functions[name] = f
    return functions


def place(memory, segment, offset, data):
    for i, value in enumerate(data):
        memory[physical(segment, (offset + i) & 0xFFFF)] = value


def build(image, rng, *, leaf, src, dst, src_bytes, dst_bytes, ds=None, sp=0x8000, ss=SS, flags=0x0602, background=None):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    memory[0:len(image)] = image
    place(memory, dst[0], dst[1], dst_bytes)
    place(memory, src[0], src[1], src_bytes)
    frame = physical(ss, sp)
    for i, value in enumerate((RET_IP, RET_CS, src[1], src[0], dst[1], dst[0])):
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
        put16(memory, 0, (frame + 2 * i) & (SIZE - 1), value)
    reg = tuple(rng.randrange(0x10000) for _ in range(7))
    entry = LEAVES[leaf][0]
    return memory, Cpu(*reg, sp, SYSTEM_CS, entry, rng.randrange(0x10000) if ds is None else ds, rng.randrange(0x10000), ss, flags), ss >= 0xF000


def compare(label, leaf, memory, cpu, high, spans, functions):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    from unicorn.x86_const import UC_X86_REG_CS, UC_X86_REG_IP
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory)); original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    decoded, retf = spans[leaf]["decoded"], LEAVES[leaf][3]
    state = dict(done=False, returned=False, unexpected=None, visited=set())

    def hook(emulator, address, size, user):
        del address, size, user
        cs, ip = emulator.reg_read(UC_X86_REG_CS), emulator.reg_read(UC_X86_REG_IP)
        if state["returned"]:
            state["done"] = True
            return emulator.emu_stop()
        if cs != SYSTEM_CS or ip not in decoded:
            state["unexpected"] = (cs, ip)
            return emulator.emu_stop()
        state["visited"].add(ip)
        state["returned"] = ip == retf
        return None

    def mirror(emulator, access, address, size, value, user):
        del access, user
        for byte in range(size):
            raw = address + byte; at = raw & (SIZE - 1)
            emulator.mem_write(at + SIZE if raw < SIZE else at, bytes([(value >> (8 * byte)) & 0xFF]))

    original.hook_add(UC_HOOK_CODE, hook)
    if high:
        original.hook_add(UC_HOOK_MEM_WRITE, mirror)
    original.emu_start(physical(SYSTEM_CS, LEAVES[leaf][0]), 2 * SIZE, count=100000)
    check(state["unexpected"] is None and state["done"], f"original did not return cleanly ({state['unexpected']}): {label}")
    expected = bytes(original.mem_read(0, SIZE))
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    check(functions[leaf](ram, SIZE, ctypes.byref(cpu)) == 0, f"C status: {label}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f"RAM mismatch {label}: {[(hex(d), actual[d], expected[d]) for d in differences[:6]]} ({len(differences)} bytes)")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        mine, want = getattr(cpu, name), original.reg_read(register) & 0xFFFF
        check(mine == want, f"{name} mismatch {label}: C={mine:04x}, original={want:04x}")
    return state["visited"]


def string(rng, length):
    return bytes([length]) + bytes(rng.randrange(256) for _ in range(length))


def cases(rng):
    spec = []
    for i in range(400):
        leaf = ("load", "concat")[i % 2]
        mode = rng.choice(("apart", "apart", "dst-after-src", "src-after-dst", "same", "wrap"))
        slen, dlen = rng.choice((0, 1, 2, 7, 128, 254, 255, rng.randrange(256))), rng.choice((0, 1, 100, 200, 254, 255, rng.randrange(256)))
        seg = rng.randrange(0x3000, 0x8000)
        if mode == "apart":
            src, dst = (seg, rng.randrange(0x0000, 0xF000)), (rng.randrange(0x3000, 0x8000), rng.randrange(0x0000, 0xF000))
        elif mode == "dst-after-src":
            off = rng.randrange(0x0000, 0xE000); src, dst = (seg, off), (seg, off + rng.randrange(1, 40))
        elif mode == "src-after-dst":
            off = rng.randrange(0x0000, 0xE000); dst, src = (seg, off), (seg, off + rng.randrange(1, 40))
        elif mode == "same":
            off = rng.randrange(0x0000, 0xE000); src = dst = (seg, off)
        else:
            src, dst = (seg, rng.randrange(0xFF00, 0x10000)), (rng.randrange(0x3000, 0x8000), rng.randrange(0xFF00, 0x10000))
        spec.append((f"random-{i}-{leaf}-{mode}", leaf, dict(src=src, dst=dst, src_bytes=string(rng, slen), dst_bytes=string(rng, dlen),
                                                            flags=FLAGS[i % len(FLAGS)], sp=rng.randrange(0x0400, 0xF000) & ~1)))
    for leaf in ("load", "concat"):
        spec.append((f"{leaf}-empty", leaf, dict(src=(0x4000, 0x100), dst=(0x5000, 0x200), src_bytes=b"\x00", dst_bytes=b"\x00")))
        spec.append((f"{leaf}-full", leaf, dict(src=(0x4000, 0x100), dst=(0x5000, 0x200), src_bytes=string(rng, 255), dst_bytes=string(rng, 255))))
        spec.append((f"{leaf}-ss-a20", leaf, dict(src=(0x4000, 0x100), dst=(0x5000, 0x200), src_bytes=string(rng, 9), dst_bytes=string(rng, 9),
                                                  sp=0x0400, ss=0xFFFF)))
        spec.append((f"{leaf}-df-set", leaf, dict(src=(0x4000, 0x100), dst=(0x5000, 0x200), src_bytes=string(rng, 9), dst_bytes=string(rng, 9),
                                                  flags=0x0603)))
        spec.append((f"{leaf}-random-background", leaf, dict(src=(0x4000, 0x100), dst=(0x4000, 0x105), src_bytes=string(rng, 50),
                                                             dst_bytes=string(rng, 60), background=bytes(random.Random(0x0B98).randbytes(SIZE)))))
    spec.append(("concat-exact-255", "concat", dict(src=(0x4000, 0x100), dst=(0x5000, 0x200), src_bytes=string(rng, 55), dst_bytes=string(rng, 200))))
    spec.append(("concat-clamp", "concat", dict(src=(0x4000, 0x100), dst=(0x5000, 0x200), src_bytes=string(rng, 56), dst_bytes=string(rng, 200))))
    spec.append(("concat-di-carry", "concat", dict(src=(0x4000, 0x100), dst=(0x5000, 0xFFF0), src_bytes=string(rng, 5), dst_bytes=string(rng, 40))))
    spec.append(("concat-di-8000", "concat", dict(src=(0x4000, 0x100), dst=(0x5000, 0x7FF0), src_bytes=string(rng, 5), dst_bytes=string(rng, 15))))
    return spec


def differential(spans, functions):
    rng = random.Random(0x0B0B0B98)
    visited, counts = {k: set() for k in LEAVES}, {k: 0 for k in LEAVES}
    for label, leaf, options in cases(rng):
        memory, cpu, high = build(spans["image"], rng, leaf=leaf, **options)
        visited[leaf] |= compare(label, leaf, memory, cpu, high, spans, functions)
        counts[leaf] += 1
    for leaf in LEAVES:
        check(visited[leaf] == set(spans[leaf]["decoded"]), f"{leaf} coverage incomplete")
    print(f"SYSTEM string leaves: {sum(counts.values())} original-byte cases ({counts}); full 1 MiB RAM and all 14 CPU words equal after RETF 4, "
          f"no mask; every instruction of both leaves executed, both outcomes of the concatenation's JAE")
    return dict(cases=counts, starts={k: len(v) for k, v in visited.items()})


# ---- DOSBox-X synthetic oracle -------------------------------------------------------------------------
SYS_N, DATA_N = 0x7000, 0x5000
MAIN_AT, SPAN_AT, BLOB_AT, CASES_AT = 0x0300, 0x0C00, 0x0E00, 0x2000
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xD800, 0xF000
WINDOW_LOW, WINDOW_SIZE, BLOB_SIZE = STACK_TOP - 0x40, 0x40, 0x1000
CASE_SIZE, NATIVE_CASES = 16, 128
RECORD_BYTES = 28 + WINDOW_SIZE + 0x10000
NATIVE_FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)


def native_cases(rng):
    cases = []
    for n in range(NATIVE_CASES):
        leaf = n % 2
        mode = rng.choice(("apart", "after", "before", "same", "wrap"))
        slen, dlen = rng.choice((0, 1, 9, 128, 255, rng.randrange(256))), rng.choice((0, 1, 100, 200, 255, rng.randrange(256)))
        if mode == "apart":
            s, d = rng.randrange(0x0000, 0x7000), rng.randrange(0x8000, 0xF000)
        elif mode == "after":
            s = rng.randrange(0x0000, 0xE000); d = s + rng.randrange(1, 40)
        elif mode == "before":
            d = rng.randrange(0x0000, 0xE000); s = d + rng.randrange(1, 40)
        elif mode == "same":
            s = d = rng.randrange(0x0000, 0xE000)
        else:
            s, d = rng.randrange(0xFF00, 0x10000), rng.randrange(0xFE00, 0x10000)
        cases.append(dict(leaf=leaf, flags=n % len(NATIVE_FLAGS), s=s, d=d, slen=slen, dlen=dlen,
                          sblob=rng.randrange(0, BLOB_SIZE - 256), dblob=rng.randrange(0, BLOB_SIZE - 256)))
    return cases


def native_memory(case, blob):
    data = bytearray([0xA5]) * 0x10000
    for offset, length, start in ((case["d"], case["dlen"], case["dblob"]), (case["s"], case["slen"], case["sblob"])):
        data[offset & 0xFFFF] = length
        for i in range(length):
            data[(offset + 1 + i) & 0xFFFF] = blob[start + i]
    return data


def native_audit(spans, functions):
    directory = ROOT / "resources/oracle-runs/system-string-leaves-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    image = spans["image"]
    span = image[physical(SYSTEM_CS, 0x0B00):physical(SYSTEM_CS, 0x0C00)]
    rng = random.Random(0x0B0B0B9D)
    blob = bytes(rng.randrange(256) for _ in range(BLOB_SIZE))
    cases = native_cases(rng)
    check(SPAN_AT + len(span) <= BLOB_AT and BLOB_AT + len(blob) <= CASES_AT and CASES_AT + CASE_SIZE * len(cases) <= OUTPUT, "COM layout overlaps")
    t = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main", f".org {MAIN_AT - 0x100:#x}", "main:",
         "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}", "    cld",
         f"    mov ax,{SYS_N:#x}", "    mov es,ax", "    mov si,offset span", "    mov di,0x0b00", f"    mov cx,{len(span):#x}", "    rep movsb",
         "    push cs", "    pop es", f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
         f"    mov word ptr [{HEADER + 4:#x}],{len(cases)}", f"    mov word ptr [{HEADER + 6:#x}],0",
         "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
         "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8", "    mov ah,0x40", "    int 0x21", "    jc failure",
         "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    push cs", "    pop ds", "    push cs", "    pop es",
         "    mov al,0xa5", f"    mov di,{WINDOW_LOW:#x}", f"    mov cx,{WINDOW_SIZE:#x}", "    rep stosb",
         "    mov ax,word ptr [index]", f"    mov bx,{CASE_SIZE}", "    mul bx", "    add ax,offset cases", "    mov bx,ax", "    mov word ptr [case_ptr],ax",
         f"    mov ax,{DATA_N:#x}", "    mov es,ax", "    xor di,di", "    mov cx,0x8000", "    mov ax,0xa5a5", "    rep stosw",
         # the destination string, then the source string (its bytes may overwrite the destination's)
         "    mov di,word ptr [bx+4]", "    mov al,byte ptr [bx+7]", "    stosb", "    xor ch,ch", "    mov cl,al", "    mov si,offset blob",
         "    add si,word ptr [bx+10]", "    rep movsb",
         "    mov di,word ptr [bx+2]", "    mov al,byte ptr [bx+6]", "    stosb", "    xor ch,ch", "    mov cl,al", "    mov si,offset blob",
         "    add si,word ptr [bx+8]", "    rep movsb",
         f"    mov ax,{DATA_N:#x}", "    push ax", "    push word ptr [bx+4]", "    push ax", "    push word ptr [bx+2]",
         "    mov ax,0x0b0b", "    cmp byte ptr [bx],0", "    je chosen", "    mov ax,0x0b98", "chosen:", "    mov word ptr [target_far],ax",
         "    mov bl,byte ptr [bx+1]", "    xor bh,bh", "    shl bx,1",
         "    push word ptr [flags_table+bx]", "    push cs", "    pop es", "    mov ax,0x4444", "    mov ds,ax"]
    for name, value in ENTRY_REGS.items():
        t.append(f"    mov {name},{value:#x}")
    t += ["    popf", "    call dword ptr cs:[target_far]", "returned:"]
    for i, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        t.append(f"    mov word ptr cs:[{OUTPUT + 2 * i:#x}],{reg}")
    t += [f"    mov word ptr cs:[{OUTPUT + 18:#x}],offset returned", f"    mov word ptr cs:[{OUTPUT + 20:#x}],ds",
          f"    mov word ptr cs:[{OUTPUT + 22:#x}],es", f"    mov word ptr cs:[{OUTPUT + 24:#x}],ss",
          f"    mov sp,{STACK_TOP + 0x800:#x}", "    pushf", "    pop ax", f"    mov word ptr cs:[{OUTPUT + 26:#x}],ax",
          "    cld", "    push cs", "    pop ds", "    push cs", "    pop es", f"    mov di,{OUTPUT + 28:#x}",
          f"    mov si,{WINDOW_LOW:#x}", f"    mov cx,{WINDOW_SIZE:#x}", "    rep movsb", "    sti",
          "    mov bx,word ptr [handle]", f"    mov dx,{OUTPUT:#x}", f"    mov cx,{28 + WINDOW_SIZE}", "    mov ah,0x40", "    int 0x21", "    jc failure",
          f"    mov ax,{DATA_N:#x}", "    mov ds,ax", "    xor dx,dx", "    mov cx,0x8000", "    mov ah,0x40", "    int 0x21", "    jc failure",
          "    mov dx,0x8000", "    mov cx,0x8000", "    mov ah,0x40", "    int 0x21", "    jc failure", "    push cs", "    pop ds",
          "    inc word ptr [index]", "    mov ax,word ptr [index]", f"    cmp ax,{len(cases)}", "    jb again",
          "    mov bx,word ptr [handle]", "    mov ah,0x3e", "    int 0x21", "    mov ax,0x4c00", "    int 0x21",
          "failure:", "    mov ax,0x4c01", "    int 0x21",
          'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", "case_ptr: .word 0", f"target_far: .word 0x0b0b, {SYS_N:#x}",
          "flags_table: .word " + ",".join(f"{f:#x}" for f in NATIVE_FLAGS)]
    table = b"".join(struct.pack("<BBHHBBHH", c["leaf"], c["flags"], c["s"] & 0xFFFF, c["d"] & 0xFFFF, c["slen"], c["dlen"], c["sblob"], c["dblob"])
                     + bytes(CASE_SIZE - 12) for c in cases)
    for label, data, where in (("span", span, SPAN_AT), ("blob", blob, BLOB_AT), ("cases", table, CASES_AT)):
        t += [f".org {where - 0x100:#x}", f"{label}:"]
        for i in range(0, len(data), 16):
            t.append("    .byte " + ",".join(f"{b:#x}" for b in data[i:i + 16]))
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
    for n, case in enumerate(cases):
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        memory = bytearray(SIZE)
        at = physical(SYS_N, 0x0B00); memory[at:at + len(span)] = span
        at = physical(DATA_N, 0); memory[at:at + 0x10000] = native_memory(case, blob)
        for i in range(WINDOW_SIZE):
            memory[physical(segment, WINDOW_LOW + i)] = 0xA5
        sp = STACK_TOP
        for value in (DATA_N, case["d"] & 0xFFFF, DATA_N, case["s"] & 0xFFFF, segment, returned):
            sp -= 2; put16(memory, segment, sp, value)
        leaf = ("load", "concat")[case["leaf"]]
        cpu = Cpu(*ENTRY_REGS.values(), sp, SYS_N, LEAVES[leaf][0], 0x4444, segment, segment, NATIVE_FLAGS[case["flags"]])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        check(functions[leaf](ram, SIZE, ctypes.byref(cpu)) == 0, f"C status on native case {n}")
        for (name, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, name) == want, f"native case {n} ({leaf}) {name}: C={getattr(cpu, name):04x}, DOS={want:04x}")
        got = bytes(ram[physical(segment, WINDOW_LOW):physical(segment, WINDOW_LOW) + WINDOW_SIZE]) + bytes(ram[physical(DATA_N, 0):physical(DATA_N, 0) + 0x10000])
        if got != record[28:]:
            differences = [i for i, (a, b) in enumerate(zip(got, record[28:])) if a != b]
            raise ValueError(f"native case {n} ({leaf}) bytes differ at {[hex(d) for d in differences[:8]]} ({len(differences)} bytes)")
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {len(cases)} (load and concatenation alternating; apart, overlapping both ways, "
          f"same and segment-wrapping strings); all 14 CPU words, the stack window and the whole 64 KiB string segment; no mask")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}; SYSTEM span 0B00..0BFF at {SYS_N:04X}")
    return dict(cases=len(cases), result_sha256=digest)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unicorn", action="store_true")
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    image, spans = source()
    spans["image"] = image
    report = dict(target={k: dict(bytes=v["bytes"], instructions=v["instructions"], sha256=v["sha256"]) for k, v in spans.items() if k in LEAVES})
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-strings-") as temp:
            functions = library(Path(temp))
            if args.unicorn:
                report["unicorn"] = differential(spans, functions)
            if args.native:
                report["native"] = native_audit(spans, functions)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
