# Fourth-call resident +275B branch gate

The private run015 `DEBUGBOX START` fourth call reached loaded resident
`0C88:275B` before `JL`. Its measured CPU tuple, ordered as
`CS IP AX BX CX DX SI DI BP SP DS ES SS FLAGS`, was:

```text
0C88 275B 00A8 0000 0068 0000 2612 0008 3E80 3E5C 14A0 3DE4 1D16 7246
```

The loaded eight bytes at `+275B..+2762` were
`7C 06 7F 1E 3B C1 73 1A`: `JL`, `JG`, `CMP AX,CX`, and `JAE`.
One bounded stop per instruction measured `JL` fallthrough `+275D`,
`JG` fallthrough `+275F`, pre-`JAE` `+2761`, then taken target `+277D`.
All four successors retained the input registers, except `IP`; the
comparison changed `FLAGS` from `7246` to `7202` at `+2761` and `+277D`.
Each of the five full 640 KiB RAM images had SHA-256
`21efc0155f250c31578643ac2d3f5030379fdedf92a155367ffc221b05ce1867`.

`pr_r464_fourth_branch_275b` models only this branch combination. Its
optional private test compares every full RAM image and CPU tuple from the
same run015 path. The portable test checks unsupported inputs leave CPU and
RAM unchanged. This gate does not establish the later `+28D9`, `+291C`,
`+2921`, `+2968`, or caller-return state for run015; run014 images and
original-byte replay images are separate baselines.
