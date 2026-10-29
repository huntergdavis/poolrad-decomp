# Scrubbed span offset attribution (t356)

This follow-up attributes **12 of 18 marker occurrences** to byte slices in
the pinned original files. The 18 occurrences contain **17 distinct hashes**:
the U38 installer slice appears twice in one document. The six remaining
occurrences retain `offset=UNRESOLVED`. They do not pass media verification.
This is evidence repair, not a function, ownership, or credit decision.
These are proposed attributions. Per-span acceptance remains pending
independent verification; this author report does not lift that review gate.

The source scrub is PR #1618, head
`c4ea7829a43809a6f4fa0ea60b24e014d102b058`. The
[manifest](scrubbed_span_offsets.csv) preserves every marker's document,
span ID, original length and SHA-256. It adds raw-file coordinates only where
the digest matches and the surrounding document selects that occurrence.
Neither the manifest nor this report contains original byte sequences.

| File | Required SHA-256 |
| --- | --- |
| START.EXE | d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd |
| GAME.OVR | 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 |

Discovery searched every possible slice of the recorded lengths in both
raw files and in the loader-expanded START image, comparing SHA-256 values.
It ran through the shared heavy-job wrapper. Hash equality alone does not
identify which repeated occurrence a document means. The manifest records
the contextual selection, including duplicate occurrences that were rejected
as the document's intended location.

| Documented span | Raw-file coordinate | Context selecting it |
| --- | --- | --- |
| RES0464 +29D7 | START.EXE:0x7050 | Expanded image 0x7017 = 0x4640 + 0x29D7 |
| RES0464 +29CB | START.EXE:0x7044 | Expanded image 0x700B = 0x4640 + 0x29CB |
| Review handoff decoy prefix | GAME.OVR:0x2BEBC | Explicit decoy file offset, not an equal resident prefix |
| Slot-A key +33BD | GAME.OVR:0x4103 | Explicit file span |
| Resident +001F caller, two overlapping records | START.EXE:0x8B16 | Explicit file span in both documents |
| RES0464 +29E6 store | START.EXE:0x705F | Expanded image 0x7026 = 0x4640 + 0x29E6 |
| U1C DELAY resident helper | START.EXE:0x7E87 | Explicit expanded image 0x7E4E; unique equal raw-file slice |
| U20 trailing-data prefix | GAME.OVR:0x6826 | Decimal body offset 26410 + 252-byte extent |
| U38 installer, two repeated records | GAME.OVR:0x12070 | Explicit unit base 0xEC5B + offset 0x3415 |
| U9E entry8 slice | GAME.OVR:0x2BCEF | Explicit file interval [0x2BCEF,0x2BCF8) |

Raw START.EXE offsets are not loader-expanded image offsets or loaded RAM
addresses. The RES0464 records describe loaded CS 0C88 with original segment
0464; no RAM capture or relocation correctness is newly verified here. The
raw slices match the retained digests; that fact does not recreate the saved
execution that originally motivated the documents.

## Six suspended occurrences

- `OVL_009E_0000_INDEX_REACHABILITY-1`: the numeric-table marker cuts
  surrounding tokens. Its digest matches neither raw file nor the expanded
  image. A complete table/source representation was not reconstructed.
- `RES_08AE_ENTRY_017B_CPU-1`: context describes a set of eight synthetic
  input values, not a stated contiguous original-file span. No slice matches.
- `U1C_AND_OR_0DEF-1`: saved ECL script data has no match in the searched
  executable/overlay representations. DAX or decompressed-resource
  attribution was not performed.
- `U25_SKIP_1EA6-1`, `-2`, and `-3`: opcode-value sets grouped by prefetch
  count are not stated contiguous original-file spans. No slice matches.

These findings do not restore the scrubbed text or silently reinterpret its
digest. A future semantic correction of a synthetic/value-set record must
keep that distinction explicit; inventing a file offset is not a repair.

## Verification contract

Run through the shared wrapper from an owner-media checkout:

```sh
../poolrad-heavy.sh python3 tools/verify_scrubbed_spans.py --media-dir resources/DOS/POOLRAD
```

The checker pins both complete input files, requires the exact marker/manifest
membership, and compares each resolved slice's length and digest. It emits
only metadata and errors. Exit **0** means every recorded span verifies;
exit **1** means a malformed record, missing/unsupported media, stale marker,
out-of-range location or hash mismatch; exit **2** means verified resolved
spans coexist with explicitly suspended unresolved records. For this
manifest the required result is **SUSPENDED: 12 verified, 6 unresolved,
0 errors, exit 2**, not a blanket pass.

The canonical parent checker, `tools/overlay/check_byte_dumps.py
--verify-evidence --verify-base BASE --verify-head HEAD`, compares metadata
with historical document text. The merged parent removed the earlier
`tools/check_original_bytes.py` entry point. Historical-text checking does
not verify media offsets and does not substitute for this checker. Synthetic tests in
`tools/test_verify_scrubbed_spans.py` exercise the failure contract without
original bytes.

NOT VERIFIED: independent offset-attribution review; the six suspended
records' source representation; complete removal of all original-byte
exposure classes; historical capture/relocation correctness; full media
suite, runtime/native parity, function identity, ownership or credit.
