#!/usr/bin/env python3
"""Differential oracle for unit 0x1c +0x02E0 (ECL opcode 0x0A, LOAD CHARACTER).

The 212 owner bytes at GAME.OVR file offset 0x1026 run under Unicorn (16-bit real mode). Their four far calls go to stubs that log
the arguments, apply the case's DS writes, and return. The same cases run through the C translation (the vector driver
tests/overlay/ovl1c_02e0_harness.c linked with src/recovered/overlay_u1c_load_character_02e0.c). Both sides start from the same
sparse DS image and far words and see the same callee return values and writes. Compared per case: the call log, the status, and
every DS byte that differs from its initial value.

Mutants: one-edit copies of the translation are built in the work directory and must each differ from the original bytes on at
least one case. The owner GAME.OVR is read at run time and is never committed; the work directory must be outside the repository.

Usage: ovl1c_02e0_oracle.py --game-ovr PATH --work-dir DIR [--cases N] [--seed S]
"""
import argparse
import hashlib
import os
import random
import subprocess
import sys

from unicorn import (Uc, UcError, UC_ARCH_X86, UC_HOOK_CODE, UC_HOOK_MEM_READ_UNMAPPED, UC_HOOK_MEM_WRITE_UNMAPPED,
                     UC_MODE_16, UC_PROT_ALL)
from unicorn.x86_const import (UC_X86_REG_AX, UC_X86_REG_BP, UC_X86_REG_CS, UC_X86_REG_DS, UC_X86_REG_ES,
                               UC_X86_REG_IP, UC_X86_REG_SP, UC_X86_REG_SS)

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SRC = os.path.join(ROOT, "src", "recovered", "overlay_u1c_load_character_02e0.c")
INC = os.path.join(ROOT, "include")
HARNESS = os.path.join(ROOT, "tests", "overlay", "ovl1c_02e0_harness.c")

FILE_OFF = 0x1026
SPAN_LEN = 212
SPAN_SHA = "07903667cd47edfcd46ca97d08b10a6bb494b07008fd02290b22b642d7e42333"

DS_SEG = 0x2000
SS_SEG = 0x9000
SP0 = 0xFF00
BP0 = 0x7000
BODY_CS = 0x0000
BODY_IP = 0x02E0
RET_CS = 0x0000          # the caller's return point is linear 0x0F00, which no code occupies
RET_IP = 0x0F00
DS_SIZE = 0x84D4
NODE_SEGS = (0x3000, 0x4000, 0x5000, 0x6000)   # far node segments; no far word lies in DS or the stack
JUNK_SEG = 0x7000                              # never holds a far word: every read there is zero in both models
INTERESTING_DS = [0x4430, 0x4436, 0x4437, 0x4438, 0x4439, 0x5D92, 0x5D93, 0x5D94, 0x5D95, 0x5D96, 0x5D97,
                  0x5D98, 0x5D99, 0x84D2, 0x84D3]

# callee name, far target, bytes of arguments the callee pops (retf N)
STUBS = {
    "a": (0x2B, 0x2A, 2),
    "b": (0x2B, 0x25, 2),
    "c": (0x78, 0x2A, 4),
    "d": (0xBA, 0x767, 4),
}
STUB_BY_TARGET = {(seg, off): name for name, (seg, off, _n) in STUBS.items()}

MUTANTS = [
    ("first-call-ah-zero", "ah = (uint8_t)(ret_a >> 8);", "ah = 0u; (void)ret_a;"),
    ("walk-off-by-one", "i < count && (p_off | p_seg)", "i <= count && (p_off | p_seg)"),
    ("flag-no-segment-compare", "ds_word(ds, 0x4438u) == ds_word(ds, 0x5D94u) && w == ds_word(ds, 0x5D92u)",
     "w == ds_word(ds, 0x5D92u)"),
    ("call-d-seg-zero", "h->call_ba_767(ctx, ds_word(ds, 0x5D92u), ds_word(ds, 0x5D94u));",
     "h->call_ba_767(ctx, ds_word(ds, 0x5D92u), 0u);"),
    ("flag-path-ah-low-byte", "ah = (uint8_t)(w >> 8);", "ah = (uint8_t)(w & 0xFFu);"),
    ("skip-84d3-clear", "ds[0x84D3u] = 0u;", ""),
]


def rd16(uc, lin):
    return int.from_bytes(uc.mem_read(lin, 2), "little")


def gen_case(rng):
    case = {"entry_ah": rng.randrange(256), "ds": {}, "far": [], "ret_a": rng.randrange(65536),
            "ret_b": 0, "eff": {"a": [], "b": [], "c": [], "d": []}}
    for off in INTERESTING_DS:
        if rng.random() < 0.6:
            case["ds"][off] = rng.randrange(256)
    if rng.random() < 0.7:
        case["ds"][0x84D2] = rng.choice([0, 1, 0x80, 0xFF])
        case["ds"][0x84D3] = rng.choice([0, 1, 0x80, 0xFF])
    count_raw = rng.choice([0x00, 0x01, 0x02, 0x7F, 0x80, 0x81, rng.randrange(256), rng.randrange(256)])
    case["ret_b"] = (rng.randrange(256) << 8) | count_raw

    nnodes = rng.randrange(0, 7)
    nodes, used = [], set()
    while len(nodes) < nnodes:
        key = (rng.choice(NODE_SEGS), rng.randrange(0, 0xF001, 0x200))
        if key not in used:
            used.add(key)
            nodes.append(key)
    linear = bool(nodes) and rng.random() < 0.5
    for i, (seg, base) in enumerate(nodes):
        if linear:
            nxt = nodes[i + 1] if i + 1 < len(nodes) else None
        else:
            nxt = rng.choice(nodes + [None, None])
        n_off, n_seg = (0, 0) if nxt is None else (nxt[1], nxt[0])
        case["far"].append((seg, base + 0x104, n_off))
        case["far"].append((seg, base + 0x106, n_seg))
    if linear:
        # Linear chain, head = node 0, count = j: the walk ends on node j, which is stored, and the flag compare can match it.
        head = nodes[0]
        j = rng.randrange(len(nodes))
        hi = 0x80 if rng.random() < 0.8 else 0x00
        case["ret_b"] = (rng.randrange(256) << 8) | hi | j
        case["ds"][0x84D2], case["ds"][0x84D3] = (rng.choice([1, 0x80, 0xFF]) if rng.random() < 0.8 else 0,
                                                  rng.choice([1, 0x80, 0xFF]) if rng.random() < 0.8 else 0)
        tgt = nodes[j] if rng.random() < 0.8 else rng.choice(nodes)
        case["ds"][0x4436], case["ds"][0x4437] = tgt[1] & 0xFF, tgt[1] >> 8
        case["ds"][0x4438], case["ds"][0x4439] = tgt[0] & 0xFF, tgt[0] >> 8
    elif nodes and rng.random() < 0.8:
        head = rng.choice(nodes)
    elif rng.random() < 0.5:
        head = (JUNK_SEG, rng.randrange(0, 0xF001, 2))
    else:
        head = (0, 0)
    case["ds"][0x5D96], case["ds"][0x5D97] = head[1] & 0xFF, head[1] >> 8
    case["ds"][0x5D98], case["ds"][0x5D99] = head[0] & 0xFF, head[0] >> 8

    for name in "abcd":
        for _ in range(rng.randrange(3)):
            off = rng.choice(INTERESTING_DS) if rng.random() < 0.7 else rng.randrange(DS_SIZE)
            val = rng.randrange(256)
            if 0x5D96 <= off <= 0x5D99:
                # Head writes keep the far memory in the safe window: a null head is never changed (a nonzero offset with
                # segment 0 would read code bytes), the offset's high byte stays below 0xF0 (no 16-bit wrap on +0x106), and
                # the segment's high byte is one of the node segments or the junk segment.
                if head == (0, 0):
                    continue
                if off == 0x5D97:
                    val = rng.randrange(0xF0)
                elif off == 0x5D99:
                    val = rng.choice([0x30, 0x40, 0x50, 0x60, 0x70])
            case["eff"][name].append((off, val))
    return case


def case_text(case):
    lines = ["case", "entry_ah %d" % case["entry_ah"]]
    lines += ["ds %d %d" % (off, val) for off, val in sorted(case["ds"].items())]
    lines += ["far %d %d %d" % w for w in case["far"]]
    lines += ["ret_a %d" % case["ret_a"], "ret_b %d" % case["ret_b"]]
    for name in "abcd":
        lines += ["eff %s %d %d" % (name, off, val) for off, val in case["eff"][name]]
    lines.append("end")
    return "\n".join(lines) + "\n"


def run_unicorn_case(body, case):
    log = []
    uc = Uc(UC_ARCH_X86, UC_MODE_16)
    uc.mem_map(0, 0x100000, UC_PROT_ALL)
    uc.mem_write(BODY_CS * 16 + BODY_IP, body)
    ds_init = dict(case["ds"])
    for off, val in ds_init.items():
        uc.mem_write(DS_SEG * 16 + off, bytes([val]))
    for seg, off, word in case["far"]:
        uc.mem_write(seg * 16 + off, word.to_bytes(2, "little"))
    uc.mem_write(SS_SEG * 16 + SP0, RET_IP.to_bytes(2, "little") + RET_CS.to_bytes(2, "little"))
    uc.reg_write(UC_X86_REG_CS, BODY_CS)
    uc.reg_write(UC_X86_REG_IP, BODY_IP)
    uc.reg_write(UC_X86_REG_DS, DS_SEG)
    uc.reg_write(UC_X86_REG_ES, 0)
    uc.reg_write(UC_X86_REG_SS, SS_SEG)
    uc.reg_write(UC_X86_REG_SP, SP0)
    uc.reg_write(UC_X86_REG_BP, BP0)
    uc.reg_write(UC_X86_REG_AX, (case["entry_ah"] << 8) | 0x5A)   # AL is not read on entry

    def on_code(mu, address, size, _user):
        cs = mu.reg_read(UC_X86_REG_CS)
        ip = mu.reg_read(UC_X86_REG_IP)
        name = STUB_BY_TARGET.get((cs, ip))
        if name is None:
            return
        sp = mu.reg_read(UC_X86_REG_SP)

        def word_at(k):
            return rd16(mu, SS_SEG * 16 + ((sp + k) & 0xFFFF))

        ret_ip, ret_cs = word_at(0), word_at(2)
        if name == "a":
            log.append("call a %d" % word_at(4))
            mu.reg_write(UC_X86_REG_AX, case["ret_a"])
        elif name == "b":
            log.append("call b %d" % word_at(4))
            mu.reg_write(UC_X86_REG_AX, case["ret_b"])
        elif name == "c":
            log.append("call c %d %d" % (word_at(6), word_at(4)))
        else:
            log.append("call d %d %d" % (word_at(4), word_at(6)))
        for off, val in case["eff"][name]:
            mu.mem_write(DS_SEG * 16 + off, bytes([val]))
        mu.reg_write(UC_X86_REG_IP, ret_ip)
        mu.reg_write(UC_X86_REG_CS, ret_cs)
        mu.reg_write(UC_X86_REG_SP, (sp + 4 + STUBS[name][2]) & 0xFFFF)

    faults = []

    def on_unmapped(mu, access, address, size, value, _user):
        faults.append("unmapped access %#x size %d at CS:IP %04X:%04X" % (
            address, size, mu.reg_read(UC_X86_REG_CS), mu.reg_read(UC_X86_REG_IP)))
        return False

    uc.hook_add(UC_HOOK_CODE, on_code)
    uc.hook_add(UC_HOOK_MEM_READ_UNMAPPED | UC_HOOK_MEM_WRITE_UNMAPPED, on_unmapped)
    try:
        uc.emu_start(BODY_CS * 16 + BODY_IP, RET_CS * 16 + RET_IP, timeout=5000000)
    except UcError as e:
        raise RuntimeError("oracle emulation stopped (%s); %s" % (e, "; ".join(faults) or "no unmapped access recorded"))
    if faults:
        raise RuntimeError("oracle touched unmapped memory: " + "; ".join(faults))
    returned = uc.reg_read(UC_X86_REG_CS) == RET_CS and uc.reg_read(UC_X86_REG_IP) == RET_IP
    balanced = uc.reg_read(UC_X86_REG_SP) == SP0 + 4
    out = list(log)
    out.append("status 0" if returned and balanced else "status 9")
    final = bytes(uc.mem_read(DS_SEG * 16, DS_SIZE))
    for off in range(DS_SIZE):
        init = ds_init.get(off, 0)
        if final[off] != init:
            out.append("ds %d %d" % (off, final[off]))
    out.append("endcase")
    return out


def run_harness(binary, text):
    res = subprocess.run([binary], input=text, capture_output=True, text=True, timeout=600)
    if res.returncode != 0:
        raise RuntimeError("harness failed: %s" % res.stderr.strip())
    return res.stdout.splitlines()


def build(src, out):
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-I", INC, src, HARNESS, "-o", out], check=True)


def split_cases(lines):
    cases, cur = [], []
    for line in lines:
        cur.append(line)
        if line == "endcase":
            cases.append(cur)
            cur = []
    if cur:
        cases.append(cur + ["<unterminated>"])
    return cases


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--game-ovr", required=True, help="path to the owner GAME.OVR (read only, never committed)")
    ap.add_argument("--work-dir", required=True, help="scratch directory outside the repository")
    ap.add_argument("--cases", type=int, default=400)
    ap.add_argument("--seed", type=int, default=1)
    a = ap.parse_args()

    ovr = open(a.game_ovr, "rb").read()
    body = ovr[FILE_OFF:FILE_OFF + SPAN_LEN]
    if len(body) != SPAN_LEN or hashlib.sha256(body).hexdigest() != SPAN_SHA:
        print("FAIL span at file offset 0x%04X is not the pinned 212 bytes" % FILE_OFF)
        return 1
    os.makedirs(a.work_dir, exist_ok=True)

    rng = random.Random(a.seed)
    cases = [gen_case(rng) for _ in range(a.cases)]
    text = "".join(case_text(c) for c in cases)
    oracle = [run_unicorn_case(body, c) for c in cases]

    base_bin = os.path.join(a.work_dir, "harness_base")
    build(SRC, base_bin)
    base = split_cases(run_harness(base_bin, text))
    failures = 0
    for idx, (want, got) in enumerate(zip(oracle, base)):
        if want != got:
            failures += 1
            if failures <= 3:
                print("FAIL case %d differs" % idx)
                print("  oracle:", want)
                print("  C     :", got)
    if len(base) != len(oracle):
        print("FAIL case count: oracle %d, C %d" % (len(oracle), len(base)))
        failures += 1
    flag_cleared = sum(1 for o in oracle if "ds %d 0" % 0x4430 in o)
    out_stored = sum(1 for o in oracle if any(x.startswith("ds %d " % 0x5D92) for x in o))
    print("COVERAGE flag 0x4430 changed to 0 in %d cases; output pair 0x5D92 changed in %d cases"
          % (flag_cleared, out_stored))
    print("RESULT baseline: %d/%d cases equal" % (len(oracle) - failures, len(oracle)))

    mutant_fail = 0
    for name, old, new in MUTANTS:
        src_text = open(SRC).read()
        if src_text.count(old) != 1:
            print("FAIL mutant %s: anchor not unique" % name)
            mutant_fail += 1
            continue
        mpath = os.path.join(a.work_dir, "mutant_%s.c" % name)
        with open(mpath, "w") as f:
            f.write(src_text.replace(old, new))
        mbin = os.path.join(a.work_dir, "harness_%s" % name)
        build(mpath, mbin)
        got = split_cases(run_harness(mbin, text))
        killed = sum(1 for want, g in zip(oracle, got) if want != g)
        verdict = "KILLED" if killed else "SURVIVED"
        print("MUTANT %s: %s (%d of %d cases differ)" % (name, verdict, killed, len(oracle)))
        if not killed:
            mutant_fail += 1

    ok = failures == 0 and mutant_fail == 0
    print("RESULT %s" % ("PASS" if ok else "FAIL"))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
