"""Native DOSBox-X corroboration for RES_0802_0040 (the character writer); used by res0802_0040_cpu_audit.py.

The loader-expanded START.EXE image, relocated to segment 3000h with the loader's own table, is read from IMAGE.BIN before every case, so
each case starts from the original bytes. TWO native runs, each pinning a different claim:

  RUN A, stub level (declared substitution). INT 10h is the same real-byte logging stub as the Unicorn run, with its log segment moved to
  2800h. The C model, composed with Dos.Intr and a service that emulates that stub, is compared with the oracle on all 14 CPU words, the stack
  window, DS:4A00..4A1F and the log. This pins: the sequence of INT 10h register blocks the ORIGINAL body and ORIGINAL Intr issue.
  The mechanism is ours on both sides (the stub), so a wrong belief about INT 10h would leave this run green.

  RUN B, BIOS live. NO hook: the original body and Intr call DOSBox-X's REAL INT 10h in text mode 03h (80x25, eight pages). After each case
  the eight text pages (B800:0000..7FFF) and the BIOS cursor positions (0040:0050..005F) are read back and compared with an INDEPENDENT Python
  text-mode model driven only by the contract (inputs -> register blocks -> cells), which shares no code with the C model. This pins: that
  the register blocks mean what we think (AH=02 page/row/col, AH=09 char/attribute/count) and that attributes are translated through DS:25D5
  only when DS:4A1E is 0. Its mechanism is DOSBox-X's BIOS, not ours. Cases stay inside one row and inside pages 0..7: what the BIOS does
  beyond a row end or on an unsupported page is not claimed."""
import ctypes, os, struct, subprocess
from hashlib import sha256
import res0802_0040_cpu_audit as A
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, physical, put16, check

NBASE, NSS, TOP, LOW, WINDOW, NLOG = 0x3000, 0x6000, 0xF000, 0xEC00, 0x0400, 0x2800
DATA_SEG = NBASE + A.SEG['data']
ARG_HI = (0xAB, 0xCD, 0xEF, 0x12, 0x34)


def rows_a():
    """(name, options) for run A: a subset of the Unicorn cases that is meaningful natively (DS fixed, stack in the stack segment)."""
    out = []
    for ega in (0, 1, 2, 0xFF):
        for attr in (0x00, 0x01, 0x0F, 0x12, 0x7F, 0x80, 0xFF):
            out.append((f'a-ega{ega:02x}-attr{attr:02x}', dict(ega=ega, attr=attr, ch=0x41 + attr % 26, count=1 + attr % 5, row=attr % 25, col=attr % 70)))
    for ch, count, row, col in ((0x00, 0, 0, 0), (0xDB, 0xFF, 24, 79), (0x20, 0x28, 0xFF, 0xFF), (0x7F, 2, 10, 39)):
        for ega in (0, 1, 2):
            out.append((f'a-edge-{ch:02x}-ega{ega}', dict(ega=ega, ch=ch, count=count, row=row, col=col, attr=0x31)))
    for pa, pb in ((0, 0), (7, 3), (0xFF, 0)):
        out.append((f'a-pages{pa:02x}{pb:02x}', dict(ega=1, page_a=pa, page_b=pb)))
    out += [(f'a-flags{fl:x}', dict(flags=fl, ega=1)) for fl in A.FLAGS]
    return out


def rows_b():
    """(name, options) for run B: inside one row, pages 0..7, count small."""
    out = []
    for ega in (0, 1, 2):
        for attr in (0x00, 0x01, 0x07, 0x0C, 0x1F, 0x4E, 0x7F, 0xF0):
            out.append((f'b-ega{ega}-attr{attr:02x}', dict(ega=ega, attr=attr, ch=0x41 + (attr % 26), count=1 + attr % 4, row=1 + attr % 20, col=3 + attr % 60,
                                                          page_a=5, page_b=2)))
    for ch in (0x20, 0x41, 0x7E, 0xB0, 0xDB, 0xFE):
        out.append((f'b-ch{ch:02x}', dict(ega=1, ch=ch, count=3, row=12, col=30, attr=0x1E, page_a=6, page_b=1)))
    for pa, pb in ((0, 0), (0, 3), (7, 0), (4, 4), (1, 6)):
        out.append((f'b-pages{pa}{pb}', dict(ega=1, page_a=pa, page_b=pb, ch=0x58, count=2, row=0, col=0, attr=0x2A)))
    for row, col, count in ((0, 0, 1), (24, 79, 1), (24, 0, 80), (0, 70, 10), (12, 40, 40)):
        out.append((f'b-pos-{row}-{col}-{count}', dict(ega=0, row=row, col=col, count=count, ch=0x2A, attr=0x13, page_a=5, page_b=2)))
    return out


def table_for(o):
    return list(o['table']) if o.get('table') else [(0xA0 + k * 7 + 3) & 0xFF for k in range(256)]


def text_model(o):
    """The independent expectation: what a text-mode video BIOS does with the contract's register blocks. Returns (cells, cursors)."""
    cells = [0x0720] * (8 * 0x800)
    cursor = {p: (0, 0) for p in range(8)}
    for ax, bx, cx, dx, *_ in A.expected_log(None, o):
        ah, al, bh, bl = ax >> 8, ax & 0xFF, bx >> 8, bx & 0xFF
        if ah == 0x02:
            cursor[bh] = (dx >> 8, dx & 0xFF)
        elif ah == 0x09:
            row, col = cursor[bh]
            for i in range(cx):
                cells[bh * 0x800 + row * 80 + col + i] = (bl << 8) | al
    return cells, cursor


def c_run(image_reloc, o, comseg, stub_off, stub, ret_off, keep):
    """The C model on the native layout (run A); returns (status, cpu, ram)."""
    m = bytearray([0x5A]) * SIZE
    b = NBASE * 16; m[b:b + len(image_reloc)] = image_reloc
    ds = DATA_SEG
    tab = table_for(o)
    for k in range(256):
        m[physical(ds, 0x25D5 + k)] = tab[k]
    m[physical(ds, 0x4A1A)] = o.get('page_a', 7); m[physical(ds, 0x4A1B)] = o.get('page_b', 3); m[physical(ds, 0x4A1E)] = o.get('ega', 0)
    for k, v in enumerate((0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x8888, 0x9999, 0x0246)):
        put16(m, ds, 0x4A02 + 2 * k, v)
    put16(m, 0, 0x40, stub_off); put16(m, 0, 0x42, comseg)
    s = physical(comseg, stub_off); m[s:s + len(stub)] = stub
    for k in range(0x100):
        m[physical(NLOG, k)] = 0x5A
    put16(m, NLOG, 0, 0)
    sp = TOP - 14
    put16(m, NSS, sp, ret_off); put16(m, NSS, sp + 2, comseg)             # the far return frame: the oracle's `returned` label
    for k, (v, hi) in enumerate(zip((o.get('ch', 0x41), o.get('count', 1), o.get('attr', 0x12), o.get('row', 5), o.get('col', 7)), ARG_HI)):
        put16(m, NSS, sp + 4 + 2 * k, (hi << 8) | v)
    c = Cpu(0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, sp, NBASE + A.SEG['body'], A.ENTRY, ds, 0x1111, NSS, o.get('flags', 2))
    env = A.Env(A.c_video(keep, NLOG, (comseg, stub_off)), None); d = A.Diag()
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    status = A.F(ram, SIZE, ctypes.byref(c), ctypes.byref(env), ctypes.byref(d))
    return status, c, bytes(ram)


def program(image_reloc, rows, live):
    """One DOSBox-X COM: for each row, reload the image, set the case state, call 0802:0040 through a far frame and record the return."""
    ds_seg = DATA_SEG
    regions = [(0xB800, 0x0000, 0x8000), (0x0040, 0x0050, 0x10)] if live else [(NSS, LOW, WINDOW), (ds_seg, 0x4A00, 0x20), (NLOG, 0, 0x100)]
    writes = []
    for seg, off, n in regions:
        for k in range(0, n, 0x4000):
            writes += [f'mov ax,{seg}', 'mov ds,ax', 'mov bx,word ptr cs:[handle]', f'mov dx,{off + k}', f'mov cx,{min(0x4000, n - k)}', 'mov ah,0x40', 'int 0x21']
    fill = []
    for chunk in range(2):                      # 128 KiB of 5Ah from the load base: the image area and the data segment's BSS
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
         f'mov word ptr [stub10+4],{NLOG}',                                  # the stub's log-segment immediate (assembled for 0060h)
         'again:', 'cld'] + fill + [
         'push cs', 'pop ds', 'mov dx,offset imagename', 'mov ax,0x3d00', 'int 0x21', 'jc failure', 'mov word ptr cs:[image],ax'] + load + [
         'push cs', 'pop ds', 'mov bx,word ptr [image]', 'mov ah,0x3e', 'int 0x21',
         'mov ax,word ptr [index]', 'mov bx,18', 'mul bx', 'add ax,offset table', 'mov bx,ax',                          # bx = the row (nine words)
         # the attribute map: 256 bytes from the COM to DS:25D5
         f'mov ax,{ds_seg}', 'mov es,ax', 'mov di,0x25d5', 'mov si,offset attr_table', 'mov cx,256', 'rep movsb',
         'mov al,byte ptr cs:[bx+2]', 'mov byte ptr es:[0x4a1e],al', 'mov al,byte ptr cs:[bx+14]', 'mov byte ptr es:[0x4a1a],al',
         'mov al,byte ptr cs:[bx+16]', 'mov byte ptr es:[0x4a1b],al'] + [
         f'mov word ptr es:[{0x4A02 + 2 * k}],{v}' for k, v in enumerate(regs)]
    if live:
        t += ['mov ax,3', 'int 0x10']                                        # text mode 03h through the REAL BIOS: clears the screen and the cursors
    else:
        t += ['xor ax,ax', 'mov es,ax', 'mov word ptr es:[0x40],offset stub10', 'mov word ptr es:[0x42],cs',
              f'mov ax,{NLOG}', 'mov es,ax', 'xor di,di', 'mov cx,0x80', 'mov ax,0x5a5a', 'rep stosw', 'mov word ptr es:[0],0']
    t += [f'mov ax,{NSS}', 'mov es,ax', f'mov di,{LOW}', f'mov cx,{WINDOW // 2}', 'mov ax,0x5a5a', 'rep stosw',
          'cli', f'mov ax,{NSS}', 'mov ss,ax', f'mov sp,{TOP}',
          'push word ptr cs:[bx+12]', 'push word ptr cs:[bx+10]', 'push word ptr cs:[bx+8]', 'push word ptr cs:[bx+6]', 'push word ptr cs:[bx+4]',   # col row attr count char
          'push cs', 'push offset returned', 'push word ptr cs:[bx]',          # the far return frame, then the case flags
          f'mov ax,{ds_seg}', 'mov ds,ax', 'mov ax,0x1111', 'mov es,ax',
          'mov ax,0x1234', 'mov bx,0x3456', 'mov cx,0x5678', 'mov dx,0x789a', 'mov si,0x9abc', 'mov di,0xbcde', 'mov bp,0x7777',
          'popf', f'ljmp {NBASE + A.SEG["body"]},{A.ENTRY}', 'returned:']
    t += [f'mov word ptr cs:[out+{2 * n}],{r}' for n, r in enumerate('ax bx cx dx si di bp sp cs'.split())]
    t += ['mov word ptr cs:[out+18],offset returned', 'mov word ptr cs:[out+20],ds', 'mov word ptr cs:[out+22],es', 'mov word ptr cs:[out+24],ss',
          'mov ax,cs', 'mov ss,ax', 'mov sp,0xfff0', 'pushf', 'pop ax', 'mov word ptr cs:[out+26],ax',
          'mov ax,cs', 'mov ds,ax', 'xor ax,ax', 'mov es,ax', 'mov ax,word ptr [old10]', 'mov word ptr es:[0x40],ax', 'mov ax,word ptr [old10+2]', 'mov word ptr es:[0x42],ax',
          'mov bx,word ptr [handle]', 'mov dx,offset out', 'mov cx,28', 'mov ah,0x40', 'int 0x21'] + writes + [
          'push cs', 'pop ds', 'sti', 'inc word ptr [index]', f'cmp word ptr [index],{len(rows)}', 'jae done', 'jmp again', 'done:',
          'mov bx,word ptr [handle]', 'mov ah,0x3e', 'int 0x21', 'mov ax,0x4c00', 'int 0x21', 'failure:', 'mov ax,0x4c01', 'int 0x21',
          'stub10:', '.byte ' + ','.join(str(x) for x in STUB),
          'outname: .asciz "RESULT.BIN"', 'imagename: .asciz "IMAGE.BIN"', 'handle: .word 0', 'image: .word 0', 'index: .word 0',
          '.align 2', 'header: .space 8', 'out: .space 28', 'old10: .word 0,0',
          'attr_table: .byte ' + ','.join(str(x) for x in [(0xA0 + k * 7 + 3) & 0xFF for k in range(256)]), '.align 2', 'table:']
    t += ['.word ' + ','.join(str(v) for v in row) for row in rows]
    return t, regions


def native(image, sites, decoded, f, audit, stub):
    global A, STUB
    A = audit; A.F = f; STUB = stub_for_native = audit.assemble_stub(0x0060)
    image_reloc = A.relocate(image, sites, NBASE)
    results = {}
    for live, label, specs in ((False, 'stub', rows_a()), (True, 'bios', rows_b())):
        directory = ROOT / f'resources/0802-0040/native-{label}'; directory.mkdir(parents=True, exist_ok=True)
        (directory / 'IMAGE.BIN').write_bytes(image_reloc)
        rows = []
        for _, o in specs:
            vals = [o.get('ch', 0x41), o.get('count', 1), o.get('attr', 0x12), o.get('row', 5), o.get('col', 7)]
            rows.append((o.get('flags', 2), o.get('ega', 0)) + tuple((ARG_HI[k] << 8) | v for k, v in enumerate(vals)) + (o.get('page_a', 7), o.get('page_b', 3)))
        t, regions = program(image_reloc, rows, live)
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
                           env=dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy'), stdout=log, stderr=subprocess.STDOUT, check=True, timeout=900)
        data = out.read_bytes(); rec_len = 28 + sum(n for _, _, n in regions); record = rec_len
        check(len(data) == 8 + len(rows) * record, f'{label}: native length {len(data)} expected {8 + len(rows) * record}')
        comseg, stub_off, count, ret_off = struct.unpack_from('<4H', data); check(count == len(rows), 'native count')
        compared = 0; nonblank_total = 0
        for n, (name, o) in enumerate(specs):
            rec = data[8 + n * record:8 + (n + 1) * record]
            if not live:
                keep = []
                st, c, ram = c_run(image_reloc, o, comseg, stub_off, STUB, ret_off, keep)
                check(A.STATUS.get(st) == 'RETURNED', f'native C status {name}: {A.STATUS.get(st)}')
                for (reg, _), want in zip(Cpu._fields_, struct.unpack_from('<14H', rec)):
                    check(getattr(c, reg) == want, f'native CPU {name}/{reg}: C {getattr(c, reg):04x} DOS {want:04x}')
                at = 28
                for seg, off, length in regions:
                    for k in range(length):
                        p = physical(seg, (off + k) & 0xFFFF)
                        check(ram[p] == rec[at + k], f'native {name} at {seg:04x}:{(off + k) & 0xFFFF:04x}: C {ram[p]:02x} DOS {rec[at + k]:02x}')
                    at += length
                    compared += length
            else:
                cells, cursors = text_model(o)
                got_cells = struct.unpack_from('<%dH' % (8 * 0x800), rec, 28)
                got_cursors = struct.unpack_from('<8H', rec, 28 + 0x8000)
                bad = [i for i in range(len(cells)) if cells[i] != got_cells[i]]
                check(not bad, f'BIOS-live {name}: cell {bad[0]:04x} page {bad[0] // 0x800} row {(bad[0] % 0x800) // 80} col {(bad[0] % 0x800) % 80}: model {cells[bad[0]]:04x} BIOS {got_cells[bad[0]]:04x} ({len(bad)} differ)' if bad else '')
                nonblank = sum(1 for x in got_cells if x != 0x0720)
                check(nonblank == sum(1 for x in cells if x != 0x0720) and nonblank > 0, f'BIOS-live {name}: {nonblank} cells written, the model expects {sum(1 for x in cells if x != 0x0720)}')
                nonblank_total += nonblank
                want_cursors = tuple((cursors[p][0] << 8) | cursors[p][1] for p in range(8))
                check(tuple(got_cursors) == want_cursors, f'BIOS-live {name}: cursors {got_cursors} != model {want_cursors}')
                compared += len(cells)
        probe = subprocess.run([binary, '--version'], capture_output=True, text=True, env=dict(os.environ, SDL_VIDEODRIVER='dummy'))
        version = next(line.split(',')[0].strip() for line in (probe.stdout + probe.stderr).splitlines() if 'DOSBox-X version' in line)
        results[label] = dict(cases=len(rows), result_sha256=sha256(data).hexdigest(), compared_units=compared, nonblank_cells_written=nonblank_total, regions=[f'{s:04X}:{o:04X}+{n:X}' for s, o, n in regions],
                              dosbox_x=dict(path=binary, sha256=sha256(open(binary, 'rb').read()).hexdigest(), version=version,
                                            pinned_source='5a9c7c53481c2e933948039c33256776653f4faf', pinned_build_used=False))
    results['image_bin_sha256'] = sha256(image_reloc).hexdigest()
    return results
