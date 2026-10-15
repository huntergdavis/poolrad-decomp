# Fourth saved-view resident first DS write

## Scope and prior work

This bounded C model starts at `0C88:+2968`, executes the first DS
global byte write and local arithmetic, and stops at the first
successor of `+297F JBE`. The saved path falls through to `+2981`;
the synthetic equal case takes the branch to `+2999`. Neither
successor body is executed. This is not a complete resident routine
and carries no strict-count claim.

Before decoding, `deja "Poolrad fourth RES_0464 2632 2968 DS 4A28
first global write next branch saved run014"` was run; the bounded
35-second query timed out. The reviewed +2968 RAM/CPU prestate and
first global-write boundary were reused from
[RESIDENT_464_2632_FOURTH_POSTFILL.md](RESIDENT_464_2632_FOURTH_POSTFILL.md).
The prior fourth FillChar gate supplies this path's earlier runtime
return, but this DS write has its own hash-locked original-byte replay.

## Address and instruction evidence

The ignored START.EXE SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
The 25-byte file `0x6FE1..0x6FFA` span maps to loaded
`0C88:+2968..+2981`; both file and loaded SHA-256 are
`41f800c8cfabc2109e34744b51025a614f7f760d2e0cd4af49b12bbdf2d4c807`.
The saved route executes eight instructions and has nine instruction
checkpoints. At `+2968`, the code writes one byte to `DS:4A66`.
It loads a stack word at `SS:3E72`, adds it to the word at `SS:3E6A`,
doubles the index loaded from `SS:3E62`, and compares the word at
`DS:4C9C` with the updated local word. The saved `+297F JBE` is
false, reaching `+2981`. Only physical addresses `0x19466` and
`0x20FCA` differ between the input and +2981 RAM images; the latter
is one changed byte of the local word.

The ordered CPU tuple `CS IP AX BX CX DX SI DI BP SP DS ES SS FLAGS`
at entry is:

    0C88 2968 0000 3E50 0000 0000 2612 003E 3E80 3E5C 14A0 1D16 1D16 7297

At the saved fallthrough it is:

    0C88 2981 FFFF 3E50 0000 0000 2612 007C 3E80 3E5C 14A0 1D16 1D16 7286

Input RAM SHA-256 is
`3f30630f17e536b66af4e0b8e1861e5924407598bdd1721338e6d28027f9c310`;
fallthrough RAM SHA-256 is
`5f4367fda50ad7fcefa0cde9cd7e9d9b488125b1171e80ab0d21f85909d645ff`.
The C model matches **all 640 KiB RAM and all 14 CPU fields at all
nine original-byte checkpoints**. Synthetic tests cover the saved
fallthrough, equal-value taken branch, both RAM writes, invalid host
input, and unsupported entry without mutation.

## Reproduction and limit

With `PYTHONPATH=/tmp/poolrad-unicorn`, run
`tools/replay/resident_464_fourth_ds_write_unicorn.py` on the ignored
+2968 RAM snapshot. Optional `--snapshot-dir` stays beneath ignored
`resources/` and writes nine private RAM images and a CPU manifest.
The focused C test accepts the pre-RAM path and snapshot directory as
two private arguments; normal CTest uses synthetic RAM only.

These are original-byte replay versus C checkpoints. Direct
same-run native +2968/+2981 capture remains queued with pD. The
separate fourth resident return's 18-byte IRQ-like residue remains
unattributed and is not masked or modeled here.
