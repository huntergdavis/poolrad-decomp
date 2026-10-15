# Fourth saved-view resident +2981 global word writes

## Scope and prior work

This bounded C model starts at `0C88:+2981`, executes two DS word
writes, and stops after `+29A4 JG` at its first successor. The saved
path falls through to `+29A6`; a synthetic greater case reaches
`+29D7`. Neither successor body is executed. This is not a complete
resident routine and carries no strict-count claim.

Before decoding, `deja "Poolrad fourth RES_0464 2632 2981 4AD0
4C20 saved successor global writer branch"` was run; the bounded
35-second query timed out. The reviewed +2981 full-state preimage and
branch fallthrough were reused from
[RESIDENT_464_2632_FOURTH_DS_WRITE.md](RESIDENT_464_2632_FOURTH_DS_WRITE.md).
The new two-writer path has its own hash-locked original-byte replay.

## Address and instruction evidence

The ignored START.EXE SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
The 37-byte file `0x6FFA..0x701F` span maps to loaded
`0C88:+2981..+29A6`; both file and loaded SHA-256 are
`946580045b636749790c945d8455f85f3aa7f1924688ea4ba19b341d3b7fdc8e`.
The saved route executes 13 instructions and has 14 instruction
checkpoints. It loads two stack words and writes them to
`DS:4B4C` and `DS:4C9C`, using the doubled local index. It then
copies one stack byte to another local, compares that byte with the
immediate byte in AL, and reaches `+29A6` because `+29A4 JG` is
false. Between input and fallthrough, only physical DS addresses
`0x1954C`, `0x1969C`, and `0x1969D` differ. The local byte write
restores its prior value on this saved preimage.

The ordered CPU tuple `CS IP AX BX CX DX SI DI BP SP DS ES SS FLAGS`
at entry is:

    0C88 2981 FFFF 3E50 0000 0000 2612 007C 3E80 3E5C 14A0 1D16 1D16 7286

At the saved fallthrough it is:

    0C88 29A6 1401 3E50 0000 0000 2612 007C 3E80 3E5C 14A0 1D16 1D16 7297

Input RAM SHA-256 is
`5f4367fda50ad7fcefa0cde9cd7e9d9b488125b1171e80ab0d21f85909d645ff`;
fallthrough RAM SHA-256 is
`0155b5aa519bacdffa71e1a20ae0326f77bf37754265ab100e04c6f25e4acd0f`.
The C model matches **all 640 KiB RAM and all 14 CPU fields at all
14 original-byte checkpoints**. Synthetic tests cover both word
writes, the saved fallthrough, a taken JG, invalid host input, and
unsupported entry without mutation.

## Reproduction and limit

With `PYTHONPATH=/tmp/poolrad-unicorn`, run
`tools/replay/resident_464_fourth_2981_unicorn.py` on the ignored
+2981 RAM snapshot. Optional `--snapshot-dir` stays beneath ignored
`resources/` and writes 14 private RAM images and a CPU manifest.
The focused C test accepts that pre-RAM path and snapshot directory
as two private arguments; normal CTest uses synthetic RAM only.

These are original-byte replay versus C checkpoints. Direct
same-run native +2981/+29A6 capture remains pending with pD. The
run015 first resident call is a separate native path and is not
cross-paired with this run014 fourth-call preimage. The fourth
resident return's 18-byte IRQ-like residue remains unattributed.
