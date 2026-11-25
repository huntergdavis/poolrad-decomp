#!/usr/bin/env python3
"""Differential oracle for unit 0x3B4 +0x016D (view refresh).

The unit's 0x2A5 code bytes are loaded at CS 0x03CA, so the three near case maps (+00C7, +00FB, +012F) run their real owner bytes under
Unicorn (16-bit real mode). The 307 routine bytes run from +0x016D. Its far calls to other units (0x3D5:0x34, 0x3D5:0x25, 0x3D5:0x39,
0x3CD:0x25) and its near call to +0x0000 go to stubs at those addresses; each stub logs its pushed words, applies the case's DS writes,
returns the case's AX (for the three that return one), and pops its pushed words with RETF. The same cases run through the C translation
(tests/overlay/ovl3b4_016d_harness.c linked with src/recovered/overlay_u3b4_view_refresh_016d.c). Compared per case: the call log (with the
pushed words, and the near case-map argument words read from the real callee's stack), the status, and every DS byte that differs from its
initial value. The status requires SP restored to entry+4 and BP to its entry value, and every stub entry to see the stack the routine
implies (SP = entry SP - 6 - 2 x argument words - 4).

The far record pointer at DS:0x49D2 points into a far window (segments 0x3000..0x70FF) that is written through the byte-level far table;
offsets are kept below 0xF000 so no 16-bit wrap is generated (the wrap is not oracle-checked).

Mutants: one-edit copies of the translation must each differ on at least one case. The owner GAME.OVR is read at run time and never
committed; the work directory must be outside the repository.

Usage: ovl3b4_016d_oracle.py --game-ovr PATH --work-dir DIR [--cases N] [--seed S]
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
SRC = os.path.join(ROOT, "src", "recovered", "overlay_u3b4_view_refresh_016d.c")
INC = os.path.join(ROOT, "include")
HARNESS = os.path.join(ROOT, "tests", "overlay", "ovl3b4_016d_harness.c")

UNIT_FILE = 0x2DB87             # unit 0x3B4 code in GAME.OVR (file offset)
UNIT_LEN = 0x2A5
BODY_OFF = 0x016D
BODY_LEN = 307
BODY_SHA = "c6bb0b220344d76f8924b0f96bcea506bb55dbdad753e2f945fa25bf782255e8"

CS_SEG = 0x03CA                 # the unit's memory segment
RET_CS = 0x0000
RET_IP = 0x0F00
DS_SEG = 0x2000
DS_SIZE = 0x10000
SS_SEG = 0x9000
FRAME = 6                       # push bp (2) and sub sp,4 (2) plus... see note below
NEAR_BYTES = 4                  # the return CS and IP of a near call with the pushed CS (2 + 2)

RECORD_FIELDS = [0x186, 0x188, 0x1CC, 0x1F6, 0x1FA, 0x1FC]
RECORD_SEGS = [0x3000, 0x4000, 0x5000, 0x6000, 0x7000]

# stub (cs, ip) -> (name, pushed argument words)
STUBS = {
    (0x3D5, 0x0034): ("34", 2),
    (0x3D5, 0x0025): ("25", 4),
    (0x3D5, 0x0039): ("39", 3),
    (0x3CD, 0x0025): ("cd", 2),
    (CS_SEG, 0x0000): ("z", 0),
}
NATIVE_NEAR = {(CS_SEG, 0x00C7): "00c7", (CS_SEG, 0x00FB): "00fb", (CS_SEG, 0x012F): "012f"}
EFFECT_OFFSETS = [0x49FA, 0x49D2, 0x49D3, 0x84E5, 0x6AAC, 0x6AAD, 0x6AAE, 0x6AAF, 0x6AB1, 0x6A60, 0x6A61, 0x6A62,
                  0x6A63, 0x6F68, 0x6F69, 0x3C76, 0x3C77, 0x3C78, 0x3C79, 0x3C7A]

MUTANTS = [
    ("entry-ah-ignored", "ax = (uint16_t)((uint16_t)entry_ah << 8);", "ax = (uint16_t)((uint16_t)(entry_ah & 0u) << 8);"),
    ("callee-25-return-ah-dropped", "ax = h->call_3d5_25(env->ctx, w1, w2, w3, w4);",
     "(void)h->call_3d5_25(env->ctx, w1, w2, w3, w4);"),
    ("third-word-of-39-ah-zeroed", "ax = (uint16_t)((ax & 0xFF00u) | ds[0x6AAFu]);", "ax = (uint16_t)(ds[0x6AAFu]);"),
    ("cbw-keeps-ah-instead-of-sign", "ax = (uint16_t)(int16_t)(int8_t)b1;", "ax = (uint16_t)((ax & 0xFF00u) | (uint8_t)b1);"),
    ("upper-clamp-b2-dropped", "if ((int8_t)b2 > 0x1F) {", "if (0) {"),
    ("coordinate-path-skipped", "if (flag < 2u || flag > 4u) {", "if (1) {"),
]


def rd16(uc, lin):
    return int.from_bytes(uc.mem_read(lin, 2), "little")


def gen_case(rng):
    case = {
        "entry_ah": rng.randrange(256),
        "entry_sp": rng.randrange(0x0200, 0xF000),
        "bp0": rng.randrange(65536),
        "ds": {},
        "far": [],
        "ret34": rng.randrange(65536),
        "ret25": rng.randrange(65536),
        "ret39": rng.randrange(65536),
        "eff": {k: [] for k in ("34", "25", "39", "cd", "z")},
    }
    ds = case["ds"]
    ds[0x49FA] = rng.choice([0, 1, 1, 2, 3, 4, 5, 0xFF])
    ds[0x84E5] = rng.choice([0, 1, 0x80])
    for off in (0x6AAC, 0x6AAD, 0x6AAE, 0x6AAF, 0x6AB1, 0x6A60, 0x6A61, 0x6A62, 0x6A63, 0x6F68, 0x6F69):
        if rng.random() < 0.8:
            ds[off] = rng.randrange(256)
    for off in range(0x3C76, 0x3C7B):
        ds[off] = rng.randrange(256)
    rec_seg = rng.choice(RECORD_SEGS)
    rec_off = rng.randrange(0x0000, 0xF000)
    ds[0x49D2], ds[0x49D3] = rec_off & 0xFF, rec_off >> 8
    ds[0x49D4], ds[0x49D5] = rec_seg & 0xFF, rec_seg >> 8
    for field in RECORD_FIELDS:
        if field in (0x1CC, 0x1F6):
            val = rng.choice([0, 0, 1, 0x8000, rng.randrange(65536)])
        elif field in (0x1FA, 0x1FC):
            val = rng.randrange(0x0000, 0xF000)   # used as a DS offset with +0xC26: kept below 0xF000 so no wrap
        else:
            val = rng.randrange(65536)
        case["far"].append((rec_seg, rec_off + field, val))
    for name in ("34", "25", "39", "cd", "z"):
        for _ in range(rng.randrange(3)):
            off = rng.choice(EFFECT_OFFSETS) if rng.random() < 0.7 else rng.randrange(0x6000, 0x8000)
            if off == 0x49D3:
                val = rng.randrange(0x30, 0x71)  # keeps the record offset below 0xF000 (hi byte), so no wrap
            elif off == 0x49D5:
                val = rng.choice([0x30, 0x40, 0x50, 0x60, 0x70])
            else:
                val = rng.randrange(256)
            case["eff"][name].append((off, val))
    return case


def case_text(case):
    lines = ["case", "entry_ah %d" % case["entry_ah"]]
    lines += ["ds %d %d" % (off, val) for off, val in sorted(case["ds"].items())]
    lines += ["far %d %d %d" % w for w in case["far"]]
    lines += ["ret34 %d" % case["ret34"], "ret25 %d" % case["ret25"], "ret39 %d" % case["ret39"]]
    for name in ("34", "25", "39", "cd", "z"):
        lines += ["eff %s %d %d" % (name, off, val) for off, val in case["eff"][name]]
    lines.append("end")
    return "\n".join(lines) + "\n"


def run_unicorn_case(unit, case):
    log = []
    cov = {"p1": 0, "p2": 0, "e84": 0, "arity": 0}
    uc = Uc(UC_ARCH_X86, UC_MODE_16)
    uc.mem_map(0, 0x100000, UC_PROT_ALL)
    uc.mem_write(CS_SEG * 16, unit)
    ds_init = dict(case["ds"])
    for off, val in ds_init.items():
        uc.mem_write(DS_SEG * 16 + off, bytes([val]))
    for seg, off, word in case["far"]:
        uc.mem_write(seg * 16 + off, word.to_bytes(2, "little"))
    entry_sp = case["entry_sp"]
    frame_sp = (entry_sp - 6) & 0xFFFF
    uc.mem_write(SS_SEG * 16 + entry_sp, RET_IP.to_bytes(2, "little") + RET_CS.to_bytes(2, "little"))
    uc.reg_write(UC_X86_REG_CS, CS_SEG)
    uc.reg_write(UC_X86_REG_IP, BODY_OFF)
    uc.reg_write(UC_X86_REG_DS, DS_SEG)
    uc.reg_write(UC_X86_REG_ES, 0)
    uc.reg_write(UC_X86_REG_SS, SS_SEG)
    uc.reg_write(UC_X86_REG_SP, entry_sp)
    uc.reg_write(UC_X86_REG_BP, case["bp0"])
    uc.reg_write(UC_X86_REG_AX, (case["entry_ah"] << 8) | 0x5A)   # AL is not read on entry

    def on_code(mu, address, size, _user):
        cs = mu.reg_read(UC_X86_REG_CS)
        ip = mu.reg_read(UC_X86_REG_IP)
        if cs == CS_SEG:
            if ip == 0x0193:
                cov["p1"] += 1
            elif ip == 0x0238:
                cov["p2"] += 1
            elif ip == 0x01CC:
                cov["e84"] += 1
        key = (cs, ip)
        sp = mu.reg_read(UC_X86_REG_SP)
        ss = mu.reg_read(UC_X86_REG_SS) * 16
        if key in NATIVE_NEAR:
            # the real near case map runs natively; log its argument word from the stack ([SP+6] per its header)
            log.append("near %s %d" % (NATIVE_NEAR[key], rd16(mu, ss + ((sp + 6) & 0xFFFF))))
            return
        if key not in STUBS:
            return
        name, nwords = STUBS[key]

        def word_at(k):
            return rd16(mu, ss + ((sp + k) & 0xFFFF))

        args = [word_at(4 + 2 * (nwords - 1 - k)) for k in range(nwords)]
        ret_ip, ret_cs = word_at(0), word_at(2)
        if sp != (frame_sp - 2 * nwords - NEAR_BYTES) & 0xFFFF:
            cov["arity"] += 1
        if name == "34":
            log.append("call 34 %d %d" % tuple(args))
        elif name == "25":
            log.append("call 25 %d %d %d %d" % tuple(args))
        elif name == "39":
            log.append("call 39 %d %d %d" % tuple(args))
        elif name == "cd":
            log.append("call cd %d %d" % tuple(args))
        else:
            log.append("near 0000")
        for off, val in case["eff"][name]:
            mu.mem_write(DS_SEG * 16 + off, bytes([val]))
        if name == "34":
            mu.reg_write(UC_X86_REG_AX, case["ret34"])
        elif name == "25":
            mu.reg_write(UC_X86_REG_AX, case["ret25"])
        elif name == "39":
            mu.reg_write(UC_X86_REG_AX, case["ret39"])
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
        uc.emu_start(CS_SEG * 16 + BODY_OFF, RET_CS * 16 + RET_IP, timeout=5000000)
    except UcError as e:
        raise RuntimeError("oracle emulation stopped (%s); %s" % (e, "; ".join(faults) or "no unmapped access"))
    if faults:
        raise RuntimeError("oracle touched unmapped memory: " + "; ".join(faults))
    returned = uc.reg_read(UC_X86_REG_CS) == RET_CS and uc.reg_read(UC_X86_REG_IP) == RET_IP
    balanced = uc.reg_read(UC_X86_REG_SP) == (entry_sp + 4) & 0xFFFF and uc.reg_read(UC_X86_REG_BP) == case["bp0"]
    ok = returned and balanced and cov["arity"] == 0
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
    unit = ovr[UNIT_FILE:UNIT_FILE + UNIT_LEN]
    body = unit[BODY_OFF:BODY_OFF + BODY_LEN]
    if len(unit) != UNIT_LEN or hashlib.sha256(body).hexdigest() != BODY_SHA:
        print("FAIL the 307-byte span at unit +0x016D is not the pinned bytes")
        return 1
    os.makedirs(a.work_dir, exist_ok=True)

    rng = random.Random(a.seed)
    cases = [gen_case(rng) for _ in range(a.cases)]
    text = "".join(case_text(c) for c in cases)
    results = [run_unicorn_case(unit, c) for c in cases]
    oracle = [r[0] for r in results]
    print("COVERAGE (executed in emulation): flag-1 path in %d cases; coordinate path (flag 2..4) in %d; [0x84E5] set in %d; "
          "stub arity mismatches %d"
          % (sum(1 for r in results if r[1]["p1"]), sum(1 for r in results if r[1]["p2"]),
             sum(1 for r in results if r[1]["e84"]), sum(r[1]["arity"] for r in results)))

    base_bin = os.path.join(a.work_dir, "harness_base")
    build(SRC, base_bin)
    base = split_cases(run_harness(base_bin, text))
    failures = 0
    for idx, (want, got) in enumerate(zip(oracle, base)):
        if want != got:
            failures += 1
            if failures <= 3:
                print("FAIL case %d differs" % idx)
                print("  oracle:", want[:14])
                print("  C     :", got[:14])
    if len(base) != len(oracle):
        print("FAIL case count: oracle %d, C %d" % (len(oracle), len(base)))
        failures += 1
    print("RESULT baseline: %d/%d cases equal" % (len(oracle) - failures, len(oracle)))

    mutant_fail = 0
    src_text = open(SRC).read()
    for name, old, new in MUTANTS:
        if src_text.count(old) != 1:
            print("FAIL mutant %s: anchor not unique (%d)" % (name, src_text.count(old)))
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

    ok = failures == 0 and mutant_fail == 0 and all(r[1]["arity"] == 0 for r in results)
    print("RESULT %s" % ("PASS" if ok else "FAIL"))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
