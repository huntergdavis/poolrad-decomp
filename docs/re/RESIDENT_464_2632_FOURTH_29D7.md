# Fourth saved-view RES_0464 +29D7 exit decision

## Scope and prior evidence

This bounded C gate begins at loaded `0C88:+29D7` after the
four-byte Move loop and stops at the first `+29E4 JBE` successor:
`+29E6` or `+29F3`. Neither successor body is executed. It is
not a complete resident routine or a native fourth-call result.

Before implementation, `deja "Poolrad fourth RES_0464 2632 29D7
exit arm after 29AB second Move 4D70"` ran with a 35-second bound
and timed out. The hash-pinned +29AB preimage and relocated Move
route were reused from
[RESIDENT_464_2632_FOURTH_29AB.md](RESIDENT_464_2632_FOURTH_29AB.md).

## Original-byte route and address evidence

The ignored START.EXE SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
Starting from the reviewed +29AB RAM SHA-256
`4c0f78a0462057168f8b7b4edd86d66af4da0f8dcf7d51c5a8079ecb4a0adde2`,
original-byte replay executes 320 instructions: ten relocated Move
calls at `+29C6` and ten `+29AB` increment arms. The `+29D5
JNE` finally falls through to `+29D7`. The derived exit RAM
SHA-256 is
`39e9beed0ef26736226b15b280ba1bb3d2e904f9e6d93e87f008f0e190d82eb3`.

The 15-byte file `0x7050..0x705F` span maps to loaded
`0C88:+29D7..+29E6`; both SHA-256 values are
`77c99f2b8a4298ab945d6692895d264a293029f10415df8bfa25d33b4bfef5b7`.
Six instructions yield seven checkpoints. `+29D7` loads
`[BP-0x16]`, `+29DA` decrements AX, `+29DB` loads the local
index, `+29DE` doubles DI, and `+29E0` compares AX unsigned
against `DS:[DI+0x4D70]`. On the derived route the table word is
below AX, so `+29E4 JBE` falls through to `+29E6` without any
RAM write. Original-byte synthetic equality and greater-table cases
take `+29F3`.

Ordered CPU tuple `CS IP AX BX CX DX SI DI BP SP DS ES SS FLAGS`:

    entry    0C88 29D7 000B 3E4E 0000 14A0 3E68 14AB 3E80 3E5C 14A0 3DE4 1D16 7246
    boundary 0C88 29E6 148B 3E4E 0000 14A0 3E68 007C 3E80 3E5C 14A0 3DE4 1D16 7206

The C gate matches original bytes at all seven checkpoints across
all 640 KiB RAM and all 14 CPU fields. Synthetic tests cover
fallthrough, equal and unsigned-below JBE cases, plus invalid and
unsupported input without mutation.

With `PYTHONPATH=/tmp/poolrad-unicorn`, the public
`tools/replay/resident_464_fourth_29d7_unicorn.py` derives the
exit preimage from the ignored
`fourth-29ab-stops/ram-00.bin` parent image and optionally emits
the seven private snapshots beneath ignored `resources/`.

The saved fallthrough begins a distinct `+29E6..+29F3`
`DS:[DI+0x4D70]` writer, file `0x705F..0x706C`, SHA-256
`9ca81ab58e1a2b24f894125a76afde21c2e0ad12c9bc527463e3250a77c876b5`.
That writer is outside this gate. Direct same-run native fourth-call
capture and the 18-byte IRQ-like return residue remain unresolved;
run015 first-call native state is not cross-paired with this route.
