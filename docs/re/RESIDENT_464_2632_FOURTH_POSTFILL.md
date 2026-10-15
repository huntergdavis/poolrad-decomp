# Fourth saved-view resident continuation after FillChar

## Scope and prior work

This bounded C model starts at `0C88:+2921` after the fourth resident
call's runtime FillChar return and stops **before** the first global
write at `0C88:+2968`. It covers the four local mask checks and loop
setup on the saved path. It does not execute the +2968 write or the
remaining resident caller, and makes no full-call or strict-count claim.

Before decoding, `deja "Poolrad fourth RES_0464 2632 after FillChar
2921 2926 saved run014 postcall next effect"` was run; the bounded
35-second query timed out. The reviewed +2921 RAM/CPU prestate was
reused from
[RESIDENT_464_2632_FOURTH_FILLCHAR.md](RESIDENT_464_2632_FOURTH_FILLCHAR.md)
and its original-byte replay. The prior first-call architecture in
[RESIDENT_464_2632_RUN014_COMPOSED.md](RESIDENT_464_2632_RUN014_COMPOSED.md)
was used to identify the runtime return boundary; this fourth path
has its own hash-locked replay and full-state comparison.

## Address and route evidence

The ignored START.EXE SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
The 71-byte file `0x6F9A..0x6FE1` span maps to loaded
`0C88:+2921..+2968`; both file and loaded SHA-256 are
`9b6905f9e3845793b2f958ef8dfb80ab09ff3d81a4da08613a5f1daecea195f9`.
The saved route executes 57 instructions and reaches 58 instruction
checkpoints. Its four indexed mask outcomes are miss, hit, hit, miss.
The two hits set local bytes at `SS:3E65` and `SS:3E66`; the loop
also updates its index and two local words. At the +2968 stop, four
physical RAM bytes differ from the input: `0x20FBD`, `0x20FC5`,
`0x20FC6`, and `0x20FC8`. All four are stack-frame bytes. The next
instruction would write `DS:[0x4A28 + DI]`, with saved `DI=0x003E`;
that effect is outside this gate.

The ordered CPU tuple `CS IP AX BX CX DX SI DI BP SP DS ES SS FLAGS`
at entry is:

    0C88 2921 0000 3E50 0000 0000 2612 3E68 3E80 3E5C 14A0 1D16 1D16 7202

At the first global-write boundary it is:

    0C88 2968 0000 3E50 0000 0000 2612 003E 3E80 3E5C 14A0 1D16 1D16 7297

Input RAM SHA-256 is
`4b02a5589727a60b13ab8e62ef21904c5257f9583015f7e8b902d7292f5dcc8c`;
boundary RAM SHA-256 is
`3f30630f17e536b66af4e0b8e1861e5924407598bdd1721338e6d28027f9c310`.
The C model matches **all 640 KiB RAM and all 14 CPU fields at all
58 original-byte checkpoints**. Synthetic tests cover no hits, the
saved mixed path, four hits, no write at the global boundary, invalid
host input, and unsupported entry without mutation.

## Reproduction and limit

With `PYTHONPATH=/tmp/poolrad-unicorn`, run
`tools/replay/resident_464_fourth_postfill_unicorn.py` on the ignored
+2921 RAM snapshot. Optional `--snapshot-dir` stays beneath ignored
`resources/` and writes the 58 private RAM images and CPU manifest.
The focused C test takes that pre-RAM path and snapshot directory as
two private arguments; normal CTest uses synthetic RAM only.

This is original-byte replay versus C evidence. Direct same-run
native +2921/+2968 capture remains pending with pD. The fourth
resident return's separate 18-byte IRQ-like residue remains
unattributed and is not masked or modeled here.
