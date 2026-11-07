#!/usr/bin/env python3
"""OVL_0038_09C0 (GAME.OVR unit 38h +09C0, the 009E dispatcher's slot-28 handler): the static source checker.

It proves, from the pinned media read as bytes (nothing is executed; no Unicorn, no DOSBox-X: original and native
execution of this body are a separately authorised review, see docs/re/OVL_0038_09C0_CPU.md):
  * the unit 38h code identity and that public entry 24 is +09C0 (stub 0041:0098) and entry 25 starts at +0A3D;
  * the body [09C0,0A3D): 125 bytes (the whole queue bound), 52 instructions, its SHA-256, one RETF 0Ah, four forward
    jumps to +0A37, no backward jump, no INT, no port access, no shift or multiply;
  * its five calls in order: LCALL 00B0:0048 at 09D4, PUSH CS; CALL +0000 at 09EE/09EF, LCALL 0AF8:0B0B at 0A02,
    LCALL 00BA:16EC at 0A0D, LCALL 00B0:002A at 0A32, and that the four far-call segment words are the only fixups in the span;
  * the two DS reads (681Bh, 6820h), the A+3 decrement and the CS-relative string pointer 09B2h the body pushes for StrLoad:
    a 13-character printable Pascal string at [09B2,09C0) (right after the +0947 body), hashed, not quoted;
  * unit 9E's descriptor: run-time stub segment 00B0, entry 8 (stub 00B0:0048) is +0DE3 and entry 2 (00B0:002A) is +0026;
  * the emitter regenerates the committed C byte for byte (tools/overlay/emit_ovl038_09c0.py --check).
--compile-only builds the model with its composed sources through the audit's own compiler command and binds the symbol,
with no media, and prints CHECKER_BUILD_OK.

python3 tools/overlay/ovl038_09c0_cpu_audit.py [--compile-only] [--output report.json]"""
import argparse, ctypes, hashlib, json, subprocess, sys, tempfile
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, check
from overlay_inventory import find_units, parse_mz

CHECKER_ROWS = ('OVL_0038_09C0',)
ENTRY, END = 0x09C0, 0x0A3D
STRING = 0x09B2
EXE_SHA = 'd825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd'
OVR_SHA = '872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614'
UNIT_SHA = '1dd5e9d59ea97dad6646a28b2be1e68fd5a2906d578f1eabe0911a5b514a6ee7'
BODY_SHA = 'd089ea6db19457c4a3c0f720eb001eb6530247d6ed98f4c6e41b777483422601'
STRING_SHA = '883cf55c59bbe4a5de2d7c744b75ba65cd39407feeccc8eb4793fbb640e5567a'
SOURCES = ['overlay_u38_helper_cluster_cpu', 'system_string_leaves_cpu', 'system_freemem_0254_cpu',
           'overlay_u9e_list_release_0026_cpu', 'overlay_u9e_entry8_fullstate_compose_0de3',
           'overlay_u9e_entry8_fullstate_prefix_0de3', 'overlay_u9e_entry8_fullstate_loop_0e05',
           'overlay_u9e_entry8_fullstate_postcall_0e13', 'overlay_u9e_entry8_fullstate_tail_0e1f',
           'overlay_u9e_entry8_system_call_0e0e', 'system_random_fullstate_11c0', 'system_random_bit_fullstate_11d6',
           'system_seed_fullstate_120f', 'overlay_u38_handler_09c0_cpu']
CALLS = [(0x09D4, 'lcall', '0xb0, 0x48'), (0x09EF, 'call', '0'), (0x0A02, 'lcall', '0xaf8, 0xb0b'),
         (0x0A0D, 'lcall', '0xba, 0x16ec'), (0x0A32, 'lcall', '0xb0, 0x2a')]
FIXUPS = (0x09D7, 0x0A05, 0x0A10, 0x0A35)
JUMPS = [(0x09DB, 'jbe', '0xa37'), (0x09E2, 'jbe', '0xa37'), (0x09E9, 'jne', '0xa37'), (0x0A21, 'jne', '0xa37')]


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
    check(u['entries'][24] == ENTRY and u['entries'][25] == END, 'public entry 24 (stub 0041:0098) is +09C0; entry 25 starts at +0A3D')
    check(img[u['stub_base'] + 5 * 24:u['stub_base'] + 5 * 24 + 4] == b'\xcd\x3f' + ENTRY.to_bytes(2, 'little'),
          'the file stub of entry 24 is INT 3Fh to +09C0')
    body = code[ENTRY:END]
    check(len(body) == 125 and hashlib.sha256(body).hexdigest() == BODY_SHA, 'body identity (the whole 125-byte bound is code)')
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(len(ins) == 52 and sum(i.size for i in ins) == len(body), 'decode covers the span: 52 instructions')
    check(ins[-1].address == 0x0A3A and ins[-1].mnemonic == 'retf' and ins[-1].op_str == '0xa'
          and [i.mnemonic for i in ins].count('retf') == 1, 'single RETF 0Ah at 0A3A')
    check([(i.address, i.mnemonic, i.op_str) for i in ins if i.mnemonic in ('call', 'lcall')] == CALLS, 'the five calls')
    check(all(fx in u['fixups'] for fx in FIXUPS), 'the four far-call segment words are loader fixups')
    check(not [fx for fx in u['fixups'] if STRING <= fx < END and fx not in FIXUPS], 'no other fixup in the string or the span')
    by = {i.address: i for i in ins}
    check(by[0x09EE].mnemonic == 'push' and by[0x09EE].op_str == 'cs', 'PUSH CS before the near call')
    check([(i.address, i.mnemonic, i.op_str) for i in ins if i.mnemonic.startswith('j')] == JUMPS, 'four forward jumps to the epilogue')
    check(not [i for i in ins if i.mnemonic in ('int', 'in', 'out', 'iret', 'ret', 'jmp', 'shl', 'shr', 'mul', 'div')],
          'no INT, port access, RET, JMP, shift or multiply')
    check([(i.address, i.op_str) for i in ins if i.mnemonic == 'cmp' and 'ptr [0x68' in i.op_str]
          == [(0x09DD, 'byte ptr [0x681b], 0'), (0x09E4, 'byte ptr [0x6820], 0')], 'the two DS byte tests')
    check(by[0x0A15].mnemonic == 'dec' and by[0x0A15].op_str == 'byte ptr es:[di + 3]', 'the A+3 decrement')
    check(by[0x09FD].mnemonic == 'mov' and by[0x09FD].op_str == f'di, 0x{STRING:x}', 'the CS string pointer 09B2h')
    s = code[STRING:ENTRY]
    check(len(s) == 14 and s[0] == 13 and hashlib.sha256(s).hexdigest() == STRING_SHA and all(0x20 <= b < 0x7F for b in s[1:]),
          'a 13-character printable Pascal string at [09B2,09C0)')
    u9e = next(x for x in units if x['stub_para'] == 0x9E)
    check(u9e['memory_segment'] == 0xB0 and u9e['entries'][8] == 0x0DE3 and u9e['entries'][2] == 0x0026,
          'unit 9E stubs 00B0:0048 (entry 8) = +0DE3 and 00B0:002A (entry 2) = +0026')
    check(img[u9e['stub_base'] + 40:u9e['stub_base'] + 44] == b'\xcd\x3f\xe3\x0d'
          and img[u9e['stub_base'] + 10:u9e['stub_base'] + 14] == b'\xcd\x3f\x26\x00', 'the file stubs of U9E entries 8 and 2')
    return dict(unit_sha256=UNIT_SHA, span=f'unit 38h +[{ENTRY:04X},{END:04X})', bytes=len(body), instructions=len(ins),
                body_sha256=BODY_SHA, string_sha256=STRING_SHA, string_length=s[0],
                calls=[f'{a:04X} {m} {o}' for a, m, o in CALLS], fixups=[f'{fx:04X}' for fx in FIXUPS],
                stubs={'0041:0098': f'U38 +{ENTRY:04X}', '00B0:0048': 'U9E +0DE3', '00B0:002A': 'U9E +0026'})


def library(directory):
    path = directory / 'u38_09c0.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include')] +
                   [str(ROOT / f'src/recovered/{s}.c') for s in SOURCES] + ['-o', str(path)], check=True)
    f = ctypes.CDLL(str(path)).pr_u38_handler_09c0_cpu
    f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_void_p, ctypes.c_void_p]
    f.restype = ctypes.c_int
    return f


def emitter_check():
    r = subprocess.run([sys.executable, str(ROOT / 'tools/overlay/emit_ovl038_09c0.py'), '--check'], capture_output=True, text=True)
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
