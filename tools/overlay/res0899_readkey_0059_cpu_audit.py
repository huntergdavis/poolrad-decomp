#!/usr/bin/env python3
"""RES_0899_0059 (the game's ReadKey with hotkeys) against the original bytes, as a composed closure: the body, Crt ReadKey/
KeyPressed (INT 16h the declared service), the 08AE driver entries 00DC and 017B with their member models, SYSTEM set IN,
string compare and ParamStr, and RES_0899_0000 QUIT (Intr, Halt as EXIT).

The whole loader-expanded START.EXE image is loaded at a test base with the loader's own relocation table applied; no byte
is patched. The declared environment is RES_0899_0000's (INT 21h 25h/35h/2Ch, ports, the INT 10h stub) plus INT 16h:
AH=00h takes the next key word of the case's script, AH=01h reports it (ZF set when the script is empty) without taking it;
any other AH or interrupt is refused. Unicorn: CPU14 + full 1 MiB + stop + the ordered environment event log, unmasked.
Media and generated programs stay under ignored resources/."""
import argparse, ctypes, json, os, random, struct, subprocess, sys, tempfile
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, REGISTERS, physical, put16, check
import res0899_quit_0000_cpu_audit as Q
CHECKER_ROWS = ('RES_0899_0059',)
ENTRY, END = 0x0059, 0x0104
DIGEST = '4a361032123f0bdc7fa2ff158dc2d8f690716ee716462f66bda9740f32e3362f'
BASE, SEG, SS, STUB_SEG, PSP = Q.BASE, Q.SEG, Q.SS, Q.STUB_SEG, 0x6800
CRT = 0x0A40
FLAGS = Q.FLAGS
SOURCES = Q.SOURCES + ['resident_0899_readkey_0059_cpu', 'crt_keypressed_02fd_cpu', 'crt_readkey_030f_cpu', 'system_paramstr_16bc_cpu',
                       'system_runtime_leaves_cpu', 'system_string_move_leaves_cpu']
CONDITIONALS = {0x006C: (0x00A6, 0x006E), 0x0073: (0x008B, 0x0075), 0x0099: (0x00A6, 0x009B), 0x00BF: (0x00DE, 0x00C1),
                0x00C5: (0x00D4, 0x00C7), 0x00CE: (0x00D1, 0x00D0), 0x00D8: (0x00DE, 0x00DA), 0x00E2: (0x00F7, 0x00E4),
                0x00EB: (0x00F7, 0x00ED)}
KB_FN = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(Cpu), ctypes.POINTER(Cpu))


class KeyEnv(ctypes.Structure):
    _fields_ = [('quit', Q.QuitEnv), ('keyboard', KB_FN), ('keyboard_context', ctypes.c_void_p), ('drain_limit', ctypes.c_uint)]


class KeyDiag(ctypes.Structure):
    _fields_ = [('callee', ctypes.c_uint16), ('status', ctypes.c_int), ('driver', Q.DriverDiag), ('quit', Q.QuitDiag),
                ('readkeys', ctypes.c_uint), ('keypressed', ctypes.c_uint)]


STATUS = {0: 'RETURN', 1: 'ARGUMENT', 2: 'EXIT', 3: 'BREAK', 4: 'CRT', 5: 'DRIVER', 6: 'LEAF', 7: 'QUIT', 8: 'LIMIT', 9: 'TRANSFER'}
RETURN_TO = (0x9001, 0x7000)
BREAK_AT = (BASE + CRT, 0x014B)
MEMBERS = Q.MEMBERS + [('08AE:017B entry', 'driver', 0x017B, 0x01A5), ('0899:0000 QUIT', 'body', 0x0000, 0x0031),
                       ('0A40:02FD KeyPressed', 'crt', 0x02FD, 0x030F), ('0A40:030F ReadKey', 'crt', 0x030F, 0x0331),
                       ('0A40:0143 break check', 'crt', 0x0143, 0x014B), ('0AF8:16BC ParamStr', 'system', 0x16BC, 0x16CB),
                       ('0AF8:16FB ParamStr tail', 'system', 0x16FB, 0x170B), ('0AF8:1712 word walk', 'system', 0x1712, 0x1744),
                       ('0AF8:0DC0 set IN', 'system', 0x0DC0, 0x0DE1), ('0AF8:0C10 strcmp', 'system', 0x0C10, 0x0C3B)]
SEGS = dict(SEG, crt=CRT)


def source(image):
    body = image[SEG['body'] * 16 + ENTRY:SEG['body'] * 16 + END]
    check(sha256(body).hexdigest() == DIGEST, 'ReadKey-with-hotkeys body identity')
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(len(ins) == 61 and sum(i.size for i in ins) == len(body), 'decode covers the span')
    check([i.address for i in ins if i.mnemonic in ('ret', 'retf')] == [0x0103], 'single RETF')
    check([(i.address, i.op_str) for i in ins if i.mnemonic == 'lcall'] ==
          [(0x0060, '0xa40, 0x30f'), (0x007F, '0x8ae, 0xdc'), (0x0094, '0xaf8, 0xdc0'), (0x00A1, '0x8ae, 0x17b'),
           (0x00B0, '0xaf8, 0x16bc'), (0x00BA, '0xaf8, 0xc10'), (0x00E4, '0xa40, 0x2fd'), (0x00ED, '0xa40, 0x30f')], 'far calls')
    check([(i.address, i.op_str) for i in ins if i.mnemonic == 'call'] == [(0x00DB, '0')], 'near CALL 0000')
    check({i.address for i in ins if i.mnemonic.startswith('j') and i.mnemonic != 'jmp'} == set(CONDITIONALS), 'conditional sites')
    check(image[SEG['body'] * 16 + 0x33:SEG['body'] * 16 + 0x53] == b'\x03' + b'\x00' * 31, 'the CS:0033 set is {0,1}')
    check(image[SEG['body'] * 16 + 0x53:SEG['body'] * 16 + 0x59] == b'\x05STING', "the CS:0053 literal is 'STING'")
    return body, {i.address: i for i in ins}


def library(directory):
    path = directory / 'readkey59.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include')] +
                   [str(ROOT / f'src/recovered/{s}.c') for s in SOURCES] + ['-o', str(path)], check=True)
    f = ctypes.CDLL(str(path)).pr_r0899_readkey_0059_cpu
    f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(KeyEnv), ctypes.POINTER(KeyDiag)]
    f.restype = ctypes.c_int
    return f


def build(image_reloc, *, keys=(0x1E61,), cur=1, saved=0, w=0x0001, arg=0, mode=0x03, flag49=0, buffered=0, brk=0,
          tail=b' STING', ss=SS, sp=0xF000, flags=2, cpu=None, in61=0xB0, time_start=4736147, time_step=7, table=None, base=BASE):
    m = bytearray([0x5A]) * SIZE
    b = base * 16; m[b:b + len(image_reloc)] = image_reloc
    ds = base + SEG['data']
    if ss == 'ds':
        ss = ds
    m[physical(ds, 0x5372)] = cur; m[physical(ds, 0x5373)] = saved
    put16(m, ds, 0x5374, w); put16(m, ds, 0x2612, arg); m[physical(ds, 0x4A1C)] = mode
    m[physical(ds, 0x49F7)] = flag49; m[physical(ds, 0x8501)] = buffered; m[physical(ds, 0x8502)] = brk
    put16(m, ds, 0x4422, PSP)
    m[physical(PSP, 0x80)] = len(tail)
    for k, ch in enumerate(tail + b'\r'):
        m[physical(PSP, 0x81 + k)] = ch
    s = physical(STUB_SEG, 0); m[s:s + len(Q.INT10_STUB)] = Q.INT10_STUB
    put16(m, 0, 0x40, 0x0000); put16(m, 0, 0x42, STUB_SEG)
    m[physical(0x0040, 0x0049)] = 0x5A
    put16(m, ss, sp, RETURN_TO[1]); put16(m, ss, (sp + 2) & 0xFFFF, RETURN_TO[0])
    r = cpu or (0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777)
    c = Cpu(*r, sp, base + SEG['body'], ENTRY, ds, 0x1111, ss, flags)
    env = dict(table=table or {8: (0x0123, 0xF000)}, in61=in61, time_start=time_start, time_step=time_step)
    return m, c, env, list(keys)


def run_unicorn(name, m, c, envspec, keys, body_ips, base=BASE):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_INTR, UC_HOOK_INSN
    from unicorn import x86_const as R
    env = Q.Env(**envspec); script = list(keys)
    uc = Uc(UC_ARCH_X86, UC_MODE_16); backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    uc.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing)); uc.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        uc.reg_write(reg, getattr(c, n))
    s = dict(stop=None, steps=0, prev=None); seen = set(); closure = set(); edges = set()

    def code(em, address, size, user):
        cs = uc.reg_read(R.UC_X86_REG_CS); ip = uc.reg_read(R.UC_X86_REG_IP)
        if s['prev'] in CONDITIONALS and cs == base + SEG['body']:
            edges.add((s['prev'], ip))
        s['prev'] = None
        if (cs, ip) == (base + SEG['system'], 0x00E9):
            s['stop'] = 'EXIT'; return uc.emu_stop()
        if (cs, ip) == (base + CRT, 0x014B):
            s['stop'] = 'BREAK'; return uc.emu_stop()
        if (cs, ip) == RETURN_TO:
            s['stop'] = 'RETURN'; return uc.emu_stop()
        if cs not in {base + v for v in SEGS.values()} | {STUB_SEG}:
            s['stop'] = 'TRANSFER'; return uc.emu_stop()      # control left every known segment (a corrupted return)
        closure.add((cs, ip))
        if cs == base + SEG['body'] and ENTRY <= ip < END:
            check(ip in body_ips, f'escape inside 0059 {name} {ip:04x}'); seen.add(ip); s['prev'] = ip
        s['steps'] += 1
        if s['steps'] > 600000:
            s['stop'] = 'RUNAWAY'; uc.emu_stop()

    def intr(em, intno, user):
        ax = uc.reg_read(R.UC_X86_REG_AX); ah, al = ax >> 8, ax & 0xFF
        if intno == 0x16:
            check(ah in (0x00, 0x01), f'ENV refusal {name}: INT 16h AH {ah:#x}')
            fl = uc.reg_read(R.UC_X86_REG_EFLAGS)
            if ah == 0x01:
                env.log.append(('16', 1, script[0] if script else None))
                if script:
                    uc.reg_write(R.UC_X86_REG_AX, script[0]); uc.reg_write(R.UC_X86_REG_EFLAGS, fl & ~0x40)
                else:
                    uc.reg_write(R.UC_X86_REG_EFLAGS, fl | 0x40)
                return
            check(bool(script), f'ENV refusal {name}: INT 16h AH 00h with the script empty')
            k = script.pop(0); env.log.append(('16', 0, k)); uc.reg_write(R.UC_X86_REG_AX, k); return
        check(intno == 0x21, f'ENV refusal {name}: INT {intno:#x}')
        if ah == 0x2C:
            cx, dx = env.gettime(); uc.reg_write(R.UC_X86_REG_CX, cx); uc.reg_write(R.UC_X86_REG_DX, dx); return
        check(ah in (0x25, 0x35), f'ENV refusal {name}: INT 21h AH {ah:#x}')
        sp = uc.reg_read(R.UC_X86_REG_SP); ss_ = uc.reg_read(R.UC_X86_REG_SS)
        fl = uc.reg_read(R.UC_X86_REG_EFLAGS) & 0xFFFF
        for k, v in ((2, fl), (4, uc.reg_read(R.UC_X86_REG_CS)), (6, uc.reg_read(R.UC_X86_REG_IP))):
            uc.mem_write(physical(ss_, (sp - k) & 0xFFFF), struct.pack('<H', v))
        out = env.dos(ah, al, dict(ds=uc.reg_read(R.UC_X86_REG_DS), dx=uc.reg_read(R.UC_X86_REG_DX)))
        for k, v in out.items():
            uc.reg_write(dict(es=R.UC_X86_REG_ES, bx=R.UC_X86_REG_BX)[k], v)

    def port_in(em, port, size, user):
        check(port == 0x61 and size == 1, f'ENV refusal {name}: IN {port:#x}')
        env.log.append(('in', port, env.in61)); return env.in61

    def port_out(em, port, size, value, user):
        check(port in (0x40, 0x43, 0x61, 0xC0) and size == 1, f'ENV refusal {name}: OUT {port:#x}')
        env.log.append(('out', port, value))
    uc.hook_add(UC_HOOK_CODE, code); uc.hook_add(UC_HOOK_INTR, intr)
    uc.hook_add(UC_HOOK_INSN, port_in, None, 1, 0, R.UC_X86_INS_IN)
    uc.hook_add(UC_HOOK_INSN, port_out, None, 1, 0, R.UC_X86_INS_OUT)
    uc.emu_start(physical(c.cs, c.ip), 2 * SIZE, count=700000)
    check(s['stop'] in ('RETURN', 'EXIT', 'BREAK', 'TRANSFER'), f'original stop {name}: {s["stop"]}')
    return uc, backing, env, s['stop'], seen, closure, edges


def keyboard(env, keys, keep, kb_sps=None):
    script = list(keys)

    def kb(ctx, before, after):
        b = before[0]; ah = b.ax >> 8
        if kb_sps is not None:
            kb_sps.append(b.sp)
        a = Cpu.from_buffer_copy(b); a.ip = (b.ip + 2) & 0xFFFF
        if ah == 0x01:
            env.log.append(('16', 1, script[0] if script else None))
            if script:
                a.ax = script[0]; a.flags = b.flags & ~0x40
            else:
                a.flags = b.flags | 0x40
            after[0] = a; return 0
        if ah == 0x00 and script:
            k = script.pop(0); env.log.append(('16', 0, k)); a.ax = k; after[0] = a; return 0
        return 1
    fn = KB_FN(kb); keep.append(fn)
    return fn


def run_c(f, m, c, envspec, keys, stub_at=None, time_sps=None, kb_sps=None):
    keep = []
    q, env = Q.c_env(envspec, keep, stub_at=stub_at, time_sps=time_sps)
    ke = KeyEnv(q, keyboard(env, keys, keep, kb_sps), None, 64)
    actual = Cpu.from_buffer_copy(c); ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m); d = KeyDiag()
    status = STATUS.get(f(ram, SIZE, ctypes.byref(actual), ctypes.byref(ke), ctypes.byref(d)))
    return status, actual, bytes(ram), env, d


def expected(o):
    """Independent statement of the body's semantics over the Crt key protocol (not either model)."""
    keys = list(o.get('keys', (0x1E61,))); buffered = o.get('buffered', 0)

    def readkey():
        nonlocal buffered
        if buffered:
            k, buffered = buffered, 0; return k
        word = keys.pop(0); al, ah = word & 0xFF, word >> 8
        if al == 0:
            buffered = ah
            return 3 if ah == 0 else 0
        return al
    key = readkey()
    cur, saved = o.get('cur', 1), o.get('saved', 0); flag49 = o.get('flag49', 0)
    if key == 0x13:
        if cur != 2:
            saved, cur = cur, 2
        elif saved in (0, 1):
            cur = saved
    tail = o.get('tail', b' STING')
    words = [w for w in bytes(b if b > 0x20 else 0x20 for b in tail).split(b' ') if w]
    sting = bool(words) and words[0] == b'STING'
    if sting and key == 0x04:
        flag49 = 1 if flag49 == 0 else 0
    if sting and key == 0x03:
        return dict(stop='EXIT', cur=saved, saved=saved, flag49=flag49)     # QUIT: [5372] := [5373], then Halt
    if key != 0:
        while buffered or keys:
            key = readkey()
    return dict(stop='RETURN', key=key, cur=cur, saved=saved, flag49=flag49)


def compare(name, state, body_ips, f):
    m, c, envspec, keys = state
    uc, backing, uenv, ustop, seen, closure, edges = run_unicorn(name, m, c, envspec, keys, body_ips)
    status, actual, ram, cenv, d = run_c(f, m, c, envspec, keys)
    check(status == ustop, f'C status {name}: {status} original {ustop} (callee {d.callee:04x}/{d.status})')
    check(cenv.log == uenv.log, f'environment events {name}: C {cenv.log[:6]} original {uenv.log[:6]}')
    if ram != bytes(backing):
        diff = [hex(i) for i, (a, b) in enumerate(zip(ram, bytes(backing))) if a != b]
        raise ValueError(f'RAM {name}: {diff[:8]} ({len(diff)} bytes)')
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        want = uc.reg_read(reg) & 0xFFFF
        check(getattr(actual, n) == want, f'CPU {name}/{n} C {getattr(actual, n):04x} original {want:04x}')
    return ustop, seen, closure, edges, actual, ram


K = dict(a=0x1E61, s=0x1F13, d=0x2004, c=0x2E03, f1=0x3B00, zero=0x0000, x=0x2D78)


def cases():
    rng = random.Random(0x08990059)
    out = []
    for cur in (0, 1, 2):
        for saved in (0, 1, 2, 5):
            out.append((f'ctrl-s-c{cur}-s{saved}', dict(keys=(K['s'],), cur=cur, saved=saved)))
    for w in (0, 1, 0x100):
        for arg in (0, 1, 0xFF, 5):
            out.append((f'ctrl-s-off-w{w:x}-a{arg:x}', dict(keys=(K['s'],), cur=0, w=w, arg=arg)))
    out += [(f'ctrl-s-on-w{w:x}', dict(keys=(K['s'],), cur=2, saved=1, w=w)) for w in (0, 1, 0x100)]
    for tail in (b' STING', b'', b' STING foo', b'STING', b' sting', b' STINGX', b'\tSTING', b' xSTING'):
        for key in ('d', 'c', 'a'):
            out.append((f'{key}-tail{tail!r}', dict(keys=(K[key],), tail=tail)))
    out += [(f'ctrl-d-49f7-{v:x}', dict(keys=(K['d'],), flag49=v)) for v in (0, 1, 5, 0xFF)]
    out += [(f'quit-mode{md:02x}', dict(keys=(K['c'],), mode=md, cur=c_, saved=s_)) for md, c_, s_ in ((3, 2, 2), (0x0D, 0, 1), (0x10, 1, 0))]
    out += [('drain-3', dict(keys=(K['a'], K['x'], K['a'], K['x']))), ('drain-ext', dict(keys=(K['a'], K['f1'], K['a']))),
            ('ext-first', dict(keys=(K['f1'],))), ('zero-first', dict(keys=(K['zero'],))), ('zero-then', dict(keys=(K['zero'], K['a']))),
            ('buffered', dict(keys=(), buffered=0x48)), ('buffered-then', dict(keys=(K['a'],), buffered=0x48)),
            ('ctrl-s-drain', dict(keys=(K['s'], K['a'], K['s']))),
            # 006C's wrapped-clock guard and a long wait, on the sound-off path (00DC with [5374] <> 0).
            ('ctrl-s-midnight', dict(keys=(K['s'],), cur=0, w=1, time_start=8639995, time_step=7)),
            ('ctrl-s-slow-clock', dict(keys=(K['s'],), cur=1, w=1, time_step=1))]
    out += [(f'break-{lab}', dict(keys=ks, brk=1)) for lab, ks in (('a', (K['a'],)), ('drain', (K['a'], K['x'])))]
    out += [(f'flags{fl:x}-{lab}', dict(flags=fl, keys=ks)) for fl in FLAGS for lab, ks in (('a', (K['a'],)), ('s', (K['s'],)), ('d', (K['d'],)))]
    out += [('sp-low', dict(sp=0x0200)), ('sp-odd', dict(sp=0xF001)), ('ss-ds', dict(ss=BASE + SEG['data'], sp=0xE000))]
    for k in range(24):
        n = rng.randrange(1, 4)
        out.append((f'random{k}', dict(keys=tuple(rng.choice(list(K.values()) + [rng.randrange(1, 65536)]) for _ in range(n)),
                                       cur=rng.choice((0, 1, 2, rng.randrange(256))), saved=rng.choice((0, 1, 2, rng.randrange(256))),
                                       w=rng.choice((0, 1, rng.randrange(65536))), arg=rng.choice((0, 1, 0xFF, rng.randrange(65536))),
                                       flag49=rng.randrange(256), tail=rng.choice((b' STING', b'', b' xyz')), flags=rng.choice(FLAGS),
                                       cpu=tuple(rng.randrange(65536) for _ in range(7)), in61=rng.randrange(256))))
    return out


def differential(image_reloc, decoded, f):
    seen, closure, edges, stops = set(), set(), set(), {}
    specs = cases(); ds = BASE + SEG['data']
    for name, o in specs:
        state = build(image_reloc, **o)
        stop, v, cl, e, final, ram = compare(name, state, set(decoded), f)
        seen |= v; closure |= cl; edges |= e; stops[stop] = stops.get(stop, 0) + 1
        if o.get('brk') or o.get('ss') is not None:
            continue
        want = expected(o)
        check(stop == want['stop'], f'{name}: independent stop {want["stop"]}, got {stop}')
        check((ram[physical(ds, 0x5372)], ram[physical(ds, 0x5373)], ram[physical(ds, 0x49F7)]) ==
              (want['cur'], want['saved'], want['flag49']), f'{name}: independent sound/flag state')
        if stop == 'RETURN':
            check(final.ax & 0xFF == want['key'] and (final.cs, final.ip) == RETURN_TO, f'{name}: independent key {want["key"]:02x}')
    check(seen == set(decoded), f'body starts missing {sorted(set(decoded) - seen)}')
    want_edges = {(site, t) for site, ts in CONDITIONALS.items() for t in ts}
    check(edges == want_edges, f'outcomes missing {sorted(want_edges - edges)}')
    per_member = {}
    for label, seg, start, end in MEMBERS:
        base = SEGS[seg] * 16
        decoded_ips = {i.address for i in Cs(CS_ARCH_X86, CS_MODE_16).disasm(image_reloc[base + start:base + end], start)}
        ran = {ip for cs, ip in closure if cs == BASE + SEGS[seg] and start <= ip < end}
        check(ran <= decoded_ips, f'{label}: executed starts off the decode')
        per_member[label] = f'{len(ran)}/{len(decoded_ips)}'
    return dict(cases=len(specs), stops=stops, body_starts=len(seen), body_starts_total=len(decoded),
                conditional_outcomes=len(edges), conditional_outcomes_total=len(want_edges), closure_member_starts=per_member,
                cpu_words=14, ram_bytes=SIZE, flag_masks=0)


ALIAS_BASE = 0x1069   # 0899 relocates to 1902h: the pushed CS word has low byte 02


def constructed_alias(image, sites, decoded, f):
    """VP i20200: SS = DS and SP placing the CS push of the +00A1 LCALL 08AE:017B on DS:5372, at a load base whose 0899
    segment has low byte 02. Muted path ([5372] = 2, [5373] = 1), key Ctrl-S. The call at +00A1 runs with SP = entry SP -
    104h, so entry SP = 5374h + 104h = 5478h puts the pushed CS word at DS:5372."""
    image_reloc = Q.relocate(image, sites, ALIAS_BASE)
    check((ALIAS_BASE + SEG['body']) & 0xFF == 0x02, 'alias base: 0899 segment low byte 02')
    o = dict(keys=(K['s'],), cur=2, saved=1, w=0, ss='ds', sp=0x5478, base=ALIAS_BASE)
    m, c, envspec, keys = build(image_reloc, **o)
    uc, backing, uenv, ustop, seen, closure, edges = run_unicorn('constructed-alias', m, c, envspec, keys, set(decoded), base=ALIAS_BASE)
    status, actual, ram, cenv, d = run_c(f, m, c, envspec, keys)
    check(status == ustop, f'alias: C {status} original {ustop}')
    check(cenv.log == uenv.log, 'alias: environment events')
    check(ram == bytes(backing), 'alias: RAM')
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        check(getattr(actual, n) == uc.reg_read(reg) & 0xFFFF, f'alias: CPU {n}')
    drv = ALIAS_BASE + SEG['driver']
    # The mechanism, asserted: the +0060 ReadKey LCALL runs at the same SP (entry SP - 104h), so its pushed CS word lands on
    # DS:5372 first and [5373] := CS high byte (19h); the set-IN test at +0094 fails, 017B is never entered, +018B unreached.
    check((drv, 0x017B) not in closure and (drv, 0x018B) not in closure, 'alias: 017B must not be entered')
    check(ustop == 'RETURN' and (ALIAS_BASE + SEG['body'], 0x009B) not in closure, 'alias: the set-IN test fails (no +009B)')
    return dict(reached_017b_entry=(drv, 0x017B) in closure,load_base=f'{ALIAS_BASE:04X}', body_cs=f'{ALIAS_BASE + SEG["body"]:04X}', entry_ss_sp=f'{c.ss:04X}:{c.sp:04X}',
                stop=ustop, reached_017b_018b=(drv, 0x018B) in closure, final_cs_ip=f'{actual.cs:04X}:{actual.ip:04X}',
                ds_5372=ram[physical(c.ds, 0x5372)], ds_5373=ram[physical(c.ds, 0x5373)], callee=f'{d.callee:04X}', cpu_words=14,
                ram_bytes=SIZE)


def main():
    p = argparse.ArgumentParser()
    for flag in ('--compile-only', '--unicorn', '--native'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--output', type=Path); a = p.parse_args()
    with tempfile.TemporaryDirectory(prefix='readkey59-') as tmp:
        f = library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        image, sites = Q.load_image()
        body, decoded = source(image)
        image_reloc = Q.relocate(image, sites, BASE)
        report = dict(target=dict(span='0899:[0059,0104)', image_span=[SEG['body'] * 16 + ENTRY, SEG['body'] * 16 + END],
                                  bytes=len(body), instructions=len(decoded), sha256=DIGEST, image_sha256=Q.IMAGE_DIGEST,
                                  relocations=len(sites), load_base=f'{BASE:04X}'))
        if a.unicorn:
            report['unicorn'] = differential(image_reloc, decoded, f)
            report['constructed_alias'] = constructed_alias(image, sites, decoded, f)
        if a.native:
            import res0899_readkey_0059_native
            report['native'] = res0899_readkey_0059_native.native(image, sites, decoded, f, sys.modules[__name__])
    if a.output:
        a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    main()
