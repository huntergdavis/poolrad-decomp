"""Native DOSBox-X corroboration for RES_00BA_1509; used by res00ba_sidecar_remove_1509_cpu_audit.py --native.

The world is the 0BF5 native's (res0709_msgbox_0bf5_native): the loader-expanded START.EXE image relocated to 3000h and re-read from IMAGE.BIN before every case,
machine=ega in mode 0Dh with the four planes pre-filled through the real ports, and INT 16h hooked to replay the case's key script. Each case's DS segment, heap,
character, font and PSP regions are built on the C side and read by the oracle program from CASES.BIN, so both runs start from the same bytes. The frame is
(return, record far pointer, character far pointer) and the body is entered at 3000h+00BAh:1509h. Compared after the return, with no masks: the 14 CPU words, the
stack window (minus the 10 DOS-side bytes under each INT 16h call), every loaded region (DS 64 KiB, heap, character, font, PSP), the key count and all four planes.
Not compared: OUT events, the EGA indexed registers. RunError cases are not run natively (FreeMem's RunError ends the program): the Unicorn differential covers them.
The host's packaged DOSBox-X 2026.01.02 is used, the same release as pinned 5a9c7c53."""
# NOT_A_ROW_CHECKER: native helper imported by res00ba_sidecar_remove_1509_cpu_audit.py --native; the audit registers the row
import ctypes, os, struct, subprocess
from hashlib import sha256
import res0709_msgbox_0bf5_native as N
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, physical, put16, check

NBASE, NSS, TOP, LOW, WINDOW, NPSP, NFONT, FONTOFF = N.NBASE, N.NSS, N.TOP, N.LOW, N.WINDOW, N.NPSP, N.NFONT, N.FONTOFF
NHEAP, NHEAPLEN, NCHAR = 0x4D00, 0x3000, 0x5800
PATTERN = N.PATTERN
A = None                         # bound to the RUNNING audit module in native()


def node(k):
    return (NHEAP + 1 + 4 * k, 0x0000)


def specs():
    K = A.K; out = []
    for mode in (1, 3):
        out.append((f'n-null-mode{mode}', dict(mode=mode, target=None)))
        out.append((f'n-absent-mode{mode}', dict(mode=mode, nodes=3, target_at=node(7))))
    for t, label in enumerate(('head', 'middle', 'tail')):
        out.append((f'n-remove-{label}', dict(nodes=3, target=t)))
    out += [('n-remove-only', dict(nodes=1, target=0)), ('n-remove-five', dict(nodes=5, order=[3, 0, 4, 1, 2], target=1)),
            ('n-absent-empty', dict(nodes=0, target_at=node(2))), ('n-free-top-block', dict(nodes=3, places=[node(0), node(1), (NHEAP + 0x1FC, 0)], target=2)),
            ('n-same-seg-head', dict(places=[(node(0)[0], 0), (node(0)[0], 8)], target=1)),
            ('n-same-seg-walk', dict(places=[(node(1)[0], 0), (node(0)[0], 0), (node(0)[0], 8)], target=2)),
            ('n-absent-two-keys', dict(nodes=2, target_at=node(6), keys=(K['a'], K['x'])))]
    out += [(f'n-flags{fl:04x}', dict(flags=fl, nodes=3, target=1)) for fl in A.M.FLAGS + tuple(x for x in A.M.DIRTY_FLAGS if not x & 0x200)]
    out += [(f'n-flags{fl:04x}-null', dict(flags=fl, target=None)) for fl in (0x0002, 0x08D7)]
    return out


def world(image_reloc, o, comseg, ret_off):
    """The C side's memory and CPU: the 0BF5 native's DS fields, the heap globals at DS:4400, the records, the character's list and the frame."""
    m = bytearray([0x5A]) * SIZE
    b = NBASE * 16; m[b:b + len(image_reloc)] = image_reloc
    ds = NBASE + A.SEG['data']
    m[physical(ds, 0x4A1E)] = o.get('mode', 1)
    put16(m, ds, 0x4A16, 0xA000); put16(m, ds, 0x4A18, 0xA200)
    put16(m, ds, 0x5E20, FONTOFF); put16(m, ds, 0x5E22, NFONT)
    for j, v in enumerate((1, 2, 4, 8)):
        m[physical(ds, 0x25C9 + j)] = v
    for j, v in enumerate((0, 0x5555, 0xAAAA, 0xFFFF)):
        put16(m, ds, 0x25CD + 2 * j, v)
    for j in range(256):
        m[physical(ds, 0x25D5 + j)] = j % 4
    for j in range(8):
        put16(m, ds, 0x25E5 + 2 * j, 0xC000 >> (2 * j))
    for j in range(2048):
        m[physical(NFONT, FONTOFF + j)] = ((j * 37 + (j >> 3) * 13) ^ 0xA6) & 255
    m[physical(ds, 0x5372)] = 1; m[physical(ds, 0x5373)] = 0; put16(m, ds, 0x5374, 0); put16(m, ds, 0x2612, 1); m[physical(ds, 0x4A1C)] = 3
    m[physical(ds, 0x49F7)] = 0; m[physical(ds, 0x8501)] = 0; m[physical(ds, 0x8502)] = 0
    put16(m, ds, 0x4422, NPSP); m[physical(NPSP, 0x80)] = 0; m[physical(NPSP, 0x81)] = 0x0D
    org, top, end = (NHEAP, 0), (NHEAP + 0x200, 0), (NHEAP + 0x280, 0)
    for k, (seg, off) in enumerate(((org[0], org[1]), (top[0], top[1]), (end[0], end[1]), (top[0], top[1]))):
        put16(m, ds, 0x4400 + 4 * k, off); put16(m, ds, 0x4402 + 4 * k, seg)
    put16(m, ds, 0x4410, 0); put16(m, ds, 0x4412, 0)
    places = o.get('places') or [node(k) for k in range(o.get('nodes', 3))]
    for k, (seg, off) in enumerate(places):
        for j in range(0x40):
            m[physical(seg, off + j)] = (0x30 + 3 * j + k) & 0xFF
    order = o.get('order', list(range(len(places))))
    for i, k in enumerate(order):
        nxt = places[order[i + 1]] if i + 1 < len(order) else (0, 0)
        seg, off = places[k]; put16(m, seg, off + 0x2A, nxt[1]); put16(m, seg, off + 0x2C, nxt[0])
    head = places[order[0]] if order else (0, 0)
    put16(m, NCHAR, 0x0100 + 0xC8, head[1]); put16(m, NCHAR, 0x0100 + 0xCA, head[0])
    target = o.get('target_at') or (places[o['target']] if o.get('target') is not None else (0, 0))
    sp = TOP - 12
    for k, w in enumerate((ret_off, comseg, target[1], target[0], 0x0100, NCHAR)):
        put16(m, NSS, sp + 2 * k, w)
    c = Cpu(0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, sp, NBASE + A.RES, A.ENTRY, ds, 0x1111, NSS, o.get('flags', 2))
    return m, c, target


def loaded(ds):
    return [(ds, 0, 0x10000), (NHEAP, 0, NHEAPLEN), (NCHAR, 0, 0x200), (NFONT, FONTOFF, 0x800), (NPSP, 0, 0x100)]


def c_run(f, lib, m, c, keys, kb_sps):
    R, Q = A.R, A.Q
    keep = []
    q, env = Q.c_env(dict(table={8: (0x0123, 0xF000)}, in61=0xB0, time_start=4736147, time_step=7), keep, stub_at=None)
    ke = R.KeyEnv(q, R.keyboard(env, keys, keep, kb_sps), None, 64)
    e = A.Env1509(A.M.Env0BF5(ke, 65536, 65536, 65536), 65536)
    actual = Cpu.from_buffer_copy(c); ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    ega = A.M.Ega(); lib.pr_ega_reset(ctypes.byref(ega))
    for p in range(4):
        ctypes.memset(ctypes.addressof(ega.plane[p]), PATTERN[p], 65536)
    ports = A.E0.Env(None, A.E0.OUT_FN(lambda *a: 0), A.E0.IN_FN()); keep.append(ports)
    bus = A.M.Bus(ram, ctypes.pointer(ega), ctypes.pointer(ports), A.G.WRITE_FN(lambda *a: None), None)
    d = A.Diag1509(); left = ctypes.c_size_t(A.BUDGET)
    status = f(ctypes.byref(bus), SIZE, ctypes.byref(actual), ctypes.byref(left), ctypes.byref(e), ctypes.byref(d))
    return A.STATUS.get(status), actual, bytes(ram), env, bytes(ega)


def program(rows, regions, outs, case_bytes):
    """The oracle COM: per case, fill and reload the image, read the case's regions from CASES.BIN, fill the planes, hook INT 16h, push the frame, run, dump."""
    writes = []
    for seg, off, n in outs:
        for k in range(0, n, 0x4000):
            writes += [f'mov ax,{seg}', 'mov ds,ax', 'mov bx,word ptr cs:[handle]', f'mov dx,{off + k}', f'mov cx,{min(0x4000, n - k)}', 'mov ah,0x40', 'int 0x21']
    for plane in range(4):
        writes += ['mov dx,0x3ce', 'mov ax,0x0005', 'out dx,ax', f'mov ax,{(plane << 8) | 4}', 'out dx,ax']
        for off in (0, 0x8000):
            writes += ['mov ax,0xa000', 'mov ds,ax', f'mov si,{off}', 'mov ax,0x7000', 'mov es,ax', 'xor di,di', 'mov cx,0x8000', 'cld', 'rep movsb',
                       'mov ax,0x7000', 'mov ds,ax', 'mov bx,word ptr cs:[handle]', 'xor dx,dx', 'mov cx,0x8000', 'mov ah,0x40', 'int 0x21']
    writes += ['push cs', 'pop ds', 'mov bx,word ptr [handle]', 'mov dx,offset kidx', 'mov cx,2', 'mov ah,0x40', 'int 0x21']
    fill = []
    for chunk in range(2):
        fill += [f'mov ax,{NBASE + 0x1000 * chunk}', 'mov es,ax', 'xor di,di', 'mov cx,0x8000', 'mov ax,0x5a5a', 'rep stosw']
    load = []
    for k, start in enumerate(range(0, A.IMAGE_LEN, 0x8000)):
        load += [f'mov ax,{NBASE + 0x800 * k}', 'mov ds,ax', 'mov bx,word ptr cs:[image]', 'xor dx,dx',
                 f'mov cx,{min(0x8000, A.IMAGE_LEN - start)}', 'mov ah,0x3f', 'int 0x21', 'jc failure']
    regs_in = []
    for seg, off, n in regions:
        for k in range(0, n, 0x8000):
            regs_in += [f'mov ax,{seg}', 'mov ds,ax', 'mov bx,word ptr cs:[cases]', f'mov dx,{off + k}', f'mov cx,{min(0x8000, n - k)}', 'mov ah,0x3f', 'int 0x21', 'jc failure']
    planes = ['mov dx,0x3ce', 'mov ax,0x0000', 'out dx,ax', 'mov ax,0x0001', 'out dx,ax', 'mov ax,0x0003', 'out dx,ax', 'mov ax,0x0004', 'out dx,ax', 'mov ax,0x0005', 'out dx,ax',
              'mov ax,0xff08', 'out dx,ax', 'mov ax,0xa000', 'mov es,ax']
    for plane in range(4):
        planes += ['mov dx,0x3c4', f'mov ax,{((1 << plane) << 8) | 2}', 'out dx,ax', 'xor di,di', f'mov ax,{PATTERN[plane] * 257}', 'mov cx,0x8000', 'rep stosw']
    planes += ['mov dx,0x3c4', 'mov ax,0x0f02', 'out dx,ax']
    regs = (0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777)
    ds_seg = NBASE + A.SEG['data']
    t = ['.code16', '.intel_syntax noprefix', '.global _start', '_start: jmp main', '.org 0x200', 'main:',
         'mov ax,cs', 'mov ds,ax', 'mov ss,ax', 'mov sp,0xfff0', 'cld', 'mov ax,0x000d', 'int 0x10',
         'mov dx,offset outname', 'xor cx,cx', 'mov ah,0x3c', 'int 0x21', 'jc failure', 'mov word ptr [handle],ax',
         'mov dx,offset casesname', 'mov ax,0x3d00', 'int 0x21', 'jc failure', 'mov word ptr [cases],ax',
         'mov word ptr [header],cs', 'mov word ptr [header+2],offset returned', f'mov word ptr [header+4],{len(rows)}', 'mov word ptr [header+6],0',
         'mov bx,word ptr [handle]', 'mov dx,offset header', 'mov cx,8', 'mov ah,0x40', 'int 0x21',
         'xor ax,ax', 'mov es,ax', 'mov ax,word ptr es:[0x58]', 'mov word ptr [old16],ax', 'mov ax,word ptr es:[0x5a]', 'mov word ptr [old16+2],ax',
         'again:', 'cld'] + fill + [
         'push cs', 'pop ds', 'mov dx,offset imagename', 'mov ax,0x3d00', 'int 0x21', 'jc failure', 'mov word ptr cs:[image],ax'] + load + [
         'push cs', 'pop ds', 'mov bx,word ptr [image]', 'mov ah,0x3e', 'int 0x21'] + regs_in + [
         'push cs', 'pop ds',
         'mov ax,word ptr [index]', 'mov bx,12', 'mul bx', 'add ax,offset table', 'mov bx,ax',                           # bx = the row: flags, record off/seg, kcount, k0..k3
         'mov word ptr [kidx],0', 'mov ax,word ptr [bx+6]', 'mov word ptr [kcount],ax',
         'mov ax,word ptr [bx+8]', 'mov word ptr [keys],ax', 'mov ax,word ptr [bx+10]', 'mov word ptr [keys+2],ax'] + planes + [
         'xor ax,ax', 'mov es,ax', 'mov word ptr es:[0x58],offset int16', 'mov word ptr es:[0x5a],cs',
         f'mov ax,{NSS}', 'mov es,ax', f'mov di,{LOW}', f'mov cx,{(WINDOW - 12) // 2}', 'mov ax,0x5a5a', 'rep stosw',
         'cli', f'mov ax,{NSS}', 'mov ss,ax', f'mov sp,{TOP}',
         f'push {NCHAR}', 'push 0x100', 'push word ptr cs:[bx+4]', 'push word ptr cs:[bx+2]',                            # character, then the record far pointer
         'push cs', 'push offset returned', 'push word ptr cs:[bx]',
         f'mov ax,{ds_seg}', 'mov ds,ax', 'mov ax,0x1111', 'mov es,ax']
    for r, v in zip('ax bx cx dx si di bp'.split(), regs):
        t.append(f'mov {r},{v:#x}')
    t += ['popf', f'ljmp {NBASE + A.RES},{A.ENTRY}', 'returned:']
    t += [f'mov word ptr cs:[out+{2 * n}],{r}' for n, r in enumerate('ax bx cx dx si di bp sp cs'.split())]
    t += ['mov word ptr cs:[out+18],offset returned', 'mov word ptr cs:[out+20],ds', 'mov word ptr cs:[out+22],es', 'mov word ptr cs:[out+24],ss',
          'mov ax,cs', 'mov ss,ax', 'mov sp,0xfff0', 'pushf', 'pop ax', 'mov word ptr cs:[out+26],ax',
          'mov ax,cs', 'mov ds,ax', 'xor ax,ax', 'mov es,ax', 'mov ax,word ptr [old16]', 'mov word ptr es:[0x58],ax', 'mov ax,word ptr [old16+2]', 'mov word ptr es:[0x5a],ax',
          'mov bx,word ptr [handle]', 'mov dx,offset out', 'mov cx,28', 'mov ah,0x40', 'int 0x21'] + writes + [
          'push cs', 'pop ds', 'sti', 'inc word ptr [index]', f'cmp word ptr [index],{len(rows)}', 'jae done', 'jmp again', 'done:',
          'mov bx,word ptr [cases]', 'mov ah,0x3e', 'int 0x21',
          'mov bx,word ptr [handle]', 'mov ah,0x3e', 'int 0x21', 'mov ax,3', 'int 0x10', 'mov ax,0x4c00', 'int 0x21', 'failure:', 'mov ax,0x4c01', 'int 0x21']
    t += ['int16:', 'cmp ah,1', 'je kb_peek', 'cmp ah,0', 'je kb_take', 'jmp dword ptr cs:[old16]',
          'kb_peek:', 'push bp', 'mov bp,sp', 'push bx', 'mov bx,word ptr cs:[kidx]', 'cmp bx,word ptr cs:[kcount]', 'jae peek_empty', 'shl bx,1',
          'mov ax,word ptr cs:[keys+bx]', 'and word ptr [bp+6],0xffbf', 'jmp peek_done',
          'peek_empty:', 'or word ptr [bp+6],0x40',
          'peek_done:', 'pop bx', 'pop bp', 'iret',
          'kb_take:', 'push bx', 'mov bx,word ptr cs:[kidx]', 'shl bx,1', 'mov ax,word ptr cs:[keys+bx]', 'inc word ptr cs:[kidx]', 'pop bx', 'iret',
          'outname: .asciz "RESULT.BIN"', 'imagename: .asciz "IMAGE.BIN"', 'casesname: .asciz "CASES.BIN"', 'handle: .word 0', 'image: .word 0', 'cases: .word 0',
          'index: .word 0', 'kidx: .word 0', 'kcount: .word 0', '.align 2', 'keys: .word 0,0', 'header: .space 8', 'out: .space 28', 'old16: .word 0,0',
          '.align 2', 'table:'] + ['.word ' + ','.join(str(v) for v in row) for row in rows]
    return t


def native(image, sites, body_ips, f, lib):
    global A
    import sys
    A = sys.modules['res00ba_sidecar_remove_1509_cpu_audit']
    image_reloc = A.Q.relocate(image, sites, NBASE)
    A.IMAGE_LEN = len(image_reloc)
    check(NBASE + (len(image_reloc) + 15) // 16 <= NBASE + A.SEG['data'] + 0x1000 <= NHEAP, 'the image and DS end below the native heap')
    sp_specs = specs()
    directory = ROOT / 'resources/00ba-1509/native'; directory.mkdir(parents=True, exist_ok=True)
    (directory / 'IMAGE.BIN').write_bytes(image_reloc)
    ds = NBASE + A.SEG['data']; regions = loaded(ds)
    outs = [(NSS, LOW, WINDOW)] + regions
    # the COM's return label must be known before the C side builds its frame: assemble once with a placeholder table, then again with the rows
    rows = [(o.get('flags', 2), 0, 0, len(o.get('keys', (A.K['a'],))), 0, 0) for _, o in sp_specs]
    for _ in range(2):
        t = program(rows, regions, outs, None)
        (directory / 'oracle.S').write_text('\n'.join(t) + '\n')
        subprocess.run(['as', '--32', str(directory / 'oracle.S'), '-o', str(directory / 'oracle.o')], check=True)
        subprocess.run(['ld', '-m', 'elf_i386', '-Ttext', '0x100', '--oformat', 'binary', str(directory / 'oracle.o'), '-o', str(directory / 'ORACLE.COM')], check=True)
        syms = subprocess.run(['nm', str(directory / 'oracle.o')], capture_output=True, text=True, check=True).stdout
        ret_off = 0x100 + next(int(line.split()[0], 16) for line in syms.splitlines() if line.endswith(' returned'))
        blobs, rows, worlds = [], [], []
        for name, o in sp_specs:
            m, c, target = world(image_reloc, o, 0, ret_off)
            keys = list(o.get('keys', (A.K['a'],))); check(len(keys) <= 2, 'at most two scripted keys')
            rows.append((o.get('flags', 2), target[1], target[0], len(keys), *(keys + [0] * (2 - len(keys)))))
            blobs.append(b''.join(bytes(m[physical(s, off):physical(s, off) + n]) for s, off, n in regions)); worlds.append((m, c))
    check((directory / 'ORACLE.COM').stat().st_size + 0x100 < 0xF000, 'COM reaches its own stack')
    (directory / 'CASES.BIN').write_bytes(b''.join(blobs))
    (directory / 'dosbox.conf').write_text('[sdl]\noutput=surface\n[dosbox]\nmachine=ega\nmemsize=16\n[cpu]\ncore=normal\ncputype=386\ncycles=max\n'
                                          '[mixer]\nnosound=true\n[midi]\nmididevice=none\n[autoexec]\nmount c '
                                          + str(directory.resolve()) + '\nc:\nORACLE.COM\nexit\n')
    out = directory / 'RESULT.BIN'
    if out.exists():
        out.unlink()
    binary = '/usr/bin/dosbox-x'
    with (directory / 'dosbox.log').open('w') as log:
        subprocess.run([binary, '-conf', str(directory / 'dosbox.conf'), '-fastlaunch', '-exit', '-noconsole'],
                       env=dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy'), stdout=log, stderr=subprocess.STDOUT, check=True, timeout=1800)
    data = out.read_bytes(); record = 28 + sum(n for _, _, n in outs) + 4 * 65536 + 2
    check(len(data) == 8 + len(rows) * record, f'native length {len(data)} expected {8 + len(rows) * record}')
    comseg, ret_seen, count, _ = struct.unpack_from('<4H', data); check(count == len(rows) and ret_seen == ret_off, 'native header')
    compared = changed = touched = 0
    for n, (name, o) in enumerate(sp_specs):
        rec = data[8 + n * record:8 + (n + 1) * record]
        m, c = worlds[n]; put16(m, NSS, c.sp + 2, comseg)
        kb_sps = []
        status, cpu, ram, env, ega = c_run(f, lib, m, c, list(o.get('keys', (A.K['a'],))), kb_sps)
        check(status == 'RETURNED', f'native C stop {name}: {status}')
        taken = struct.unpack_from('<H', rec, record - 2)[0]; want_taken = sum(1 for e in env.log if e[0] == '16' and e[1] == 0)
        check(taken == want_taken, f'native keys taken {name}: DOS {taken}, C {want_taken}')
        for (reg, _), want in zip(Cpu._fields_, struct.unpack_from('<14H', rec)):
            check(getattr(cpu, reg) == want, f'native CPU {name}/{reg}: C {getattr(cpu, reg):04x} DOS {want:04x}')
        frame_mask = {physical(NSS, (sp - k) & 0xFFFF) for sp in kb_sps for k in range(1, 11)}
        at = 28
        for seg, off, length in outs:
            for k in range(length):
                p = physical(seg, (off + k) & 0xFFFF)
                if p in frame_mask:
                    continue
                check(ram[p] == rec[at + k], f'native {name} at {seg:04x}:{(off + k) & 0xFFFF:04x}: C {ram[p]:02x} DOS {rec[at + k]:02x}')
            at += length; compared += length
        got = rec[at:at + 4 * 65536]
        if got != ega[:4 * 65536]:
            diff = [(hex(i), ega[i], got[i]) for i in range(4 * 65536) if ega[i] != got[i]]
            raise ValueError(f'native {name}: planes differ at {diff[:6]} ({len(diff)} bytes)')
        compared += 4 * 65536
        delta = sum(1 for p in range(4) for b in got[p * 65536:(p + 1) * 65536] if b != PATTERN[p]); changed += delta; touched += delta > 0
    probe = subprocess.run([binary, '--version'], capture_output=True, text=True, env=dict(os.environ, SDL_VIDEODRIVER='dummy'))
    version = next(line.split(',')[0].strip() for line in (probe.stdout + probe.stderr).splitlines() if 'DOSBox-X version' in line)
    return dict(cases=len(rows), case_names=[n for n, _ in sp_specs], result_sha256=sha256(data).hexdigest(), compared_units=compared, plane_bytes_changed_from_fill=changed,
                cases_that_drew=touched, regions=[f'{s:04X}:{o:04X}+{n:X}' for s, o, n in outs], machine='ega', video_mode='0Dh', image_bin_sha256=sha256(image_reloc).hexdigest(),
                dosbox_x=dict(path=binary, sha256=sha256(open(binary, 'rb').read()).hexdigest(), version=version,
                              pinned_source='5a9c7c53481c2e933948039c33256776653f4faf', pinned_build_used=False))
