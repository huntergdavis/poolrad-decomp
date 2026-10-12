# Slot-A unit 0x6F common party-loop gate

This is an independent bounded source audit of DOS v1.3 `GAME.OVR` unit
`0x6F:+0x19EA..+0x1A12`. The gate is reached directly when the current
slot's file selector returns AL=0, or after the allocation, record loader,
party insertion, and `+0x02AE` post-insertion call return on the AL-nonzero
path. The audit identifies the exact next-slot and loop-exit conditions,
then names the immediate post-loop pointer targets. It adds no C code and
makes no count, identity, full-loader, or C-native saved one-Up parity claim.

Before decoding, `deja "poolrad u6f 19ea 19f7 party loop count next slot
return selector failure post insert"` found no direct indexed session.
The merged [first-slot continuation map](U6F_PARTY_CONTINUATIONS.md),
[party-loop entry](SLOT_A_PARTY_ENTRY.md), and
[native post-insertion stop](SLOT_A_NATIVE_19E7_LOOP_GATE.md) supplied
address and frame leads. The original count setup, increment, loop gate,
and pointer test below were independently decoded and hash-checked.

| Ignored input or source span | Size / 16-bit instructions | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 bytes | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `START.EXE` | 65,231 bytes | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| slot-A `SAVGAMA.DAT` | 13,137 bytes | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| Count/index setup `+0x191E..+0x193A`, GAME file `0x1F904..0x1F920` | 29 bytes / 9 | `8438328a1baa5872723d65be24f22a788000085be8dde3358e5f72f7a08024ce` |
| Back-edge increment `+0x1937..+0x193A`, GAME file `0x1F91D..0x1F920` | 4 bytes / 1 | `3449ba9b6125c2e6a59920872ab4aee9c46d88a4d55d42d05b1733c021dd4859` |
| Common gate `+0x19EA..+0x19F6`, GAME file `0x1F9D0..0x1F9DC` | 13 bytes / 4 | `f8167ce0d816f623861fd01e72891f85d08215eece2fea53846ed0cba0c3b5a4` |
| Post-loop pointer gate `+0x19F7..+0x1A12`, GAME file `0x1F9DD..0x1F9F8` | 28 bytes / 8 | `bfac9c4b98fb5b3f87a5c17a87f7f14be9895def4a1333f1ced5a24d3234ae40` |
| Full audited gate `+0x19EA..+0x1A12`, GAME file `0x1F9D0..0x1F9F8` | **41 bytes / 12 contiguous** | `573a908e8146cdce53cc4248ee24874ab87122ed2d5a18ef27503b2ae7dd1c7e` |

## Branch and frame contract

| Unit offset | Source-level effect |
| --- | --- |
| `+0x191E..+0x1925` | Copy transferred saved count byte `[BP-0x1F9]` to `[BP-0x212]`. |
| `+0x1926..+0x192E` | Test unsigned `1 <= count`; count zero jumps directly to `+0x19F7`, bypassing the party loop and its `+0x19EA` test. |
| `+0x1931..+0x193A` | Set one-based current index byte `[BP-0x1C9]` to 1 for the first slot. The backward edge enters at `+0x1937`, increments that byte once, then continues at `+0x193B` for the next slot. |
| `+0x19EA..+0x19F6` | Load `[BP-0x1C9]` into AL; compare with copied count `[BP-0x212]`. **Equal** takes `JE +0x19F7` and exits the loop. **Unequal** takes the unconditional `JMP +0x1937`, increments the index, and starts the next slot. This is an equality test, not a `< count` guard. |
| `+0x19F7..+0x1A0D` | Read party-head far pointer words `DS:0x5D96` and `DS:0x5D98`, save them at `[BP-0x1CD]` and `[BP-0x1CB]`, then OR the two words. |
| `+0x1A0E..+0x1A12` | If either pointer word is nonzero, enter `+0x1A13`; if both are zero, jump to `+0x1B7E`. These are separate post-loop phases; neither target body is translated here. |

`+0x19EA` is the first instruction **after** a returning `+0x02AE` call
on the present-file route. It is also the direct target of the selector's
missing-file branch. There is no procedure `RETF` in the audited 41 bytes;
the two branch targets lead to later code. The empty-count shortcut enters
the pointer gate without creating a current loop index.

## One pinned native input and its limit

The merged [native +0x19EA watch](SLOT_A_NATIVE_19E7_LOOP_GATE.md)
stopped immediately after the first saved-slot-A `+0x02AE` return with
`SS=1D16`, `BP=3DCA`, `AX=0005`, and 640-KiB RAM SHA-256
`3ed5cea594eea502fcf1bbd33bdc51c01c250201283b1d7f7eb21a47e854976d`.
An independent numeric read of that ignored snapshot found current index
byte `SS:[BP-0x1C9]` (`1D16:3C01`) = **1** and copied count byte
`SS:[BP-0x212]` (`1D16:3BB8`) = **6**. PM and p9 independently reported
the same two values (inbox i900/i899). The merged
[first loop-back watch](SLOT_A_NATIVE_19EA_FIRST_LOOP.md) measured
`+0x19F2` with ZF=0, then `+0x19F4`, `+0x1937`, and `+0x1947` with
index 2 and count 6. It subsequently reached the second-slot selector
at `+0x19A8`, returned AL=1 at `+0x19AD`, and stopped at `+0x19B1`
before allocation. That watch establishes one loop-back and the second
selector result. The merged
[six-gate native exit](SLOT_A_NATIVE_PARTY_LOOP_EXIT.md) separately
measured all six gate visits, the equality exit after slot 6, and the
head pointer read before stopping at `+0x19FE`.

The merged [native selector return](SLOT_A_NATIVE_19AD_SELECTOR_RETURN.md)
measured AL=1 and the `+0x19B1` fall-through for this first slot. The
continuation's allocation, record load, insertion, and post-insertion
effects precede the observed `+0x19EA` stop; this note does not reproduce
or model them. The six-gate watch measured `DX:AX=449A:0008` at
`+0x19FE` before the first post-loop local write. The merged
[post-loop first-call watch](SLOT_A_NATIVE_POSTLOOP_FIRST_CALL.md)
separately measured the saved run taking the nonzero-head `+0x1A0E`
branch and reaching `+0x1A1E`. This audit maps the branch from source
and does not analyze either target body.
