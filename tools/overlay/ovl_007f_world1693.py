#!/usr/bin/env python3
"""A shared world for unit-0x7F bodies that call resident 00BA:1693: the 1693 audit's world (its EGA device, its printer, clear, delay and StrStore bodies at their own segments, its font and display
state) plus the unit-0x7F spans, the resident 13AB/11EA closure and SYSTEM StrLoad at segments of their own, and one Unicorn runner and one C runner for the composition.

The original runs as ORIGINAL bytes in Unicorn with the independent Python EGA behind A0000..AFFFF, the OUT instructions and the VRAM write operands logged in order. The C side runs the merged
models behind a pr_bus with an attached pr_ega and a ports environment. Compared unmasked: CPU14, ALL 1,048,576 RAM bytes, the EGA planes, latches and registers, the ordered OUT events, the ordered
VRAM operands, the stop and the remaining budget. The budget counts the instructions of the unit-0x7F bodies that carry one (not +0020 or +003A, whose merged models have none) and of the 13AB
closure; the 1693 closure keeps its own loop-back limits, which these audits do not exercise (its own audit does). Media stay under ignored resources/."""
import contextlib, ctypes, io, struct
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
import res00ba_display_1693_cpu_audit as D
from pinned_shift_flags import LastFlagWriter, declared_flags_split
import ovl_007f_0054_cpu_audit as B54
import ovl_007f_0000_cpu_audit as A
from res0709_printer_0626_cpu_audit import Cpu, Ega, Bus, Env, OUT_FN, IN_FN, WRITE_FN, ROOT, SIZE, REGISTERS, physical, put16, check
from overlay_inventory import find_units, parse_mz
from resident_loader_map import unpack

W, R13 = 0x9100, 0x9500                        # unit 0x7F and the resident 13AB closure (D's segments end below 9000h)
SEG = dict(D.SEGMENTS, w=W, res13=R13)
KIND_OF = {v: k for k, v in SEG.items()}
ENTRY_RET = (0x8000, 0x9000)                   # the entry frame's return address (not in any decoded body)
SOURCES = list(dict.fromkeys(['overlay_u7f_0203_cpu', 'overlay_u7f_0a88_cpu', 'overlay_u7f_044c_cpu', 'overlay_u7f_0054_cpu', 'overlay_u7f_entry0_0000_cpu', 'resident_00ba_character_0bb8_cpu', 'resident_00ba_character_helpers_cpu',
                              'overlay_u9e_entry16_10e4', 'overlay_u9e_entry17_110d', 'overlay_u7f_record_word_add_003a', 'overlay_u7f_record_subtract_0020', 'system_string_leaves_cpu',
                              'system_runtime_leaves_cpu'] + list(D.SOURCES) + ['memory_bus', 'ega_hw']))
SPANS = {0x0020: ('0020', 26, 9, 'f1e091b372c0401a7b90dac9e6a32f12c64b898c8f0039a69df380669e24e13a'), 0x003A: ('003A', 26, 9, 'ef4d49e3fef030f2acc13a9d5f78563d091bb60ce167bb986143671b6e14059f'),
         0x0203: ('0203', 145, 53, '7d0d7aaaf414d3b9ce3ef2999f2e7d7bade77e19029dd11405f0a65ed880a9d2'), 0x044C: ('044C', 188, 71, '1f9525dc814d360d5e1938b7d7fcaf1782ad65128922575603d402f4cad70a06'),
         0x0A88: ('0A88', 210, 87, '3e0eb89c3eb4beb862487bf9646c68f61bebcb70b203ce4578379c289759070a')}
CONSTS = {0x01DB: 39, 0x0441: 10, 0x0A7D: 10}  # the three message strings, plain data (no relocation): length byte + text
RELOC = {0x000F: R13, 0x022E: SEG['sys'], 0x0233: SEG['display'], 0x04FB: SEG['sys'], 0x0500: SEG['display'], 0x0AB1: SEG['sys'], 0x0AB6: SEG['display']}   # the unit's relocation-backed segment words used by +0000, +0203, +044C and +0A88
SYSTEM_STRLOAD = (0x0B0B, 0x0B25, 12, None)


SPLIT_ARRIVALS = [0]                         # states compared under the declared pinned-MULW split (the last FLAGS writer was a word MUL)
SHIFT_SPLIT_ARRIVALS = [0]                   # states compared under the declared AF-after-shift split (the last FLAGS writer was a non-zero SHL, +0A88's index scaling)


def source():
    """Every body of the world, keyed (kind, ip): D's closure, the 13AB closure, the +0000/+0054 spans, the +0020/+003A/+0203/+044C/+0A88 spans, StrLoad, and the three constants."""
    b54, d54, meta54 = B54.source()
    bodies, decoded, meta = D.source()
    bodies = dict(bodies); decoded = dict(decoded)
    for (k, s), blob in b54.items():
        bodies[('res13' if k == 'res' else 'w', s)] = blob
    for (k, s), ins in d54.items():
        decoded[('res13' if k == 'res' else 'w', s)] = ins
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes(); ovr = (ROOT / 'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    unit = next(u for u in find_units(exe[parse_mz(exe)['header_bytes']:], ovr) if u['stub_para'] == 0x7F)
    base = unit['ovr_offset']
    for off, (name, n, count, digest) in SPANS.items():
        blob = ovr[base + off:base + off + n]; ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(blob, off))
        check(sha256(blob).hexdigest() == digest and len(ins) == count and sum(i.size for i in ins) == n, f'unit 0x7F +{name} identity')
        bodies['w', off] = blob; decoded.update({('w', i.address): i for i in ins})
    for off, ln in CONSTS.items():
        check(not [f for f in unit['fixups'] if off <= f < off + 1 + ln], f'constant at {off:04X} has no relocation')
        bodies[('data', off)] = ovr[base + off:base + off + 1 + ln]
    check({f for f in unit['fixups'] if f in RELOC} == set(RELOC), 'the relocation-backed segment words are the unit fixups')
    with contextlib.redirect_stdout(io.StringIO()):
        image = bytes(unpack(exe)[0])
    blob = image[physical(0xAF8, 0x0B0B):physical(0xAF8, 0x0B25)]; ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(blob, 0x0B0B))
    check(len(ins) == 12 and sum(i.size for i in ins) == 26 and [i.mnemonic for i in ins][-1] == 'retf', 'SYSTEM StrLoad 0B0B identity')
    bodies['sys', 0x0B0B] = blob; decoded.update({('sys', i.address): i for i in ins})
    meta = dict(meta, strload=dict(bytes=26, instructions=12, sha256=sha256(blob).hexdigest()), unit7f_spans={f'{k:04X}': v[3] for k, v in SPANS.items()})
    return bodies, decoded, meta


def library(directory, override=None):
    import subprocess
    path = Path(directory) / ('w1693_mut.so' if override else 'w1693.so')
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include')] +
                   [str((override or {}).get(s, ROOT / f'src/recovered/{s}.c')) for s in SOURCES if (ROOT / f'src/recovered/{s}.c').exists() or s in (override or {})] + ['-o', str(path)], check=True)
    lib = ctypes.CDLL(str(path)); lib.pr_ega_reset.argtypes = [ctypes.POINTER(Ega)]
    return lib


class Limits(ctypes.Structure):
    _fields_ = [('clear', ctypes.c_size_t), ('rect', ctypes.c_size_t), ('printer', ctypes.c_size_t), ('glyph', ctypes.c_size_t), ('delay', ctypes.c_uint64)]


class Diag1693(ctypes.Structure):
    _fields_ = [('callee', ctypes.c_int), ('status', ctypes.c_int)]


def install(m, bodies):
    """D's installation (its bodies and their relocations), then the unit-0x7F spans, the constants, the 13AB closure and StrLoad at their own segments."""
    D.install(m, {k: v for k, v in bodies.items() if k[0] in D.SEGMENTS and not (k[0] == 'sys' and k[1] == 0x0B0B)})
    for (kind, start), blob in bodies.items():
        if kind == 'w':
            at = physical(W, start)
        elif kind == 'res13':
            at = physical(R13, start)
        elif kind == 'data':
            at = physical(W, start)
        elif kind == 'sys' and start == 0x0B0B:
            at = physical(SEG['sys'], start)
        else:
            continue
        m[at:at + len(blob)] = blob
    for off, seg in RELOC.items():
        put16(m, W, off, seg)


def base_state(bodies, **options):
    """D's world state (memory), with the CPU left to the caller. D.build supplies the display state (page, delay count, fonts) and installs D's bodies."""
    dbodies = {k: v for k, v in bodies.items() if k[0] in D.SEGMENTS and not (k[0] == 'sys' and k[1] == 0x0B0B)}
    m, c = D.build(dbodies, **options)
    install(m, bodies)
    return m, c


def run_c(lib, fn, m, c, call, budget, limits=None):
    """Run a C model. `fn` is the ctypes function; `call(bus, ram_size, cpu, budget, limits, diag)` adapts it. Returns (status, ram, events, writes, ega bytes, cpu, budget left, diagnostics)."""
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m); ega = Ega(); lib.pr_ega_reset(ctypes.byref(ega))
    for p in range(4):
        ctypes.memset(ctypes.addressof(ega.plane[p]), 0x31 + p * 0x27, 65536)
    events = []; writes = []

    def out(ctx, port, value, size, cs, ip):
        events.append((port, value, size, cs, ip)); return 0
    env = Env(None, OUT_FN(out), IN_FN()); bus = Bus(ram, ctypes.pointer(ega), ctypes.pointer(env), WRITE_FN(lambda ctx, off, v: writes.append((off, v))), None)
    cpu = Cpu.from_buffer_copy(c); left = ctypes.c_size_t(budget); lim = limits or Limits(65536, 65536, 65536, 65536, 1000000)
    status, diag = call(fn, ctypes.byref(bus), SIZE, ctypes.byref(cpu), ctypes.byref(left), lim)
    return status, bytes(ram), events, writes, bytes(ega), cpu, left.value, diag


def run_original(m, c, decoded, budget):
    """The original bytes in Unicorn with the independent EGA device. One unit of budget per executed instruction in the unit-0x7F bodies that carry one (not +0020 or +003A) and in the 13AB closure;
    the stop is the return to the entry frame's address (RETURNED), the budget (LIMIT, CPU at the next unexecuted counted instruction), or a return that goes elsewhere (TRANSFER)."""
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_INSN
    from unicorn import x86_const as R
    uc = Uc(UC_ARCH_X86, UC_MODE_16); backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(m); device = D.PythonEga()

    def read(em, off, size, user):
        return sum(device.read((off + j) & 65535) << (8 * j) for j in range(size))

    def write(em, off, size, value, user):
        for j in range(size):
            device.write((off + j) & 65535, (value >> (8 * j)) & 255)
    for base in (0, SIZE):
        uc.mem_map_ptr(base, 0xA0000, 7, ctypes.addressof(backing)); uc.mmio_map(base + 0xA0000, 65536, read, None, write, None)
        uc.mem_map_ptr(base + 0xB0000, 0x50000, 7, ctypes.addressof(backing) + 0xB0000)
    for (n, _), r in zip(Cpu._fields_, REGISTERS):
        uc.reg_write(r, getattr(c, n))
    left = [budget]; calls = []; pending = [None]; status = [None]; seen = set(); events = []; writer = LastFlagWriter()

    def word(seg, off):
        at = physical(seg, off & 0xFFFF); return backing[at] | (backing[(at + 1) & (SIZE - 1)] << 8)

    def hook(uc_, address, size, user):
        cs = uc.reg_read(R.UC_X86_REG_CS); ip = uc.reg_read(R.UC_X86_REG_IP)
        writer.finish(uc)                                    # settle a MUL that executed just before this arrival (pinned MULW needs its DX:AX)
        if pending[0] is not None:
            target, expected = pending[0]; pending[0] = None
            if expected is None:
                status[0] = 'RETURNED' if target == ENTRY_RET else 'TRANSFER'; uc.emu_stop(); return
            if target != expected:
                status[0] = 'TRANSFER'; uc.emu_stop(); return
        kind = KIND_OF.get(cs)
        if (kind, ip) not in decoded:
            raise AssertionError(('unexpected original PC', kind, hex(cs), hex(ip)))
        counted = (kind == 'w' and not (0x20 <= ip < 0x54)) or kind == 'res13'
        if counted:
            if not left[0]:
                status[0] = 'LIMIT'; uc.emu_stop(); return
            left[0] -= 1
        seen.add((kind, ip)); ins = decoded[(kind, ip)]
        writer.step(uc, address)
        if ins.mnemonic in ('call', 'lcall'):
            calls.append((cs, (ip + ins.size) & 0xFFFF))
        if ins.mnemonic == 'retf':
            sp = uc.reg_read(R.UC_X86_REG_SP); ss = uc.reg_read(R.UC_X86_REG_SS)
            pending[0] = ((word(ss, sp + 2), word(ss, sp)), calls.pop() if calls else None)
        elif ins.mnemonic == 'ret':                                                  # a near return (the delay helpers use them): the target is (CS, [SS:SP]) and must match its near call
            sp = uc.reg_read(R.UC_X86_REG_SP); ss = uc.reg_read(R.UC_X86_REG_SS)
            check(bool(calls), 'a near RET with no call on record')
            pending[0] = ((cs, word(ss, sp)), calls.pop())

    def out(em, port, size, value, user):
        events.append((port, value, size, uc.reg_read(R.UC_X86_REG_CS), uc.reg_read(R.UC_X86_REG_IP))); device.out(port, value, size)
    uc.hook_add(UC_HOOK_CODE, hook); uc.hook_add(UC_HOOK_INSN, out, None, 1, 0, R.UC_X86_INS_OUT)
    uc.emu_start(physical(c.cs, c.ip), 2 * SIZE, count=max(5000000, 40 * budget))
    check(status[0] is not None, 'original execution did not reach return, limit or transfer')
    cpu = Cpu(*(uc.reg_read(r) & 0xFFFF for r in REGISTERS))
    return status[0], bytes(backing), events, device.writes, device.packed(), cpu, left[0], seen, writer


def compare_states(label, expected, actual, bodies, m, decoded=None):
    """expected = run_original(...), actual = run_c(...). Raises on any difference; the EGA, the OUT events and the VRAM operands are compared in order."""
    estat, eram, eev, ewr, eega, ecpu, eleft, seen, writer = expected
    astat, aram, aev, awr, aega, acpu, aleft, _ = actual[:8]
    check(astat == estat, f'{label}: status C {astat} original {estat}')
    check(aleft == eleft, f'{label}: budget left C {aleft} original {eleft}')
    # MUL (in the 13AB closure): the model computes the PINNED MULW flags (tools/overlay/pinned_shift_flags.py, instructions.h:605-619: SF and AF kept, ZF from AX, PF over DX:AX, CF=OF=DX!=0); raw Unicorn leaves SF/ZF/AF/PF
    # at its own values. The split is declared, never masked: whenever the last FLAGS writer in the original is a word MUL, the C flags must equal Unicorn's outside the six arithmetic bits and the pinned value inside them;
    # on every other state the flags are compared exactly. Counted in SPLIT_ARRIVALS.
    # SHL (in +0A88): the model sets AF after a non-zero shift as the pinned core does (get_AF = count & 1Fh); raw Unicorn leaves it clear. Declared the same way: when the last FLAGS writer
    # is a non-zero shift, the C flags must equal Unicorn's with AF set. Counted in SHIFT_SPLIT_ARRIVALS.
    split = writer.rule == 'mul' and writer.division is not None
    shift = writer.division is None and writer.shift_nonzero
    for name, _ in Cpu._fields_:
        if name == 'flags' and (split or shift):
            problem = declared_flags_split(acpu.flags, ecpu.flags, writer)
            check(problem is None, f'{label}: {problem}')
            (SPLIT_ARRIVALS if split else SHIFT_SPLIT_ARRIVALS)[0] += 1
            continue
        check(getattr(acpu, name) == getattr(ecpu, name), f'{label}: {name} C {getattr(acpu, name):04x} original {getattr(ecpu, name):04x}')
    if aram != eram:
        diffs = [i for i in range(SIZE) if aram[i] != eram[i]]
        raise ValueError(f'{label}: RAM {[(hex(i), aram[i], eram[i]) for i in diffs[:12]]} ({len(diffs)} bytes)')
    check(aev == eev, f'{label}: OUT events C {aev[:3]} original {eev[:3]} ({len(aev)}/{len(eev)})')
    check(awr == ewr, f'{label}: VRAM operands ({len(awr)}/{len(ewr)})')
    if aega != eega:
        diffs = [(hex(i), a, b) for i, (a, b) in enumerate(zip(aega, eega)) if a != b]
        raise ValueError(f'{label}: EGA {diffs[:8]} ({len(diffs)})')
    for (kind, start), blob in bodies.items():
        if kind == 'data':
            seg = W
        elif kind in SEG:
            seg = SEG[kind]
        else:
            continue
        at = physical(seg, start); check(eram[at:at + len(blob)] == m[at:at + len(blob)], f'{label}: code written ({kind} {start:04X})')
    return seen
