#!/usr/bin/env python3
"""Compare 0709:0C3F to the original bytes using the existing 08AE Unicorn harness.

Requires private START.EXE, Capstone and Unicorn. No native or identity claim.
"""
import ctypes
import tempfile
from pathlib import Path
from hashlib import sha256
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
import res08ae_time_0000_cpu_audit as audit
from resident_loader_map import unpack

image = bytes(unpack((audit.ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes())[0])
audit.SPANS['body'] = (0x0709, 0x0c3f, 0x0cab,
                       'a8e33767ba204b74a7c909cb85a7580ddbbaa36d60dfafb5fb3425d8c16412ed', 48)
audit.SEGWORDS = {0x0c5c: audit.DOSSEG, 0x0c6c: audit.SYS}
audit.SOURCES = ('resident_0709_time_0c3f_cpu', 'dos_gettime_00a2_cpu', 'system_longmul_0a5b_cpu', 'memory_bus', 'ega_hw')
code, decoded = {}, {}
for kind, (seg, start, end, digest, count) in audit.SPANS.items():
    blob = image[audit.physical(seg, start):audit.physical(seg, end)]
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(blob, start))
    audit.check(len(ins) == count and sum(i.size for i in ins) == len(blob), f'decode {kind}')
    audit.check(digest is None or sha256(blob).hexdigest() == digest, f'hash {kind}')
    code[kind] = blob
    decoded.update({(kind, i.address): i for i in ins})
audit.check([(i.address, i.op_str) for (kind, _), i in decoded.items() if kind == 'body' and i.mnemonic == 'lcall'] ==
            [(0x0c59, '0xa32, 0xa2'), (0x0c69, '0xaf8, 0xa5b')], 'far calls')
with tempfile.TemporaryDirectory() as d:
    # Reuse compilation conventions while selecting this entry point.
    import subprocess
    so = Path(d) / 'time.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared',
                    '-I', str(audit.ROOT / 'include'),
                    *[str(audit.ROOT / 'src/recovered' / (s + '.c')) for s in audit.SOURCES], '-o', str(so)], check=True)
    f = ctypes.CDLL(str(so)).pr_r0709_time_0c3f_cpu
    f.argtypes = [ctypes.POINTER(audit.Bus), ctypes.c_size_t, ctypes.POINTER(audit.Cpu), audit.SERVICE_FN,
                  ctypes.c_void_p, ctypes.POINTER(audit.Diagnostic)]
    f.restype = ctypes.c_int
    seen = set()
    for name, options in audit.cases():
        m, cpu, time = audit.build(code, **options)
        cpu.ip = 0x0c3f
        status, covered = audit.compare(name, (m, cpu, time), code, decoded, f)
        audit.check(status == 'RETURN', f'stop {name}')
        seen |= covered
    audit.check(seen == set(decoded), 'uncovered instruction')
    print(f'0709 Unicorn: {len(audit.cases())} cases; {len(seen)}/{len(decoded)} closure starts; CPU14 and 1 MiB RAM match')
