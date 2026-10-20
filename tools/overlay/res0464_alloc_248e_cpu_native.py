"""DOSBox-X corroboration for resident 0464:248E (synthetic frames; declared setup in RES_0464_ALLOC_248E_CPU.md).

A COM program loads per-case DS and heap segments, places the original body at 8800:248E and the original SYSTEM
GetMem spans and FillChar at 6000 (the six far-call segment words patched to 6000h, as in the Unicorn fixture),
pushes the six argument words and calls far, with interrupts off from the stack-window fill until the record is
taken. It records CPU14, a 1 KiB stack window and both 64 KiB segments. Only RETURN cases run natively: RunError
entry, a custom HeapError and the walk cap are Unicorn-only stops. Nothing in the body or SYSTEM code is stubbed."""
import ctypes, os, struct, subprocess
from hashlib import sha256
from res0464_alloc_248e_cpu_audit import (ROOT, SIZE, CS, SYS, DS, HEAP, ENTRY, SEGWORDS, Cpu, Ega, Bus, Env, OUT_FN, IN_FN,
                                         WRITE_FN, Diagnostic, build, check, physical, put16)
TOP, WINDOW, LOW = 0xF000, 1024, 0xEC00
RECORD = 28 + WINDOW + 2 * 65536
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)


def specifications():
    out = []
    for f in FLAGS:
        for mode in (0, 1, 2):
            for bit in (0, 1):
                out.append(dict(flags=f, mode=mode, bit=bit))
    out += [dict(flags=2, mode=0, bit=1, heap_mode=h) for h in ('exact', 'split', 'split-carry', 'skip')]
    out += [dict(flags=2, mode=0, bit=0, w0=1, w1=1, mult=5, blocks=[(0x10, 0, 2, 8)]),
            dict(flags=2, mode=0, bit=0, w0=1, w1=1, mult=3, blocks=[(0x10, 0, 2, 8)]),
            dict(flags=2, mode=0, bit=0, end=(9, 0)), dict(flags=2, mode=0, bit=1, mult=0),
            dict(flags=2, mode=1, bit=1, w0=3, w1=7, mult=5), dict(flags=2, mode=0, bit=1, pvar=(DS, 0x4404))]
    return out


def fixture(case, code, segment, returned):
    m, c = build(code, ss=segment, sp=TOP - 0x10, **case)
    put16(m, segment, TOP - 0x10, returned); put16(m, segment, TOP - 0x0E, segment)
    return m, c


def native(code, lib):
    directory = ROOT / 'resources/alloc248e/native'; directory.mkdir(parents=True, exist_ok=True)
    specs = specifications()
    data = bytearray()
    for case in specs:
        m, c = fixture(case, code, 0x0814, 0)
        pvar = case.get('pvar', (DS, 0x9000))
        data += struct.pack('<8H', case['flags'], pvar[1], pvar[0], case.get('bit', 1), case.get('mult', 3), case.get('w1', 5),
                            case.get('w0', 4), 0)
        for seg in (DS, HEAP):
            data += m[physical(seg, 0):physical(seg, 0) + 65536]
    (directory / 'INPUT.BIN').write_bytes(data)
    (directory / 'body.bin').write_bytes(bytes(code['body']))
    blobs = []
    for n, (a, blob) in enumerate(sorted(code['sys'].items())):
        (directory / f'sys{n}.bin').write_bytes(blob); blobs.append((f'sys{n}', a, len(blob)))
    t = ['.code16', '.intel_syntax noprefix', '.global _start', '_start: jmp main', '.org 0x200', 'main:',
         'mov ax,cs', 'mov ds,ax', 'mov ss,ax', f'mov sp,{TOP}', 'cld',
         f'mov ax,{CS}', 'mov es,ax', f'mov di,{ENTRY}', 'mov si,offset body', f'mov cx,{len(code["body"])}', 'rep movsb']
    t += [f'mov word ptr es:[{w}],{SYS}' for w in SEGWORDS]
    t += [f'mov ax,{SYS}', 'mov es,ax']
    for label, a, n in blobs:
        t += [f'mov di,{a}', f'mov si,offset {label}', f'mov cx,{n}', 'rep movsb']
    t += ['mov dx,offset inputname', 'mov ax,0x3d00', 'int 0x21', 'jc failure', 'mov word ptr [inputhandle],ax',
          'mov dx,offset outputname', 'xor cx,cx', 'mov ah,0x3c', 'int 0x21', 'jc failure', 'mov word ptr [outputhandle],ax',
          'mov word ptr [0xd800],cs', 'mov word ptr [0xd802],offset returned', f'mov word ptr [0xd804],{len(specs)}',
          'mov dx,0xd800', 'mov cx,6', 'call write_output',
          'again:', 'push cs', 'pop ds', 'mov bx,word ptr [inputhandle]', 'mov dx,offset spec', 'mov cx,16', 'mov ah,0x3f', 'int 0x21',
          'jc failure', 'cmp ax,16', 'jne failure']
    for seg in (DS, HEAP):
        for off in (0, 0x8000):
            t += [f'mov ax,{seg}', 'mov ds,ax', f'mov dx,{off}', 'mov cx,0x8000', 'call read_input']
    t += ['cli', 'push cs', 'pop ds', 'mov al,0x5a', 'push cs', 'pop es', 'cld', f'mov di,{LOW}', f'mov cx,{WINDOW - 16}', 'rep stosb',
          'push word ptr cs:[spec+12]', 'push word ptr cs:[spec+10]', 'push word ptr cs:[spec+8]', 'push word ptr cs:[spec+6]',
          'push word ptr cs:[spec+4]', 'push word ptr cs:[spec+2]', 'push word ptr cs:[spec]',
          'mov ax,0x1111', 'mov es,ax', f'mov ax,{DS}', 'mov ds,ax',
          'mov ax,0x1234', 'mov bx,0x3456', 'mov cx,0x5678', 'mov dx,0x789a', 'mov si,0x9abc', 'mov di,0xbcde', 'mov bp,0x7777',
          'popf', f'lcall {CS},{ENTRY}', 'returned:']
    t += [f'mov word ptr cs:[{0xC000 + 2 * n}],{r}' for n, r in enumerate('ax bx cx dx si di bp sp cs'.split())]
    t += ['mov word ptr cs:[0xc012],offset returned', 'mov word ptr cs:[0xc014],ds', 'mov word ptr cs:[0xc016],es',
          'mov word ptr cs:[0xc018],ss', 'mov sp,0xf800', 'pushf', 'pop ax', 'mov word ptr cs:[0xc01a],ax',
          'mov ax,cs', 'mov ds,ax', 'mov es,ax', 'cld', f'mov si,{LOW}', 'mov di,0xc01c', f'mov cx,{WINDOW}', 'rep movsb',
          'sti', 'mov dx,0xc000', f'mov cx,{28 + WINDOW}', 'call write_output']
    for seg in (DS, HEAP):
        for off in (0, 0x8000):
            t += [f'mov ax,{seg}', 'mov ds,ax', f'mov dx,{off}', 'mov cx,0x8000', 'call write_output']
    t += ['push cs', 'pop ds', 'mov ss,word ptr cs:[0xd800]', f'mov sp,{TOP}', 'inc word ptr [index]',
          f'cmp word ptr [index],{len(specs)}', 'jae done', 'jmp again', 'done:',
          'mov bx,word ptr [outputhandle]', 'mov ah,0x3e', 'int 0x21', 'mov ax,0x4c00', 'int 0x21',
          'write_output:', 'mov bx,word ptr cs:[outputhandle]', 'mov ah,0x40', 'int 0x21', 'jc failure', 'cmp ax,cx', 'jne failure', 'ret',
          'read_input:', 'mov bx,word ptr cs:[inputhandle]', 'mov ah,0x3f', 'int 0x21', 'jc failure', 'cmp ax,cx', 'jne failure', 'ret',
          'failure:', 'mov ax,0x4c01', 'int 0x21',
          'inputname: .asciz "INPUT.BIN"', 'outputname: .asciz "RESULT.BIN"', 'inputhandle: .word 0', 'outputhandle: .word 0',
          'index: .word 0', 'spec: .space 16', 'body:', f'.incbin "{directory / "body.bin"}"']
    for label, a, n in blobs:
        t += [f'{label}:', f'.incbin "{directory / (label + ".bin")}"']
    (directory / 'oracle.S').write_text('\n'.join(t) + '\n')
    subprocess.run(['as', '--32', str(directory / 'oracle.S'), '-o', str(directory / 'oracle.o')], check=True)
    com = directory / 'ORACLE.COM'
    subprocess.run(['ld', '-m', 'elf_i386', '-Ttext', '0x100', '--oformat', 'binary', str(directory / 'oracle.o'), '-o', str(com)], check=True)
    check(com.stat().st_size + 0x100 < 0xC000, 'COM overlaps the output buffer')
    (directory / 'dosbox.conf').write_text('[sdl]\noutput=surface\n[dosbox]\nmachine=ega\nmemsize=16\n[cpu]\ncore=normal\ncputype=386\n'
                                          'cycles=max\n[mixer]\nnosound=true\n[midi]\nmididevice=none\n[autoexec]\nmount c '
                                          + str(directory.resolve()) + '\nc:\nORACLE.COM\nexit\n')
    out = directory / 'RESULT.BIN'
    if out.exists():
        out.unlink()
    with (directory / 'dosbox.log').open('w') as log:
        subprocess.run(['dosbox-x', '-conf', str(directory / 'dosbox.conf'), '-fastlaunch', '-exit', '-noconsole'],
                       env=dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy'), stdout=log, stderr=subprocess.STDOUT,
                       check=True, timeout=600)
    result = out.read_bytes()
    check(len(result) == 6 + len(specs) * RECORD, f'native output length {len(result)}')
    segment, returned, count = struct.unpack_from('<3H', result); check(count == len(specs), 'native count')
    af_set = 0
    for n, case in enumerate(specs):
        rec = result[6 + n * RECORD:6 + (n + 1) * RECORD]
        m, c = fixture(case, code, segment, returned)
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m); ega = Ega(); lib.pr_ega_reset(ctypes.byref(ega))
        env = Env(None, OUT_FN(lambda *a: 0), IN_FN()); bus = Bus(ram, ctypes.pointer(ega), ctypes.pointer(env), WRITE_FN(lambda *a: None), None)
        d = Diagnostic()
        status = lib.pr_r0464_alloc_248e_cpu(ctypes.byref(bus), SIZE, ctypes.byref(c), 1 << 20, 64, ctypes.byref(d))
        check(status == 0, f'native C status {n}: {status}')
        for (reg, _), want in zip(Cpu._fields_, struct.unpack_from('<14H', rec)):
            check(getattr(c, reg) == want, f'native CPU {n}/{reg}: C {getattr(c, reg):04x} DOS {want:04x}')
        af_set += bool(c.flags & 0x10)
        mine = bytes(ram[physical(segment, LOW):physical(segment, LOW) + WINDOW])
        for seg in (DS, HEAP):
            mine += bytes(ram[physical(seg, 0):physical(seg, 0) + 65536])
        if mine != rec[28:]:
            diff = [i for i, (a, b) in enumerate(zip(mine, rec[28:])) if a != b]
            raise ValueError(f'native memory {n}: first {[hex(i) for i in diff[:6]]} ({len(diff)} bytes)')
    return dict(cases=len(specs), af_set_at_return=af_set, cpu_words=14, stack_bytes=WINDOW, segment_bytes=2 * 65536, flag_masks=0,
                machine='ega', core='normal', cpu='386', result_sha256=sha256(result).hexdigest(), com_segment=f'{segment:04X}')
