#!/usr/bin/env python3
"""Differential oracle for unit 0x1c +0x2E10 (ECL opcode 0x38, PROGRAM).

The 109 owner bytes at GAME.OVR file offset 0x3B56 run under Unicorn (16-bit real mode) with CS = a fixed segment, so that the
CS-pushed near calls land on stubs at (CS, +0x2DD3) and (CS, +0x0001). The far calls go to stubs at their pinned targets. Each stub logs
its call, applies the case's DS writes for that callee, sets AX where the callee returns a value, and returns with RETF (popping the
pushed words). The same cases run through the C translation (the vector driver tests/overlay/ovl1c_2e10_harness.c linked with
src/recovered/overlay_u1c_program_2e10.c). Compared per case: the call log, the status, and every DS byte that differs from its
initial value. The status requires SP restored to entry+4 and BP to its entry value, and every stub entry to see the stack the
translation implies (SP = entry SP - 6 - 2*argument words - 4).

Mutants: one-edit copies of the translation are built in the work directory and must each differ on at least one case. The owner
GAME.OVR is read at run time and is never committed; the work directory must be outside the repository.

Usage: ovl1c_2e10_oracle.py --game-ovr PATH --work-dir DIR [--cases N] [--seed S]
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
SRC = os.path.join(ROOT, "src", "recovered", "overlay_u1c_program_2e10.c")
INC = os.path.join(ROOT, "include")
HARNESS = os.path.join(ROOT, "tests", "overlay", "ovl1c_2e10_harness.c")

FILE_OFF = 0x3B56
SPAN_LEN = 109
SPAN_SHA = "839f9df6a713088c9889d3d9969424d858f757a7ab51e1d7e05f9a8f0bab100c"

BODY_CS = 0x0100                # the unit's code segment in this model; the near calls push it
BODY_IP = 0x2E10
RET_CS = 0x0000
RET_IP = 0x0F00                 # the caller's return point: linear 0x0F00, no code there
DS_SEG = 0x3000
DS_SIZE = 0x5D96                # PR_U1C_PROGRAM_DS_SIZE: the routine touches DS offsets below 0x5D96
SS_SEG = 0x9000
FRAME = 6                       # push bp (2) and sub sp,4

# (seg, off) -> (name, argument words, is a near call with its own return CS: the return frame is the same 4 bytes)
STUBS = {
    (0x2B, 0x2A): ("a", 1),
    (0x2B, 0x25): ("b", 1),
    (0x78, 0x25): ("c", 0),
    (0x82, 0x25): ("d", 0),
    (0xBA, 0x27D9): ("e", 0),
    (BODY_CS, 0x2DD3): ("f", 0),
    (BODY_CS, 0x0001): ("g", 0),
}
EFFECT_OFFSETS = [0x4430, 0x4436, 0x4437, 0x4438, 0x4439, 0x49ED, 0x49EE, 0x5D92, 0x5D93, 0x5D94, 0x5D95]

MUTANTS = [
    ("flag-path-ah-from-entry", "ah = (uint8_t)(w >> 8);", "ah = entry_ah; (void)w;"),
    ("second-arg-ah-from-entry", "ah = (uint8_t)(ret_a >> 8);", "ah = entry_ah; (void)ret_a;"),
    ("sel8-calls-78", "h->call_82_25(env->ctx);", "h->call_78_25(env->ctx);"),
    ("saved-pc-not-restored", "ds_put_word(ds, 0x49EDu, saved);", "(void)saved;"),
    ("flag-copy-dropped", "ds_put_word(ds, 0x5D92u, w);", "(void)w;"),
    ("near-0001-dropped", "h->call_near_0001(env->ctx);", "(void)0;"),
]


def rd16(uc, lin):
    return int.from_bytes(uc.mem_read(lin, 2), "little")


def gen_case(rng):
    case = {
        "entry_ah": rng.randrange(256),
        "entry_sp": rng.randrange(0x0200, 0xF000),
        "bp0": rng.randrange(65536),
        "ds": {},
        "ret_a": rng.randrange(65536),
        "ret_b": 0,
        "eff": {k: [] for k in "abcdefg"},
    }
    for off in EFFECT_OFFSETS:
        if rng.random() < 0.6:
            case["ds"][off] = rng.randrange(256)
    case["ds"][0x4430] = rng.choice([0, 0, 1, 2, 0xFF])
    sel = rng.choice([0, 8, 9, 9, 1, 5, 0x10])
    case["ret_b"] = (rng.randrange(256) << 8) | sel
    for name in "abcdefg":
        for _ in range(rng.randrange(3)):
            off = rng.choice(EFFECT_OFFSETS) if rng.random() < 0.7 else rng.randrange(DS_SIZE)
            case["eff"][name].append((off, rng.randrange(256)))
    return case


def case_text(case):
    lines = ["case", "entry_ah %d" % case["entry_ah"]]
    lines += ["ds %d %d" % (off, val) for off, val in sorted(case["ds"].items())]
    lines += ["ret_a %d" % case["ret_a"], "ret_b %d" % case["ret_b"]]
    for name in "abcdefg":
        lines += ["eff %s %d %d" % (name, off, val) for off, val in case["eff"][name]]
    lines.append("end")
    return "\n".join(lines) + "\n"


def run_unicorn_case(body, case):
    log = []
    cov = {"flag_path": 0, "sel0": 0, "sel8": 0, "sel9": 0, "arity_mismatch": 0}
    uc = Uc(UC_ARCH_X86, UC_MODE_16)
    uc.mem_map(0, 0x100000, UC_PROT_ALL)
    uc.mem_write(BODY_CS * 16 + BODY_IP, body)
    ds_init = dict(case["ds"])
    for off, val in ds_init.items():
        uc.mem_write(DS_SEG * 16 + off, bytes([val]))
    entry_sp = case["entry_sp"]
    frame_sp = (entry_sp - FRAME) & 0xFFFF
    uc.mem_write(SS_SEG * 16 + entry_sp, RET_IP.to_bytes(2, "little") + RET_CS.to_bytes(2, "little"))
    uc.reg_write(UC_X86_REG_CS, BODY_CS)
    uc.reg_write(UC_X86_REG_IP, BODY_IP)
    uc.reg_write(UC_X86_REG_DS, DS_SEG)
    uc.reg_write(UC_X86_REG_ES, 0)
    uc.reg_write(UC_X86_REG_SS, SS_SEG)
    uc.reg_write(UC_X86_REG_SP, entry_sp)
    uc.reg_write(UC_X86_REG_BP, case["bp0"])
    uc.reg_write(UC_X86_REG_AX, (case["entry_ah"] << 8) | 0x5A)   # AL is not read on entry

    def on_code(mu, address, size, _user):
        cs = mu.reg_read(UC_X86_REG_CS)
        ip = mu.reg_read(UC_X86_REG_IP)
        if cs == BODY_CS:
            if ip == 0x2E1D:
                cov["flag_path"] += 1
            elif ip == 0x2E4A:
                cov["sel0"] += 1
            elif ip == 0x2E5A:
                cov["sel8"] += 1
            elif ip == 0x2E65:
                cov["sel9"] += 1
        key = (cs, ip)
        if key not in STUBS:
            return
        name, nwords = STUBS[key]
        sp = mu.reg_read(UC_X86_REG_SP)
        ss = mu.reg_read(UC_X86_REG_SS) * 16

        def word_at(k):
            return rd16(mu, ss + ((sp + k) & 0xFFFF))

        args = [word_at(4 + 2 * (nwords - 1 - k)) for k in range(nwords)]
        ret_ip, ret_cs = word_at(0), word_at(2)
        # Stub-entry arity: the body has pushed exactly the argument words (and, for the near calls, the pushed CS is the return CS).
        if sp != (frame_sp - 2 * nwords - 4) & 0xFFFF:
            cov["arity_mismatch"] += 1
        if name == "a":
            log.append("call a %d" % args[0])
            mu.reg_write(UC_X86_REG_AX, case["ret_a"])
        elif name == "b":
            log.append("call b %d" % args[0])
            mu.reg_write(UC_X86_REG_AX, case["ret_b"])
        elif name == "c":
            log.append("call 78 25")
        elif name == "d":
            log.append("call 82 25")
        elif name == "e":
            log.append("call ba 27d9")
        elif name == "f":
            log.append("call near 2dd3")
        else:
            log.append("call near 0001")
        for off, val in case["eff"][name]:
            mu.mem_write(DS_SEG * 16 + off, bytes([val]))
        mu.reg_write(UC_X86_REG_IP, ret_ip)
        mu.reg_write(UC_X86_REG_CS, ret_cs)
        mu.reg_write(UC_X86_REG_SP, (sp + 4 + 2 * nwords) & 0xFFFF)

    faults = []

    def on_unmapped(mu, access, address, size, value, _user):
        faults.append("unmapped access %#x at CS:IP %04X:%04X" % (
            address, mu.reg_read(UC_X86_REG_CS), mu.reg_read(UC_X86_REG_IP)))
        return False

    uc.hook_add(UC_HOOK_CODE, on_code)
    uc.hook_add(UC_HOOK_MEM_READ_UNMAPPED | UC_HOOK_MEM_WRITE_UNMAPPED, on_unmapped)
    try:
        uc.emu_start(BODY_CS * 16 + BODY_IP, RET_CS * 16 + RET_IP, timeout=5000000)
    except UcError as e:
        raise RuntimeError("oracle emulation stopped (%s); %s" % (e, "; ".join(faults) or "no unmapped access"))
    if faults:
        raise RuntimeError("oracle touched unmapped memory: " + "; ".join(faults))
    returned = uc.reg_read(UC_X86_REG_CS) == RET_CS and uc.reg_read(UC_X86_REG_IP) == RET_IP
    balanced = uc.reg_read(UC_X86_REG_SP) == (entry_sp + 4) & 0xFFFF and uc.reg_read(UC_X86_REG_BP) == case["bp0"]
    ok = returned and balanced and cov["arity_mismatch"] == 0
    out = list(log)
    out.append("status 0" if ok else "status 9")
    final = bytes(uc.mem_read(DS_SEG * 16, DS_SIZE))
    for off in range(DS_SIZE):
        if final[off] != ds_init.get(off, 0):
            out.append("ds %d %d" % (off, final[off]))
    out.append("endcase")
    return out, cov


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
        print("FAIL span at file offset 0x%04X is not the pinned 109 bytes" % FILE_OFF)
        return 1
    os.makedirs(a.work_dir, exist_ok=True)

    rng = random.Random(a.seed)
    cases = [gen_case(rng) for _ in range(a.cases)]
    text = "".join(case_text(c) for c in cases)
    results = [run_unicorn_case(body, c) for c in cases]
    oracle = [r[0] for r in results]
    cov = {k: sum(r[1][k] for r in results) for k in results[0][1]}
    print("COVERAGE (executed in emulation): flag-path load in %d cases; selector 0 in %d; selector 8 in %d; selector 9 in %d; "
          "stub arity mismatches %d"
          % (sum(1 for r in results if r[1]["flag_path"]), sum(1 for r in results if r[1]["sel0"]),
             sum(1 for r in results if r[1]["sel8"]), sum(1 for r in results if r[1]["sel9"]), cov["arity_mismatch"]))

    base_bin = os.path.join(a.work_dir, "harness_base")
    build(SRC, base_bin)
    base = split_cases(run_harness(base_bin, text))
    failures = 0
    for idx, (want, got) in enumerate(zip(oracle, base)):
        if want != got:
            failures += 1
            if failures <= 3:
                print("FAIL case %d differs" % idx)
                print("  oracle:", want[:12])
                print("  C     :", got[:12])
    if len(base) != len(oracle):
        print("FAIL case count: oracle %d, C %d" % (len(oracle), len(base)))
        failures += 1
    print("RESULT baseline: %d/%d cases equal" % (len(oracle) - failures, len(oracle)))

    mutant_fail = 0
    src_text = open(SRC).read()
    for name, old, new in MUTANTS:
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
        print("MUTANT %s: %s (%d of %d cases differ)" % (name, "KILLED" if killed else "SURVIVED", killed, len(oracle)))
        if not killed:
            mutant_fail += 1

    ok = failures == 0 and mutant_fail == 0 and cov["arity_mismatch"] == 0
    print("RESULT %s" % ("PASS" if ok else "FAIL"))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
