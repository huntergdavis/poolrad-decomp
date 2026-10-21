#!/usr/bin/env python3
"""Resident 08AE:0000 time-in-hundredths (raw CPU, DRIVER_INTERNAL candidate, never a credit) against the original bytes.

Composes Dos.GetTime 0A32:00A2 (runtime coverage, its INT 21h/2Ch the declared service) and SYSTEM LongMul 0AF8:0A5B.
Unicorn: CPU14 + full 1 MiB; native DOSBox-X with a declared INT 21h stub. Media stays under ignored resources/."""
import argparse, ctypes, json, os, random, struct, subprocess, tempfile
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from resident_loader_map import unpack
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, REGISTERS, physical, put16, check
CHECKER_ROWS = ('RES_08AE_0000',)
SOURCES = ('resident_08ae_time_0000_cpu', 'dos_gettime_00a2_cpu', 'system_longmul_0a5b_cpu', 'memory_bus', 'ega_hw')
CS, DOSSEG, SYS, DS, SS = 0x8800, 0x6000, 0x6800, 0x5000, 0x2000
SPANS = {'body': (0x8AE, 0x0000, 0x006C, 'a8e33767ba204b74a7c909cb85a7580ddbbaa36d60dfafb5fb3425d8c16412ed', 48),
         'dos': (0xA32, 0x00A2, 0x00C7, '175b7cf9fd00cb6dfa0db3b24bef93332f16cf3ca2c711c670f9c80333ced7fc', 19),
         'sys': (0xAF8, 0x0A5B, 0x0A76, None, 16)}
LOAD = {'body': CS, 'dos': DOSSEG, 'sys': SYS}
SEGWORDS = {0x001D: DOSSEG, 0x002D: SYS}
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
SERVICE_FN = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(Cpu), ctypes.POINTER(Cpu))
STOPS = {0: 'RETURN', 1: 'ARGUMENT', 2: 'CALLEE', 4: 'TRANSFER'}


class Bus(ctypes.Structure):
    _fields_ = [('ram', ctypes.POINTER(ctypes.c_uint8)), ('ega', ctypes.c_void_p), ('ports', ctypes.c_void_p),
                ('on_vram_write', ctypes.c_void_p), ('ctx', ctypes.c_void_p)]


class Diagnostic(ctypes.Structure):
    _fields_ = [('gettime_status', ctypes.c_int), ('longmul_status', ctypes.c_int)]


def source():
    image = bytes(unpack((ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes())[0])
    md = Cs(CS_ARCH_X86, CS_MODE_16); code, decoded, meta = {}, {}, {}
    for kind, (seg, a, b, digest, n) in SPANS.items():
        blob = image[physical(seg, a):physical(seg, b)]; ins = list(md.disasm(blob, a))
        check(len(ins) == n and sum(i.size for i in ins) == len(blob) and (digest is None or sha256(blob).hexdigest() == digest),
              f'identity {kind}')
        rets = [i.address for i in ins if i.mnemonic in ('ret', 'retf', 'iret')]
        check(rets == [ins[-1].address], f'first return {kind} {rets}')
        code[kind] = blob; decoded.update({(kind, i.address): i for i in ins})
        meta[f'{seg:04X}:{a:04X}'] = dict(bytes=len(blob), instructions=n, sha256=sha256(blob).hexdigest(),
                                          image_span=[physical(seg, a), physical(seg, b)])
    body = [i for k, i in decoded.items() if k[0] == 'body']
    check([(i.address, i.op_str) for i in body if i.mnemonic == 'lcall'] == [(0x001A, '0xa32, 0xa2'), (0x002A, '0xaf8, 0xa5b')],
          'the two far calls')
    return code, decoded, meta


def library(directory):
    path = directory / 'time.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include'),
                    *[str(ROOT / 'src/recovered' / (s + '.c')) for s in SOURCES], '-o', str(path)], check=True)
    f = ctypes.CDLL(str(path)).pr_r08ae_time_0000_cpu
    f.argtypes = [ctypes.POINTER(Bus), ctypes.c_size_t, ctypes.POINTER(Cpu), SERVICE_FN, ctypes.c_void_p, ctypes.POINTER(Diagnostic)]
    f.restype = ctypes.c_int
    return f


def build(code, *, time=(13, 42, 7, 55), ss=SS, sp=0x8000, flags=2, regs=None, background=0x5A):
    m = bytearray([background]) * SIZE
    for kind, blob in code.items():
        at = physical(LOAD[kind], SPANS[kind][1]); m[at:at + len(blob)] = blob
    for off, seg in SEGWORDS.items():
        put16(m, CS, off, seg)
    put16(m, ss, sp, 0x7000); put16(m, ss, (sp + 2) & 0xFFFF, 0x9001)
    c = Cpu(*(regs or (0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777)), sp, CS, 0x0000, DS, 0x1111, ss, flags)
    return m, c, time


def service_values(time):
    hour, minute, second, sec100 = time
    return ((hour & 255) << 8) | (minute & 255), ((second & 255) << 8) | (sec100 & 255)


def run_c(f, m, c, time):
    cx, dx = service_values(time)
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m); bus = Bus(ram, None, None, None, None); d = Diagnostic()

    def svc(ctx, before, after):
        after[0] = Cpu.from_buffer_copy(before[0]); after[0].cx = cx; after[0].dx = dx; after[0].ip = 0x00A9; return 0
    status = f(ctypes.byref(bus), SIZE, ctypes.byref(c), SERVICE_FN(svc), None, ctypes.byref(d))
    return status, bytes(ram), d


def expected_hundredths(time):
    hour, minute, second, sec100 = time
    # The original keeps only the low word of minute*6000 and second*100 (MUL then XOR DX,DX), so those two
    # terms wrap mod 65536; hour*360000 goes through the 32-bit LongMul. Valid seconds never wrap; minutes >= 11 do.
    return (hour * 360000 + ((minute * 6000) & 0xFFFF) + ((second * 100) & 0xFFFF) + sec100) & 0xFFFFFFFF


def compare(name, state, code, decoded, f):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_INTR
    from unicorn import x86_const as R
    m, c, time = state; cx, dx = service_values(time)
    uc = Uc(UC_ARCH_X86, UC_MODE_16); backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    uc.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing)); uc.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        uc.reg_write(reg, getattr(c, n))
    seg_kind = {v: k for k, v in LOAD.items()}
    s = dict(stop=None, ret=False, calls=[]); seen = set()

    def code_hook(em, address, size, user):
        cs = uc.reg_read(R.UC_X86_REG_CS); ip = uc.reg_read(R.UC_X86_REG_IP)
        if s['ret']:
            s['ret'] = False
            if not s['calls']:
                s['stop'] = 0; return uc.emu_stop()
            if (cs, ip) != s['calls'].pop():
                s['stop'] = 4; return uc.emu_stop()
        key = (seg_kind.get(cs), ip)
        check(key in decoded, f'escape {name} {cs:04x}:{ip:04x}')
        seen.add(key); ins = decoded[key]
        if ins.mnemonic == 'lcall':
            s['calls'].append((cs, ip + ins.size))
        if ins.mnemonic == 'retf':
            s['ret'] = True

    def intr(em, intno, user):
        check(intno == 0x21 and (uc.reg_read(R.UC_X86_REG_AX) >> 8) == 0x2C, f'unexpected interrupt {name}')
        uc.reg_write(R.UC_X86_REG_CX, cx); uc.reg_write(R.UC_X86_REG_DX, dx)
    uc.hook_add(UC_HOOK_CODE, code_hook); uc.hook_add(UC_HOOK_INTR, intr)
    uc.emu_start(physical(c.cs, c.ip), 2 * SIZE, count=5000)
    check(s['stop'] is not None, f'no stop {name}')
    actual = Cpu.from_buffer_copy(c); status, ram, d = run_c(f, m, actual, time)
    check(status == s['stop'], f'status {name}: C {status} original {s["stop"]}')
    if ram != bytes(backing):
        diff = [hex(i) for i, (a, b) in enumerate(zip(ram, bytes(backing))) if a != b]
        raise ValueError(f'RAM {name}: {diff[:8]} ({len(diff)} bytes)')
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        want = uc.reg_read(reg) & 0xFFFF
        check(getattr(actual, n) == want, f'CPU {name}/{n} C {getattr(actual, n):04x} original {want:04x}')
    if status == 0:  # independent arithmetic check of the returned DX:AX
        check(((actual.dx << 16) | actual.ax) == expected_hundredths(time), f'value {name}')
    return STOPS[status], seen


def cases():
    rng = random.Random(0x08AE0000)
    out = [(f'hour{h}', dict(time=(h, (h * 7) % 60, (h * 13) % 60, (h * 17) % 100))) for h in range(24)]
    out += [(f'edge-{"-".join(map(str, t))}', dict(time=t)) for t in ((0, 0, 0, 0), (23, 59, 59, 99), (24, 60, 60, 100),
                                                                      (255, 255, 255, 255), (11, 59, 59, 99), (12, 0, 0, 0))]
    out += [(f'flags{f:x}', dict(flags=f)) for f in FLAGS]
    # The deepest frame is GetTime's: its entry SP = SP - 2 (BP) - 0Ch (locals) - 10h (four far pointers) - 4 (far call).
    # GetTime's own guard admits [0002,FFEC]; the body adds no guard of its own, so these stay inside it.
    out += [(f'sp{sp:x}', dict(sp=sp)) for sp in (0x0030, 0x0031, 0x8001, 0xFFFC)]
    out += [('ss-ds', dict(ss=DS)), ('a20-stack', dict(ss=0xFFFF, sp=0x0400))]
    for k in range(32):
        out.append((f'random{k}', dict(time=tuple(rng.randrange(256) for _ in range(4)), flags=rng.choice(FLAGS),
                                       regs=tuple(rng.randrange(65536) for _ in range(7)))))
    return out


def differential(code, decoded, f):
    seen, stops, specs = set(), {}, cases()
    for name, options in specs:
        status, v = compare(name, build(code, **options), code, decoded, f)
        seen |= v; stops[status] = stops.get(status, 0) + 1
    body = {k for k in decoded if k[0] == 'body'}
    check(seen == set(decoded), f'starts missing {sorted(set(decoded) - seen)}')
    return dict(cases=len(specs), stops=stops, body_starts=len(body & seen), body_starts_total=len(body),
                closure_starts=len(seen), closure_total=len(decoded), conditional_outcomes_total=0,
                cpu_words=14, ram_bytes=SIZE, flag_masks=0)


TOP, LOW, WINDOW = 0xF000, 0xEC00, 0x0400


def native(code, f):
    directory = ROOT / 'resources/res08ae-time/native'; directory.mkdir(parents=True, exist_ok=True)
    for kind, blob in code.items():
        (directory / f'{kind}.bin').write_bytes(blob)
    specs = [(n, o) for n, o in cases() if n.startswith(('hour', 'edge', 'flags'))]
    rows = [(o.get('flags', 2),) + service_values(o.get('time', (13, 42, 7, 55))) for _, o in specs]
    t = ['.code16', '.intel_syntax noprefix', '.global _start', '_start: jmp main', '.org 0x200', 'main:',
         'mov ax,cs', 'mov ds,ax', 'mov ss,ax', f'mov sp,{TOP}', 'cld']
    for kind in ('body', 'dos', 'sys'):
        t += [f'mov ax,{LOAD[kind]}', 'mov es,ax', f'mov di,{SPANS[kind][1]}', f'mov si,offset b_{kind}', f'mov cx,{len(code[kind])}',
              'rep movsb']
    t += [f'mov ax,{CS}', 'mov es,ax'] + [f'mov word ptr es:[{off}],{seg}' for off, seg in SEGWORDS.items()]
    t += ['xor ax,ax', 'mov es,ax', 'mov ax,word ptr es:[0x84]', 'mov word ptr [old21],ax', 'mov ax,word ptr es:[0x86]',
          'mov word ptr [old21+2],ax',
          'mov dx,offset outname', 'xor cx,cx', 'mov ah,0x3c', 'int 0x21', 'jc failure', 'mov word ptr [handle],ax',
          'mov word ptr [header],cs', 'mov word ptr [header+2],offset returned', f'mov word ptr [header+4],{len(rows)}',
          'mov bx,word ptr [handle]', 'mov dx,offset header', 'mov cx,6', 'mov ah,0x40', 'int 0x21',
          'again:', 'cli', 'cld', 'mov ax,cs', 'mov ds,ax', 'mov es,ax', 'mov al,0x5a', f'mov di,{LOW}', f'mov cx,{WINDOW - 4}', 'rep stosb',
          'mov si,word ptr cs:[index]', 'mov ax,si', 'shl si,1', 'add si,ax', 'shl si,1', 'add si,offset table',
          'mov ax,word ptr cs:[si+2]', 'mov word ptr cs:[case_cx],ax', 'mov ax,word ptr cs:[si+4]', 'mov word ptr cs:[case_dx],ax',
          'xor ax,ax', 'mov es,ax', 'mov word ptr es:[0x84],offset int21', 'mov word ptr es:[0x86],cs',
          f'mov sp,{TOP}', 'push word ptr cs:[si]', f'mov ax,{DS}', 'mov ds,ax', 'mov ax,0x1111', 'mov es,ax',
          'mov ax,0x1234', 'mov bx,0x3456', 'mov cx,0x5678', 'mov dx,0x789a', 'mov si,0x9abc', 'mov di,0xbcde', 'mov bp,0x7777',
          'popf', f'lcall {CS},0', 'returned:']
    t += [f'mov word ptr cs:[out+{2 * n}],{r}' for n, r in enumerate('ax bx cx dx si di bp sp cs'.split())]
    t += ['mov word ptr cs:[out+18],offset returned', 'mov word ptr cs:[out+20],ds', 'mov word ptr cs:[out+22],es',
          'mov word ptr cs:[out+24],ss', 'mov sp,0xf800', 'pushf', 'pop ax', 'mov word ptr cs:[out+26],ax',
          'mov ax,cs', 'mov ds,ax', 'xor ax,ax', 'mov es,ax', 'mov ax,word ptr [old21]', 'mov word ptr es:[0x84],ax',
          'mov ax,word ptr [old21+2]', 'mov word ptr es:[0x86],ax', 'sti',
          'mov bx,word ptr [handle]', 'mov dx,offset out', 'mov cx,28', 'mov ah,0x40', 'int 0x21',
          'mov bx,word ptr [handle]', f'mov dx,{LOW}', f'mov cx,{WINDOW}', 'mov ah,0x40', 'int 0x21',
          'inc word ptr [index]', f'cmp word ptr [index],{len(rows)}', 'jae done', 'jmp again', 'done:',
          'mov bx,word ptr [handle]', 'mov ah,0x3e', 'int 0x21', 'mov ax,0x4c00', 'int 0x21', 'failure:', 'mov ax,0x4c01', 'int 0x21',
          'int21:', 'cmp ah,0x2c', 'jne chain', 'mov cx,word ptr cs:[case_cx]', 'mov dx,word ptr cs:[case_dx]', 'iret',
          'chain:', 'jmp dword ptr cs:[old21]',
          'outname: .asciz "RESULT.BIN"', 'handle: .word 0', 'index: .word 0', 'header: .space 6', 'out: .space 28',
          'old21: .word 0,0', 'case_cx: .word 0', 'case_dx: .word 0', 'table:']
    t += ['.word ' + ','.join(str(v) for v in row) for row in rows]
    t += [f'b_{kind}: .incbin "{directory / (kind + ".bin")}"' for kind in ('body', 'dos', 'sys')]
    (directory / 'oracle.S').write_text('\n'.join(t) + '\n')
    subprocess.run(['as', '--32', str(directory / 'oracle.S'), '-o', str(directory / 'oracle.o')], check=True)
    com = directory / 'ORACLE.COM'
    subprocess.run(['ld', '-m', 'elf_i386', '-Ttext', '0x100', '--oformat', 'binary', str(directory / 'oracle.o'), '-o', str(com)], check=True)
    check(com.stat().st_size + 0x100 < LOW, 'COM reaches the stack window')
    (directory / 'dosbox.conf').write_text('[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n[cpu]\ncore=normal\ncputype=386\ncycles=max\n'
                                          '[mixer]\nnosound=true\n[midi]\nmididevice=none\n[autoexec]\nmount c '
                                          + str(directory.resolve()) + '\nc:\nORACLE.COM\nexit\n')
    out = directory / 'RESULT.BIN'
    if out.exists():
        out.unlink()
    binary = '/usr/bin/dosbox-x'
    with (directory / 'dosbox.log').open('w') as log:
        subprocess.run([binary, '-conf', str(directory / 'dosbox.conf'), '-fastlaunch', '-exit', '-noconsole'],
                       env=dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy'), stdout=log, stderr=subprocess.STDOUT,
                       check=True, timeout=300)
    data = out.read_bytes(); record = 28 + WINDOW
    check(len(data) == 6 + len(rows) * record, f'native length {len(data)}')
    seg, ret_ip, count = struct.unpack_from('<3H', data); check(count == len(rows), 'native count')
    # GetTime's INT happens at its SP: entry SP - 2 - 0Ch - 10h - 4 - 2 (its own PUSH BP).
    int_sp = (TOP - 4) - 2 - 0x0C - 0x10 - 4 - 2
    for n, ((name, o), (flags, cx, dx)) in enumerate(zip(specs, rows)):
        rec = data[6 + n * record:6 + (n + 1) * record]
        m, c, time = build(code, time=o.get('time', (13, 42, 7, 55)), ss=seg, sp=TOP - 4, flags=flags, background=0)
        m[physical(seg, LOW):physical(seg, LOW) + WINDOW - 4] = b'\x5a' * (WINDOW - 4)
        put16(m, seg, TOP - 4, ret_ip); put16(m, seg, TOP - 2, seg)
        status, ram, d = run_c(f, m, c, time)
        check(status == 0, f'native C status {name}')
        for (reg, _), want in zip(Cpu._fields_, struct.unpack_from('<14H', rec)):
            check(getattr(c, reg) == want, f'native CPU {name}/{reg}: C {getattr(c, reg):04x} DOS {want:04x}')
        lo = int_sp - LOW
        mine = ram[physical(seg, LOW):physical(seg, LOW) + WINDOW]
        check(mine[lo:] == rec[28 + lo:28 + WINDOW], f'native stack {name}')
    probe = subprocess.run([binary, '--version'], capture_output=True, text=True, env=dict(os.environ, SDL_VIDEODRIVER='dummy'))
    version = next(line.split(',')[0].strip() for line in (probe.stdout + probe.stderr).splitlines() if 'DOSBox-X version' in line)
    return dict(cases=len(rows), cpu_words=14, stack_bytes_compared=f'{TOP - int_sp} (from GetTime INT-time SP to F000)',
                flag_masks=0, declared_stub='INT 21h AH=2Ch -> case CX/DX, IRET', result_sha256=sha256(data).hexdigest(),
                dosbox_x=dict(path=binary, sha256=sha256(open(binary, 'rb').read()).hexdigest(), version=version,
                              pinned_source='5a9c7c53481c2e933948039c33256776653f4faf', pinned_build_used=False))


def main():
    p = argparse.ArgumentParser()
    for flag in ('--compile-only', '--unicorn', '--native'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--output', type=Path); a = p.parse_args()
    with tempfile.TemporaryDirectory(prefix='time08ae-') as tmp:
        f = library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        code, decoded, meta = source(); report = dict(target=meta)
        if a.unicorn:
            report['unicorn'] = differential(code, decoded, f)
        if a.native:
            report['native'] = native(code, f)
    if a.output:
        a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    main()
