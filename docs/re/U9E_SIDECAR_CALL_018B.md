# U9E +018B sidecar lookup composition: bounded original-byte parity

`pr_u9e_sidecar_call_018b` starts with the five prepared words at U9E
`CS:018B`. It executes the far-call frame, composes the merged resident
`RES_00BA_2199` lookup over a real 1 MiB RAM image, and follows the U9E
result test. An absent result stops at `CS:019C`, before the set-membership
call setup. A found result stops at `CS:02D4`, before the indirect handler
dispatch setup. Neither stop executes another outside effect.

This is a bounded continuation on a separate branch from the earlier
`+0160..+018A` prefix. It does not establish a natural reached caller, the
remaining U9E function, or strict full-C/identity credit.

## Recall and source bounds

`deja "Poolrad U9E +018B sidecar lookup 00BA:2199 real-memory alias CPU flags composition"`
timed out locally. Reused [U9E_0160.md](U9E_0160.md) for the U9E routine
extent and argument frame, [RESIDENT_SIDECAR_LOOKUP.md](RESIDENT_SIDECAR_LOOKUP.md)
for the independently translated resident list walk, and
[RESIDENT_LOADER_MAP.md](RESIDENT_LOADER_MAP.md) for the loaded-image
reconstruction. The two exact spans below are checked against ignored local
executables by the parity script.

| Input/span | Evidence |
| --- | --- |
| `GAME.OVR` | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| U9E call/result slice | file `[0x2B075,0x2B086)`, `+018B..+019B`; 17 bytes, 5 instructions; SHA-256 `e73d8f291398fc6975f6e2164133aebc49cf6ff33774e365c746c0ef58330646` |
| `START.EXE` | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Resident loaded body | expanded image `[0x2D39,0x2DA8)`, `00BA:2199..2207`; 111 bytes, 37 linear instructions ending with `RETF 10`; SHA-256 `721e86627eac3e72deed41d2a26283e7c95444d99fc4f762a9714b11504dfa9c` |

The resident reads the caller's character `+0x7F` head, walks nine-byte
sidecar nodes through each `+5` link, compares node byte zero with the low
byte of the key word, and writes the current node or null to the caller's
far-pointer cell. The merged `pr_rec_res_00ba_2199` supplies that access
order. The new adapter maps each callback to the same mutable 1 MiB RAM as
the U9E stack and sidecar nodes; no host objects stand in for those bytes.

The original resident uses separate word reads for the cursor null test,
then `LES` for the node pointer. At effective offset `FFFF`, `LES` consumes
four contiguous physical bytes, while a separate second word at offset
`0001` can address another physical location. The adapter follows the
merged routine's read sequence to supply the `LES` high word from the
contiguous address. It also reads the two far arguments from the caller
stack as `LES` would. This matters for wrapped stack pointers and for a
cursor cell that aliases the character head. The model reconstructs the
resident's saved BP, local bytes, far return, `AX`/`DX`/`DI`/`ES`, and the
U9E `OR AL,AL` flags before choosing the stop.

## Verification and boundary

`tests/recovered/check_overlay_u9e_sidecar_call_018b.py` executes both
original code spans in 16-bit Unicorn, then compares all 1 MiB RAM bytes
and all 14 CPU words with the C composition at the same stop. Its 140
synthetic cases include empty, first-node, later-node and exhausted lists;
same-segment and cross-segment cursor/head aliases; distinct and shared
`DS`/`SS`; wrapped stack pointers; a cursor cell at offset `FFFF`; and
two input flag patterns.
`tests/recovered/test_overlay_u9e_sidecar_call_018b.c` pins the found and
absent return frames and argument rejection. A focused C11 `-Wall -Wextra
-Werror` ASan/UBSan CTest is the publication gate.

The merged resident lookup has a host `max_nodes` guard for corrupt cyclic
lists; its limit status is outside the DOS parity claim. The 140 cases are
synthetic entry frames, not a natural game execution through the caller.
