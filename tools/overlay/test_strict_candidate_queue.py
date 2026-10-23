#!/usr/bin/env python3
"""Focused synthetic checks for strict-candidate ranking and detection.

No media, git or decoder needed: the queue join reuses function_queue's
overlay rows, and the prior-translation evidence is injected as structured
records the real scanners produce. The checks cover the two behaviours the
queue exists for: an untranslated, game-owned, ready-callee original ranks
above a prior-translated one, and inline/branch evidence is reported.
"""

import copy
import contextlib
import csv
import hashlib
import io
import json
import re
import sys
import tempfile
from unittest import mock
from types import SimpleNamespace
from pathlib import Path
import unittest
import warnings
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parent))
import strict_candidate_queue as scq
import function_ledger as ledger_tool


def fixture() -> tuple[dict, list, list]:
    inventory = dict(inputs=scq.DIGESTS.copy(), units=[dict(
        stub_para=0x19, memory_segment=0x1C, ovr_offset=100, code_size=0x200,
        entries=[0x10], functions=[dict(rel=0x10), dict(rel=0x20), dict(rel=0x30)])])
    ledger, classifications = [], []
    for rel, grade, entry, near, classification in (
            (0x10, "A", 1, 0, "game_verified"),
            (0x20, "B", 0, 1, "game_evidenced"),
            (0x30, "U", 0, 0, "game_or_other_library_unresolved")):
        common = dict(id=f"0x19:{rel:#06x}", unit="0x19", offset=hex(rel),
                      ovr_offset=str(100 + rel), grade=grade)
        ledger.append(dict(common, entry=str(entry), entry_alias="0", near_called=str(near),
                           far_called="0", procptr="0", bound="40", decoded="40", insns="12",
                           cls="leaf"))
        classifications.append(dict(common, far_in="0", classification=classification,
                                    confidence="provisional"))
    return inventory, ledger, classifications


def evidence():
    mentions = {
        "OVL_0019_0020": {"source": [], "tooling": [], "docs": []},
        "OVL_0019_0030": {"source": [dict(file="src/recovered/overlay_u19_alpha_0010.c",
                                          line=9, kind="offset", text="call +0x0030")],
                          "tooling": [], "docs": []}}
    main_files = {"OVL_0019_0010": ["src/recovered/overlay_u19_alpha_0010.c"]}
    branches = {"OVL_0019_0020": [dict(sha="0" * 40, refs=["origin/codex/u19-0020"],
                                       path="src/recovered/overlay_u19_beta_0020.c")]}
    return mentions, main_files, branches


def callgraph():
    return dict(edges=[
        dict(kind="far_fixup_confirmed", unit=0x19, site=0x25, src_func=0x20, seg=0x1C, off=0x20,
             resolution="overlay", target_unit=0x19, target_entry=0, target_rel=0x10,
             target_func=0x10, exact_prologue=True),
        dict(kind="near_candidate", unit=0x19, site=0x35, src_func=0x30, target=0x20,
             target_func=0x20, exact_prologue=True),
        dict(kind="near_candidate", unit=0x19, site=0x1A, src_func=0x20, target=0x10,
             target_func=0x10, exact_prologue=True)])


def queue(data=None, graph=None, prior=None) -> dict:
    inventory, ledger, classifications = data or fixture()
    state = scq.load_queue(inventory, ledger, classifications,
                           [dict(id="OVL_0019_0010", evidence_doc="docs/re/synthetic.md")], None)
    mentions, main_files, branches = prior or evidence()
    overlay = bytearray(200)
    overlay[100 + 0x35 - 1] = 0x0E
    return scq.build_queue(state, graph or callgraph(), inventory["units"], ledger,
                           classifications, mentions, main_files, branches, {},
                           dict(ovr=bytes(overlay)))


class DeclaredBoundaryTest(unittest.TestCase):
    """A declared boundary endpoint composes but is never credited."""

    ROOT = Path(__file__).resolve().parents[2]

    def test_declaration_documents_exist(self):
        for endpoint, document in scq.DECLARED_BOUNDARY_ENDPOINTS.items():
            path = self.ROOT / document
            self.assertTrue(path.is_file(), f"{endpoint} names a missing document")
            text = path.read_text()
            self.assertIn(endpoint, text, f"{document} does not name {endpoint}")
            self.assertIn("NEVER a credit", text)

    def test_no_declared_boundary_appears_in_a_credit_table(self):
        """The invariant the loader also enforces, checked on the committed tables."""
        for table in ("docs/re/strict_full_c_addresses.csv",
                      "docs/re/reviewed_function_identities.csv"):
            with (self.ROOT / table).open() as handle:
                ids = {row["id"] for row in csv.DictReader(handle)}
            overlap = ids & set(scq.DECLARED_BOUNDARY_ENDPOINTS)
            self.assertFalse(overlap, f"{table} credits declared boundary endpoints: {overlap}")

    def test_guard_refuses_a_declared_boundary_credit_row(self):
        endpoint = sorted(scq.DECLARED_BOUNDARY_ENDPOINTS)[0]
        with self.assertRaises(Exception) as caught:
            scq.require_not_declared_boundary(endpoint)
        self.assertIn("can never be strict-credited", str(caught.exception))
        # A normal overlay id passes the same guard.
        scq.require_not_declared_boundary("OVL_0038_12B2")


class DriverInternalTest(unittest.TestCase):
    """A driver-internal body composes as ready but can never be credited (VP i19925)."""

    ROOT = Path(__file__).resolve().parents[2]
    SHA = "ab" * 32

    def table(self, folder, member="RES_08AE_00DC", body=None, doc_text=None):
        doc = Path(folder) / "docs/re/DRIVER_TEST.md"
        doc.parent.mkdir(parents=True, exist_ok=True)
        doc.write_text(doc_text if doc_text is not None else f"# {member}\nbody {self.SHA}\n")
        row = dict(id=member, evidence_doc="docs/re/DRIVER_TEST.md", review_pr="1", reviewed_commit="c" * 40,
                   body_sha256=body or self.SHA, body_bytes="159")
        return [row]

    def test_committed_table_loads_with_exactly_the_reviewed_members(self):
        # Pinned membership: adding a member is an acceptance act, so it must also update this set.
        path = self.ROOT / "docs/re/driver_internal_models.csv"
        with path.open() as handle:
            reader = csv.DictReader(handle)
            rows = list(reader)
            members = scq.load_driver_internal(rows, self.ROOT, fields=reader.fieldnames)
        self.assertEqual(tuple(reader.fieldnames), scq.DRIVER_INTERNAL_FIELDS)
        self.assertEqual(members, frozenset({"RES_08AE_181A", "RES_08AE_0000", "RES_08AE_1742", "RES_08AE_17AE", "RES_08AE_137C", "RES_08AE_13B1", "RES_08AE_006C", "RES_08AE_00DC", "RES_08AE_017B"}))  # #1243, #1249, #1250, #1253; accepted by par-e

    def test_valid_member_loads(self):
        with tempfile.TemporaryDirectory() as folder:
            self.assertEqual(scq.load_driver_internal(self.table(folder), folder), {"RES_08AE_00DC"})

    @staticmethod
    def image(*bodies):
        """A synthetic loader image holding each (08AE offset, body bytes) at its linear address."""
        memory = bytearray(0x8AE * 16 + 0x10000)
        for offset, body in bodies:
            memory[0x8AE * 16 + offset:0x8AE * 16 + offset + len(body)] = body
        return bytes(memory)

    def test_member_with_an_unready_callee_is_refused(self):
        # A member whose closure is not ready would read as ready and launder that callee (par-e, #1249 review).
        retf = b"\xcb"
        refused = {
            "0AF8:0001 is not runtime-covered": b"\x9a\x01\x00\xf8\x0a" + retf,   # LCALL to an uncovered endpoint
            "indirect near transfer": b"\xff\x17" + retf,                         # CALL WORD [BX]
            "indirect far transfer": b"\xff\x1f" + retf,                          # LCALL DWORD [BX]
            "AH not statically resolved": b"\xcd\x21" + retf,                      # INT 21h, AH unknown
            "INT 10h is not a declared environment service": b"\xb4\x00\xcd\x10" + retf,  # INT 10h
            "RES_08AE_0200 is not a member or strict": b"\xe8\x21\x01" + retf,    # CALL 0200 (a non-member)
            "no return found": b"\x90" * 16,
        }
        for message, body in refused.items():
            with self.subTest(message), tempfile.TemporaryDirectory() as folder:
                with self.assertRaises(Exception) as caught:
                    scq.load_driver_internal(self.table(folder), folder, image=self.image((0x00DC, body)))
                self.assertIn("incomplete closure", str(caught.exception))
                self.assertIn(message, str(caught.exception))

    def test_member_with_a_ready_closure_loads(self):
        with tempfile.TemporaryDirectory() as folder:
            covered = self.image((0x00DC, b"\x9a\x5b\x0a\xf8\x0a\xcb"))       # LCALL 0AF8:0A5B (LongMul), RETF
            self.assertEqual(scq.load_driver_internal(self.table(folder), folder, image=covered), {"RES_08AE_00DC"})
            strict = self.image((0x00DC, b"\xe8\x21\x01\xcb"))                # CALL 0200, a strict resident
            self.assertEqual(scq.load_driver_internal(self.table(folder), folder, image=strict,
                                                      strict_ids=frozenset({"RES_08AE_0200"})), {"RES_08AE_00DC"})
            rows = self.table(folder) + [dict(self.table(folder)[0], id="RES_08AE_0200")]
            Path(folder, "docs/re/DRIVER_TEST.md").write_text(f"RES_08AE_00DC RES_08AE_0200 {self.SHA}\n")
            pair = self.image((0x00DC, b"\xe8\x21\x01\xcb"), (0x0200, b"\xcb"))  # member calls member
            self.assertEqual(scq.load_driver_internal(rows, folder, image=pair), {"RES_08AE_00DC", "RES_08AE_0200"})

    def test_declared_dos_services_are_the_only_allowed_interrupts(self):
        # VP i20129: INT 21h is allowed only with AH set by a literal MOV in the same basic block, only for the
        # declared services, and only when the evidence document declares each service it uses.
        setvect = b"\xb8\x08\x25\xcd\x21\xcb"                                  # MOV AX,2508h; INT 21h; RETF
        with tempfile.TemporaryDirectory() as folder:
            declared = f"# RES_08AE_00DC\nbody {self.SHA}\nINT 21h AH=25h: DOS set interrupt vector\n"
            self.assertEqual(scq.load_driver_internal(self.table(folder, doc_text=declared), folder,
                                                      image=self.image((0x00DC, setvect))), {"RES_08AE_00DC"})
            with self.assertRaises(Exception) as caught:          # the same body, but the doc does not declare it
                scq.load_driver_internal(self.table(folder), folder, image=self.image((0x00DC, setvect)))
            self.assertIn("does not declare INT 21h AH=25h", str(caught.exception))
        refused = {
            "INT 21h AH=4Ch is not a declared environment service": b"\xb4\x4c\xcd\x21\xcb",      # MOV AH,4Ch
            "AH not statically resolved": b"\x8a\x27\xcd\x21\xcb",                              # MOV AH,[BX]
            "AH not statically resolved ": b"\xb4\x25\x74\x00\xcd\x21\xcb",                      # JE between
            "AH not statically resolved  ": b"\xb4\x25\xe8\x00\x00\xcd\x21\xcb",                 # CALL between
        }
        for message, body in refused.items():
            with self.subTest(message), tempfile.TemporaryDirectory() as folder:
                doc = f"# RES_08AE_00DC\nbody {self.SHA}\nINT 21h AH=25h INT 21h AH=4Ch\n"
                with self.assertRaises(Exception) as caught:
                    scq.load_driver_internal(self.table(folder, doc_text=doc), folder, image=self.image((0x00DC, body)))
                self.assertIn(message.strip(), str(caught.exception))

    def test_near_call_to_offset_zero_is_a_direct_call(self):
        # Capstone prints a zero target as "0", not "0x0"; that is a direct call, never "indirect" (006C -> 0000).
        body = b"\xe8\x21\xff\xcb"                                              # CALL 0000 from +00DC, RETF
        with tempfile.TemporaryDirectory() as folder:
            rows = self.table(folder) + [dict(self.table(folder)[0], id="RES_08AE_0000")]
            Path(folder, "docs/re/DRIVER_TEST.md").write_text(f"RES_08AE_00DC RES_08AE_0000 {self.SHA}\n")
            image = self.image((0x00DC, body), (0x0000, b"\xcb"))
            self.assertEqual(scq.load_driver_internal(rows, folder, image=image), {"RES_08AE_00DC", "RES_08AE_0000"})
            with self.assertRaises(Exception) as caught:
                scq.load_driver_internal(self.table(folder), folder, image=image)
            self.assertIn("RES_08AE_0000 is not a member or strict", str(caught.exception))
            self.assertNotIn("indirect", str(caught.exception))

    def test_committed_members_are_closure_complete_on_the_media(self):
        exe = self.ROOT / "resources/DOS/POOLRAD/START.EXE"
        if not exe.is_file():
            self.skipTest("owner media absent")
        from resident_loader_map import unpack
        image = bytes(unpack(exe.read_bytes())[0])
        path = self.ROOT / "docs/re/driver_internal_models.csv"
        rows = list(csv.DictReader(path.read_text().splitlines()))
        self.assertEqual(scq.load_driver_internal(rows, self.ROOT, image=image),
                         frozenset({"RES_08AE_181A", "RES_08AE_0000", "RES_08AE_1742", "RES_08AE_17AE", "RES_08AE_137C", "RES_08AE_13B1", "RES_08AE_006C", "RES_08AE_00DC", "RES_08AE_017B"}))
        # The real 006C calls 08AE:0000 twice (printed as "call 0"); with 0000 a member it has no gap at all.
        self.assertEqual(scq.driver_internal_closure_gaps("RES_08AE_006C", image,
                                                         {"RES_08AE_0000", "RES_08AE_006C"}), [])
        # The real 137C/13B1 use INT 21h AH=35h/25h, statically resolved: no gap, and exactly those services.
        for body, expected in (("RES_08AE_137C", {(0x21, 0x35), (0x21, 0x25)}), ("RES_08AE_13B1", {(0x21, 0x25)})):
            services = set()
            self.assertEqual(scq.driver_internal_closure_gaps(body, image, {body}, frozenset(), services), [])
            self.assertEqual(services, expected)
        # The real 0000 body calls Dos.GetTime: withdraw that coverage and the member must be refused.
        with mock.patch.dict(scq.RUNTIME_COVERED_ENDPOINTS):
            del scq.RUNTIME_COVERED_ENDPOINTS[(0x0A32, 0x00A2)]
            with self.assertRaises(Exception) as caught:
                scq.load_driver_internal(rows, self.ROOT, image=image)
            self.assertIn("RES_08AE_0000 has an incomplete closure", str(caught.exception))
            self.assertIn("0A32:00A2", str(caught.exception))

    def test_membership_is_narrow_and_fail_closed(self):
        cases = {
            "outside the 08AE driver segment": dict(member="RES_0899_0059"),
            "does not name": dict(doc_text="# nothing here\n"),
            "lacks body/approval provenance": dict(body="not-a-hash"),
        }
        for message, kwargs in cases.items():
            with self.subTest(message), tempfile.TemporaryDirectory() as folder:
                with self.assertRaises(Exception) as caught:
                    scq.load_driver_internal(self.table(folder, **kwargs), folder)
                self.assertIn(message.split(" (")[0], str(caught.exception))
        # Defence in depth inside the 08AE segment: a declared boundary or a
        # game-specific resident target is refused even with valid provenance.
        import resident_inventory
        with tempfile.TemporaryDirectory() as folder, \
                mock.patch.dict(scq.DECLARED_BOUNDARY_ENDPOINTS, {"RES_08AE_00DC": "docs/re/X.md"}):
            with self.assertRaises(Exception) as caught:
                scq.load_driver_internal(self.table(folder), folder)
            self.assertIn("declared boundary", str(caught.exception))
        with tempfile.TemporaryDirectory() as folder, \
                mock.patch.object(resident_inventory, "GAME_SPECIFIC_TARGETS",
                                  resident_inventory.GAME_SPECIFIC_TARGETS | {(0x8AE, 0x00DC)}):
            with self.assertRaises(Exception) as caught:
                scq.load_driver_internal(self.table(folder), folder)
            self.assertIn("game-specific resident target", str(caught.exception))
        with tempfile.TemporaryDirectory() as folder:
            rows = self.table(folder) * 2
            with self.assertRaises(Exception) as caught:
                scq.load_driver_internal(rows, folder)
            self.assertIn("duplicate", str(caught.exception))
            with self.assertRaises(Exception) as caught:
                scq.load_driver_internal(self.table(folder), folder, fields=("id", "evidence_doc"))
            self.assertIn("columns", str(caught.exception))

    def test_attempt_to_credit_a_member_is_refused_in_every_table(self):
        """The proof by attempt: write a member into each credit/manifest table."""
        members = frozenset({"RES_08AE_00DC"})
        attempts = {
            "strict_full_c_addresses.csv": dict(strict_rows=[{"id": "RES_08AE_00DC", "evidence_doc": "docs/re/X.md"}]),
            "reviewed_function_identities.csv": dict(identity_rows=[{"id": "RES_08AE_00DC"}]),
            "partial_callee_models.csv": dict(partial_rows=[{"id": "RES_08AE_00DC", "status": "full",
                                                             "evidence_doc": "docs/re/X.md"}]),
        }
        for table, kwargs in attempts.items():
            with self.subTest(table):
                arguments = dict(strict_rows=[], identity_rows=(), partial_rows=None)
                arguments.update(kwargs)
                with self.assertRaises(Exception) as caught:
                    scq.load_queue({}, [], [], arguments["strict_rows"], None, arguments["partial_rows"],
                                   members, arguments["identity_rows"])
                self.assertIn(f"can never appear in {table}", str(caught.exception))

    def test_no_member_appears_in_a_committed_credit_table(self):
        with (self.ROOT / "docs/re/driver_internal_models.csv").open() as handle:
            members = {row["id"] for row in csv.DictReader(handle)}
        for table in ("docs/re/strict_full_c_addresses.csv", "docs/re/reviewed_function_identities.csv",
                      "docs/re/partial_callee_models.csv"):
            with (self.ROOT / table).open() as handle:
                ids = {row["id"] for row in csv.DictReader(handle)}
            self.assertFalse(ids & members, f"{table} names driver_internal members: {ids & members}")


class ResidentRuntimeTest(unittest.TestCase):
    """A near-called resident runtime body composes as ready but can never be credited (VP i20291/i20300, t30)."""

    ROOT = Path(__file__).resolve().parents[2]
    SHA = "cd" * 32

    def table(self, folder, member="RES_0802_0020", body=None, doc_text=None):
        doc = Path(folder) / "docs/re/RUNTIME_TEST.md"
        doc.parent.mkdir(parents=True, exist_ok=True)
        doc.write_text(doc_text if doc_text is not None else f"# {member}\nbody {self.SHA}\n")
        return [dict(id=member, evidence_doc="docs/re/RUNTIME_TEST.md", review_pr="1", reviewed_commit="c" * 40,
                     body_sha256=body or self.SHA, body_bytes="32")]

    @staticmethod
    def image(*bodies):
        """A synthetic loader image holding each (segment, offset, body bytes) at its linear address."""
        memory = bytearray(1 << 20)
        for segment, offset, body in bodies:
            memory[segment * 16 + offset:segment * 16 + offset + len(body)] = body
        return bytes(memory)

    def load(self, rows, folder, **kwargs):
        kwargs.setdefault("ledger_ids", frozenset())
        return scq.load_resident_runtime(rows, folder, **kwargs)

    def test_committed_table_loads_with_exactly_the_reviewed_members(self):
        # Pinned membership: adding a member is an acceptance act, so it must also update this set.
        path = self.ROOT / "docs/re/resident_runtime_models.csv"
        with path.open() as handle:
            reader = csv.DictReader(handle)
            rows = list(reader)
            members = scq.load_resident_runtime(rows, self.ROOT, fields=reader.fieldnames)
        self.assertEqual(tuple(reader.fieldnames), scq.RESIDENT_RUNTIME_FIELDS)
        self.assertEqual(members, frozenset({"RES_0802_0175"}))  # 0175 joined after 0040's strict credit (par-e)

    def test_0175_closure_is_complete_once_0040_is_strict(self):
        """t30 side (ii): 0175's closure is 0040. It was refused on the real image until 0040 was strict; 0040's
        acceptance flips this pin (par-e), so 0175's membership row can follow as its own acceptance."""
        exe = self.ROOT / "resources/DOS/POOLRAD/START.EXE"
        if not exe.is_file():
            self.skipTest("owner media absent")
        from resident_loader_map import unpack
        image = bytes(unpack(exe.read_bytes())[0])
        strict = {r["id"] for r in csv.DictReader((self.ROOT / "docs/re/strict_full_c_addresses.csv").open())}
        doc = (self.ROOT / "docs/re/RES_0802_0175_CPU.md").read_text()
        sha = re.search(r"`([0-9a-f]{64})`", doc).group(1)
        rows = [dict(id="RES_0802_0175", evidence_doc="docs/re/RES_0802_0175_CPU.md", review_pr="1",
                     reviewed_commit="c" * 40, body_sha256=sha, body_bytes="30")]
        self.assertIn("RES_0802_0040", strict)
        self.assertEqual(self.load(rows, self.ROOT, image=image, strict_ids=strict), {"RES_0802_0175"})

    def test_valid_member_loads(self):
        with tempfile.TemporaryDirectory() as folder:
            self.assertEqual(self.load(self.table(folder), folder), {"RES_0802_0020"})

    def test_membership_is_narrow_and_fail_closed(self):
        cases = {
            "is outside the class (08AE bodies are driver_internal)": dict(member="RES_08AE_00DC"),
            "is outside the class": dict(member="OVL_0025_0341"),
            "does not name": dict(doc_text="# nothing here\n"),
            "lacks body/approval provenance": dict(body="not-a-hash"),
        }
        for message, kwargs in cases.items():
            with self.subTest(message), tempfile.TemporaryDirectory() as folder:
                with self.assertRaises(Exception) as caught:
                    self.load(self.table(folder, **kwargs), folder)
                self.assertIn(message, str(caught.exception))
        # Near-called only: an overlay-called entry must earn identity, never be declared non-credit.
        with tempfile.TemporaryDirectory() as folder:
            with self.assertRaises(Exception) as caught:
                self.load(self.table(folder), folder, ledger_ids=frozenset({"RES_0802_0020"}))
            self.assertIn("overlay-called entry", str(caught.exception))
        # The committed ledger is the default source: its real overlay-called entry 0802:0425 (unresolved) is refused.
        # (0802:0716 served here until its strict credit (par-e); it is now refused earlier, as a game-specific target.)
        with tempfile.TemporaryDirectory() as folder:
            with self.assertRaises(Exception) as caught:
                scq.load_resident_runtime(self.table(folder, member="RES_0802_0425"), folder,
                                          ledger_ids=scq.resident_ledger_ids(self.ROOT))
            self.assertIn("overlay-called entry", str(caught.exception))
        with tempfile.TemporaryDirectory() as folder:
            with self.assertRaises(Exception) as caught:
                scq.load_resident_runtime(self.table(folder, member="RES_0802_0716"), folder,
                                          ledger_ids=scq.resident_ledger_ids(self.ROOT))
            self.assertIn("game-specific resident target", str(caught.exception))
        import resident_inventory
        with tempfile.TemporaryDirectory() as folder, \
                mock.patch.dict(scq.DECLARED_BOUNDARY_ENDPOINTS, {"RES_0802_0020": "docs/re/X.md"}):
            with self.assertRaises(Exception) as caught:
                self.load(self.table(folder), folder)
            self.assertIn("declared boundary", str(caught.exception))
        with tempfile.TemporaryDirectory() as folder, \
                mock.patch.object(resident_inventory, "GAME_SPECIFIC_TARGETS",
                                  resident_inventory.GAME_SPECIFIC_TARGETS | {(0x802, 0x0020)}):
            with self.assertRaises(Exception) as caught:
                self.load(self.table(folder), folder)
            self.assertIn("game-specific resident target", str(caught.exception))
        with tempfile.TemporaryDirectory() as folder:
            with self.assertRaises(Exception) as caught:
                self.load(self.table(folder) * 2, folder)
            self.assertIn("duplicate resident_runtime member", str(caught.exception))
            with self.assertRaises(Exception) as caught:
                self.load(self.table(folder), folder, fields=("id", "evidence_doc"))
            self.assertIn("columns", str(caught.exception))

    def test_closure_must_be_complete(self):
        getmem = b"\x9a\x3f\x02\xf8\x0a\xcb"                     # LCALL 0AF8:023F (GetMem, runtime-covered); RETF
        with tempfile.TemporaryDirectory() as folder:
            self.assertEqual(self.load(self.table(folder), folder, image=self.image((0x802, 0x0020, getmem))),
                             {"RES_0802_0020"})
        refused = {
            "0AF8:0567 is not runtime-covered": b"\x9a\x67\x05\xf8\x0a\xcb",   # LCALL an uncovered endpoint
            "RES_0802_0040 is not a member or strict": b"\xe8\x1d\x00\xcb",     # CALL 0040, a non-member
        }
        for message, body in refused.items():
            with self.subTest(message), tempfile.TemporaryDirectory() as folder:
                with self.assertRaises(Exception) as caught:
                    self.load(self.table(folder), folder, image=self.image((0x802, 0x0020, body)))
                self.assertIn("resident_runtime member RES_0802_0020 has an incomplete closure", str(caught.exception))
                self.assertIn(message, str(caught.exception))
        # A callee that is a member of the OTHER never-credit class is ready (the classes compose).
        with tempfile.TemporaryDirectory() as folder:
            far_driver = b"\x9a\xdc\x00\xae\x08\xcb"                   # LCALL 08AE:00DC
            self.assertEqual(self.load(self.table(folder), folder, image=self.image((0x802, 0x0020, far_driver)),
                                       driver_internal=frozenset({"RES_08AE_00DC"})), {"RES_0802_0020"})

    def test_attempt_to_credit_a_member_is_refused_in_every_table(self):
        members = frozenset({"RES_0802_0020"})
        attempts = {
            "strict_full_c_addresses.csv": dict(strict_rows=[{"id": "RES_0802_0020", "evidence_doc": "docs/re/X.md"}]),
            "reviewed_function_identities.csv": dict(identity_rows=[{"id": "RES_0802_0020"}]),
            "partial_callee_models.csv": dict(partial_rows=[{"id": "RES_0802_0020", "status": "full",
                                                             "evidence_doc": "docs/re/X.md"}]),
        }
        for table, kwargs in attempts.items():
            with self.subTest(table):
                arguments = dict(strict_rows=[], identity_rows=(), partial_rows=None)
                arguments.update(kwargs)
                with self.assertRaises(Exception) as caught:
                    scq.load_queue({}, [], [], arguments["strict_rows"], None, arguments["partial_rows"],
                                   frozenset(), arguments["identity_rows"], members)
                self.assertIn(f"resident_runtime members can never appear in {table}", str(caught.exception))

    def test_member_reads_as_ready_and_is_labelled(self):
        edges = {(0x25, 0x0341): [dict(kind="far_fixup_confirmed", resolution="resident", seg=0x802, off=0x0020)]}
        args = (edges, set(), {}, {}, set(), set(), set(), set(), set())
        before = scq.callee_evidence((0x25, 0x0341), *args)
        self.assertEqual([t["readiness"] for t in before["targets"]], ["resident_unmapped"])
        self.assertEqual((before["ready"], before["unready"]), (0, 1))
        after = scq.callee_evidence((0x25, 0x0341), *args, resident_runtime=frozenset({"RES_0802_0020"}))
        self.assertEqual([t["readiness"] for t in after["targets"]], ["resident_runtime"])
        self.assertEqual((after["ready"], after["unready"]), (1, 0))
        self.assertTrue(scq.resident_target_ready("RES_0802_0020", set(), resident_runtime=frozenset({"RES_0802_0020"})))
        self.assertFalse(scq.resident_target_ready("RES_0802_0020", set()))

    def test_no_member_appears_in_a_committed_credit_table(self):
        with (self.ROOT / "docs/re/resident_runtime_models.csv").open() as handle:
            members = {row["id"] for row in csv.DictReader(handle)}
        for table in ("docs/re/strict_full_c_addresses.csv", "docs/re/reviewed_function_identities.csv",
                      "docs/re/partial_callee_models.csv"):
            with (self.ROOT / table).open() as handle:
                ids = {row["id"] for row in csv.DictReader(handle)}
            self.assertFalse(ids & members, f"{table} names resident_runtime members: {ids & members}")


class StrictCandidateQueueTest(unittest.TestCase):
    def test_reviewed_tail_is_excluded_before_feature_decoding(self):
        # The decoder deliberately treats tail bytes as ES/global accesses.
        # Checking only the final byte count would leave these false features.
        code = b"AAAAzzzzBBBB"
        decoded = []

        class Decoder:
            def disasm(self, data, start):
                decoded.append((start, data))
                return [SimpleNamespace(address=start + i, size=1,
                                        mnemonic="mov" if byte == ord("z") else "nop",
                                        op_str="es:[0xabcd]" if byte == ord("z") else "")
                        for i, byte in enumerate(data)]

        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            inputs = dict(game_ovr_sha256=hashlib.sha256(code).hexdigest(),
                          start_exe_sha256=hashlib.sha256(b"synthetic exe").hexdigest())
            inventory = dict(inputs=inputs, units=[dict(
                stub_para=0x19, memory_segment=0x1C, ovr_offset=0,
                code_size=len(code), entry_count=2, entries=[0, 8],
                functions=[dict(rel=0), dict(rel=8)], fixups=[])])
            (root / "inventory.json").write_text(json.dumps(inventory))
            (root / "graph.json").write_text(json.dumps(dict(inputs=inputs, edges=[])))
            (root / "overlay.bin").write_bytes(code)
            argv = ["function_ledger.py", "--inventory", str(root / "inventory.json"),
                    "--callgraph", str(root / "graph.json"),
                    "--ovr", str(root / "overlay.bin"),
                    "--csv", str(root / "ledger.csv"), "--md", str(root / "ledger.md")]
            pin = {(0x19, 0): (4, hashlib.sha256(b"AAAA").hexdigest())}
            with patch.object(sys, "argv", argv), \
                    patch.object(ledger_tool, "Cs", return_value=Decoder()), \
                    patch.object(ledger_tool, "CS_ARCH_X86", 0, create=True), \
                    patch.object(ledger_tool, "CS_MODE_16", 0, create=True), \
                    patch.dict(ledger_tool.EXECUTABLE_SPANS, pin, clear=True), \
                    contextlib.redirect_stdout(io.StringIO()), warnings.catch_warnings():
                # The legacy CLI uses short-lived open(...).read() handles.
                warnings.simplefilter("ignore", ResourceWarning)
                ledger_tool.main()
            with (root / "ledger.csv").open() as source:
                rows = list(csv.DictReader(source))
        self.assertEqual(decoded, [(0, b"AAAA"), (8, b"BBBB")])
        self.assertEqual(len(rows), 2)
        for row in rows:
            self.assertEqual((row["bound"], row["decoded"], row["insns"]), ("4", "4", "4"))
            self.assertEqual((row["ds_globals"], row["es_refs"], row["cls"]), ("0", "0", "leaf"))

    def test_reviewed_executable_span_refuses_mismatched_bytes_or_bounds(self):
        code = b"padding" + b"body!" + b"tail after return"
        pin = {(0x19, 7): (12, hashlib.sha256(b"body!").hexdigest())}
        with patch.dict(ledger_tool.EXECUTABLE_SPANS, pin, clear=True):
            self.assertEqual(ledger_tool.executable_end(0x19, 7, len(code), code), 12)
            self.assertEqual(ledger_tool.executable_end(0x19, 7, 12, code), 12)
            with self.assertRaisesRegex(ValueError, "hash differs"):
                ledger_tool.executable_end(0x19, 7, len(code), code[:8] + b"?" + code[9:])
            with self.assertRaisesRegex(ValueError, "outside candidate bounds"):
                ledger_tool.executable_end(0x19, 7, 11, code)
            # Unreviewed routines retain their full proposed span: an early
            # return alone does not prove that the remaining bytes are DATA.
            self.assertEqual(ledger_tool.executable_end(0x19, 8, len(code), code), len(code))

    def test_unresolved_indirect_dispatch_blocks_self_and_modeled_callers(self):
        inventory, ledger, classifications = fixture()
        state = scq.load_queue(inventory, ledger, classifications, [], None)
        mentions = {cid: {"source": [], "tooling": [], "docs": []}
                    for cid in state["candidates"].values()}
        main_files = {cid: ["synthetic.c"] for cid in mentions}
        dispatch = {"OVL_0019_0010": dict(site=0x16, note="synthetic unresolved handler",
                                          evidence_doc="synthetic.md")}
        with patch.dict(scq.UNRESOLVED_INDIRECT_DISPATCH, dispatch, clear=True):
            # Even a stale 'full' model label cannot erase an explicit live
            # indirect boundary. The callgraph has no outgoing edge for 0010.
            state["full_models"]["OVL_0019_0010"] = "synthetic.md"
            result = scq.build_queue(state, callgraph(), inventory["units"], ledger,
                                     classifications, mentions, main_files, {}, {})
        rows = {r["id"]: r for r in result["rows"]}
        direct = rows["OVL_0019_0010"]["callee_readiness"]
        self.assertEqual(direct["total"], 1)
        self.assertEqual(direct["unready"], 1)
        self.assertEqual(direct["targets"][0]["kind"], "indirect")
        self.assertIsNone(direct["targets"][0]["offset"])
        self.assertEqual(direct["targets"][0]["site"], 0x16)
        self.assertEqual(rows["OVL_0019_0020"]["callee_readiness"]["counts"],
                         {"indirect_unresolved": 1})
        self.assertEqual(rows["OVL_0019_0030"]["callee_readiness"]["counts"],
                         {"modeled_blocked": 1})
        self.assertTrue(all(not r["callee_readiness"]["composable"] for r in rows.values()))
        self.assertEqual(result["schema_version"], 9)

    def test_string_compare_runtime_coverage_keeps_overlay_interface_caveat(self):
        note = scq.RAW_CPU_INTERFACE_CAVEATS["OVL_0025_1815"]
        self.assertIn("not a CPU-state callee", note)
        self.assertIn("pr_system_strcmp_0c10", note)
        self.assertNotIn("unmodeled runtime", note)
        self.assertIn((0x0AF8, 0x0C10), scq.RUNTIME_COVERED_ENDPOINTS)

    def test_ranking_and_prior_detection(self):
        result = queue()
        rows = {row["id"]: row for row in result["rows"]}
        self.assertEqual([row["id"] for row in result["rows"]],
                         ["OVL_0019_0020", "OVL_0019_0030"])
        self.assertEqual(result["summary"]["strict_overlay_excluded"], 1)
        self.assertNotIn("OVL_0019_0010", rows)
        untranslated, prior = rows["OVL_0019_0020"], rows["OVL_0019_0030"]
        self.assertEqual(untranslated["prior_translation"]["status"], "unmerged_branch")
        self.assertEqual(untranslated["callee_readiness"]["counts"], {"strict": 1})
        self.assertTrue(untranslated["callee_readiness"]["composable"])
        self.assertEqual(untranslated["score_terms"]["composable"], 6)
        self.assertEqual(untranslated["callers_aligned_push_cs"], 1)
        self.assertGreater(untranslated["score"], prior["score"])
        self.assertEqual(prior["prior_translation"]["status"], "inline_in_strict")
        self.assertEqual(prior["prior_translation"]["inline"][0]["covered_by"], "OVL_0019_0010")
        self.assertEqual(prior["callee_readiness"]["counts"], {"untranslated": 1})
        self.assertFalse(prior["callee_readiness"]["composable"])
        self.assertEqual(prior["score_terms"]["composable"], 0)
        self.assertEqual(result["summary"]["composable_bodies"], 1)

    def test_partial_callees_are_unready(self):
        inventory, ledger, classifications = fixture()
        state = scq.load_queue(inventory, ledger, classifications, [], None,
                               [dict(id="OVL_0019_0010", status="partial",
                                     evidence_doc="docs/re/synthetic.md")])
        mentions, main_files, branches = evidence()
        mentions = {**mentions, "OVL_0019_0010": {"source": [], "tooling": [], "docs": []}}
        result: dict = scq.build_queue(state, callgraph(), inventory["units"], ledger,
                                       classifications, mentions, main_files, branches, {},
                                       dict(ovr=bytes(200)))
        row = next(r for r in result["rows"] if r["id"] == "OVL_0019_0020")
        self.assertEqual(row["callee_readiness"]["counts"], {"partial": 1})
        self.assertFalse(row["callee_readiness"]["composable"])
        self.assertEqual(row["callee_readiness"]["unready"], 1)

    def test_raw_cpu_interface_caveats_are_unready(self):
        inventory, ledger, classifications = fixture()
        state = scq.load_queue(inventory, ledger, classifications,
                               [dict(id="OVL_0019_0010", evidence_doc="docs/re/synthetic.md")], None)
        mentions = {cid: {"source": [], "tooling": [], "docs": []}
                    for cid in state["candidates"].values()}
        mentions["OVL_0019_0010"] = {"source": [], "tooling": [], "docs": []}
        saved = dict(scq.RAW_CPU_INTERFACE_CAVEATS)
        try:
            scq.RAW_CPU_INTERFACE_CAVEATS["OVL_0019_0010"] = "synthetic scalar-only endpoint"
            result: dict = scq.build_queue(state, callgraph(), inventory["units"], ledger,
                                           classifications, mentions, {}, {}, {},
                                           dict(ovr=bytes(200)))
            row = next(r for r in result["rows"] if r["id"] == "OVL_0019_0020")
            self.assertEqual(row["callee_readiness"]["counts"], {"interface_caveat": 1})
            self.assertFalse(row["callee_readiness"]["composable"])
            self.assertEqual(row["score_terms"]["composable"], 0)
            self.assertEqual(row["callee_readiness"]["raw_cpu_caveats"],
                             [{"id": "OVL_0019_0010", "note": "synthetic scalar-only endpoint"}])
            self.assertEqual(result["summary"]["interface_caveat_bodies"], 1)
            graph = dict(edges=[
                dict(kind="far_fixup_confirmed", unit=0x19, site=0x45, src_func=0x20,
                     seg=0x00BA, off=0x2E8B, resolution="resident")])
            strict_resident = [dict(id="RES_00BA_2E8B", evidence_doc="docs/re/synthetic.md")]
            state2 = scq.load_queue(inventory, ledger, classifications, strict_resident,
                                    None, [dict(id="RES_00BA_2E8B", status="full",
                                                evidence_doc="docs/re/synthetic.md")])
            scq.RAW_CPU_INTERFACE_CAVEATS["RES_00BA_2E8B"] = "synthetic callback endpoint"
            result2: dict = scq.build_queue(state2, graph, inventory["units"], ledger,
                                            classifications, mentions, {}, {}, {},
                                            dict(ovr=bytes(200)))
            row2 = next(r for r in result2["rows"] if r["id"] == "OVL_0019_0020")
            self.assertEqual(row2["callee_readiness"]["counts"],
                             {"resident_interface_caveat": 1})
            self.assertFalse(row2["callee_readiness"]["composable"])
        finally:
            scq.RAW_CPU_INTERFACE_CAVEATS.clear()
            scq.RAW_CPU_INTERFACE_CAVEATS.update(saved)

    def test_covered_runtime_leaves_are_ready(self):
        inventory, ledger, classifications = fixture()
        state = scq.load_queue(inventory, ledger, classifications, [], None)
        mentions = {cid: {"source": [], "tooling": [], "docs": []}
                    for cid in state["candidates"].values()}
        covered = dict(edges=[dict(kind="far_fixup_confirmed", unit=0x19, site=0x45,
                                   src_func=0x20, seg=0x0AF8, off=0x16A8,
                                   resolution="runtime")])
        result: dict = scq.build_queue(state, covered, inventory["units"], ledger,
                                       classifications, mentions, {}, {}, {},
                                       dict(ovr=bytes(200)))
        row = next(r for r in result["rows"] if r["id"] == "OVL_0019_0020")
        self.assertEqual(row["callee_readiness"]["counts"], {"runtime_covered": 1})
        self.assertTrue(row["callee_readiness"]["composable"])
        uncovered = dict(edges=[dict(kind="far_fixup_confirmed", unit=0x19, site=0x45,
                                     src_func=0x20, seg=0x0AF8, off=0x0999,
                                     resolution="runtime")])
        result2: dict = scq.build_queue(state, uncovered, inventory["units"], ledger,
                                        classifications, mentions, {}, {}, {},
                                        dict(ovr=bytes(200)))
        row2 = next(r for r in result2["rows"] if r["id"] == "OVL_0019_0020")
        self.assertEqual(row2["callee_readiness"]["counts"], {"runtime_unmapped": 1})
        self.assertFalse(row2["callee_readiness"]["composable"])

    def test_every_runtime_coverage_row_names_a_model_in_the_tree(self):
        """A coverage row is only as good as the model it names: the label's
        model must exist under src/recovered (a test-target name is not a model)."""
        root = Path(__file__).resolve().parents[2]
        for endpoint, label in scq.RUNTIME_COVERED_ENDPOINTS.items():
            model = label.rsplit("(", 1)[1].split(",")[0].rstrip(")")
            self.assertTrue((root / "src" / "recovered" / f"{model}.c").is_file(),
                            f"{endpoint}: {model}.c")
        self.assertIn((0x0AF8, 0x13B1), scq.RUNTIME_COVERED_ENDPOINTS)

    def test_covered_endpoint_cannot_land_before_its_terminal(self):
        """04A9's RUNERROR stop transfers into 00E2: covering 04A9 while 00E2 is
        undeclared would let every caller compose into an undeclared terminal."""
        inventory, ledger, classifications = fixture()
        self.assertEqual(scq.RUNTIME_COVERED_TERMINALS, {
            (0x0AF8, 0x023F): "RES_0AF8_00E2", (0x0AF8, 0x0254): "RES_0AF8_00E2",
            (0x0AF8, 0x04A9): "RES_0AF8_00E2", (0x0AF8, 0x0A76): "RES_0AF8_00E2",
            (0x0AF8, 0x1186): "RES_0AF8_00E2", (0x0AF8, 0x119E): "RES_0AF8_00E2",
            (0x0AF8, 0x1593): "RES_0AF8_00E2", (0x0AF8, 0x14FB): "RES_0AF8_00DF"})
        # 14FB is covered only because RES_0AF8_00DF is declared; withdrawing 00DF must be refused.
        self.assertIn((0x0AF8, 0x14FB), scq.RUNTIME_COVERED_ENDPOINTS)
        no_00df = {k: v for k, v in scq.DECLARED_BOUNDARY_ENDPOINTS.items() if k != "RES_0AF8_00DF"}
        with mock.patch.dict(scq.DECLARED_BOUNDARY_ENDPOINTS, no_00df, clear=True):
            with self.assertRaises(ValueError) as held:
                scq.load_queue(inventory, ledger, classifications, [], None)
        self.assertIn("RES_0AF8_00DF", str(held.exception))
        # Reset's CLOSE stop: 1410 is usable only while Close 1491 is covered.
        self.assertEqual(scq.RUNTIME_COVERED_STOP_CALLEES, {(0x0AF8, 0x1410): (0x0AF8, 0x1491)})
        without_close = {k: v for k, v in scq.RUNTIME_COVERED_ENDPOINTS.items() if k != (0x0AF8, 0x1491)}
        with mock.patch.dict(scq.RUNTIME_COVERED_ENDPOINTS, without_close, clear=True):
            with self.assertRaises(ValueError) as stop:
                scq.load_queue(inventory, ledger, classifications, [], None)
        self.assertIn("0AF8:1491", str(stop.exception))
        scq.load_queue(inventory, ledger, classifications, [], None)
        undeclared = {k: v for k, v in scq.DECLARED_BOUNDARY_ENDPOINTS.items() if k != "RES_0AF8_00E2"}
        with mock.patch.dict(scq.DECLARED_BOUNDARY_ENDPOINTS, undeclared, clear=True):
            with self.assertRaises(ValueError) as caught:
                scq.load_queue(inventory, ledger, classifications, [], None)
        self.assertIn("RES_0AF8_00E2", str(caught.exception))

    def test_declared_boundary_applies_to_runtime_and_transitive_targets(self):
        """Every readiness pass consults every product: a declared boundary is
        ready whether the graph resolves it as runtime or resident, both at the
        direct call and through a modeled callee (0AF8:16BC read runtime_unmapped)."""
        inventory, ledger, classifications = fixture()
        state = scq.load_queue(inventory, ledger, classifications, [], None)
        mentions = {cid: {"source": [], "tooling": [], "docs": []}
                    for cid in state["candidates"].values()}
        main_files = {"OVL_0019_0030": ["src/recovered/overlay_u19_gamma_0030.c"]}

        def build(boundary_edge):
            graph = dict(edges=[
                dict(kind="near_candidate", unit=0x19, site=0x25, src_func=0x20, target=0x30,
                     target_func=0x30, exact_prologue=True),
                dict(boundary_edge, kind="far_fixup_confirmed", unit=0x19, site=0x35,
                     src_func=0x30)])
            result = scq.build_queue(state, graph, inventory["units"], ledger, classifications,
                                     mentions, main_files, {}, {}, dict(ovr=bytes(200)))
            rows = {row["id"]: row["callee_readiness"] for row in result["rows"]}
            return rows["OVL_0019_0030"], rows["OVL_0019_0020"]

        for edge in (dict(seg=0x0AF8, off=0x16BC, resolution="runtime"),
                     dict(seg=0x0A40, off=0x02FD, resolution="resident")):
            direct, caller = build(edge)
            self.assertEqual(direct["counts"], {"declared_boundary": 1}, edge)
            self.assertTrue(direct["composable"], edge)
            self.assertEqual(caller["counts"], {"modeled": 1}, edge)
            self.assertTrue(caller["composable"], edge)
        direct, caller = build(dict(seg=0x0AF8, off=0x16BD, resolution="runtime"))
        self.assertEqual(direct["counts"], {"runtime_unmapped": 1})
        self.assertEqual(caller["counts"], {"modeled_blocked": 1})
        self.assertEqual(scq.runtime_readiness(0x0AF8, 0x00E9), "declared_boundary")
        self.assertEqual(scq.runtime_readiness(0x0AF8, 0x0254), "runtime_covered")

    def test_closure_chain_and_cycle(self):
        inventory, ledger, classifications = fixture()
        state = scq.load_queue(inventory, ledger, classifications, [], None)
        mentions = {cid: {"source": [], "tooling": [], "docs": []}
                    for cid in state["candidates"].values()}
        result: dict = scq.build_queue(
            state, callgraph(), inventory["units"], ledger, classifications, mentions,
            {"OVL_0019_0020": ["src/recovered/overlay_u19_x_0020.c"]}, {}, {},
            dict(ovr=bytes(200)))
        rows = {row["id"]: row for row in result["rows"]}
        self.assertEqual(rows["OVL_0019_0020"]["callee_readiness"]["counts"], {"verified": 1})
        self.assertFalse(rows["OVL_0019_0020"]["callee_readiness"]["composable"])
        self.assertEqual(rows["OVL_0019_0030"]["callee_readiness"]["counts"],
                         {"modeled_blocked": 1})
        cycle = dict(edges=[
            dict(kind="near_candidate", unit=0x19, site=0x1A, src_func=0x20, target=0x10,
                 target_func=0x10, exact_prologue=True),
            dict(kind="near_candidate", unit=0x19, site=0x35, src_func=0x30, target=0x20,
                 target_func=0x20, exact_prologue=True),
            dict(kind="near_candidate", unit=0x19, site=0x40, src_func=0x10, target=0x20,
                 target_func=0x20, exact_prologue=True)])
        result2: dict = scq.build_queue(
            state, cycle, inventory["units"], ledger, classifications, mentions,
            {"OVL_0019_0010": ["src/recovered/overlay_u19_x_0010.c"],
             "OVL_0019_0020": ["src/recovered/overlay_u19_x_0020.c"]}, {}, {},
            dict(ovr=bytes(200)))
        rows2 = {row["id"]: row for row in result2["rows"]}
        self.assertTrue(rows2["OVL_0019_0020"]["callee_readiness"]["composable"])
        self.assertEqual(rows2["OVL_0019_0030"]["callee_readiness"]["counts"], {"modeled": 1})

    def test_resident_callee_requires_strict_credit(self):
        # Isolate from any real caveat on this synthetic resident id.
        _saved = dict(scq.RAW_CPU_INTERFACE_CAVEATS)
        scq.RAW_CPU_INTERFACE_CAVEATS.pop("RES_00BA_2E8B", None)
        self.addCleanup(lambda: (scq.RAW_CPU_INTERFACE_CAVEATS.clear(),
                                 scq.RAW_CPU_INTERFACE_CAVEATS.update(_saved)))
        inventory, ledger, classifications = fixture()
        state = scq.load_queue(inventory, ledger, classifications, [], None)
        mentions = {cid: {"source": [], "tooling": [], "docs": []}
                    for cid in state["candidates"].values()}
        graph = dict(edges=[
            dict(kind="near_candidate", unit=0x19, site=0x35, src_func=0x30, target=0x20,
                 target_func=0x20, exact_prologue=True),
            dict(kind="far_fixup_confirmed", unit=0x19, site=0x45, src_func=0x20, seg=0x00BA,
                 off=0x2E8B, resolution="resident")])
        main_files = {"OVL_0019_0020": ["src/recovered/overlay_u19_x_0020.c"]}
        result: dict = scq.build_queue(state, graph, inventory["units"], ledger,
                                       classifications, mentions, main_files, {}, {},
                                       dict(ovr=bytes(200)))
        rows = {row["id"]: row for row in result["rows"]}
        self.assertEqual(rows["OVL_0019_0020"]["callee_readiness"]["counts"],
                         {"resident_unmapped": 1})
        self.assertEqual(rows["OVL_0019_0030"]["callee_readiness"]["counts"],
                         {"modeled_blocked": 1})
        strict_resident = [dict(id="RES_00BA_2E8B",
                                evidence_doc="docs/re/synthetic.md")]
        state2 = scq.load_queue(inventory, ledger, classifications,
                                strict_resident, None)
        result2: dict = scq.build_queue(state2, graph, inventory["units"], ledger,
                                        classifications, mentions, main_files, {}, {},
                                        dict(ovr=bytes(200)))
        rows2 = {row["id"]: row for row in result2["rows"]}
        self.assertEqual(rows2["OVL_0019_0020"]["callee_readiness"]["counts"],
                         {"resident_unmapped": 1})
        state3 = scq.load_queue(inventory, ledger, classifications, strict_resident,
                                None, [dict(id="RES_00BA_2E8B", status="full",
                                            evidence_doc="docs/re/synthetic.md")])
        result3: dict = scq.build_queue(state3, graph, inventory["units"], ledger,
                                        classifications, mentions, main_files, {}, {},
                                        dict(ovr=bytes(200)))
        rows3 = {row["id"]: row for row in result3["rows"]}
        self.assertEqual(rows3["OVL_0019_0020"]["callee_readiness"]["counts"],
                         {"resident_covered": 1})
        self.assertTrue(rows3["OVL_0019_0020"]["callee_readiness"]["composable"])
        self.assertEqual(rows3["OVL_0019_0030"]["callee_readiness"]["counts"], {"modeled": 1})
        state4 = scq.load_queue(inventory, ledger, classifications, strict_resident,
                                None, [dict(id="RES_00BA_2E8B", status="callback",
                                            evidence_doc="docs/re/synthetic.md")])
        result4: dict = scq.build_queue(state4, graph, inventory["units"], ledger,
                                        classifications, mentions, main_files, {}, {},
                                        dict(ovr=bytes(200)))
        rows4 = {row["id"]: row for row in result4["rows"]}
        self.assertEqual(rows4["OVL_0019_0020"]["callee_readiness"]["counts"],
                         {"resident_callback": 1})
        self.assertFalse(rows4["OVL_0019_0020"]["callee_readiness"]["composable"])
        self.assertEqual(rows4["OVL_0019_0030"]["callee_readiness"]["counts"],
                         {"modeled_blocked": 1})

    def test_merged_raw_dependency_updates_readiness(self):
        """A merged raw implementation must land in full_models so callers see a
        ready 'modeled' callee instead of staying modeled_blocked."""
        inventory, ledger, classifications = fixture()
        without = scq.load_queue(inventory, ledger, classifications, [], None)
        self.assertNotIn("OVL_0019_0020", without["full_models"])
        manifest = [dict(id="OVL_0019_0020", status="full",
                         evidence_doc="docs/re/synthetic.md")]
        with_manifest = scq.load_queue(inventory, ledger, classifications, [],
                                       None, manifest)
        self.assertIn("OVL_0019_0020", with_manifest["full_models"])
        self.assertEqual(with_manifest["full_models"]["OVL_0019_0020"],
                         "docs/re/synthetic.md")
        partial = scq.load_queue(inventory, ledger, classifications, [], None,
                                 [dict(id="OVL_0019_0020", status="partial",
                                       evidence_doc="docs/re/synthetic.md")])
        self.assertIn("OVL_0019_0020", partial["partial_models"])
        self.assertNotIn("OVL_0019_0020", partial["full_models"])

    def test_phantom_near_candidates_are_ignored(self):
        inventory, ledger, classifications = fixture()
        state = scq.load_queue(inventory, ledger, classifications, [], None)
        mentions = {cid: {"source": [], "tooling": [], "docs": []}
                    for cid in state["candidates"].values()}
        phantom = dict(edges=[dict(kind="near_candidate", unit=0x19, site=0x44, src_func=0x30,
                                   target=0x11, target_func=0x10, exact_prologue=False)])
        result: dict = scq.build_queue(state, phantom, inventory["units"], ledger,
                                       classifications, mentions, {}, {}, {},
                                       dict(ovr=bytes(200)))
        row = next(r for r in result["rows"] if r["id"] == "OVL_0019_0030")
        self.assertEqual(row["callee_readiness"]["counts"], {})
        self.assertTrue(row["callee_readiness"]["composable"])

    def test_non_model_mentions_are_not_inline_evidence(self):
        """A call site, a span hash, a next-prologue comment or a previous-body
        end assertion mentions an address without modelling it."""
        known = {"OVL_007D_03A3": (0x7D, 0x03A3)}
        main_files = {"OVL_007D_03A3": ["src/recovered/overlay_u7d_update_03a3.c"]}
        shapes = [
            'assert ins[-1].address + ins[-1].size == 0x047A',
            "0x0829: 0x047A,",
            '(0x047A, 0x0565): "0c687ca966432b6c241673264978796173f6f31e91905a8a67ee3dee5110bc4e",',
            "* 67 instructions, `retf 4`, no trailing data: the next prologue is +0x047A)",
        ]
        for text in shapes:
            self.assertTrue(scq.is_boundary_mention({"text": text}), text)
            flags = scq.prior_evidence(
                "OVL_007D_047A", main_files,
                {"source": [{"file": "tests/recovered/check_overlay_u7d_03a3_natural_route.py",
                             "text": text}], "tooling": [], "docs": []},
                [], [], {"OVL_007D_03A3"}, known)
            self.assertNotEqual(flags["status"], "inline_in_strict", text)
        # positive control: a real inline mention still counts
        real = {"file": "tests/recovered/check_overlay_u7d_03a3_natural_route.py",
                "text": "call OVL_007D_03A3 entry at 0x03A3"}
        self.assertFalse(scq.is_boundary_mention(real))
        flags = scq.prior_evidence(
            "OVL_007D_047A", main_files,
            {"source": [real], "tooling": [], "docs": []},
            [], [], {"OVL_007D_03A3"}, known)
        self.assertEqual(flags["status"], "inline_in_strict")

    def test_range_comment_is_not_inline_evidence(self):
        """A comment naming this routine only as the END of a neighbour's range
        is not inline-model evidence; an explicit call/inline word still is."""
        shapes = [
            ("/* GAME.OVR U38 public entry 99, +2A87..+2AE6 (95 original bytes). */", True),
            ("assert ins[-1].address + ins[-1].size == 0x11F3", True),
            ("0x0829: 0x047A,", True),
            ("* 67 instructions: the next prologue is +0x047A)", True),
            ("call OVL_0038_101D entry inline at 0x101D", False),
            ("/* calls OVL_0038_2A87 at +2A87..+2AE6 */", False),
        ]
        for text, expected in shapes:
            self.assertEqual(scq.is_boundary_mention({"text": text}), expected, text)

    def test_ip_redirect_and_comment_mentions_fail_closed(self):
        """A loader's `c->ip = 0x....u` redirect and a bare comment mention name
        an address without implementing it, so neither is inline evidence."""
        shapes = [
            ("c->ip = 0x07BCu;", True),
            ("                c->ip = 0x0A74u;", True),
            ("/* DOS v1.3 unit 25 load word +0E0C, classifier +06FB, load hook +07BC.", True),
            (" * +0x179B. Evidence: docs/re/U1C_SURPRISE_16C9.md.", True),
            # keyword rescue: a comment that says the body is inlined still counts
            ("/* this routine is inlined here, entry 0x07BC */", False),
            ("call OVL_0038_101D at 0x101D", False),
        ]
        for text, expected in shapes:
            self.assertEqual(scq.is_boundary_mention({"text": text}), expected, text)

    def test_redirect_into_a_span_the_strict_checker_pins_is_inline(self):
        """The exact 07BC case: +0E0C's strict model reaches +07BC by `c->ip = 0x07BCu` and models it
        (load_hook), and +0E0C's checker pins (0x07BC, 0x0A74, sha256). That is inline_in_strict; the same
        text without the pin stays a boundary, and a pin for another offset or unit rescues nothing."""
        known = {"OVL_0025_0E0C": (0x25, 0x0E0C), "OVL_0025_07BC": (0x25, 0x07BC)}
        main_files = {"OVL_0025_0E0C": ["src/recovered/overlay_u25_load_word_0e0c.c"]}
        source = {"file": "src/recovered/overlay_u25_load_word_0e0c.c"}
        texts = ("        c->ip = 0x07BCu;", "/* DOS v1.3 unit 25 load word +0E0C, classifier +06FB, load hook +07BC")

        def status(text, pinned, cid="OVL_0025_07BC"):
            return scq.prior_evidence(cid, main_files, {"source": [dict(source, text=text)], "tooling": [], "docs": []},
                                      [], [], {"OVL_0025_0E0C"}, known, pinned)

        pin = {"OVL_0025_0E0C": {0x06FB, 0x0756, 0x07BC, 0x0E0C}}
        for text in texts:
            result = status(text, pin)
            self.assertEqual(result["status"], "inline_in_strict", text)
            self.assertEqual([(m["covered_by"], m["pinned_span"]) for m in result["inline"]], [("OVL_0025_0E0C", True)])
            self.assertEqual(status(text, None)["status"], "source_mention", text)
            self.assertEqual(status(text, {})["status"], "source_mention", text)
            self.assertEqual(status(text, {"OVL_0025_0E0C": {0x07BD, 0x0E0C}})["status"], "source_mention", text)
            self.assertEqual(status(text, {"OVL_0026_0E0C": {0x07BC}})["status"], "source_mention", text)
        # the pin is unit-local: the same offset in another unit is not covered by +0E0C's checker
        known["OVL_0026_07BC"] = (0x26, 0x07BC)
        self.assertEqual(status(texts[0], pin, "OVL_0026_07BC")["status"], "source_mention")

    def test_pinned_spans_reads_the_ci_registry(self):
        """On the real tree, +0E0C's registered checker pins exactly its four spans, +07BC among them."""
        pinned = scq.pinned_spans(Path(__file__).resolve().parents[2])
        self.assertEqual(pinned["OVL_0025_0E0C"], {0x06FB, 0x0756, 0x07BC, 0x0E0C})

    def test_prior_status_none_when_no_evidence(self):
        inventory, ledger, classifications = fixture()
        state = scq.load_queue(inventory, ledger, classifications, [], None)
        empty = {cid: {"source": [], "tooling": [], "docs": []}
                 for cid in state["candidates"].values()}
        result: dict = scq.build_queue(state, callgraph(), inventory["units"], ledger,
                                       classifications, empty, {}, {}, {},
                                       dict(ovr=bytes(200)))
        self.assertTrue(all(row["prior_translation"]["status"] == "none"
                            for row in result["rows"]))

    def test_export_is_order_independent(self):
        original = queue()
        data, graph = copy.deepcopy(fixture()), copy.deepcopy(callgraph())
        data[0]["units"][0]["functions"].reverse()
        data[1].reverse()
        data[2].reverse()
        graph["edges"].reverse()
        self.assertEqual(original, queue(data=data, graph=graph))


if __name__ == "__main__":
    unittest.main()
