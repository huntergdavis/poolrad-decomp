#!/usr/bin/env python3
"""Record the event/revision being checked before any hygiene gate can fail."""
import html
import json
import os
from pathlib import Path


def context(environ):
    event = environ.get("GITHUB_EVENT_NAME", "unknown")
    kind = {"push": "pushed revision", "pull_request": "pull-request merge revision"}.get(
        event, "unknown revision kind")
    fields = [("event", event), ("revision_kind", kind)]
    for label, name in (("workflow_sha", "GITHUB_SHA"), ("ref", "GITHUB_REF"),
                        ("head_ref", "GITHUB_HEAD_REF"), ("base_ref", "GITHUB_BASE_REF"),
                        ("run_id", "GITHUB_RUN_ID")):
        fields.append((label, environ.get(name) or "unknown"))
    return "\n".join(f"{label}={json.dumps(value)}" for label, value in fields) + "\n"


def main(environ=None):
    environ = os.environ if environ is None else environ
    text = context(environ)
    print(text, end="")
    summary = environ.get("GITHUB_STEP_SUMMARY")
    if summary:
        with Path(summary).open("a") as stream:
            stream.write("### Workflow event and revision\n\n<pre>" + html.escape(text) + "</pre>\n\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
