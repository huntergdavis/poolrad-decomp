#!/usr/bin/env python3
"""181A original-byte parity: unmasked Unicorn CPU14 + 1 MiB; masked native corroboration."""
import argparse
import ctypes
from hashlib import sha256
import json
from pathlib import Path
import random
import struct
import subprocess
import tempfile
from capstone import CS_ARCH_X86, CS_MODE_16, Cs
from resident_loader_map import unpack, relocations
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, REGISTERS, physical, put16

CHECKER_ROWS = ('RES_08AE_181A',)
ENTRY, END, CODE = 0x181A, 0x1830, 0x8800
BODY_SHA = '4b13f9458c5b74c2a0dde8cd81bfd304d197c7712bf282efcf6cf4e38b88b79a'
EXPANDED_SHA = 'a42a57e68d3f7815831821cc9365bb917befda45a58e958544451fc7a878df8e'
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x7202, 0x0ED7)

def check(condition, message):
    if not condition:
        raise ValueError(message)

def source():
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, 'START.EXE provenance')
    expanded = bytes(unpack(exe)[0])
    check(sha256(expanded).hexdigest() == EXPANDED_SHA, 'expanded image provenance')
    span = expanded[physical(0x08AE, ENTRY):physical(0x08AE, END)]
    insns = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(span, ENTRY))
    check(sha256(span).hexdigest() == BODY_SHA and len(insns) == 12 and
          sum(i.size for i in insns) == 22 and
          (insns[-1].address, insns[-1].mnemonic, insns[-1].op_str) == (0x182D, 'retf', '2'), 'span/return differs')
    rels, _ = relocations(exe, expanded)
    check(0xA300 in rels, 'MOV AX segment relocation absent')
    return span, {i.address for i in insns}

def ownership():
    from overlay_inventory import find_units, parse_mz
    from u86_procvars_3413_audit import OVR_SHA
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes()
    ovr = (ROOT / 'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    check(sha256(exe).hexdigest() == EXE_SHA and sha256(ovr).hexdigest() == OVR_SHA, 'ownership media')
    targets = (0x181A, 0x137C, 0x13B1, 0x1742, 0x17AE, 0x006C)
    sites = {f'08AE:{a:04X}': [] for a in targets}
    for unit in find_units(exe[parse_mz(exe)['header_bytes']:], ovr):
        for fix in unit['fixups']:
            at = unit['ovr_offset'] + fix - 3
            call = ovr[at:at+5]
            if len(call) != 5 or call[0] != 0x9A:
                continue
            off, seg = struct.unpack_from('<HH', call, 1)
            if seg == 0x08AE and off in targets:
                sites[f'08AE:{off:04X}'].append(f"{unit['stub_para']:04X}:{fix-3:04X}")
    check(not any(sites.values()), 'direct overlay ownership changed')
    image = bytes(unpack(exe)[0])
    callers = []
    for ins in Cs(CS_ARCH_X86, CS_MODE_16).disasm(image[0x8BBC:0x8C5B], 0xDC):
        if ins.mnemonic == 'call' and ins.op_str == '0x181a':
            callers.append(ins.address)
    check(callers == [0x10E, 0x11D], '00DC callers differ')
    return dict(overlay_fixup_sites=sites, resident_00dc_near_sites=callers,
                identity='unresolved', readiness_added=False, credit_claimed=False)

def library(directory):
    path = directory / '181a.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-O2', '-fPIC', '-shared',
                    '-I', str(ROOT / 'include'), str(ROOT / 'src/recovered/resident_08ae_byte_181a_cpu.c'),
                    '-o', str(path)], check=True)
    fn = ctypes.CDLL(str(path)).pr_r08ae_byte_181a_cpu
    fn.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu)]
    fn.restype = ctypes.c_int
    return fn

def fixture(span, rng, arg, *, ss=0x4000, sp=0x8000, data=0x5000, flags=2, background=None):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    at = physical(CODE, ENTRY)
    memory[at:at+len(span)] = span
    put16(memory, CODE, 0x1820, data)
    for off, value in ((0, 0x7000), (2, 0x9000), (4, arg)):
        put16(memory, ss, sp+off, value)
    cpu = Cpu(*(rng.randrange(65536) for _ in range(7)), sp, CODE, ENTRY,
              rng.randrange(65536), rng.randrange(65536), ss, flags)
    return memory, cpu

def compare(memory, cpu, fn, label):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
    oracle = Uc(UC_ARCH_X86, UC_MODE_16)
    shared = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    # Two mappings of the same backing bytes provide A20-off physical aliases.
    for base in (0, SIZE):
        oracle.mem_map_ptr(base, SIZE, 7, ctypes.addressof(shared))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        oracle.reg_write(reg, getattr(cpu, name))
    seen, state = set(), dict(returned=False, stopped=False)
    def hook(uc, address, size, user):
        if state['returned']:
            state['stopped'] = True
            uc.emu_stop()
            return
        ip = address - (CODE << 4)
        check(ENTRY <= ip < END, f'{label}: escaped before RETF: {ip:x}')
        seen.add(ip)
        state['returned'] = ip == 0x182D
    oracle.hook_add(UC_HOOK_CODE, hook)
    oracle.emu_start(physical(CODE, ENTRY), 2*SIZE, count=32)
    check(state['stopped'], f'{label}: missing RETURN stop')
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    check(fn(ram, SIZE, ctypes.byref(cpu)) == 0, f'{label}: C status')
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        want = oracle.reg_read(reg) & 65535
        check(getattr(cpu, name) == want, f'{label}: {name}: C={getattr(cpu,name):04x} oracle={want:04x}')
    check(bytes(ram) == bytes(shared), f'{label}: full RAM differs')
    return seen

def differential(span, starts, fn):
    rng = random.Random(0x08AE181A)
    cases = [(f'byte-{i}', i | (rng.randrange(256) << 8), dict(flags=FLAGS[i % len(FLAGS)])) for i in range(256)]
    cases += [(f'alias-sp-{sp:x}', 0xAB5C, dict(ss=0x5000, data=0x5000, sp=sp, flags=0x0ED7))
              for sp in range(0x0208, 0x0215)]
    cases += [(f'sp-boundary-{sp:x}', 0xCDEF, dict(sp=sp)) for sp in (6, 7, 0xFFF9, 0xFFFA)]
    cases += [('a20-stack', 0x1256, dict(ss=0xFFFF, sp=0x0013)),
              ('a20-data', 0x3456, dict(data=0xFFFF)),
              ('random-ram', 0x9876, dict(background=rng.randbytes(SIZE)))]
    seen = set()
    for label, arg, kw in cases:
        memory, cpu = fixture(span, rng, arg, **kw)
        seen |= compare(memory, cpu, fn, label)
    check(seen == starts, f'starts missing: {starts-seen}')
    # Invalid API inputs are a separate named stop, not execution outcomes.
    memory, base = fixture(span, rng, 0x1234)
    rejects = 0
    for sp, ip, size in ((5, ENTRY, SIZE), (0xFFFB, ENTRY, SIZE), (0x8000, ENTRY+1, SIZE), (0x8000, ENTRY, SIZE-1)):
        cpu = Cpu.from_buffer_copy(base); cpu.sp=sp; cpu.ip=ip
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        before = bytes(cpu)
        check(fn(ram,size,ctypes.byref(cpu)) == 1 and bytes(cpu)==before and bytes(ram)==memory, 'ARGUMENT mutation')
        rejects += 1
    return dict(return_cases=len(cases), argument_cases=rejects, starts=len(seen), starts_total=len(starts),
                conditional_edges=0, calls=0, cpu_words=14, ram_bytes=SIZE, masked_ram_bytes=0,
                stops={'RETURN':len(cases), 'ARGUMENT':rejects})

CALLER_N, DATA_N = 0x8800, 0x5000
MAIN_AT, WRAP_AT, CASES_AT = 0x0300, 0x2600, 0x2700
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xD800, 0xF000
WINDOW_LOW, WINDOW_SIZE = STACK_TOP - 0x100, 0x100
NATIVE_CASES, CASE_SIZE = 32, 0x100
RECORD_BYTES = 28 + WINDOW_SIZE + 0x10000
NATIVE_FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)


def native_audit(span, function):
    import os
    r = dict(entry=ENTRY)
    span = bytearray(span)
    struct.pack_into("<H", span, 0x1820-ENTRY, DATA_N)
    span = bytes(span)
    directory = ROOT / "resources/oracle-runs/res08ae-181a-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    rng = random.Random(0x08AE181AD)
    cases = []
    for n in range(NATIVE_CASES):
        data = bytearray([0xA5]) * 0x10000
        cases.append(dict(flags=n % len(NATIVE_FLAGS), data=data, writes=[],
                          count=rng.randrange(65536), src=0, dst=0))
    table = b"".join((struct.pack("<BBHHHH", c["flags"], 0, len(c["writes"]), c["count"], c["src"], c["dst"])
                      + b"".join(struct.pack("<HH", o, v) for o, v in c["writes"])).ljust(CASE_SIZE, b"\0") for c in cases)
    check(CASES_AT + len(table) <= OUTPUT, "COM layout overlaps")
    push_args = ["    push word ptr [bx+4]"]
    t = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main", f".org {MAIN_AT - 0x100:#x}", "main:",
         "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}", "    cld",
         f"    mov ax,{CALLER_N:#x}", "    mov es,ax", "    mov si,offset wrap", f"    mov di,{r['entry']:#x}", f"    mov cx,{len(span):#x}", "    rep movsb",
         "    push cs", "    pop es", f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
         f"    mov word ptr [{HEADER + 4:#x}],{NATIVE_CASES}", f"    mov word ptr [{HEADER + 6:#x}],0",
         "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
         "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8", "    mov ah,0x40", "    int 0x21", "    jc failure",
         "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    push cs", "    pop ds", "    push cs", "    pop es",
         "    mov al,0xa5", f"    mov di,{WINDOW_LOW:#x}", f"    mov cx,{WINDOW_SIZE:#x}", "    rep stosb",
         "    mov ax,word ptr [index]", f"    mov bx,{CASE_SIZE}", "    mul bx", "    add ax,offset cases", "    mov bx,ax",
         f"    mov ax,{DATA_N:#x}", "    mov es,ax", "    xor di,di", "    mov cx,0x8000", "    mov ax,0xa5a5", "    rep stosw",
         "    mov cx,word ptr [bx+2]", "    jcxz written", "    lea si,[bx+10]", "write_loop:", "    mov di,word ptr [si]", "    mov ax,word ptr [si+2]",
         "    mov word ptr es:[di],ax", "    add si,4", "    loop write_loop", "written:"] + push_args + [
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
          "    inc word ptr [index]", "    mov ax,word ptr [index]", f"    cmp ax,{NATIVE_CASES}", "    jb again",
          "    mov bx,word ptr [handle]", "    mov ah,0x3e", "    int 0x21", "    mov ax,0x4c00", "    int 0x21",
          "failure:", "    mov ax,0x4c01", "    int 0x21",
          'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", f"target_far: .word {r['entry']:#x}, {CALLER_N:#x}",
          "flags_table: .word " + ",".join(f"{f:#x}" for f in NATIVE_FLAGS)]
    for label, blob, where in (("wrap", span, WRAP_AT), ("cases", table, CASES_AT)):
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
    check(len(result) == 8 + NATIVE_CASES * RECORD_BYTES, f"native output incomplete: {len(result)} bytes")
    segment, returned, count, _ = struct.unpack_from("<4H", result)
    check(count == NATIVE_CASES and segment == 0x0814, f"native header differs or COM segment {segment:04x} is not 0814")
    for n, case in enumerate(cases):
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        memory = bytearray(SIZE)
        at = physical(CALLER_N, r["entry"]); memory[at:at + len(span)] = span
        at = physical(DATA_N, 0); memory[at:at + 0x10000] = case["data"]
        for i in range(WINDOW_SIZE):
            memory[physical(segment, WINDOW_LOW + i)] = 0xA5
        sp = STACK_TOP
        args = (case["count"],)
        for value in (*args, segment, returned):
            sp -= 2; put16(memory, segment, sp, value)
        cpu = Cpu(*ENTRY_REGS.values(), sp, CALLER_N, r["entry"], DATA_N, segment, segment, NATIVE_FLAGS[case["flags"]])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        check(function(ram, SIZE, ctypes.byref(cpu)) == 0, f"C status on native case {n}")
        for (reg, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, reg) == want, f"native case {n} {reg}: C={getattr(cpu, reg):04x}, DOS={want:04x}")
        got = bytes(ram[physical(segment, WINDOW_LOW):physical(segment, WINDOW_LOW) + WINDOW_SIZE]) + bytes(ram[physical(DATA_N, 0):physical(DATA_N, 0) + 0x10000])
        if got != record[28:]:
            differences = [i for i, (x, y) in enumerate(zip(got, record[28:])) if x != y]
            raise ValueError(f"native case {n} bytes differ at {[hex(d) for d in differences[:8]]} ({len(differences)} bytes)")
    digest = sha256(result).hexdigest()
    print(f"181A DOSBox-X normal-core/386 synthetic cases passed: {NATIVE_CASES}; all 14 CPU words, the stack window and the whole 64 KiB "
          f"data segment; remaining RAM masked; result SHA-256 {digest}")
    return dict(cases=NATIVE_CASES, result_sha256=digest, cpu_words=14, ram_bytes=WINDOW_SIZE+65536, masked_ram_bytes=SIZE-WINDOW_SIZE-65536, synthetic_frames=True, callee_stubs=0)



def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--compile-only', action='store_true')
    p.add_argument('--unicorn', action='store_true')
    p.add_argument('--native', action='store_true')
    p.add_argument('--output', type=Path)
    a=p.parse_args()
    report={}
    with tempfile.TemporaryDirectory(prefix='poolrad-181a-') as tmp:
        fn=library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK')
            return
        span, starts=source()
        report['target']=dict(bytes=len(span), starts=len(starts), sha256=BODY_SHA, expanded_sha256=EXPANDED_SHA)
        report['ownership']=ownership()
        if a.unicorn: report['unicorn']=differential(span, starts, fn)
        if a.native: report['native']=native_audit(span, fn)
    if a.output:
        a.output.parent.mkdir(parents=True, exist_ok=True)
        a.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(report,sort_keys=True))

if __name__ == '__main__':
    main()
