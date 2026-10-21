#!/usr/bin/env python3
"""RES_0899_0000 (the game's QUIT routine) against the original bytes, as a composed closure: the body, the 08AE driver
dispatch 00DC with its member models (137C, 13B1, 1742, 17AE, 181A, 006C -> RES_08AE_0000 -> Dos.GetTime, SYSTEM LongMul),
Dos.Intr 0A32:000B, and the SYSTEM Halt boundary as the EXIT stop.

The whole loader-expanded START.EXE image is loaded at a test base with the loader's own relocation table applied (what
DOS does; no byte is patched), so every original byte of the closure runs. Declared environment, identical in every
oracle and in the C service callbacks: INT 21h AH=25h/35h over an environment-owned vector table (the 137C/13B1 contract:
hardware frame written, IRET-equivalent return with the pushed flags), INT 21h AH=2Ch from a scripted advancing clock (the
GetTime contract: no frame), OUT/IN through port hooks (IN 61h scripted), and a real-byte INT 10h stub at the vector
(sets the BIOS mode byte 0040:0049 := AL and IRETs). Unicorn: CPU14 + full 1 MiB + stop, unmasked. Media and generated
programs stay under ignored resources/."""
import argparse, ctypes, json, os, random, struct, subprocess, sys, tempfile
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from resident_loader_map import unpack, relocations
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, REGISTERS, physical, put16, check
CHECKER_ROWS = ('RES_0899_0000',)
ENTRY, END = 0x0000, 0x0033
DIGEST = 'f4d6f25f38c01ad8730fc424ad38214c037804afe497ff125794d8e008465d7d'
IMAGE_DIGEST = 'a42a57e68d3f7815831821cc9365bb917befda45a58e958544451fc7a878df8e'
BASE = 0x1000
SEG = dict(body=0x0899, driver=0x08AE, dos=0x0A32, system=0x0AF8, data=0x0C7C)
SS, STUB_SEG = 0x7000, 0x6000
INT10_STUB = bytes.fromhex('1e' '50' 'b84000' '8ed8' '58' 'a24900' '1f' 'cf')  # push ds; push ax; mov ax,40h; mov ds,ax; pop ax; mov [49h],al; pop ds; iret
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
SOURCES = ['resident_0899_quit_0000_cpu', 'resident_08ae_driver_cpu', 'resident_08ae_dispatch_00dc_cpu', 'resident_08ae_entry_017b_cpu',
           'resident_08ae_vector_137c_cpu', 'resident_08ae_vector_13b1_cpu', 'resident_08ae_table_1742_cpu', 'resident_08ae_table_17ae_cpu',
           'resident_08ae_byte_181a_cpu', 'resident_08ae_delay_006c_cpu', 'resident_08ae_time_0000_cpu', 'dos_gettime_00a2_cpu',
           'system_longmul_0a5b_cpu', 'dos_intr_000b_cpu', 'memory_bus', 'ega_hw']

DOS_FN = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8), ctypes.POINTER(Cpu), ctypes.POINTER(ctypes.c_uint16))
OUT_FN = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.c_uint16, ctypes.c_uint8)
IN_FN = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.c_uint16, ctypes.POINTER(ctypes.c_uint8))
TIME_FN = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(Cpu), ctypes.POINTER(Cpu))
VIDEO_FN = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8), ctypes.POINTER(Cpu), ctypes.POINTER(Cpu))


class DriverEnv(ctypes.Structure):
    _fields_ = [('context', ctypes.c_void_p), ('dos_vector', DOS_FN), ('port_out', OUT_FN), ('port_in', IN_FN),
                ('gettime', TIME_FN), ('delay_steps', ctypes.c_uint)]


class QuitEnv(ctypes.Structure):
    _fields_ = [('driver', DriverEnv), ('video', VIDEO_FN), ('video_context', ctypes.c_void_p)]


class DriverDiag(ctypes.Structure):
    _fields_ = [('member', ctypes.c_uint16), ('status', ctypes.c_int), ('time_status', ctypes.c_int), ('calls', ctypes.c_uint)]


class QuitDiag(ctypes.Structure):
    _fields_ = [('driver', DriverDiag), ('intr_status', ctypes.c_int)]


STATUS = {0: 'EXIT', 1: 'ARGUMENT', 2: 'DRIVER', 3: 'INTR', 4: 'TRANSFER'}


def load_image():
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes()
    image = bytes(unpack(exe)[0])
    check(sha256(image).hexdigest() == IMAGE_DIGEST, 'loader-expanded image identity')
    sites = relocations(exe, image)[0]
    return image, sites


def relocate(image, sites, base):
    out = bytearray(image)
    for s in sites:
        out[s:s + 2] = struct.pack('<H', (struct.unpack_from('<H', out, s)[0] + base) & 0xFFFF)
    return bytes(out)


def source(image):
    body = image[SEG['body'] * 16 + ENTRY:SEG['body'] * 16 + END]
    check(sha256(body).hexdigest() == DIGEST, 'QUIT body identity')
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(len(ins) == 19 and sum(i.size for i in ins) == len(body), 'decode covers the span')
    check([(i.address, i.op_str) for i in ins if i.mnemonic == 'lcall'] == [(0x000D, '0x8ae, 0xdc'), (0x0025, '0xa32, 0xb'),
                                                                           (0x002C, '0xaf8, 0xe9')], 'three far calls')
    check([i.address for i in ins if i.mnemonic in ('ret', 'retf')] == [0x0032], 'single RETF (dead, after Halt)')
    check(not [i for i in ins if i.mnemonic.startswith('j') or i.mnemonic in ('int', 'call')], 'straight line')
    return body, {i.address: i for i in ins}


def library(directory):
    path = directory / 'quit.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include')] +
                   [str(ROOT / f'src/recovered/{s}.c') for s in SOURCES] + ['-o', str(path)], check=True)
    f = ctypes.CDLL(str(path)).pr_r0899_quit_0000_cpu
    f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(QuitEnv), ctypes.POINTER(QuitDiag)]
    f.restype = ctypes.c_int
    return f


def clock(start, step):
    """Scripted DOS time: the n-th AH=2Ch call returns start + n*step hundredths, wrapping at midnight."""
    def at(n):
        t = (start + n * step) % 8640000
        h, r = divmod(t, 360000); m, r = divmod(r, 6000); s, c = divmod(r, 100)
        return (h << 8) | m, (s << 8) | c
    return at


class Env:
    """The declared environment, one instance per oracle run (Unicorn) or C run; both see the same script."""
    def __init__(self, table, in61, time_start, time_step):
        self.table = dict(table); self.in61 = in61; self.time = clock(time_start, time_step); self.calls = 0; self.log = []

    def dos(self, ah, al, regs):
        """INT 21h AH=25h/35h over the table. regs: dict with ds/dx (25h); returns dict of outputs (35h: es/bx)."""
        if ah == 0x35:
            off, seg = self.table.get(al, (0x5A5A, 0x5A5A)); self.log.append(('35', al)); return dict(es=seg, bx=off)
        if ah == 0x25:
            self.table[al] = (regs['dx'], regs['ds']); self.log.append(('25', al, regs['dx'], regs['ds'])); return {}
        return None

    def gettime(self):
        cx, dx = self.time(self.calls); self.calls += 1; self.log.append(('2c', cx, dx)); return cx, dx


def build(image_reloc, *, saved=1, w=0x0001, arg=0, mode=0x03, regs_rest=None, ss=SS, sp=0xF000, flags=2, cpu=None, ds_off=0,
          table=None, in61=0xB0, time_start=4736147, time_step=7, bda=0x5A):
    m = bytearray([0x5A]) * SIZE
    b = BASE * 16; m[b:b + len(image_reloc)] = image_reloc
    ds = BASE + SEG['data'] + ds_off
    m[physical(ds, 0x5372)] = 0xEE                              # overwritten by the body from [5373]
    m[physical(ds, 0x5373)] = saved
    put16(m, ds, 0x5374, w); put16(m, ds, 0x2612, arg)
    m[physical(ds, 0x4A1C)] = mode
    for k, v in enumerate(regs_rest or (0x0B0B, 0x0C0C, 0x0D0D, 0x0E0E, 0x0F0F, 0x1010, 0x2121, 0x3131, 0x0246)):
        put16(m, ds, 0x4A04 + 2 * k, v)                       # Regs.BX .. Regs.FLAGS (Regs.AX is written by the body)
    s = physical(STUB_SEG, 0); m[s:s + len(INT10_STUB)] = INT10_STUB
    put16(m, 0, 0x40, 0x0000); put16(m, 0, 0x42, STUB_SEG)       # IVT[10h] -> the declared INT 10h stub
    m[physical(0x0040, 0x0049)] = bda
    put16(m, ss, sp, 0x7000); put16(m, ss, (sp + 2) & 0xFFFF, 0x9001)   # a caller's far return (never used: EXIT)
    r = cpu or (0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777)
    c = Cpu(*r, sp, BASE + SEG['body'], ENTRY, ds, 0x1111, ss, flags)
    env = dict(table=table or {8: (0x0123, 0xF000)}, in61=in61, time_start=time_start, time_step=time_step)
    return m, c, env


MEMBERS = [('08AE:0000 time', 'driver', 0x0000, 0x006C), ('08AE:006C delay', 'driver', 0x006C, 0x00DC),
           ('08AE:00DC dispatch', 'driver', 0x00DC, 0x017B), ('08AE:137C vector', 'driver', 0x137C, 0x13B1),
           ('08AE:13B1 vector', 'driver', 0x13B1, 0x13EA), ('08AE:1742 table', 'driver', 0x1742, 0x17AE),
           ('08AE:17AE table', 'driver', 0x17AE, 0x181A), ('08AE:181A byte', 'driver', 0x181A, 0x1830),
           ('0A32:000B Intr', 'dos', 0x000B, 0x006C), ('0A32:00A2 GetTime', 'dos', 0x00A2, 0x00C7),
           ('0AF8:0A5B LongMul', 'system', 0x0A5B, 0x0A76)]
HALT = (BASE + SEG['system'], 0x00E9)
DEAD = {0x0031, 0x0032}  # POP BP; RETF after LCALL 0AF8:00E9 (Halt never returns): dead code, never reached


def run_unicorn(name, m, c, envspec, body_ips):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_INTR, UC_HOOK_INSN
    from unicorn import x86_const as R
    env = Env(**envspec)
    uc = Uc(UC_ARCH_X86, UC_MODE_16); backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    uc.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing)); uc.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        uc.reg_write(reg, getattr(c, n))
    s = dict(stop=None, steps=0); seen = set(); closure = set()

    def code(em, address, size, user):
        cs = uc.reg_read(R.UC_X86_REG_CS); ip = uc.reg_read(R.UC_X86_REG_IP)
        if (cs, ip) == HALT:
            s['stop'] = 'EXIT'; return uc.emu_stop()
        closure.add((cs, ip))
        if cs == BASE + SEG['body']:
            check(ip in body_ips, f'escape inside 0899 {name} {ip:04x}'); seen.add(ip)
        s['steps'] += 1
        if s['steps'] > 400000:
            s['stop'] = 'RUNAWAY'; uc.emu_stop()

    def intr(em, intno, user):
        ax = uc.reg_read(R.UC_X86_REG_AX); ah, al = ax >> 8, ax & 0xFF
        check(intno == 0x21, f'ENV refusal {name}: INT {intno:#x}')
        if ah == 0x2C:
            cx, dx = env.gettime(); uc.reg_write(R.UC_X86_REG_CX, cx); uc.reg_write(R.UC_X86_REG_DX, dx); return
        check(ah in (0x25, 0x35), f'ENV refusal {name}: INT 21h AH {ah:#x}')
        # The 137C/13B1 contract: the hardware frame (FLAGS, CS, next IP) is written below SP and stays there.
        sp = uc.reg_read(R.UC_X86_REG_SP); ss_ = uc.reg_read(R.UC_X86_REG_SS)
        fl = uc.reg_read(R.UC_X86_REG_EFLAGS) & 0xFFFF
        for k, v in ((2, fl), (4, uc.reg_read(R.UC_X86_REG_CS)), (6, uc.reg_read(R.UC_X86_REG_IP))):
            uc.mem_write(physical(ss_, (sp - k) & 0xFFFF), struct.pack('<H', v))
        out = env.dos(ah, al, dict(ds=uc.reg_read(R.UC_X86_REG_DS), dx=uc.reg_read(R.UC_X86_REG_DX)))
        for k, v in out.items():
            uc.reg_write(dict(es=R.UC_X86_REG_ES, bx=R.UC_X86_REG_BX)[k], v)

    def port_in(em, port, size, user):
        check(port == 0x61 and size == 1, f'ENV refusal {name}: IN {port:#x}')
        env.log.append(('in', port, env.in61)); return env.in61

    def port_out(em, port, size, value, user):
        check(port in (0x40, 0x43, 0x61, 0xC0) and size == 1, f'ENV refusal {name}: OUT {port:#x}')
        env.log.append(('out', port, value))
    uc.hook_add(UC_HOOK_CODE, code); uc.hook_add(UC_HOOK_INTR, intr)
    uc.hook_add(UC_HOOK_INSN, port_in, None, 1, 0, R.UC_X86_INS_IN)
    uc.hook_add(UC_HOOK_INSN, port_out, None, 1, 0, R.UC_X86_INS_OUT)
    uc.emu_start(physical(c.cs, c.ip), 2 * SIZE, count=500000)
    check(s['stop'] == 'EXIT', f'original did not reach Halt {name}: {s["stop"]}')
    return uc, backing, env, seen, closure


def c_env(envspec, keep, stub_at=None, time_sps=None):
    """The same declared environment for the C model, as ctypes callbacks (kept alive through `keep`)."""
    env = Env(**envspec)

    def dos(ctx, ram, cpu, flags_out):
        c = cpu[0]; ah, al = c.ax >> 8, c.ax & 0xFF
        out = env.dos(ah, al, dict(ds=c.ds, dx=c.dx))
        if out is None:
            return 1
        for k, v in out.items():
            setattr(c, k, v)
        # IRET-equivalent: the handler returns with the pushed FLAGS word (at SS:SP+4 of the frame).
        flags_out[0] = ram[physical(c.ss, (c.sp + 4) & 0xFFFF)] | ram[physical(c.ss, (c.sp + 5) & 0xFFFF)] << 8
        return 0

    def pout(ctx, port, value):
        env.log.append(('out', port, value)); return 0 if port in (0x40, 0x43, 0x61, 0xC0) else 1

    def pin(ctx, port, value):
        if port != 0x61:
            return 1
        env.log.append(('in', port, env.in61)); value[0] = env.in61; return 0

    def gettime(ctx, before, after):
        b = before[0]; check(b.ax >> 8 == 0x2C, 'GetTime service AH'); a = Cpu.from_buffer_copy(b)
        if time_sps is not None:
            time_sps.append(b.sp)
        a.cx, a.dx = env.gettime(); a.ip = 0x00A9; after[0] = a; return 0

    def video(ctx, ram, before, after):
        """The INT 10h stub's effect, as its bytes run: [SP-2] := DS, [SP-4] := AX, 0040:0049 := AL, IRET."""
        b = before[0]
        if (b.cs, b.ip) != (stub_at or (STUB_SEG, 0)):
            return 1
        a = Cpu.from_buffer_copy(b)
        for k, v in ((2, b.ds), (4, b.ax)):
            at = physical(b.ss, (b.sp - k) & 0xFFFF); ram[at] = v & 255; ram[(at + 1) % SIZE] = v >> 8
        ram[physical(0x0040, 0x0049)] = b.ax & 0xFF
        rd = lambda o: ram[physical(b.ss, (b.sp + o) & 0xFFFF)] | ram[physical(b.ss, (b.sp + o + 1) & 0xFFFF)] << 8
        a.ip, a.cs, a.flags = rd(0), rd(2), rd(4); a.sp = (b.sp + 6) & 0xFFFF
        after[0] = a; return 0
    fns = [DOS_FN(dos), OUT_FN(pout), IN_FN(pin), TIME_FN(gettime), VIDEO_FN(video)]
    keep.extend(fns)
    q = QuitEnv(DriverEnv(None, fns[0], fns[1], fns[2], fns[3], 1000000), fns[4], None)
    return q, env


def compare(name, state, body_ips, f):
    m, c, envspec = state
    uc, backing, uenv, seen, closure = run_unicorn(name, m, c, envspec, body_ips)
    from unicorn import x86_const as R  # noqa: F401
    actual = Cpu.from_buffer_copy(c); ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m); keep = []
    q, cenv = c_env(envspec, keep); d = QuitDiag()
    status = f(ram, SIZE, ctypes.byref(actual), ctypes.byref(q), ctypes.byref(d))
    check(STATUS.get(status) == 'EXIT', f'C status {name}: {STATUS.get(status)} member {d.driver.member:04x}/{d.driver.status} '
                                         f'time {d.driver.time_status} intr {d.intr_status}')
    check(cenv.log == uenv.log, f'environment events {name}: C {cenv.log[:6]} original {uenv.log[:6]}')
    if bytes(ram) != bytes(backing):
        diff = [hex(i) for i, (a, b) in enumerate(zip(bytes(ram), bytes(backing))) if a != b]
        raise ValueError(f'RAM {name}: {diff[:8]} ({len(diff)} bytes)')
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        want = uc.reg_read(reg) & 0xFFFF
        check(getattr(actual, n) == want, f'CPU {name}/{n} C {getattr(actual, n):04x} original {want:04x}')
    return seen, closure, actual, bytes(ram), uenv, d


def cases():
    rng = random.Random(0x08990000)
    out = []
    for saved in (0, 1, 2, 3, 0xFF):
        for w in (0x0000, 0x0001, 0x0100):
            for arg in (0x0000, 0x0001, 0x00FF, 0x0005):
                out.append((f's{saved:x}-w{w:x}-a{arg:x}', dict(saved=saved, w=w, arg=arg)))
    out += [(f'mode{md:02x}', dict(mode=md)) for md in (0x00, 0x03, 0x0D, 0x10, 0x13, 0xFF)]
    out += [(f'flags{fl:x}-{lab}', dict(flags=fl, **o)) for fl in FLAGS for lab, o in (('muted', dict(saved=2)), ('on', dict(saved=1, arg=1)))]
    out += [(f'in61-{v:02x}', dict(in61=v, saved=0, arg=3)) for v in (0x00, 0x03, 0x30, 0xFF)]
    out += [('midnight-wrap', dict(saved=1, time_start=8639995, time_step=7)), ('slow-clock', dict(saved=0, time_step=1)),
            ('vector-zero', dict(saved=1, table={8: (0, 0)})), ('sp-low', dict(sp=0x0400)), ('sp-odd', dict(sp=0xF001)),
            ('ss-ds', dict(ss=BASE + SEG['data'], sp=0xE000, saved=1, arg=1))]
    for k in range(24):
        out.append((f'random{k}', dict(saved=rng.choice((0, 1, 2, rng.randrange(256))), w=rng.choice((0, 1, 0x100, rng.randrange(65536))),
                                       arg=rng.choice((0, 1, 0xFF, rng.randrange(65536))), mode=rng.randrange(256), flags=rng.choice(FLAGS),
                                       cpu=tuple(rng.randrange(65536) for _ in range(7)), in61=rng.randrange(256),
                                       time_start=rng.randrange(8640000), time_step=rng.randrange(1, 50),
                                       regs_rest=tuple(rng.randrange(65536) for _ in range(9)))))
    return out


def differential(image_reloc, decoded, f):
    seen, closure = set(), set(); specs = cases(); members = {}
    for name, options in specs:
        state = build(image_reloc, **options)
        v, cl, final, ram, env, d = compare(name, state, set(decoded), f); seen |= v; closure |= cl
        # Independent expectations from the body's own bytes (not from either model).
        ds = BASE + SEG['data']; saved = options.get('saved', 1); mode = options.get('mode', 0x03)
        check(ram[physical(ds, 0x5372)] == saved or options.get('ss') is not None, f'{name}: [5372] := [5373]')
        check(ram[physical(ds, 0x4A02)] == mode and ram[physical(ds, 0x4A03)] == 0, f'{name}: Regs.AX = (0, mode)')
        check(ram[physical(0x0040, 0x0049)] == mode, f'{name}: INT 10h saw AL = the saved mode')
        check(final.ax == 0 and (final.cs, final.ip) == HALT, f'{name}: EXIT with AX 0 at Halt')
        sp = options.get('sp', 0xF000); ss = options.get('ss', SS)
        check(struct.unpack_from('<HH', ram, physical(ss, (sp - 6) & 0xFFFF)) == (0x0031, BASE + SEG['body']), f'{name}: Halt return frame')
        members[d.driver.calls] = members.get(d.driver.calls, 0) + 1
    live = set(decoded) - DEAD
    check(seen == live, f'body starts missing {sorted(live - seen)} or dead reached {sorted(seen & DEAD)}')
    by_seg = {}
    for cs, ip in closure:
        by_seg.setdefault(f'{cs - BASE:04X}', set()).add(ip)
    # Per member: instruction starts executed in any case / instructions decoded in that member's span (original bytes).
    per_member = {}
    for label, seg, start, end in MEMBERS:
        base = SEG[seg] * 16
        decoded_ips = {i.address for i in Cs(CS_ARCH_X86, CS_MODE_16).disasm(image_reloc[base + start:base + end], start)}
        ran = {ip for cs, ip in closure if cs == BASE + SEG[seg] and start <= ip < end}
        check(ran <= decoded_ips, f'{label}: executed starts off the decode')
        per_member[label] = f'{len(ran)}/{len(decoded_ips)}'
    return dict(cases=len(specs), stops=dict(EXIT=len(specs)), body_starts=len(seen), body_starts_total=len(live),
                dead_starts=['0031 POP BP', '0032 RETF: after Halt, which never returns'],
                conditional_outcomes_total=0, closure_instruction_starts={k: len(v) for k, v in sorted(by_seg.items())},
                closure_member_starts=per_member,
                driver_member_calls_histogram=dict(sorted(members.items())), cpu_words=14, ram_bytes=SIZE, flag_masks=0)


def main():
    p = argparse.ArgumentParser()
    for flag in ('--compile-only', '--unicorn', '--native'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--output', type=Path); a = p.parse_args()
    with tempfile.TemporaryDirectory(prefix='quit-') as tmp:
        f = library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        image, sites = load_image()
        body, decoded = source(image)
        image_reloc = relocate(image, sites, BASE)
        report = dict(target=dict(span='0899:[0000,0033)', image_span=[SEG['body'] * 16, SEG['body'] * 16 + END], bytes=len(body),
                                  instructions=len(decoded), sha256=DIGEST, image_sha256=IMAGE_DIGEST, relocations=len(sites),
                                  load_base=f'{BASE:04X}'))
        if a.unicorn:
            report['unicorn'] = differential(image_reloc, decoded, f)
        if a.native:
            import res0899_quit_0000_native
            report['native'] = res0899_quit_0000_native.native(image, sites, decoded, f, sys.modules[__name__])
    if a.output:
        a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    main()
