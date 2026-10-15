# U3C2 +0909 entry prefix before its first jump

`pr_u3c2_entry_prefix_0909` executes four instructions at +0909..+0912
and stops before +0913 JMP +0918. It saves BP, establishes BP, subtracts
32 from SP, and clears the byte at SS:[BP-3]. This is a bounded synthetic
prefix with zero strict full-C credit and no natural/native-frame claim.
It does not execute a branch or the separately published caller windows.

| Artifact | Measured identity |
| --- | --- |
| GAME.OVR | 206105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Caller +0909..+0BD8 | File `[197723,198443)`; SHA-256 `38a673986257d14dbe4da3ede438a98c0b516884c6fc92c79228088a68dba9ed` |
| Prefix +0909..+0912 | File `[197723,197733)`; 10 bytes / 4 instructions; SHA-256 `fc6542ae7bd0b6f694715bb0984ef7bf2118607c0dff04566c1352b189f8214b` |
| Stop boundary | Before +0913's two-byte JMP +0918; no jump execution |

For initial SP `s`, the resulting BP is `(s-2) mod 65536`, SP is
`(s-34) mod 65536`, and IP is +0913. The saved initial BP occupies the word
starting at SS:`(s-2) mod 65536`; SS:`(s-5) mod 65536` becomes zero.
Only these three physical bytes may change. Other allocated local bytes
remain untouched, and general registers and segments other than BP/SP/IP
retain their values. Word stores use contiguous physical bytes with A20
disabled; effective offsets and SP wrap at 16 bits.

CF, PF, AF, ZF, SF and OF follow the 16-bit SUB of 32 from the SP value
after PUSH BP. AF is defined zero because the subtrahend's low nibble is
zero. MOV leaves these flags intact; all other flags are preserved.
There is no undefined-AF convention in this prefix. Immutable code and no
asynchronous traps are preconditions. Null pointers, short RAM and other
entry IPs are rejected without RAM or CPU mutation.

## Validation and reproduction

With measured Capstone 5.0.6 and Unicorn 2.1.4 importable:

```sh
python3 tools/overlay/u3c2_entry_prefix_0909_audit.py --quick \
  --output resources/u3c2-entry-0909/quick-parity.json
```

The bounded original-byte gate passes 478 cases with all 4/4 prefix sites,
comparing every byte of 1 MiB RAM and all 14 CPU words. It checks all 256
low SP values, all 32 signed-overflow SP values, six initial flag states,
stack/effective-offset/A20 boundaries for three SS values, and four seeded
full-RAM inputs. Each case has a five-instruction cap and stops before JMP.
There are zero masked flags, undefined flags, or emulator substitutions.

| Defined flag | Clear cases | Set cases |
| --- | ---: | ---: |
| CF | 374 | 104 |
| PF | 223 | 255 |
| AF | 478 | 0 |
| ZF | 465 | 13 |
| SF | 309 | 169 |
| OF | 422 | 56 |

The focused C test checks 1,104 whole-RAM/CPU synthetic snapshots plus
rejection behavior. Its expected flags use signed arithmetic and low-byte
popcounts independently of the model's bit formulas. Warning-clean C11
ASan/UBSan validation passes. The warning-clean focused CMake build and
`poolrad_overlay_u3c2_entry_prefix_0909_tests` CTest pass (1/1). Tracked-file
media hygiene and the copied research snapshot checksums also pass.
Original media, disassembly and audit reports
stay under ignored `resources/`; public tests use synthetic data only.

## Parity review

- `target_function`: U3C2 +0909 restricted to the four-instruction prefix.
- `source_candidates`: `src/recovered/overlay_u3c2_entry_prefix_0909.c`.
- `signals`: pinned bytes, exact stop boundary, save/allocation/clear order,
  complete defined flags, wrap semantics, and whole-RAM/all-CPU comparisons.
- `score`: 4/4 sites and 478/478 original-byte cases; bounded coverage only.
- `verdict`: `match` for the prefix under its documented preconditions;
  whole-caller parity remains `unclear`.
- `review_notes`: neither the jump, successor block, other caller windows,
  callee, return path nor a natural/native frame is covered. Mutable code
  and asynchronous exceptions are excluded. No function credit is added.

Bounded `deja "poolrad U3C2 0909 0913 entry prefix SUB SP 20 flags"`
completed with no matches and reported zero sessions searched. Reused
evidence/conventions: the reviewed `2235fc5` finite-stop/full-state harness,
merged `u86_procvars_3413_audit.py` CPU/A20 helpers, and merged
`overlay_u6c_record_table_sum_3f31.h` CPU interface. The new prefix hashes,
write guard and SUB flags are checked independently. This branch has no
code dependency on the separately published `b22dbb1` caller windows.
