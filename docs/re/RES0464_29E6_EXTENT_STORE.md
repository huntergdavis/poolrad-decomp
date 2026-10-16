# RES0464 +29E6 extent store: supplied successor

This separate source gate starts with the supplied synthetic successor of the
merged `+29D7..+29E4` extent comparison. The input CPU is `0C88:29E6
AX=138F BX=3E4E CX=0000 DX=14A0 SI=3E68 DI=0076 BP=3E80 SP=3E5C
DS=14A0 ES=3DE4 SS=1D16 FLAGS=7202`. It requires local `SS:3E6A=1390`,
row index `SS:3E62=003B`, stored extent `DS:4DE6=0000`, counter
`DS:70C7=0B`, and the previous far-call stack residue and five arguments.

The 13 loaded bytes at `0C88:29E6..+29F2` decode as `MOV AX,[BP-16]`,
`DEC AX`, `MOV DI,[BP-1E]`, `SHL DI,1`, `MOV DS:[DI+4D70],AX`.
For this supplied frame the only RAM delta is `DS:4DE6..4DE7 = 8F 13`,
with `IP=29F3`, `AX=138F`, and `DI=0076`. The 8086 leaves AF undefined
after SHL. The model retains the supplied AF as a host convention; it does
not claim exact native post-SHL flags or a natural run015 fourth-call stop.

The focused synthetic 640 KiB test asserts the one word delta, exact other
RAM bytes, unaffected CPU fields, and rejected inputs with no effects. It
does not execute prior calls or continue at +29F3. Source evidence is the
reviewed local loaded-byte sequence and merged +29D7 branch guard; the
cross-session `deja` index was inconclusive while stalled.
