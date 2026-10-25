"""Identify the executable used by an unpinned native DOSBox-X harness.

The caller must launch the returned path. A release string or source revision
does not establish binary provenance, so this helper never asserts a pinned
build. Existing independently attested pinned-build harnesses stay separate.
"""
# NOT_A_ROW_CHECKER: executable provenance only; no recovered-body claim.
from hashlib import sha256
import os
from pathlib import Path
import shutil
import subprocess

PINNED_SOURCE = '5a9c7c53481c2e933948039c33256776653f4faf'


def describe_dosbox(binary):
    """Resolve once, hash and query that file; refuse incomplete provenance."""
    requested = os.fspath(binary)
    if os.path.dirname(requested):
        selected = requested
    else:
        selected = shutil.which(requested)
        if selected is None:
            raise FileNotFoundError(f'DOSBox-X executable not found: {requested}')
    path = Path(selected).resolve(strict=True)
    digest = sha256(path.read_bytes()).hexdigest()
    probe = subprocess.run(
        [str(path), '--version'], check=False, capture_output=True, text=True,
        timeout=10,
        env=dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy'))
    version = next((line.split(',')[0].strip()
                    for line in (probe.stdout + probe.stderr).splitlines()
                    if 'DOSBox-X version' in line), None)
    # The installed DOSBox-X prints its version successfully but exits with 1.
    # Accept that convention only with the expected banner, and expose it.
    if probe.returncode not in (0, 1):
        raise subprocess.CalledProcessError(probe.returncode, probe.args,
                                            probe.stdout, probe.stderr)
    if not version:
        raise ValueError(f'No DOSBox-X version reported by {path}')
    return dict(path=str(path), version=version, sha256=digest,
                version_exit_code=probe.returncode,
                pinned_source=PINNED_SOURCE, pinned_build_used=False)
