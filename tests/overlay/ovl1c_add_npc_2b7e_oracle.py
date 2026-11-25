#!/usr/bin/env python3
"""Differential oracle for unit 0x1c +0x2B7E (ECL opcode 0x36, ADD NPC).

The 133 owner bytes at GAME.OVR file offset 0x38C4 run under Unicorn (16-bit real mode), loaded at unit offset 0x2B7E of
the code segment 0x0100 so that the unit's own jump targets resolve, entered at IP 0x2B7E. Its five external calls go to
stubs keyed by their target: lcall 0x2B:0x2A (loader, one word), lcall 0x2B:0x25 (slot value, one word, result in AX),
lcall 0x7C:0x43 (add-NPC, one word), lcall 0xBA:0x0BB8 and lcall 0xBA:0x0767 (far pointer, two words, segment pushed
first). Each stub logs its arguments, returns the case's slot word for slot values, and pops its arguments with RETF.
The same cases run through the C model (the vector driver tests/overlay/ovl1c_add_npc_2b7e_harness.c linked with
src/recovered/overlay_u1c_add_npc_2b7e.c). Compared per case: the call log, the status, and every record byte that
differs from its input.

The status requires SP restored to the entry value plus 4, BP to its entry value, and every stub entry to see the
stack the routine implies (SP = entry SP - 12 - 2 x argument words). The far record pointer at DS:0x5D92 (offset) and
DS:0x5D94 (segment) points into a 0x11D-byte record; offsets are kept below 0xE000 so no 16-bit wrap is generated.

Scope: the stubs return OK-status values only. HOOK, BOUNDS, UNRESOLVED and ARGUMENT statuses are covered by the focused
test tests/recovered/test_overlay_u1c_add_npc_2b7e.c, not by this oracle. The record pointer is read once on entry by the
model; a callee that rewrites DS:0x5D92 is not modelled here.

Mutants: one-edit copies of the translation must each differ on at least one case. The owner GAME.OVR is read at run time
and never committed; the work directory must be outside the repository.

Usage: ovl1c_add_npc_2b7e_oracle.py --game-ovr PATH --work-dir DIR [--cases N] [--seed S]
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
SRC = os.path.join(ROOT, "src", "recovered", "overlay_u1c_add_npc_2b7e.c")
ECL = os.path.join(ROOT, "src", "recovered", "ecl_helpers.c")
INC = os.path.join(ROOT, "include")
HARNESS = os.path.join(ROOT, "tests", "overlay", "ovl1c_add_npc_2b7e_harness.c")

FILE_OFF = 0x38C4
SPAN_LEN = 133
SPAN_SHA = "5b711971cd12bfbcd60d87f2ac6a6cb836292b4729229417fa73962d48efeca1"
UNIT_OFF = 0x2B7E
CS_SEG = 0x0100
RET_CS = 0x0000
RET_IP = 0x0F00
DS_SEG = 0x2000
SS_SEG = 0x9000
REC_SIZE = 0x11D
REC_SEGS = [0x3000, 0x4000, 0x5000, 0x6000, 0x7000]
FAR_OFF_ADDR = 0x5D92
FAR_SEG_ADDR = 0x5D94

# (target segment, target offset): (name, word arguments)
STUBS = {
    (0x2B, 0x2A): ("load", 1),
    (0x2B, 0x25): ("slot", 1),
    (0x7C, 0x43): ("npc", 1),
    (0xBA, 0x0BB8): ("r0bb8", 2),
    (0xBA, 0x0767): ("r0767", 2),
}

MUTANTS = [
    ("class-without-or-80", "r->class_byte = (uint8_t)(((uint8_t)(v2 & 0xFFu) >> 1) | 0x80u);",
     "r->class_byte = (uint8_t)(((uint8_t)(v2 & 0xFFu) >> 1));"),
    ("class-shift-2", "r->class_byte = (uint8_t)(((uint8_t)(v2 & 0xFFu) >> 1) | 0x80u);",
     "r->class_byte = (uint8_t)(((uint8_t)(v2 & 0xFFu) >> 2) | 0x80u);"),
    ("flag-id-19", "(v1 & 0xFFu) == PR_U1C_ADD_NPC_FLAG_ID ? 1u : 0u",
     "(v1 & 0xFFu) == 0x19u ? 1u : 0u"),
    ("flag-inverted", "(v1 & 0xFFu) == PR_U1C_ADD_NPC_FLAG_ID ? 1u : 0u",
     "(v1 & 0xFFu) == PR_U1C_ADD_NPC_FLAG_ID ? 0u : 1u"),
    ("npc-word-low-byte-only", "h->add_npc(h->context, v1);",
     "h->add_npc(h->context, (uint16_t)(v1 & 0xFFu));"),
    ("class-stored-at-flag", "rec->record[PR_U1C_ADD_NPC_OFF_CLASS] = r->class_byte;",
     "rec->record[PR_U1C_ADD_NPC_OFF_FLAG] = r->class_byte;"),
]


def rd16(uc, lin):
    return int.from_bytes(uc.mem_read(lin, 2), "little")


def gen_case(rng):
    case = {"slot": {1: rng.randrange(65536), 2: rng.randrange(65536)},
            "rec_seg": rng.choice(REC_SEGS), "rec_off": rng.randrange(0x0000, 0xE000),
            "rec": bytes(rng.randrange(256) for _ in range(REC_SIZE)),
            "entry_sp": rng.randrange(0x0400, 0xE000), "bp0": rng.randrange(65536)}
    # bias some cases toward the flag and class boundaries
    if rng.random() < 0.3:
        case["slot"][1] = (case["slot"][1] & 0xFF00) | 0x18
    if rng.random() < 0.3:
        case["slot"][2] = (case["slot"][2] & 0xFF00) | rng.choice([0x00, 0x01, 0x7F, 0x80, 0xFF])
    return case


def case_text(case):
    lines = ["case", "slot 1 %d" % case["slot"][1], "slot 2 %d" % case["slot"][2],
             "rec %d %d" % (case["rec_seg"], case["rec_off"]), "recdata " + case["rec"].hex(), "end"]
    return "\n".join(lines) + "\n"


def run_unicorn_case(body, case):
    log = []
    cov = {"flag1": 0, "flag0": 0, "arity": 0, "calls": 0}
    uc = Uc(UC_ARCH_X86, UC_MODE_16)
    uc.mem_map(0, 0x100000, UC_PROT_ALL)
    uc.mem_write(CS_SEG * 16 + UNIT_OFF, body)
    uc.mem_write(DS_SEG * 16 + FAR_OFF_ADDR, case["rec_off"].to_bytes(2, "little"))
    uc.mem_write(DS_SEG * 16 + FAR_SEG_ADDR, case["rec_seg"].to_bytes(2, "little"))
    rec_base = case["rec_seg"] * 16 + case["rec_off"]
    uc.mem_write(rec_base, case["rec"])
    entry_sp = case["entry_sp"]
    uc.mem_write(SS_SEG * 16 + entry_sp, RET_IP.to_bytes(2, "little") + RET_CS.to_bytes(2, "little"))
    uc.reg_write(UC_X86_REG_CS, CS_SEG)
    uc.reg_write(UC_X86_REG_IP, UNIT_OFF)
    uc.reg_write(UC_X86_REG_DS, DS_SEG)
    uc.reg_write(UC_X86_REG_ES, 0)
    uc.reg_write(UC_X86_REG_SS, SS_SEG)
    uc.reg_write(UC_X86_REG_SP, entry_sp)
    uc.reg_write(UC_X86_REG_BP, case["bp0"])
    uc.reg_write(UC_X86_REG_AX, 0x5A5A)

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

        # pushed words in push order: the first pushed is the deepest
        args = [word_at(4 + 2 * (nwords - 1 - k)) for k in range(nwords)]
        ret_ip, ret_cs = word_at(0), word_at(2)
        if sp != (entry_sp - 12 - 2 * nwords) & 0xFFFF:
            cov["arity"] += 1
        cov["calls"] += 1
        if name == "load":
            log.append("load %d" % (args[0] & 0xFF))
        elif name == "slot":
            slot = args[0] & 0xFF
            value = case["slot"][slot]
            log.append("slot %d %d" % (slot, value))
            mu.reg_write(UC_X86_REG_AX, value)
        elif name == "npc":
            log.append("npc %d" % args[0])
            cov["flag1" if (args[0] & 0xFF) == 0x18 else "flag0"] += 1
        elif name == "r0bb8":
            log.append("r0bb8 %d %d" % (args[0], args[1]))
        elif name == "r0767":
            log.append("r0767 %d %d" % (args[0], args[1]))
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
        uc.emu_start(CS_SEG * 16 + UNIT_OFF, RET_CS * 16 + RET_IP, timeout=5000000)
    except UcError as e:
        raise RuntimeError("oracle emulation stopped (%s); %s" % (e, "; ".join(faults) or "no unmapped access"))
    if faults:
        raise RuntimeError("oracle touched unmapped memory: " + "; ".join(faults))
    returned = uc.reg_read(UC_X86_REG_CS) == RET_CS and uc.reg_read(UC_X86_REG_IP) == RET_IP
    balanced = uc.reg_read(UC_X86_REG_SP) == (entry_sp + 4) & 0xFFFF and uc.reg_read(UC_X86_REG_BP) == case["bp0"]
    ok = returned and balanced and cov["arity"] == 0
    out = list(log)
    out.append("status 0" if ok else "status 9")
    final = bytes(uc.mem_read(rec_base, REC_SIZE))
    for i in range(REC_SIZE):
        if final[i] != case["rec"][i]:
            out.append("rec %d %d" % (i, final[i]))
    out.append("endcase")
    return out, cov


def run_harness(binary, text):
    res = subprocess.run([binary], input=text, capture_output=True, text=True, timeout=600)
    if res.returncode != 0:
        raise RuntimeError("harness failed: %s" % res.stderr.strip())
    return res.stdout.splitlines()


def build(src, out):
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-I", INC, src, ECL, HARNESS, "-o", out],
                   check=True)


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
        print("FAIL the 133-byte span at unit +0x2B7E is not the pinned bytes")
        return 1
    os.makedirs(a.work_dir, exist_ok=True)

    rng = random.Random(a.seed)
    cases = [gen_case(rng) for _ in range(a.cases)]
    text = "".join(case_text(c) for c in cases)
    results = [run_unicorn_case(body, c) for c in cases]
    oracle = [r[0] for r in results]
    tot = {k: sum(r[1][k] for r in results) for k in results[0][1]}
    print("COVERAGE (executed in emulation): calls %d; add-NPC with id 0x18 %d; with another id %d; stub arity mismatches %d"
          % (tot["calls"], tot["flag1"], tot["flag0"], tot["arity"]))

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

    ok = failures == 0 and mutant_fail == 0 and tot["arity"] == 0
    print("RESULT %s" % ("PASS" if ok else "FAIL"))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
