# October release parity hooks (t148)

Status: source-only port checked; build and native restore not run yet.

The January emulator serializes the renderer's `TempLine` host pointer in a
save state. A private cross-process restore crashed at that stale pointer in
both the January system executable and the January parity executable. Upstream
[commit b0eccfca](https://github.com/joncampbell123/dosbox-x/commit/b0eccfca48daad19f9a3ab91e169e6fa685ad360)
replaced pointer serialization with the buffer size and contents on January 15,
2026; the fix appears in the March 29 changelog and remains in the October
release. The separate May 24 graphics fix initializes `drawline_idx`; it is not
the pointer fix.

This patch carries the existing dispatch/observer hooks to upstream tag
[`dosbox-x-v2026.10.01`](https://github.com/joncampbell123/dosbox-x/releases/tag/dosbox-x-v2026.10.01),
commit `b6abbd5980a885f5f310a4088c59a8688d1b116c`. It leaves the January patch
and binaries unchanged. The October renderer restore code is not patched.
The intended claim is **same release and same restore code**, not the same
binary as the owner's installed executable.

## Source verification

New patch: `tools/hybrid/dosbox-x-b6abbd59-dispatch.patch`.
SHA-256: `ebb5775bb7c0e48943b7463f540d62b13ebb2981dfb502a9472fb1d60243cdcd`.

`git apply --check --verbose` passes against the exact upstream files from that
commit; all six affected paths are checked. The original January dispatch patch
fails its CPU macro context hunk there. The stage-0 observer patch checks cleanly,
but alone does not supply the dispatch helpers used by the parity checkpoint.

The new patch changes only context: the upstream `LoadMq`/`SaveMq` macro spelling
and removal of two trailing context lines. This retains the new upstream
`lock.h` include and `LockPrefixRead` helper untouched. Every added and deleted
line within the patch is byte-for-byte identical to the January patch. Hunk
line offsets are accepted by `git apply`; no fuzz or ignored whitespace is used.

- [Diff of patch files](HYBRID_OCTOBER_PATCH_DIFF.txt)
- [Apply-check log](HYBRID_OCTOBER_PATCH_CHECK.txt)

## Build provenance

Not run. The build must use a new private output directory after the live-binary
owner-state control completes. It must record the source tag/commit, patch and
checkpoint hashes, model inputs, compiler and build commands, resulting binary
hash and version banner. No existing installed binary is a build destination.

The checkpoint still requires `tools/hybrid/install_parity_checkpoint.py` and
its dispatch register/counter helpers. The planned inertness check clears all
`PR_*` environment variables and runs the new binary independently. Source
inspection or a version banner alone does not establish runtime inertness.

## Native evidence and limits

Not run for this port. The next authorized evidence is one private owner-state
copy restored with the exact live executable, then with this new build, surviving
a keypress, followed by one original/replacement capture comparison. Original
save files and the remaining state corpus are outside that run's write scope.

Not verified: compilation/linking; ABI compatibility; hooks inert at runtime;
restore compatibility or post-load survival in the new build; frame/register/RAM/
VGA parity; owner live-desk transport/audio; the other owner states; archive or
migration. No equality, whole-game parity, or function-credit claim follows from
this source-only check. The deterministic scripted-run path remains available
independently of save-state support.
