# U9E entry8 full-state prefix through first SYSTEM call

`pr_u9e_entry8_fullstate_prefix_0de3` executes only the first bounded
source/ABI prefix of U9E entry8. A nonzero count stops **before** the original
`LCALL 0AF8:11C0` at +0E0E; a zero count stops at the shared +0E1F tail.
Neither stop executes the nested SYSTEM callee, the +0E13 return-side body,
or U9E's `RETF 4`. This is a source candidate with no native, full-function,
or strict-count credit.

The exact U9E entry8 source scope was released by p5 (i7496) and cleared
relative to pG (i7514); PM approved this first local prefix only (i7520).
The bounded `deja` ownership query timed out under host load. This note
reuses the checked-in [entry8 audit](U9E_ENTRY8_ROLL.md) and
[full-state callback map](U9E_ENTRY8_FULLSTATE_CALLBACK_MAP.md) for the
caller ABI, then independently checks the original prefix bytes and loaded
code mapping. The existing U7F [call adapter](U7F_ROLL_CALL_0FDE.md) remains
a separate source boundary.

## Original and loaded extent

Ignored DOS v1.3 `GAME.OVR` has SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Unit `0x9E` has overlay file base `0x2AEEA`, stub paragraph `0xB0`, entry8
at +0DE3 and entry9 at +0E2E. The complete entry8 body is file
`[0x2BCCD,0x2BD18)`, 75 bytes / 29 starts, SHA-256
`7df225682ebb94351984048ef7a4fdccc8de60f9ef2450751a696fde74c583ba`.
This prefix ends before +0E0E: file `[0x2BCCD,0x2BCF8)`, 43 bytes /
17 starts, SHA-256
`d60bca0e93dde6af6abab280714fe496fa6fe7f9f2c0e5deb3dba528b6d5665f`.
The manual static audit checks both hashes, unit entry bounds and branch
targets, copies the original 75 bytes to loaded CS memory, and confirms
the C prefix leaves them unchanged. U7F +0FDE calls the literal B0:0048
stub; dispatch into the loaded U9E CS is supplied before this API begins.

At entry SS:SP holds caller return IP and CS, range WORD at SP+4, and count
WORD at SP+6. The prefix pushes BP, reserves four local bytes, zeros the
accumulator at `[BP-3]`, and tests the low count byte at `[BP+8]`. Zero
count retains AX, sets the defined CMP flags, and stops at +0E1F with
SP=entry SP-6. Nonzero count snapshots its low byte at `[BP-4]`, stores
loop index 1 at `[BP-2]`, rereads the low range byte from live `[BP+6]`,
clears AH, pushes the resulting WORD, and stops at +0E0E with SP=entry
SP-8. All SS offsets wrap at 16 bits before A20-off physical addressing;
each WORD's two bytes are physically contiguous. `XOR AH,AH` defines
CF/OF/SF/ZF/PF; AF is undefined and preserved by the source convention.

Null pointers, wrong RAM size/IP, and writes into the pinned loaded entry8
instruction bytes reject unchanged. Loaded CS:+0DE3..+0E2D must be the
original body; the source does not hash runtime RAM. Focused strict C11
tests check both stops, full 1 MiB RAM effects, all 14 CPU words, stack
offset wraps, and unchanged unsupported input. The manual original/loaded
audit exercises count zero, one, and two with the pinned code in RAM. No
Unicorn or natural caller frame was run under the host native hold.

The +0E13..+0E1E post-call path and +0E1F..+0E2D shared tail are separate
PM-gated follow-ons. A full-state callback still requires an independently
verified nested SYSTEM return frame and U9E `RETF 4` completion.

## Pinned AF after OR/XOR: model fixed (2026-10-23, census #1464 family 1)

The model kept the incoming AF across `XOR AH,AH` at `+0E0B` (mask `08C5h`, 'AF undefined'). Pinned DOSBox-X clears AF after XOR (`flags.cpp:684-691`). Fix: the mask is `08D5h`.

- **Did the evidence depend on AF?** The prefix C test did: it asserted `flags == 7256h` (AF kept) at the `+0E0E` exit. It now asserts the pinned `7246h`. NOTE: that assertion is an `assert()`, which a Release (`NDEBUG`) build compiles away; built with asserts enabled, the prefix and composition tests pass on the fixed model, and the prefix test fails on the old one. The byte audit makes no flag claim.
- **Caller-observable?** No. After `XOR AH,AH` the body runs `PUSH AX` and calls SYSTEM `0AF8:11C0`, which enters `+120F`; its `MUL` at `+1218` keeps AF (pinned `MULW`), and `SHL CX,1` at `+121D` overwrites it, before any instruction reads AF. On return, `INC AX` at `+0E13` writes AF again.
- **Evidence for the verdict (par-e decides):** the difference is not observable, and the C test now pins the corrected value, so we expect the credit to stand.
