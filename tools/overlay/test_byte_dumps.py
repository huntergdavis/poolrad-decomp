#!/usr/bin/env python3
"""Controls for the no-verbatim-byte-dump check.

Positive control: a document carrying a real-looking span dump must fail.
Negative controls: a digest/offset-only document must pass, an allowlisted dump
must pass, and an allowlist row that names no task, no expiry or no real
justification must be an error.

Two shapes are treated differently from spans. `0x02, 0x0A, ...` enumerations are
FATAL by default - syntax cannot tell a semantic set from original bytes
reformatted with `0x` and commas - and each needs a per-file reason in the
allowlist. Space-separated two-digit decimal lists (`17 18 19 ...`, values below
0x80) are not byte-like, which is what keeps this repository's function-number
lists out of the failure set. The tests pin both directions.

An allowlist permission also expires: the row names the task that removes it and
carries an expiry marker, and the checker warns when that task is done while rows
remain.
"""
import io
import json
import shutil
import subprocess
import tempfile
import unittest
from contextlib import redirect_stdout
from pathlib import Path
from unittest.mock import patch

import check_byte_dumps as check


def document(body: str) -> str:
    return "# Evidence\n\n" + body + "\n"


DUMP = "55 89 e5 83 ec 04 b0 02 50 9a 2a 00 2b 00 a0 87"
JUSTIFICATION = "pre-rule span dump; the scrub replaces it with a digest at the offset; expires when t139 completes"


class ByteDumpTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.allow = self.root / "allowlist.tsv"
        self.allow.write_text("path\ttask\tjustification\n")

    def tearDown(self):
        self.temp.cleanup()

    def write(self, name: str, text: str) -> Path:
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text)
        return path

    def row(self, name: str, task="t139", why=JUSTIFICATION) -> None:
        self.allow.write_text(f"path\ttask\tjustification\n{name}\t{task}\t{why}\n")

    def run_check(self, *paths, report=False, task_status=None):
        argv = ["--root", str(self.root), "--allowlist", str(self.allow)]
        argv += [str(p) for p in paths]
        if report:
            argv.append("--report")
        if task_status:
            argv += ["--task-status", str(task_status)]
        buffer = io.StringIO()
        with redirect_stdout(buffer):
            code = check.main(argv)
        self.output = buffer.getvalue()
        return code

    def test_a_span_dump_fails(self):
        path = self.write("docs/re/dumped.md", document("The body is:\n\n`" + DUMP + "`\n"))
        self.assertEqual(self.run_check(path), 1)

    def test_an_xxd_or_objdump_column_fails(self):
        path = self.write("docs/re/xxd.md", document(
            "0000a1f0: 55 89 e5 83 ec 04 b0 02 50 9a 2a 00 2b 00  a0 87 70\n"))
        self.assertEqual(self.run_check(path), 1)

    def test_a_bytes_fence_fails_even_when_short(self):
        path = self.write("docs/re/fenced.md", document("```bytes\n55 89\n```\n"))
        self.assertEqual(self.run_check(path), 1)

    def test_a_digest_at_an_offset_passes(self):
        path = self.write("docs/re/digest.md", document(
            "Body `OVL_001C_0292`, 78 bytes at GAME.OVR offset 4056,\n"
            "SHA-256 `7f66f8be811fe835d3ed3ab0fc0772f57e10ab39b5b5beda8aa2542262f92ebb`.\n"))
        self.assertEqual(self.run_check(path), 0)

    def test_an_0x_value_list_fails_by_default(self):
        # Syntax alone cannot tell a semantic opcode set from copied bytes, so the
        # default is fatal and an exception has to be argued for per file.
        path = self.write("docs/re/values.md", document(
            "the exact compares `0xB8, 0xBB, 0xBD, 0xBF, 0xC1, 0xC3, 0xF7, 0xF9, 0x100, 0x10C`\n"))
        result = check.scan(self.root, [path], self.allow)
        self.assertEqual(self.run_check(path), 1)
        self.assertTrue(any(kind.startswith("0x value list") for _, _, kind in result["hits"]))

    def test_an_allowlisted_value_list_passes(self):
        path = self.write("docs/re/values.md", document(
            "the compares `0xB8, 0xBB, 0xBD, 0xBF, 0xC1, 0xC3, 0xF7, 0xF9`\n"))
        self.row("docs/re/values.md", why="opcode compare set the sentence needs; expires when t139 completes")
        self.assertEqual(self.run_check(path), 0)

    def test_a_high_bit_run_is_a_dump(self):
        path = self.write("docs/re/highbit.md", document("80 81 82 83 84 85 86 87\n"))
        self.assertEqual(self.run_check(path), 1)

    def test_a_decimal_list_above_80_is_flagged_and_needs_a_reason(self):
        # Documented blind spot: the high-bit rule cannot tell 0x95 from decimal 95.
        path = self.write("docs/re/decimals.md", document(
            "table indices 51 52 53 54 58 59 95 98 137\n"))
        self.assertEqual(self.run_check(path), 1)
        self.row("docs/re/decimals.md",
                 why="decimal table indices, not bytes; flagged by the high-bit rule; expires when t139 completes")
        self.assertEqual(self.run_check(path), 0)

    def test_an_indented_bytes_fence_fails(self):
        path = self.write("docs/re/indented.md", document("   ```bytes\n   55 89\n   ```\n"))
        self.assertEqual(self.run_check(path), 1)

    def test_a_numeric_run_does_not_hide_a_later_byte_run(self):
        path = self.write("docs/re/mixed.md", document(
            "counts 17 18 19 20 21 22 23 24 then bytes " + DUMP + "\n"))
        self.assertEqual(self.run_check(path), 1)

    def test_an_addressed_column_without_a_trailing_space_fails(self):
        # Shape decides an addressed column, whatever the byte values are.
        path = self.write("docs/re/xxd-nospace.md", document(
            "00000100: 01 02 03 04 05 06 07 08\n"))
        self.assertEqual(self.run_check(path), 1)

    def test_an_addressed_column_with_ascii_gutter_fails(self):
        path = self.write("docs/re/xxd-ascii.md", document(
            "00000100: 01 02 03 04 05 06 07 08  ........\n"))
        self.assertEqual(self.run_check(path), 1)

    def test_addressed_column_spacing_variants(self):
        cases = {
            "internal-double-space": ("00000100: 01 02 03 04  05 06 07 08\n", 1),
            "internal-tab": ("00000100: 01 02 03\t04 05 06 07 08\n", 1),
            "double-space-trailing": ("00000100: 01 02 03 04  05 06 07 08 \n", 1),
            "single-space": ("00000100: 01 02 03 04 05 06 07 08\n", 1),
            "eight-with-gutter": ("00000100: 01 02 03 04 05 06 07 08  ....55 89\n", 1),
            "seven-with-gutter": ("00000100: 01 02 03 04 05 06 07  ....55 89\n", 0),
            "seven-trailing-space": ("00000100: 01 02 03 04 05 06 07 \n", 0),
        }
        for name, (line, expected) in cases.items():
            with self.subTest(name):
                path = self.write(f"docs/re/{name}.md", document(line))
                self.assertEqual(self.run_check(path), expected, name)

    def test_a_seven_pair_addressed_row_passes(self):
        path = self.write("docs/re/seven.md", document(
            "0000a1f0: 55 89 e5 83 ec 04 b0 \n"))
        self.assertEqual(self.run_check(path), 0)

    def test_the_cli_first_line_status_is_parsed(self):
        line = "t119 [high] done  Poolrad: some finished task\n"
        with patch.object(check.subprocess, "run",
                          return_value=subprocess.CompletedProcess(["dunkingsheep"], 1, stdout="{}", stderr="")):
            self.assertIsNone(check.task_status("t119"))
        with patch.object(check.subprocess, "run",
                          side_effect=[subprocess.CompletedProcess(["d"], 1, stdout="{}", stderr=""),
                                       subprocess.CompletedProcess(["d"], 0, stdout=line, stderr="")]):
            self.assertEqual(check.task_status("t119"), "done")

    def test_a_decimal_number_list_is_not_a_dump(self):
        path = self.write("docs/re/numbers.md", document(
            "functions 17 18 19 20 21 22 23 26 27 28 are path-complete\n"))
        self.assertEqual(self.run_check(path), 0)

    def test_an_allowlisted_dump_passes_and_shows_the_permission(self):
        path = self.write("docs/re/legacy.md", document("`" + DUMP + "`\n"))
        self.row("docs/re/legacy.md")
        self.assertEqual(self.run_check(path), 0)
        self.assertIn("ALLOWED: docs/re/legacy.md", self.output)

    def test_a_row_without_a_task_is_an_error(self):
        path = self.write("docs/re/legacy.md", document("`" + DUMP + "`\n"))
        self.allow.write_text(f"path\ttask\tjustification\ndocs/re/legacy.md\t{''}\t{JUSTIFICATION}\n")
        self.assertEqual(self.run_check(path), 1)

    def test_a_row_without_an_expiry_marker_is_an_error(self):
        path = self.write("docs/re/legacy.md", document("`" + DUMP + "`\n"))
        self.row("docs/re/legacy.md", why="pre-rule landing, reviewed, no end date at all")
        self.assertEqual(self.run_check(path), 1)

    def test_a_done_task_with_rows_remaining_warns(self):
        path = self.write("docs/re/legacy.md", document("`" + DUMP + "`\n"))
        self.row("docs/re/legacy.md")
        status = self.root / "status.json"
        status.write_text(json.dumps({"t139": "done"}))
        self.assertEqual(self.run_check(path, task_status=status), 0)
        self.assertIn("t139", self.output)
        self.assertIn("WARNING", self.output)

    @unittest.skipUnless(shutil.which("dunkingsheep"), "no Dunking Sheep CLI on this host")
    def test_the_real_cli_completed_task_warns(self):
        # The expiry guard has to work against the real CLI, not only an injected
        # fixture: t119 is a real, already-completed task (read-only lookup).
        if check.task_status("t119") is None:
            self.skipTest("no Dunking Sheep daemon/socket on this host")
        path = self.write("docs/re/legacy.md", document("`" + DUMP + "`\n"))
        self.row("docs/re/legacy.md", task="t119")
        out = io.StringIO()
        with redirect_stdout(out):
            code = check.main(["--root", str(self.root), "--allowlist", str(self.allow), str(path)])
        self.assertEqual(code, 0)
        self.assertIn("t119", out.getvalue())
        self.assertIn("WARNING", out.getvalue())

    def test_a_stale_allowlist_entry_only_warns(self):
        clean = self.write("docs/re/clean.md", document(
            "SHA-256 `7f66f8be811fe835d3ed3ab0fc0772f57e10ab39b5b5beda8aa2542262f92ebb`\n"))
        self.row("docs/re/scrubbed.md")
        self.assertEqual(self.run_check(clean), 0)
        self.assertIn("no longer hits", self.output)

    def test_report_names_hits_and_does_not_claim_a_pass(self):
        path = self.write("docs/re/dumped.md", document("`" + DUMP + "`\n"))
        result = check.scan(self.root, [path], self.allow)
        self.assertEqual(len(result["hits"]), 1)
        self.assertEqual(result["hits"][0][0], "docs/re/dumped.md")
        self.assertEqual(self.run_check(path, report=True), 0)


if __name__ == "__main__":
    unittest.main()
