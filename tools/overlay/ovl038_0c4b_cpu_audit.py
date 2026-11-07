#!/usr/bin/env python3
"""OVL_0038_0C4B (GAME.OVR unit 38h +0C4B, the 009E dispatcher's slot-39 handler): the static source checker.

It proves, from the pinned media read as bytes (nothing is executed; no Unicorn, no DOSBox-X: original and native
execution of this body are a separately authorised review, see docs/re/OVL_0038_0C4B_CPU.md):
  * the unit 38h code identity and that public entry 33 is +0C4B (stub 0041:00C5) and entry 34 starts at +0CC3;
  * the body [0C4B,0CA8): 93 bytes, 37 instructions, its SHA-256, one RETF 0Ah, one forward jump to +0C98, no backward
    jump, no INT, no port access, no multiply, one SHL (AX by 1); the remaining 27 bytes of the 120-byte ledger bound are
    entry 34's 26-character Pascal string (hashed, not quoted);
  * its two calls in order: LCALL 0AF8:0B0B at 0C81 and LCALL 00BA:16EC at 0C8C, and that their segment words are the
    only fixups in the span;
  * the A+3 bit test, the A+3 store, the B+30h increment, the DS:681A read and write, and the CS-relative string pointer
    0C46h the body pushes for StrLoad: a 4-character printable Pascal string at [0C46,0C4B) (right after the +0C26 body);
  * the emitter regenerates the committed C byte for byte (tools/overlay/emit_ovl038_0c4b.py --check).
--compile-only builds the model with its composed source through the audit's own compiler command and binds the symbol,
with no media, and prints CHECKER_BUILD_OK.

python3 tools/overlay/ovl038_0c4b_cpu_audit.py [--compile-only] [--output report.json]"""
import argparse, ctypes, hashlib, json, subprocess, sys, tempfile
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, check
from overlay_inventory import find_units, parse_mz

CHECKER_ROWS = ('OVL_0038_0C4B',)
ENTRY, END, NEXT = 0x0C4B, 0x0CA8, 0x0CC3
STRING = 0x0C46
EXE_SHA = 'd825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd'
OVR_SHA = '872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614'
UNIT_SHA = '1dd5e9d59ea97dad6646a28b2be1e68fd5a2906d578f1eabe0911a5b514a6ee7'
BODY_SHA = '0a0640175f1273dac34d180a437f9eae7d0957b21264332f6f3bc80e19f07575'
STRING_SHA = '83ef31762ecfef759ce52e08b62c41df2fe2040d42d1fe3ceda42c5995bb1aca'
NEXT_STRING_SHA = '17fa53bb9131d10c69ad2371da75a9cebf83ae79e1ab9721f2df08426a15ad18'
SOURCES = ['system_string_leaves_cpu', 'overlay_u38_0c4b_cpu']
CALLS = [(0x0C81, 'lcall', '0xaf8, 0xb0b'), (0x0C8C, 'lcall', '0xba, 0x16ec')]
FIXUPS = (0x0C84, 0x0C8F)
JUMPS = [(0x0C5C, 'jne', '0xc98')]


def media():
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes(); ovr = (ROOT / 'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    check(len(exe) == 65231 and hashlib.sha256(exe).hexdigest() == EXE_SHA, 'START.EXE identity')
    check(len(ovr) == 206105 and hashlib.sha256(ovr).hexdigest() == OVR_SHA, 'GAME.OVR identity')
    return exe, ovr, find_units(exe[parse_mz(exe)['header_bytes']:], ovr)


def source():
    exe, ovr, units = media()
    img = exe[parse_mz(exe)['header_bytes']:]
    u = next(x for x in units if x['stub_para'] == 0x38)
    code = ovr[u['ovr_offset']:u['ovr_offset'] + u['code_size']]
    check(u['ovr_offset'] == 0xEC5B and u['memory_segment'] == 0x41, 'unit 38h file offset and run-time stub segment')
    check(hashlib.sha256(code).hexdigest() == UNIT_SHA, 'unit 38h code identity')
    check(u['entries'][33] == ENTRY and u['entries'][34] == NEXT, 'public entry 33 (stub 0041:00C5) is +0C4B; entry 34 starts at +0CC3')
    check(0x20 + 5 * 33 == 0xC5, 'the loaded stub of entry 33 is 0041:00C5')
    check(img[u['stub_base'] + 5 * 33:u['stub_base'] + 5 * 33 + 4] == b'\xcd\x3f' + ENTRY.to_bytes(2, 'little'),
          'the file stub of entry 33 is INT 3Fh to +0C4B')
    body = code[ENTRY:END]
    check(len(body) == 93 and hashlib.sha256(body).hexdigest() == BODY_SHA, 'body identity')
    t = code[END:NEXT]
    check(len(t) == 27 and t[0] == 26 and hashlib.sha256(t).hexdigest() == NEXT_STRING_SHA and all(0x20 <= b < 0x7F for b in t[1:]),
          'the rest of the 120-byte ledger bound is entry 34\'s 26-character printable Pascal string')
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(len(ins) == 37 and sum(i.size for i in ins) == len(body), 'decode covers the span: 37 instructions')
    check(ins[-1].address == 0x0CA5 and ins[-1].mnemonic == 'retf' and ins[-1].op_str == '0xa'
          and [i.mnemonic for i in ins].count('retf') == 1, 'single RETF 0Ah at 0CA5')
    check([(i.address, i.mnemonic, i.op_str) for i in ins if i.mnemonic in ('call', 'lcall')] == CALLS, 'the two far calls, no near call')
    check(all(fx in u['fixups'] for fx in FIXUPS), 'the two far-call segment words are loader fixups')
    check(not [fx for fx in u['fixups'] if STRING <= fx < NEXT and fx not in FIXUPS], 'no other fixup in the string, the span or the trailing string')
    by = {i.address: i for i in ins}
    check([(i.address, i.mnemonic, i.op_str) for i in ins if i.mnemonic.startswith('j')] == JUMPS, 'one forward jump to +0C98')
    check(not [i for i in ins if i.mnemonic in ('int', 'in', 'out', 'iret', 'ret', 'jmp', 'shr', 'mul', 'div', 'rep')],
          'no INT, port access, RET, JMP, SHR or multiply')
    check([(i.address, i.op_str) for i in ins if i.mnemonic == 'shl'] == [(0x0C9D, 'ax, 1')], 'one SHL: AX by 1 (AF undefined, visible at the RETF)')
    check(by[0x0C54].op_str == 'al, byte ptr es:[di + 3]' and by[0x0C58].op_str == 'al, 0x10' and by[0x0C58].mnemonic == 'and'
          and by[0x0C5A].op_str == 'al, al', 'the A+3 bit-4 test')
    check(by[0x0C67].mnemonic == 'add' and by[0x0C67].op_str == 'ax, 0x10' and by[0x0C6D].mnemonic == 'mov'
          and by[0x0C6D].op_str == 'byte ptr es:[di + 3], al', 'the A+3 store')
    check(by[0x0C94].mnemonic == 'inc' and by[0x0C94].op_str == 'word ptr es:[di + 0x30]', 'the B+30h increment (after the stop)')
    check(by[0x0C98].op_str == 'al, byte ptr [0x681a]' and by[0x0C9F].op_str == 'byte ptr [0x681a], al'
          and not [i for i in ins if 'ptr [0x6' in i.op_str and i.address not in (0x0C98, 0x0C9F)], 'DS:681A is the only DS access')
    check(by[0x0C7C].mnemonic == 'mov' and by[0x0C7C].op_str == f'di, 0x{STRING:x}' and by[0x0C7F].op_str == 'cs', 'the CS string pointer 0C46h')
    s = code[STRING:ENTRY]
    check(len(s) == 5 and s[0] == 4 and hashlib.sha256(s).hexdigest() == STRING_SHA and all(0x20 <= b < 0x7F for b in s[1:]),
          'a 4-character printable Pascal string at [0C46,0C4B)')
    check(u['entries'][32] == 0x0C26 and code[0x0C43:0x0C46] == b'\xca\x0a\x00', 'the string follows the +0C26 body\'s RETF 0Ah')
    return dict(unit_sha256=UNIT_SHA, span=f'unit 38h +[{ENTRY:04X},{END:04X})', bytes=len(body), instructions=len(ins),
                body_sha256=BODY_SHA, string_sha256=STRING_SHA, string_length=s[0], trailing_string_sha256=NEXT_STRING_SHA,
                calls=[f'{a:04X} {m} {o}' for a, m, o in CALLS], fixups=[f'{fx:04X}' for fx in FIXUPS],
                stubs={'0041:00C5': f'U38 +{ENTRY:04X}'}, dispatcher_slot='39 (DS:68C4), per OVL_009E_0000_HANDLERS.md')


def library(directory):
    path = directory / 'u38_0c4b.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include')] +
                   [str(ROOT / f'src/recovered/{s}.c') for s in SOURCES] + ['-o', str(path)], check=True)
    f = ctypes.CDLL(str(path)).pr_u38_handler_0c4b_cpu
    f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_void_p]
    f.restype = ctypes.c_int
    return f


def emitter_check():
    r = subprocess.run([sys.executable, str(ROOT / 'tools/overlay/emit_ovl038_0c4b.py'), '--check'], capture_output=True, text=True)
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
