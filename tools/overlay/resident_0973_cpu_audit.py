#!/usr/bin/env python3
"""Original-byte full-state parity for START.EXE resident 00BA:0973 (86 B / 32 instructions),
composing the accepted 00BA:1037 formatter and 0709:0626 printer. The world is the real
expanded START.EXE at load 0, DS:5E20 font and printer tables, A0000 as RAM for the
original; the C model runs the same body with the C 1037 and printer models. Compares
CPU14, all RAM outside A0000..AFFFF, the ordered OUT events and the ordered A0000 writes.
Media stays private."""
import argparse, ctypes, json, random, struct, subprocess, tempfile
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from resident_loader_map import unpack
import res0709_printer_0626_cpu_audit as P
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, REGISTERS, physical, put16

def check(condition, message):
    if not condition:
        raise ValueError(message)

CS, DS, SS = 0x00BA, 0x5000, 0x2000
ENTRY, END, RETF = 0x0973, 0x09C9, 0x09C6
FMT_ENTRY, FMT_END = 0x1037, 0x1073
NODE_SEG, NODE_OFF = 0x6000, 0x1000
RETURN_TO = (0x9001, 0x7000)
FONT_SEG, FONT_OFF = 0x7000, 0x1000
FLAGS = (0x0602, 0x0202, 0x0ED7, 0x0647, 0x72D7)
CHECKER_ROWS = ('RES_00BA_0973',)


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, "START.EXE provenance differs")
    image = bytes(unpack(exe)[0])
    body = image[physical(CS, ENTRY):physical(CS, END)]
    md = Cs(CS_ARCH_X86, CS_MODE_16)
    ins = list(md.disasm(body, ENTRY))
    check(len(body) == 86 and len(ins) == 32 and sum(i.size for i in ins) == 86 and ins[-1].address == RETF
          and ins[-1].mnemonic == "retf" and ins[-1].op_str == "0xa", "RES_00BA_0973 extent differs")
    fmt = image[physical(CS, FMT_ENTRY):physical(CS, FMT_END)]
    fins = list(md.disasm(fmt, FMT_ENTRY))
    check(len(fmt) == 60 and len(fins) == 28 and fins[-1].address == 0x1070 and fins[-1].mnemonic == "retf",
          "00BA:1037 extent differs")
    cond = {i.address: (i.address + i.size, int(i.op_str, 16)) for i in ins if i.mnemonic.startswith("j") and i.mnemonic != "jmp"}
    starts = [i.address for i in ins]
    print(f"RES_00BA:0973 ({len(body)} bytes / {len(ins)} instructions, {len(cond)} conditionals) sha {sha256(body).hexdigest()}")
    return dict(conditional=cond, starts=starts), image


def library(directory):
    path = directory / "r0973.so"
    sources = ["resident_00ba_0973_cpu.c", "resident_00ba_byte_string_1037_cpu.c", "system_str_1366_cpu.c"] + \
              [name + ".c" for name in P.SOURCES]
    built = subprocess.run(["cc", "-std=c11", "-O2", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                            *[str(ROOT / "src/recovered" / s) for s in sources], "-o", str(path)],
                           capture_output=True, text=True)
    check(built.returncode == 0, built.stderr[-2000:])
    lib = ctypes.CDLL(str(path))
    lib.pr_ega_reset.argtypes = [ctypes.POINTER(P.Ega)]
    f = lib.pr_res_00ba_0973_cpu
    f.argtypes = [ctypes.POINTER(P.Bus), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_void_p, ctypes.c_void_p]
    f.restype = ctypes.c_int
    return lib, f


def build(name, *, first=0x10, threshold=0x20, flag=0, x=5, y=2, sp=0x8000, flags=0x0202, high=0xA500):
    rng = random.Random(f"RES_00BA_0973/{name}")
    m = bytearray([0xA5]) * SIZE
    m[0:len(IMAGE)] = IMAGE
    put16(m, DS, 0x4A16, 0xA000); put16(m, DS, 0x4A18, 0xA200)
    put16(m, DS, 0x5E20, FONT_OFF); put16(m, DS, 0x5E22, FONT_SEG)
    for j in range(2048):
        m[physical(FONT_SEG, FONT_OFF + j)] = ((j * 37 + (j >> 3) * 13) ^ 0xA6) & 255
    for j, v in enumerate((1, 2, 4, 8)):
        m[physical(DS, 0x25C9 + j)] = v
    for j, v in enumerate((0, 0x5555, 0xAAAA, 0xFFFF)):
        put16(m, DS, 0x25CD + 2 * j, v)
    for j in range(256):
        m[physical(DS, 0x25D5 + j)] = j % 4
    for j in range(8):
        put16(m, DS, 0x25E5 + 2 * j, 0xC000 >> (2 * j))
    m[physical(DS, 0x5E24)], m[physical(DS, 0x5E25)] = 1, 1
    m[physical(DS, 0x4A00)] = 0; m[physical(DS, 0x49E2)] = 0; put16(m, DS, 0x84FC, 2); m[0x400] = 0x42
    m[physical(DS, 0x4A1E)] = 1
    m[physical(NODE_SEG, (NODE_OFF + 0x11B) & 0xFFFF)] = first & 0xFF
    m[physical(NODE_SEG, (NODE_OFF + 0x32) & 0xFFFF)] = threshold & 0xFF
    frame = (RETURN_TO[1], RETURN_TO[0], high | flag, high | y, high | x, NODE_OFF, NODE_SEG)
    for k, v in enumerate(frame):
        put16(m, SS, (sp + 2 * k) & 0xFFFF, v)
    c = Cpu(0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, sp, CS, ENTRY,
            DS, 0x1111, SS, (flags | 0x0002) & ~0x0100)
    return m, c


def run_original(name, m, c, spec):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_INSN, UC_HOOK_MEM_WRITE
    from unicorn import x86_const as R
    uc = Uc(UC_ARCH_X86, UC_MODE_16)
    backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    uc.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing)); uc.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for (n, _), r in zip(Cpu._fields_, REGISTERS): uc.reg_write(r, getattr(c, n))
    st = dict(stop=None, returned=False, error=None)
    seen, edges, events, writes = set(), set(), [], []
    prev = [None]
    def stop(v): st['stop'] = v; uc.emu_stop()
    def hook(em, address, size, user):
        cs, ip = uc.reg_read(R.UC_X86_REG_CS), uc.reg_read(R.UC_X86_REG_IP)
        if st['returned']:
            st['stop'] = 0; return uc.emu_stop()
        if cs not in (CS, 0x0AF8, 0x0709):
            st['error'] = (cs, ip); return stop(99)
        if cs == CS and ip == RETF:
            st['returned'] = True
        if cs == CS and ip == 0x09BB and bytes(uc.mem_read(address, 3)) != bytes.fromhex('e87906'):
            st['error'] = 'near-call'; return stop(2)
        if cs == CS and ip == 0x09BE and bytes(uc.mem_read(address, 5)) != bytes.fromhex('9a26060907'):
            st['error'] = 'far-call'; return stop(2)
        if cs == CS and ip in spec['starts']:
            if prev[0] in spec['conditional'] and prev[0] != ip:
                edges.add((prev[0], ip))
            seen.add(ip); prev[0] = ip if spec['conditional'] else None
        else:
            prev[0] = None
        return None
    def out(em, port, size, value, user): events.append((port, value, size))
    def write(em, access, address, size, value, user):
        for j in range(size):
            at = (address + j) & (SIZE - 1)
            if 0xA0000 <= at < 0xB0000:
                writes.append((at - 0xA0000, (value >> (8 * j)) & 255))
    uc.hook_add(UC_HOOK_CODE, hook)
    uc.hook_add(UC_HOOK_INSN, out, None, 1, 0, R.UC_X86_INS_OUT)
    uc.hook_add(UC_HOOK_MEM_WRITE, write)
    uc.emu_start(physical(c.cs, c.ip), 2 * SIZE, count=3_000_000)
    check(st['error'] is None and st['stop'] is not None, f"original did not stop cleanly: {name} {st}")
    final = {n: uc.reg_read(r) & 0xFFFF for (n, _), r in zip(Cpu._fields_, REGISTERS)}
    return bytes(backing), final, seen, edges, events, writes


def run_c(f, lib, m, c, limit=65536, glyph=65536):
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    ega = P.Ega(); lib.pr_ega_reset(ctypes.byref(ega))
    for p in range(4): ctypes.memset(ctypes.addressof(ega.plane[p]), 0x31 + p * 0x27, 65536)
    events, writes = [], []
    def out(ctx, port, value, size, cs, ip):
        events.append((port, value, size)); return 0
    def wr(ctx, off, value): writes.append((off, value))
    env = P.Env(None, P.OUT_FN(out), P.IN_FN())
    bus = P.Bus(ram, ctypes.pointer(ega), ctypes.pointer(env), P.WRITE_FN(wr), None)
    c2 = Cpu(*[getattr(c, n) for n, _ in Cpu._fields_])
    status = f(ctypes.byref(bus), SIZE, ctypes.byref(c2), None, None)
    return status, bytes(ram), events, writes, c2


def compare(name, m, c, spec, lib, f):
    expected, final, seen, edges, events, writes = run_original(name, m, c, spec)
    status, actual, aevents, awrites, c2 = run_c(f, lib, m, c)
    check(status == 0, f"C status {status}: {name}")
    diffs = [i for i in range(SIZE) if not (0xA0000 <= i < 0xB0000) and actual[i] != expected[i]]
    if diffs:
        raise ValueError(f"RAM mismatch {name}: {[(hex(d), actual[d], expected[d]) for d in diffs[:16]]} ({len(diffs)} bytes) CPU={ {n:(getattr(c2,n),final[n]) for n,_ in Cpu._fields_ if getattr(c2,n)!=final[n]} }")
    for n, _ in Cpu._fields_:
        check(getattr(c2, n) == final[n], f"CPU mismatch {name}/{n}: {getattr(c2, n):04x}/{final[n]:04x}")
    check(aevents == events, f"OUT mismatch {name}: {aevents[:4]} / {events[:4]}")
    check(awrites == writes, f"VRAM mismatch {name}: {awrites[:4]} / {writes[:4]} ({len(awrites)}/{len(writes)})")
    return seen, edges, len(events), len(writes)


def cases(quick=False):
    out = []
    for first, threshold, tag in ((0x10, 0x20, 'below'), (0x20, 0x20, 'equal'), (0x21, 0x20, 'above'), (0, 0, 'zero'), (255, 0, 'max-first'), (0, 255, 'max-threshold')):
        for flag in (0, 1):
            out.append((f'{tag}-flag{flag}', dict(first=first, threshold=threshold, flag=flag)))
    out += [('x0-y0', dict(x=0, y=0)), ('x39-y24', dict(x=39, y=24)), ('sp-edge', dict(sp=0x8100))]
    if quick: out = out[:8]
    return out


def main():
    global IMAGE
    ap = argparse.ArgumentParser(); ap.add_argument('--quick', action='store_true'); ap.add_argument('--compile-only', action='store_true')
    ap.add_argument('--unicorn', action='store_true'); ap.add_argument('--output', type=Path)
    a = ap.parse_args()
    spec, IMAGE = source()
    with tempfile.TemporaryDirectory(prefix='poolrad-0973-') as d:
        lib, f = library(Path(d))
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        report = dict(target='RES_00BA_0973', cases=0, ram_bytes=SIZE - 65536, cpu_words=14, flag_masks=0)
        seen, edges, outs, vram = set(), set(), 0, 0
        for name, opts in cases(a.quick):
            m, c = build(name, **opts)
            s, e, o, w = compare(name, m, c, spec, lib, f)
            seen |= s; edges |= e; outs += o; vram += w; report['cases'] += 1
        outcomes = {(a, b) for a, dests in spec['conditional'].items() for b in dests}
        report.update(caller_starts=len(seen), caller_starts_total=32, caller_outcomes=len(edges),
                      uncovered_starts=[f'{ip:04X}' for ip in sorted(set(spec['starts']) - seen)],
                      uncovered_outcomes=[f'{a:04X}->{b:04X}' for a, b in sorted(outcomes - edges)],
                      out_events=outs, vram_write_operands=vram)
        text = json.dumps(report, indent=1, sort_keys=True)
        if a.output: a.output.parent.mkdir(parents=True, exist_ok=True); a.output.write_text(text + '\n')
        print(text)


if __name__ == '__main__':
    main()
