#!/usr/bin/env python3
"""OVL_0038_0775 (GAME.OVR unit 38h +0775, the 009E dispatcher's slot-22 handler): the static source checker.

It proves, from the pinned media read as bytes (nothing is executed; no Unicorn, no DOSBox-X: original and native
execution of this body are a separately authorised review, see docs/re/OVL_0038_0775_CPU.md):
  * the unit 38h code identity and that public entry 20 is +0775 (stub 0041:0084);
  * the body [0775,07D2): 93 bytes, 40 instructions, its SHA-256, one RETF 0Ah, no backward jump, no INT, no port access;
  * its four calls: LCALL 00BA:2199 at 0789, LCALL 0AF8:0B0B at 07A5, PUSH CS; CALL +0058 at 07AA/07AB, LCALL 00B0:002A at
    07C2, and that the three far-call segment words are loader fixups;
  * the two DS writes (681Fh) and the CS-relative string pointer 0764h the body pushes for StrLoad: a 16-character
    Pascal string at [0764,0775) (length byte 10h), hashed, not quoted;
  * the 14 bytes [07D2,07E0) that the queue's 107-byte bound includes are the next entry's string, not code of this body;
  * unit 9E's descriptor: run-time stub segment 00B0, entry 2 (stub 00B0:002A) is +0026, the composed list-node removal;
  * the emitter regenerates the committed C byte for byte (tools/overlay/emit_ovl038_0775.py --check).
--compile-only builds the model with its four composed sources through the audit's own compiler command and binds the
symbol, with no media, and prints CHECKER_BUILD_OK.

python3 tools/overlay/ovl038_0775_cpu_audit.py [--compile-only] [--output report.json]"""
import argparse, ctypes, hashlib, json, subprocess, sys, tempfile
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, check
from overlay_inventory import find_units, parse_mz

CHECKER_ROWS = ('OVL_0038_0775',)
ENTRY, END, BOUND = 0x0775, 0x07D2, 0x07E0
STRING = 0x0764
EXE_SHA = 'd825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd'
OVR_SHA = '872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614'
UNIT_SHA = '1dd5e9d59ea97dad6646a28b2be1e68fd5a2906d578f1eabe0911a5b514a6ee7'
BODY_SHA = '6173186b01c7030bf0becfca1ad439d5eba8da5bf5f6526dbef8200c3a1e817b'
STRING_SHA = '99631419966cd7537dc59fc18969c3026310ffdcc49d2455294ea7710e363927'
SOURCES = ['resident_00ba_sidecar_lookup_2199_cpu', 'system_string_leaves_cpu', 'system_freemem_0254_cpu',
           'overlay_u9e_list_release_0026_cpu', 'overlay_u38_handler_0775_cpu']
CALLS = [(0x0789, 'lcall', '0xba, 0x2199'), (0x07A5, 'lcall', '0xaf8, 0xb0b'), (0x07AB, 'call', '0x58'),
         (0x07C2, 'lcall', '0xb0, 0x2a')]
FIXUPS = (0x078C, 0x07A8, 0x07C5)


def media():
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes(); ovr = (ROOT / 'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    check(len(exe) == 65231 and hashlib.sha256(exe).hexdigest() == EXE_SHA, 'START.EXE identity')
    check(len(ovr) == 206105 and hashlib.sha256(ovr).hexdigest() == OVR_SHA, 'GAME.OVR identity')
    units = find_units(exe[parse_mz(exe)['header_bytes']:], ovr)
    return exe, ovr, units


def source():
    exe, ovr, units = media()
    u = next(x for x in units if x['stub_para'] == 0x38)
    code = ovr[u['ovr_offset']:u['ovr_offset'] + u['code_size']]
    check(u['ovr_offset'] == 0xEC5B and u['memory_segment'] == 0x41, 'unit 38h file offset and run-time stub segment')
    check(hashlib.sha256(code).hexdigest() == UNIT_SHA, 'unit 38h code identity')
    check(u['entries'][20] == ENTRY, 'unit 38h public entry 20 (stub 0041:0084) is +0775')
    img = exe[parse_mz(exe)['header_bytes']:]
    check(img[u['stub_base'] + 5 * 20:u['stub_base'] + 5 * 20 + 4] == b'\xcd\x3f' + ENTRY.to_bytes(2, 'little'),
          'the file stub of entry 20 is INT 3Fh to +0775')
    body = code[ENTRY:END]
    check(len(body) == 93 and hashlib.sha256(body).hexdigest() == BODY_SHA, 'body identity')
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(len(ins) == 40 and sum(i.size for i in ins) == len(body), 'decode covers the span: 40 instructions')
    check(ins[-1].address == 0x07CF and ins[-1].mnemonic == 'retf' and ins[-1].op_str == '0xa'
          and [i.mnemonic for i in ins].count('retf') == 1, 'single RETF 0Ah at 07CF')
    check([(i.address, i.mnemonic, i.op_str) for i in ins if i.mnemonic in ('call', 'lcall')] == CALLS, 'the four calls')
    check(all(fx in u['fixups'] for fx in FIXUPS), 'the three far-call segment words are loader fixups')
    check(not [fx for fx in u['fixups'] if ENTRY <= fx < BOUND and fx not in FIXUPS], 'no other fixup in the span')
    by = {i.address: i for i in ins}
    check(by[0x07AA].mnemonic == 'push' and by[0x07AA].op_str == 'cs', 'PUSH CS before the near call')
    jumps = [i for i in ins if i.mnemonic.startswith('j')]
    check([(i.address, i.mnemonic, i.op_str) for i in jumps] == [(0x0790, 'je', '0x7ae')], 'one forward conditional jump')
    check(not [i for i in ins if i.mnemonic in ('int', 'in', 'out', 'iret', 'ret', 'jmp', 'shl', 'shr', 'mul', 'div')],
          'no INT, port access, RET, JMP, shift or multiply')
    check([(i.address, i.op_str) for i in ins if i.mnemonic == 'mov' and 'ptr [0x681f]' in i.op_str]
          == [(0x07AE, 'byte ptr [0x681f], 1'), (0x07C7, 'byte ptr [0x681f], 0')], 'the two DS:681F writes')
    check(by[0x07A0].mnemonic == 'mov' and by[0x07A0].op_str == f'di, 0x{STRING:x}', 'the CS string pointer 0764h')
    s = code[STRING:ENTRY]
    check(len(s) == 17 and s[0] == 16 and hashlib.sha256(s).hexdigest() == STRING_SHA and all(0x20 <= b < 0x7F for b in s[1:]),
          'a 16-character printable Pascal string at [0764,0775)')
    tail = code[END:BOUND]
    check(len(tail) == 14 and tail[0] == 13 and all(0x20 <= b < 0x7F for b in tail[1:]),
          'the 14 bytes [07D2,07E0) inside the 107-byte bound are a 13-character Pascal string (the next entry\'s), not code')
    check(u['entries'][21] == BOUND, 'the next public entry (21) starts at 07E0')
    u9e = next(x for x in units if x['stub_para'] == 0x9E)
    check(u9e['memory_segment'] == 0xB0 and u9e['entries'][2] == 0x0026, 'unit 9E stub 00B0:002A (entry 2) is +0026')
    check(img[u9e['stub_base'] + 10:u9e['stub_base'] + 14] == b'\xcd\x3f\x26\x00', 'the file stub of U9E entry 2 is INT 3Fh to +0026')
    return dict(unit_sha256=UNIT_SHA, span=f'unit 38h +[{ENTRY:04X},{END:04X})', bytes=len(body), instructions=len(ins),
                body_sha256=BODY_SHA, bound_bytes=BOUND - ENTRY, string_sha256=STRING_SHA, string_length=s[0],
                calls=[f'{a:04X} {m} {o}' for a, m, o in CALLS], fixups=[f'{fx:04X}' for fx in FIXUPS],
                stubs={'0041:0084': f'U38 +{ENTRY:04X}', '00B0:002A': 'U9E +0026'})


def library(directory):
    path = directory / 'u38_0775.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include')] +
                   [str(ROOT / f'src/recovered/{s}.c') for s in SOURCES] + ['-o', str(path)], check=True)
    f = ctypes.CDLL(str(path)).pr_u38_handler_0775_cpu
    f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_void_p, ctypes.c_void_p]
    f.restype = ctypes.c_int
    return f


def emitter_check():
    r = subprocess.run([sys.executable, str(ROOT / 'tools/overlay/emit_ovl038_0775.py'), '--check'], capture_output=True, text=True)
    check(r.returncode == 0 and 'EMIT_CHECK OK' in r.stdout, 'the emitter regenerates the committed C byte for byte')
    return r.stdout.strip().splitlines()


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--compile-only', action='store_true')
    p.add_argument('--output', type=Path); a = p.parse_args()
    with tempfile.TemporaryDirectory() as tmp:
        library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
    report = dict(body=source(), emitter=emitter_check(), executed=False,
                  note='static source checks only; no original or native execution (separately authorised)')
    text = json.dumps(report, indent=1, sort_keys=True)
    if a.output:
        a.output.parent.mkdir(parents=True, exist_ok=True); a.output.write_text(text + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    main()
