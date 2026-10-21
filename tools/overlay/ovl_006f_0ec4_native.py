"""Native DOSBox-X corroboration for OVL_006F_0EC4; used by ovl_006f_0ec4_cpu_audit.py.

The loader-expanded START.EXE image is relocated to segment 3000h with the loader's own table; the unit-0x6F span, the unit-0x9E span and the stub
trampoline are placed exactly as the Unicorn audit places them (the same build(), under a native layout); all initial memory the case needs is
written to STATE.BIN and read back into place by the oracle program through real INT 21h reads before each case. File I/O is REAL DOS: each case's
three archives are real files in the mounted directory, named by what the code itself builds (a one-letter DS:547C prefix + 'MON' + the case digit +
the constant + '.dax'). Declared native substitutions, each named here and in the evidence doc:
  * the 00B0:0048 stub entry is the same far JMP (EAh) to unit 0x9E +0DE3 as in the Unicorn run (the INT 3Fh overlay manager is not run);
  * the five bytes at 0AF8:00E2 (RunError's entry) are replaced by a far JMP to a capture routine, so a RunError is recorded instead of ending the
    oracle (the same boundary the C model stops at);
  * the two return words of the entry frame are written by the oracle program with its own return address;
  * real DOS (DOSBox-X 2026.01.02) uses the 24 bytes below each INT 21h's SP: those addresses are masked in the stack window.
Not native (Unicorn only): the message path (EGA, ReadKey, QUIT), prompts, keyboard, STALLED, aliasing and layout variants, flags with IF set.
Compared: all 14 CPU words and the regions below, against the C model on the identical state."""
import os, struct, subprocess
from hashlib import sha256
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, physical, put16, check
import res0899_quit_0000_cpu_audit as Q

A = None
NBASE = 0x3000
LAYOUT = dict(base=NBASE, heap=0x5000, cs_ov=0x8000, dice=0x8400, rec=0x8800, font=0x8C00)
SS, SP0, WIN_LO, WIN_LEN = 0x7000, 0xF000, 0xE000, 0x1020
HEAP_PARAS, HEAP_LEN = 0x0800, 0x8000
DOS_STACK = 24
CASES = ('all-three', 'class-01', 'class-02', 'class-03', 'class-0a', 'class-80', 'chain2-1', 'chain2-9', 'chain2-10', 'chain2-27', 'chain2-300',
         'chain3-1', 'chain3-62', 'chain3-63', 'chain3-64', 'chain3-127', 'chain3-400', 'block2-file-absent', 'block3-file-absent', 'blocks23-absent',
         'chain2-0', 'chain3-0', 'record-11c', 'record-200', 'heap-0040', 'heap-0060', 'seed-00000000', 'seed-ffffffff',
         'flags00d7', 'flags08d7', 'flags0402', 'flags0893', 'flags0003')


def chunks(phys, length):
    while length:
        n = min(length, 0x8000)
        yield phys, n
        phys += n; length -= n


def native(ctx, f, lib, audit):
    global A
    A = audit
    directory = ROOT / 'resources/0ec4-native'; directory.mkdir(parents=True, exist_ok=True)
    image, sites = Q.load_image()
    reloc = Q.relocate(image, sites, NBASE)
    stub = A.B.assemble_stub(scratch=A.SCRATCH)
    ds = NBASE + Q.SEG['data']; sysseg = NBASE + Q.SEG['system']
    first = {}
    for c in A.cases():
        first.setdefault(c[0], c)             # a name that two image lists share (flags0402) keeps its first case
    wanted = [first[name] for name in CASES if name in first]
    check(len(wanted) == len(CASES) and len(wanted) <= 260, 'native case list resolves')
    specs = []
    for n, (name, o0) in enumerate(wanted):
        o = A.normalise(o0)
        o.update(digit=n % 10, ds_name=bytes([65 + n // 10]), first_handle=7, heap_paras=min(o.get('heap_paras', HEAP_PARAS), HEAP_PARAS),
                 flags=(o.get('flags', 2) & ~0x200), L=dict(LAYOUT, ret=(0x9001, 0x7000)))
        specs.append((name, o))
    suffixes = {k: ctx['names'][n].decode('latin-1') for k, n in (('b1', 'c0e9c'), ('b2', 'c0ebc'), ('b3', 'c0ec0'))}
    ext = ctx['names']['c0ea0'].decode('latin-1')
    prefix = ctx['names']['c0e98'].decode('latin-1')
    states, rows_regions = [], None
    for n, (name, o) in enumerate(specs):
        for k in ('b1', 'b2', 'b3'):
            fname = directory / f'{chr(65 + n // 10)}{prefix}{n % 10}{suffixes[k]}{ext}'.upper()
            blob = A.pack_block(o['id'], o[k]) if o.get(k) is not None else (A.A716.archive([(0xEE, b'xx')]) if o.get(k + '_idmiss') else None)
            if blob is None:
                fname.unlink(missing_ok=True)
            else:
                fname.write_bytes(blob)
        m, c, spec = A.build(ctx, reloc, stub, o)
        regions = list(chunks(NBASE * 16, ds * 16 + 0x10000 - NBASE * 16)) + list(chunks(LAYOUT['heap'] * 16, HEAP_LEN)) + [
            (physical(LAYOUT['cs_ov'], 0xE98), 0x1262 - 0xE98), (physical(LAYOUT['dice'], ctx['dice_lo']), len(ctx['dice'])),
            (physical(spec['rec'][0], spec['rec'][1]), 0x240), (physical(SS, WIN_LO), WIN_LEN)]
        rows_regions = regions
        states.append((name, o, m, c, spec, b''.join(bytes(m[p:p + l]) for p, l in regions)))
    dump = [(physical(ds, 0x4400), 0x40), (physical(ds, 0x557C), 8), (physical(LAYOUT['heap'], 0), HEAP_LEN), (physical(states[0][4]['rec'][0], states[0][4]['rec'][1]), 0x240),
            (physical(SS, WIN_LO), WIN_LEN)]
    check(all(s[4]['rec'] == states[0][4]['rec'] for s in states), 'one record location')
    (directory / 'STATE.BIN').write_bytes(b''.join(s[5] for s in states))
    flags_table = [o['flags'] for _, o, *_ in states]
    row = lambda p, l: f'.word {p >> 4},{p & 0xF},{l}'
    t = ['.code16', '.intel_syntax noprefix', '.global _start', '_start: jmp main', '.org 0x200', 'main:',
         'mov ax,cs', 'mov ds,ax', 'mov ss,ax', 'mov sp,0xfff0', 'cld',
         'mov dx,offset outname', 'xor cx,cx', 'mov ah,0x3c', 'int 0x21', 'jc failure', 'mov word ptr [handle],ax',
         'mov dx,offset statename', 'mov ax,0x3d00', 'int 0x21', 'jc failure', 'mov word ptr [state],ax',
         'mov word ptr [header],cs', 'mov word ptr [header+2],offset returned', f'mov word ptr [header+4],{len(states)}', 'mov word ptr [header+6],0',
         'mov bx,word ptr [handle]', 'mov dx,offset header', 'mov cx,8', 'mov ah,0x40', 'int 0x21',
         'again:', 'cld', 'push cs', 'pop ds', 'mov word ptr [r],offset rows', f'mov word ptr [n],{len(rows_regions)}',
         'rd:', 'mov si,word ptr [r]', 'mov ax,word ptr [si]', 'mov dx,word ptr [si+2]', 'mov cx,word ptr [si+4]', 'mov bx,word ptr [state]',
         'mov ds,ax', 'mov ah,0x3f', 'int 0x21', 'push cs', 'pop ds', 'jc failure', 'add word ptr [r],6', 'dec word ptr [n]', 'jnz rd',
         f'mov ax,{sysseg}', 'mov es,ax', 'mov byte ptr es:[0xe2],0xea', 'mov word ptr es:[0xe3],offset cap_err', 'mov word ptr es:[0xe5],cs',
         f'mov ax,{SS}', 'mov es,ax', f'mov word ptr es:[{SP0}],offset returned', f'mov word ptr es:[{SP0 + 2}],cs',
         'mov si,word ptr [index]', 'shl si,1', 'mov ax,word ptr [si+flagtable]',
         'cli', f'mov bx,{SS}', 'mov ss,bx', f'mov sp,{SP0}', 'push ax', 'popf', f'mov word ptr ss:[{SP0 - 2}],0x5a5a',
         f'mov ax,{ds}', 'mov ds,ax', 'mov ax,0x1111', 'mov es,ax',
         'mov ax,0x1234', 'mov bx,0x3456', 'mov cx,0x5678', 'mov dx,0x789a', 'mov si,0x9abc', 'mov di,0xbcde', 'mov bp,0x7777',
         f'.byte 0xea; .word 0x0ec4,{LAYOUT["cs_ov"]}', 'returned:']
    t += [f'mov word ptr cs:[out+{2 * k}],{r}' for k, r in enumerate('ax bx cx dx si di bp sp'.split())]
    t += ['mov word ptr cs:[out+16],cs', 'mov word ptr cs:[out+18],offset returned', 'jmp common', 'cap_err:']
    t += [f'mov word ptr cs:[out+{2 * k}],{r}' for k, r in enumerate('ax bx cx dx si di bp sp'.split())]
    t += [f'mov word ptr cs:[out+16],{sysseg}', 'mov word ptr cs:[out+18],0xe2',
          'common:', 'mov word ptr cs:[out+20],ds', 'mov word ptr cs:[out+22],es', 'mov word ptr cs:[out+24],ss',
          'mov ax,cs', 'mov ss,ax', 'mov sp,0xfff0', 'pushf', 'pop ax', 'mov word ptr cs:[out+26],ax', 'cld', 'sti',
          'mov ax,cs', 'mov ds,ax', 'mov bx,word ptr [handle]', 'mov dx,offset out', 'mov cx,28', 'mov ah,0x40', 'int 0x21',
          'mov word ptr [r],offset dumprows', f'mov word ptr [n],{len(dump)}',
          'wr:', 'mov si,word ptr [r]', 'mov ax,word ptr [si]', 'mov dx,word ptr [si+2]', 'mov cx,word ptr [si+4]', 'mov bx,word ptr [handle]',
          'mov ds,ax', 'mov ah,0x40', 'int 0x21', 'push cs', 'pop ds', 'add word ptr [r],6', 'dec word ptr [n]', 'jnz wr',
          'mov bx,7', 'closeall:', 'mov ah,0x3e', 'push bx', 'int 0x21', 'pop bx', 'inc bx', 'cmp bx,20', 'jb closeall',
          'inc word ptr [index]', f'cmp word ptr [index],{len(states)}', 'jae done', 'jmp again', 'done:',
          'mov bx,word ptr [handle]', 'mov ah,0x3e', 'int 0x21', 'mov ax,0x4c00', 'int 0x21', 'failure:', 'mov ax,0x4c01', 'int 0x21',
          'outname: .asciz "RESULT.BIN"', 'statename: .asciz "STATE.BIN"', 'handle: .word 0', 'state: .word 0', 'index: .word 0', 'r: .word 0', 'n: .word 0',
          '.align 2', 'header: .space 8', 'out: .space 28', 'flagtable:', '.word ' + ','.join(str(v) for v in flags_table), 'rows:']
    t += [row(p, l) for p, l in rows_regions] + ['dumprows:'] + [row(p, l) for p, l in dump]
    (directory / 'oracle.S').write_text('\n'.join(t) + '\n')
    subprocess.run(['as', '--32', str(directory / 'oracle.S'), '-o', str(directory / 'oracle.o')], check=True)
    com = directory / 'ORACLE.COM'
    subprocess.run(['ld', '-m', 'elf_i386', '-Ttext', '0x100', '--oformat', 'binary', str(directory / 'oracle.o'), '-o', str(com)], check=True)
    check(com.stat().st_size + 0x100 < 0xF000, 'COM reaches its own stack')
    (directory / 'dosbox.conf').write_text('[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n[cpu]\ncore=normal\ncputype=386\ncycles=max\n'
                                          '[mixer]\nnosound=true\n[midi]\nmididevice=none\n[autoexec]\nmount c ' + str(directory.resolve()) + '\nc:\nORACLE.COM\nexit\n')
    out = directory / 'RESULT.BIN'
    if out.exists():
        out.unlink()
    binary = '/usr/bin/dosbox-x'
    with (directory / 'dosbox.log').open('w') as log:
        subprocess.run([binary, '-conf', str(directory / 'dosbox.conf'), '-fastlaunch', '-exit', '-noconsole'],
                       env=dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy'), stdout=log, stderr=subprocess.STDOUT, check=True, timeout=900)
    data = out.read_bytes(); rec_len = 28 + sum(l for _, l in dump)
    check(len(data) == 8 + len(states) * rec_len, f'native length {len(data)} expected {8 + len(states) * rec_len}')
    comseg, ret_off, count, _ = struct.unpack_from('<4H', data); check(count == len(states), 'native count')
    stops, compared = {}, 0
    for n, (name, o, m, c, spec, _) in enumerate(states):
        rec = data[8 + n * rec_len:8 + (n + 1) * rec_len]
        m2 = bytearray(m); put16(m2, SS, SP0, ret_off); put16(m2, SS, SP0 + 2, comseg)
        r = A.run_c(f, lib, bytes(m2), c, dict(spec, ret=(comseg, ret_off)))
        stop = r['status']
        check(stop in ('RETURN', 'RUNERROR'), f'native C {name}: {stop}')
        stops[stop] = stops.get(stop, 0) + 1
        for (reg, _), want in zip(Cpu._fields_, struct.unpack_from('<14H', rec)):
            check(getattr(r['cpu'], reg) == want, f'native {name} CPU {reg}: model {getattr(r["cpu"], reg):04x} DOS {want:04x}')
        mask = {physical(s_, (sp - k) & 0xFFFF) for s_, sp in r['int_sps'] for k in range(1, DOS_STACK + 1)}
        at = 28
        for p, length in dump:
            for k in range(length):
                if p + k not in mask:
                    check(r['ram'][p + k] == rec[at + k], f'native {name} at {p + k:05x}: model {r["ram"][p + k]:02x} DOS {rec[at + k]:02x}')
                    compared += 1
            at += length
    probe = subprocess.run([binary, '--version'], capture_output=True, text=True, env=dict(os.environ, SDL_VIDEODRIVER='dummy'))
    version = next(line.split(',')[0].strip() for line in (probe.stdout + probe.stderr).splitlines() if 'DOSBox-X version' in line)
    return dict(cases=len(states), stops=dict(sorted(stops.items())), cpu_words=14, dos_stack_mask_bytes_below_int_sp=DOS_STACK,
                compared_regions=[f'{p:05X}+{l:X}' for p, l in dump], bytes_compared=compared, state_bin_sha256=sha256((directory / 'STATE.BIN').read_bytes()).hexdigest(),
                result_sha256=sha256(data).hexdigest(), image_bin_sha256=sha256(reloc).hexdigest(),
                captures=dict(runerror=dict(at='0AF8:00E2')), dosbox_x=dict(path=binary, sha256=sha256(open(binary, 'rb').read()).hexdigest(), version=version,
                                                                         pinned_source='5a9c7c53481c2e933948039c33256776653f4faf', pinned_build_used=False))
