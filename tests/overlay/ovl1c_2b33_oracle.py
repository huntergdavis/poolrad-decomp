#!/usr/bin/env python3
"""Differential oracle for unit 0x1c +0x2B33 (ECL opcode 0x39, WHO).

The 75 owner bytes at GAME.OVR file offset 0x3879 run under Unicorn (16-bit real mode). Their four far calls go to stubs that log the
pushed argument words, return the case's AX values, and return with `retf N`. The same cases run through the C translation (the vector
driver tests/overlay/ovl1c_2b33_harness.c linked with src/recovered/overlay_u1c_who_2b33.c). Compared per case: the call log and the
status. The status requires SP restored to entry+4 and BP to its entry value after the RETF, and it requires that the body's own memory
writes (its pushes) never land inside the 256-byte frame [BP-0x100, BP).

Mutants: one-edit copies of the translation are built in the work directory and must each differ on at least one case. The owner
GAME.OVR is read at run time and is never committed; the work directory must be outside the repository.

Usage: ovl1c_2b33_oracle.py --game-ovr PATH --work-dir DIR [--cases N] [--seed S]
"""
import argparse
import hashlib
import os
import random
import subprocess
import sys

from unicorn import (Uc, UcError, UC_ARCH_X86, UC_HOOK_CODE, UC_HOOK_MEM_READ_UNMAPPED, UC_HOOK_MEM_WRITE,
                     UC_HOOK_MEM_WRITE_UNMAPPED, UC_MODE_16, UC_PROT_ALL)
from unicorn.x86_const import (UC_X86_REG_AX, UC_X86_REG_BP, UC_X86_REG_CS, UC_X86_REG_DS, UC_X86_REG_ES,
                               UC_X86_REG_IP, UC_X86_REG_SP, UC_X86_REG_SS)

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SRC = os.path.join(ROOT, "src", "recovered", "overlay_u1c_who_2b33.c")
INC = os.path.join(ROOT, "include")
HARNESS = os.path.join(ROOT, "tests", "overlay", "ovl1c_2b33_harness.c")

FILE_OFF = 0x3879
SPAN_LEN = 75
SPAN_SHA = "b373a049e2df103b77382331370aefb2a5378b0f9d66b947333c16298e646083"

BODY_CS = 0x0000
BODY_IP = 0x2B33
RET_CS = 0x0000
RET_IP = 0x0F00                 # the caller's return point, linear 0x0F00: no code there
WHO_FRAME = 0x100

# callee (seg, off) -> (name, number of pushed words, bytes popped by RETF = 2 * words)
STUBS = {
    (0x2B, 0x2A): ("a", 1),
    (0x709, 0x0DD3): ("d", 4),
    (0xAF8, 0x0B25): ("c", 5),
    (0xBA, 0x2C48): ("e", 3),
}
STUB_BY_TARGET = {key: name for key, name in STUBS.items()}

MUTANTS = [
    ("display-ah-from-entry", "ah = (uint8_t)(ret_a >> 8);", "ah = entry_ah; (void)ret_a;"),
    ("final-ah-from-first-return", "ah = (uint8_t)(ret_c >> 8);", "ah = (uint8_t)(ret_a >> 8); (void)ret_c;"),
    ("frame-address-without-bp", "dst_off = (uint16_t)(env->entry_sp - 2u - WHO_FRAME_SIZE);",
     "dst_off = (uint16_t)(env->entry_sp - WHO_FRAME_SIZE);"),
    ("strstore-max-0100", "#define WHO_STR_MAX 0x00FFu", "#define WHO_STR_MAX 0x0100u"),
    ("first-word-ah-zeroed", "(uint16_t)(((uint16_t)entry_ah << 8) | 1u)", "(uint16_t)(((uint16_t)(entry_ah & 0u) << 8) | 1u)"),
]


def rd16(uc, lin):
    return int.from_bytes(uc.mem_read(lin, 2), "little")


def gen_case(rng):
    return {
        "entry_ah": rng.randrange(256),
        "entry_sp": rng.choice([0x0102, rng.randrange(0x0102, 0xFFF1)]),
        "ds_seg": rng.randrange(0x10000),
        "ss_seg": rng.randrange(0x1000, 0x8000),
        "ret_a": rng.randrange(65536),
        "ret_c": rng.randrange(65536),
        "bp0": rng.randrange(65536),
    }


def case_text(case):
    lines = ["case", "entry_ah %d" % case["entry_ah"], "entry_sp %d" % case["entry_sp"], "ds_seg %d" % case["ds_seg"],
             "ss_seg %d" % case["ss_seg"], "ret_a %d" % case["ret_a"], "ret_c %d" % case["ret_c"], "end"]
    return "\n".join(lines) + "\n"


def run_unicorn_case(body, case):
    log = []
    cov = {"writes": 0, "frame_violations": 0}
    uc = Uc(UC_ARCH_X86, UC_MODE_16)
    uc.mem_map(0, 0x100000, UC_PROT_ALL)
    uc.mem_write(BODY_CS * 16 + BODY_IP, body)
    ss_base = case["ss_seg"] * 16
    entry_sp = case["entry_sp"]
    frame_lo = ss_base + ((entry_sp - 2 - WHO_FRAME) & 0xFFFF)
    frame_hi = ss_base + ((entry_sp - 2) & 0xFFFF)     # BP = entry_sp - 2: the frame is [BP-0x100, BP); the saved BP sits at BP
    uc.mem_write(ss_base + entry_sp, RET_IP.to_bytes(2, "little") + RET_CS.to_bytes(2, "little"))
    uc.reg_write(UC_X86_REG_CS, BODY_CS)
    uc.reg_write(UC_X86_REG_IP, BODY_IP)
    uc.reg_write(UC_X86_REG_DS, case["ds_seg"])
    uc.reg_write(UC_X86_REG_ES, 0)
    uc.reg_write(UC_X86_REG_SS, case["ss_seg"])
    uc.reg_write(UC_X86_REG_SP, entry_sp)
    uc.reg_write(UC_X86_REG_BP, case["bp0"])
    uc.reg_write(UC_X86_REG_AX, (case["entry_ah"] << 8) | 0x5A)   # AL is not read on entry

    def on_code(mu, address, size, _user):
        cs = mu.reg_read(UC_X86_REG_CS)
        ip = mu.reg_read(UC_X86_REG_IP)
        key = (cs, ip)
        if key not in STUBS:
            return
        name, nwords = STUBS[key]
        sp = mu.reg_read(UC_X86_REG_SP)
        ss = mu.reg_read(UC_X86_REG_SS) * 16

        def word_at(k):
            return rd16(mu, ss + ((sp + k) & 0xFFFF))

        # the words are pushed in order; the last pushed is at SP+4, the first at SP+4+2*(n-1)
        args = [word_at(4 + 2 * (nwords - 1 - k)) for k in range(nwords)]
        ret_ip, ret_cs = word_at(0), word_at(2)
        if name == "a":
            log.append("call a %d" % args[0])
            mu.reg_write(UC_X86_REG_AX, case["ret_a"])
        elif name == "d":
            log.append("call d %d %d %d %d" % tuple(args))
        elif name == "c":
            log.append("call c %d %d %d %d %d" % tuple(args))
            mu.reg_write(UC_X86_REG_AX, case["ret_c"])
        else:
            log.append("call e %d %d %d" % tuple(args))
        mu.reg_write(UC_X86_REG_IP, ret_ip)
        mu.reg_write(UC_X86_REG_CS, ret_cs)
        mu.reg_write(UC_X86_REG_SP, (sp + 4 + 2 * nwords) & 0xFFFF)

    def on_write(mu, access, address, size, value, _user):
        cov["writes"] += 1
        if frame_lo <= address < frame_hi:
            cov["frame_violations"] += 1

    faults = []

    def on_unmapped(mu, access, address, size, value, _user):
        faults.append("unmapped access %#x at CS:IP %04X:%04X" % (
            address, mu.reg_read(UC_X86_REG_CS), mu.reg_read(UC_X86_REG_IP)))
        return False

    uc.hook_add(UC_HOOK_CODE, on_code)
    uc.hook_add(UC_HOOK_MEM_WRITE, on_write)
    uc.hook_add(UC_HOOK_MEM_READ_UNMAPPED | UC_HOOK_MEM_WRITE_UNMAPPED, on_unmapped)
    try:
        uc.emu_start(BODY_CS * 16 + BODY_IP, RET_CS * 16 + RET_IP, timeout=5000000)
    except UcError as e:
        raise RuntimeError("oracle emulation stopped (%s); %s" % (e, "; ".join(faults) or "no unmapped access"))
    if faults:
        raise RuntimeError("oracle touched unmapped memory: " + "; ".join(faults))
    returned = uc.reg_read(UC_X86_REG_CS) == RET_CS and uc.reg_read(UC_X86_REG_IP) == RET_IP
    balanced = uc.reg_read(UC_X86_REG_SP) == (entry_sp + 4) & 0xFFFF and uc.reg_read(UC_X86_REG_BP) == case["bp0"]
    ok = returned and balanced and cov["frame_violations"] == 0
    out = list(log)
    out.append("status 0" if ok else "status 9")
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
        print("FAIL span at file offset 0x%04X is not the pinned 75 bytes" % FILE_OFF)
        return 1
    os.makedirs(a.work_dir, exist_ok=True)

    rng = random.Random(a.seed)
    cases = [gen_case(rng) for _ in range(a.cases)]
    text = "".join(case_text(c) for c in cases)
    results = [run_unicorn_case(body, c) for c in cases]
    oracle = [r[0] for r in results]
    writes = sum(r[1]["writes"] for r in results)
    frame_viol = sum(r[1]["frame_violations"] for r in results)

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
    print("COVERAGE body memory writes (its pushes) observed: %d; writes inside the frame: %d" % (writes, frame_viol))
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

    ok = failures == 0 and mutant_fail == 0 and frame_viol == 0
    print("RESULT %s" % ("PASS" if ok else "FAIL"))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
