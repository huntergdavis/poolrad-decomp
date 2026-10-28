"""Event summaries must distinguish push and PR runs without trusting ref markup."""
import contextlib
import io
import tempfile
import unittest
from pathlib import Path

import report_ci_context as report


class ContextTest(unittest.TestCase):
    def test_push_and_pr_have_distinct_revision_semantics(self):
        for event, kind in (("push", "pushed revision"),
                            ("pull_request", "pull-request merge revision")):
            with self.subTest(event=event), tempfile.TemporaryDirectory() as temp:
                path = Path(temp) / "summary"
                path.write_text("existing summary\n")
                env = {"GITHUB_EVENT_NAME": event, "GITHUB_SHA": "a" * 40,
                       "GITHUB_REF": "refs/heads/main", "GITHUB_STEP_SUMMARY": str(path)}
                output = io.StringIO()
                with contextlib.redirect_stdout(output):
                    self.assertEqual(report.main(env), 0)
                self.assertIn(f'event="{event}"', output.getvalue())
                self.assertIn(kind, output.getvalue())
                self.assertIn("a" * 40, output.getvalue())
                self.assertTrue(path.read_text().startswith("existing summary\n"))
                self.assertIn(kind, path.read_text())

    def test_absent_metadata_is_unknown(self):
        text = report.context({})
        self.assertIn('event="unknown"', text)
        self.assertIn('workflow_sha="unknown"', text)
        self.assertIn('revision_kind="unknown revision kind"', text)

    def test_ref_cannot_inject_html_or_extra_lines(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "summary"
            with contextlib.redirect_stdout(io.StringIO()):
                report.main({"GITHUB_HEAD_REF": "</pre><script>bad</script>\nevent=push",
                             "GITHUB_STEP_SUMMARY": str(path)})
            text = path.read_text()
            self.assertNotIn("<script>", text)
            self.assertIn("&lt;script&gt;", text)
            self.assertIn(r"\nevent=push", text)

    def test_each_hygiene_job_reports_context_before_any_gate(self):
        # No third-party YAML parser: these literal step boundaries are intentional.
        workflow = (Path(__file__).resolve().parents[2] /
                    ".github/workflows/repository-hygiene.yml").read_text()
        for job in ("checker-builds", "verify-public-tree"):
            text = workflow.split(f"  {job}:\n", 1)[1]
            first_named_step = text.split("      - name:", 1)[1].split("      - ", 1)[0]
            self.assertIn("Record workflow event and revision before gates", first_named_step)
            self.assertIn("python3 tools/overlay/report_ci_context.py", first_named_step)


if __name__ == "__main__":
    unittest.main()
