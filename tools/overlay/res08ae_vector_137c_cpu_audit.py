#!/usr/bin/env python3
"""RES_08AE +137C original-byte parity: unmasked Unicorn CPU14 + 1 MiB under a declared, scripted,
fail-closed DOS-vector/port environment; synthetic native run (frame mechanics only)."""
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

CHECKER_ROWS = ('RES_08AE_137C',)
ENTRY, END, CODE = 0x137C, 0x13B1, 0x8800
BODY_SHA = '2dbbe197d8c276b5cb7dc04dc19e0156b06d1772f880396de147fc244470696d'
EXPANDED_SHA = 'a42a57e68d3f7815831821cc9365bb917befda45a58e958544451fc7a878df8e'
RELOCATED = (0x137F, 0x1394)             # live segment immediates (loader relocations)
SLOT = 0x1378                            # saved old vector, CS:1378..137B: four bytes BEFORE the entry
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x7202, 0x0ED7, 0x0003, 0x0046)
STATUS = {0: 'RETURN', 1: 'ARGUMENT', 2: 'ENV_DOS', 3: 'ENV_PORT'}
LISTED_PORTS = (0x43, 0x40)
DOS_HOOK = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8), ctypes.POINTER(Cpu),
                            ctypes.POINTER(ctypes.c_uint16))
PORT_HOOK = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.c_uint16, ctypes.c_uint8)

class Env(ctypes.Structure):
    _fields_ = [('context', ctypes.c_void_p), ('dos', DOS_HOOK), ('port_out', PORT_HOOK)]

def check(condition, message):
    if not condition:
        raise ValueError(message)

class Environment:
    """The ONE declared environment: the documented DOS vector-table service for AH=35h/25h and
    OUT to ports 43h/40h. Anything else is a named refusal, never a default. Used by the Unicorn
    hooks and the C-side hooks alike."""
    def __init__(self, table_base, script):
        self.table_base, self.script, self.calls, self.log = table_base, list(script), 0, []
    def dos(self, mem, regs):
        ah, al = regs['ax'] >> 8, regs['ax'] & 255
        entry = (self.table_base + 4 * al) & (SIZE - 1)
        if ah == 0x35:
            regs['bx'] = mem[entry] | (mem[entry + 1] << 8)
            regs['es'] = mem[entry + 2] | (mem[entry + 3] << 8)
            self.log.append(('int21', 0x35, al, regs['bx'], regs['es']))
        elif ah == 0x25:
            for i, v in enumerate((regs['dx'] & 255, regs['dx'] >> 8, regs['ds'] & 255, regs['ds'] >> 8)):
                mem[(entry + i) & (SIZE - 1)] = v
            self.log.append(('int21', 0x25, al, regs['dx'], regs['ds']))
        else:
            return None                    # refusal: unlisted DOS function
        flags, al_after = self.script[self.calls % len(self.script)]
        self.calls += 1
        if ah == 0x25:
            regs['ax'] = (regs['ax'] & 0xFF00) | al_after
        return flags
    def port_out(self, port, value):
        if port not in LISTED_PORTS:
            return False                   # refusal: unlisted port
        self.log.append(('out', port, value))
        return True

def source():
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, 'START.EXE provenance')
    expanded = bytes(unpack(exe)[0])
    check(sha256(expanded).hexdigest() == EXPANDED_SHA, 'expanded image provenance')
    span = expanded[physical(0x08AE, ENTRY):physical(0x08AE, END)]
    insns = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(span, ENTRY))
    check(sha256(span).hexdigest() == BODY_SHA and len(insns) == 24 and sum(i.size for i in insns) == 53 and
          (insns[-1].address, insns[-1].mnemonic) == (0x13B0, 'retf'), 'span/return differs')
    check([i.address for i in insns if i.mnemonic in ('ret', 'retf')] == [0x13B0], 'unexpected return')
    check(not_branching(insns), "conditional branch present")
    ints = [(i.address, i.op_str) for i in insns if i.mnemonic == 'int']
    check(ints == [(0x1386, '0x21'), (0x139E, '0x21')], 'INT sites differ')
    outs = [(i.address, i.op_str) for i in insns if i.mnemonic in ('out', 'in')]
    check(outs == [(0x13A3, '0x43, al'), (0x13A8, '0x40, al'), (0x13AC, '0x40, al')], 'port sites differ')
    rels, _ = relocations(exe, expanded)
    lo, hi = physical(0x08AE, ENTRY), physical(0x08AE, END)
    check(sorted(r - physical(0x08AE, 0) for r in rels if lo <= r < hi) == list(RELOCATED), 'relocation sites differ')
    stores = [(i.address, i.op_str) for i in insns if i.mnemonic == 'mov' and 'cs:' in i.op_str and i.op_str.startswith('word ptr cs')]
    check([s for s in stores] == [(0x1388, 'word ptr cs:[0x1378], bx'), (0x138D, 'word ptr cs:[0x137a], es')], 'slot stores differ')
    check(set(expanded[physical(0x08AE, SLOT):physical(0x08AE, ENTRY)]) == {0}, 'slot bytes in the image are not zero')
    return span, {i.address for i in insns}

def not_branching(insns):
    return not any(i.mnemonic.startswith('j') or i.mnemonic in ('loop', 'call') for i in insns)

def library(directory):
    path = directory / '137c.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-O2', '-fPIC', '-shared',
                    '-I', str(ROOT / 'include'), str(ROOT / 'src/recovered/resident_08ae_vector_137c_cpu.c'),
                    '-o', str(path)], check=True)
    fn = ctypes.CDLL(str(path)).pr_r08ae_vector_137c_cpu
    fn.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(Env)]
    fn.restype = ctypes.c_int
    return fn

def fixture(span, rng, *, ss=0x4000, sp=0x8000, cs=CODE, flags=2, relocated=None, table=0, old=None):
    memory = bytearray([0xA5]) * SIZE
    if old is None:
        old = (rng.randrange(65536), rng.randrange(65536))
    for i in range(256):                  # the vector table: arbitrary entries, entry 8 is the old vector
        for j, v in enumerate(old if i == 8 else (rng.randrange(65536), rng.randrange(65536))):
            memory[(table + 4 * i + 2 * j) & (SIZE - 1)] = v & 255
            memory[(table + 4 * i + 2 * j + 1) & (SIZE - 1)] = v >> 8
    at = physical(cs, ENTRY)
    memory[at:at + len(span)] = span
    seg1, seg2 = relocated or (rng.randrange(65536), rng.randrange(65536))
    put16(memory, cs, RELOCATED[0], seg1); put16(memory, cs, RELOCATED[1], seg2)
    for off, value in ((0, 0x7000), (2, 0x9000)):      # return frame, written last
        put16(memory, ss, sp + off, value)
    cpu = Cpu(*(rng.randrange(65536) for _ in range(7)), sp, cs, ENTRY, rng.randrange(65536), rng.randrange(65536), ss, flags)
    return memory, cpu

def compare(memory, cpu, script, fn, label, seen_global, table=0):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_INTR, UC_HOOK_INSN
    from unicorn.x86_const import UC_X86_INS_OUT, UC_X86_INS_IN
    oracle = Uc(UC_ARCH_X86, UC_MODE_16)
    shared = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    for base in (0, SIZE):
        oracle.mem_map_ptr(base, SIZE, 7, ctypes.addressof(shared))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        oracle.reg_write(reg, getattr(cpu, name))
    names = [n for n, _ in Cpu._fields_]
    R = dict(zip(names, REGISTERS))
    env_u = Environment(table, script)
    cs = cpu.cs
    seen, state = set(), dict(returned=False, stopped=None)
    def phys_w(seg, off, value):
        p = ((seg << 4) + off) & (SIZE - 1)
        shared[p] = value & 255; shared[(p + 1) & (SIZE - 1)] = value >> 8
    def phys_r(seg, off):
        p = ((seg << 4) + off) & (SIZE - 1)
        return shared[p] | (shared[(p + 1) & (SIZE - 1)] << 8)
    def code(uc, address, size, user):
        if state['returned']:
            state['stopped'] = 'RETURN'; uc.emu_stop(); return
        ip = address - (cs << 4)
        ip &= 0xFFFF
        check(ENTRY <= ip < END, f'{label}: escaped before RETF: {ip:x}')
        seen.add(ip); state['returned'] = ip == 0x13B0
    def intr(uc, number, user):
        check(number == 0x21, f'{label}: unlisted interrupt {number:#x}')
        g = lambda n: uc.reg_read(R[n]) & 0xFFFF
        nxt, ss, sp = g('ip'), g('ss'), g('sp')
        for value in (g('flags'), g('cs'), nxt):                 # hardware frame: FLAGS, CS, next IP
            sp = (sp - 2) & 0xFFFF; phys_w(ss, sp, value)
        uc.reg_write(R['sp'], sp)
        regs = dict(ax=g('ax'), bx=g('bx'), dx=g('dx'), ds=g('ds'), es=g('es'))
        flags = env_u.dos(shared, regs)
        check(flags is not None, f'{label}: DOS refusal reached in Unicorn (unlisted AH)')
        for n in ('ax', 'bx', 'es'):
            uc.reg_write(R[n], regs[n])
        ip, c = phys_r(ss, sp), phys_r(ss, (sp + 2) & 0xFFFF)     # RETF 2
        uc.reg_write(R['sp'], (sp + 6) & 0xFFFF); uc.reg_write(R['cs'], c); uc.reg_write(R['ip'], ip)
        uc.reg_write(R['flags'], flags)
    def out(uc, port, size, value, user):
        check(env_u.port_out(port, value & 255), f'{label}: unlisted port {port:#x} reached in Unicorn')
    def inn(uc, port, size, user):
        raise ValueError(f'{label}: IN {port:#x} reached 137C')
    oracle.hook_add(UC_HOOK_CODE, code)
    oracle.hook_add(UC_HOOK_INTR, intr)
    oracle.hook_add(UC_HOOK_INSN, out, None, 1, 0, UC_X86_INS_OUT)
    oracle.hook_add(UC_HOOK_INSN, inn, None, 1, 0, UC_X86_INS_IN)
    oracle.emu_start(physical(cs, ENTRY), 2 * SIZE, count=1000)
    check(state['stopped'] == 'RETURN', f'{label}: missing RETURN stop')
    # C side
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    env_c = Environment(table, script)
    def dos_c(_ctx, _ram, c, fl):
        regs = dict(ax=c.contents.ax, bx=c.contents.bx, dx=c.contents.dx, ds=c.contents.ds, es=c.contents.es)
        flags = env_c.dos(ram, regs)
        if flags is None:
            return 1
        c.contents.ax, c.contents.bx, c.contents.es = regs['ax'], regs['bx'], regs['es']
        fl[0] = flags
        return 0
    def port_c(_ctx, port, value):
        return 0 if env_c.port_out(port, value) else 1
    env = Env(None, DOS_HOOK(dos_c), PORT_HOOK(port_c))
    status = fn(ram, SIZE, ctypes.byref(cpu), ctypes.byref(env))
    check(STATUS[status] == 'RETURN', f'{label}: C status {STATUS[status]}')
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        want = oracle.reg_read(reg) & 65535
        check(getattr(cpu, name) == want, f'{label}: {name}: C={getattr(cpu,name):04x} oracle={want:04x}')
    check(env_c.log == env_u.log, f'{label}: environment logs differ {env_c.log} / {env_u.log}')
    check(bytes(ram) == bytes(shared), f'{label}: full RAM differs')
    # the body writes BEFORE its entry: CS:1378..137B must hold the old vector (an OUTPUT, not span bytes)
    slot = physical(cs, SLOT)
    want = struct.pack('<HH', env_u.log[0][3], env_u.log[0][4])
    check(bytes(ram[slot:slot + 4]) == want and bytes(shared[slot:slot + 4]) == want, f'{label}: slot CS:1378..137B differs')
    seen_global |= seen
    return seen, env_u.log

def differential(span, starts, fn):
    rng = random.Random(0x08AE137C)
    cases = []
    for i in range(300):
        script = [(rng.choice(FLAGS), rng.randrange(256)) for _ in range(2)]
        reloc = (0x08AE, 0x08AE) if i % 5 == 0 else None
        cases.append((f'random-{i}', script, dict(flags=FLAGS[i % len(FLAGS)], relocated=reloc), 0))
    for sp in (0x000C, 0x000D, 0x00FE, 0xFFFC, 0xFFFB):
        cases.append((f'sp-{sp:x}', [(2, 0), (0x46, 255)], dict(sp=sp), 0))
    for sp in range(0x0030, 0x0042, 2):                      # a low stack ending just above the entry-8 table slot
        cases.append((f'low-stack-{sp:x}', [(0x0246, 8), (0x0AD7, 9)], dict(ss=0, sp=sp, cs=0x7000), 0))
    cases += [('a20-stack', [(2, 1), (3, 2)], dict(ss=0xFFFF, sp=0x0013), 0),
              ('a20-high-stack', [(2, 1), (3, 2)], dict(ss=0xF000, sp=0xFFFC), 0),
              ('code-segment-ffff', [(2, 1), (3, 2)], dict(cs=0xFFFF), 0),
              ('shifted-table', [(2, 7), (0x0AD7, 3)], dict(), 0x30000)]
    seen, stops, logs = set(), {}, 0
    for label, script, kw, table in cases:
        mem, cpu = fixture(span, rng, table=table, **kw)
        s, log = compare(mem, cpu, script, fn, label, set(), table)
        seen |= s; logs += len(log); stops['RETURN'] = stops.get('RETURN', 0) + 1
        check(log[0][:3] == ('int21', 0x35, 8) and log[1][:3] == ('int21', 0x25, 8) and log[2:] == [('out', 0x43, 0xB6), ('out', 0x40, 0xB1), ('out', 0x40, 0x13)],
              f'{label}: event sequence {log}')
    check(seen == starts, f'starts missing: {sorted(starts - seen)}')
    return dict(cases=len(cases), starts=len(seen), starts_total=len(starts), conditional_edges=0, cpu_words=14,
                ram_bytes=SIZE, masked_ram_bytes=0, stops=stops, events=logs)

CALLER_N = 0x8800
MAIN_AT, WRAP_AT, CASES_AT = 0x0300, 0x2600, 0x2700
HANDLER_AT, VARS_AT, SHADOW_AT = 0x4000, 0x4F00, 0x5000
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xD800, 0xF000
WINDOW_LOW, WINDOW_SIZE = STACK_TOP - 0x100, 0x100
NATIVE_CASES, CASE_SIZE = 32, 0x20
EXTRA = 10
RECORD_BYTES = 28 + WINDOW_SIZE + EXTRA
NATIVE_FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)

def native_cases():
    rng = random.Random(0x08AE137CD)
    cases = []
    for i in range(NATIVE_CASES):
        reloc = (0x08AE, 0x08AE) if i % 4 == 0 else (rng.randrange(1, 65536), rng.randrange(1, 65536))
        script = [(rng.choice(NATIVE_FLAGS), rng.randrange(256)) for _ in range(2)]
        cases.append(dict(flags=i % len(NATIVE_FLAGS), reloc=reloc, old=(rng.randrange(65536), rng.randrange(65536)), script=script))
    return cases

def native_audit(span, function):
    import os
    cases = native_cases()
    directory = ROOT / "resources/oracle-runs/res08ae-137c-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    table = b""
    for c in cases:
        record = struct.pack("<BBHHHH", c["flags"], 0, *c["reloc"], *c["old"]) + b"".join(struct.pack("<HH", f, a) for f, a in c["script"])
        check(len(record) <= CASE_SIZE, "case record too large")
        table += record.ljust(CASE_SIZE, b"\0")
    check(CASES_AT + len(table) <= HANDLER_AT, "COM layout overlaps")
    handler = ["dos_handler:", "    mov word ptr cs:[sv_si],si", "    mov word ptr cs:[sv_di],di",
               "    mov si,word ptr cs:[call_idx]", "    add word ptr cs:[call_idx],4",
               "    mov di,word ptr cs:[dos_script+si]", "    mov word ptr cs:[flags_img],di",
               "    mov di,word ptr cs:[dos_script+si+2]", "    mov word ptr cs:[al_img],di",
               "    mov si,ax", "    and si,0xff", "    shl si,2", "    cmp ah,0x35", "    je h_get", "    cmp ah,0x25", "    je h_set",
               "    mov byte ptr cs:[refused],1", "    jmp h_ret",
               "h_get:", "    mov bx,word ptr cs:[shadow+si]", "    mov es,word ptr cs:[shadow+si+2]", "    jmp h_ret",
               "h_set:", "    mov word ptr cs:[shadow+si],dx", "    mov word ptr cs:[shadow+si+2],ds",
               "    mov al,byte ptr cs:[al_img]",
               "h_ret:", "    mov si,sp", "    mov di,word ptr ss:[si-2]", "    push word ptr cs:[flags_img]", "    popf",
               "    mov word ptr ss:[si-2],di",              # POPF scratch word restored: no stack residue
               "    mov si,word ptr cs:[sv_si]", "    mov di,word ptr cs:[sv_di]", "    retf 2"]
    t = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main", f".org {MAIN_AT - 0x100:#x}", "main:",
         "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}", "    cld",
         f"    mov ax,{CALLER_N:#x}", "    mov es,ax", "    mov si,offset wrap", f"    mov di,{ENTRY:#x}", f"    mov cx,{len(span):#x}", "    rep movsb",
         "    push cs", "    pop es", f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
         f"    mov word ptr [{HEADER + 4:#x}],{NATIVE_CASES}", f"    mov word ptr [{HEADER + 6:#x}],0",
         "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
         "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8", "    mov ah,0x40", "    int 0x21", "    jc failure",
         "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    push cs", "    pop ds", "    push cs", "    pop es",
         "    mov al,0xa5", f"    mov di,{WINDOW_LOW:#x}", f"    mov cx,{WINDOW_SIZE:#x}", "    rep stosb",
         "    mov ax,word ptr [index]", f"    mov bx,{CASE_SIZE}", "    mul bx", "    add ax,offset cases", "    mov bx,ax",
         f"    mov ax,{CALLER_N:#x}", "    mov es,ax", "    mov ax,word ptr [bx+2]", "    mov word ptr es:[0x137f],ax",
         "    mov ax,word ptr [bx+4]", "    mov word ptr es:[0x1394],ax",
         "    mov word ptr es:[0x1378],0xa5a5", "    mov word ptr es:[0x137a],0xa5a5",
         "    push cs", "    pop es", "    mov di,offset shadow", "    mov cx,512", "    xor ax,ax", "    rep stosw",
         "    mov ax,word ptr [bx+6]", "    mov word ptr [shadow+32],ax", "    mov ax,word ptr [bx+8]", "    mov word ptr [shadow+34],ax",
         "    mov ax,word ptr [bx+10]", "    mov word ptr [dos_script],ax", "    mov ax,word ptr [bx+12]", "    mov word ptr [dos_script+2],ax",
         "    mov ax,word ptr [bx+14]", "    mov word ptr [dos_script+4],ax", "    mov ax,word ptr [bx+16]", "    mov word ptr [dos_script+6],ax",
         "    mov word ptr [call_idx],0", "    mov byte ptr [refused],0",
         "    xor ah,ah", "    mov al,byte ptr [bx]", "    shl ax,1", "    mov si,ax", "    push word ptr [flags_table+si]",
         "    xor ax,ax", "    mov es,ax", "    mov ax,word ptr es:[0x84]", "    mov word ptr [save_ivt],ax",
         "    mov ax,word ptr es:[0x86]", "    mov word ptr [save_ivt+2],ax",
         "    mov word ptr es:[0x84],offset dos_handler", "    mov word ptr es:[0x86],cs",
         "    push cs", "    pop es", "    push cs", "    pop ds"]
    for reg, value in ENTRY_REGS.items():
        t.append(f"    mov {reg},{value:#x}")
    t += ["    popf", "    call dword ptr cs:[target_far]", "returned:"]
    for i, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        t.append(f"    mov word ptr cs:[{OUTPUT + 2 * i:#x}],{reg}")
    t += [f"    mov word ptr cs:[{OUTPUT + 18:#x}],offset returned", f"    mov word ptr cs:[{OUTPUT + 20:#x}],ds",
          f"    mov word ptr cs:[{OUTPUT + 22:#x}],es", f"    mov word ptr cs:[{OUTPUT + 24:#x}],ss",
          f"    mov sp,{STACK_TOP + 0x800:#x}", "    pushf", "    pop ax", f"    mov word ptr cs:[{OUTPUT + 26:#x}],ax",
          "    xor ax,ax", "    mov es,ax", "    mov ax,word ptr cs:[save_ivt]", "    mov word ptr es:[0x84],ax",
          "    mov ax,word ptr cs:[save_ivt+2]", "    mov word ptr es:[0x86],ax",         # IVT[21h] restored before any DOS call
          "    cld", "    push cs", "    pop ds", "    push cs", "    pop es", f"    mov di,{OUTPUT + 28:#x}",
          f"    mov si,{WINDOW_LOW:#x}", f"    mov cx,{WINDOW_SIZE:#x}", "    rep movsb",
          f"    mov ax,{CALLER_N:#x}", "    mov es,ax", "    mov ax,word ptr es:[0x1378]", f"    mov word ptr [{OUTPUT + 28 + WINDOW_SIZE}],ax",
          "    mov ax,word ptr es:[0x137a]", f"    mov word ptr [{OUTPUT + 30 + WINDOW_SIZE}],ax",
          "    mov ax,word ptr [shadow+32]", f"    mov word ptr [{OUTPUT + 32 + WINDOW_SIZE}],ax",
          "    mov ax,word ptr [shadow+34]", f"    mov word ptr [{OUTPUT + 34 + WINDOW_SIZE}],ax",
          "    xor ax,ax", "    mov al,byte ptr [refused]", f"    mov word ptr [{OUTPUT + 36 + WINDOW_SIZE}],ax",
          "    push cs", "    pop es", "    sti",
          "    mov bx,word ptr [handle]", f"    mov dx,{OUTPUT:#x}", f"    mov cx,{28 + WINDOW_SIZE + EXTRA}", "    mov ah,0x40", "    int 0x21", "    jc failure",
          "    inc word ptr [index]", "    mov ax,word ptr [index]", f"    cmp ax,{NATIVE_CASES}", "    jb again",
          "    mov bx,word ptr [handle]", "    mov ah,0x3e", "    int 0x21", "    mov ax,0x4c00", "    int 0x21",
          "failure:", "    mov ax,0x4c01", "    int 0x21",
          'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", f"target_far: .word {ENTRY:#x}, {CALLER_N:#x}",
          "flags_table: .word " + ",".join(f"{f:#x}" for f in NATIVE_FLAGS)]
    for label, blob, where in (("wrap", span, WRAP_AT), ("cases", table, CASES_AT)):
        t += [f".org {where - 0x100:#x}", f"{label}:"]
        for i in range(0, len(blob), 16):
            t.append("    .byte " + ",".join(f"{x:#x}" for x in blob[i:i + 16]))
    t += [f".org {HANDLER_AT - 0x100:#x}"] + handler
    t += [f".org {VARS_AT - 0x100:#x}", "sv_si: .word 0", "sv_di: .word 0", "call_idx: .word 0", "flags_img: .word 0", "al_img: .word 0",
          "refused: .byte 0,0", "save_ivt: .word 0,0", "dos_script: .word 0,0,0,0",
          f".org {SHADOW_AT - 0x100:#x}", "shadow: .fill 1024,1,0"]
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
    shadow_phys = physical(segment, SHADOW_AT)
    for n, case in enumerate(cases):
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        memory = bytearray(SIZE)
        at = physical(CALLER_N, ENTRY); memory[at:at + len(span)] = span
        put16(memory, CALLER_N, RELOCATED[0], case["reloc"][0]); put16(memory, CALLER_N, RELOCATED[1], case["reloc"][1])
        put16(memory, CALLER_N, SLOT, 0xA5A5); put16(memory, CALLER_N, SLOT + 2, 0xA5A5)
        for i in range(WINDOW_SIZE):
            memory[physical(segment, WINDOW_LOW + i)] = 0xA5
        put16(memory, segment, SHADOW_AT + 32, case["old"][0]); put16(memory, segment, SHADOW_AT + 34, case["old"][1])
        sp = STACK_TOP
        for value in (segment, returned):
            sp -= 2; put16(memory, segment, sp, value)
        cpu = Cpu(*ENTRY_REGS.values(), sp, CALLER_N, ENTRY, segment, segment, segment, NATIVE_FLAGS[case["flags"]])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        env_c = Environment(shadow_phys, case["script"])
        def dos_c(_ctx, _ram, c, fl, env_c=env_c, ram=ram):
            regs = dict(ax=c.contents.ax, bx=c.contents.bx, dx=c.contents.dx, ds=c.contents.ds, es=c.contents.es)
            flags = env_c.dos(ram, regs)
            if flags is None:
                return 1
            c.contents.ax, c.contents.bx, c.contents.es = regs["ax"], regs["bx"], regs["es"]
            fl[0] = flags
            return 0
        def port_c(_ctx, port, value, env_c=env_c):
            return 0 if env_c.port_out(port, value) else 1
        env = Env(None, DOS_HOOK(dos_c), PORT_HOOK(port_c))
        status = function(ram, SIZE, ctypes.byref(cpu), ctypes.byref(env))
        check(status == 0, f"C status on native case {n}: {STATUS[status]}")
        for (reg, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, reg) == want, f"native case {n} {reg}: C={getattr(cpu, reg):04x}, DOS={want:04x}")
        got = bytes(ram[physical(segment, WINDOW_LOW):physical(segment, WINDOW_LOW) + WINDOW_SIZE])
        check(got == record[28:28 + WINDOW_SIZE], f"native case {n} stack window differs")
        slot_c = bytes(ram[physical(CALLER_N, SLOT):physical(CALLER_N, SLOT) + 4])
        shadow_c = bytes(ram[shadow_phys + 32:shadow_phys + 36])
        extra = record[28 + WINDOW_SIZE:]
        check(slot_c == extra[0:4] and shadow_c == extra[4:8] and extra[8:10] == b"\0\0",
              f"native case {n} slot/shadow/refusal differ: C={slot_c.hex()}/{shadow_c.hex()} DOS={extra.hex()}")
    digest = sha256(result).hexdigest()
    dosbox = Path("/usr/bin/dosbox-x")
    print(f"137C DOSBox-X normal-core/386 synthetic cases passed: {NATIVE_CASES}; all 14 CPU words, the 256-byte stack window, "
          f"the 4-byte slot CS:1378..137B and the shadow vector entry; result SHA-256 {digest}")
    return dict(cases=NATIVE_CASES, result_sha256=digest, cpu_words=14, ram_bytes_compared=WINDOW_SIZE + 8,
                ram_bytes_not_compared=SIZE - WINDOW_SIZE - 8, synthetic_frames=True, real_int21=True, shadow_vector_table=True,
                port_writes_observed=False, dosbox_x_sha256=sha256(dosbox.read_bytes()).hexdigest(), pinned_build_used=False)

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--compile-only', action='store_true')
    p.add_argument('--unicorn', action='store_true')
    p.add_argument('--native', action='store_true')
    p.add_argument('--output', type=Path)
    a = p.parse_args()
    report = {}
    with tempfile.TemporaryDirectory(prefix='poolrad-137c-') as tmp:
        fn = library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK')
            return
        span, starts = source()
        report['target'] = dict(bytes=len(span), starts=len(starts), sha256=BODY_SHA, expanded_sha256=EXPANDED_SHA)
        if a.unicorn:
            report['unicorn'] = differential(span, starts, fn)
        if a.native:
            report['native'] = native_audit(span, fn)
    if a.output:
        a.output.parent.mkdir(parents=True, exist_ok=True)
        a.output.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    print(json.dumps(report, sort_keys=True))

if __name__ == '__main__':
    main()
