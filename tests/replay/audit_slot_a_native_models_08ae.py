#!/usr/bin/env python3
"""`--native` (synthetic DOSBox-X) corroboration of the four 08AE C models; print no game or RAM payload.

The four per-function audits (`res08ae_table_1742_cpu_audit`, `res08ae_table_17ae_cpu_audit`,
`res08ae_vector_13b1_cpu_audit`, `res08ae_dispatch_00dc_cpu_audit`) each have a `--native` mode that assembles a
synthetic ORACLE.COM, runs it in DOSBox-X and compares every case with the C model.  This harness only
ORCHESTRATES them and checks their reports; it adds no model code and changes no source.

    collect WORK_DIR   run every audit with `--native` on two emulator builds (the system `dosbox-x` found on PATH,
                       which the audits record as a declared substitution, and the repository's pinned debugger build
                       through a PATH shim); the system pass also runs `--unicorn` so one report carries both
    verify  WORK_DIR   check the reports against pins and print the account: body SHAs, cases, masked RAM, ownership,
                       cross-emulator agreement, divergence from the filed Unicorn counts

`POOLRAD_NATIVE_MUTANT=<name>` perturbs one verify branch (see check_slot_a_native_models_08ae.sh).  Nothing here
verifies identity or parity of the routines: the audits' ownership records say `credit_claimed: false` and
`identity: unresolved` and this harness only checks that they still do.
"""

from pathlib import Path
import collections
import json
import os
import shutil
import struct
import subprocess
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.parent
MUT = os.environ.get("POOLRAD_NATIVE_MUTANT", "")
SYSTEM_EMU = dict(path="/usr/bin/dosbox-x", version="DOSBox-X version 2026.01.02 SDL2",
                  sha256="99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3")
PINNED_EMU = dict(path="resources/hybrid/bin/dosbox-x", version="DOSBox-X version 2026.10.01 SDL2",
                  sha256="d85a8563ba98a0a144032eb4569ba6e7753f01db1026c516054e4c144afcf973")
EXPANDED = "a42a57e68d3f7815831821cc9365bb917befda45a58e958544451fc7a878df8e"
SYNTHETIC_MASKED = (1 << 20) - 65792
MODELS = {
    "1742": dict(audit="res08ae_table_1742_cpu_audit", body="6128c5126be22f692937ccbb5e5eb2b07c85081a04f086e9bb84579486258484",
                 starts=43, native_cases=48, unicorn=dict(argument_cases=4, return_cases=603, reference_checked_cases=275,
                                                          conditional_outcomes=6, masked_ram_bytes=0)),
    "17AE": dict(audit="res08ae_table_17ae_cpu_audit", body="8bb73768c1c77c46b1695935e45b9ab162b9c7863c94529cbbffa248001c52a8",
                 starts=43, native_cases=48, unicorn=dict(argument_cases=4, return_cases=1019, reference_checked_cases=275,
                                                          conditional_outcomes=6, masked_ram_bytes=0)),
    "13B1": dict(audit="res08ae_vector_13b1_cpu_audit", body="0fa5df2cbe185bdb43d4ff225e14b5b23a6893e85721aac20a99f86b34a0b019",
                 starts=27, native_cases=32, unicorn=dict(cases=480, events=4320, in_values_swept=256, masked_ram_bytes=0)),
    "00DC": dict(audit="res08ae_dispatch_00dc_cpu_audit", body="aae1673000ebd85e67c66df978d2b986de8e4d52cdd7994ade46f1ac5d50bb2a",
                 starts=77, native_cases=48, unicorn=dict(argument_cases=5, cases=1788, reference_checked_cases=1050,
                                                          conditional_outcomes=16, masked_ram_bytes=0)),
}
DISPATCH_NATIVE_STOPS = {"CALL_006C": 10, "CALL_137C": 1, "CALL_13B1": 3, "CALL_1742": 11, "CALL_17AE": 7,
                         "CALL_181A": 4, "RETURN": 12}
UNICORN_STOPS = {"1742": {"ARGUMENT": 4, "RETURN": 603}, "17AE": {"ARGUMENT": 4, "RETURN": 1019}, "13B1": {"RETURN": 480},
                 "00DC": {"ARGUMENT": 5, "CALL_006C": 373, "CALL_137C": 86, "CALL_13B1": 201, "CALL_1742": 332,
                          "CALL_17AE": 196, "CALL_181A": 108, "RETURN": 492}}
REC_13B1 = 28 + 0x100 + 10        # CPU words, stack window, slot/shadow/refusal bytes per native case
PF_BIT = 0x0004
FLAG_MASK_13B1 = 0x00C4           # SF/ZF/PF: IN 61h reads live emulated hardware


def check(condition, message):
    if not condition:
        raise SystemExit(message)


def work_dir(arg):
    work = Path(arg).resolve()
    resources = (ROOT / "resources").resolve()
    check(resources in work.parents, "WORK_DIR must be under this tree's ignored resources/")
    return work


def sha(path):
    from hashlib import sha256
    return sha256(Path(path).read_bytes()).hexdigest()


def collect(work):
    work.mkdir(parents=True, exist_ok=True)
    (work / "tmp").mkdir(exist_ok=True)
    shim = work / "pinned-bin"
    shim.mkdir(exist_ok=True)
    pinned = ROOT / PINNED_EMU["path"]
    check(sha(pinned) == PINNED_EMU["sha256"], "pinned DOSBox-X binary identity mismatch")
    check(sha(SYSTEM_EMU["path"]) == SYSTEM_EMU["sha256"], "system DOSBox-X binary identity mismatch")
    link = shim / "dosbox-x"
    if link.is_symlink() or link.exists():
        link.unlink()
    link.symlink_to(pinned)
    for key, m in MODELS.items():
        for emu in ("system", "pinned"):
            env = dict(os.environ, TMPDIR=str(work / "tmp"))
            if emu == "pinned":
                env["PATH"] = str(shim) + os.pathsep + env["PATH"]
            args = ["python3", str(ROOT / "tools/overlay" / (m["audit"] + ".py")), "--native",
                    "--output", str(work / ("%s.%s.json" % (key, emu)))]
            if emu == "system":
                args.insert(2, "--unicorn")
            with open(work / ("%s.%s.log" % (key, emu)), "w") as log:
                done = subprocess.run(args, env=env, stdout=log, stderr=subprocess.STDOUT, cwd=ROOT, timeout=1800)
            check(done.returncode == 0, "%s --native on the %s emulator failed (rc %d), see %s.%s.log" % (key, emu, done.returncode, key, emu))
            print("collected %s on the %s emulator" % (key, emu), flush=True)
            if key == "13B1":                     # the native records are needed for the cross-emulator diff
                shutil.copy(ROOT / "resources/oracle-runs/res08ae-13b1-synthetic/RESULT.BIN", work / ("13B1-result-%s.bin" % emu))


def load(work, key, emu):
    return json.loads((work / ("%s.%s.json" % (key, emu))).read_text())


def verify(work):
    print("emulators: system %s (%s...), pinned build %s (%s...)" %
          (SYSTEM_EMU["version"], SYSTEM_EMU["sha256"][:12], PINNED_EMU["version"], PINNED_EMU["sha256"][:12]))
    rows = []
    for key, m in MODELS.items():
        reports = {emu: load(work, key, emu) for emu in ("system", "pinned")}
        for emu, rep in reports.items():
            want = SYSTEM_EMU if emu == "system" else PINNED_EMU
            e = rep["native"]["dosbox_x"]
            expected_sha = "0" * 64 if MUT == "emu-sha" else want["sha256"]
            check(e["sha256"] == expected_sha and e["version"] == want["version"] and
                  Path(e["path"]).resolve() == (Path(want["path"]) if emu == "system" else ROOT / want["path"]).resolve(),
                  "%s %s: emulator identity differs from the pin: %s" % (key, emu, e))
            body = ("f" * 64) if MUT == "expect-body" else m["body"]
            check(rep["target"]["sha256"] == body and rep["target"]["expanded_sha256"] == EXPANDED and rep["target"]["starts"] == m["starts"],
                  "%s %s: body or expanded image SHA differs from the filed reports" % (key, emu))
            n = rep["native"]
            cases = (m["native_cases"] - 1) if MUT == "expect-cases" else m["native_cases"]
            check(n["cases"] == cases and n["cpu_words"] == 14, "%s %s: native case count %s" % (key, emu, n["cases"]))
            if key == "13B1":
                check(n["ram_bytes_compared"] == 264 and n["ram_bytes_not_compared"] == (1 << 20) - 264 and n["real_int21"] is True and
                      n["port_writes_observed"] is False and n["pinned_build_used"] is False and
                      n["shadow_vector_table"] is True and n["synthetic_frames"] is True, "13B1 %s: native scope fields changed" % emu)
            else:
                masked = SYNTHETIC_MASKED + (1 if MUT == "expect-masked" else 0)
                check(n["ram_bytes"] == 65792 and n["masked_ram_bytes"] == masked and n["synthetic_frames"] is True,
                      "%s %s: native compared/masked RAM changed: %s/%s" % (key, emu, n["ram_bytes"], n["masked_ram_bytes"]))
            if key == "00DC":
                check(n["stops"] == DISPATCH_NATIVE_STOPS and n["callee_stubs"] == 6, "dispatcher %s: native stop mix changed" % emu)
            own = reports[emu].get("ownership")
            if key == "13B1":
                check(own is None, "13B1 %s: an ownership record appeared" % emu)
            else:
                claimed = True if MUT == "expect-owner" else False
                check(own["credit_claimed"] is claimed and own["identity"] == "unresolved",
                      "%s %s: ownership record is no longer credit_claimed false / identity unresolved" % (key, emu))
        # the system pass also ran --unicorn: compare with the filed counts
        u = reports["system"]["unicorn"]
        filed = dict(m["unicorn"])
        if MUT == "unicorn-count":
            filed[next(iter(filed))] += 1
        check(all(u[k] == v for k, v in filed.items()) and u["cpu_words"] == 14 and u["ram_bytes"] == 1 << 20 and
              u["starts"] == m["starts"] and u["starts_total"] == m["starts"] and u["stops"] == UNICORN_STOPS[key],
              "%s: the fresh Unicorn counts differ from the filed reports" % key)
        # emulator agreement
        same = reports["system"]["native"]["result_sha256"] == reports["pinned"]["native"]["result_sha256"]
        rows.append((key, m, reports, same))
    print("per-model account (native = synthetic DOSBox-X cases vs the C model; unicorn = original bytes in Unicorn, 1 MiB unmasked):")
    for key, m, reports, same in rows:
        n, u = reports["system"]["native"], reports["system"]["unicorn"]
        compared = "stack window %d B + slot/shadow 8 B" % 256 if key == "13B1" else "%d B of 1 MiB (masked %d)" % (n["ram_bytes"], n["masked_ram_bytes"])
        own = reports["system"].get("ownership")
        print("  %-5s body %s... starts %d | native cases %d, RAM %s, flags masked %s | unicorn %s, masked %d | native result identical on both emulators: %s | ownership %s" %
              (key, m["body"][:8], m["starts"], n["cases"], compared, "SF/ZF/PF" if key == "13B1" else "none",
               "%d cases" % (u.get("cases") or u["return_cases"] + u["argument_cases"]), u["masked_ram_bytes"], same,
               "none recorded" if own is None else "credit_claimed %s, identity %s" % (own["credit_claimed"], own["identity"])))
    identical = [k for k, _, _, s in rows if s]
    check(identical == ["1742", "17AE", "00DC"] and [k for k, _, _, s in rows if not s] == ["13B1"],
          "cross-emulator result agreement changed: identical %s" % identical)
    # 13B1: the two builds differ only in masked flag bits (IN 61h reads live emulated hardware)
    a, b = ((work / ("13B1-result-%s.bin" % e)).read_bytes() for e in ("system", "pinned"))
    check(len(a) == len(b) == 8 + 32 * REC_13B1, "13B1 native result size changed")
    differing, bits, outside = 0, collections.Counter(), 0
    for i in range(32):
        ra, rb = a[8 + i * REC_13B1:8 + (i + 1) * REC_13B1], b[8 + i * REC_13B1:8 + (i + 1) * REC_13B1]
        if ra == rb:
            continue
        differing += 1
        fa, fb = struct.unpack_from("<H", ra, 26)[0], struct.unpack_from("<H", rb, 26)[0]
        bits[fa ^ fb] += 1
        outside += sum(1 for k in range(REC_13B1) if ra[k] != rb[k] and not 26 <= k < 28)
    mask = 0x00C0 if MUT == "mask-narrow" else FLAG_MASK_13B1      # mutant: a mask without PF
    print("13B1 native, system vs pinned emulator: %d of 32 records differ (the count varies run to run: IN 61h reads live emulated "
          "hardware), only in the flags word, xor %s (SF/ZF/PF mask 0x%04X), %d byte(s) differ outside the flags word" %
          (differing, {hex(k): v for k, v in bits.items()}, FLAG_MASK_13B1, outside))
    check(outside == 0 and set(bits) <= {PF_BIT} and all(k & ~mask == 0 for k in bits),
          "13B1 native records differ beyond the masked flag bits (or the mask was narrowed)")
    print("native corroboration verified: the models remain credit_claimed false / identity unresolved; no parity, function or credit follows")


def main():
    check(len(sys.argv) == 3 and sys.argv[1] in ("collect", "verify"), "usage: audit_slot_a_native_models_08ae.py collect|verify WORK_DIR")
    work = work_dir(sys.argv[2])
    collect(work) if sys.argv[1] == "collect" else verify(work)


if __name__ == "__main__":
    main()
