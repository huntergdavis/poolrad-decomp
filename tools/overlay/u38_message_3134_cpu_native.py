"""Synthetic DOSBox-X EGA corroboration of original U38 +3134 and its closure.

Original 3134, 0B0B and the 1693 closure are loaded at rebound segments and run
unchanged by the DOS CPU; every case is compared on CPU14, a stack window, the
full DS/font/string segments and all four EGA planes. Stop cases replace the
reached instruction with a far JMP back to the harness (declared trap, flags
neutral); no successful callee is stubbed. Pointers A and B point into the DS
window so their effect is captured. The native snapshot is partial: backing RAM
outside the captured windows is masked and listed in the report.
"""
import os, struct, subprocess
from hashlib import sha256
from u38_message_3134_cpu_audit import (ROOT, SIZE, Cpu, FLAGS, physical, put16, run_c, check, build, base,
    Limits, stop_name, OVERLAY, ENTRY, LITERAL, SEGMENTS, KEYS, FIELDS)
CS, DS, SYS, FONT, STRING, DISPLAY, CLEAR, DELAY = base.CS, base.DS, base.SYS, base.FONT, base.STRING, base.DISPLAY, base.CLEAR, base.DELAY
TOP, LOW, WINDOW, OUTPUT = 0xF000, 0xEC00, 1024, 0xC000
RECORD = 28 + WINDOW + 3 * 65536 + 4 * 65536
REGS = (0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777)
A_PTR, B_PTR = (DS, 0x7000), (DS, 0x7100)


def specifications():
    result = [dict(), dict(gate=1), dict(b_byte=0x13), dict(b_byte=0xFF), dict(gate=0, gate_hi=0xFF), dict(b_byte=0)]
    result += [dict(flags=f) for f in FLAGS]
    result += [dict(flags=f, gate=1) for f in FLAGS[:3]]
    result += [dict(factor=f) for f in (0, 1)]
    result += [dict(active=a) for a in (0, 2, 7)]
    # Literal-length variants are Unicorn-only: the native loader restores the pristine literal each case.
    result += [dict(a=(OVERLAY, (LITERAL - 0x34) & 65535)), dict(a=(OVERLAY, (LITERAL + 1 - 0x34) & 65535)),
               dict(a=(DS, 0xFFE0)), dict(a=(0xA000, 0x0100)), dict(b=(0xA000, 0x0100), b_byte=0x13)]
    result += [dict(mode=3), dict(mode=255), dict(pages=(0xA000, 0xA000)), dict(writes=((DS, 0x5E20, bytes(4)),)),
               dict(fontseg=0xA000), dict(fontoff=0xFFFC)]
    result += [dict(stop=(OVERLAY, 0x316A)), dict(stop=(OVERLAY, 0x316F))]
    result += [dict(stop=(DISPLAY, site)) for site in base.ROOT_CALLS]
    result += [dict(stop=(CLEAR, 0x1F1)), dict(stop=(CLEAR, 0x20C)), dict(stop=(CS, 0x63C)), dict(stop=(CS, 0x207)), dict(stop=(CS, 0xDCC))]
    result += [dict(mode=m, stop=(DISPLAY, 0x16B8)) for m in (0, 2)]
    for n, c in enumerate(result):
        c.setdefault('flags', FLAGS[n % len(FLAGS)])
    return result


def fixture(case, bodies, literal, segment=0x0814, returned=0, bios=0x55):
    options = dict(case)
    stop = options.pop('stop', None)
    flags = options.pop('flags')
    writes = list(options.pop('writes', ()))
    if stop:
        writes.append((*stop, b'\x90'))
    options.setdefault('a', A_PTR)
    options.setdefault('b', B_PTR)
    m, c, spec = build(bodies, literal, ss=segment, sp=TOP - 14, bios=bios, writes=writes, flags=flags,
                       ret=(returned, segment), **options)
    c.es = segment
    return m, c, spec, options


def native(bodies, literal, lib):
    directory = ROOT / 'resources/u38-3134/native'
    directory.mkdir(parents=True, exist_ok=True)
    specs = specifications()
    payload = bytearray()
    for case in specs:
        m, c, spec, options = fixture(case, bodies, literal)
        seg, off = case.get('stop', (0, 0))
        gate = (options.get('gate_hi', 0xA5) << 8) | options.get('gate', 0)
        a, b = options['a'], options['b']
        payload += struct.pack('<8H', case['flags'], off, seg, gate, a[1], a[0], b[1], b[0])
        payload += b''.join(m[physical(s, 0):physical(s, 0) + 65536] for s in (DS, FONT, STRING))
    (directory / 'INPUT.BIN').write_bytes(payload)
    memory, _, _, _ = fixture(dict(flags=2), bodies, literal)
    blobs = []
    for n, ((kind, start), body) in enumerate(bodies.items()):
        seg = SEGMENTS[kind]
        at = physical(seg, start)
        path = directory / f'body{n}.bin'
        path.write_bytes(memory[at:at + len(body)])
        blobs.append((f'body{n}', seg, start, len(body), path))
    at = physical(OVERLAY, LITERAL)
    path = directory / 'literal.bin'
    path.write_bytes(memory[at:at + 25])
    blobs.append(('literal', OVERLAY, LITERAL, 25, path))
    t = ['.code16', '.intel_syntax noprefix', '.global _start', '_start: jmp main', '.org 0x200', 'main:',
         'mov ax,cs', 'mov ds,ax', 'mov ss,ax', f'mov sp,{TOP}', 'mov ax,0x000d', 'int 0x10', 'cld',
         'mov dx,offset inputname', 'mov ax,0x3d00', 'int 0x21', 'jc failure', 'mov word ptr [inputhandle],ax',
         'mov dx,offset outputname', 'xor cx,cx', 'mov ah,0x3c', 'int 0x21', 'jc failure', 'mov word ptr [outputhandle],ax',
         'mov word ptr [0xd800],cs', 'mov word ptr [0xd802],offset returned', f'mov word ptr [0xd804],{len(specs)}',
         'mov dx,0xd800', 'mov cx,6', 'call write_output', 'again:',
         'push cs', 'pop ds', 'mov dx,offset spec', 'mov cx,16', 'call read_input']
    for seg in (DS, FONT, STRING):
        for off in (0, 0x8000):
            t += [f'mov ax,{seg}', 'mov ds,ax', f'mov dx,{off}', 'mov cx,0x8000', 'call read_input']
    t += ['push cs', 'pop ds', 'cld']
    for label, seg, off, size, path in blobs:
        t += [f'mov ax,{seg}', 'mov es,ax', f'mov di,{off}', f'mov si,offset {label}', f'mov cx,{size}', 'rep movsb']
    t += ['mov di,word ptr cs:[spec+2]', 'test di,di', 'jz trap_ready', 'mov ax,word ptr cs:[spec+4]', 'mov es,ax',
          'mov byte ptr es:[di],0xea', 'mov word ptr es:[di+1],offset returned', 'mov word ptr es:[di+3],cs', 'trap_ready:',
          'cli', 'mov dx,0x3ce', 'mov ax,0x0000', 'out dx,ax', 'mov ax,0x0001', 'out dx,ax',
          'mov ax,0x0003', 'out dx,ax', 'mov ax,0x0004', 'out dx,ax', 'mov ax,0x0005', 'out dx,ax', 'mov ax,0xff08', 'out dx,ax',
          'mov ax,0xa000', 'mov es,ax']
    for p in range(4):
        t += ['mov dx,0x3c4', f'mov ax,{((1 << p) << 8) | 2}', 'out dx,ax', 'xor di,di', f'mov ax,{(0x31 + p * 0x27) * 257}', 'mov cx,0x8000', 'rep stosw']
    t += ['mov dx,0x3c4', 'mov ax,0x0f02', 'out dx,ax']  # reset-state map mask 0Fh after the per-plane fill
    t += ['push cs', 'pop es', 'cld', f'mov sp,{TOP}', 'mov al,0xa5', f'mov di,{LOW}', f'mov cx,{WINDOW}', 'rep stosb',
          'mov ax,0x40', 'mov ds,ax', 'mov al,byte ptr [0]', 'mov byte ptr cs:[bios],al',
          'push word ptr cs:[spec+14]', 'push word ptr cs:[spec+12]', 'push word ptr cs:[spec+10]',
          'push word ptr cs:[spec+8]', 'push word ptr cs:[spec+6]', 'push word ptr cs:[spec]', f'mov ax,{DS}', 'mov ds,ax']
    for r, v in zip('ax bx cx dx si di bp'.split(), REGS):
        t.append(f'mov {r},{v}')
    t += ['popf', 'call dword ptr cs:[target]', 'returned:']
    for n, r in enumerate('ax bx cx dx si di bp sp cs'.split()):
        t.append(f'mov word ptr cs:[{OUTPUT + 2 * n}],{r}')
    t += [f'mov word ptr cs:[{OUTPUT + 18}],offset returned']
    for off, r in ((20, 'ds'), (22, 'es'), (24, 'ss')):
        t.append(f'mov word ptr cs:[{OUTPUT + off}],{r}')
    t += [f'mov sp,{TOP + 0x800}', 'pushf', 'pop ax', f'mov word ptr cs:[{OUTPUT + 26}],ax',
          'mov ax,word ptr cs:[spec+2]', 'test ax,ax', 'jz pc_ready', f'mov word ptr cs:[{OUTPUT + 18}],ax',
          'mov ax,word ptr cs:[spec+4]', f'mov word ptr cs:[{OUTPUT + 16}],ax', 'pc_ready:', 'cld', 'push cs', 'pop ds', 'push cs', 'pop es',
          f'mov si,{LOW}', f'mov di,{OUTPUT + 28}', f'mov cx,{WINDOW}', 'rep movsb', 'sti',
          'mov dx,offset bios', 'mov cx,1', 'call write_output', f'mov dx,{OUTPUT}', f'mov cx,{28 + WINDOW}', 'call write_output']
    for seg in (DS, FONT, STRING):
        for off in (0, 0x8000):
            t += [f'mov ax,{seg}', 'mov ds,ax', f'mov dx,{off}', 'mov cx,0x8000', 'call write_output']
    for p in range(4):
        t += ['mov dx,0x3ce', 'mov ax,0x0005', 'out dx,ax', f'mov ax,{(p << 8) | 4}', 'out dx,ax']
        for off in (0, 0x8000):
            t += ['mov ax,0xa000', 'mov ds,ax', 'mov ax,0x7000', 'mov es,ax', f'mov si,{off}', 'xor di,di', 'mov cx,0x8000', 'cld', 'rep movsb',
                  'mov ax,0x7000', 'mov ds,ax', 'xor dx,dx', 'mov cx,0x8000', 'call write_output']
    t += ['push cs', 'pop ds', 'inc word ptr [index]', f'cmp word ptr [index],{len(specs)}', 'jb again',
          'mov bx,word ptr [outputhandle]', 'mov ah,0x3e', 'int 0x21', 'mov ax,3', 'int 0x10', 'mov ax,0x4c00', 'int 0x21',
          'write_output:', 'mov bx,word ptr cs:[outputhandle]', 'mov ah,0x40', 'int 0x21', 'jc failure', 'cmp ax,cx', 'jne failure', 'ret',
          'read_input:', 'mov bx,word ptr cs:[inputhandle]', 'mov ah,0x3f', 'int 0x21', 'jc failure', 'cmp ax,cx', 'jne failure', 'ret',
          'failure:', 'mov ax,0x4c01', 'int 0x21',
          'inputname: .asciz "INPUT.BIN"', 'outputname: .asciz "RESULT.BIN"',
          'inputhandle: .word 0', 'outputhandle: .word 0', 'index: .word 0', 'spec: .space 16', 'bios: .byte 0',
          f'target: .word {ENTRY},{OVERLAY}', '.org 0x3000']
    for label, seg, off, size, path in blobs:
        t += [label + ':', f'.incbin "{path}"']
    asm, obj = directory / 'oracle.S', directory / 'oracle.o'
    asm.write_text('\n'.join(t) + '\n')
    subprocess.run(['as', '--32', str(asm), '-o', str(obj)], check=True)
    com = directory / 'ORACLE.COM'
    subprocess.run(['ld', '-m', 'elf_i386', '-Ttext', '0x100', '--oformat', 'binary', str(obj), '-o', str(com)], check=True)
    check(com.stat().st_size + 0x100 < OUTPUT, 'COM/output overlap')
    cfg = directory / 'dosbox.conf'
    cfg.write_text('[sdl]\noutput=surface\n[dosbox]\nmachine=ega\nmemsize=16\n[cpu]\ncore=normal\ncputype=386\ncycles=max\n'
                   '[mixer]\nnosound=true\n[midi]\nmididevice=none\n[autoexec]\nmount c ' + str(directory.resolve()) + '\nc:\nORACLE.COM\nexit\n')
    output = directory / 'RESULT.BIN'
    if output.exists():
        output.unlink()
    with (directory / 'dosbox.log').open('w') as log:
        subprocess.run(['dosbox-x', '-conf', str(cfg), '-fastlaunch', '-exit', '-noconsole'],
                       env=dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy'),
                       stdout=log, stderr=subprocess.STDOUT, check=True, timeout=600)
    result = output.read_bytes()
    check(len(result) == 6 + len(specs) * (RECORD + 1), f'native output length {len(result)}')
    segment, returned, count = struct.unpack_from('<3H', result)
    check(count == len(specs), 'native header count')
    limits = Limits(65536, 65536, 65536, 65536, 1000000)
    stops = {}
    for n, case in enumerate(specs):
        at = 6 + n * (RECORD + 1)
        bios, record = result[at], result[at + 1:at + 1 + RECORD]
        m, c, spec, options = fixture(case, bodies, literal, segment, returned, bios)
        status, actual, ram, events, writes, ega, d = run_c(lib, (m, c, spec), limits)
        stop = case.get('stop', (None, None))[0]
        expected = 2 if stop == OVERLAY else 3 if stop else 0
        check(status == expected, f'native status {n}: {status}/{expected}')
        for (reg, _), want in zip(Cpu._fields_, struct.unpack_from('<14H', record)):
            check(getattr(actual, reg) == want, f'native CPU {n}/{reg}: C {getattr(actual, reg):04X}, DOS {want:04X}')
        mine = (ram[physical(segment, LOW):physical(segment, LOW) + WINDOW] +
                b''.join(ram[physical(s, 0):physical(s, 0) + 65536] for s in (DS, FONT, STRING)) + ega[:4 * 65536])
        if mine != record[28:]:
            diffs = [(hex(i), a, b) for i, (a, b) in enumerate(zip(mine, record[28:])) if a != b]
            raise ValueError(f'native memory {n}: {diffs[:8]} ({len(diffs)} bytes)')
        label = f'{status}/' + ('RETURN' if status == 0 else f'stop@{actual.cs:04X}:{actual.ip:04X}')
        stops[label] = stops.get(label, 0) + 1
    captured = sorted([(physical(segment, LOW), physical(segment, LOW) + WINDOW)] + [(physical(s, 0), physical(s, 0) + 65536) for s in (DS, FONT, STRING)])
    masked, at = [], 0
    for lo, hi in captured:
        if at < lo:
            masked.append((at, lo))
        at = hi
    if at < SIZE:
        masked.append((at, SIZE))
    ranges = lambda spans: [dict(start=f'{lo:05X}', end_exclusive=f'{hi:05X}', bytes=hi - lo) for lo, hi in spans]
    return dict(cases=len(specs), stops=stops, native_captured_ranges=ranges(captured), native_masked_ranges=ranges(masked),
                native_masked_bytes=sum(hi - lo for lo, hi in masked), cpu_words=14, ram_bytes=WINDOW + 3 * 65536,
                ram_comparison='partial; all other backing RAM is masked in this native snapshot',
                stack_bytes=WINDOW, ds_bytes=65536, font_bytes=65536, string_bytes=65536, plane_bytes=4 * 65536, flag_masks=0,
                controller_state='not captured natively; independently compared in Python EGA differential',
                machine='ega', video_mode='0Dh', core='normal', cpu='386', result_sha256=sha256(result).hexdigest(),
                com_segment=f'{segment:04X}', synthetic_callee_stubs=0,
                capture='far JMP replaces the reached instruction before CALL/OUT; snapshot reports that original boundary')
