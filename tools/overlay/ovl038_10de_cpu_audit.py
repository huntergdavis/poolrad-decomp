#!/usr/bin/env python3
"""OVL_0038_10DE (GAME.OVR unit 38h +10DE, the 009E dispatcher's slot-43 handler): the static source checker.

It proves, from the pinned media read as bytes (nothing is executed; no Unicorn, no DOSBox-X: original and native
execution of this body are a separately authorised review, see docs/re/OVL_0038_10DE_CPU.md):
  * the unit 38h code identity and that public entry 37 is +10DE (stub 0041:00D9) and entry 38 starts at +1168;
  * the body [10DE,1168): 138 bytes (the whole ledger bound), 60 instructions, its SHA-256, one RETF 0Ah, three forward
    conditional jumps and one forward JMP to the epilogue +1162, no backward jump, no INT, no port access, no shift or
    multiply (no undefined flag);
  * its five calls in order: PUSH CS; CALL +001F at 10F9/10FA, LCALL 0AF8:0B0B at 111B, LCALL 00BA:16EC at 1126,
    LCALL 00BA:2199 at 1142, LCALL 00B0:0052 at 115D, and that the four far-call segment words are the only fixups in the span;
  * the argument pushes of each call (the 2Bh, A+3 and 3Ch words for +001F; the 1Fh key and the [BP-4] cursor for 2199;
    the 1Fh, 0, 00FFh and 0 words for +0E52), the B+10h test and decrement, the CS-relative string pointer 10D2h the body
    pushes for StrLoad: an 11-character printable Pascal string at [10D2,10DE) (right after the +10C1 body's RETF), hashed,
    not quoted;
  * unit 38h +001F is the credited gated append (body SHA-256 pinned by its own audit) and unit 9E's descriptor: run-time
    stub segment 00B0, entry 10 (stub 00B0:0052) is +0E52 (file stub INT 3Fh to +0E52);
  * the emitter regenerates the committed C byte for byte (tools/overlay/emit_ovl038_10de.py --check).
--compile-only builds the model with its composed sources through the audit's own compiler command and binds the symbol,
with no media, and prints CHECKER_BUILD_OK.

python3 tools/overlay/ovl038_10de_cpu_audit.py [--compile-only] [--output report.json]"""
import argparse, ctypes, hashlib, json, subprocess, sys, tempfile
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, check
from overlay_inventory import find_units, parse_mz

CHECKER_ROWS = ('OVL_0038_10DE',)
ENTRY, END = 0x10DE, 0x1168
STRING = 0x10D2
EXE_SHA = 'd825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd'
OVR_SHA = '872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614'
UNIT_SHA = '1dd5e9d59ea97dad6646a28b2be1e68fd5a2906d578f1eabe0911a5b514a6ee7'
BODY_SHA = '814e17d09ed4b1022515a5c2e1f9933048504b1b1ff44c6e6cddba49caa58b95'
STRING_SHA = '39a03f21e7695e900e25fbc99a518f32499abb8a6b92b5498da04c61c04f9250'
HELPER_001F_SHA = '342e00b6d9cd4b29bce77746ca25f02637040d3514807fe6063ddaa175bd23f8'
U9E_0E52_SHA = 'cfde5096f098f1f0c4aa15f805eb6a02c3540f56a2f0df8632b99501a6366b20'
SOURCES = ['overlay_u38_gated_append_001f_cpu', 'overlay_u9e_list_append_0e52_cpu', 'system_getmem_023f_cpu',
           'system_string_leaves_cpu', 'resident_00ba_sidecar_lookup_2199_cpu', 'overlay_u38_10de_cpu']
CALLS = [(0x10FA, 'call', '0x1f'), (0x111B, 'lcall', '0xaf8, 0xb0b'), (0x1126, 'lcall', '0xba, 0x16ec'),
         (0x1142, 'lcall', '0xba, 0x2199'), (0x115D, 'lcall', '0xb0, 0x52')]
FIXUPS = (0x111E, 0x1129, 0x1145, 0x1160)
JUMPS = [(0x10FF, 'je', '0x1162'), (0x1109, 'jbe', '0x1134'), (0x1132, 'jmp', '0x1162'), (0x1149, 'jne', '0x1162')]


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
    check(u['entries'][37] == ENTRY and u['entries'][38] == END, 'public entry 37 (stub 0041:00D9) is +10DE; entry 38 starts at +1168')
    check(0x20 + 5 * 37 == 0xD9, 'the loaded stub of entry 37 is 0041:00D9')
    check(img[u['stub_base'] + 5 * 37:u['stub_base'] + 5 * 37 + 4] == b'\xcd\x3f' + ENTRY.to_bytes(2, 'little'),
          'the file stub of entry 37 is INT 3Fh to +10DE')
    body = code[ENTRY:END]
    check(len(body) == 138 and hashlib.sha256(body).hexdigest() == BODY_SHA, 'body identity (the whole 138-byte ledger bound is code)')
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(len(ins) == 60 and sum(i.size for i in ins) == len(body), 'decode covers the span: 60 instructions')
    check(ins[-1].address == 0x1165 and ins[-1].mnemonic == 'retf' and ins[-1].op_str == '0xa'
          and [i.mnemonic for i in ins].count('retf') == 1, 'single RETF 0Ah at 1165')
    check([(i.address, i.mnemonic, i.op_str) for i in ins if i.mnemonic in ('call', 'lcall')] == CALLS, 'the five calls')
    check(all(fx in u['fixups'] for fx in FIXUPS), 'the four far-call segment words are loader fixups')
    check(not [fx for fx in u['fixups'] if STRING <= fx < END and fx not in FIXUPS], 'no other fixup in the string or the span')
    by = {i.address: i for i in ins}
    check(by[0x10F9].mnemonic == 'push' and by[0x10F9].op_str == 'cs', 'PUSH CS before the near call')
    check([(i.address, i.mnemonic, i.op_str) for i in ins if i.mnemonic.startswith('j')] == JUMPS, 'three forward Jcc and one forward JMP')
    check(not [i for i in ins if i.mnemonic in ('int', 'in', 'out', 'iret', 'ret', 'shl', 'shr', 'mul', 'div', 'rep')],
          'no INT, port access, RET, shift or multiply')
    check([(i.address, i.op_str) for i in ins if i.mnemonic == 'mov' and i.op_str.startswith('a')] ==
          [(0x10EA, 'al, 0x2b'), (0x10F0, 'al, byte ptr es:[di + 3]'), (0x10F5, 'ax, 0x3c'), (0x1120, 'al, 0xa'), (0x1123, 'al, 1'),
           (0x113A, 'al, 0x1f'), (0x1151, 'al, 0x1f'), (0x1157, 'al, 0xff'), (0x115A, 'al, 0')], 'the argument byte and word loads in order')
    check(by[0x1154].mnemonic == 'xor' and by[0x1154].op_str == 'ax, ax', 'XOR AX,AX before the 0 word for +0E52')
    check(by[0x1104].mnemonic == 'cmp' and by[0x1104].op_str == 'byte ptr es:[di + 0x10], 3' and by[0x112E].mnemonic == 'dec'
          and by[0x112E].op_str == 'byte ptr es:[di + 0x10]', 'the B+10h test and decrement')
    check(by[0x113D].mnemonic == 'lea' and by[0x113D].op_str == 'di, [bp - 4]' and by[0x1111].op_str == 'di, [bp - 0x10]',
          'the cursor cell [BP-4] and the string buffer [BP-10h]')
    check(not [i for i in ins if 'ptr [0x' in i.op_str], 'no DS absolute access in the body')
    check(by[0x1116].mnemonic == 'mov' and by[0x1116].op_str == f'di, 0x{STRING:x}' and by[0x1119].op_str == 'cs', 'the CS string pointer 10D2h')
    s = code[STRING:ENTRY]
    check(len(s) == 12 and s[0] == 11 and hashlib.sha256(s).hexdigest() == STRING_SHA and all(0x20 <= b < 0x7F for b in s[1:]),
          'an 11-character printable Pascal string at [10D2,10DE)')
    check(u['entries'][36] == 0x10C1 and code[0x10CF:0x10D2] == b'\xca\x0a\x00', 'the string follows the +10C1 body\'s RETF 0Ah')
    check(hashlib.sha256(code[0x001F:0x0058]).hexdigest() == HELPER_001F_SHA, 'unit 38h +001F is the credited gated append (its pinned body)')
    u9e = next(x for x in units if x['stub_para'] == 0x9E)
    code9e = ovr[u9e['ovr_offset']:u9e['ovr_offset'] + u9e['code_size']]
    check(u9e['memory_segment'] == 0xB0 and u9e['entries'][10] == 0x0E52, 'unit 9E stub 00B0:0052 (entry 10) = +0E52')
    check(img[u9e['stub_base'] + 50:u9e['stub_base'] + 54] == b'\xcd\x3f\x52\x0e', 'the file stub of U9E entry 10')
    check(hashlib.sha256(code9e[0x0E52:0x0F06]).hexdigest() == U9E_0E52_SHA, 'unit 9E +0E52 is the credited list append (its pinned body)')
    return dict(unit_sha256=UNIT_SHA, span=f'unit 38h +[{ENTRY:04X},{END:04X})', bytes=len(body), instructions=len(ins),
                body_sha256=BODY_SHA, string_sha256=STRING_SHA, string_length=s[0],
                calls=[f'{a:04X} {m} {o}' for a, m, o in CALLS], fixups=[f'{fx:04X}' for fx in FIXUPS],
                stubs={'0041:00D9': f'U38 +{ENTRY:04X}', '00B0:0052': 'U9E +0E52'},
                dispatcher_slot='43 (DS:68D4), per docs/re/u9e_dispatch_handlers.json and OVL_009E_0000_HANDLERS.md')


def library(directory):
    path = directory / 'u38_10de.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include')] +
                   [str(ROOT / f'src/recovered/{s}.c') for s in SOURCES] + ['-o', str(path)], check=True)
    f = ctypes.CDLL(str(path)).pr_u38_handler_10de_cpu
    f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_void_p, ctypes.c_void_p]
    f.restype = ctypes.c_int
    return f


def emitter_check():
    r = subprocess.run([sys.executable, str(ROOT / 'tools/overlay/emit_ovl038_10de.py'), '--check'], capture_output=True, text=True)
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
