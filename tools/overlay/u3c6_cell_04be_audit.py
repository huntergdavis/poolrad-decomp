#!/usr/bin/env python3
"""Original-byte parity for standalone U3C6 +04BE, retaining declared stops."""
import argparse
import ctypes
from hashlib import sha256
import json
from pathlib import Path
import random
import subprocess
import tempfile
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_SP, UC_X86_REG_SS, UC_X86_REG_BP, UC_X86_REG_ES, UC_X86_REG_DI
from overlay_inventory import find_units, parse_mz
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, OVR_SHA, REGISTERS, physical, put16

START, END = 0x4BE, 0x52B
BODY_SHA = 'bf1db9df7666fe7c5615d826c14442fd10e08f768289f65ebfd3ac10daf2027e'
MASK = SIZE - 1
LES = {0x4F4: 6, 0x511: 10, 0x519: 10, 0x520: 6}


def source():
    exe = (ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes()
    ovr = (ROOT/'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    assert len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA
    assert len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA
    units = find_units(exe[parse_mz(exe)['header_bytes']:], ovr)
    u = next(u for u in units if u['stub_para'] == 0x3C6)
    assert u['ovr_offset'] == 198458 and u['memory_segment'] == 0x3E0 and u['entries'][6] == START
    code = ovr[u['ovr_offset']+START:u['ovr_offset']+END]
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(code, START))
    assert len(code) == 109 and sha256(code).hexdigest() == BODY_SHA
    assert len(ins) == 42 and sum(i.size for i in ins) == 109
    assert not any(i.mnemonic.startswith('call') or i.mnemonic == 'lcall' for i in ins)
    assert not any(START <= f < END for f in u['fixups'])
    assert [(i.address, i.mnemonic, i.op_str) for i in ins if i.mnemonic == 'retf'] == [(0x528, 'retf', '0xc')]
    return code, {i.address: i for i in ins}


CHECKER_ROWS = ('OVL_03C6_04BE',)


def library(directory):
    p = directory/'cell.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                    '-fPIC', '-shared', '-I', str(ROOT/'include'),
                    str(ROOT/'src/recovered/overlay_u3c6_04be_cpu.c'), '-o', str(p)], check=True)
    lib = ctypes.CDLL(str(p))
    lib.pr_u3c6_04be_cpu_run.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_int]
    lib.pr_u3c6_04be_cpu_run.restype = ctypes.c_int
    return lib


def fixture(*, x=3, y=4, a_value=0x91, b_value=0x37, ss=0x2000, sp=0x8000,
            ds=0x1000, cs=0x6000, map_ptr=(0x4000, 0x100),
            a=(0x5000, 0x100), b=(0x5000, 0x110), flags=0x202):
    memory = bytearray([0xA5])*SIZE
    cpu = Cpu(0xABCD, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777,
              sp, cs, START, ds, 0x1111, ss, flags)
    index = ((y&255)*50 + (x&255)) & 65535
    memory[physical(map_ptr[0], (map_ptr[1]+index+7)&65535)] = a_value
    memory[physical(ds, (0x60DB+index)&65535)] = b_value
    put16(memory, ds, 0x6716, map_ptr[1])
    put16(memory, ds, 0x6718, map_ptr[0])
    for off, value in ((0, 0x9000), (2, 0x7000), (4, a[1]), (6, a[0]),
                       (8, b[1]), (10, b[0]), (12, 0xA500|(y&255)), (14, 0x5A00|(x&255))):
        put16(memory, ss, (sp+off)&65535, value)
    return memory, cpu


def compare(label, initial, code, decoded, lib, policy=1):
    memory, cpu = initial
    # A20 wrapping applies to code placement too.
    for i, value in enumerate(code):
        memory[physical(cpu.cs, START+i)] = value
    backing = (ctypes.c_uint8*SIZE).from_buffer_copy(memory)
    uc = Uc(UC_ARCH_X86, UC_MODE_16)
    uc.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing))
    uc.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        uc.reg_write(reg, getattr(cpu, name))
    visited, edges = set(), set()
    previous, stop = [None], [None]
    def in_code(p):
        return ((p-physical(cpu.cs, START))&MASK) < len(code)
    def hook(uc, address, size, user):
        ip = uc.reg_read(UC_X86_REG_IP)
        assert uc.reg_read(UC_X86_REG_CS) == cpu.cs and ip in decoded, (label, 'PC', hex(address))
        if ip == START:
            at = physical(uc.reg_read(UC_X86_REG_SS), (uc.reg_read(UC_X86_REG_SP)-2)&65535)
            if in_code(at) or in_code((at+1)&MASK):
                stop[0] = 2; uc.emu_stop(); return
        if ip in (0x4F7, 0x514, 0x51C, 0x523):
            at = physical(uc.reg_read(UC_X86_REG_ES), uc.reg_read(UC_X86_REG_DI))
            if in_code(at):
                stop[0] = 2; uc.emu_stop(); return
        if ip in LES and ((uc.reg_read(UC_X86_REG_BP)+LES[ip])&65535) > 0xFFFC:
            stop[0] = 4; uc.emu_stop(); return
        if previous[0] is not None:
            edges.add((previous[0], ip))
        ins = decoded[ip]
        previous[0] = ip if ins.mnemonic.startswith('j') and ins.mnemonic != 'jmp' else None
        visited.add(ip)
        if ip == 0x528:
            stop[0] = 3 if policy == 0 else 0
            uc.emu_stop()
    handle = uc.hook_add(UC_HOOK_CODE, hook)
    uc.emu_start(physical(cpu.cs, START), 2*SIZE, count=128)
    assert stop[0] is not None, (label, 'original budget')
    wrapped = False
    if stop[0] == 0:
        sp, ss = uc.reg_read(UC_X86_REG_SP), uc.reg_read(UC_X86_REG_SS)
        at = physical(ss, (sp+2)&65535)
        cs = backing[at] | (backing[(at+1)&MASK]<<8)
        uc.hook_del(handle)
        uc.emu_start(physical(cpu.cs, 0x528), 2*SIZE, count=1)
        if policy == 1 and sp >= 0xFFFE:
            uc.reg_write(UC_X86_REG_CS, cs)
            wrapped = True
    assert all(backing[physical(cpu.cs, START+i)] == value for i, value in enumerate(code)), (label, 'code changed')
    ram = (ctypes.c_uint8*SIZE).from_buffer_copy(memory)
    actual = Cpu.from_buffer_copy(cpu)
    status = lib.pr_u3c6_04be_cpu_run(ram, SIZE, ctypes.byref(actual), policy)
    assert status == stop[0], (label, 'status', status, stop[0])
    assert bytes(ram) == bytes(backing), (label, 'RAM', [i for i in range(SIZE) if ram[i] != backing[i]][:10])
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        expected = uc.reg_read(reg)&65535
        assert getattr(actual, name) == expected, (label, name, hex(getattr(actual, name)), hex(expected))
    return visited, edges, status, wrapped, actual.ip


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--quick', action='store_true')
    p.add_argument('--output', type=Path)
    p.add_argument('--compile-only',action='store_true');args=p.parse_args()
    if args.compile_only:  # same compiler command and symbol binding as the audit; no media
        with tempfile.TemporaryDirectory(prefix='poolrad-checker-') as _t:
            library(Path(_t))
        print('CHECKER_BUILD_OK')
        return
    code, decoded = source()
    visited, edges = set(), set()
    count = wrapped = 0
    statuses, policies, stops = {}, {}, {}
    with tempfile.TemporaryDirectory() as tmp:
        lib = library(Path(tmp))
        def run(label, policy=1, **kw):
            nonlocal count, wrapped
            v, e, status, w, ip = compare(label, fixture(**kw), code, decoded, lib, policy)
            visited.update(v); edges.update(e)
            count += 1; wrapped += w
            statuses[str(status)] = statuses.get(str(status), 0)+1
            policies[str(policy)] = policies.get(str(policy), 0)+1
            if status:
                key = f'{status}:{ip:04X}'
                stops[key] = stops.get(key, 0)+1
        for x in (range(50) if not args.quick else (0, 3, 49)):
            for y in (range(25) if not args.quick else (0, 4, 24)):
                run(f'cell-{x}-{y}', x=x, y=y, a_value=(x+3*y)&255, b_value=(5*x+y)&255)
        for policy in (1, 2):
            for x in (0, 1, 49, 50, 127, 128, 255):
                for y in (0, 1, 24, 25, 127, 128, 255):
                    run(f'bounds-{policy}-{x}-{y}', policy=policy, x=x, y=y)
            for sp in (0, 1, 2, 0xFFF4, 0xFFF5, 0xFFF6, 0xFFF7, 0xFFF8,
                       0xFFF9, 0xFFFA, 0xFFFB, 0xFFFC, 0xFFFD, 0xFFFE, 0xFFFF):
                for x in (3, 128):
                    run(f'frame-{policy}-{sp:X}-{x}', policy=policy, sp=sp, x=x)
            for a in ((0x2000, 0x800C), (0x2000, 0x800E), (0x2000, 0x8008),
                      (0x2000, 0x800A), (0x2000, 0x7FFE), (0x2000, 0x8000),
                      (0x2000, 0x8002), (0x5000, 0x110), (0x1000, 0x6716)):
                run(f'live-alias-{policy}-{a}', policy=policy, a=a, a_value=0x80)
        for x in (3, 128):
            for ptr in ((0x6000, START), (0x6000, END-1), (0x6000, START-1), (0x6000, END)):
                run(f'alias-A-{x}-{ptr}', x=x, a=ptr)
                run(f'alias-B-{x}-{ptr}', x=x, b=ptr)
            for sp in (START+1, START+2, END+1, START):
                run(f'alias-push-{x}-{sp}', x=x, ss=0x6000, sp=sp)
            for sp in (0x8000, 0xFFFD, 0xFFFE, 0xFFFF):
                run(f'pending-{x}-{sp}', policy=0, x=x, sp=sp)
        for offset in (0, 0xF000, 0xFFF0, 0xFFFF):
            run(f'map-offset-{offset}', map_ptr=(0x4000, offset))
        for ptr in ((0xFFFF, 0xF), (0xFFFF, 0x10), (0xFFFF, 0xFFFF)):
            run(f'A20-map-{ptr}', map_ptr=ptr)
            run(f'A20-output-{ptr}', a=ptr, b=(0xFFFF, (ptr[1]+1)&65535))
        run('A20-code', cs=0xFFFF)
        rng = random.Random(0x3C604BE)
        for i in range(30 if args.quick else 400):
            run(f'random-{i}', policy=1+i%2, x=rng.randrange(256), y=rng.randrange(256),
                a_value=rng.randrange(256), b_value=rng.randrange(256),
                map_ptr=(0x4000, rng.randrange(65536)),
                flags=rng.choice((0x202, 0xED7, 0x47, 0xA93)))
    required = {(i.address, t) for i in decoded.values() if i.mnemonic.startswith('j') and i.mnemonic != 'jmp'
                for t in (int(i.op_str, 16), i.address+i.size)}
    report = dict(target='OVL_03C6_04BE', cases=count, whole_ram_bytes=SIZE, cpu_words=14,
                  covered_sites=len(visited), source_sites=len(decoded), conditional_edges=len(edges),
                  required_edges=len(required), missing_sites=sorted(set(decoded)-visited),
                  missing_edges=sorted(required-edges), wrapped_retf_hybrid_cases=wrapped,
                  wrapped_retf_substitutions=wrapped, masked_flags=0, status_counts=statuses,
                  policy_counts=policies, stop_counts=stops, executed_les_seams_measured=False,
                  natural_game_route_measured=False)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, indent=2, sort_keys=True)+'\n')
    print(json.dumps(report, sort_keys=True))
    assert not report['missing_sites'] and not report['missing_edges'], 'incomplete coverage'


if __name__ == '__main__':
    main()
