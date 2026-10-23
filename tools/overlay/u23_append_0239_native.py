"""Synthetic DOSBox-X EGA corroboration of original U23 +0239 and its closure, with ONE declared substitution.

Original 0239, StrLoad 0B0B, GetMem, Move, the 0BB8 closure and the 1693 closure are loaded at rebound segments and run unchanged
by the DOS CPU (machine=ega, mode 0Dh); every case is compared on CPU14, a stack window, the DS, font, character/list/item/result
and heap windows, and all four EGA planes. DECLARED SUBSTITUTION: the 275B capacity check cannot run natively (its model hard-codes
resident segment 00BA, which is DOS's own memory), so U77:275B is a five-instruction stub on the DOS side and the same stub
(u23_append_0239_native_stubs.c) replaces the model on the C side; its answer is the ITEM byte at +3Ah. 275B itself is covered
by the Unicorn differential only. The native snapshot is partial: RAM outside the captured windows is masked and listed.
"""
import ctypes, os, struct, subprocess
from hashlib import sha256
import u23_append_0239_cpu_audit as A
from u23_append_0239_cpu_audit import (ROOT, SIZE, Cpu, FLAGS, physical, check, build, base, Limits, Diag, Bus, Ega, OUT_FN, IN_FN, WRITE_FN, Env,
                                       OVERLAY, STUB, U77, ENTRY, LITERAL, SEGMENTS, SITES, DS, SYS, DISPLAY)
FONT, STACK_WINDOW = base.FONT, 1024
TOP, LOW, OUTPUT = 0xF000, 0xEC00, 0xC000
RES_N = 0x4800                                         # the 0BB8 closure's native segment (00BA is DOS memory)
REGS = (0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777)
WINDOWS = ((DS, 0, 0x10000), (FONT, 0, 0x10000), (A.CHAR_SEG, 0, 0x7000), (A.HEAP_SEG, 0, 0x4000))
RECORD = 28 + STACK_WINDOW + sum(w[2] for w in WINDOWS) + 4 * 65536
STUB_CODE = bytes.fromhex('89e3' '36c47f04' '268a453a' 'ca0800')      # mov bx,sp; les di,ss:[bx+4]; mov al,es:[di+3Ah]; retf 8


def N(n):
    return tuple(dict(weight=1 + (i * 5) % 9, quantity=1 + i % 3) for i in range(n))


def specifications():
    out = [dict(nodes=N(n), over=0) for n in (0, 1, 2, 3, 5, 15)]
    out += [dict(nodes=N(2), over=0, heap_mode=m) for m in ('exact', 'split', 'skip')]
    out += [dict(nodes=N(16), over=1, mode=m) for m in (0, 1, 2, 3, 255)]
    out += [dict(nodes=N(16), over=1, active=a) for a in (0, 2, 7)]
    out += [dict(nodes=N(16), over=1, factor=f) for f in (0, 1)]
    out += [dict(nodes=N(16), over=1, pages=(0xA000, 0xA000)), dict(nodes=N(16), over=1, writes=((DS, 0x5E20, bytes(4)),))]
    out += [dict(nodes=N(n), over=o, result=(0xA000, 0x0100)) for n, o in ((2, 0), (16, 1))]
    # Unicorn-only (not here): SP 0201h, because the oracle always enters with SP = TOP in its own segment; and GetMem failure,
    # because SYSTEM's RunError (0AF8:00E2) runs Halt's exit chain, which a bare oracle cannot survive (it hung the first run).
    out += [dict(nodes=N(2), over=0, result=(A.RESULT_SEG, 0xFFFF))]
    out += [dict(nodes=N(2), over=0, item=dict(weight=0xFFFF, quantity=255))]
    # each trapped call site must be on the case's path: 029B (head GetMem) needs an empty list, 0300 (tail GetMem) a non-empty
    # one, 0260 (StrLoad) the over-capacity path
    out += [dict(nodes=N(0 if s == 0x29B else 2), over=0, stop=(OVERLAY, s)) for s in (0x24D, 0x29B, 0x300, 0x344)]
    out += [dict(nodes=N(16), over=1, stop=(OVERLAY, 0x260))]
    # IF stays clear natively: the oracle's POPF would otherwise enable interrupts inside the body and a timer IRQ frame
    # (F000:FEAF) lands in its locals (the first native run showed exactly that in case 6). IF-set entry flags are Unicorn-only.
    for n, c in enumerate(out):
        c.setdefault('flags', (tuple(FLAGS) + (0x0202, 0x0602, 0x0A87, 0x0402, 0x0E57))[n % 11] & ~0x0200)
    return out


def fixture(case, bodies, literal, segment=0x0814, returned=0, bios=0x55):
    o = dict(case)
    stop, over = o.pop('stop', None), o.pop('over')
    writes = list(o.pop('writes', ())) + [(A.ITEM_SEG, 0x200 + 0x3A, bytes([over]))]
    if stop:
        writes.append((*stop, b'\x90'))
    o.setdefault('sp', TOP - 12)
    sp = o.pop('sp')
    m, c, spec = build(bodies, literal, ss=segment, sp=sp, bios=bios, writes=writes, ret=(returned, segment), **o)
    return m, c, spec, o, stop


def native(bodies, literal, lib, directory=None):
    directory = directory or ROOT / 'resources/u23-0239/native'
    directory.mkdir(parents=True, exist_ok=True)
    saved = (dict(A.SEGMENTS), dict(A.SEG_WORDS))
    A.SEGMENTS['res'] = RES_N
    A.SEG_WORDS[0x347] = RES_N
    try:
        specs = specifications()
        payload = bytearray()
        for case in specs:
            m, c, spec, o, stop = fixture(case, bodies, literal)
            seg, off = stop or (0, 0)
            result = spec['result']
            payload += struct.pack('<5H', case['flags'], off, seg, result[1], result[0])
            payload += b''.join(m[physical(s, o_):physical(s, o_) + size] for s, o_, size in WINDOWS)
        (directory / 'INPUT.BIN').write_bytes(payload)
        memory = fixture(dict(flags=2, over=0, nodes=()), bodies, literal)[0]
        blobs = []
        for n, ((kind, start), body) in enumerate(bodies.items()):
            if kind == 'u77':
                continue
            seg = A.SEGMENTS[kind]
            at = physical(seg, start)
            path = directory / f'body{n}.bin'
            path.write_bytes(memory[at:at + len(body)])
            blobs.append((f'body{n}', seg, start, len(body), path))
        extras = (('literal', OVERLAY, LITERAL, bytes(memory[physical(OVERLAY, LITERAL):physical(OVERLAY, LITERAL) + 11])),
                  ('stubjmp', STUB, 0x4D, bytes(memory[physical(STUB, 0x4D):physical(STUB, 0x4D) + 5])),
                  ('u77stub', U77, 0x275B, STUB_CODE))
        for label, seg, off, data in extras:
            path = directory / f'{label}.bin'
            path.write_bytes(data)
            blobs.append((label, seg, off, len(data), path))
        words = dict(A.SEG_WORDS)
    finally:
        A.SEGMENTS.clear(); A.SEGMENTS.update(saved[0]); A.SEG_WORDS.clear(); A.SEG_WORDS.update(saved[1])
    t = ['.code16', '.intel_syntax noprefix', '.global _start', '_start: jmp main', '.org 0x200', 'main:',
         'mov ax,cs', 'mov ds,ax', 'mov ss,ax', f'mov sp,{TOP}', 'mov ax,0x000d', 'int 0x10', 'cld',
         'mov dx,offset inputname', 'mov ax,0x3d00', 'int 0x21', 'jc failure', 'mov word ptr [inputhandle],ax',
         'mov dx,offset outputname', 'xor cx,cx', 'mov ah,0x3c', 'int 0x21', 'jc failure', 'mov word ptr [outputhandle],ax',
         'mov word ptr [0xd800],cs', 'mov word ptr [0xd802],offset returned', f'mov word ptr [0xd804],{len(specs)}',
         'mov dx,0xd800', 'mov cx,6', 'call write_output', 'again:',
         'push cs', 'pop ds', 'mov dx,offset spec', 'mov cx,10', 'call read_input']
    for seg, off, size in WINDOWS:
        for part in range(0, size, 0x8000):
            n = min(0x8000, size - part)
            t += [f'mov ax,{seg + (part >> 4)}', 'mov ds,ax', 'xor dx,dx', f'mov cx,{n}', 'call read_input']
    t += ['push cs', 'pop ds', 'cld']
    for label, seg, off, size, path in blobs:
        t += [f'mov ax,{seg}', 'mov es,ax', f'mov di,{off}', f'mov si,offset {label}', f'mov cx,{size}', 'rep movsb']
    t += [f'mov ax,{OVERLAY}', 'mov es,ax']
    for off, seg in sorted(words.items()):
        t.append(f'mov word ptr es:[{off}],{RES_N if off == 0x347 else seg}')
    t += ['mov di,word ptr cs:[spec+2]', 'test di,di', 'jz trap_ready', 'mov ax,word ptr cs:[spec+4]', 'mov es,ax',
          'mov byte ptr es:[di],0xea', 'mov word ptr es:[di+1],offset returned', 'mov word ptr es:[di+3],cs', 'trap_ready:',
          'cli', 'mov dx,0x3ce', 'mov ax,0x0000', 'out dx,ax', 'mov ax,0x0001', 'out dx,ax',
          'mov ax,0x0003', 'out dx,ax', 'mov ax,0x0004', 'out dx,ax', 'mov ax,0x0005', 'out dx,ax', 'mov ax,0xff08', 'out dx,ax',
          'mov ax,0xa000', 'mov es,ax']
    for p in range(4):
        t += ['mov dx,0x3c4', f'mov ax,{((1 << p) << 8) | 2}', 'out dx,ax', 'xor di,di', f'mov ax,{(0x31 + p * 0x27) * 257}', 'mov cx,0x8000', 'rep stosw']
    t += ['mov dx,0x3c4', 'mov ax,0x0f02', 'out dx,ax']
    t += ['push cs', 'pop es', 'cld', f'mov sp,{TOP}', 'mov al,0xa5', f'mov di,{LOW}', f'mov cx,{STACK_WINDOW}', 'rep stosb',
          'mov ax,0x40', 'mov ds,ax', 'mov al,byte ptr [0]', 'mov byte ptr cs:[bios],al',
          f'push {A.ITEM_SEG}', 'push 0x0200', 'push word ptr cs:[spec+8]', 'push word ptr cs:[spec+6]', 'push word ptr cs:[spec]',
          f'mov ax,{DS}', 'mov ds,ax', 'mov ax,0x1111', 'mov es,ax']        # ES as the fixture has it (0239 reads it only after an LES)
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
          f'mov si,{LOW}', f'mov di,{OUTPUT + 28}', f'mov cx,{STACK_WINDOW}', 'rep movsb', 'sti',
          'mov dx,offset bios', 'mov cx,1', 'call write_output', f'mov dx,{OUTPUT}', f'mov cx,{28 + STACK_WINDOW}', 'call write_output']
    for seg, off, size in WINDOWS:
        for part in range(0, size, 0x8000):
            n = min(0x8000, size - part)
            t += [f'mov ax,{seg + (part >> 4)}', 'mov ds,ax', 'xor dx,dx', f'mov cx,{n}', 'call write_output']
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
                       stdout=log, stderr=subprocess.STDOUT, check=True, timeout=900)
    result = output.read_bytes()
    check(len(result) == 6 + len(specs) * (RECORD + 1), f'native output length {len(result)} (expected {6 + len(specs) * (RECORD + 1)})')
    segment, returned, count = struct.unpack_from('<3H', result)
    check(count == len(specs), 'native header count')
    limits = Limits(65536, 65536, 65536, 65536, 1000000, 1 << 20, 64, 1000)
    stops, writes_seen = {}, 0
    window_total = sum(w[2] for w in WINDOWS)
    saved = (dict(A.SEGMENTS), dict(A.SEG_WORDS))
    A.SEGMENTS['res'] = RES_N
    A.SEG_WORDS[0x347] = RES_N
    try:
        for n, case in enumerate(specs):
            at = 6 + n * (RECORD + 1)
            bios, record = result[at], result[at + 1:at + 1 + RECORD]
            m, c, spec, o, stop = fixture(case, bodies, literal, segment, returned, bios)
            status, actual, ram, events, writes, ega, d = A.run_c(lib, (m, c, spec), limits)
            expected = 0 if not stop else 2
            check(status == expected, f'native status {n}: {status}/{expected}')
            for (reg, _), want in zip(Cpu._fields_, struct.unpack_from('<14H', record)):
                check(getattr(actual, reg) == want, f'native CPU {n}/{reg}: C {getattr(actual, reg):04X}, DOS {want:04X}')
            mine = ram[physical(segment, LOW):physical(segment, LOW) + STACK_WINDOW] + b''.join(
                ram[physical(s, off):physical(s, off) + size] for s, off, size in WINDOWS) + ega[:4 * 65536]
            if mine != record[28:]:
                diffs = [(hex(i), a, b) for i, (a, b) in enumerate(zip(mine, record[28:])) if a != b]
                raise ValueError(f'native memory {n}: {diffs[:8]} ({len(diffs)} bytes)')
            writes_seen += len(writes)
            label = f'{A.STATUS[status]}' + ('' if status == 0 else f'@{actual.ip:04X}')
            stops[label] = stops.get(label, 0) + 1
    finally:
        A.SEGMENTS.clear(); A.SEGMENTS.update(saved[0]); A.SEG_WORDS.clear(); A.SEG_WORDS.update(saved[1])
    captured = sorted([(physical(segment, LOW), physical(segment, LOW) + STACK_WINDOW)] + [(physical(s, off), physical(s, off) + size) for s, off, size in WINDOWS])
    masked, at = [], 0
    for lo, hi in captured:
        if at < lo:
            masked.append((at, lo))
        at = max(at, hi)
    if at < SIZE:
        masked.append((at, SIZE))
    ranges = lambda spans: [dict(start=f'{lo:05X}', end_exclusive=f'{hi:05X}', bytes=hi - lo) for lo, hi in spans]
    return dict(cases=len(specs), stops=stops, native_captured_ranges=ranges(captured), native_masked_ranges=ranges(masked),
                native_masked_bytes=sum(hi - lo for lo, hi in masked), cpu_words=14, captured_bytes_per_case=STACK_WINDOW + window_total + 4 * 65536,
                flag_masks=0, machine='ega', video_mode='0Dh', core='normal', cpu='386', result_sha256=sha256(result).hexdigest(),
                com_segment=f'{segment:04X}', declared_substitution='U77:275B replaced by a 5-instruction stub on both sides',
                vram_write_operands_c_side=writes_seen,
                capture='far JMP replaces the reached call before it executes; the snapshot reports that boundary')
