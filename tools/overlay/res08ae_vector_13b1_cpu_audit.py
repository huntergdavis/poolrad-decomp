#!/usr/bin/env python3
"""RES_08AE +13B1 original-byte parity: unmasked Unicorn CPU14 + 1 MiB under a declared, scripted,
fail-closed DOS-vector/port environment; composed original-bytes 137C->13B1 round trip; synthetic native run
(frame mechanics only, flags SF/ZF/PF masked because IN 61h reads live emulated hardware)."""
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

CHECKER_ROWS = ('RES_08AE_13B1',)
ENTRY, END, CODE = 0x13B1, 0x13EA, 0x8800
PRIOR_ENTRY = 0x137C
BODY_SHA = '0fa5df2cbe185bdb43d4ff225e14b5b23a6893e85721aac20a99f86b34a0b019'
PRIOR_SHA = '2dbbe197d8c276b5cb7dc04dc19e0156b06d1772f880396de147fc244470696d'
EXPANDED_SHA = 'a42a57e68d3f7815831821cc9365bb917befda45a58e958544451fc7a878df8e'
RELOCATED = (0x13B3,)                    # live segment immediate (loader relocation)
SLOT = 0x1378                            # saved old vector, CS:1378..137B: INPUT, outside the span, written by 137C
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x7202, 0x0ED7, 0x0003, 0x0046)
STATUS = {0: 'RETURN', 1: 'ARGUMENT', 2: 'ENV_DOS', 3: 'ENV_PORT'}
LISTED_OUT = (0x40, 0x61, 0xC0)
LISTED_IN = (0x61,)
DOS_HOOK = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8), ctypes.POINTER(Cpu),
                            ctypes.POINTER(ctypes.c_uint16))
PORT_HOOK = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.c_uint16, ctypes.c_uint8)
PORT_IN_HOOK = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.c_uint16, ctypes.POINTER(ctypes.c_uint8))

class Env(ctypes.Structure):
    _fields_ = [('context', ctypes.c_void_p), ('dos', DOS_HOOK), ('port_out', PORT_HOOK), ('port_in', PORT_IN_HOOK)]

def check(condition, message):
    if not condition:
        raise ValueError(message)

class Environment:
    """The ONE declared environment for 13B1: the documented DOS vector-table service for AH=25h ONLY
    (AH=35h is not named), OUT to ports 40h/61h/C0h and IN from port 61h. Anything else is a named
    refusal, never a default. Used by the Unicorn hooks and the C-side hooks alike. `union` widens the
    named set for the composed 137C->13B1 round trip (AH=35h and ports 43h)."""
    def __init__(self, table_base, script, in_value=0, union=False):
        self.table_base, self.script, self.calls, self.log = table_base, list(script), 0, []
        self.in_value, self.union = in_value, union
    def dos(self, mem, regs):
        ah, al = regs['ax'] >> 8, regs['ax'] & 255
        entry = (self.table_base + 4 * al) & (SIZE - 1)
        if ah == 0x25:
            for i, v in enumerate((regs['dx'] & 255, regs['dx'] >> 8, regs['ds'] & 255, regs['ds'] >> 8)):
                mem[(entry + i) & (SIZE - 1)] = v
            self.log.append(('int21', 0x25, al, regs['dx'], regs['ds']))
        elif ah == 0x35 and self.union:
            regs['bx'] = mem[entry] | (mem[entry + 1] << 8)
            regs['es'] = mem[entry + 2] | (mem[entry + 3] << 8)
            self.log.append(('int21', 0x35, al, regs['bx'], regs['es']))
        else:
            return None                    # refusal: unlisted DOS function
        flags, al_after = self.script[self.calls % len(self.script)]
        self.calls += 1
        if ah == 0x25:
            regs['ax'] = (regs['ax'] & 0xFF00) | al_after
        return flags
    def port_out(self, port, value):
        if port not in LISTED_OUT + ((0x43,) if self.union else ()):
            return False                   # refusal: unlisted port
        self.log.append(('out', port, value))
        return True
    def port_in(self, port):
        if port not in LISTED_IN:
            return None                    # refusal: unlisted IN
        self.log.append(('in', port, self.in_value))
        return self.in_value

def source():
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, 'START.EXE provenance')
    expanded = bytes(unpack(exe)[0])
    check(sha256(expanded).hexdigest() == EXPANDED_SHA, 'expanded image provenance')
    span = expanded[physical(0x08AE, ENTRY):physical(0x08AE, END)]
    insns = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(span, ENTRY))
    check(sha256(span).hexdigest() == BODY_SHA and len(insns) == 27 and sum(i.size for i in insns) == 57 and
          (insns[-1].address, insns[-1].mnemonic) == (0x13E9, 'retf'), 'span/return differs')
    check([i.address for i in insns if i.mnemonic in ('ret', 'retf')] == [0x13E9], 'unexpected return')
    check(not_branching(insns), 'conditional branch or call present')
    ints = [(i.address, i.op_str) for i in insns if i.mnemonic == 'int']
    check(ints == [(0x13C6, '0x21')], 'INT sites differ')
    outs = [(i.address, i.mnemonic, i.op_str) for i in insns if i.mnemonic in ('out', 'in')]
    check(outs == [(0x13CC, 'out', '0x40, al'), (0x13D0, 'out', '0x40, al'), (0x13D2, 'in', 'al, 0x61'),
                   (0x13D6, 'out', '0x61, al'), (0x13DA, 'out', '0xc0, al'), (0x13DE, 'out', '0xc0, al'),
                   (0x13E2, 'out', '0xc0, al'), (0x13E6, 'out', '0xc0, al')], 'port sites differ')
    rels, _ = relocations(exe, expanded)
    lo, hi = physical(0x08AE, ENTRY), physical(0x08AE, END)
    check(sorted(r - physical(0x08AE, 0) for r in rels if lo <= r < hi) == list(RELOCATED), 'relocation sites differ')
    loads = [(i.address, i.op_str) for i in insns if 'cs:' in i.op_str and i.mnemonic == 'mov']
    check(loads == [(0x13B8, 'dx, word ptr cs:[0x1378]'), (0x13BD, 'ax, word ptr cs:[0x137a]')], 'slot loads differ')
    prior = expanded[physical(0x08AE, PRIOR_ENTRY):physical(0x08AE, ENTRY)]
    check(sha256(prior).hexdigest() == PRIOR_SHA, '137C span (round trip) differs')
    return span, {i.address for i in insns}, prior

def not_branching(insns):
    return not any(i.mnemonic.startswith('j') or i.mnemonic in ('loop', 'call') for i in insns)

def library(directory):
    path = directory / '13b1.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-O2', '-fPIC', '-shared',
                    '-I', str(ROOT / 'include'), str(ROOT / 'src/recovered/resident_08ae_vector_13b1_cpu.c'),
                    '-o', str(path)], check=True)
    fn = ctypes.CDLL(str(path)).pr_r08ae_vector_13b1_cpu
    fn.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(Env)]
    fn.restype = ctypes.c_int
    return fn

def fixture(span, rng, *, ss=0x4000, sp=0x8000, cs=CODE, flags=2, relocated=None, table=0, old=None):
    memory = bytearray([0xA5]) * SIZE
    for i in range(256):                  # the vector table: arbitrary entries (entry 8 is overwritten by the body)
        for j in range(2):
            v = rng.randrange(65536)
            memory[(table + 4 * i + 2 * j) & (SIZE - 1)] = v & 255
            memory[(table + 4 * i + 2 * j + 1) & (SIZE - 1)] = v >> 8
    at = physical(cs, ENTRY)
    memory[at:at + len(span)] = span
    seg = rng.randrange(65536) if relocated is None else relocated
    put16(memory, cs, RELOCATED[0], seg)
    if old is None:
        old = (rng.randrange(65536), rng.randrange(65536))
    put16(memory, cs, SLOT, old[0]); put16(memory, cs, SLOT + 2, old[1])      # the saved vector: an INPUT
    for off, value in ((0, 0x7000), (2, 0x9000)):      # return frame, written last
        put16(memory, ss, sp + off, value)
    cpu = Cpu(*(rng.randrange(65536) for _ in range(7)), sp, cs, ENTRY, rng.randrange(65536), rng.randrange(65536), ss, flags)
    return memory, cpu, old

def compare(memory, cpu, script, in_value, fn, label, table=0, expect_old=None):
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
    env_u = Environment(table, script, in_value)
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
        ip = (address - (cs << 4)) & 0xFFFF
        check(ENTRY <= ip < END, f'{label}: escaped before RETF: {ip:x}')
        seen.add(ip); state['returned'] = ip == 0x13E9
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
        v = env_u.port_in(port)
        check(v is not None, f'{label}: unlisted IN {port:#x} reached in Unicorn')
        return v
    oracle.hook_add(UC_HOOK_CODE, code)
    oracle.hook_add(UC_HOOK_INTR, intr)
    oracle.hook_add(UC_HOOK_INSN, out, None, 1, 0, UC_X86_INS_OUT)
    oracle.hook_add(UC_HOOK_INSN, inn, None, 1, 0, UC_X86_INS_IN)
    oracle.emu_start(physical(cs, ENTRY), 2 * SIZE, count=1000)
    check(state['stopped'] == 'RETURN', f'{label}: missing RETURN stop')
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    env_c = Environment(table, script, in_value)
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
    def in_c(_ctx, port, outv):
        v = env_c.port_in(port)
        if v is None:
            return 1
        outv[0] = v
        return 0
    env = Env(None, DOS_HOOK(dos_c), PORT_HOOK(port_c), PORT_IN_HOOK(in_c))
    status = fn(ram, SIZE, ctypes.byref(cpu), ctypes.byref(env))
    check(STATUS[status] == 'RETURN', f'{label}: C status {STATUS[status]}')
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        want = oracle.reg_read(reg) & 65535
        check(getattr(cpu, name) == want, f'{label}: {name}: C={getattr(cpu,name):04x} oracle={want:04x}')
    check(env_c.log == env_u.log, f'{label}: environment logs differ {env_c.log} / {env_u.log}')
    check(bytes(ram) == bytes(shared), f'{label}: full RAM differs')
    masked = (in_value & 0xFC)
    check(env_u.log[0][:3] == ('int21', 0x25, 8) and env_u.log[1:] == [('out', 0x40, 0xFF), ('out', 0x40, 0xFF), ('in', 0x61, in_value),
          ('out', 0x61, masked), ('out', 0xC0, 0x9F), ('out', 0xC0, 0xBF), ('out', 0xC0, 0xDF), ('out', 0xC0, 0xFF)], f'{label}: events {env_u.log}')
    entry = (table + 32) & (SIZE - 1)
    check(bytes(shared[entry:entry + 4]) == struct.pack('<HH', env_u.log[0][3], env_u.log[0][4]), f'{label}: vector table entry 8')
    if expect_old is not None:      # the vector restored is the one read from CS:1378/137A
        check((env_u.log[0][3], env_u.log[0][4]) == expect_old, f'{label}: restored vector != saved vector')
    return seen, env_u.log

def differential(span, starts, fn):
    rng = random.Random(0x08AE13B1)
    cases = []
    for v in range(256):                                       # IN 61h swept over every byte value
        cases.append((f'in-{v:02x}', [(rng.choice(FLAGS), rng.randrange(256))], v, dict(flags=FLAGS[v % len(FLAGS)]), 0, True))
    for i in range(200):
        cases.append((f'random-{i}', [(rng.choice(FLAGS), rng.randrange(256))], rng.randrange(256),
                      dict(flags=FLAGS[i % len(FLAGS)], relocated=0x08AE if i % 5 == 0 else None), 0, True))
    for sp in (0x000A, 0x000B, 0x00FE, 0xFFFC, 0xFFFB):
        cases.append((f'sp-{sp:x}', [(2, 0)], 0x5A, dict(sp=sp), 0, True))
    for sp in range(0x002E, 0x0040, 2):                         # a low stack ending just above the entry-8 table slot
        cases.append((f'low-stack-{sp:x}', [(0x0246, 8)], 0xC3, dict(ss=0, sp=sp, cs=0x7000), 0, True))
    for sp in range(0x137A, 0x1386, 2):                         # the stack overlaps the saved-vector slot itself
        cases.append((f'slot-overlap-{sp:x}', [(0x0246, 8)], 0x3C, dict(ss=CODE, sp=sp), 0, False))
    cases += [('a20-stack', [(2, 1)], 1, dict(ss=0xFFFF, sp=0x0013), 0, True),
              ('a20-high-stack', [(2, 1)], 2, dict(ss=0xF000, sp=0xFFFC), 0, True),
              ('code-segment-ffff', [(2, 1)], 3, dict(cs=0xFFFF), 0, True),
              ('shifted-table', [(0x0AD7, 3)], 0xFF, dict(), 0x30000, True)]
    seen, stops, events = set(), {}, 0
    for label, script, in_value, kw, table, check_old in cases:
        mem, cpu, old = fixture(span, rng, table=table, **kw)
        s, log = compare(mem, cpu, script, in_value, fn, label, table, old if check_old else None)
        seen |= s; events += len(log); stops['RETURN'] = stops.get('RETURN', 0) + 1
    check(seen == starts, f'starts missing: {sorted(starts - seen)}')
    return dict(cases=len(cases), starts=len(seen), starts_total=len(starts), conditional_edges=0, cpu_words=14,
                ram_bytes=SIZE, masked_ram_bytes=0, stops=stops, events=events, in_values_swept=256)

def roundtrip(span, prior):
    """The ORIGINAL bytes of 137C then 13B1 under the union environment: the pair restores the vector.
    Unicorn only; no C model is involved (par-c i20038: test the pair, not two models of the pair)."""
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_INTR, UC_HOOK_INSN
    from unicorn.x86_const import UC_X86_INS_OUT, UC_X86_INS_IN
    rng = random.Random(0x08AE137C13B1)
    done = 0
    for case in range(64):
        memory = bytearray([0xA5]) * SIZE
        old = (rng.randrange(65536), rng.randrange(65536))
        for i in range(256):
            for j in range(2):
                v = old[j] if i == 8 else rng.randrange(65536)
                memory[4 * i + 2 * j] = v & 255; memory[4 * i + 2 * j + 1] = v >> 8
        cs, ss, sp = CODE, 0x4000, 0x8000
        memory[physical(cs, PRIOR_ENTRY):physical(cs, PRIOR_ENTRY) + len(prior)] = prior
        memory[physical(cs, ENTRY):physical(cs, ENTRY) + len(span)] = span
        seg = 0x08AE if case % 2 else rng.randrange(65536)
        for off in (0x137F, 0x1394, 0x13B3):
            put16(memory, cs, off, seg)
        put16(memory, cs, SLOT, 0xA5A5); put16(memory, cs, SLOT + 2, 0xA5A5)
        regs0 = [rng.randrange(65536) for _ in range(7)]
        ds0, es0, flags0 = rng.randrange(65536), rng.randrange(65536), rng.choice(FLAGS)
        oracle = Uc(UC_ARCH_X86, UC_MODE_16)
        shared = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        for base in (0, SIZE):
            oracle.mem_map_ptr(base, SIZE, 7, ctypes.addressof(shared))
        names = [n for n, _ in Cpu._fields_]
        R = dict(zip(names, REGISTERS))
        env = Environment(0, [(rng.choice(FLAGS), rng.randrange(256)), (rng.choice(FLAGS), rng.randrange(256))], rng.randrange(256), union=True)
        def phys_w(seg_, off, value):
            p = ((seg_ << 4) + off) & (SIZE - 1)
            shared[p] = value & 255; shared[(p + 1) & (SIZE - 1)] = value >> 8
        def phys_r(seg_, off):
            p = ((seg_ << 4) + off) & (SIZE - 1)
            return shared[p] | (shared[(p + 1) & (SIZE - 1)] << 8)
        state = dict(stop=None, end=None)
        def code(uc, address, size, user):
            if state['stop'] == address:
                uc.emu_stop()
            ip = (address - (cs << 4)) & 0xFFFF
            if address == physical(cs, 0x13B0) or address == physical(cs, 0x13E9):
                state['stop'] = None    # set below once the RETF target is known
        def intr(uc, number, user):
            check(number == 0x21, 'round trip: unlisted interrupt')
            g = lambda n: uc.reg_read(R[n]) & 0xFFFF
            nxt, ss_, sp_ = g('ip'), g('ss'), g('sp')
            for value in (g('flags'), g('cs'), nxt):
                sp_ = (sp_ - 2) & 0xFFFF; phys_w(ss_, sp_, value)
            r = dict(ax=g('ax'), bx=g('bx'), dx=g('dx'), ds=g('ds'), es=g('es'))
            flags = env.dos(shared, r)
            check(flags is not None, 'round trip: DOS refusal')
            for n in ('ax', 'bx', 'es'):
                uc.reg_write(R[n], r[n])
            ip, c = phys_r(ss_, sp_), phys_r(ss_, (sp_ + 2) & 0xFFFF)
            uc.reg_write(R['sp'], (sp_ + 6) & 0xFFFF); uc.reg_write(R['cs'], c); uc.reg_write(R['ip'], ip); uc.reg_write(R['flags'], flags)
        def out(uc, port, size, value, user):
            check(env.port_out(port, value & 255), 'round trip: unlisted port')
        def inn(uc, port, size, user):
            v = env.port_in(port)
            check(v is not None, 'round trip: unlisted IN')
            return v
        oracle.hook_add(UC_HOOK_INTR, intr)
        oracle.hook_add(UC_HOOK_INSN, out, None, 1, 0, UC_X86_INS_OUT)
        oracle.hook_add(UC_HOOK_INSN, inn, None, 1, 0, UC_X86_INS_IN)
        for name, value in zip(('ax', 'bx', 'cx', 'dx', 'si', 'di', 'bp'), regs0):
            oracle.reg_write(R[name], value)
        oracle.reg_write(R['ds'], ds0); oracle.reg_write(R['es'], es0); oracle.reg_write(R['ss'], ss)
        sp_now = sp
        for entry in (PRIOR_ENTRY, ENTRY):
            phys_w(ss, sp_now, 0x7000); phys_w(ss, (sp_now + 2) & 0xFFFF, 0x9000)
            oracle.reg_write(R['sp'], sp_now); oracle.reg_write(R['cs'], cs); oracle.reg_write(R['ip'], entry)
            oracle.reg_write(R['flags'], flags0)
            oracle.emu_start(physical(cs, entry), physical(0x9000, 0x7000), count=1000)
            check(oracle.reg_read(R['cs']) == 0x9000 and oracle.reg_read(R['ip']) == 0x7000, 'round trip: did not return to the caller')
            check(oracle.reg_read(R['sp']) == (sp_now + 4) & 0xFFFF, 'round trip: SP not restored')
        entry8 = bytes(shared[32:36])
        check(struct.unpack('<HH', entry8) == old, f'round trip case {case}: vector table entry 8 not restored')
        check(bytes(shared[physical(cs, SLOT):physical(cs, SLOT) + 4]) == struct.pack('<HH', *old), 'round trip: slot != old vector')
        check(oracle.reg_read(R['ds']) == ds0 and oracle.reg_read(R['es']) == es0, 'round trip: DS/ES not restored')
        check([e[:3] for e in env.log if e[0] == 'int21'] == [('int21', 0x35, 8), ('int21', 0x25, 8), ('int21', 0x25, 8)], 'round trip: INT sequence')
        done += 1
    return dict(cases=done, vector_restored=True, uses_original_bytes_of_both=True)

CALLER_N = 0x8800
MAIN_AT, WRAP_AT, CASES_AT = 0x0300, 0x2600, 0x2700
HANDLER_AT, VARS_AT, SHADOW_AT = 0x4000, 0x4F00, 0x5000
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xD800, 0xF000
WINDOW_LOW, WINDOW_SIZE = STACK_TOP - 0x100, 0x100
NATIVE_CASES, CASE_SIZE = 32, 0x20
FLAG_MASK = 0xFF3B                       # SF/ZF/PF (0x00C4) masked: IN 61h reads live emulated hardware
EXTRA = 10
RECORD_BYTES = 28 + WINDOW_SIZE + EXTRA
NATIVE_FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)

def native_cases():
    rng = random.Random(0x08AE13B1D)
    cases = []
    for i in range(NATIVE_CASES):
        reloc = 0x08AE if i % 4 == 0 else rng.randrange(1, 65536)
        script = [(rng.choice(NATIVE_FLAGS), rng.randrange(256)) for _ in range(2)]
        cases.append(dict(flags=i % len(NATIVE_FLAGS), reloc=reloc, old=(rng.randrange(65536), rng.randrange(65536)), script=script))
    return cases

def native_audit(span, function):
    import os
    cases = native_cases()
    directory = ROOT / "resources/oracle-runs/res08ae-13b1-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    table = b""
    for c in cases:
        record = struct.pack("<BBHHH", c["flags"], 0, c["reloc"], *c["old"]) + b"".join(struct.pack("<HH", f, a) for f, a in c["script"])
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
         f"    mov ax,{CALLER_N:#x}", "    mov es,ax", "    mov ax,word ptr [bx+2]", "    mov word ptr es:[0x13b3],ax",
         "    mov ax,word ptr [bx+4]", "    mov word ptr es:[0x1378],ax", "    mov ax,word ptr [bx+6]", "    mov word ptr es:[0x137a],ax",
         "    push cs", "    pop es", "    mov di,offset shadow", "    mov cx,512", "    xor ax,ax", "    rep stosw",
         "    mov ax,word ptr [bx+8]", "    mov word ptr [dos_script],ax", "    mov ax,word ptr [bx+10]", "    mov word ptr [dos_script+2],ax",
         "    mov ax,word ptr [bx+12]", "    mov word ptr [dos_script+4],ax", "    mov ax,word ptr [bx+14]", "    mov word ptr [dos_script+6],ax",
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
        put16(memory, CALLER_N, RELOCATED[0], case["reloc"])
        put16(memory, CALLER_N, SLOT, case["old"][0]); put16(memory, CALLER_N, SLOT + 2, case["old"][1])
        for i in range(WINDOW_SIZE):
            memory[physical(segment, WINDOW_LOW + i)] = 0xA5
        sp = STACK_TOP
        for value in (segment, returned):
            sp -= 2; put16(memory, segment, sp, value)
        cpu = Cpu(*ENTRY_REGS.values(), sp, CALLER_N, ENTRY, segment, segment, segment, NATIVE_FLAGS[case["flags"]])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        env_c = Environment(shadow_phys, case["script"], in_value=0)
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
        def in_c(_ctx, port, out, env_c=env_c):
            v = env_c.port_in(port)
            if v is None:
                return 1
            out[0] = v
            return 0
        env = Env(None, DOS_HOOK(dos_c), PORT_HOOK(port_c), PORT_IN_HOOK(in_c))
        status = function(ram, SIZE, ctypes.byref(cpu), ctypes.byref(env))
        check(status == 0, f"C status on native case {n}: {STATUS[status]}")
        for (reg, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            mask = FLAG_MASK if reg == "flags" else 0xFFFF
            check((getattr(cpu, reg) ^ want) & mask == 0, f"native case {n} {reg}: C={getattr(cpu, reg):04x}, DOS={want:04x}")
        got = bytes(ram[physical(segment, WINDOW_LOW):physical(segment, WINDOW_LOW) + WINDOW_SIZE])
        check(got == record[28:28 + WINDOW_SIZE], f"native case {n} stack window differs")
        slot_c = bytes(ram[physical(CALLER_N, SLOT):physical(CALLER_N, SLOT) + 4])
        shadow_c = bytes(ram[shadow_phys + 32:shadow_phys + 36])
        extra = record[28 + WINDOW_SIZE:]
        check(slot_c == extra[0:4] and shadow_c == extra[4:8] and extra[8:10] == b"\0\0",
              f"native case {n} slot/shadow/refusal differ: C={slot_c.hex()}/{shadow_c.hex()} DOS={extra.hex()}")
    digest = sha256(result).hexdigest()
    dosbox = Path("/usr/bin/dosbox-x")
    print(f"13B1 DOSBox-X normal-core/386 synthetic cases passed: {NATIVE_CASES}; all 14 CPU words (flags SF/ZF/PF masked), the 256-byte stack window, "
          f"the 4-byte slot CS:1378..137B and the shadow vector entry; result SHA-256 {digest}")
    return dict(cases=NATIVE_CASES, result_sha256=digest, cpu_words=14, ram_bytes_compared=WINDOW_SIZE + 8,
                ram_bytes_not_compared=SIZE - WINDOW_SIZE - 8, synthetic_frames=True, real_int21=True, shadow_vector_table=True,
                port_writes_observed=False, flags_masked_bits='SF,ZF,PF (IN 61h reads live emulated hardware)', dosbox_x_sha256=sha256(dosbox.read_bytes()).hexdigest(), pinned_build_used=False)

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--compile-only', action='store_true')
    p.add_argument('--unicorn', action='store_true')
    p.add_argument('--native', action='store_true')
    p.add_argument('--output', type=Path)
    a = p.parse_args()
    report = {}
    with tempfile.TemporaryDirectory(prefix='poolrad-13b1-') as tmp:
        fn = library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK')
            return
        span, starts, prior = source()
        report['target'] = dict(bytes=len(span), starts=len(starts), sha256=BODY_SHA, expanded_sha256=EXPANDED_SHA)
        if a.unicorn:
            report['unicorn'] = differential(span, starts, fn)
            report['round_trip'] = roundtrip(span, prior)
        if a.native:
            report['native'] = native_audit(span, fn)
    if a.output:
        a.output.parent.mkdir(parents=True, exist_ok=True)
        a.output.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    print(json.dumps(report, sort_keys=True))

if __name__ == '__main__':
    main()
