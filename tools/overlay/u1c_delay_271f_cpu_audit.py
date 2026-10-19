#!/usr/bin/env python3
"""Original-byte CPU14/1MiB and DOSBox-X evidence for U1C [271F,272D).
The schema-8 queue says 86B; only 14B/6 instructions are executable.
Trailing Pascal strings are data and never loaded by this audit.
Original media and generated guest artifacts remain under ignored resources/.
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

from capstone import Cs, CS_ARCH_X86, CS_MODE_16
import res0709_delay_0dbe_cpu_audit as prior
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, REGISTERS, physical, put16, OVR_SHA
from overlay_inventory import find_units, parse_mz

check = prior.check
OVERLAY, ENTRY, END = 0x9000, 0x271F, 0x272D
SOURCES = ("overlay_u1c_delay_271f", "resident_0709_delay_wrapper_0dbe_cpu", "resident_a40_delay_029c_cpu")


class Diagnostic(ctypes.Structure):
    _fields_ = [("wrapper_status", ctypes.c_int), ("delay_status", ctypes.c_int)]


def source():
    target, delay, wrap = prior.source()
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    check(sha256(ovr).hexdigest() == OVR_SHA, "GAME.OVR identity")
    unit = next(u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr) if u["stub_para"] == 0x1C)
    caller = ovr[unit["ovr_offset"] + ENTRY:unit["ovr_offset"] + END]
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(caller, ENTRY))
    digest = "aec9bf6c4361ea61af680d3d4aed03b5bdc0f62e5d8b3221bc197845ca34f8b1"
    check(len(caller) == 14 and len(ins) == 6 and sum(i.size for i in ins) == 14 and sha256(caller).hexdigest() == digest, "caller identity")
    check([(i.address, i.mnemonic, i.op_str) for i in ins] ==
          [(0x271F, "push", "bp"), (0x2720, "mov", "bp, sp"), (0x2722, "inc", "word ptr [0x49ed]"),
           (0x2726, "lcall", "0x709, 0xdbe"), (0x272B, "pop", "bp"), (0x272C, "retf", "")], "caller instructions")
    target["starts"] += [(OVERLAY, i.address) for i in ins]
    target["caller"] = dict(bytes=14, instructions=6, sha256=digest, file_offset=unit["ovr_offset"] + ENTRY,
                            end_exclusive=END, queue_bytes=86, trailing_bytes="Pascal string data, excluded")
    return target, delay, wrap, caller


def library(directory):
    path = directory / "delay-caller.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2", "-fPIC", "-shared",
                    "-I", str(ROOT / "include"), *[str(ROOT / "src/recovered" / (s + ".c")) for s in SOURCES],
                    "-o", str(path)], check=True)
    lib = ctypes.CDLL(str(path))
    function = lib.pr_u1c_delay_271f_cpu
    function.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu),
                         ctypes.c_uint64, ctypes.POINTER(Diagnostic)]
    function.restype = ctypes.c_int
    return function


def build(delay, wrap, caller, rng, *, factor=1, count=1, pc=0x1234, ss=0x4000, ds=0x3000,
          sp=0x8000, flags=0x202, wrapper=prior.R709, crt=prior.A40, patches=(), background=None):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    for segment, offset, blob in ((crt, prior.DELAY, delay), (wrapper, prior.WRAP, wrap), (OVERLAY, ENTRY, caller)):
        at = physical(segment, offset)
        memory[at:at + len(blob)] = blob
    put16(memory, OVERLAY, 0x2729, wrapper)
    put16(memory, wrapper, 0x0DCF, crt)
    memory[0x400] = rng.randrange(256)
    put16(memory, ds, 0x84FC, count)
    put16(memory, ds, 0x49ED, pc)
    memory[physical(ds, 0x49E2)] = factor
    for i, value in enumerate((0x7000, 0x9200)):
        put16(memory, ss, sp + i * 2, value)
    for segment, offset, value in patches:
        memory[physical(segment, offset)] = value
    cpu = Cpu(*(rng.randrange(65536) for _ in range(7)), sp, OVERLAY, ENTRY, ds, rng.randrange(65536), ss, flags)
    return memory, cpu, wrapper, crt


def compare(label, state, target, function, cap):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
    from unicorn.x86_const import UC_X86_REG_CS, UC_X86_REG_IP
    memory, cpu, wrapper, crt = state
    uc = Uc(UC_ARCH_X86, UC_MODE_16)
    backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    uc.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing))
    uc.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        uc.reg_write(register, getattr(cpu, name))
    starts = {(crt if seg == prior.A40 else wrapper if seg == prior.R709 else seg, ip)
              for seg, ip in target["starts"]}
    seen, edges, calls = set(), set(), []
    status = dict(stop=None, wrapper_status=-1, delay_status=-1, last=None, ret=False, loops=0)

    def stop(code):
        status["stop"] = code
        uc.emu_stop()

    def hook(em, address, size, user):
        cs, ip = uc.reg_read(UC_X86_REG_CS), uc.reg_read(UC_X86_REG_IP)
        key = (cs, ip)
        last = status["last"]
        if status["ret"]:
            status["ret"] = False
            if not calls:
                return stop(0)
            expected, owner = calls.pop()
            status["delay_status" if owner == "delay" else "wrapper_status"] = 0
            if key != expected:
                if owner == "delay":
                    status["wrapper_status"] = 4
                    return stop(3)
                return stop(4)
        if last and last[0] == crt and prior.LOOPS.get(last[1]) == ip and cs == crt:
            status["loops"] += 1
            if status["loops"] > cap:
                status["wrapper_status"], status["delay_status"] = 3, 2
                return stop(3)
        check(key in starts, f"{label}: unexpected fetch {cs:04x}:{ip:04x}")
        if last:
            edges.add((last, key))
        seen.add(key)
        status["last"] = key
        if key in ((OVERLAY, 0x2726), (wrapper, 0x0DCC)):
            expected_ip = prior.WRAP if cs == OVERLAY else prior.DELAY
            raw = bytes(uc.mem_read(address, 5))
            if raw[0] != 0x9A or struct.unpack_from("<H", raw, 1)[0] != expected_ip:
                if cs == OVERLAY:
                    return stop(2)
                status["wrapper_status"] = 2
                return stop(3)
            calls.append(((cs, ip + 5), "wrapper" if cs == OVERLAY else "delay"))
        if key in ((OVERLAY, 0x272C), (wrapper, prior.WRAP_RETF), (crt, prior.DELAY_RETF)):
            status["ret"] = True

    uc.hook_add(UC_HOOK_CODE, hook)
    uc.emu_start(physical(cpu.cs, cpu.ip), 2 * SIZE, count=6000000)
    check(status["stop"] is not None, f"{label}: instruction cap, not an accepted stop")
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    report = Diagnostic()
    got = function(ram, SIZE, ctypes.byref(cpu), cap, ctypes.byref(report))
    check((got, report.wrapper_status, report.delay_status) ==
          (status["stop"], status["wrapper_status"], status["delay_status"]), f"{label}: status/diagnostic mismatch")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        want = uc.reg_read(register) & 65535
        check(getattr(cpu, name) == want, f"{label}: {name}: C={getattr(cpu, name):04x}, original={want:04x}")
    if bytes(ram) != bytes(backing):
        differences = [i for i in range(SIZE) if ram[i] != backing[i]]
        raise ValueError(f"{label}: {len(differences)} RAM differences, first={differences[:8]}")
    canonical = lambda key: (prior.A40 if key[0] == crt else prior.R709 if key[0] == wrapper else key[0], key[1])
    return got, report.wrapper_status, report.delay_status, {canonical(k) for k in seen}, {(canonical(a), canonical(b)) for a, b in edges}


def differential(target, delay, wrap, caller, function):
    rng = random.Random(0x1C271F)
    cases = []
    def add(name, cap=300000, **kwargs):
        cases.append((name, build(delay, wrap, caller, rng, **kwargs), cap))
    for factor in range(256):
        add(f"factor-{factor}-nested-guard", factor=factor, patches=((prior.R709, 0x0DCC, 0x90),), flags=prior.FLAGS[factor % len(prior.FLAGS)])
    for pc in (0, 1, 0xF, 0xFF, 0x7FFF, 0x8000, 0xFFFF):
        for flags in prior.FLAGS:
            add(f"pc-{pc:04x}-flags-{flags:04x}-guard", pc=pc, flags=flags, patches=((OVERLAY, 0x2726, 0x90),))
    add("outer-wrong-offset", patches=((OVERLAY, 0x2727, 0),))
    add("inner-wrong-offset", patches=((prior.R709, 0x0DCD, 0),))
    for factor, count in ((0, 0), (0, 65535), (1, 1), (2, 2), (255, 1)):
        add(f"return-{factor}-{count}", factor=factor, count=count, pc=0xFFFF)
    for count in (0, 1, 2, 65535):
        for cap in (0, 2, 3, 4, 7, 11):
            add(f"budget-{count}-{cap}", count=count, cap=cap)
    for n in range(24):
        add(f"random-{n}", factor=rng.randrange(4), count=rng.randrange(1, 4), pc=rng.randrange(65536), flags=rng.choice(prior.FLAGS))
    for ss, sp in ((0xFFFF, 0x10), (0xFFFF, 0xFFFC), (0x4000, 0x10), (0x4000, 0xFFFC), (0, 0x410)):
        add(f"stack-{ss:04x}-{sp:04x}", ss=ss, sp=sp, factor=1, count=2)
    # Live aliases: INC can change the saved BP or caller's return; pushes can
    # change the factor/count or the BIOS byte. The oracle decides each result.
    for sp in (0x49ED, 0x49EF, 0x49E2, 0x49E4, 0x84FC, 0x84FE, 0x850C):
        add(f"ds-stack-{sp:04x}", ds=0x4000, ss=0x4000, sp=sp, cap=11)
    add("relocated", wrapper=0x8800, crt=0x6000, count=2, factor=2)
    add("random-memory", factor=1, count=2, background=rng.randbytes(SIZE))
    seen, edges, counts = set(), set(), {}
    for label, state, cap in cases:
        code, wrapper_status, delay_status, visited, transitions = compare(label, state, target, function, cap)
        name = f"{code}/{wrapper_status}/{delay_status}"
        counts[name] = counts.get(name, 0) + 1
        seen |= visited
        edges |= transitions
    conditionals = {(key, (key[0], destination)) for key, destinations in target["conditional"].items() for destination in destinations}
    report = dict(cases=len(cases), statuses=counts, cpu_words=14, ram_bytes=SIZE, masks=[],
                  caller_instructions_seen=sum(seg == OVERLAY for seg, ip in seen),
                  closure_instructions_seen=len(seen), closure_instructions=35,
                  conditional_edges_seen=len(conditionals & edges), conditional_edges=len(conditionals),
                  missing_instructions=[f"{seg:04X}:{ip:04X}" for seg, ip in target["starts"] if (seg, ip) not in seen],
                  missing_edges=[f"{a[0]:04X}:{a[1]:04X}->{b[1]:04X}" for a, b in sorted(conditionals - edges)])
    print(json.dumps(report, sort_keys=True), flush=True)
    return report


# Native capture adapted from the merged resident delay audit.
CALLER_N, DATA_N = 0x8800, 0x5000
MAIN_AT, DELAY_AT, WRAP_AT, CASES_AT = 0x0300, 0x255C, 0x2600, 0x2700
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xD800, 0xF000
WINDOW_LOW, WINDOW_SIZE = STACK_TOP - 0x100, 0x100
NATIVE_CASES, CASE_SIZE = 32, 0x20
CALLER_AT = 0x2650
A40, DELAY, WRAP, OK = prior.A40, prior.DELAY, prior.WRAP, 0
RECORD_BYTES = 28 + WINDOW_SIZE + 0x10000
NATIVE_FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)


def native_memory(delay, wrap, caller, data, bios):
    memory = bytearray(SIZE)
    at = physical(A40, DELAY); memory[at:at + len(delay)] = delay
    at = physical(CALLER_N, WRAP); memory[at:at + len(wrap)] = wrap
    at = physical(DATA_N, 0); memory[at:at + 0x10000] = data
    at = physical(OVERLAY, ENTRY); memory[at:at + len(caller)] = caller
    put16(memory, OVERLAY, 0x2729, CALLER_N)
    memory[0x400] = bios
    return memory


def native_audit(delay, wrap, caller, function):
    import os
    directory = ROOT / "resources/oracle-runs/u1c-delay-271f-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    check(physical(0x0814, DELAY_AT) == physical(A40, DELAY), "the Delay hole is not at 0A40:029C")
    rng = random.Random(0x0DBE029CD)
    cases = []
    for n in range(NATIVE_CASES):
        data = bytearray([0xA5]) * 0x10000
        factor, count = rng.choice((0, 1, 2, 3)), rng.randrange(1, 9)
        struct.pack_into("<H", data, 0x84FC, count)
        if n == 31: factor, count = 255, 1
        struct.pack_into("<H", data, 0x84FC, count)
        struct.pack_into("<H", data, 0x49ED, (0, 0xF, 0xFF, 0x7FFF, 0x8000, 0xFFFF)[n % 6])
        data[0x49E2] = factor
        writes = [(o, data[o] | (data[o + 1] << 8)) for o in range(0, 0x10000, 2) if data[o] != 0xA5 or data[o + 1] != 0xA5]
        check(4 + 4 * len(writes) <= CASE_SIZE, f"native case needs {len(writes)} writes")
        cases.append(dict(flags=n % len(NATIVE_FLAGS), data=data, writes=writes))
    table = b"".join((struct.pack("<BBH", c["flags"], 0, len(c["writes"])) + b"".join(struct.pack("<HH", o, v) for o, v in c["writes"]))
                     .ljust(CASE_SIZE, b"\0") for c in cases)
    check(MAIN_AT < DELAY_AT and DELAY_AT + len(delay) <= WRAP_AT and WRAP_AT + len(wrap) <= CALLER_AT and CALLER_AT + len(caller) <= CASES_AT and CASES_AT + len(table) <= OUTPUT,
          "COM layout overlaps")
    t = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main", f".org {MAIN_AT - 0x100:#x}", "main:",
         "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}", "    cld",
         f"    mov ax,{CALLER_N:#x}", "    mov es,ax", "    mov si,offset wrap", f"    mov di,{WRAP:#x}", f"    mov cx,{len(wrap):#x}", "    rep movsb",
         f"    mov ax,{OVERLAY:#x}", "    mov es,ax", "    mov si,offset caller", f"    mov di,{ENTRY:#x}", f"    mov cx,{len(caller)}", "    rep movsb", f"    mov word ptr es:[0x2729],{CALLER_N:#x}",
         "    push cs", "    pop es", f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
         f"    mov word ptr [{HEADER + 4:#x}],{len(cases)}", "    mov ax,0x40", "    mov es,ax", "    xor ah,ah", "    mov al,byte ptr es:[0]",
         f"    mov word ptr [{HEADER + 6:#x}],ax", "    push cs", "    pop es",
         "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
         "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8", "    mov ah,0x40", "    int 0x21", "    jc failure",
         "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    push cs", "    pop ds", "    push cs", "    pop es",
         "    mov al,0xa5", f"    mov di,{WINDOW_LOW:#x}", f"    mov cx,{WINDOW_SIZE:#x}", "    rep stosb",
         "    mov ax,word ptr [index]", f"    mov bx,{CASE_SIZE}", "    mul bx", "    add ax,offset cases", "    mov bx,ax",
         f"    mov ax,{DATA_N:#x}", "    mov es,ax", "    xor di,di", "    mov cx,0x8000", "    mov ax,0xa5a5", "    rep stosw",
         "    mov cx,word ptr [bx+2]", "    jcxz written", "    lea si,[bx+4]", "write_loop:", "    mov di,word ptr [si]", "    mov ax,word ptr [si+2]",
         "    mov word ptr es:[di],ax", "    add si,4", "    loop write_loop", "written:",
         "    xor ah,ah", "    mov al,byte ptr [bx]", "    shl ax,1", "    mov si,ax",
         "    push word ptr [flags_table+si]", "    push cs", "    pop es", f"    mov ax,{DATA_N:#x}", "    mov ds,ax"]
    for reg, value in ENTRY_REGS.items():
        t.append(f"    mov {reg},{value:#x}")
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
          'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", f"target_far: .word {ENTRY:#x}, {OVERLAY:#x}",
          "flags_table: .word " + ",".join(f"{f:#x}" for f in NATIVE_FLAGS)]
    for label, blob, where in (("delay", delay, DELAY_AT), ("wrap", wrap, WRAP_AT), ("caller", caller, CALLER_AT), ("cases", table, CASES_AT)):
        t += [f".org {where - 0x100:#x}", f"{label}:"]
        for i in range(0, len(blob), 16):
            t.append("    .byte " + ",".join(f"{x:#x}" for x in blob[i:i + 16]))
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
    segment, returned, count, bios = struct.unpack_from("<4H", result)
    check(count == len(cases) and segment == 0x0814, f"native header differs or COM segment {segment:04x} is not 0814")
    delayed = 0
    for n, case in enumerate(cases):
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        memory = native_memory(delay, wrap, caller, case["data"], bios & 0xFF)
        for i in range(WINDOW_SIZE):
            memory[physical(segment, WINDOW_LOW + i)] = 0xA5
        sp = STACK_TOP
        for value in (segment, returned):
            sp -= 2; put16(memory, segment, sp, value)
        cpu = Cpu(*ENTRY_REGS.values(), sp, OVERLAY, ENTRY, DATA_N, segment, segment, NATIVE_FLAGS[case["flags"]])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        report = Diagnostic()
        check(function(ram, SIZE, ctypes.byref(cpu), 1 << 32, ctypes.byref(report)) == OK and (report.wrapper_status, report.delay_status) == (0, 0), f"C status on native case {n}")
        for (reg, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, reg) == want, f"native case {n} {reg}: C={getattr(cpu, reg):04x}, DOS={want:04x}")
        got = bytes(ram[physical(segment, WINDOW_LOW):physical(segment, WINDOW_LOW) + WINDOW_SIZE]) + bytes(ram[physical(DATA_N, 0):physical(DATA_N, 0) + 0x10000])
        if got != record[28:]:
            differences = [i for i, (x, y) in enumerate(zip(got, record[28:])) if x != y]
            raise ValueError(f"native case {n} bytes differ at {[hex(d) for d in differences[:8]]} ({len(differences)} bytes)")
        delayed += case["data"][0x49E2] != 0
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {len(cases)} ({delayed} with a nonzero delay); all 14 CPU words, the stack window "
          f"and the whole 64 KiB data segment; no mask; live BIOS byte 0040:0000 = {bios & 0xFF:02X}h")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}; Delay at 0A40:029C (COM 255Ch), wrapper at {CALLER_N:04X}:{WRAP:04X}")
    return dict(cases=len(cases), delayed=delayed, result_sha256=digest, cpu_words=14, stack_bytes=WINDOW_SIZE, ds_bytes=65536, masks=[])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unicorn", action="store_true")
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    target, delay, wrap, caller = source()
    report = dict(caller=target["caller"], closure_bytes=80, closure_instructions=35,
                  wrapper_sha256=target["wrap_sha"], delay_sha256=target["delay_sha"])
    with tempfile.TemporaryDirectory(prefix="poolrad-u1c-delay-") as temp:
        function = library(Path(temp))
        if args.unicorn:
            report["unicorn"] = differential(target, delay, wrap, caller, function)
        if args.native:
            report["native"] = native_audit(delay, wrap, caller, function)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))

if __name__ == "__main__":
    main()
