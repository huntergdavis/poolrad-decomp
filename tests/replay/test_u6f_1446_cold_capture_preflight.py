#!/usr/bin/env python3
"""Synthetic offline artifact consistency tests; no guest executor."""
import copy
from hashlib import sha256
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("preflight", ROOT / "tools/replay/u6f_1446_cold_capture_preflight.py")
preflight = importlib.util.module_from_spec(spec)
spec.loader.exec_module(preflight)


def put(ram, seg, off, value):
    p = (seg << 4) + off
    ram[p:p + 2] = value.to_bytes(2, "little")


class CaptureTests(unittest.TestCase):
    def setUp(self):
        (ROOT / "resources").mkdir(exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(dir=ROOT / "resources")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.manifest = self.root / "manifest.json"
        self.data = preflight.plan("synthetic-one-invocation")
        ram = bytearray(655360)
        put(ram, 0, 0xfc, 0x2e6); put(ram, 0, 0xfe, 0x12c6)
        p = (0x8d0 << 4) + 0x25
        ram[p:p + 4] = bytes((0xcd, 0x3f, 0, 0))
        put(ram, 0x1d16, 0x3796, 0x1579); put(ram, 0x1d16, 0x3798, 0x2a8e)
        put(ram, 0x1d16, 0x37ae, 0x3dca)
        # Deliberately different from retained19E6: successor must be live.
        put(ram, 0x1d16, 0x37b0, 0x1a00); put(ram, 0x1d16, 0x37b2, 0x2a8e)
        cpu = dict(zip(preflight.CPU_FIELDS,
                       [0x8d0, 0x25, 0x14a0, 0x3a16, 0, 0, 0x1d16,
                        0x3796, 0x37ae, 0, 0x1f, 7, 0x449a, 0x44ba, 0, 0x7202]))
        for stop, (label, cs, ip) in zip(self.data["stops"], preflight.CUTS):
            c = dict(cpu); image = bytearray(ram)
            c["CS"] = cs if cs is not None else 0x2a8e
            c["IP"] = ip if ip is not None else 0x1a00
            if label == "entry": c.update(SP=0x37b0, BP=0x3dca)
            if label in ("handler", "iret"):
                c["SP"] = 0x3790
                c["FLAGS"] = 0x7002 if label == "handler" else 0x7297
                put(image, c["SS"], 0x3790, 0x27 if label == "handler" else 0x25)
                put(image, c["SS"], 0x3792, 0x8d0)
                put(image, c["SS"], 0x3794, 0x7202)
            if label in ("patched", "u9b", "callee_return", "outer_ret", "outer_return"):
                image[p:p + 5] = bytes((0xea, 0, 0, 0xcd, 0x2c))
            if label == "callee_return": c["SP"] = 0x379e
            if label == "outer_ret": c.update(SP=0x37b0, BP=0x3dca)
            if label == "outer_return": c.update(SP=0x37b8, BP=0x3dca)
            stop["cpu"] = c
            file = self.root / stop["ram_file"]
            file.write_bytes(image); stop["ram_sha256"] = sha256(image).hexdigest()
        self.save()

    def save(self):
        self.manifest.write_text(json.dumps(self.data))

    def reject(self, text):
        self.save()
        with self.assertRaisesRegex(ValueError, text): preflight.check(self.manifest)

    def test_valid_cut_consistency_is_not_parity(self):
        result = preflight.check(self.manifest)
        self.assertEqual(result["cuts"], 9)
        self.assertFalse(result["native_parity"])
        self.assertFalse(result["device_proof"])
        self.assertEqual(result["new_strict_credit"], 0)

    def test_run_cpu_hash_and_order_rejection(self):
        original = copy.deepcopy(self.data)
        for change, error in [
            (lambda: self.data["stops"][4].update(run_id="other-run"), "mixed run"),
            (lambda: self.data["stops"][4]["cpu"].pop("FS"), "all16"),
            (lambda: self.data["stops"][4].update(ram_sha256="0" * 64), "RAM size/hash"),
            (lambda: self.data["stops"][4].update(ordinal=0), "stop order"),
        ]:
            self.data = copy.deepcopy(original); change(); self.reject(error)

    def test_cold_and_iret_frames_cannot_be_invented(self):
        for label, off, error in [("cold", 0x25, "actually be cold"),
                                  ("iret", 0x3790, "live IRET frame")]:
            row = next(x for x in self.data["stops"] if x["label"] == label)
            file = self.root / row["ram_file"]; original = file.read_bytes()
            image = bytearray(original)
            p = ((0x8d0 if label == "cold" else 0x1d16) << 4) + off
            image[p] ^= 1; file.write_bytes(image)
            row["ram_sha256"] = sha256(image).hexdigest(); self.reject(error)
            file.write_bytes(original); row["ram_sha256"] = sha256(original).hexdigest()

    def test_live_successor_and_missing_coverage(self):
        self.data["stops"][-1]["cpu"]["IP"] = 0x19e6
        self.reject("derived live outer successor")
        self.data["ram_bytes"] = 1024; self.reject("RAM coverage")

    def test_malformed_manifest_objects_rejected(self):
        self.manifest.write_text("[]")
        with self.assertRaisesRegex(ValueError, "manifest object"): preflight.check(self.manifest)
        self.data["stops"][0] = []
        self.reject("cut object")

    def test_index_requires_raw_complete_ordered_ev(self):
        plan = self.root / "plan.json"
        plan.write_text(json.dumps(preflight.plan(self.data["run_id"])))
        raw = self.root / "ev-numeric.tsv"
        lines = ["label " + " ".join(preflight.CPU_FIELDS)]
        lines += [row["label"] + " " + " ".join(f'{row["cpu"][f]:x}' for f in preflight.CPU_FIELDS)
                  for row in self.data["stops"]]
        raw.write_text("\n".join(lines) + "\n")
        indexed = preflight.index(plan, raw)
        self.manifest.write_text(json.dumps(indexed))
        self.assertEqual(preflight.check(self.manifest)["cuts"], 9)
        raw.write_text("\n".join(lines[:-1]))
        with self.assertRaisesRegex(ValueError, "nine ordered rows"): preflight.index(plan, raw)


if __name__ == "__main__":
    unittest.main()
