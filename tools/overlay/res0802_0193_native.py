"""Native DOSBox-X corroboration for RES_0802_0193 (the status-line prompt); used by res0802_0193_cpu_audit.py.

The loader-expanded START.EXE image, relocated to segment 3000h with the loader's own table, is read from IMAGE.BIN before every case, so each case starts from the
original bytes. The cases use plain keys only (no ctrl-S driver path, no QUIT, no Ctrl-Break), so the declared environment is just the keyboard: INT 16h is a hook in
the oracle program that replays the case's key script (AH=00h takes the next word, AH=01h reports it and sets ZF in the IRET frame when the script is exhausted).
TWO native runs, each pinning a different claim:

  RUN A, stub level (declared substitution): INT 10h is the real-byte logging stub of the 0040 audit (log segment 2800h). The C model composed with StrStore, 0175, 0105,
  0040, Dos.Intr and ReadKey is compared with the oracle on all 14 CPU words, the stack window (minus the 10 DOS-side bytes under each INT 16h call: its 3-word frame, the hook saved BP and BX), DS:4A00..4A1F, the
  hotkey state, the log and the number of keys taken. Pins: the original closure issues the INT 10h blocks and consumes the keys the C model says. The stub is ours on both sides.

  RUN B, BIOS live: INT 10h is DOSBox-X's REAL BIOS in text mode 03h. INT 16h is the same declared hook, which also SNAPSHOTS the eight text pages (B800:0000..7FFF) at its
  first call, i.e. while the prompt is waiting. After the return the final pages and the BIOS cursor words are read back. Both the snapshot (blank row, then the message) and the
  final screen (row 24 blanked again) are compared with an INDEPENDENT Python text-mode model driven only by the contract. Pins: the prompt shows the message on row 24 while it
  waits and blanks row 24 afterwards, on a mechanism that is not ours. Text mode only; one row; pages 0..7."""
import ctypes, os, struct, subprocess
from hashlib import sha256
import res0802_0193_cpu_audit as A
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, physical, put16, check

NBASE, NSS, TOP, LOW, WINDOW, NLOG, NMSG, NPSP, NSNAP = 0x3000, 0x6000, 0xF000, 0xEC00, 0x0400, 0x2800, 0x2E00, 0x2F00, 0x2200
LOGBYTES = 0xB00
DATA_SEG = NBASE + A.SEG['data']
ROW = 24
K = A.K


def specs_a():
    out = []
    for ega in (0, 1, 2, 0xFF):
        for n, msg in enumerate((b'', b'A', b'Insert disk 2', b'x' * 40, b'y' * 60)):
            out.append((f'a-ega{ega:02x}-m{n}', dict(msg=msg, ega=ega, attr=0x0E if n % 2 else 0xFF if n == 4 else 0x00, keys=(K['a'],))))
    for label, keys in (('drain3', (K['a'], K['x'], K['a'], K['x'])), ('drainext', (K['a'], K['f1'], K['a'])), ('ext', (K['f1'],)), ('zero', (K['zero'],)), ('zerothen', (K['zero'], K['a']))):
        out.append((f'a-keys-{label}', dict(msg=b'Press a key', ega=1, attr=0x0E, keys=keys)))
    for pa, pb in ((0, 0), (7, 3), (0xFF, 0)):
        out.append((f'a-pages{pa:02x}{pb:02x}', dict(msg=b'Pg', ega=1, page_a=pa, page_b=pb, keys=(K['a'],))))
    out += [(f'a-flags{fl:04x}', dict(msg=b'Fl', ega=1, flags=fl, keys=(K['a'],))) for fl in A.FLAGS + tuple(x for x in A.DIRTY_FLAGS if not x & 0x200)]
    return out


def specs_b():
    out = []
    for ega in (0, 1, 2):
        for n, (msg, attr) in enumerate(((b'Insert disk 2', 0x0E), (b'Hi', 0x4F), (b'x' * 40, 0x1F), (b'', 0x07), (b'y' * 60, 0x31))):
            out.append((f'b-ega{ega}-m{n}', dict(msg=msg, ega=ega, attr=attr, page_a=5, page_b=2, keys=(K['a'],))))
    for pa, pb in ((0, 0), (0, 3), (7, 0), (4, 4), (1, 6)):
        out.append((f'b-pages{pa}{pb}', dict(msg=b'Pages', ega=1, attr=0x2A, page_a=pa, page_b=pb, keys=(K['a'],))))
    out += [('b-drain', dict(msg=b'Drain', ega=0, attr=0x0E, page_a=5, page_b=2, keys=(K['a'], K['x'], K['a']))), ('b-ext', dict(msg=b'Ext', ega=1, attr=0x0E, page_a=5, page_b=2, keys=(K['f1'],)))]
    return out


def split_blocks(o):
    """(blank, message, blank) INT 10h blocks of the contract: three lists, so the snapshot (before the key) and the final screen can be modelled."""
    allb = A.blocks_for(o)
    per = 4 if o.get('ega', 0) == 1 else 2
    n = min(len(o.get('msg', b'Insert disk')), 40)
    return allb[:per], allb[per:per + per * n], allb[per + per * n:]


def apply_blocks(cells, cursor, blocks):
    for ax, bx, cx, dx, *_ in blocks:
        ah, al, bh, bl = ax >> 8, ax & 0xFF, bx >> 8, bx & 0xFF
        if ah == 0x02:
            cursor[bh] = (dx >> 8, dx & 0xFF)
        elif ah == 0x09:
            row, col = cursor[bh]
            for i in range(cx):
                cells[bh * 0x800 + row * 80 + col + i] = (bl << 8) | al


def text_models(o):
    cells = [0x0720] * (8 * 0x800); cursor = {p: (0, 0) for p in range(8)}
    b1, bm, b2 = split_blocks(o)
    apply_blocks(cells, cursor, b1); apply_blocks(cells, cursor, bm)
    snap = (list(cells), dict(cursor))
    apply_blocks(cells, cursor, b2)
    return snap, (cells, cursor)


def c_run(image_reloc, o, comseg, stub_off, stub, ret_off, kb_sps):
    m = bytearray([0x5A]) * SIZE
    b = NBASE * 16; m[b:b + len(image_reloc)] = image_reloc
    ds = DATA_SEG
    tab = [(0xA0 + k * 7 + 3) & 0xFF for k in range(256)]
    for k in range(256):
        m[physical(ds, 0x25D5 + k)] = tab[k]
    m[physical(ds, 0x4A1A)] = o.get('page_a', 7); m[physical(ds, 0x4A1B)] = o.get('page_b', 3); m[physical(ds, 0x4A1E)] = o.get('ega', 0)
    for k, v in enumerate((0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x8888, 0x9999, 0x0246)):
        put16(m, ds, 0x4A02 + 2 * k, v)
    m[physical(ds, 0x5372)] = 1; m[physical(ds, 0x5373)] = 0; put16(m, ds, 0x5374, 0); put16(m, ds, 0x2612, 1); m[physical(ds, 0x4A1C)] = 3
    m[physical(ds, 0x49F7)] = 0; m[physical(ds, 0x8501)] = 0; m[physical(ds, 0x8502)] = 0
    put16(m, ds, 0x4422, NPSP); m[physical(NPSP, 0x80)] = 0; m[physical(NPSP, 0x81)] = 0x0D
    put16(m, 0, 0x40, stub_off); put16(m, 0, 0x42, comseg)
    s = physical(comseg, stub_off); m[s:s + len(stub)] = stub
    for k in range(LOGBYTES):
        m[physical(NLOG, k)] = 0x5A
    put16(m, NLOG, 0, 0)
    msg = o.get('msg', b'Insert disk')
    m[physical(NMSG, 0x100)] = len(msg) & 0xFF
    for k, ch in enumerate(msg[:255]):
        m[physical(NMSG, 0x101 + k)] = ch
    sp = TOP - 10
    put16(m, NSS, sp, ret_off); put16(m, NSS, sp + 2, comseg)
    put16(m, NSS, sp + 4, 0x0100); put16(m, NSS, sp + 6, NMSG); put16(m, NSS, sp + 8, (0xAB << 8) | o.get('attr', 0x0E))
    c = Cpu(0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, sp, NBASE + A.SEG['text'], A.ENTRY, ds, 0x1111, NSS, o.get('flags', 2))
    spec = dict(table={8: (0x0123, 0xF000)}, in61=0xB0, time_start=4736147, time_step=7)
    stop, status, actual, ram, env, d = A.run_c(A.F, bytes(m), c, spec, list(o.get('keys', (K['a'],))), stub_at=(comseg, stub_off), log_seg=NLOG, kb_sps=kb_sps)
    return stop, status, actual, ram, env


def program(image_reloc, rows, live, stub):
    ds_seg = DATA_SEG
    regions = ([(0xB800, 0x0000, 0x8000), (0x0040, 0x0050, 0x10), (NSNAP, 0, 0x8000)] if live else
               [(NSS, LOW, WINDOW), (ds_seg, 0x4A00, 0x20), (ds_seg, 0x5370, 0x08), (ds_seg, 0x2610, 0x04), (ds_seg, 0x49F6, 0x02), (ds_seg, 0x8500, 0x04), (NLOG, 0, LOGBYTES)])
    writes = []
    for seg, off, n in regions:
        for k in range(0, n, 0x4000):
            writes += [f'mov ax,{seg}', 'mov ds,ax', 'mov bx,word ptr cs:[handle]', f'mov dx,{off + k}', f'mov cx,{min(0x4000, n - k)}', 'mov ah,0x40', 'int 0x21']
    writes += ['push cs', 'pop ds', 'mov bx,word ptr [handle]', 'mov dx,offset kidx', 'mov cx,2', 'mov ah,0x40', 'int 0x21']      # the number of keys taken
    fill = []
    for chunk in range(2):
        fill += [f'mov ax,{NBASE + 0x1000 * chunk}', 'mov es,ax', 'xor di,di', 'mov cx,0x8000', 'mov ax,0x5a5a', 'rep stosw']
    load = []
    for k, start in enumerate(range(0, len(image_reloc), 0x8000)):
        load += [f'mov ax,{NBASE + 0x800 * k}', 'mov ds,ax', 'mov bx,word ptr cs:[image]', 'xor dx,dx',
                 f'mov cx,{min(0x8000, len(image_reloc) - start)}', 'mov ah,0x3f', 'int 0x21', 'jc failure']
    regs = (0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x8888, 0x9999, 0x0246)
    t = ['.code16', '.intel_syntax noprefix', '.global _start', '_start: jmp main', '.org 0x200', 'main:',
         'mov ax,cs', 'mov ds,ax', 'mov ss,ax', 'mov sp,0xfff0', 'cld',
         'mov dx,offset outname', 'xor cx,cx', 'mov ah,0x3c', 'int 0x21', 'jc failure', 'mov word ptr [handle],ax',
         'mov word ptr [header],cs', 'mov word ptr [header+2],offset stub10', f'mov word ptr [header+4],{len(rows)}', 'mov word ptr [header+6],offset returned',
         'mov bx,word ptr [handle]', 'mov dx,offset header', 'mov cx,8', 'mov ah,0x40', 'int 0x21',
         'xor ax,ax', 'mov es,ax', 'mov ax,word ptr es:[0x40]', 'mov word ptr [old10],ax', 'mov ax,word ptr es:[0x42]', 'mov word ptr [old10+2],ax',
         'mov ax,word ptr es:[0x58]', 'mov word ptr [old16],ax', 'mov ax,word ptr es:[0x5a]', 'mov word ptr [old16+2],ax',
         f'mov word ptr [stub10+4],{NLOG}',
         'again:', 'cld'] + fill + [
         'push cs', 'pop ds', 'mov dx,offset imagename', 'mov ax,0x3d00', 'int 0x21', 'jc failure', 'mov word ptr cs:[image],ax'] + load + [
         'push cs', 'pop ds', 'mov bx,word ptr [image]', 'mov ah,0x3e', 'int 0x21',
         'mov ax,word ptr [index]', 'mov bx,24', 'mul bx', 'add ax,offset table', 'mov bx,ax',                          # bx = the row (twelve words)
         f'mov ax,{ds_seg}', 'mov es,ax', 'mov di,0x25d5', 'mov si,offset attr_table', 'mov cx,256', 'rep movsb',
         'mov al,byte ptr cs:[bx+2]', 'mov byte ptr es:[0x4a1e],al', 'mov al,byte ptr cs:[bx+6]', 'mov byte ptr es:[0x4a1a],al',
         'mov al,byte ptr cs:[bx+8]', 'mov byte ptr es:[0x4a1b],al'] + [
         f'mov word ptr es:[{0x4A02 + 2 * k}],{v}' for k, v in enumerate(regs)] + [
         'mov byte ptr es:[0x5372],1', 'mov byte ptr es:[0x5373],0', 'mov word ptr es:[0x5374],0', 'mov word ptr es:[0x2612],1', 'mov byte ptr es:[0x4a1c],3',
         'mov byte ptr es:[0x49f7],0', 'mov byte ptr es:[0x8501],0', 'mov byte ptr es:[0x8502],0', f'mov word ptr es:[0x4422],{NPSP}',
         f'mov ax,{NPSP}', 'mov es,ax', 'mov byte ptr es:[0x80],0', 'mov byte ptr es:[0x81],0x0d',
         # the key script and the message
         'push cs', 'pop ds', 'mov word ptr [kidx],0', 'mov byte ptr [snapped],0', 'mov ax,word ptr [bx+12]', 'mov word ptr [kcount],ax',
         'mov ax,word ptr [bx+14]', 'mov word ptr [keys],ax', 'mov ax,word ptr [bx+16]', 'mov word ptr [keys+2],ax',
         'mov ax,word ptr [bx+18]', 'mov word ptr [keys+4],ax', 'mov ax,word ptr [bx+20]', 'mov word ptr [keys+6],ax',
         'mov ax,word ptr [bx+10]', f'mov cx,{SLOT}', 'mul cx', 'add ax,offset msgs', 'mov si,ax', f'mov ax,{NMSG}', 'mov es,ax', 'mov di,0x100',
         f'mov cx,{SLOT}', 'rep movsb',
         'xor ax,ax', 'mov es,ax', 'mov word ptr es:[0x58],offset int16', 'mov word ptr es:[0x5a],cs']
    if live:
        t += ['mov ax,3', 'int 0x10']
    else:
        t += ['mov word ptr es:[0x40],offset stub10', 'mov word ptr es:[0x42],cs',
              f'mov ax,{NLOG}', 'mov es,ax', 'xor di,di', f'mov cx,{LOGBYTES // 2}', 'mov ax,0x5a5a', 'rep stosw', 'mov word ptr es:[0],0']
    t += [f'mov ax,{NSS}', 'mov es,ax', f'mov di,{LOW}', f'mov cx,{WINDOW // 2}', 'mov ax,0x5a5a', 'rep stosw',
          'cli', f'mov ax,{NSS}', 'mov ss,ax', f'mov sp,{TOP}',
          'push word ptr cs:[bx+4]', f'push {NMSG}', 'push 0x100',                                                   # attribute, then the message far pointer
          'push cs', 'push offset returned', 'push word ptr cs:[bx]',
          f'mov ax,{ds_seg}', 'mov ds,ax', 'mov ax,0x1111', 'mov es,ax',
          'mov ax,0x1234', 'mov bx,0x3456', 'mov cx,0x5678', 'mov dx,0x789a', 'mov si,0x9abc', 'mov di,0xbcde', 'mov bp,0x7777',
          'popf', f'ljmp {NBASE + A.SEG["text"]},{A.ENTRY}', 'returned:']
    t += [f'mov word ptr cs:[out+{2 * n}],{r}' for n, r in enumerate('ax bx cx dx si di bp sp cs'.split())]
    t += ['mov word ptr cs:[out+18],offset returned', 'mov word ptr cs:[out+20],ds', 'mov word ptr cs:[out+22],es', 'mov word ptr cs:[out+24],ss',
          'mov ax,cs', 'mov ss,ax', 'mov sp,0xfff0', 'pushf', 'pop ax', 'mov word ptr cs:[out+26],ax',
          'mov ax,cs', 'mov ds,ax', 'xor ax,ax', 'mov es,ax', 'mov ax,word ptr [old10]', 'mov word ptr es:[0x40],ax', 'mov ax,word ptr [old10+2]', 'mov word ptr es:[0x42],ax',
          'mov ax,word ptr [old16]', 'mov word ptr es:[0x58],ax', 'mov ax,word ptr [old16+2]', 'mov word ptr es:[0x5a],ax',
          'mov bx,word ptr [handle]', 'mov dx,offset out', 'mov cx,28', 'mov ah,0x40', 'int 0x21'] + writes + [
          'push cs', 'pop ds', 'sti', 'inc word ptr [index]', f'cmp word ptr [index],{len(rows)}', 'jae done', 'jmp again', 'done:',
          'mov bx,word ptr [handle]', 'mov ah,0x3e', 'int 0x21', 'mov ax,0x4c00', 'int 0x21', 'failure:', 'mov ax,0x4c01', 'int 0x21']
    snapshot = []
    if live:
        snapshot = ['cmp byte ptr cs:[snapped],0', 'jne snapdone', 'push ax', 'push cx', 'push si', 'push di', 'push ds', 'push es', f'mov ax,0xb800', 'mov ds,ax',
                    'xor si,si', f'mov ax,{NSNAP}', 'mov es,ax', 'xor di,di', 'mov cx,0x4000', 'cld', 'rep movsw', 'pop es', 'pop ds', 'pop di', 'pop si', 'pop cx', 'pop ax',
                    'mov byte ptr cs:[snapped],1', 'snapdone:']
    t += ['int16:'] + snapshot + [
          'cmp ah,1', 'je kb_peek', 'cmp ah,0', 'je kb_take', 'jmp dword ptr cs:[old16]',
          'kb_peek:', 'push bp', 'mov bp,sp', 'push bx', 'mov bx,word ptr cs:[kidx]', 'cmp bx,word ptr cs:[kcount]', 'jae peek_empty', 'shl bx,1',
          'mov ax,word ptr cs:[keys+bx]', 'and word ptr [bp+6],0xffbf', 'jmp peek_done',
          'peek_empty:', 'or word ptr [bp+6],0x40',
          'peek_done:', 'pop bx', 'pop bp', 'iret',
          'kb_take:', 'push bx', 'mov bx,word ptr cs:[kidx]', 'shl bx,1', 'mov ax,word ptr cs:[keys+bx]', 'inc word ptr cs:[kidx]', 'pop bx', 'iret',
          'stub10:', '.byte ' + ','.join(str(x) for x in stub),
          'outname: .asciz "RESULT.BIN"', 'imagename: .asciz "IMAGE.BIN"', 'handle: .word 0', 'image: .word 0', 'index: .word 0', 'kidx: .word 0', 'kcount: .word 0',
          'snapped: .byte 0', '.align 2', 'keys: .word 0,0,0,0', 'header: .space 8', 'out: .space 28', 'old10: .word 0,0', 'old16: .word 0,0',
          'attr_table: .byte ' + ','.join(str(x) for x in [(0xA0 + k * 7 + 3) & 0xFF for k in range(256)]), '.align 2', 'msgs:']
    for msg in MESSAGES:
        slot = bytes([len(msg) & 0xFF]) + msg[:255]
        t.append('.byte ' + ','.join(str(x) for x in slot.ljust(SLOT, b'\0')))
    t += ['.align 2', 'table:'] + ['.word ' + ','.join(str(v) for v in row) for row in rows]
    return t, regions


SLOT = 260
MESSAGES = []


def native(image, sites, decoded, f, audit, stub):
    global A, MESSAGES
    A = audit; A.F = f
    stub = A.B.assemble_stub(0x0060, scratch=A.SCRATCH)
    image_reloc = A.Q.relocate(image, sites, NBASE)
    results = {}
    for live, label, specs in ((False, 'stub', specs_a()), (True, 'bios', specs_b())):
        directory = ROOT / f'resources/0802-0193/native-{label}'; directory.mkdir(parents=True, exist_ok=True)
        (directory / 'IMAGE.BIN').write_bytes(image_reloc)
        MESSAGES = []
        for _, o in specs:
            if o.get('msg', b'Insert disk') not in MESSAGES:
                MESSAGES.append(o.get('msg', b'Insert disk'))
        rows = []
        for _, o in specs:
            keys = list(o.get('keys', (K['a'],)))
            check(len(keys) <= 4, 'at most four scripted keys')
            rows.append((o.get('flags', 2), o.get('ega', 0), (0xAB << 8) | o.get('attr', 0x0E), o.get('page_a', 7), o.get('page_b', 3), MESSAGES.index(o.get('msg', b'Insert disk')),
                         len(keys), *(keys + [0] * (4 - len(keys))), 0))
        # row words: flags, ega, attr_w, page_a, page_b, msg_idx, kcount, k0..k3, pad  (twelve words)
        rows = [(r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7], r[8], r[9], r[10], 0) for r in rows]
        t, regions = program(image_reloc, rows, live, stub)
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
                           env=dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy'), stdout=log, stderr=subprocess.STDOUT, check=True, timeout=1800)
        data = out.read_bytes(); record = 28 + sum(n for _, _, n in regions) + 2
        check(len(data) == 8 + len(rows) * record, f'{label}: native length {len(data)} expected {8 + len(rows) * record}')
        comseg, stub_off, count, ret_off = struct.unpack_from('<4H', data); check(count == len(rows), 'native count')
        compared = 0; nonblank_total = 0
        for n, (name, o) in enumerate(specs):
            rec = data[8 + n * record:8 + (n + 1) * record]
            taken = struct.unpack_from('<H', rec, record - 2)[0]
            if not live:
                kb_sps = []
                stop, status, c, ram, env = c_run(image_reloc, o, comseg, stub_off, stub, ret_off, kb_sps)
                check(stop == 'RETURN', f'native C stop {name}: {stop}')
                check(taken == sum(1 for e in env.log if e[0] == '16' and e[1] == 0), f'native keys taken {name}: DOS {taken}, C {sum(1 for e in env.log if e[0] == "16" and e[1] == 0)}')
                for (reg, _), want in zip(Cpu._fields_, struct.unpack_from('<14H', rec)):
                    check(getattr(c, reg) == want, f'native CPU {name}/{reg}: C {getattr(c, reg):04x} DOS {want:04x}')
                frame_mask = {physical(NSS, (sp - k) & 0xFFFF) for sp in kb_sps for k in range(1, 11)}
                at = 28
                for seg, off, length in regions:
                    for k in range(length):
                        p = physical(seg, (off + k) & 0xFFFF)
                        if p in frame_mask:
                            continue
                        check(ram[p] == rec[at + k], f'native {name} at {seg:04x}:{(off + k) & 0xFFFF:04x}: C {ram[p]:02x} DOS {rec[at + k]:02x}')
                    at += length
                    compared += length
            else:
                (scells, scur), (fcells, fcur) = text_models(o)
                got_final = struct.unpack_from('<%dH' % (8 * 0x800), rec, 28)
                got_cursors = struct.unpack_from('<8H', rec, 28 + 0x8000)
                got_snap = struct.unpack_from('<%dH' % (8 * 0x800), rec, 28 + 0x8000 + 0x10)
                for what, want, got in (('snapshot (message visible, key pending)', scells, got_snap), ('final screen (row 24 blanked)', fcells, got_final)):
                    bad = [i for i in range(len(want)) if want[i] != got[i]]
                    check(not bad, f'BIOS-live {name} {what}: cell {bad[0]:04x} page {bad[0] // 0x800} row {(bad[0] % 0x800) // 80} col {(bad[0] % 0x800) % 80}: model {want[bad[0]]:04x} BIOS {got[bad[0]]:04x} ({len(bad)} differ)' if bad else '')
                n_snap = sum(1 for x in got_snap if x != 0x0720); n_final = sum(1 for x in got_final if x != 0x0720)
                check(n_snap == sum(1 for x in scells if x != 0x0720) and n_final == sum(1 for x in fcells if x != 0x0720) and n_final > 0 and n_snap >= n_final,
                      f'BIOS-live {name}: non-blank cells snapshot {n_snap}, final {n_final}')
                nonblank_total += n_snap + n_final
                want_cursors = tuple((fcur[p][0] << 8) | fcur[p][1] for p in range(8))
                check(tuple(got_cursors) == want_cursors, f'BIOS-live {name}: cursors {got_cursors} != model {want_cursors}')
                check(taken >= 1, f'BIOS-live {name}: the key was never taken')
                compared += 2 * len(scells)
        probe = subprocess.run([binary, '--version'], capture_output=True, text=True, env=dict(os.environ, SDL_VIDEODRIVER='dummy'))
        version = next(line.split(',')[0].strip() for line in (probe.stdout + probe.stderr).splitlines() if 'DOSBox-X version' in line)
        results[label] = dict(cases=len(rows), result_sha256=sha256(data).hexdigest(), compared_units=compared, nonblank_cells_written=nonblank_total,
                              regions=[f'{s:04X}:{o:04X}+{n:X}' for s, o, n in regions],
                              dosbox_x=dict(path=binary, sha256=sha256(open(binary, 'rb').read()).hexdigest(), version=version,
                                            pinned_source='5a9c7c53481c2e933948039c33256776653f4faf', pinned_build_used=False))
    results['image_bin_sha256'] = sha256(image_reloc).hexdigest()
    return results
