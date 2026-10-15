# Fourth saved-view resident +29A6 call setup

## Scope and prior evidence

This bounded C model starts at `0C88:+29A6`, follows the saved
successor of `+29A4 JG`, and stops before the relocated runtime Move
call at `+29C6`. It does not execute that call or the alternate
`+29AB` increment arm. It is not a complete resident routine.

Before decoding, `deja "Poolrad fourth RES_0464 2632 29A6 70C7
saved successor branch"` was run with a 35-second bound and timed out.
The reviewed +2981 branch fallthrough CPU and RAM preimage were
reused from [RESIDENT_464_2632_FOURTH_2981.md](RESIDENT_464_2632_FOURTH_2981.md).

## Exact addresses and effects

The ignored START.EXE is SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
The 32-byte file span `0x701F..0x703F` maps to loaded
`0C88:+29A6..+29C6`. File and loaded span SHA-256 are both
`4b87d4d848d0c54b731f76026acfc65220a0b7cdabe7c690e028dc99393b33a1`.

The saved path executes 13 instructions and has 14 instruction
checkpoints. It writes AL to `DS:70C7`, skips `+29AB`, forms a
stack-local source pointer, loads the destination far pointer from
`SS:[BP+6]`, adds the local offset and `0x17`, and pushes the
arguments for a four-byte Move. The call has not run at the stop.
The input RAM SHA-256 is
`0155b5aa519bacdffa71e1a20ae0326f77bf37754265ab100e04c6f25e4acd0f`;
the call-entry RAM SHA-256 is
`eb0bcc2d94e3fe2291a0ceabf577f330dfa10e49bb87f9c9f430b98ce967b47d`.
Seven RAM bytes differ.

Ordered CPU tuple `CS IP AX BX CX DX SI DI BP SP DS ES SS FLAGS`:

    input    0C88 29A6 1401 3E50 0000 0000 2612 007C 3E80 3E5C 14A0 1D16 1D16 7297
    boundary 0C88 29C6 0004 3E50 0000 0000 2612 147F 3E80 3E52 14A0 3DE4 1D16 7202

The model is checked against original-byte Unicorn replay for all
640 KiB RAM and all 14 CPU fields at every checkpoint. A synthetic
test covers the global byte, pointer arithmetic, five pushed words,
and guarded entry. With `PYTHONPATH=/tmp/poolrad-unicorn`, run
`tools/replay/resident_464_fourth_29a6_unicorn.py` on the ignored
`fourth-2981-stops/ram-13.bin` preimage. Optional snapshots stay
beneath ignored `resources/`.

This is original-byte replay parity for the saved fourth-call path.
Direct same-run native fourth-call capture and the 18-byte
IRQ-like return residue remain unresolved; the separate run015
first-call native state is not paired with this preimage.
