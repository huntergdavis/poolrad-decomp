#!/usr/bin/env python3
"""Deterministic native return fixtures for the U68 +0294 raw upgrade.
Capture pattern adapted from u68_movement_flag_04df_cpu_audit.py; cases
are chosen before running C, with no candidate-based filtering."""
import ctypes,random,struct,subprocess
from hashlib import sha256
from u68_roll_gate_0294_cpu_audit import Cpu,ROOT,SIZE,ENTRY,END,ROLLS,dice,world,physical,put16,check
CALLER_N, DICE_N, STUB_N, DATA_N = 0x8800, 0x9000, 0x9800, 0x5000
MAIN_AT, CASES_AT, SYSTEM_AT, UNIT_AT, DICE_AT = 0x0300, 0x0C00, 0x4000, 0x4200, 0xB000
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xD800, 0xF000
WINDOW_LOW, WINDOW_SIZE = STACK_TOP - 0x100, 0x100
CASE_SIZE = 0x100
FIELD_N, FIELD_AT = 0x7000, 0xB200
OK = 0
RECORD_BYTES = 28 + WINDOW_SIZE + 0x10000
NATIVE_FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)


def native_specifications():
    routes=[(3,0x12,95),(3,0x12,100),(3,0x13,0),(3,0x15,0),(3,0x17,0),(3,0x18,0),
            (1,3,0),(1,8,0),(1,0x10,0),(1,0x12,0),(1,0x12,51),(1,0x12,100)]
    specs=[]
    for field,kind,value in routes:
        for n in range(2):
            specs.append(dict(field=field,members=((DATA_N,0x8000,kind,value),),d=(n*2)))
    for d in range(8):
        specs.append(dict(field=3,d=d,members=((DATA_N,0x8000,0x19,0),)))
    specs.append(dict(members=()))
    specs.append(dict(field=3,members=((DATA_N,0x8000,0,0),)))
    for x,y,map_id in ((16,255,0),(128,254,10),(16,16,11),(255,255,11),(0,0,0),(15,15,0)):
        specs.append(dict(x=x,y=y,map_id=map_id,field=3,members=((DATA_N,0x8000,0x19,0),)))
    for n in range(8):
        specs.append(dict(field=n%4,d=(n%4)*2,members=((DATA_N,0x8000,0,0),(DATA_N,0x8200,0x19,0))))
    return specs

def native_world(rng, specification):
    memory=bytearray([0xA5])*SIZE
    world(memory,DATA_N,rng,seed=rng.randrange(2**32),map_at=(DATA_N,0xC000),**specification)
    data=memory[physical(DATA_N,0):physical(DATA_N,0)+65536]
    data[0x2747:0x2846]=bytes([0xA5])*(0x2846-0x2747)
    return data


def native_memory(unit, chain, system, field_blob, data):
    memory = bytearray(SIZE)
    at = physical(CALLER_N, 0); memory[at:at + len(unit)] = unit
    at = physical(DICE_N, dice.U9E_SPAN[0]); memory[at:at + len(chain["dice"])] = chain["dice"]
    at = physical(STUB_N, dice.STUB_OFFSET); memory[at:at + 5] = struct.pack("<BHH", 0xEA, dice.ENTRY8, DICE_N)
    at = physical(dice.SYSTEM_CS, dice.SYSTEM_OFFSET); memory[at:at + len(system)] = system
    at = physical(DATA_N, 0); memory[at:at + 0x10000] = data
    at=physical(FIELD_N,0x31F);memory[at:at+len(field_blob)]=field_blob
    at=physical(STUB_N,0x2F);memory[at:at+5]=struct.pack("<BHH",0xEA,0x350,FIELD_N)
    return memory


def native_audit(bodies, function):
    import os
    directory = ROOT / "resources/oracle-runs/u68-0294-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    unit=bytearray([0xA5])*END
    for name in ("caller","map"):
        first,blob=bodies[name];unit[first:first+len(blob)]=blob
    for site in ROLLS:struct.pack_into("<H",unit,site+3,STUB_N)
    struct.pack_into("<H",unit,0x2D7,STUB_N)
    field_blob=b"".join(bodies[n][1] for n in ("range","field","nibble"))
    chain={"dice":bodies["dice"][1]}
    system=bytearray([0xA5])*0x87
    for name in ("random","seed","seed_multiplier"):
        first,blob=bodies[name];at=first-0x11C0;system[at:at+len(blob)]=blob
    system=bytes(system)
    check(physical(0x0814, SYSTEM_AT) == physical(dice.SYSTEM_CS, dice.SYSTEM_OFFSET), "the SYSTEM hole is not at 0AF8:11C0")
    rng = random.Random(0x6804DFD)
    cases = []
    for specification in native_specifications():
        data,flags=native_world(rng,specification),len(cases)%len(NATIVE_FLAGS)
        writes = [(o, data[o] | (data[o + 1] << 8)) for o in range(0, 0x10000, 2) if data[o] != 0xA5 or data[o + 1] != 0xA5]
        check(4 + 4 * len(writes) <= CASE_SIZE, f"native case needs {len(writes)} writes")
        cases.append(dict(flags=flags, data=data, writes=writes))
    table = b"".join((struct.pack("<BBH", c["flags"], 0, len(c["writes"])) + b"".join(struct.pack("<HH", o, v) for o, v in c["writes"])).ljust(CASE_SIZE, b"\0")
                     for c in cases)
    dice_blob = chain["dice"]
    check(CASES_AT + len(table) <= SYSTEM_AT and SYSTEM_AT + len(system) <= UNIT_AT and UNIT_AT + len(unit) <= DICE_AT
          and DICE_AT + len(dice_blob) <= FIELD_AT and FIELD_AT + len(field_blob) <= OUTPUT, "COM layout overlaps")
    t = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main", f".org {MAIN_AT - 0x100:#x}", "main:",
         "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}", "    cld"]
    for segment, label, offset, length in ((CALLER_N, "unit", 0, len(unit)), (DICE_N, "dice_span", dice.U9E_SPAN[0], len(dice_blob)), (FIELD_N,"field_span",0x31F,len(field_blob))):
        t += [f"    mov ax,{segment:#x}", "    mov es,ax", f"    mov si,offset {label}", f"    mov di,{offset:#x}", f"    mov cx,{length:#x}", "    rep movsb"]
    t += [f"    mov ax,{STUB_N:#x}", "    mov es,ax", "    mov byte ptr es:[0x2f],0xea", "    mov word ptr es:[0x30],0x350",
          f"    mov word ptr es:[0x32],{FIELD_N:#x}"]
    t += [f"    mov ax,{STUB_N:#x}", "    mov es,ax", f"    mov byte ptr es:[{dice.STUB_OFFSET:#x}],0xea", f"    mov word ptr es:[{dice.STUB_OFFSET + 1:#x}],{dice.ENTRY8:#x}",
          f"    mov word ptr es:[{dice.STUB_OFFSET + 3:#x}],{DICE_N:#x}",
          "    push cs", "    pop es", f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
          f"    mov word ptr [{HEADER + 4:#x}],{len(cases)}", f"    mov word ptr [{HEADER + 6:#x}],0",
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
          'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", f"target_far: .word {ENTRY:#x}, {CALLER_N:#x}",
          "flags_table: .word " + ",".join(f"{f:#x}" for f in NATIVE_FLAGS)]
    for label, blob, where in (("cases", table, CASES_AT), ("system", system, SYSTEM_AT), ("unit", bytes(unit), UNIT_AT), ("dice_span", dice_blob, DICE_AT), ("field_span",field_blob,FIELD_AT)):
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
    found = 0
    for n, case in enumerate(cases):
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        memory = native_memory(unit, chain, system, field_blob, case["data"])
        for i in range(WINDOW_SIZE):
            memory[physical(segment, WINDOW_LOW + i)] = 0xA5
        sp = STACK_TOP
        for value in (segment, returned):
            sp -= 2; put16(memory, segment, sp, value)
        cpu = Cpu(*ENTRY_REGS.values(), sp, CALLER_N, ENTRY, DATA_N, segment, segment, NATIVE_FLAGS[case["flags"]])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        check(function(ram, SIZE, ctypes.byref(cpu), 64, 256, None) == OK, f"C status on native case {n}")
        for (reg, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, reg) == want, f"native case {n} {reg}: C={getattr(cpu, reg):04x}, DOS={want:04x}")
        got = bytes(ram[physical(segment, WINDOW_LOW):physical(segment, WINDOW_LOW) + WINDOW_SIZE]) + bytes(ram[physical(DATA_N, 0):physical(DATA_N, 0) + 0x10000])
        if got != record[28:]:
            differences = [i for i, (a, b) in enumerate(zip(got, record[28:])) if a != b]
            raise ValueError(f"native case {n} bytes differ at {[hex(d) for d in differences[:8]]} ({len(differences)} bytes)")
        found += (cpu.ax & 0xFF) != 0
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {len(cases)} ({found} with a success); all 14 CPU words, the stack window and the "
          f"whole 64 KiB data segment; no mask")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}; SYSTEM at 0AF8:11C0 (COM 4000h), U9E at {DICE_N:04X}, stub at {STUB_N:04X}")
    return dict(cases=len(cases), successes=found, result_sha256=digest, cpu_words=14, stack_bytes=WINDOW_SIZE, ds_bytes=65536, masks=[])
