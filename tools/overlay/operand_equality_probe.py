#!/usr/bin/env python3
"""Measure whether an audit's differential ever ran an ordering conditional with EQUAL operands (the 275B JBE lesson).

Usage: operand_equality_probe.py OUT.json AUDIT_MODULE [AUDIT ARGS...]

A model that tests CF where the original's JBE tests CF|ZF (or JB for JBE, JL for JLE ...) is wrong only when the two compared
values are equal, and branch accounting cannot see that: both sides of the branch are taken through the unequal cases. This runs
the audit's own main() with every Unicorn emulation hooked and records, for each ordering conditional (JB JAE JBE JA JL JGE JLE JG),
how often it executed with ZF set (equal / zero) and with ZF clear. The ROOT segment of an emulation is the CS of its first
instruction; only conditionals in a root segment are judged (callee bodies belong to their own audits). It judges nothing else.
"""
# NOT_A_ROW_CHECKER: wraps another audit's emulator to count the flag state at its ordering conditionals; it checks no registered row
import sys, importlib, json, time, collections
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
import unicorn
from unicorn import UC_HOOK_CODE
from unicorn import x86_const as R

ORDERING = {0x72: 'jb', 0x73: 'jae', 0x76: 'jbe', 0x77: 'ja', 0x7C: 'jl', 0x7D: 'jge', 0x7E: 'jle', 0x7F: 'jg'}
sites = {}          # (root_cs, cs, ip) -> [mnemonic, zf1, zf0]
emulations = []


def instrument(uc):
    state = dict(root=None)

    def hook(em, address, size, user):
        cs = em.reg_read(R.UC_X86_REG_CS)
        if state['root'] is None:
            state['root'] = cs
            emulations.append(cs)
        if size != 2:
            return
        op = em.mem_read(address, 1)[0]
        if op not in ORDERING:
            return
        ip = em.reg_read(R.UC_X86_REG_IP)
        row = sites.setdefault((state['root'], cs, ip), [ORDERING[op], 0, 0])
        row[1 if em.reg_read(R.UC_X86_REG_EFLAGS) & 0x40 else 2] += 1
    uc.hook_add(UC_HOOK_CODE, hook)


original_init = unicorn.Uc.__init__


def patched(self, *a, **k):
    original_init(self, *a, **k)
    instrument(self)


unicorn.Uc.__init__ = patched
out, name, extra = sys.argv[1], sys.argv[2], sys.argv[3:]
mod = importlib.import_module(name)
sys.argv = [name + '.py'] + extra
started = time.time()
try:
    rc = mod.main()
except SystemExit as exc:
    rc = exc.code
judged = [(root, cs, ip, row) for (root, cs, ip), row in sites.items() if cs == root]
rows = [dict(site=f'{cs:04X}:{ip:04X}', root=f'{root:04X}', cc=row[0], equal=row[1], unequal=row[2]) for root, cs, ip, row in sorted(judged, key=lambda t: (t[0], t[2]))]
report = dict(audit=name, args=extra, rc=rc if isinstance(rc, int) or rc is None else str(rc)[:80], emulations=len(emulations), seconds=round(time.time() - started, 1),
              ordering_sites=len(rows), never_equal=[r for r in rows if r['equal'] == 0], rows=rows)
Path(out).write_text(json.dumps(report, indent=1))
print(json.dumps({k: v for k, v in report.items() if k not in ('rows', 'never_equal')}), 'never_equal', len(report['never_equal']))
