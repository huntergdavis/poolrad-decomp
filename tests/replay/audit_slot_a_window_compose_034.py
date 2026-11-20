#!/usr/bin/env python3
"""Compose the run-034 `2E60:0747 -> 3582:3492` window from C models and diff it with native 3492.

Print no game or RAM payload.  Control flow and every model's entry state come from the hash-locked
W1 LOGL trace (declared); each modeled span is run through the existing raw-CPU C models over the
composed RAM (built with ctypes from the same sources the per-function audits use) and verified
against the trace: exit registers, the OUT sequence, the in-order display (A000) write sequence and the
RAM below 0xA0000.  Spans without a model, host callbacks (DOS seek/read from GAME.OVR, BIOS tick,
hardware IRQ0 frame) and the IRQ0 landing inside a modeled span are applied from the store-replay
journal of audit_slot_a_window_store_replay_034.py (log-derived, declared).  The final composed RAM is
diffed with native 3492 and with the store-replay prediction; every byte is attributed to the source
that last wrote it.  Needs capstone and unicorn (the imported audit modules); run without `python3 -I`.
Environment `POOLRAD_COMPOSE_MUTANT=<name>` perturbs one new branch (see the check script).
"""

from pathlib import Path
import collections
import ctypes
import os
import sys
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.parent
sys.path.insert(0, str(ROOT / "tools" / "overlay"))
sys.path.insert(0, str(HERE))
import audit_slot_a_window_store_replay_034 as A   # noqa: E402
import res0709_printer_0626_cpu_audit as printer    # noqa: E402
import res03f1_clear_0137_cpu_audit as clear         # noqa: E402
import system_str_res1037_audit as s1037             # noqa: E402
import res00ba_word_string_1073_audit as w1073       # noqa: E402
import res0464_bitmap_release_2595_audit as rel2595  # noqa: E402
import u7d_list_timer_0000_cpu_audit as u7d          # noqa: E402

MUT = os.environ.get("POOLRAD_COMPOSE_MUTANT", "")
RAM_1M = 1 << 20
VIDEO_BASE = 0xA0000
EOI_PORT = 0x20
REGS = ("ax", "bx", "cx", "dx", "si", "di", "bp", "sp", "ds", "es", "ss")
# Modeled spans: (name, callee cs, callee ip, nth call, expected span rows) -- rows pinned from the trace.
MODELED = (
    ("u7d-list-timer-0000", 0x2F6B, 0x0000, 0, 876),
    ("res-1073-a", 0x08DE, 0x1073, 0, 166),
    ("res-1073-b", 0x08DE, 0x1073, 1, 186),
    ("res-1037-a", 0x08DE, 0x1037, 0, 183),
    ("res-1037-b", 0x08DE, 0x1037, 1, 171),
    ("clear-0137", 0x0C15, 0x0137, 0, 871),
    ("printer-0626", 0x0F2D, 0x0626, 0, 18671),
    ("release-2595-a", 0x0C88, 0x2595, 0, 11),
    ("release-2595-b", 0x0C88, 0x2595, 1, 11),
)
EXCLUSIONS_TOTAL = 20
# Run B final-image exclusions by (last writer, why): 1 tick + 2 DOS-callback + 10 game-code bytes inside
# the run-to-run noise set, and 7 ROM-stub bytes Run B contradicts (its IRQ0 landed elsewhere).
EXCLUSION_GROUPS = {("bios-callback", "in-noise"): 1, ("dos-callback", "in-noise"): 2, ("game", "in-noise"): 10,
                    ("rom-stub", "contradicted"): 7}
check = A.check


def span_of(rows, cs, ip, nth=0):
    """Entry row (first callee row) and exit row (row after the matching return) of the nth call to cs:ip."""
    hits = [i for i in range(1, len(rows) - 1) if (rows[i][1], rows[i][2]) == (cs, ip) and
            rows[i - 1][3].split()[0] in ("call", "lcall")]
    check(len(hits) > nth, "no call to %04X:%04X" % (cs, ip))
    e = hits[nth]
    sp0 = rows[e][5]["sp"]
    x = next(i for i in range(e + 1, len(rows)) if rows[i][3].split()[0] in ("ret", "retf") and rows[i][5]["sp"] == sp0) + 1
    return e, x


def cpu_of(cpu_type, row):
    c = cpu_type()
    for n, _ in cpu_type._fields_:
        if n in row[5]:
            setattr(c, n, row[5][n])
    c.cs, c.ip, c.flags = row[1], row[2], row[6]
    return c


def logged_state(row):
    d = dict(row[5])
    d.update(cs=row[1], ip=row[2], flags=row[6])
    return d


def image(base, mem):
    m = bytearray(base) + bytearray(RAM_1M - len(base))
    for a, v in mem.items():
        m[a] = v
    return m


def build_libs(tmp):
    for mod in (clear, s1037, rel2595, u7d):
        assert [n for n, _ in mod.Cpu._fields_] == [n for n, _ in printer.Cpu._fields_], "Cpu layout differs"
    return {"printer": printer.library(tmp), "clear": clear.library(tmp), "s1037": s1037.library(tmp)["res"],
            "w1073": w1073.library(tmp), "rel2595": rel2595.library(tmp), "u7d": u7d.library(tmp)}


def run_model(name, libs, m, c):
    """Run one modeled span; returns (status, RAM bytes, OUT events or None, display writes or None)."""
    if name.startswith("printer"):
        st, ram, ev, wr, _, _ = printer.run_c(libs["printer"], m, c)
        return st, ram, ev, wr
    if name.startswith("clear"):
        st, ram, ev, wr, _, _ = clear.run_c(libs["clear"], m, c)
        return st, ram, ev, wr
    ram = (ctypes.c_uint8 * RAM_1M).from_buffer_copy(m)
    if name.startswith("u7d"):
        st = libs["u7d"](ram, RAM_1M, ctypes.byref(c), 100000)
    elif name.startswith("res-1037"):
        callee, cst = ctypes.c_int(99), ctypes.c_int(99)
        st = libs["s1037"](ram, RAM_1M, ctypes.byref(c), ctypes.byref(callee), ctypes.byref(cst))
    elif name.startswith("res-1073"):
        callee, cst = ctypes.c_int(99), ctypes.c_int(99)
        bus = w1073.Bus(ram, None, None, None, None)
        st = libs["w1073"](ctypes.byref(bus), RAM_1M, ctypes.byref(c), ctypes.byref(callee), ctypes.byref(cst))
    else:
        systems = {int.from_bytes(m[((rows_cs(name)) << 4) + site + 3:((rows_cs(name)) << 4) + site + 5], "little")
                   for site in rel2595.SITES}
        d = rel2595.Diagnostic()
        st = libs["rel2595"](ram, RAM_1M, ctypes.byref(c), next(iter(systems)), 64, ctypes.byref(d))
    return st, bytes(ram), None, None


def rows_cs(name):
    return 0x0C88


def load_runb(runs_dir, name, fname):
    from hashlib import sha256
    mpath = runs_dir / name / "MANIFEST-CAPTURE.sha256"
    check(sha256(mpath.read_bytes()).hexdigest() == A.RUNB_MANIFESTS[name], name + " manifest identity mismatch")
    table = {}
    for line in mpath.read_text().splitlines():
        parts = line.split()
        if len(parts) == 2:
            table[parts[1].lstrip("*").split("/")[-1]] = parts[0]
    data = (runs_dir / name / fname).read_bytes()
    check(table.get(fname) == sha256(data).hexdigest() and len(data) == A.RAM_SIZE, name + "/" + fname + " is not the manifest image")
    return data


def runb(rows, base, pred, writer, delta, snaps, runs_dir):
    """Declared exclusions: composed delta bytes that Run B's final 3492 image does not support, and the
    stop-by-stop control with the stack bound tightened to the window's measured SP range."""
    sps = [r[5]["sp"] for r in rows]
    ss_base = A.STACK_SEG << 4
    lo, hi = ss_base + min(sps), ss_base + max(sps) + 2
    if MUT == "bound-narrow":
        lo += 1
    check(len({r[5]["ss"] for r in rows}) == 1 and rows[0][5]["ss"] == A.STACK_SEG, "stack segment is not constant")
    print("tightened stack bound SS:%04X-%04X (%d bytes of the SP range; measured over %d trace lines)" %
          (min(sps), max(sps), max(sps) - min(sps), len(rows)))
    A.runb_control(rows, base, snaps, runs_dir, (lo, hi))
    b3492 = load_runb(runs_dir, "t4492-slot7-images-035", "RAM-3582-3492.BIN")
    b0747 = load_runb(runs_dir, "t4492-slot7-images-035", "RAM-2E60-0747.BIN")
    noise = {a for a in range(A.RAM_SIZE) if b0747[a] != base[a]}
    unsupported = [a for a in delta if pred[a] != b3492[a] or a in noise]
    contradicted = [a for a in delta if pred[a] != b3492[a]]
    def klass(a):       # writer class: C-model and journal game stores are both game code
        lab = writer.get(a, "none")
        return "game" if lab.startswith("c-model") or lab == "journal:game" else lab.split(":")[-1]
    groups = collections.Counter((klass(a), "in-noise" if a in noise else "contradicted") for a in unsupported)
    print("composed delta bytes whose Run B final value differs: %d; delta bytes inside the run-to-run noise set: %d" %
          (len(contradicted), len([a for a in delta if a in noise])))
    print("composed delta bytes independently supported by Run B final 3492: %d of %d; declared exclusions %d: %s" %
          (len(delta) - len(unsupported), len(delta), len(unsupported), dict(sorted(groups.items()))))
    if MUT != "exclusions-unpinned":
        check(dict(groups) == EXCLUSION_GROUPS, "declared exclusion groups changed: %s" % dict(groups))
    in_stack = [a for a in unsupported if lo <= a < hi]
    print("  exclusions inside the tightened stack bound: %d of %d" % (len(in_stack), len(unsupported)))
    check(len(unsupported) == EXCLUSIONS_TOTAL, "declared exclusion count changed: %d" % len(unsupported))


def main():
    check(len(sys.argv) in (3, 4), "usage: audit_slot_a_window_compose_034.py RUN034_DIR GAME.OVR [OVERRIDE_RUNS_DIR]")
    run, game_path = Path(sys.argv[1]), Path(sys.argv[2])
    runs_dir = Path(sys.argv[3]) if len(sys.argv) == 4 else None
    from hashlib import sha256
    for name, digest in A.RUN_FILES.items():
        check(sha256((run / name).read_bytes()).hexdigest() == digest, name + " identity mismatch")
    game = game_path.read_bytes()
    check(sha256(game).hexdigest() == A.GAME_HASH, "GAME.OVR identity mismatch")
    rows = A.parse(run / "LOGL-WIN-0747-to-3492.TXT")
    check(len(rows) == A.LINES, "window line count mismatch")
    base = (run / "RAM-2E60-0747.BIN").read_bytes()
    native = (run / "RAM-3582-3492.BIN").read_bytes()
    spans = {}
    for name, cs, ip, nth, expect_rows in MODELED:
        e, x = span_of(rows, cs, ip, nth)
        check(x - e == expect_rows, "%s span length mismatch: %d" % (name, x - e))
        spans[name] = (e, x)
    snap_rows = {r for e, x in spans.values() for r in (e, x)}
    if runs_dir:
        snap_rows |= {st[1] for st in A.RUNB_STOPS}
    mem, stores, video, notes = A.replay(rows, base, game, snap_rows)
    journal, vjournal = notes["journal"], notes["vjournal"]
    by_row = collections.defaultdict(list)
    for row, addr, byte, cls in journal:
        by_row[row].append((addr, byte, cls))
    irq_rows = [i for i, r in enumerate(rows) if (r[1], r[2]) == (0xF000, 0xFEA5)]
    check(len(irq_rows) == 1, "IRQ0 entry count mismatch")
    irq_in = irq_rows[0]
    irq_out = next(i for i in range(irq_in + 1, len(rows)) if rows[i][3].split()[0] == "iret" and
                   rows[i][5]["sp"] == rows[irq_in][5]["sp"])
    libs = build_libs(Path(tempfile.mkdtemp()))
    out = {}
    print("Modeled spans (entry state from the trace; models run over the composed RAM):")
    comp = {}                 # composed state: byte address -> value, over the native 0747 base
    writer = {}               # composed byte address -> source label
    order = sorted((e, x, name) for name, (e, x) in spans.items())
    cursor = 1
    last_row = len(rows) - 1
    for e, x, name in order + [(last_row, last_row, None)]:
        # rows outside modeled spans: journaled stores (declared fallback)
        for i in range(cursor, e):
            if MUT == "drop-journal" and 1 <= i < 300:
                continue
            for addr, byte, cls in by_row.get(i, ()):
                comp[addr] = byte
                writer[addr] = "journal:" + cls
        if name is None:
            break
        m = image(base, {} if MUT == "stale-ram" else comp)
        c = cpu_of(printer.Cpu, rows[e])
        if MUT == "entry-ax":
            c.ax = (c.ax + 1) & 0xFFFF
        if MUT == "entry-sp":
            c.sp = (c.sp + 2) & 0xFFFF
        status, ram, events, writes = run_model(name, libs, m, c)
        rx = logged_state(rows[x])
        exit_bad = {n: (getattr(c, n), rx[n]) for n, _ in printer.Cpu._fields_ if getattr(c, n) != rx[n]}
        logouts = [(r[5]["dx"], r[5]["ax"] & 255, 1, r[1], r[2]) for i, r in enumerate(rows[e:x], e)
                   if r[3].split()[0] == "out" and not (irq_in <= i <= irq_out)]
        lv = [(o, b) for (i, o, b, cl) in vjournal if e <= i < x]
        if events is None:           # a model without ports or display: the trace must have none in the span
            check(not logouts and not lv, "span %s has OUTs or display stores but its model has none" % name)
            events, writes = [], []
        modelouts = [ev for ev in events]
        # RAM below 0xA0000: model vs the log shadow at the exit row, excluding bytes whose last in-span
        # log writer lies in the IRQ0 interval
        shadow = image(base, mem if x == last_row else {})
        in_span_last = {}
        for row, addr, byte, cls in journal:
            if e <= row < x:
                in_span_last[addr] = row
        irq_only = {a for a, rr in in_span_last.items() if irq_in <= rr <= irq_out}
        # state the model must reach = composed-before + all in-span journal stores (log shadow)
        want = bytearray(m)
        for row, addr, byte, cls in journal:
            if e <= row < x:
                want[addr] = byte
        diffs = [a for a in range(VIDEO_BASE) if ram[a] != want[a]]
        unexplained = [a for a in diffs if a not in irq_only]
        print("  %-13s rows %5d-%5d status %d  exit-regs %s  OUTs %d/%d %s  display writes %d/%d %s  RAM diffs %d (IRQ0-interval writers %d, other %d)" %
              (name, e, x, status, "equal" if not exit_bad else "DIFFER " + str(exit_bad),
               len(modelouts), len(logouts), "equal" if modelouts == logouts else "DIFFER",
               len(writes), len(lv), "equal" if writes == lv else "DIFFER",
               len(diffs), len(diffs) - len(unexplained), len(unexplained)))
        out[name] = dict(status=status, exit_bad=exit_bad, outs=modelouts == logouts, vram=writes == lv,
                         unexplained=unexplained, diffs=diffs, irq_only=irq_only)
        # apply the model's RAM effects to the composed state
        for a in range(VIDEO_BASE):
            if MUT == "skip-model-apply":
                break
            if ram[a] != m[a]:
                comp[a] = ram[a]
                writer[a] = "c-model:" + name
        # IRQ0 interval merge: an in-span IRQ store survives where no later in-span log store overwrote it
        if e <= irq_in < x and MUT != "no-irq-merge":
            for row, addr, byte, cls in journal:
                if irq_in <= row <= irq_out and in_span_last.get(addr) == row:
                    comp[addr] = byte
                    writer[addr] = "journal:" + cls
        cursor = x
    pred = bytearray(base)
    for a, v in comp.items():
        pred[a] = v
    shadow_pred = bytearray(base)
    for a, v in mem.items():
        if a < A.RAM_SIZE:
            shadow_pred[a] = v
    delta = [a for a in range(A.RAM_SIZE) if base[a] != native[a]]
    vs_native = [a for a in range(A.RAM_SIZE) if pred[a] != native[a]]
    vs_shadow = [a for a in range(A.RAM_SIZE) if pred[a] != shadow_pred[a]]
    explained = [a for a in delta if pred[a] == native[a]]
    extra = [a for a in vs_native if base[a] == native[a]]
    unattributed = [a for a in delta if pred[a] != native[a]]
    src = collections.Counter(writer.get(a, "none").split(":")[0] if a in writer else "none" for a in delta)
    print("composed RAM vs native 3492 (%d delta bytes): explained %d, unattributed (composed != native, native changed) %d, "
          "extra (composed changes an unchanged byte) %d; composed vs store-replay prediction: %d differing bytes" %
          (len(delta), len(explained), len(unattributed), len(extra), len(vs_shadow)))
    by = collections.Counter(writer.get(a, "none") for a in delta)
    print("delta bytes by last-writing source:", dict(sorted(by.items())))
    if runs_dir:
        runb(rows, base, pred, writer, delta, notes["snaps"], runs_dir)
    for name, o in out.items():
        check(o["status"] == 0 and not o["exit_bad"] and o["outs"] and o["vram"] and not o["unexplained"],
              "span %s does not reproduce the trace" % name)
    check(not vs_native, "composed RAM differs from native 3492 in %d byte(s)" % len(vs_native))
    print("window composition verified (modeled spans + declared journal fallback)")


if __name__ == "__main__":
    main()
