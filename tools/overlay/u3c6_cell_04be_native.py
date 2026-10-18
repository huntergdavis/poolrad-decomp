#!/usr/bin/env python3
"""DOSBox-X original standalone +04BE returns and explicit pre-instruction stops."""
import ctypes
from hashlib import sha256
import json
import os
from pathlib import Path
import struct
import subprocess
from u3c6_cell_04be_audit import Cpu, ROOT, SIZE, START, physical, put16, source, library, fixture

OUT = ROOT/'resources/u3c6-04be/native'
STACK = tuple(range(0xEFC0, 0xF020)) + tuple(range(0xFFC0, 0x10000)) + tuple(range(0x40))
DATA_SIZE = 0x7000
RECORD_SIZE = 38 + len(STACK) + DATA_SIZE


def options_for(options, ss, ds, cs):
    kw = {k: v for k, v in options.items() if k not in ('alias', 'stop', 'pending')}
    kw = dict(dict(sp=0xEFF8, ss=ss, ds=ds, cs=cs, flags=2,
                   map_ptr=(ds, 0x2000), a=(ds, 0x1000), b=(ds, 0x1010)), **kw)
    sp = kw['sp']
    aliases = {'row': (ss, (sp+12)&65535), 'column': (ss, (sp+14)&65535),
               'b-offset': (ss, (sp+8)&65535), 'saved-bp': (ss, (sp-2)&65535),
               'same': kw['b'], 'code-a': (cs, START)}
    if options.get('alias') in aliases:
        kw['a'] = aliases[options['alias']]
    if options.get('alias') == 'code-b':
        kw['b'] = (cs, START)
    return kw


def assembly(codepath, options):
    kw = options_for(options, 0x2000, 0x3000, 0x1000)
    memory, _ = fixture(**kw)
    sp, stop = kw['sp'], options.get('stop')
    a = ['.intel_syntax noprefix', '.code16', '.global _start', '_start: jmp main',
         '.org 0x200', 'code:', f'.incbin "{codepath.resolve()}"', '.org 0x900', 'main:',
         'mov ax,cs', 'mov ds,ax', 'mov es,ax', 'mov ss,ax', 'mov sp,0xf000',
         'mov bx,0x4100', 'mov ah,0x4a', 'int 0x21', 'jc failure',
         'mov dx,offset filename', 'xor cx,cx', 'mov ah,0x3c', 'int 0x21', 'jc failure',
         'mov word ptr cs:[handle],ax', 'cli', 'cld',
         'mov ax,cs', 'add ax,0x1000', 'mov word ptr cs:[dispatch+2],ax',
         'mov word ptr cs:[output],ax', 'mov es,ax', 'mov si,offset code',
         'xor di,di', 'mov cx,0x600', 'rep movsb']
    if stop:
        # Execute only the prefix before the instruction declined by the model.
        a += [f'mov byte ptr es:[{stop}],0xea', f'mov word ptr es:[{stop+1}],offset returned',
              'mov ax,cs', f'mov word ptr es:[{stop+3}],ax']
    a += ['mov ax,cs', 'add ax,0x2000', 'mov ss,ax', 'mov es,ax',
          'mov word ptr cs:[output+2],ax', 'xor di,di', 'mov cx,0xffff',
          'mov al,0xa5', 'rep stosb', 'stosb',
          'mov ax,ss', 'add ax,0x1000', 'mov es,ax', 'mov word ptr cs:[output+4],ax',
          'xor di,di', 'mov cx,0xffff', 'mov al,0xa5', 'rep stosb', 'stosb',
          'mov ax,es', 'mov ds,ax']
    data = memory[physical(0x3000, 0):physical(0x3000, 0)+DATA_SIZE]
    for i, value in enumerate(data):
        if value != 0xA5:
            a += [f'mov byte ptr [{i}],{value}']
    a += ['mov ax,ds', 'mov word ptr [0x6718],ax']
    for off, value in ((0, 'offset returned'), (4, str(kw['a'][1])), (8, str(kw['b'][1])),
                       (12, str(0xA500|kw.get('y', 4))), (14, str(0x5A00|kw.get('x', 3)))):
        a += [f'mov word ptr ss:[{(sp+off)&65535}],{value}']
    for off, seg in ((6, kw['a'][0]), (10, kw['b'][0])):
        operand = {0x2000: 'ss', 0x3000: 'ds', 0x1000: 'word ptr cs:[output]'}[seg]
        a += [f'mov ax,{operand}', f'mov word ptr ss:[{(sp+off)&65535}],ax']
    a += ['mov ax,cs', f'mov word ptr ss:[{(sp+2)&65535}],ax',
          'mov word ptr cs:[output+8],ax', 'mov word ptr cs:[output+6],offset returned',
          'mov sp,0x8000', 'push 2', 'popf', 'mov ax,0x1111', 'mov es,ax',
          'mov ax,0xabcd', 'mov bx,0x3456', 'mov cx,0x5678', 'mov dx,0x789a',
          'mov si,0x9abc', 'mov di,0xbcde', 'mov bp,0x7777', f'mov sp,{sp}',
          'jmp far ptr cs:[dispatch]', 'returned:']
    for i, reg in enumerate('ax bx cx dx si di bp sp cs'.split()):
        a += [f'mov word ptr cs:[output+{10+2*i}],{reg}']
    a += ['mov word ptr cs:[output+28],offset returned', 'mov word ptr cs:[output+30],ds',
          'mov word ptr cs:[output+32],es', 'mov word ptr cs:[output+34],ss',
          'mov sp,0x8000', 'pushf', 'pop word ptr cs:[output+36]']
    if stop:
        a += ['mov ax,word ptr cs:[output]', 'mov word ptr cs:[output+26],ax',
              f'mov word ptr cs:[output+28],{stop}']
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
          'dispatch: .word 0x4be,0', f'output: .space {RECORD_SIZE}']
    # DOSBox-X's AX-to-moffs word store checks the segment limit at FFFF.
    # This is fixture initialization, not an instruction under test. Write
    # its bytes at the intended consecutive physical addresses explicitly.
    a = [line for ins in a for line in
         (['mov byte ptr ss:[65535],al', 'mov byte ptr ds:[0],ah']
          if ins == 'mov word ptr ss:[65535],ax' else [ins])]
    return '\n'.join(a)+'\n'


def main():
    body, _ = source()
    OUT.mkdir(parents=True, exist_ok=True)
    code = bytearray(0x600)
    code[START:START+len(body)] = body
    cp = OUT/'code.bin'
    cp.write_bytes(code)
    lib = library(OUT)
    cases = [dict(), dict(x=0, y=0), dict(x=49, y=24), dict(x=50),
             dict(x=128), dict(x=255), dict(y=25), dict(y=128),
             dict(y=255), dict(x=127, y=127)]
    cases += [dict(alias=alias, a_value=0x80)
              for alias in ('row', 'column', 'b-offset', 'saved-bp', 'same')]
    cases += [dict(sp=sp, x=x) for sp in (0xFFFE, 0xFFFF) for x in (3, 128)]
    cases += [dict(sp=1, x=x) for x in (3, 128)]
    cases += [dict(pending=True, stop=0x528, x=x) for x in (3, 128)]
    cases += [dict(sp=sp, x=x, stop=(0x511 if x == 3 else 0x519))
              for sp in (0xFFF5, 0xFFF6, 0xFFF7) for x in (3, 128)]
    cases += [dict(sp=sp, x=x, stop=(0x4F4 if x == 3 else 0x520))
              for sp in (0xFFF9, 0xFFFA, 0xFFFB) for x in (3, 128)]
    cases += [dict(alias='code-a', x=x, stop=(0x4F7 if x == 3 else 0x523)) for x in (3, 128)]
    cases += [dict(alias='code-b', x=x, stop=(0x514 if x == 3 else 0x51C)) for x in (3, 128)]
    hashes, statuses, stops = [], {}, {}
    for index, options in enumerate(cases):
        d = OUT/f'case-{index:02d}'
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
        memory, cpu = fixture(**options_for(options, ss, ds, code_cs))
        put16(memory, ss, cpu.sp, return_ip)
        put16(memory, ss, (cpu.sp+2)&65535, return_cs)
        memory[physical(code_cs, START):physical(code_cs, START)+len(body)] = body
        ram = (ctypes.c_uint8*SIZE).from_buffer(memory)
        status = lib.pr_u3c6_04be_cpu_run(ram, SIZE, ctypes.byref(cpu), 0 if options.get('pending') else 1)
        expected = (3 if options.get('pending') else 2 if options.get('alias', '').startswith('code-')
                    else 4 if options.get('stop') else 0)
        assert status == expected, (index, 'status', status, expected)
        for (name, _), value in zip(Cpu._fields_, regs):
            assert getattr(cpu, name) == value, (index, name, hex(getattr(cpu, name)), hex(value))
        for i, off in enumerate(STACK):
            assert memory[physical(ss, off)] == result[38+i], (index, 'stack', hex(off))
        expected_data = memory[physical(ds, 0):physical(ds, 0)+DATA_SIZE]
        actual_data = result[38+len(STACK):]
        assert expected_data == actual_data, (index, 'data', [(i, a, b) for i, (a, b) in enumerate(zip(expected_data, actual_data)) if a != b][:10])
        hashes.append(sha256(result).hexdigest())
        statuses[str(status)] = statuses.get(str(status), 0)+1
        if status:
            key = f'{status}:{cpu.ip:04X}'
            stops[key] = stops.get(key, 0)+1
        print(f'PASS native case {index}: {options}', flush=True)
    report = dict(target='OVL_03C6_04BE', native_cases=len(cases), full_returns=statuses.get('0', 0),
                  partial_stops=len(cases)-statuses.get('0', 0), cpu_words=14,
                  stack_bytes=len(STACK), data_bytes=DATA_SIZE, masked_flags=0,
                  entry_return_wrap_cases=4, bp_push_pop_word_seam_cases=2,
                  executed_les_seams_measured=False, natural_game_route_measured=False,
                  status_counts=statuses, stop_counts=stops, result_sha256=hashes)
    (OUT/'report.json').write_text(json.dumps(report, indent=2, sort_keys=True)+'\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    main()
