#!/usr/bin/env python3
"""Store replay of the run-037 `10D2:0165 -> 10D2:13C6` LOGL window; print no game or RAM payload.

Hash-locks the private run-037 inputs (window LOGL trace, native RAM images and CPU transcriptions at both
stops) and the run-030 images used for the cross-run control, then replays every memory store of the window
over the native 0165 image and compares the result with the native 13C6 image byte by byte.  Stores are
decoded from the logged instruction bytes (capstone, 16-bit) and addressed with the logged registers.

Unlike the 030 and 034 windows this one needs no declared host writer: the audit FAILS CLOSED if the trace
contains an IRQ entry, a callback, an IN/OUT, a ROM-segment line, a store at or above 0xA0000, or a BIOS tick
change.  Controls: trace read annotations vs the replayed shadow memory, omission by routine / opcode / region,
the log-vs-transcription CPU check at both stops, and the run-030 cross-run control (same delta address set;
the single differing value is a pushed SI = host hour, and the tick word is host time of day in both runs).
Needs `capstone` (not stdlib); run without `python3 -I`.  `POOLRAD_GAP_MUTANT=<name>` perturbs one branch
(used by check_slot_a_gap_store_replay_037.sh); the baseline passes and every mutant must fail or be listed
as equivalent there.
"""

from hashlib import sha256
from pathlib import Path
import collections
import os
import re
import sys

MUT = os.environ.get("POOLRAD_GAP_MUTANT", "")
RUN037 = {
    "LOGL-W2-0165-to-13C6.TXT": "9a18628141a799e7bbc519d7e66aeed46ee73f06eda2bdfe887f5eb03199000d",
    "RAM-10D2-0165.BIN": "0914b16f3278486195a111769095f97c55315330a12f278f2c3dc86a2d84b643",
    "RAM-10D2-13C6.BIN": "7f255423f12188d9d961c9bb8f041bb69f51b175466f188d3bb84427332c841e",
    "CPU-10D2-0165.TXT": "60cc866a7629f1a6e94dac7352e151bbf7547759796a7b0cd1c3d21cdf9fca85",
    "CPU-10D2-13C6.TXT": "653663076aa7e53dca5aa9e92a728b34d8aeac4e17c45f8aac2c60a65c8f3521",
}
RUN030 = {
    "RAM-10D2-0165.BIN": "b227e2c1fadb3b1b8d3dd00a1020d83edddcdc193e16acbe26c93f181c5890c8",
    "RAM-10D2-13C6.BIN": "342882515c7dd4f852a17a15bc78c4d7bf147b2cb287e3e7169abbcaee02e270",
    "CPU-10D2-0165.TXT": "be674fba164d5b5bde0407d34d6cfd3f6f4515a05247c80bd227a2c5d3f73218",
}
RAM_SIZE = 655360
VIDEO_BASE = 0xA0000
LINES = 766
EXPECT_STORES = {"mov": 224, "push": 19, "dec": 8, "call": 3}
DELTA_BYTES = 54
SS_BASE = 0x1D160                  # SS = 1D16
XR_DIFF = (0x20F06,)               # the one delta address whose final value differs from run 030 (SS:3DA6)
TICKS_PER_HOUR = 1573040 / 24      # BIOS ticks per hour (0x1800B0 per day)
ROUTINES = {"10D2:17AE": "17AE", "10D2:1742": "1742", "10D2:13B1": "13B1"}


def check(condition, message):
    if not condition:
        raise SystemExit(message)


LINE = re.compile(
    r"^(\d+\.\d+)\s+([0-9A-F]{4}):([0-9A-F]{8})\s+(.*?)\s{2,}((?:[0-9A-F]{2} )*[0-9A-F]{2})\s+"
    r"EAX:([0-9A-F]{8}) EBX:([0-9A-F]{8}) ECX:([0-9A-F]{8}) EDX:([0-9A-F]{8}) ESI:([0-9A-F]{8}) "
    r"EDI:([0-9A-F]{8}) EBP:([0-9A-F]{8}) ESP:([0-9A-F]{8}) DS:([0-9A-F]{4}) ES:([0-9A-F]{4}) "
    r"FS:([0-9A-F]{4}) GS:([0-9A-F]{4}) SS:([0-9A-F]{4}) CF:(\d) ZF:(\d) SF:(\d) OF:(\d) AF:(\d) "
    r"PF:(\d) IF:(\d) TF:(\d) VM:\d FLG:([0-9A-F]{8})")
ANNOT = re.compile(r"\b(ds|es|ss|cs|fs|gs):\[([0-9A-F]{4})\]=([0-9A-F]+)\b")


def parse(path):
    """Rows: (ts, cs, ip, text, bytes, regs, flags).  Flags are rebuilt from the per-flag columns."""
    rows = []
    with open(path, "r") as handle:
        for line in handle:
            m = LINE.match(line)
            check(m, "unparsed trace line in " + Path(path).name)
            g = m.groups()
            regs = dict(zip(("ax", "bx", "cx", "dx", "si", "di", "bp", "sp"),
                            [int(x, 16) & 0xFFFF for x in g[5:13]]))
            regs.update(ds=int(g[13], 16), es=int(g[14], 16), fs=int(g[15], 16), gs=int(g[16], 16),
                        ss=int(g[17], 16), cs=int(g[1], 16))
            cf, zf, sf, of, af, pf, iff, tf = (int(x) for x in g[18:26])
            flags = ((int(g[26], 16) & 0xF000) | 0x0002 | cf | (pf << 2) | (af << 4) | (zf << 6) |
                     (sf << 7) | (tf << 8) | (iff << 9) | (of << 11))
            rows.append((float(g[0]), int(g[1], 16), int(g[2], 16) & 0xFFFF, g[3],
                         bytes(int(x, 16) for x in g[4].split()), regs, flags))
    return rows


def mnemonic(text):
    parts = text.split()
    return parts[0] if parts else ""


def cpu_fields(path):
    out = {}
    for part in Path(path).read_text().split():
        name, _, value = part.partition("=")
        check(len(value) == 4 and name not in out, "malformed CPU field in " + Path(path).name)
        out[name.lower()] = int(value, 16)
    check(len(out) == 14, "CPU file is not 14 fields")
    return out


def routines(rows):
    """Routine of every row by the call stack (entry row of each near/far call target)."""
    func, stack, entries = [None] * len(rows), [("0165-body", None)], {}
    for i in range(1, len(rows) - 1):
        r = rows[i]
        func[i] = stack[-1][0]
        m = mnemonic(r[3])
        if m in ("call", "lcall", "int"):
            nx = rows[i + 1]
            name = "%04X:%04X" % (nx[1], nx[2])
            stack.append((ROUTINES.get(name, name), r[5]["sp"]))
            entries.setdefault(ROUTINES.get(name, name), i)
        elif m in ("ret", "retf", "iret") and len(stack) > 1:
            stack.pop()
    return func, entries


def replay(rows, base):
    import capstone
    from capstone import x86 as X
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_16)
    md.detail = True
    mem, stores, notes = {}, [], {"annot_match": 0, "annot_bad": [], "video": 0}

    def rd(a):
        a &= 0xFFFFF
        check(a < VIDEO_BASE, "a replayed read depends on unobserved display memory")
        return mem.get(a, base[a])

    def rdw(a):
        return rd(a) | (rd(a + 1) << 8)

    def wr(i, a, n, v, kind):
        a &= 0xFFFFF
        for k in range(n):
            b = (a + k) & 0xFFFFF
            check(b < VIDEO_BASE, "store at/above 0xA0000: the display rule applies and this window needs none")
            mem[b] = (v >> (8 * k)) & 255
        stores.append((i, a, n, kind))

    reg_ids = {X.X86_REG_AX: ("ax", 0xFFFF, 0), X.X86_REG_BX: ("bx", 0xFFFF, 0), X.X86_REG_CX: ("cx", 0xFFFF, 0),
               X.X86_REG_DX: ("dx", 0xFFFF, 0), X.X86_REG_SI: ("si", 0xFFFF, 0), X.X86_REG_DI: ("di", 0xFFFF, 0),
               X.X86_REG_BP: ("bp", 0xFFFF, 0), X.X86_REG_SP: ("sp", 0xFFFF, 0),
               X.X86_REG_AL: ("ax", 0xFF, 0), X.X86_REG_AH: ("ax", 0xFF, 8), X.X86_REG_BL: ("bx", 0xFF, 0),
               X.X86_REG_BH: ("bx", 0xFF, 8), X.X86_REG_CL: ("cx", 0xFF, 0), X.X86_REG_CH: ("cx", 0xFF, 8),
               X.X86_REG_DL: ("dx", 0xFF, 0), X.X86_REG_DH: ("dx", 0xFF, 8), X.X86_REG_DS: ("ds", 0xFFFF, 0),
               X.X86_REG_ES: ("es", 0xFFFF, 0), X.X86_REG_SS: ("ss", 0xFFFF, 0), X.X86_REG_CS: ("cs", 0xFFFF, 0)}

    def reg(r, rid):
        name, mask, shift = reg_ids[rid]
        return (r[name] >> shift) & mask

    def ea(r, op):
        m = op.mem
        off = m.disp + (reg(r, m.base) if m.base else 0) + (reg(r, m.index) if m.index else 0)
        if MUT == "ea-ds":
            seg = r["ds"]            # mutant: ignore the SS default of BP/SP-based operands
        elif m.segment:
            seg = reg(r, m.segment)
        elif m.base == X.X86_REG_BP:
            seg = r["ss"]
        else:
            seg = r["ds"]
        return ((seg << 4) + (off & 0xFFFF)) & 0xFFFFF

    def val(r, op, size):
        if op.type == X.X86_OP_REG:
            return reg(r, op.reg)
        if op.type == X.X86_OP_IMM:
            return op.imm & ((1 << (8 * size)) - 1)
        a = ea(r, op)
        return rd(a) if size == 1 else rdw(a)

    alu = {"add": lambda o, s, c: o + s, "sub": lambda o, s, c: o - s, "adc": lambda o, s, c: o + s + c,
           "sbb": lambda o, s, c: o - s - c, "and": lambda o, s, c: o & s, "or": lambda o, s, c: o | s,
           "xor": lambda o, s, c: o ^ s}
    last = len(rows) - 1
    for i, (ts, cs, ip, text, byt, r, flags) in enumerate(rows):
        if i == 0:
            continue                 # the first line at a breakpoint is logged twice (same instruction)
        if i == last and MUT != "apply-stop-row":
            continue                 # the final line is the 13C6 stop state: not executed
        check(cs == 0x10D2 and not text.startswith("callback") and mnemonic(text) not in ("in", "out"),
              "undeclared host effect in the window at row %d: %s" % (i, text[:30]))
        sspb, sp = r["ss"] << 4, r["sp"]
        m = ANNOT.search(text)
        if m:
            seg, off, hexv = r[m.group(1)], int(m.group(2), 16), m.group(3)
            a = ((seg << 4) + off) & 0xFFFFF
            got = bytes(mem.get((a + k) & 0xFFFFF, base[(a + k) & 0xFFFFF]) for k in range(len(hexv) // 2))
            if got == int(hexv, 16).to_bytes(len(hexv) // 2, "little"):
                notes["annot_match"] += 1
            else:
                notes["annot_bad"].append(i)
        ins = next(md.disasm(byt, ip), None)
        check(ins is not None, "undecoded instruction at row %d" % i)
        check(ins.addr_size == 2, "32-bit address size at row %d" % i)
        mn, ops = ins.mnemonic, ins.operands
        if mn == "push":
            if MUT != "push-skip":
                wr(i, sspb + ((sp - 2) & 0xFFFF), 2, val(r, ops[0], 2) & 0xFFFF, "push")
        elif mn == "call":
            ret = (ip + ins.size + (1 if MUT == "call-retaddr" else 0)) & 0xFFFF
            wr(i, sspb + ((sp - 2) & 0xFFFF), 2, ret, "call")
        elif mn == "lcall":
            wr(i, sspb + ((sp - 2) & 0xFFFF), 2, cs, "lcall")
            wr(i, sspb + ((sp - 4) & 0xFFFF), 2, (ip + ins.size) & 0xFFFF, "lcall")
        elif mn == "int":
            wr(i, sspb + ((sp - 2) & 0xFFFF), 2, flags, "int")
            wr(i, sspb + ((sp - 4) & 0xFFFF), 2, cs, "int")
            wr(i, sspb + ((sp - 6) & 0xFFFF), 2, (ip + ins.size) & 0xFFFF, "int")
        elif mn.split()[-1] in ("stosb", "stosw", "movsb", "movsw", "xchg", "pushf", "pusha", "popf"):
            raise SystemExit("store form %s appears in this window: not covered" % mn)
        elif ops and ops[0].type == X.X86_OP_MEM and mn not in (
                "cmp", "test", "lea", "les", "lds", "mul", "imul", "div", "idiv", "jmp", "ljmp", "call", "lcall", "push") \
                and not mn.startswith("j"):
            a, size = ea(r, ops[0]), ops[0].size
            check(size in (1, 2), "unsupported operand size at row %d" % i)
            old = rd(a) if size == 1 else rdw(a)
            carry = flags & 1
            if mn == "mov":
                if MUT == "mov-skip":
                    continue
                new = val(r, ops[1], size)
            elif mn in alu:
                new = alu[mn](old, val(r, ops[1], size), carry)
            elif mn == "inc":
                new = old + 1
            elif mn == "dec":
                new = old - (2 if MUT == "dec-two" else 1)
            else:
                raise SystemExit("unsupported memory-destination form %s at row %d" % (mn, i))
            wr(i, a, size, new & ((1 << (8 * size)) - 1), mn)
    return mem, stores, notes


def main():
    check(len(sys.argv) == 3, "usage: audit_slot_a_gap_store_replay_037.py RUN037_DIR RUN030_DIR")
    run37, run30 = Path(sys.argv[1]), Path(sys.argv[2])
    for d, locks in ((run37, RUN037), (run30, RUN030)):
        for name, digest in locks.items():
            check(sha256((d / name).read_bytes()).hexdigest() == digest, "%s/%s identity mismatch" % (d.name, name))
    try:
        import capstone  # noqa: F401
    except ImportError:
        raise SystemExit("capstone is required for the store decode (pip install capstone)")
    rows = parse(run37 / "LOGL-W2-0165-to-13C6.TXT")
    check(len(rows) == LINES, "window line count mismatch")
    check((rows[0][1], rows[0][2]) == (0x10D2, 0x0165) and rows[0][1:3] == rows[1][1:3] and
          (rows[-1][1], rows[-1][2]) == (0x10D2, 0x13C6), "window start/stop rows mismatch")
    base = (run37 / "RAM-10D2-0165.BIN").read_bytes()
    native = (run37 / "RAM-10D2-13C6.BIN").read_bytes()
    check(len(base) == RAM_SIZE and len(native) == RAM_SIZE, "RAM size mismatch")
    # log vs manual CPU transcriptions at both stops
    for row, cpu_file in ((rows[1], "CPU-10D2-0165.TXT"), (rows[-1], "CPU-10D2-13C6.TXT")):
        want = cpu_fields(run37 / cpu_file)
        got = dict(row[5])
        got.update(cs=row[1], ip=row[2], flags=row[6])
        bad = [k for k in want if want[k] != got[k]]
        check(not bad, "%s disagrees with the trace in %s" % (cpu_file, bad))
    tick0, tick1 = (int.from_bytes(x[0x46C:0x470], "little") for x in (base, native))
    check(tick0 == tick1, "BIOS tick changed inside the window")
    mem, stores, notes = replay(rows, base)
    check(collections.Counter(k for _, _, _, k in stores) == EXPECT_STORES, "store counts changed: %s" %
          dict(collections.Counter(k for _, _, _, k in stores)))
    pred = bytearray(base)
    for a, v in mem.items():
        pred[a] = v
    delta = [i for i in range(RAM_SIZE) if base[i] != native[i]]
    mism = [i for i in range(RAM_SIZE) if pred[i] != native[i]]
    check(len(delta) == DELTA_BYTES, "native 0165->13C6 delta size mismatch: %d" % len(delta))
    writers, lastw = collections.defaultdict(list), {}
    for i, a, n, kind in stores:
        for k in range(n):
            writers[(a + k) & 0xFFFFF].append((i, kind))
            lastw[(a + k) & 0xFFFFF] = (i, kind)
    func, entries = routines(rows)
    explained = [a for a in delta if writers[a] and pred[a] == native[a]]
    bad_value = [a for a in delta if writers[a] and pred[a] != native[a]]
    no_writer = [a for a in delta if not writers[a]]
    spurious = [a for a in mism if base[a] == native[a]]
    benign = [a for a in writers if base[a] == native[a] and pred[a] == native[a]]
    print("window rows %d (executed %d), decoded stores %d, native delta %d bytes, tick %d -> %d" %
          (len(rows), len(rows) - 2, len(stores), len(delta), tick0, tick1))
    print("per-byte account: explained %d, writer-but-wrong-value %d, native-changed-without-writer %d, "
          "spurious %d, written-but-unchanged %d" %
          (len(explained), len(bad_value), len(no_writer), len(spurious), len(benign)))
    print("unattributed (delta bytes with no replayed writer): %d; image mismatches: %d" % (len(no_writer), len(mism)))

    def region(a):
        if 0x10F00 <= a < 0x11100:
            return "resident data 10F00-110FF"
        if 0x11100 <= a < 0x11200:
            return "resident records 11100-111FF"
        if SS_BASE <= a < SS_BASE + 0x10000:
            return "stack SS:%04X-%04X" % (min(x for x in delta if SS_BASE <= x < SS_BASE + 0x10000) - SS_BASE,
                                           max(x for x in delta if SS_BASE <= x < SS_BASE + 0x10000) - SS_BASE)
        return "other"
    by_routine = collections.Counter(func[lastw[a][0]] for a in delta if a in lastw)
    by_kind = collections.Counter(lastw[a][1] for a in delta if a in lastw)
    by_region = collections.Counter(region(a) for a in delta)
    by_both = collections.Counter((func[lastw[a][0]], region(a).split(" ")[0]) for a in delta if a in lastw)
    print("explained delta bytes by last-writing routine:", dict(sorted(by_routine.items())))
    print("explained delta bytes by writing opcode:", dict(sorted(by_kind.items())))
    print("explained delta bytes by region:", dict(sorted(by_region.items())))
    print("routine x region:", {"%s/%s" % k: v for k, v in sorted(by_both.items())})
    print("routine entry rows:", dict(sorted(entries.items(), key=lambda x: x[1])))
    print("trace read annotations vs replayed shadow memory: match %d, mismatches %d" %
          (notes["annot_match"], len(notes["annot_bad"])))

    def omit(select):
        alt = bytearray(base)
        for a, v in mem.items():
            if not select(lastw[a]):
                alt[a] = v
        return sum(1 for i in range(RAM_SIZE) if alt[i] != native[i])
    print("omission controls by routine (bytes left unexplained):",
          {r: omit(lambda w, r=r: func[w[0]] == r) for r in sorted(by_routine)})
    print("omission controls by opcode (bytes left unexplained):",
          {k: omit(lambda w, k=k: w[1] == k) for k in sorted(by_kind)})
    # cross-run control against run 030's own 0165 and 13C6 images
    b30, n30 = (run30 / "RAM-10D2-0165.BIN").read_bytes(), (run30 / "RAM-10D2-13C6.BIN").read_bytes()
    d30 = [i for i in range(RAM_SIZE) if b30[i] != n30[i]]
    diff = sorted(a for a in delta if n30[a] != native[a])
    expected_diff = () if MUT == "xr-expect-none" else XR_DIFF
    print("cross-run vs run 030: delta %d vs %d bytes, same address set %s, same final value at %d of %d, differing %s" %
          (len(delta), len(d30), set(delta) == set(d30), len(delta) - len(diff), len(delta),
           ["SS:%04X" % (a - SS_BASE) for a in diff]))
    cpu30 = cpu_fields(run30 / "CPU-10D2-0165.TXT")
    reg_name = "dx" if MUT == "xr-wrong-reg" else "si"
    explained_diff = []
    for a in diff:
        # the last replayed writer of the byte must be a push of a register that was not modified since the
        # 0165 entry; its run-030 value is that register's run-030 entry value
        i, kind = lastw[a]
        check(kind == "push" and mnemonic(rows[i][3]) == "push", "differing byte %06X is not a push" % a)
        pushed = rows[i][3].split()[1]
        check(pushed == reg_name, "differing byte %06X is a push of %s, not %s" % (a, pushed, reg_name))
        check(rows[i][5][reg_name] == rows[1][5][reg_name], "%s changed before the push at row %d" % (reg_name, i))
        low = a == [s for s in stores if s[0] == i][0][1]     # low byte of the pushed word
        want030 = (cpu30[reg_name] >> (0 if low else 8)) & 255
        check(n30[a] == want030, "run-030 byte at %06X is not its %s" % (a, reg_name))
        explained_diff.append((a, reg_name, rows[i][5][reg_name], cpu30[reg_name]))
    print("  each differing byte is a push of the host-time register: %s (run 037 %s=%04X, run 030 %s=%04X)" %
          (["SS:%04X" % (a - SS_BASE) for a, _, _, _ in explained_diff], reg_name.upper(),
           explained_diff[0][2] if explained_diff else 0, reg_name.upper(), explained_diff[0][3] if explained_diff else 0))
    hour37, hour30 = int(tick0 // TICKS_PER_HOUR), int((int.from_bytes(b30[0x46C:0x470], "little")) // TICKS_PER_HOUR)
    hour_off = 1 if MUT == "tick-hour" else 0
    print("  tick words are host time of day: run 037 tick %d = hour %d (SI=%d), run 030 tick %d = hour %d (SI=%d)" %
          (tick0, hour37, rows[1][5]["si"], int.from_bytes(b30[0x46C:0x470], "little"), hour30, cpu30["si"]))
    check(hour37 + hour_off == rows[1][5]["si"] and hour30 == cpu30["si"], "SI is not the host hour in both runs")
    base_diff = sum(1 for i in range(RAM_SIZE) if base[i] != b30[i])
    print("  the two 0165 images differ in %d bytes (time, tick and stack noise)" % base_diff)
    check(set(delta) == set(d30), "run-030 delta address set differs")
    check(tuple(diff) == expected_diff, "cross-run differing set changed: %s" % diff)
    check(not notes["annot_bad"], "trace read annotations disagree with replayed memory at %d row(s)" % len(notes["annot_bad"]))
    check(not no_writer and not bad_value and not mism,
          "store replay leaves %d unattributed byte(s), %d wrong value(s), %d image mismatch(es)" %
          (len(no_writer), len(bad_value), len(mism)))
    print("run-037 window store replay verified")


if __name__ == "__main__":
    main()
