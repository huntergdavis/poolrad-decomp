#!/usr/bin/env python3
"""DOSBox-X original U3C6 +0CB9 chain, including declared +0D29 seam stops."""
import ctypes
from hashlib import sha256
import json
import os
from pathlib import Path
import struct
import subprocess
from u3c6_scan_0cb9_audit import Cpu, ROOT, SIZE, physical, put16, source, library, fixture

OUT = ROOT / 'resources/u3c6-0cb9/native'
STACK = tuple(range(0xEFC0, 0xF020)) + tuple(range(0xFFC0, 0x10000)) + tuple(range(0x40))
DATA_SIZE = 0x7000
RECORD_SIZE = 10 + 28 + len(STACK) + DATA_SIZE


def assembly(codepath, options):
    sp = options.get('sp', 0xEFF8)
    partial = options.get('partial', False)
    kw = {k: v for k, v in options.items() if k != 'partial'}
    # Use symbolic DS=3000 for generation, then relocate only the map segment.
    memory, _ = fixture(**dict(dict(ss=0x2000, ds=0x3000, sp=0xEFF8,
                                    p6=(0x3000, 0x1000), p10=(0x3000, 0x1010),
                                    map_ptr=(0x3000, 0x100)), **kw))
    a = ['.intel_syntax noprefix', '.code16', '.global _start', '_start: jmp main',
         '.org 0x200', 'code:', f'.incbin "{codepath.resolve()}"', '.org 0x1200', 'main:',
         'mov ax,cs', 'mov ds,ax', 'mov es,ax', 'mov ss,ax', 'mov sp,0xf000',
         'mov bx,0x4100', 'mov ah,0x4a', 'int 0x21', 'jc failure',
         'mov dx,offset filename', 'xor cx,cx', 'mov ah,0x3c', 'int 0x21', 'jc failure',
         'mov word ptr cs:[handle],ax', 'cli', 'cld',
         'mov ax,cs', 'add ax,0x1000', 'mov word ptr cs:[dispatch+2],ax',
         'mov word ptr cs:[output],ax', 'mov es,ax', 'mov si,offset code',
         'xor di,di', 'mov cx,0xf00', 'rep movsb']
    if partial:
        # Observer replaces the first instruction the model declines to execute.
        a += ['mov byte ptr es:[0xd29],0xea', 'mov word ptr es:[0xd2a],offset returned',
              'mov ax,cs', 'mov word ptr es:[0xd2c],ax']
    a += ['mov ax,cs', 'add ax,0x2000', 'mov ss,ax', 'mov es,ax',
          'mov word ptr cs:[output+2],ax', 'xor di,di', 'mov cx,0xffff',
          'mov al,0xa5', 'rep stosb', 'stosb',
          'mov ax,ss', 'add ax,0x1000', 'mov es,ax', 'mov word ptr cs:[output+4],ax',
          'xor di,di', 'mov cx,0xffff', 'mov al,0xa5', 'rep stosb', 'stosb',
          'mov ax,es', 'mov ds,ax']
    data = memory[physical(0x3000, 0):physical(0x3000, 0) + DATA_SIZE]
    # Emit synthetic initialization as runs; no game bytes are embedded here.
    i = 0
    while i < len(data):
        if data[i] == 0xA5:
            i += 1
            continue
        end = i + 1
        while end < len(data) and data[end] == data[i]:
            end += 1
        if end - i >= 4:
            a += [f'mov di,{i}', f'mov cx,{end-i}', f'mov al,{data[i]}', 'rep stosb']
        else:
            a += [f'mov byte ptr [{j}],{data[j]}' for j in range(i, end)]
        i = end
    a += ['mov ax,ds', 'mov word ptr [0x6718],ax']
    for off, val in ((0, 'offset returned'), (4, '0x1000'), (8, '0x1010'),
                     (12, str(options.get('k', 0))), (14, '0x1234'), (16, '0x5678')):
        a += [f'mov word ptr ss:[{(sp+off)&65535}],{val}']
    a += ['mov ax,ds', f'mov word ptr ss:[{(sp+6)&65535}],ax',
          f'mov word ptr ss:[{(sp+10)&65535}],ax',
          'mov ax,cs', f'mov word ptr ss:[{(sp+2)&65535}],ax',
          'mov word ptr cs:[output+8],ax', 'mov word ptr cs:[output+6],offset returned',
          'mov sp,0x8000', 'push 2', 'popf', 'mov ax,0x1111', 'mov es,ax',
          'mov ax,0xabcd', 'mov bx,0x3456', 'mov cx,0x5678', 'mov dx,0x789a',
          'mov si,0x9abc', 'mov di,0xbcde', 'mov bp,0x7777', f'mov sp,{sp}',
          'jmp far ptr cs:[dispatch]', 'returned:']
    for i, reg in enumerate('ax bx cx dx si di bp sp cs'.split()):
        a += [f'mov word ptr cs:[output+{10+2*i}],{reg}']
    a += ['mov word ptr cs:[output+28],offset returned',
          'mov word ptr cs:[output+30],ds', 'mov word ptr cs:[output+32],es',
          'mov word ptr cs:[output+34],ss', 'mov sp,0x8000',
          'pushf', 'pop word ptr cs:[output+36]']
    if partial:
        a += ['mov ax,word ptr cs:[output]', 'mov word ptr cs:[output+26],ax',
              'mov word ptr cs:[output+28],0xd29']
    for i, off in enumerate(STACK):
        a += [f'mov al,byte ptr ss:[{off}]', f'mov byte ptr cs:[output+{38+i}],al']
    a += ['mov ax,cs', 'mov es,ax', 'xor si,si', f'mov di,offset output+{38+len(STACK)}',
          f'mov cx,{DATA_SIZE}', 'cld', 'rep movsb',
          'mov ax,cs', 'mov ss,ax', 'mov ds,ax', 'mov sp,0xf000', 'sti',
          'mov bx,word ptr [handle]', 'mov dx,offset output', f'mov cx,{RECORD_SIZE}',
          'mov ah,0x40', 'int 0x21', 'jc failure', f'cmp ax,{RECORD_SIZE}', 'jne failure',
          'mov bx,word ptr [handle]', 'mov ah,0x3e', 'int 0x21',
          'mov ax,0x4c00', 'int 0x21', 'failure:', 'mov ax,0x4c01', 'int 0x21',
          'filename: .asciz "RESULT.BIN"', 'handle: .word 0',
          'dispatch: .word 0xcb9,0', f'output: .space {RECORD_SIZE}']
    return '\n'.join(a) + '\n'


def main():
    bodies, _ = source()
    OUT.mkdir(parents=True, exist_ok=True)
    code = bytearray(0xF00)
    for start, body in bodies.items():
        code[start:start+len(body)] = body
    cp = OUT / 'code.bin'
    cp.write_bytes(code)
    lib = library(OUT)
    cases = [dict(), dict(direction=0), dict(slot=None), dict(slot=255, count=0),
             dict(cells=(0, 2, 5, 7)), dict(cells=(2, 0, 5, 7)),
             dict(cells=(30, 2, 5, 7)), dict(cells=(2, 30, 5, 7)),
             dict(ranks=(0, 0, 0, 0)), dict(ranks=(4, 3, 2, 1)),
             dict(ranks=(1, 1, 1, 1)), dict(occupants=(1, 1, 1, 1)),
             dict(x=49, y=24), dict(x=50), dict(x=128, y=255),
             dict(pairs=((128, 0),)*4), dict(k=3, bias=(255, 255))]
    cases += [dict(sp=sp, **kw) for sp in (0xFFFE, 0xFFFF)
              for kw in (dict(), dict(direction=0), dict(cells=(30, 2, 5, 7)))]
    cases += [dict(sp=sp) for sp in (0x1C, 0x1D)]
    cases += [dict(sp=sp, partial=True) for sp in (0x14, 0x15)]
    hashes = []
    for index, options in enumerate(cases):
        d = OUT / f'case-{index:02d}'
        d.mkdir(exist_ok=True)
        asm, obj, com = d/'oracle.S', d/'oracle.o', d/'ORACLE.COM'
        asm.write_text(assembly(cp, options))
        subprocess.run(['as', '--32', str(asm), '-o', str(obj)], check=True)
        subprocess.run(['ld', '-m', 'elf_i386', '-Ttext', '0x100', '--oformat', 'binary', str(obj), '-o', str(com)], check=True)
        assert com.stat().st_size < 0xEF00, com.stat().st_size
        cfg = d/'dosbox.conf'
        cfg.write_text('[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n[cpu]\ncore=normal\ncputype=386\ncycles=max\n[mixer]\nnosound=true\n[midi]\nmididevice=none\n[autoexec]\nmount c '+str(d)+'\nc:\nORACLE.COM\nexit\n')
        output = d/'RESULT.BIN'
        output.unlink(missing_ok=True)
        with (d/'dosbox.log').open('w') as log:
            subprocess.run(['dosbox-x', '-conf', str(cfg), '-fastlaunch', '-exit', '-noconsole'],
                           env=dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy'),
                           stdout=log, stderr=subprocess.STDOUT, check=True, timeout=45)
        result = output.read_bytes()
        assert len(result) == RECORD_SIZE, (index, len(result))
        code_cs, ss, ds, return_ip, return_cs = struct.unpack_from('<5H', result)
        regs = struct.unpack_from('<14H', result, 10)
        kw = {k: v for k, v in options.items() if k != 'partial'}
        memory, cpu = fixture(**dict(dict(ss=ss, ds=ds, cs=code_cs, flags=2, sp=0xEFF8,
                                          map_ptr=(ds, 0x100), p6=(ds, 0x1000), p10=(ds, 0x1010)), **kw))
        put16(memory, ss, cpu.sp, return_ip)
        put16(memory, ss, (cpu.sp+2)&65535, return_cs)
        for start, body in bodies.items():
            memory[physical(code_cs, start):physical(code_cs, start)+len(body)] = body
        ram = (ctypes.c_uint8*SIZE).from_buffer(memory)
        status = lib.pr_u3c6_0cb9_cpu_run(ram, SIZE, ctypes.byref(cpu), 1)
        assert status == (5 if options.get('partial') else 0), (index, status)
        for (name, _), value in zip(Cpu._fields_, regs):
            assert getattr(cpu, name) == value, (index, name, hex(getattr(cpu, name)), hex(value))
        for i, off in enumerate(STACK):
            assert memory[physical(ss, off)] == result[38+i], (index, 'stack', hex(off))
        expected_data = memory[physical(ds, 0):physical(ds, 0)+DATA_SIZE]
        actual_data = result[38+len(STACK):]
        assert expected_data == actual_data, (index, 'data', [(i, a, b) for i, (a, b) in enumerate(zip(expected_data, actual_data)) if a != b][:10])
        hashes.append(sha256(result).hexdigest())
        print(f'PASS native case {index}: {options}', flush=True)
    report = dict(target='OVL_03C6_0CB9', native_cases=len(cases), full_chain_returns=len(cases)-2,
                  partial_seam_stops=2, cpu_words=14, stack_bytes=len(STACK), data_bytes=DATA_SIZE,
                  masked_flags=0, entry_return_wrap_cases=6, internal_return_wrap_full_cases=2,
                  internal_return_wrap_partial_cases=2, les_segment_seams_measured=False,
                  natural_game_route_measured=False, result_sha256=hashes)
    (OUT/'report.json').write_text(json.dumps(report, indent=2, sort_keys=True)+'\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    main()
