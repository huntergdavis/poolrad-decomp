"""Native DOSBox-X corroboration for RES_0899_0059 as a composed closure; used by res0899_readkey_0059_cpu_audit.py.

Same layout and declared substitutions as res0899_quit_0000_native.py (the relocated image read from IMAGE.BIN before
every case; the INT 21h hook for 25h/35h/2Ch; the INT 10h stub; the Halt capture; the derived IN 61h mask; the GetTime
frame mask), plus:
  * INT 16h: a hook replays the case's key script (AH=00h takes the next word, AH=01h reports it and sets ZF in the IRET
    frame when the script is empty), returning by IRET with no stack use of its own;
  * BREAK capture: the native copy's five bytes at 0A40:014B (C6 06 02 85 00, MOV BYTE [8502],0: the first instruction of
    Crt's Ctrl-Break path, which the model stops before) are replaced by a far JMP to a capture routine; the receipt
    records the bytes before and after;
  * RETURN: the body returns through a real far-return frame into the oracle program;
  * the PSP the ParamStr bytes read is a synthetic one at segment 5800h: the count byte at :0080, the tail from :0081,
    then 0Dh."""
import ctypes, os, struct, subprocess
from hashlib import sha256
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, physical, put16, check
import res0899_quit_0000_cpu_audit as Q
K = None  # the running 0059 audit module (set by native())

NBASE, NSS, NPSP, TOP, LOW, WINDOW = 0x3000, 0x6000, 0x5800, 0xF000, 0xEC00, 0x0400
HALT_BYTES = bytes.fromhex('33c933dbba')
BREAK_BYTES = bytes.fromhex('c606028500')
TAILS = [b' STING', b'', b' STING foo', b' sting', b' xyz']
MAXKEYS = 4
TIMES = 96


def regions(ds):
    drv = NBASE + Q.SEG['driver']
    return [(NSS, LOW, WINDOW), (ds, 0x4A00, 0x20), (ds, 0x5370, 0x08), (ds, 0x2610, 0x04), (ds, 0x49F6, 0x02),
            (ds, 0x8500, 0x04), (drv, 0x01D0, 0x0530), (drv, 0x1378, 0x04), (0x0040, 0x0049, 0x01)]


def native_cases():
    k = K.K
    out = []
    for cur, saved in ((0, 0), (1, 1), (2, 0), (2, 1), (2, 5)):
        out.append((f'ctrl-s-c{cur}-s{saved}', dict(keys=(k['s'],), cur=cur, saved=saved, w=1)))
    out += [(f'ctrl-s-off-a{arg:x}', dict(keys=(k['s'],), cur=0, w=1, arg=arg)) for arg in (0, 1, 0xFF, 5)]
    for t, tail in enumerate(TAILS):
        for key in ('d', 'c', 'a'):
            out.append((f'{key}-tail{t}', dict(keys=(k[key],), tail=tail)))
    out += [('drain-3', dict(keys=(k['a'], k['x'], k['a'], k['x']))), ('drain-ext', dict(keys=(k['a'], k['f1'], k['a']))),
            ('ext-first', dict(keys=(k['f1'],))), ('zero-first', dict(keys=(k['zero'],))), ('buffered', dict(keys=(), buffered=0x48)),
            ('break-a', dict(keys=(k['a'],), brk=1)), ('break-drain', dict(keys=(k['a'], k['x']), brk=1)),
            ('ctrl-d-49f7-1', dict(keys=(k['d'],), flag49=1))]
    out += [(f'flags{fl:x}-{lab}', dict(flags=fl, keys=ks)) for fl in K.FLAGS for lab, ks in (('a', (k['a'],)), ('s', (k['s'],)))]
    return out


def c_layout(image_reloc, o, comseg, stub_off, ret_off):
    m = bytearray([0x5A]) * SIZE
    b = NBASE * 16; m[b:b + len(image_reloc)] = image_reloc
    ds = NBASE + Q.SEG['data']
    m[physical(ds, 0x5372)] = o.get('cur', 1); m[physical(ds, 0x5373)] = o.get('saved', 0)
    put16(m, ds, 0x5374, o.get('w', 1)); put16(m, ds, 0x2612, o.get('arg', 0)); m[physical(ds, 0x4A1C)] = o.get('mode', 3)
    m[physical(ds, 0x49F7)] = o.get('flag49', 0); m[physical(ds, 0x8501)] = o.get('buffered', 0); m[physical(ds, 0x8502)] = o.get('brk', 0)
    put16(m, ds, 0x4422, NPSP)
    tail = o.get('tail', b' STING')
    m[physical(NPSP, 0x80)] = len(tail)
    for i, ch in enumerate(tail + b'\r'):
        m[physical(NPSP, 0x81 + i)] = ch
    put16(m, 0, 0x40, stub_off); put16(m, 0, 0x42, comseg)
    s = physical(comseg, stub_off); m[s:s + len(Q.INT10_STUB)] = Q.INT10_STUB
    m[physical(0x0040, 0x0049)] = 0x5A
    sp = TOP - 4
    put16(m, NSS, sp, ret_off); put16(m, NSS, sp + 2, comseg)
    c = Cpu(0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, sp, NBASE + Q.SEG['body'], K.ENTRY, ds, 0x1111, NSS, o.get('flags', 2))
    return m, c


def native(image, sites, decoded, f, audit):
    global K
    K = audit
    directory = ROOT / 'resources/0899-readkey/native'; directory.mkdir(parents=True, exist_ok=True)
    image_reloc = Q.relocate(image, sites, NBASE)
    sysseg = NBASE + Q.SEG['system']; crtseg = NBASE + K.CRT
    check(image_reloc[sysseg * 16 + 0xE9 - NBASE * 16:][:5] == HALT_BYTES, 'Halt entry bytes before the native capture')
    check(image_reloc[crtseg * 16 + 0x14B - NBASE * 16:][:5] == BREAK_BYTES, 'Crt break-path bytes before the native capture')
    (directory / 'IMAGE.BIN').write_bytes(image_reloc)
    specs = native_cases()
    clock = Q.clock(4736147, 7)
    times = [w for n in range(TIMES) for w in clock(n)]
    rows = []
    for _, o in specs:
        keys = list(o.get('keys', (0x1E61,))) + [0] * MAXKEYS
        rows.append((o.get('flags', 2), o.get('cur', 1), o.get('saved', 0), o.get('w', 1), o.get('arg', 0), o.get('flag49', 0),
                     o.get('buffered', 0), o.get('brk', 0), TAILS.index(o.get('tail', b' STING')), len(o.get('keys', (0x1E61,))))
                    + tuple(keys[:MAXKEYS]))
    ROW = 2 * len(rows[0])
    ds = NBASE + Q.SEG['data']
    rec_len = 28 + sum(n for _, _, n in regions(ds))
    writes = []
    for seg, off, n in regions(ds):
        writes += [f'mov ax,{seg}', 'mov ds,ax', 'mov bx,word ptr cs:[handle]', f'mov dx,{off}', f'mov cx,{n}', 'mov ah,0x40', 'int 0x21']
    fill = []
    for chunk in range(2):
        fill += [f'mov ax,{NBASE + 0x1000 * chunk}', 'mov es,ax', 'xor di,di', 'mov cx,0x8000', 'mov ax,0x5a5a', 'rep stosw']
    load = []
    for k, start in enumerate(range(0, len(image_reloc), 0x8000)):
        load += [f'mov ax,{NBASE + 0x800 * k}', 'mov ds,ax', 'mov bx,word ptr cs:[image]', 'xor dx,dx',
                 f'mov cx,{min(0x8000, len(image_reloc) - start)}', 'mov ah,0x3f', 'int 0x21', 'jc failure']
    tails_bin = b''.join(bytes([len(t)]) + t + b'\r' + b'\x5a' * (30 - len(t)) for t in TAILS)   # 32 bytes per tail
    capture = []
    for label, cs_val, ip_val in (('capture_exit', sysseg, 0xE9), ('capture_break', crtseg, 0x14B)):
        capture += [f'{label}:', f'mov word ptr cs:[out+16],{cs_val}', f'mov word ptr cs:[out+18],{ip_val}', 'jmp record']
    t = ['.code16', '.intel_syntax noprefix', '.global _start', '_start: jmp main', '.org 0x200', 'main:',
         'mov ax,cs', 'mov ds,ax', 'mov ss,ax', 'mov sp,0xfff0', 'cld',
         'mov dx,offset outname', 'xor cx,cx', 'mov ah,0x3c', 'int 0x21', 'jc failure', 'mov word ptr [handle],ax',
         'mov word ptr [header],cs', 'mov word ptr [header+2],offset stub10', f'mov word ptr [header+4],{len(rows)}',
         'mov word ptr [header+6],offset returned',
         'mov bx,word ptr [handle]', 'mov dx,offset header', 'mov cx,8', 'mov ah,0x40', 'int 0x21',
         'xor ax,ax', 'mov es,ax', 'mov ax,word ptr es:[0x84]', 'mov word ptr [old21],ax', 'mov ax,word ptr es:[0x86]', 'mov word ptr [old21+2],ax',
         'mov ax,word ptr es:[0x40]', 'mov word ptr [old10],ax', 'mov ax,word ptr es:[0x42]', 'mov word ptr [old10+2],ax',
         'mov ax,word ptr es:[0x58]', 'mov word ptr [old16],ax', 'mov ax,word ptr es:[0x5a]', 'mov word ptr [old16+2],ax',
         'again:', 'cld'] + fill + [
         'push cs', 'pop ds', 'mov dx,offset imagename', 'mov ax,0x3d00', 'int 0x21', 'jc failure', 'mov word ptr cs:[image],ax'] + load + [
         'push cs', 'pop ds', 'mov bx,word ptr [image]', 'mov ah,0x3e', 'int 0x21',
         f'mov ax,{sysseg}', 'mov es,ax', 'mov byte ptr es:[0xe9],0xea', 'mov word ptr es:[0xea],offset capture_exit', 'mov word ptr es:[0xec],cs',
         f'mov ax,{crtseg}', 'mov es,ax', 'mov byte ptr es:[0x14b],0xea', 'mov word ptr es:[0x14c],offset capture_break', 'mov word ptr es:[0x14e],cs',
         'mov si,word ptr [index]', f'mov ax,{ROW}', 'mul si', 'mov si,ax', 'add si,offset table',
         'mov word ptr cs:[row],si',
         f'mov ax,{ds}', 'mov es,ax', 'mov al,byte ptr cs:[si+2]', 'mov byte ptr es:[0x5372],al', 'mov al,byte ptr cs:[si+4]',
         'mov byte ptr es:[0x5373],al', 'mov ax,word ptr cs:[si+6]', 'mov word ptr es:[0x5374],ax', 'mov ax,word ptr cs:[si+8]',
         'mov word ptr es:[0x2612],ax', 'mov byte ptr es:[0x4a1c],3', 'mov al,byte ptr cs:[si+10]', 'mov byte ptr es:[0x49f7],al',
         'mov al,byte ptr cs:[si+12]', 'mov byte ptr es:[0x8501],al', 'mov al,byte ptr cs:[si+14]', 'mov byte ptr es:[0x8502],al',
         f'mov word ptr es:[0x4422],{NPSP}',
         # The synthetic PSP tail: 32 bytes from tails[index] to 5800:0080.
         'push si', 'mov ax,word ptr cs:[si+16]', 'mov cl,5', 'shl ax,cl', 'mov si,ax', 'add si,offset tails', f'mov ax,{NPSP}', 'mov es,ax',
         'mov di,0x80', 'mov cx,32', 'rep movsb', 'pop si',
         # The key script: count and words for the INT 16h hook.
         'mov ax,word ptr cs:[si+18]', 'mov word ptr cs:[kcount],ax', 'mov word ptr cs:[kidx],0',
         'lea ax,[si+20]', 'mov word ptr cs:[kbase],ax',
         f'mov ax,{NSS}', 'mov es,ax', f'mov di,{LOW}', f'mov cx,{WINDOW // 2}', 'mov ax,0x5a5a', 'rep stosw',
         'mov word ptr cs:[vtable+32],0x0123', 'mov word ptr cs:[vtable+34],0xf000', 'mov word ptr cs:[tidx],0',
         'cli', 'xor ax,ax', 'mov es,ax', 'mov word ptr es:[0x84],offset int21', 'mov word ptr es:[0x86],cs',
         'mov word ptr es:[0x40],offset stub10', 'mov word ptr es:[0x42],cs', 'mov word ptr es:[0x58],offset int16', 'mov word ptr es:[0x5a],cs',
         'mov ax,0x40', 'mov es,ax', 'mov al,byte ptr es:[0x49]', 'mov byte ptr cs:[oldmode],al', 'mov byte ptr es:[0x49],0x5a',
         f'mov ax,{NSS}', 'mov ss,ax', f'mov sp,{TOP}', 'push cs', 'push offset returned', 'push word ptr cs:[si]',
         f'mov ax,{ds}', 'mov ds,ax', 'mov ax,0x1111', 'mov es,ax',
         'mov ax,0x1234', 'mov bx,0x3456', 'mov cx,0x5678', 'mov dx,0x789a', 'mov si,0x9abc', 'mov di,0xbcde', 'mov bp,0x7777',
         'popf', f'ljmp {NBASE + Q.SEG["body"]},{K.ENTRY}',
         'returned:', 'mov word ptr cs:[out+16],cs', 'mov word ptr cs:[out+18],offset returned', 'jmp record'] + capture + ['record:']
    t += [f'mov word ptr cs:[out+{2 * n}],{r}' for n, r in enumerate('ax bx cx dx si di bp sp'.split())]
    t += ['mov word ptr cs:[out+20],ds', 'mov word ptr cs:[out+22],es', 'mov word ptr cs:[out+24],ss', 'mov ax,cs', 'mov ss,ax',
          'mov sp,0xfff0', 'pushf', 'pop ax', 'mov word ptr cs:[out+26],ax', 'mov ax,cs', 'mov ds,ax', 'xor ax,ax', 'mov es,ax',
          'mov ax,word ptr [old21]', 'mov word ptr es:[0x84],ax', 'mov ax,word ptr [old21+2]', 'mov word ptr es:[0x86],ax',
          'mov ax,word ptr [old10]', 'mov word ptr es:[0x40],ax', 'mov ax,word ptr [old10+2]', 'mov word ptr es:[0x42],ax',
          'mov ax,word ptr [old16]', 'mov word ptr es:[0x58],ax', 'mov ax,word ptr [old16+2]', 'mov word ptr es:[0x5a],ax',
          'mov bx,word ptr [handle]', 'mov dx,offset out', 'mov cx,28', 'mov ah,0x40', 'int 0x21'] + writes + [
          'push cs', 'pop ds', 'mov bx,word ptr [handle]', 'mov dx,offset vtable+32', 'mov cx,4', 'mov ah,0x40', 'int 0x21',
          'mov bx,word ptr [handle]', 'mov dx,offset tidx', 'mov cx,2', 'mov ah,0x40', 'int 0x21',
          'mov bx,word ptr [handle]', 'mov dx,offset kidx', 'mov cx,2', 'mov ah,0x40', 'int 0x21',
          'mov ax,0x40', 'mov es,ax', 'mov al,byte ptr cs:[oldmode]', 'mov byte ptr es:[0x49],al', 'sti',
          'inc word ptr [index]', f'cmp word ptr [index],{len(rows)}', 'jae done', 'jmp again', 'done:',
          'mov bx,word ptr [handle]', 'mov ah,0x3e', 'int 0x21', 'mov ax,0x4c00', 'int 0x21', 'failure:', 'mov ax,0x4c01', 'int 0x21',
          'int21:', 'cmp ah,0x2c', 'je t2c', 'cmp ah,0x35', 'je f35', 'cmp ah,0x25', 'je f25', 'jmp dword ptr cs:[old21]',
          'f35:', 'mov bl,al', 'xor bh,bh', 'shl bx,1', 'shl bx,1', 'les bx,dword ptr cs:[vtable+bx]', 'iret',
          'f25:', 'mov word ptr cs:[save],bx', 'mov bl,al', 'xor bh,bh', 'shl bx,1', 'shl bx,1', 'mov word ptr cs:[vtable+bx],dx',
          'mov word ptr cs:[vtable+bx+2],ds', 'mov bx,word ptr cs:[save]', 'iret',
          't2c:', 'mov word ptr cs:[save],bx', 'mov bx,word ptr cs:[tidx]', 'mov cx,word ptr cs:[times+bx]', 'mov dx,word ptr cs:[times+bx+2]',
          'add word ptr cs:[tidx],4', 'mov bx,word ptr cs:[save]', 'iret',
          # Declared INT 16h hook: the key script. AH=00h takes the next word; AH=01h reports it, ZF in the frame when empty.
          'int16:', 'cmp ah,1', 'je kb_peek', 'cmp ah,0', 'je kb_take', 'jmp dword ptr cs:[old16]',
          'kb_take:', 'mov word ptr cs:[save],bx', 'mov bx,word ptr cs:[kidx]', 'shl bx,1', 'add bx,word ptr cs:[kbase]',
          'mov ax,word ptr cs:[bx]', 'inc word ptr cs:[kidx]', 'mov bx,word ptr cs:[save]', 'iret',
          'kb_peek:', 'mov word ptr cs:[save],bx', 'mov word ptr cs:[save2],bp', 'mov bp,sp', 'and word ptr [bp+4],0xffbf',
          'mov bx,word ptr cs:[kidx]', 'cmp bx,word ptr cs:[kcount]', 'jb kb_key', 'or word ptr [bp+4],0x40', 'jmp kb_out',
          'kb_key:', 'shl bx,1', 'add bx,word ptr cs:[kbase]', 'mov ax,word ptr cs:[bx]',
          'kb_out:', 'mov bp,word ptr cs:[save2]', 'mov bx,word ptr cs:[save]', 'iret',
          'stub10:', '.byte ' + ','.join(str(x) for x in Q.INT10_STUB),
          'outname: .asciz "RESULT.BIN"', 'imagename: .asciz "IMAGE.BIN"', 'handle: .word 0', 'image: .word 0', 'index: .word 0',
          'row: .word 0', 'save: .word 0', 'save2: .word 0', 'tidx: .word 0', 'kidx: .word 0', 'kcount: .word 0', 'kbase: .word 0',
          'oldmode: .byte 0', '.align 2', 'header: .space 8', 'out: .space 28',
          'old21: .word 0,0', 'old10: .word 0,0', 'old16: .word 0,0', 'vtable: .space 1024', 'times: .word ' + ','.join(str(x) for x in times),
          'tails: .byte ' + ','.join(str(x) for x in tails_bin), '.align 2', 'table:']
    t += ['.word ' + ','.join(str(v) for v in row) for row in rows]
    (directory / 'oracle.S').write_text('\n'.join(t) + '\n')
    subprocess.run(['as', '--32', str(directory / 'oracle.S'), '-o', str(directory / 'oracle.o')], check=True)
    com = directory / 'ORACLE.COM'
    subprocess.run(['ld', '-m', 'elf_i386', '-Ttext', '0x100', '--oformat', 'binary', str(directory / 'oracle.o'), '-o', str(com)], check=True)
    check(com.stat().st_size + 0x100 < 0xF000, 'COM reaches its own stack')
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
                       check=True, timeout=600)
    data = out.read_bytes(); record = rec_len + 4 + 2 + 2
    check(len(data) == 8 + len(rows) * record, f'native length {len(data)} expected {8 + len(rows) * record}')
    comseg, stub_off, count, ret_off = struct.unpack_from('<4H', data); check(count == len(rows), 'native count')
    stops, masked_cases = {}, 0
    for n, (name, o) in enumerate(specs):
        rec = data[8 + n * record:8 + (n + 1) * record]
        m, c = c_layout(image_reloc, o, comseg, stub_off, ret_off)
        spec = dict(table={8: (0x0123, 0xF000)}, in61=0x00, time_start=4736147, time_step=7)
        sps, kbs = [], []
        st, actual, ram, env, d = K.run_c(f, m, c, spec, o.get('keys', (0x1E61,)), stub_at=(comseg, stub_off), time_sps=sps,
                                          kb_sps=kbs)
        st2, _, ram2, _, _ = K.run_c(f, m, c, dict(spec, in61=0x80), o.get('keys', (0x1E61,)), stub_at=(comseg, stub_off))
        check(st in ('RETURN', 'EXIT', 'BREAK') and st == st2, f'native C status {name}: {st}/{st2}')
        stops[st] = stops.get(st, 0) + 1
        for (reg, _), want in zip(Cpu._fields_, struct.unpack_from('<14H', rec)):
            check(getattr(actual, reg) == want, f'native CPU {name}/{reg}: C {getattr(actual, reg):04x} DOS {want:04x}')
        dep = {i: ram[i] ^ ram2[i] for i in range(SIZE) if ram[i] != ram2[i]}
        check(all(v & ~0xC4 == 0 for v in dep.values()) and len(dep) <= 2 and (not dep or physical(ds, 0x4A14) in dep),
              f'{name}: IN-dependent bytes {sorted((hex(k), hex(v)) for k, v in dep.items())}')
        masked_cases += bool(dep)
        # The BIOS/DOS-side frames of the real INT 21h AH=2Ch and INT 16h calls (outside the GetTime and Crt models).
        frame_mask = {physical(NSS, (sp - k) & 0xFFFF) for sp in sps + kbs for k in range(1, 7)}
        at = 28
        for seg, off, length in regions(ds):
            for k in range(length):
                p = physical(seg, (off + k) & 0xFFFF)
                if p in frame_mask:
                    continue
                mask = ~dep.get(p, 0) & 0xFF
                check(ram[p] & mask == rec[at + k] & mask, f'native {name} at {seg:04x}:{(off + k) & 0xFFFF:04x}: C {ram[p]:02x} DOS {rec[at + k]:02x}')
            at += length
        check(struct.unpack_from('<HH', rec, at) == env.table.get(8), f'native vector entry {name}')
        check(struct.unpack_from('<H', rec, at + 4)[0] == 4 * len(sps), f'native GetTime calls {name}')
        check(struct.unpack_from('<H', rec, at + 6)[0] == sum(1 for e in env.log if e[0] == '16' and e[1] == 0), f'native keys taken {name}')
    probe = subprocess.run([binary, '--version'], capture_output=True, text=True, env=dict(os.environ, SDL_VIDEODRIVER='dummy'))
    version = next(line.split(',')[0].strip() for line in (probe.stdout + probe.stderr).splitlines() if 'DOSBox-X version' in line)
    return dict(cases=len(rows), stops=stops, cpu_words=14, compared_regions=[f'{s:04X}:{o:04X}+{n:X}' for s, o, n in regions(ds)],
                in61_masked_cases=masked_cases,
                captures=dict(halt=dict(at='0AF8:00E9', before=HALT_BYTES.hex()), crt_break=dict(at='0A40:014B', before=BREAK_BYTES.hex()),
                              after='EA <capture> <oracle CS>'),
                result_sha256=sha256(data).hexdigest(), image_bin_sha256=sha256(image_reloc).hexdigest(),
                dosbox_x=dict(path=binary, sha256=sha256(open(binary, 'rb').read()).hexdigest(), version=version,
                              pinned_source='5a9c7c53481c2e933948039c33256776653f4faf', pinned_build_used=False))
