#!/usr/bin/env python3
"""RES_0464_00CF (video-mode set over Dos.Intr) and RES_0464_00FF (restore) against the original bytes, composed with
Dos.Intr 0A32:000B (#1259).

The whole loader-expanded START.EXE image is loaded at a test base with the loader's own relocation table applied, so every
original byte of the closure runs. Declared environment, identical in every oracle and in the C service: a real-byte
INT 10h stub at the vector (sets the BIOS mode byte 0040:0049 := AL and IRETs), the same stub RES_0899_0000's audit uses.
Unicorn: CPU14 + full 1 MiB, unmasked. DOSBox-X: the image is read from IMAGE.BIN, IVT[10h] points at the stub in the
oracle COM, the routine is far-called and returns to a capture label. Media and generated programs stay under ignored
resources/."""
import argparse, ctypes, json, os, random, struct, subprocess, tempfile
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from resident_loader_map import unpack, relocations
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, REGISTERS, physical, put16, check

CHECKER_ROWS = ('RES_0464_00CF', 'RES_0464_00FF')
IMAGE_DIGEST = 'a42a57e68d3f7815831821cc9365bb917befda45a58e958544451fc7a878df8e'
BODY_SEG, DOS_SEG, DATA_SEG = 0x0464, 0x0A32, 0x0C7C
ROUTINES = {'00cf': dict(entry=0x00CF, end=0x00FF, insns=19, symbol='pr_r0464_set_mode_00cf_cpu', arg=True, sp=(0x0022, 0xFFFA)),
            '00ff': dict(entry=0x00FF, end=0x010C, insns=8, symbol='pr_r0464_restore_mode_00ff_cpu', arg=False, sp=(0x002A, 0xFFFC))}
BASE, SS, STUB_SEG = 0x1000, 0x7000, 0x6000
RET_CS, RET_IP = 0x9001, 0x7000
INT10_STUB = bytes.fromhex('1e' '50' 'b84000' '8ed8' '58' 'a24900' '1f' 'cf')  # push ds; push ax; mov ax,40h; mov ds,ax; pop ax; mov [49h],al; pop ds; iret
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003, 0x0E02)
SOURCES = ['resident_0464_video_mode_cpu', 'dos_intr_000b_cpu']
VIDEO_FN = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8), ctypes.POINTER(Cpu), ctypes.POINTER(Cpu))
STATUS = {0: 'OK', 1: 'ARGUMENT', 2: 'INTR', 3: 'TRANSFER'}


class VideoEnv(ctypes.Structure):
    _fields_ = [('video', VIDEO_FN), ('video_context', ctypes.c_void_p)]


def load_image():
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes()
    image = bytes(unpack(exe)[0])
    check(sha256(image).hexdigest() == IMAGE_DIGEST, 'loader-expanded image identity')
    return image, relocations(exe, image)[0]


def relocate(image, sites, base):
    out = bytearray(image)
    for s in sites:
        out[s:s + 2] = struct.pack('<H', (struct.unpack_from('<H', out, s)[0] + base) & 0xFFFF)
    return bytes(out)


def source(image, sites):
    spans = {}
    for name, r in ROUTINES.items():
        body = image[BODY_SEG * 16 + r['entry']:BODY_SEG * 16 + r['end']]
        ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, r['entry']))
        check(len(ins) == r['insns'] and sum(i.size for i in ins) == len(body), f'{name}: decode covers the span')
        check([i.mnemonic for i in ins if i.mnemonic.startswith('ret')] == ['retf'] and not [i for i in ins if i.mnemonic.startswith('j')],
              f'{name}: straight line, one RETF')
        spans[name] = dict(bytes=len(body), sha256=sha256(body).hexdigest(), starts=[i.address for i in ins])
    calls = [(i.address, i.op_str) for i in Cs(CS_ARCH_X86, CS_MODE_16).disasm(image[BODY_SEG * 16 + 0x00CF:BODY_SEG * 16 + 0x010C], 0x00CF)
             if 'call' in i.mnemonic]
    check(calls == [(0x00EE, '0xa32, 0xb'), (0x0107, '0xcf')], f'calls {calls}')
    check(BODY_SEG * 16 + 0x00F1 in set(sites), 'the LCALL segment word is a loader relocation')
    return spans


def library(directory):
    path = directory / 'video.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include')] +
                   [str(ROOT / f'src/recovered/{s}.c') for s in SOURCES] + ['-o', str(path)], check=True)
    lib = ctypes.CDLL(str(path)); out = {}
    for name, r in ROUTINES.items():
        f = getattr(lib, r['symbol'])
        f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(VideoEnv), ctypes.POINTER(ctypes.c_int)]
        f.restype = ctypes.c_int; out[name] = f
    return out


def c_video(stub_at, keep):
    def video(ctx, ram, before, after):
        """The INT 10h stub's effect, as its bytes run: [SP-2] := DS, [SP-4] := AX, 0040:0049 := AL, IRET."""
        b = before[0]
        if (b.cs, b.ip) != stub_at:
            return 1
        a = Cpu.from_buffer_copy(b)
        for k, v in ((2, b.ds), (4, b.ax)):
            at = physical(b.ss, (b.sp - k) & 0xFFFF); ram[at] = v & 255; ram[(at + 1) % SIZE] = v >> 8
        ram[physical(0x0040, 0x0049)] = b.ax & 0xFF
        rd = lambda o: ram[physical(b.ss, (b.sp + o) & 0xFFFF)] | ram[physical(b.ss, (b.sp + o + 1) & 0xFFFF)] << 8
        a.ip, a.cs, a.flags = rd(0), rd(2), rd(4); a.sp = (b.sp + 6) & 0xFFFF
        after[0] = a; return 0
    fn = VIDEO_FN(video); keep.append(fn)
    return VideoEnv(fn, None)


def build(image_reloc, base, name, *, mode=0x0D, prev=0x03, saved=0x13, regs=None, ss=SS, sp=0xF000, flags=2, cpu=None,
          stub=(STUB_SEG, 0), bda=0x5A, fill=0x5A):
    r = ROUTINES[name]
    m = bytearray([fill]) * SIZE
    b = base * 16; m[b:b + len(image_reloc)] = image_reloc
    ds = base + DATA_SEG
    m[physical(ds, 0x4A1C)] = saved; m[physical(ds, 0x4A1D)] = prev
    m[physical(ds, 0x4A02)] = 0xEE; m[physical(ds, 0x4A03)] = 0xEE
    for k, v in enumerate(regs or (0x0B0B, 0x0C0C, 0x0D0D, 0x0E0E, 0x0F0F, 0x1010, 0x2121, 0x3131, 0x0246)):
        put16(m, ds, 0x4A04 + 2 * k, v)                       # Regs.BX .. Regs.FLAGS (Regs.AX is written by the body)
    s = physical(*stub); m[s:s + len(INT10_STUB)] = INT10_STUB
    put16(m, 0, 0x40, stub[1]); put16(m, 0, 0x42, stub[0])
    m[physical(0x0040, 0x0049)] = bda
    words = (RET_IP, RET_CS, (0xA500 | mode) if r['arg'] else None)
    for k, v in enumerate(w for w in words if w is not None):
        put16(m, ss, (sp + 2 * k) & 0xFFFF, v)
    regs7 = cpu or (0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777)
    return m, Cpu(*regs7, sp, base + BODY_SEG, r['entry'], ds, 0x1111, ss, flags)


def run_unicorn(label, m, c, stub):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_INTR
    from unicorn import x86_const as R
    uc = Uc(UC_ARCH_X86, UC_MODE_16); backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    uc.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing)); uc.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        uc.reg_write(reg, getattr(c, n))
    s = dict(stop=None, steps=0); seen = set()

    def code(em, address, size, user):
        cs = uc.reg_read(R.UC_X86_REG_CS); ip = uc.reg_read(R.UC_X86_REG_IP)
        if (cs, ip) == (RET_CS, RET_IP):
            s['stop'] = 'OK'; return uc.emu_stop()
        check(cs in (BASE + BODY_SEG, BASE + DOS_SEG, stub[0]), f'escape {label} {cs:04x}:{ip:04x}')
        seen.add((cs, ip)); s['steps'] += 1
        if s['steps'] > 5000:
            s['stop'] = 'RUNAWAY'; uc.emu_stop()

    def intr(em, intno, user):
        raise ValueError(f'unexpected INT {intno:#x} {label}')
    uc.hook_add(UC_HOOK_CODE, code); uc.hook_add(UC_HOOK_INTR, intr)
    uc.emu_start(physical(c.cs, c.ip), 2 * SIZE, count=20000)
    check(s['stop'] == 'OK', f'original did not return {label}: {s["stop"]}')
    return uc, backing, seen


def compare(fs, label, name, m, c, stub):
    uc, backing, seen = run_unicorn(label, m, c, stub)
    actual = Cpu.from_buffer_copy(c); ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m); keep = []
    env = c_video(stub, keep); status = ctypes.c_int(99)
    st = fs[name](ram, SIZE, ctypes.byref(actual), ctypes.byref(env), ctypes.byref(status))
    check(STATUS.get(st) == 'OK', f'C status {label}: {STATUS.get(st)} intr {status.value}')
    if bytes(ram) != bytes(backing):
        diff = [hex(i) for i, (a, b) in enumerate(zip(bytes(ram), bytes(backing))) if a != b]
        raise ValueError(f'RAM {label}: {diff[:8]} ({len(diff)} bytes)')
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        want = uc.reg_read(reg) & 0xFFFF
        check(getattr(actual, n) == want, f'CPU {label}/{n} C {getattr(actual, n):04x} original {want:04x}')
    return seen, actual, bytes(ram)


def cases(name):
    rng = random.Random(int('0464' + name, 16)); lo, hi = ROUTINES[name]['sp']
    out = [(f'mode{md:02x}-prev{pv:02x}', dict(mode=md, prev=pv, saved=sv)) for md, pv, sv in
           ((0x00, 0x03, 0x0D), (0x03, 0x0D, 0x03), (0x0D, 0x03, 0x0D), (0x10, 0x0D, 0x00), (0x13, 0xFF, 0x13), (0xFF, 0x00, 0xFF))]
    out += [(f'flags{fl:x}', dict(flags=fl)) for fl in FLAGS]
    out += [('sp-low', dict(sp=lo)), ('sp-high', dict(sp=hi & ~1)), ('sp-odd', dict(sp=0xE001)), ('ss-ds', dict(ss=BASE + DATA_SEG, sp=0xE000)),
            ('ss-a20', dict(ss=0xFFFF, sp=0x8000)), ('stub-high', dict(stub=(0xF800, 0x0100)))]
    for k in range(40):
        out.append((f'random{k}', dict(mode=rng.randrange(256), prev=rng.randrange(256), saved=rng.randrange(256), flags=rng.choice(FLAGS),
                                       sp=rng.randrange(max(lo, 0x0800), 0xF000) & ~1, cpu=tuple(rng.randrange(65536) for _ in range(7)),
                                       regs=tuple(rng.randrange(65536) for _ in range(9)), bda=rng.randrange(256), fill=rng.randrange(256))))
    return out


def differential(image_reloc, spans, fs):
    report = {}
    for name, r in ROUTINES.items():
        seen = set(); specs = cases(name)
        for label, o in specs:
            m, c = build(image_reloc, BASE, name, **o)
            v, final, ram = compare(fs, f'{name} {label}', name, m, c, o.get('stub', (STUB_SEG, 0))); seen |= v
            ds = BASE + DATA_SEG
            # Independent expectations from the body's own bytes.
            if name == '00cf':
                mode, prev = o.get('mode', 0x0D), o.get('prev', 0x03)
                check(ram[physical(ds, 0x4A1C)] == prev and ram[physical(ds, 0x4A1D)] == mode, f'{label}: [4A1C] := old [4A1D]; [4A1D] := mode')
            else:
                mode, prev = o.get('saved', 0x13), o.get('prev', 0x03)
                check(ram[physical(ds, 0x4A1C)] == prev and ram[physical(ds, 0x4A1D)] == mode, f'{label}: restore the saved mode')
            check(ram[physical(ds, 0x4A02)] == mode and ram[physical(ds, 0x4A03)] == 0, f'{label}: Regs.AX = (0, mode)')
            check(ram[physical(0x0040, 0x0049)] == mode, f'{label}: INT 10h saw AL = the mode')
            sp = o.get('sp', 0xF000)
            check((final.cs, final.ip) == (RET_CS, RET_IP) and final.sp == (sp + (6 if r['arg'] else 4)) & 0xFFFF, f'{label}: returned')
        body = {ip for cs, ip in seen if cs == BASE + BODY_SEG and r['entry'] <= ip < r['end']}
        check(body == set(spans[name]['starts']), f'{name}: body starts {sorted(set(spans[name]["starts"]) - body)} missing')
        intr = {ip for cs, ip in seen if cs == BASE + DOS_SEG}
        report[name] = dict(cases=len(specs), body_starts=f"{len(body)}/{len(spans[name]['starts'])}", intr_starts=len(intr),
                            conditional_outcomes_total=0, cpu_words=14, ram_bytes=SIZE, flag_masks=0)
    return report


# ---- DOSBox-X ---------------------------------------------------------------------------------------------
NBASE, NSS, TOP, LOW, WINDOW = 0x3000, 0x6000, 0xF000, 0xEF00, 0x0100


def native_cases():
    return [(f'm{md:02x}-p{pv:02x}-f{fl:x}', dict(mode=md, prev=pv, saved=sv, flags=fl)) for md, pv, sv, fl in
            ((0x00, 0x03, 0x0D, 0x0002), (0x03, 0x0D, 0x03, 0x00D7), (0x0D, 0x03, 0x0D, 0x08D7), (0x10, 0x0D, 0x00, 0x0402),
             (0x13, 0xFF, 0x13, 0x0893), (0xFF, 0x00, 0xFF, 0x0003), (0x01, 0x02, 0x04, 0x0E02), (0x0E, 0x12, 0x07, 0x0002))]


def native(image, sites, spans, fs):
    directory = ROOT / 'resources/0464-video/native'; directory.mkdir(parents=True, exist_ok=True)
    image_reloc = relocate(image, sites, NBASE)
    (directory / 'IMAGE.BIN').write_bytes(image_reloc)
    ds = NBASE + DATA_SEG
    rows = []
    for name in ROUTINES:
        for label, o in native_cases():
            rows.append((name, label, o))
    regs_rest = (0x0B0B, 0x0C0C, 0x0D0D, 0x0E0E, 0x0F0F, 0x1010, 0x2121, 0x3131, 0x0246)
    regions = [(NSS, LOW, WINDOW), (ds, 0x4A00, 0x20), (0x0040, 0x0049, 1)]
    writes = []
    for seg, off, n in regions:
        writes += [f'mov ax,{seg}', 'mov ds,ax', 'mov bx,word ptr cs:[handle]', f'mov dx,{off}', f'mov cx,{n}', 'mov ah,0x40', 'int 0x21']
    fill = []
    for chunk in range(2):
        fill += [f'mov ax,{NBASE + 0x1000 * chunk}', 'mov es,ax', 'xor di,di', 'mov cx,0x8000', 'mov ax,0x5a5a', 'rep stosw']
    load = []
    for k, start in enumerate(range(0, len(image_reloc), 0x8000)):
        load += [f'mov ax,{NBASE + 0x800 * k}', 'mov ds,ax', 'mov bx,word ptr cs:[image]', 'xor dx,dx',
                 f'mov cx,{min(0x8000, len(image_reloc) - start)}', 'mov ah,0x3f', 'int 0x21', 'jc failure']
    table = [(ROUTINES[n]['entry'], o['flags'], o['mode'], o['prev'], o['saved']) for n, _, o in rows]
    t = ['.code16', '.intel_syntax noprefix', '.global _start', '_start: jmp main', '.org 0x200', 'main:',
         'mov ax,cs', 'mov ds,ax', 'mov ss,ax', 'mov sp,0xfff0', 'cld',
         'mov dx,offset outname', 'xor cx,cx', 'mov ah,0x3c', 'int 0x21', 'jc failure', 'mov word ptr [handle],ax',
         'mov word ptr [header],cs', 'mov word ptr [header+2],offset stub10', f'mov word ptr [header+4],{len(rows)}',
         'mov bx,word ptr [handle]', 'mov dx,offset header', 'mov cx,6', 'mov ah,0x40', 'int 0x21',
         'xor ax,ax', 'mov es,ax', 'mov ax,word ptr es:[0x40]', 'mov word ptr [old10],ax', 'mov ax,word ptr es:[0x42]', 'mov word ptr [old10+2],ax',
         'again:', 'cld'] + fill + [
         'push cs', 'pop ds', 'mov dx,offset imagename', 'mov ax,0x3d00', 'int 0x21', 'jc failure', 'mov word ptr cs:[image],ax'] + load + [
         'push cs', 'pop ds', 'mov bx,word ptr [image]', 'mov ah,0x3e', 'int 0x21',
         'mov si,word ptr [index]', 'mov ax,si', 'shl si,1', 'shl si,1', 'add si,ax', 'shl si,1', 'add si,offset table',   # 10 bytes/row
         f'mov ax,{ds}', 'mov es,ax', 'mov al,byte ptr cs:[si+6]', 'mov byte ptr es:[0x4a1d],al', 'mov al,byte ptr cs:[si+8]',
         'mov byte ptr es:[0x4a1c],al', 'mov byte ptr es:[0x4a02],0xee', 'mov byte ptr es:[0x4a03],0xee'] + [
         f'mov word ptr es:[{0x4A04 + 2 * k}],{v}' for k, v in enumerate(regs_rest)] + [
         f'mov ax,{NSS}', 'mov es,ax', f'mov di,{LOW}', f'mov cx,{WINDOW // 2}', 'mov ax,0x5a5a', 'rep stosw',
         'cli', 'xor ax,ax', 'mov es,ax', 'mov word ptr es:[0x40],offset stub10', 'mov word ptr es:[0x42],cs',
         'mov ax,0x40', 'mov es,ax', 'mov al,byte ptr es:[0x49]', 'mov byte ptr cs:[oldmode],al', 'mov byte ptr es:[0x49],0x5a',
         'mov ax,word ptr cs:[si]', 'mov word ptr cs:[target],ax', f'mov word ptr cs:[target+2],{NBASE + BODY_SEG}',
         f'mov ax,{NSS}', 'mov ss,ax', f'mov sp,{TOP}',
         'cmp word ptr cs:[si],0xcf', 'jne noarg', 'mov al,byte ptr cs:[si+4]', 'mov ah,0xa5', 'push ax', 'noarg:',
         'push cs', 'push offset capture', 'push word ptr cs:[si+2]',
         f'mov ax,{ds}', 'mov ds,ax', 'mov ax,0x1111', 'mov es,ax',
         'mov ax,0x1234', 'mov bx,0x3456', 'mov cx,0x5678', 'mov dx,0x789a', 'mov si,0x9abc', 'mov di,0xbcde', 'mov bp,0x7777',
         'popf', 'jmp dword ptr cs:[target]', 'capture:']
    t += [f'mov word ptr cs:[out+{2 * n}],{r}' for n, r in enumerate('ax bx cx dx si di bp sp'.split())]
    t += ['mov word ptr cs:[out+16],cs', 'mov word ptr cs:[out+18],offset capture', 'mov word ptr cs:[out+20],ds', 'mov word ptr cs:[out+22],es',
          'mov word ptr cs:[out+24],ss', 'mov ax,cs', 'mov ss,ax', 'mov sp,0xfff0',
          'pushf', 'pop ax', 'mov word ptr cs:[out+26],ax', 'mov ax,cs', 'mov ds,ax', 'xor ax,ax', 'mov es,ax',
          'mov ax,word ptr [old10]', 'mov word ptr es:[0x40],ax', 'mov ax,word ptr [old10+2]', 'mov word ptr es:[0x42],ax',
          'mov bx,word ptr [handle]', 'mov dx,offset out', 'mov cx,28', 'mov ah,0x40', 'int 0x21'] + writes + [
          'push cs', 'pop ds', 'mov ax,0x40', 'mov es,ax', 'mov al,byte ptr cs:[oldmode]', 'mov byte ptr es:[0x49],al', 'sti',
          'inc word ptr [index]', f'cmp word ptr [index],{len(rows)}', 'jae done', 'jmp again', 'done:',
          'mov bx,word ptr [handle]', 'mov ah,0x3e', 'int 0x21', 'mov ax,0x4c00', 'int 0x21', 'failure:', 'mov ax,0x4c01', 'int 0x21',
          'stub10:', '.byte ' + ','.join(str(x) for x in INT10_STUB),
          'outname: .asciz "RESULT.BIN"', 'imagename: .asciz "IMAGE.BIN"', 'handle: .word 0', 'image: .word 0', 'index: .word 0',
          'oldmode: .byte 0', '.align 2', 'header: .space 6', 'out: .space 28', 'old10: .word 0,0', 'target: .word 0,0', 'table:']
    t += ['.word ' + ','.join(str(v) for v in row) for row in table]
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
    with (directory / 'dosbox.log').open('w') as log:
        subprocess.run(['/usr/bin/dosbox-x', '-conf', str(directory / 'dosbox.conf'), '-fastlaunch', '-exit', '-noconsole'],
                       env=dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy'), stdout=log, stderr=subprocess.STDOUT,
                       check=True, timeout=600)
    data = out.read_bytes(); rec = 28 + sum(n for _, _, n in regions)
    check(len(data) == 6 + len(rows) * rec, f'native length {len(data)}')
    comseg, stub_off, count = struct.unpack_from('<3H', data); check(count == len(rows), 'native count')
    for n, (name, label, o) in enumerate(rows):
        r = data[6 + n * rec:6 + (n + 1) * rec]
        m, c = build(image_reloc, NBASE, name, mode=o['mode'], prev=o['prev'], saved=o['saved'], flags=o['flags'], ss=NSS,
                     sp=TOP - (6 if ROUTINES[name]['arg'] else 4), stub=(comseg, stub_off))
        # The harness frame: the far return is the oracle's capture label (its offset is the recorded IP).
        put16(m, NSS, c.sp, struct.unpack_from('<H', r, 18)[0]); put16(m, NSS, c.sp + 2, comseg)
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m); keep = []
        env = c_video((comseg, stub_off), keep); st_i = ctypes.c_int(99)
        st = fs[name](ram, SIZE, ctypes.byref(c), ctypes.byref(env), ctypes.byref(st_i))
        check(STATUS.get(st) == 'OK', f'native C status {name} {label}: {STATUS.get(st)}')
        want = struct.unpack_from('<14H', r)
        for k, (reg, _) in enumerate(Cpu._fields_):
            if reg in ('cs', 'ip'):
                continue
            check(getattr(c, reg) == want[k], f'native CPU {name} {label}/{reg}: C {getattr(c, reg):04x} DOS {want[k]:04x}')
        check((c.cs, c.ip) == (comseg, want[9]), f'native return {name} {label}')
        at = 28
        for seg, off, length in regions:
            got = bytes(ram[physical(seg, off):physical(seg, off) + length])
            check(got == r[at:at + length], f'native {name} {label} region {seg:04x}:{off:04x}')
            at += length
    return dict(cases=len(rows), regions=[f'{s:04X}:{o:04X}+{n:X}' for s, o, n in regions], result_sha256=sha256(data).hexdigest(),
                image_bin_sha256=sha256(image_reloc).hexdigest())


def main():
    p = argparse.ArgumentParser()
    for flag in ('--compile-only', '--unicorn', '--native'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--output', type=Path); a = p.parse_args()
    with tempfile.TemporaryDirectory(prefix='video-') as tmp:
        fs = library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        image, sites = load_image()
        spans = source(image, sites)
        report = dict(targets={k: dict(span=f"0464:[{ROUTINES[k]['entry']:04X},{ROUTINES[k]['end']:04X})", **v) for k, v in spans.items()},
                      image_sha256=IMAGE_DIGEST, relocations=len(sites), load_base=f'{BASE:04X}')
        if a.unicorn:
            report['unicorn'] = differential(relocate(image, sites, BASE), spans, fs)
        if a.native:
            report['native'] = native(image, sites, spans, fs)
    if a.output:
        a.output.parent.mkdir(parents=True, exist_ok=True); a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    main()
