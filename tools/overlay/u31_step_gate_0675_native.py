"""DOSBox-X synthetic oracle for u31 +0675 with U3C6 +0CB9 and RES_00BA_2199 (imported by u31_step_gate_0675_audit.py).

The COM carries the original unit 0x31 code, the three unit 0x3C6 spans that +0CB9
runs and RES_00BA_2199's 111 bytes, and copies them to absolute paragraphs at
start-up. The five far-call segment words are fixup-backed, so they are relocated
loader-style to the stub paragraph 8C00 (EA to +0CB9) and the resident paragraph
8C10. For each case the harness streams the audit fixture's data regions from
WORLDS.BIN into the same absolute segments: the DS image, X, the grid, the buffer,
Y, the sidecar nodes and R. It then pushes the Pascal arguments and far-calls
+0675 with IRQs masked. The record holds all 14 CPU words, 256 stack bytes and
every data region.
"""
import ctypes
from hashlib import sha256
import os
import random
import struct
import subprocess

from u86_procvars_3413_audit import Cpu, ROOT, SIZE, physical, put16
from u33_place_search_1577_audit import U3C6_SPANS

CALLER_N, SCAN_N, STUB_N, RES_N, DATA_DS, OUT_SEG = 0x8800, 0x8A00, 0x8C00, 0x8C10, 0x1900, 0x9400
CHUNKS = ((0x1900, 0x0000, 0x8600), (0x2332, 0x0000, 0x0200), (0x8000, 0x0000, 0x04F0), (0x5010, 0x0000, 0x0400),
          (0x6020, 0x0000, 0x0810), (0x7000, 0x0000, 0x0200), (0x6810, 0x0000, 0x0100))
MAIN_AT, CASES_AT, UNIT31_AT, SPANS_AT, STUB_AT, RES_AT = 0x0300, 0x0C00, 0x1400, 0x3400, 0x3700, 0x3710
HEADER, STACK_TOP, FRAME_LOW, FRAME_SIZE = 0x0A00, 0xF000, 0xEF00, 0x100
RECORD_BYTES = 28 + FRAME_SIZE + sum(c[2] for c in CHUNKS)
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)
CASE_SIZE = 16
STOP_SITES = (0x06DA, 0x0758, 0x076F, 0x0786, 0x079D)


def check(condition, message):
    if not condition:
        raise ValueError(message)


def relocated_unit31(code31):
    code = bytearray(code31)
    struct.pack_into("<H", code, 0x06DA + 3, STUB_N)
    for site in (0x0758, 0x076F, 0x0786, 0x079D):
        struct.pack_into("<H", code, site + 3, RES_N)
    return bytes(code)


def blocks(code31, code3c6, image):
    out = [("unit31", CALLER_N, 0x0000, relocated_unit31(code31), UNIT31_AT)]
    at = SPANS_AT
    for n, (lo, hi) in enumerate(U3C6_SPANS):
        out.append((f"span{n}", SCAN_N, lo, code3c6[lo:hi], at)); at += hi - lo
    out.append(("stub", STUB_N, 0x0070, struct.pack("<BHH", 0xEA, 0x0CB9, SCAN_N), STUB_AT))
    out.append(("resident", RES_N, 0x2199, image[0x2D39:0x2DA8], RES_AT))
    return out


def assembly(code_blocks, cases):
    text = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main",
            f".org {MAIN_AT - 0x100:#x}", "main:",
            "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}", "    cld"]
    for label, segment, offset, data, _ in code_blocks:
        text += [f"    mov ax,{segment:#x}", "    mov es,ax", f"    mov si,offset {label}", f"    mov di,{offset:#x}",
                 f"    mov cx,{len(data):#x}", "    rep movsb"]
    text += ["    push cs", "    pop es",
             f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
             f"    mov word ptr [{HEADER + 4:#x}],{len(cases)}", f"    mov word ptr [{HEADER + 6:#x}],0",
             "    mov dx,offset worlds_name", "    mov ax,0x3d00", "    int 0x21", "    jc failure", "    mov word ptr [world_handle],ax",
             "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
             "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8",
             "    mov ah,0x40", "    int 0x21", "    jc failure",
             "again:"]
    for segment, offset, length in CHUNKS:
        text += ["    mov bx,word ptr [world_handle]", f"    mov cx,{length:#x}", f"    mov dx,{offset:#x}",
                 f"    mov ax,{segment:#x}", "    push ds", "    mov ds,ax", "    mov ah,0x3f", "    int 0x21", "    pop ds",
                 "    jc failure", f"    cmp ax,{length:#x}", "    jne failure"]
    text += ["    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    push cs", "    pop es",
             "    mov al,0xa5", f"    mov di,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE:#x}", "    rep stosb",
             "    mov ax,word ptr [index]", f"    mov bx,{CASE_SIZE}", "    mul bx", "    add ax,offset cases", "    mov si,ax",
             # t44 CODE-stop cases: JMP FAR (EAh, the LCALL's five bytes) over the call site to stopcap; no flag or stack effect
             "    mov bx,word ptr [si+14]", "    mov word ptr [stop_site],bx", "    or bx,bx", "    jz no_patch",
             f"    mov ax,{CALLER_N:#x}", "    mov es,ax", "    mov byte ptr es:[bx],0xea", "    mov word ptr es:[bx+1],offset stopcap",
             "    mov word ptr es:[bx+3],cs", "    push cs", "    pop es", "no_patch:",
             "    push word ptr [si+10]", "    push word ptr [si+8]", "    push word ptr [si+6]", "    push word ptr [si+4]",
             "    mov ax,word ptr [si+2]", "    or ax,ax", "    jnz have_segment", "    mov ax,cs", "have_segment:",
             "    push ax", "    push word ptr [si]",
             "    mov bx,word ptr [si+12]", "    shl bx,1", "    push word ptr [flags_table+bx]",
             f"    mov ax,{DATA_DS:#x}", "    mov ds,ax", "    popf"]
    for name, value in ENTRY_REGS.items():
        text.append(f"    mov {name},{value:#x}")
    text += ["    call dword ptr cs:[target_far]", "returned:"]
    for i, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        text.append(f"    mov word ptr cs:[record+{2 * i}],{reg}")
    text += ["    mov word ptr cs:[record+18],offset returned", "    mov word ptr cs:[record+20],ds",
             "    mov word ptr cs:[record+22],es", "    mov word ptr cs:[record+24],ss", "    jmp common", "stopcap:"]
    for i, reg in enumerate("ax bx cx dx si di bp sp".split()):
        text.append(f"    mov word ptr cs:[record+{2 * i}],{reg}")
    text += ["    mov word ptr cs:[record+20],ds", "    mov word ptr cs:[record+22],es", "    mov word ptr cs:[record+24],ss",
             f"    mov word ptr cs:[record+16],{CALLER_N:#x}", "    mov ax,word ptr cs:[stop_site]", "    mov word ptr cs:[record+18],ax",
             "common:",
             f"    mov sp,{STACK_TOP + 0x800:#x}", "    pushf", "    pop ax", "    mov word ptr cs:[record+26],ax",
             "    push cs", "    pop ds", "    cld",
             "    mov bx,word ptr [stop_site]", "    or bx,bx", "    jz no_restore", f"    mov ax,{CALLER_N:#x}", "    mov es,ax",
             "    mov si,offset unit31", "    add si,bx", "    mov di,bx", "    mov cx,5", "    rep movsb", "no_restore:",
             f"    mov ax,{OUT_SEG:#x}", "    mov es,ax", "    xor di,di", "    mov si,offset record", "    mov cx,28", "    rep movsb",
             f"    mov si,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE:#x}", "    rep movsb"]
    for segment, offset, length in CHUNKS:
        text += [f"    mov ax,{segment:#x}", "    mov ds,ax", f"    mov si,{offset:#x}", f"    mov cx,{length:#x}", "    rep movsb"]
    text += ["    push cs", "    pop ds", "    sti",
             "    mov bx,word ptr [handle]", f"    mov ax,{OUT_SEG:#x}", "    push ds", "    mov ds,ax", "    xor dx,dx",
             f"    mov cx,{RECORD_BYTES}", "    mov ah,0x40", "    int 0x21", "    pop ds", "    jc failure",
             f"    cmp ax,{RECORD_BYTES}", "    jne failure",
             "    inc word ptr [index]", "    mov ax,word ptr [index]", f"    cmp ax,{len(cases)}", "    jb again",
             "    mov bx,word ptr [handle]", "    mov ah,0x3e", "    int 0x21",
             "    mov bx,word ptr [world_handle]", "    mov ah,0x3e", "    int 0x21",
             "    mov ax,0x4c00", "    int 0x21",
             "failure:", "    mov ax,0x4c01", "    int 0x21",
             'filename: .asciz "RESULT.BIN"', 'worlds_name: .asciz "WORLDS.BIN"', "handle: .word 0", "world_handle: .word 0",
             "index: .word 0", "stop_site: .word 0", f"target_far: .word 0x0675, {CALLER_N:#x}",
             "flags_table: .word " + ",".join(f"{f:#x}" for f in FLAGS), "record: .fill 28, 1, 0",
             f".org {CASES_AT - 0x100:#x}", "cases:"]
    for n, case in enumerate(cases):
        w = case["words"]
        r_seg = 0 if case["r_in_frame"] else w[1]
        text.append(f"    .word {w[0]:#x},{r_seg:#x},{w[2]:#x},{w[3]:#x},{w[4]:#x},{w[5]:#x},{n % len(FLAGS)},{case.get('stop', 0):#x}")
    for label, _, _, data, at in code_blocks:
        text += [f".org {at - 0x100:#x}", f"{label}:"]
        for i in range(0, len(data), 16):
            text.append("    .byte " + ",".join(f"{b:#x}" for b in data[i:i + 16]))
    return "\n".join(text) + "\n"


def native_cases():
    spec = [(f"random-{i}", dict()) for i in range(24)]
    spec += [(f"class-1e-{i}", dict(p6_force=0x1E, y6=0xFF - 3 * i, keys=[k for n, k in enumerate((0x20, 0x1E, 0x6F, 0x7D)) if (i >> n) & 1],
                                    open_cells=True, weight=1 + i % 3, steer=True)) for i in range(16)]
    spec += [(f"class-1e-7d-{i}", dict(p6_force=0x1E, y6=0xFF, keys=[0x7D, 0x31 + i], open_cells=True, weight=2, steer=True)) for i in range(4)]
    spec += [("class-1e-none", dict(p6_force=0x1E, y6=10, keys=[], open_cells=True, weight=1, steer=True)),
             ("class-1e-none-ff", dict(p6_force=0x1E, y6=0xFF, keys=[], open_cells=True, weight=1, steer=True))]
    spec += [(f"class-{v:02x}", dict(p6_force=v)) for v in (0x00, 0x02, 0x80)]
    spec += [("result-in-frame", dict(r_in_frame=True, sp=0xEFF0 + 0x30 - 0x30))]
    # t44: CODE stops at each far call (the flags there: IDIV's at +06DA, CMP's at +0758, OR AL,AL's at +076F/+0786/+079D)
    for k in range(3):
        spec.append((f"code-06da-{k}", dict(stop=0x06DA)))
        for site in STOP_SITES[1:]:
            spec.append((f"code-{site:04x}-{k}", dict(stop=site, p6_force=0x1E, y6=0xFF - k, keys=[], open_cells=True, weight=1 + k, steer=True)))
    return spec


def native_audit(target, code31, code3c6, image, function, Callee):
    from u31_step_gate_0675_audit import fixture
    directory = ROOT / "resources/oracle-runs/u31-0675-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    rng = random.Random(0x3106750)
    worlds, cases = bytearray(), []
    for label, options in native_cases():
        options = dict(options); stop = options.pop("stop", 0)
        memory, _, info = fixture(code31, code3c6, image, rng, **options)
        info["stop"] = stop
        if info["r_in_frame"]:
            info["words"][0] = (STACK_TOP - 12 - 4 - 0x30) & 0xFFFF
        for k, (segment, offset, length) in enumerate(CHUNKS):
            at = physical(0x1800 if k == 0 else segment, offset); worlds += memory[at:at + length]   # fixture DS is 1800
        cases.append(dict(label=label, **info))
    (directory / "WORLDS.BIN").write_bytes(bytes(worlds))
    code_blocks = blocks(code31, code3c6, image)
    asm, obj = directory / "oracle.S", directory / "oracle.o"
    asm.write_text(assembly(code_blocks, cases))
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
    check(count == len(cases) and 0x10000 + 16 * segment <= 16 * DATA_DS, "native header or COM placement differs")
    world_size = sum(c[2] for c in CHUNKS)
    lookups = stops = 0
    for n, case in enumerate(cases):
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        memory = bytearray(SIZE)
        for _, seg, off, data, _ in code_blocks:
            at = physical(seg, off); memory[at:at + len(data)] = data
        if case["stop"]:
            memory[physical(CALLER_N, case["stop"])] = 0xEA          # the oracle's JMP FAR patch: not the original LCALL
        offset = n * world_size
        for seg, off, length in CHUNKS:
            at = physical(seg, off); memory[at:at + length] = worlds[offset:offset + length]; offset += length
        for i in range(FRAME_SIZE):
            memory[physical(segment, FRAME_LOW + i)] = 0xA5
        w = case["words"]
        sp = STACK_TOP
        for value in (w[5], w[4], w[3], w[2], segment if case["r_in_frame"] else w[1], w[0], segment, returned):
            sp -= 2; put16(memory, segment, sp, value)
        cpu = Cpu(*ENTRY_REGS.values(), sp, CALLER_N, 0x0675, DATA_DS, segment, segment, FLAGS[n % len(FLAGS)])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        callee = Callee()
        status = function(ram, SIZE, ctypes.byref(cpu), 1000, ctypes.byref(callee))
        check(status == (2 if case["stop"] else 0), f"C status {status} on native case {n} ({case['label']})")
        stops += bool(case["stop"])
        lookups += callee.site not in (0, 0x06DA)
        for (name, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, name) == want, f"native case {n} ({case['label']}) {name}: C={getattr(cpu, name):04x}, DOS={want:04x}")
        offset = 28
        for seg, off, length in ((segment, FRAME_LOW, FRAME_SIZE),) + CHUNKS:
            at = physical(seg, off)
            if bytes(ram[at:at + length]) != record[offset:offset + length]:
                diff = [i for i in range(length) if ram[at + i] != record[offset + i]]
                raise ValueError(f"native case {n} ({case['label']}) bytes at {seg:04x}:{off + diff[0]:04x} differ ({len(diff)} bytes)")
            offset += length
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {len(cases)} ({lookups} reaching RES_00BA_2199); all 14 CPU words, "
          f"{FRAME_SIZE} stack bytes and {world_size} data bytes per case; no mask")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}; WORLDS.BIN SHA-256 {sha256(bytes(worlds)).hexdigest()[:16]}...")
    print(f"  of which CODE stops at the five far-call sites: {stops}")
    return dict(cases=len(cases), code_stops=stops, lookups=lookups, result_sha256=digest)
