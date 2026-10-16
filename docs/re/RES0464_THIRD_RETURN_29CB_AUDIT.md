# RES0464 third Move return at +29CB: supplied-frame audit

This standalone read-only validator accepts the supplied third Move return
frame. It does not execute Move, derive the destination bytes, advance the
resident caller, or establish a natural run015 third-call stop.

The accepted CPU tuple is `0C88:29CB AX=0004 BX=3E4E CX=0000 DX=14A0
SI=3E68 DI=148B BP=3E80 SP=3E5C DS=14A0 ES=3DE4 SS=1D16 FLAGS=7216`.
The five stack arguments at `SS:3E52..3E5A` are `0004 1487 3DE4 3E64
1D16`; return residue is `29CB:0C88` at `SS:3E4E`; the row counter is
`DS:70C7=03` and local `SS:3E6A=1468`. The source mask and supplied
destination masks at `ES:147F,1483,1487` are each `00 FF FF 00`.

The guard verifies the loaded far CALL at `0C88:29C6` and all 35 bytes of
the resident Move routine at `131C:1685`, ending in `RETF 10`. The focused
test constructs a synthetic 640 KiB image, checks acceptance and malformed
frame rejection, and asserts byte-for-byte CPU/RAM preservation. This is a
source and supplied-state gate, not native replay parity. Earlier local
unpublished `codex/r464-sixth-29c6-call-local` source and its supplied frame
were reviewed for these constants; the cross-session `deja` index was
inconclusive while stalled, so no indexed-session claim is used.
