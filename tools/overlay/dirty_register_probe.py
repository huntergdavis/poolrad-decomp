"""Count the entry registers a body audit's differential actually runs with (the dirty-register sweep, VP i20417).

Usage: dirty_register_probe.py AUDIT_MODULE [AUDIT ARGS...]   (for example: u1c_goto_00f4_cpu_audit --unicorn)

It runs the audit's own main() with its Cpu constructor wrapped, then prints how many CPUs were built, how many had a
nonzero BP low byte (the saved BP of a word store over [bp+0] would be invisible when it is zero), nonzero SI, DI and ES,
odd SP, and how many distinct BP low bytes were used. It changes nothing in the audit and judges nothing: a case that
passes with these values is a pass with these values.
"""
# NOT_A_ROW_CHECKER: wraps another audit's constructor to count entry-register values; it checks no registered row
import sys, importlib, time, json, collections
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
name = sys.argv[1]
extra = sys.argv[2:]
log = []
mod = importlib.import_module(name)
C = None
for attr in ('Cpu',):
    C = getattr(mod, attr, None)
if C is None:
    print('NOCPU'); sys.exit(2)
orig = C.__init__
def init(self, *a, **k):
    orig(self, *a, **k)
    log.append((self.bp, self.si, self.di, self.es, self.ax, self.bx, self.cx, self.dx, self.sp))
C.__init__ = init
sys.argv = [name + '.py'] + extra
t = time.time()
try:
    mod.main()
    rc = 0
except SystemExit as e:
    rc = e.code
n = len(log)
bp_dirty = sum(1 for r in log if (r[0] & 255) != 0)
si_dirty = sum(1 for r in log if r[1] != 0)
di_dirty = sum(1 for r in log if r[2] != 0)
es_dirty = sum(1 for r in log if r[3] != 0)
sp_odd = sum(1 for r in log if r[8] & 1)
bps = collections.Counter(r[0] & 255 for r in log)
print(json.dumps(dict(audit=name, rc=rc, cpus=n, bp_low_nonzero=bp_dirty, si_nonzero=si_dirty, di_nonzero=di_dirty, es_nonzero=es_dirty,
    sp_odd=sp_odd, distinct_bp_low_bytes=len(bps), seconds=round(time.time() - t, 1))))
